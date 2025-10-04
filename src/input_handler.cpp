#include <functional>
#include <cassert>
#include <regex>
#include <fstream>
#include "json.hpp"
#include "pch.hpp"
#include "input_handler.hpp"
#include "argparser.hpp"
#include "simulation_info.hpp"
#include "drug_info.hpp"
#include "arg_constants.hpp"
#include "time_utils.hpp"
#include "convert_utils.hpp"
#include "pk_utils.hpp"

using std::stoi;
using std::stof;
using std::string;
using json = nlohmann::json;
using namespace PK;
namespace Dose = UnitConverter::Dose;

std::regex reConfigName{"^[a-z0-9]+?\\.json$", std::regex::icase};

void checkBadArgs(const ArgParser&, const SimulationInfo&);
void checkConfig(ArgParser&, SimulationInfo&);

struct HandleHelper {
    Args::Metadata arg;
    string label; // NOTE: if empty there will be no prompt
    std::function<void(string val)> handler;
    bool skipIfOneComp = false;
};

void handleInput(ArgParser& parser, SimulationInfo& info)
{
    checkConfig(parser, info);

    std::string line;
    DrugInfo& drug = info.drugInfo;
    bool isTwoCompModel = info.compModel == TWO_COMP_MODEL;

    drug.isDr = parser.isArgUsed(Args::DR) || parser.isArgUsed(Args::DR_FRAC);
    drug.isProdrug = parser.isArgUsed(Args::PRODRUG) ||
                     parser.isArgUsed(Args::T12M);
    info.isMaxStatEnabled = parser.isArgUsed(Args::MAX);
    info.isAucEnabled = parser.isArgUsed(Args::AUC);

    auto labelIfProdrug = [&](string str) { return drug.isProdrug ? str : ""; };
    auto labelIfDr = [&](string str) { return drug.isDr ? str : ""; };

    checkBadArgs(parser, info);

    /* List of helpers for specific args. */
    std::vector<HandleHelper> helper =
    {
        {
            Args::ROA, "", [&](string val) {
                for (const auto& it : roaToStringMap) {
                    if (vectorContains(it.second, val)) { drug.roa = it.first; break; }
                }
                info.compModel = roaToCompModelMap.at(drug.roa);
                isTwoCompModel = info.compModel == TWO_COMP_MODEL;
            }
        },

        {
            Args::MSG, "", [&](string val) {
                string s{};
                if (parser.isArgUsed(Args::COUNT)) {
                    s = parser.getArg(Args::COUNT).value.value() + "x ";
                }
                s += val;

                info.msg = s;
            }
        },

        {
            Args::PRECISION, "", [&](string val) {
                info.precision = std::clamp(stoi(val), 0, 15);
                info.state.prec = info.precision;
            }
        },

        {
            Args::SIGFIGS, "", [&](string val) {
                info.sigfigs = std::clamp(stoi(val), 1, 6);
            }
        },

        {
            Args::VOLUME, "", [&](string val) {
                setFractionsToDecimal(val);
                drug.vd = stof(val);
                info.baseUnitsEnabled = true;
            }
        },

        {
            Args::DOSE, "dose: ", [&](string val)
            {
                auto inp = parseDoseInput(val);

                if (inp.useBaseUnit && info.baseUnitsEnabled) {
                    double frac = Dose::toMgPerLiterFactor(inp.doseUnit, inp.baseUnit);

                    //NOTE: Convert dose to mg/L, then to total mg by multiplying by vd.
                    //Calculating concentrations take into account volume of distribution.
                    drug.dose = inp.value * frac;
                    drug.dose *= drug.vd;

                    info.state.doseUnit = inp.doseUnit;
                    info.state.baseUnit = inp.baseUnit;
                    info.doseUnitsEnabled = true;
                    info.baseUnitsEnabled = true;

                    return;
                }

                drug.dose = inp.value * Dose::toDefaultFactor(inp.doseUnit);
                info.state.doseUnit = inp.doseUnit;

                if (!info.doseUnitsEnabled && inp.useDoseUnit) {
                    info.doseUnitsEnabled = true;
                    info.isDoseUnitVolume = isDoseUnitVolume(inp.doseUnit);
                }

                if (!info.baseUnitsEnabled && inp.useBaseUnit) {
                    info.baseUnitsEnabled = true;
                    info.state.baseUnit = inp.baseUnit;
                }
            }
        },

        {
            Args::ED50, "", [&](string val) {
                auto inp = parseDoseInput(val);

                info.ed50Enabled = true;

                if (info.baseUnitsEnabled && !inp.useBaseUnit && inp.useDoseUnit) {
                    drug.ed50 = inp.value / drug.vd;

                    return;
                }

                info.drugInfo.ed50 = inp.value * Dose::toDefaultFactor(inp.doseUnit);
            }
        },

        {
            Args::COUNT, "", [&](string val){
                setFractionsToDecimal(val);
                setPercentagesToDecimal(val);
                drug.dose *= stof(val);
            }
        },

        {
            Args::BIOAVAILABILITY, "bioavailability: ", [&](string val) {
                setPercentagesToDecimal(val);
                setFractionsToDecimal(val);
                drug.bioavailability = stof(val);
            },
            true
        },

        {
            Args::T12ABS, "absorption half-life: ",
            [&](string val) {
                if (!isTwoCompModel) return;
                double t = timeInputToSeconds(val);
                drug.ka = convertRateConstant(t);
            },
            true
        },

        {
            Args::T12, "half-life: ", [&](string val) {
                double t = timeInputToSeconds(val);
                drug.ke = convertRateConstant(t);
            }
        },

        {
            Args::PRODRUG, labelIfProdrug("active drug factor: "), [&](string val) {
                setFractionsToDecimal(val);
                setPercentagesToDecimal(val);
                drug.activeFrac = stod(val);
            }
        },

        {
            Args::EXCRETION, "", [&](string val) {
                setFractionsToDecimal(val);
                setPercentagesToDecimal(val);
                drug.excretionFrac = stof(val);
                info.displayExcreted = true;
            }
        },

        {
            Args::T12M, labelIfProdrug("active drug half-life: "),
            [&](string val) {
                double t = timeInputToSeconds(val);
                drug.activeKe = convertRateConstant(t);
            }
        },

        {
            Args::DR_FRAC, labelIfDr("delayed release fraction (def. 0.5): "),
                [&](string val) {
                    assert(info.compModel == TWO_COMP_MODEL);
                    if (val.empty()) { drug.drFrac = 0.5; return; }
                    setFractionsToDecimal(val);
                    drug.drFrac = stod(val);
            }
        },

        {
            Args::DR, labelIfDr("time until delayed dose: "), [&](string val) {
                assert(info.compModel == TWO_COMP_MODEL);
                drug.drLagtime = timeInputToSeconds(val);
            }
        },

        {
            Args::LAGTIME, "", [&](string val) {
                drug.lagtime = timeInputToSeconds(val);
            }
        },

        {
            Args::MIN, "", [&](string val) {
                auto inp = parseDoseInput(val);

                info.minDoseAllowed = inp.value * Dose::toDefaultFactor(inp.doseUnit);

                if (!info.baseUnitsEnabled) {
                    return;
                }
                else if (inp.useDoseUnit && !inp.useBaseUnit) {
                    info.minDoseAllowed /= drug.vd;
                }
                else if (inp.useBaseUnit) {
                    double frac = UnitConverter::Base::toLitersFactor(inp.baseUnit);
                    info.minDoseAllowed /= frac;
                }
            }
        },

        {
            Args::TIME, "", [&](string val) {
                auto t = getTimeEpoch(val);
                info.epoch = t;

                // Enable 12 hour format.
                if (std::tolower(val.back()) == 'm') {
                    info.is12HrFormat = true;
                }
            }
        },

        {
            Args::DATE, "", [&](string val) {
                info.epoch = getDateEpoch(val);

                // Enable 12 hour format.
                if (std::tolower(val.back()) == 'm') {
                    info.is12HrFormat = true;
                }
            }
        },

        {
            Args::ELAPSED, "", [&](string val) {
                if (std::all_of(val.begin(), val.end(), ::isdigit)) {
                    info.epoch = getEpoch() - hhmmToSeconds(val);
                    return;
                }

                auto s = std::chrono::duration<double>(timeInputToSeconds(val));
                info.epoch = getEpoch() - s;
            }
        },
    };

    // Loop through each arg function in helper.
    for (const auto& it : helper)
    {
        if (parser.isArgUsed(it.arg)) {
            it.handler(parser.getArgByFlag(it.arg.flag).value.value());
        }
        else if (!it.label.empty()) {
            if (info.compModel == ONE_COMP_MODEL && it.skipIfOneComp) continue;
            std::cout << it.label;
            std::getline(std::cin, line);
            it.handler(line);
        }
    }
}

void checkBadArgs(const ArgParser& parser, const SimulationInfo& sim)
{
    const auto& drug = sim.drugInfo;

    if (drug.isDr && !parser.isArgUsed(Args::ROA)) {
        throw std::logic_error("delayed release must be a two compartment model");
    }
}

/* Set arg values and simulation values depending on config. */
void checkConfig(ArgParser& parser, SimulationInfo& sim)
{
    if (!parser.isArgUsed(Args::ARG_FILE))
        return;

    std::smatch match;

    string path = parser.getArg(Args::ARG_FILE).value.value();
    path += ".json";

    bool safe = std::regex_search(path, match, reConfigName);
    if (!safe) {
        throw std::invalid_argument("file name cannot be used: " + path);
    }
    assert(safe);

    // Throw error if file does not exist.
    if (!std::filesystem::exists(path)) {
        throw std::invalid_argument("file does not exist");
    }

    std::ifstream ifs(path.c_str());
    json config = json::parse(ifs);

    // Check config and set args based on file values.
    for (const auto& it : config.items())
    {
        for (const auto& itConf : configArgs)
        {
            if (itConf.second != it.key())
                continue;
            else if (!parser.isArgUsed(*itConf.first)) {
                parser.getArg(*itConf.first).value = it.value().get<string>();
                break;
            }
        }
    }
}
