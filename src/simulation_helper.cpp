#include <cassert>
#include <string_view>
#include "simulation_helper.hpp"
#include "pch.hpp"
#include "common.hpp"
#include "pk_utils.hpp"
#include "convert_utils.hpp"
#include "time_utils.hpp"

using std::string;

using namespace PK;
namespace OneComp = PK::OneCompartment;
namespace TwoComp = PK::TwoCompartment;
namespace Convert = UnitConverter;

const double EPSILON_MULT = 1.00001;

/* Validate everything is set up properly. */
void SimHelper::validateInit(SimulationInfo& sim)
{
    using namespace UnitConverter;

    auto& state = sim.state;
    auto& cache = sim.cache;
    auto& drug = sim.drugInfo;

    /* Reserve cache string sizes */
    cache.output.reserve(128);
    cache.altOutput.reserve(128);
    cache.doseUnitStr.reserve(3);
    cache.baseUnitStr.reserve(3);
    cache.fullDoseUnitStr.reserve(7);

    /* Validate cache strings */
    cache.doseUnitStr = sim.doseUnitsEnabled ?
                        Convert::unitToString(state.doseUnit) : "unit";
    if (sim.baseUnitsEnabled) {
        cache.baseUnitStr = Convert::unitToString<BASE_UNIT>(state.baseUnit);
    }
    cache.updateFullDoseUnitStr();

    /* Flip absorption/elimination constants if flip-flop effect occurs. */
    if (drug.ka > 0 && drug.ka < drug.ke) {
        double newKa = drug.ke;
        double newKe = drug.ka;

        drug.ka = newKa;
        drug.ke = newKe;
    }

    if (drug.isProdrug) {
        state.isMultiline = true;

        /* Ensure no rate constants are equal; prevent zero division. */
        double& ka = drug.ka;
        double& ke = drug.ke;
        double& km = drug.activeKe.value();
        if (ka == ke && ke == km) {
            ka *= EPSILON_MULT;
            ke *= EPSILON_MULT * EPSILON_MULT;
        } else {
            if (ka == ke) {
                ke *= EPSILON_MULT;
            }
            if (ka == km) {
                km *= EPSILON_MULT;
            }
            if (ke == km) {
                km *= EPSILON_MULT;
            }
        }
    }

    /* Do not start as peak if not intravenous */
    if (drug.roa != ROA_TYPE_IV) {
        state.hasTmaxed = false;
        state.fullyAbsorbed = false;
        drug.tmax = TwoComp::computeTmax(drug);
    }

    if (sim.precision > 0) {
        state.minDisplayDose = getMinDisplayDose(sim.precision);
    }

    if (!sim.epoch.count()) {
        sim.epoch = getEpoch();
    }
    // Add lagtime if the specified start time is ahead of current time.
    else if (sim.epoch > getEpoch()) {
        auto epoch = getEpoch();
        drug.lagtime = (sim.epoch - epoch).count();
        sim.epoch = epoch;
    }
}

double SimHelper::getMinDisplayDose(int prec)
{
    //NOTE: Multiply by 0.5 for rounding.
    return pow(10.0, -prec) * 0.5;
}

/*
 * Updates all drug and dose unit info for each simulation tick.
*/
void SimHelper::updateCurrentDoses(SimulationInfo& sim)
{
    const auto& drug = sim.drugInfo;
    auto& state = sim.state;
    double& elapsed = state.elapsed;
    double& drugContent = state.drugContent;
    auto& activeDrugContent = state.activeDrugContent;
    double defUnitFactor = 1.0 / Convert::Dose::toDefaultFactor(state.doseUnit);

    drugContent = computeDrugContent(sim, elapsed);
    state.doseAsUnit = drugContent * defUnitFactor;

    if (drug.isProdrug) {
        if (sim.compModel == ONE_COMP_MODEL)
            activeDrugContent = OneComp::computeMetaboliteContent(drug, elapsed);
        else if (sim.compModel == TWO_COMP_MODEL)
            activeDrugContent = TwoComp::computeMetaboliteContent(drug, elapsed);
        state.activeDoseAsUnit = *activeDrugContent * defUnitFactor;
    }

    // Updated effectiveness.
    if (sim.ed50Enabled) {
        const double& dose = (drug.isProdrug) ? *activeDrugContent : drugContent;
        state.effectiveness = computeEffectiveness(drug.ed50, dose);
    }

    // Update excreted.
    if (sim.displayExcreted) {
        switch (sim.compModel) {
            case ONE_COMP_MODEL:
                if (drug.isProdrug) {
                    state.excreted = OneComp::computeMetaboliteExcreted(drug, elapsed);
                } else {
                    state.excreted = OneComp::computeExcreted(drug, elapsed);
                }
                break;
            case TWO_COMP_MODEL:
                if (drug.isProdrug) {
                    state.excreted = TwoComp::computeMetaboliteExcreted(drug, elapsed);
                } else {
                    state.excreted = TwoComp::computeExcreted(drug, elapsed);
                }
                break;
        }
    }

    // Nothing more to do if auc is not enabled.
    if (!sim.isAucEnabled)
        return;

    // Compute area under curve.
    switch (sim.compModel) {
        case ONE_COMP_MODEL:
            if (!drug.isProdrug) {
                state.auc = OneComp::computeAuc(drug, drug.dose, elapsed);
            } else {
                state.auc = OneComp::computeMetaboliteAuc(drug, elapsed);
            }
            break;
        case TWO_COMP_MODEL:
            if (drug.isProdrug) {
                state.auc = TwoComp::computeAucMetabolite(drug, elapsed);
            } else if (drug.isDr) {
                state.auc = TwoComp::computeAucDr(drug, drug.dose, elapsed);
            } else {
                state.auc = TwoComp::computeAuc(drug, drug.dose, elapsed);
            }
            break;
    }
}

void SimHelper::checkMaxAchieved(SimulationInfo& sim)
{
    const auto& drug = sim.drugInfo;
    auto& state = sim.state;
    double& max = state.maxAchieved;

    if (!drug.isProdrug && state.drugContent > max) {
        max = state.drugContent;
    }
    else if (drug.isProdrug && state.activeDrugContent.value() > max) {
        max = state.activeDrugContent.value();
    }
}

void SimHelper::checkTmaxState(SimulationInfo& sim)
{
    auto& drug = sim.drugInfo;
    auto& state = sim.state;

    const auto& elapsed = sim.state.elapsed;

    if (!state.hasTmaxed && elapsed >= drug.tmax) {
        state.hasTmaxed = true;
    }

    if (drug.isDr && !state.hasDrTmaxed &&
        elapsed >= drug.tmax + drug.drLagtime.value())
    {
        state.hasDrTmaxed = true;
    }
}

/* Adjust dose and unit to a fixed precision. */
void SimHelper::useFixedPrecision(SimulationInfo& sim)
{
    if (!sim.doseUnitsEnabled || sim.sigfigs.has_value())
        return;

    const auto& drug = sim.drugInfo;
    auto& state = sim.state;
    bool& hasTmaxed = state.hasTmaxed;
    int& statePrec = state.prec;
    DOSE_UNIT& unit = state.doseUnit;
    string& unitStr = sim.cache.doseUnitStr;
    double& doseAsUnit = drug.isProdrug ? state.activeDoseAsUnit.value() :
                                          state.doseAsUnit;

    // Return if prodrug dose is still being displayed.
    if (drug.isProdrug && state.drugContent >= state.minProdrugDisplayDose)
        return;

    if (unit == DOSE_UNIT_NANOGRAM && hasTmaxed)
        return;
    else if (statePrec < 3 || doseAsUnit >= 1)
        return;
    else if (!hasTmaxed) {
        if (unit != DOSE_UNIT_GRAM)
            return;
    }

    /* Update dose unit before adjusting dose. */
    switch (unit) {
        case DOSE_UNIT_MICROGRAM:
            unit = DOSE_UNIT_NANOGRAM;
            break;
        case DOSE_UNIT_MG:
            unit = DOSE_UNIT_MICROGRAM;
            break;
        case DOSE_UNIT_GRAM:
            unit = DOSE_UNIT_MG;
            break;
        case DOSE_UNIT_L:
            unit = DOSE_UNIT_ML;
            break;
        default:
            return;
    }

    doseAsUnit *= 1e+3;   // adjust dose as unit to match new unit
    statePrec -= 3;       // reduce precision to equal initial precision

    // Update unit string to cache.
    unitStr = UnitConverter::unitToString(unit);
    sim.cache.updateFullDoseUnitStr();

    // Adjust minimum display dose.
    state.minDisplayDose = getMinDisplayDose(statePrec);
}

void SimHelper::checkFullyAbsorbed(SimulationInfo& sim)
{
    if (sim.state.fullyAbsorbed || sim.compModel == ONE_COMP_MODEL)
        return;

    auto& drug = sim.drugInfo;

    sim.state.fullyAbsorbed = TwoComp::computeIsAbsorbed(drug, sim.state.elapsed);
}

/* Check if the dose is low enough to be considered complete. */
bool SimHelper::isMinDose(SimulationInfo& sim)
{
    auto& state = sim.state;
    double& minAllowed = sim.minDoseAllowed;
    double& minDisp = state.minDisplayDose;

    auto isMin = [&](double& dose, double& doseAsUnit) {
        return dose < minAllowed || doseAsUnit < minDisp;
    };

    if (!isMin(state.drugContent, state.doseAsUnit)) {
        return false;
    }
    else if (sim.drugInfo.isProdrug) {
        return isMin(*state.activeDrugContent, *state.activeDoseAsUnit);
    }

    return true;
}

/* Update simulations cached info for output. */
void SimHelper::updateCache(SimulationInfo& sim)
{
    auto& cache = sim.cache;
    auto& state = sim.state;
    auto& drug = sim.drugInfo;

    auto& out = cache.output;
    auto& altOut = cache.altOutput;

    const auto& statePrec = state.prec; // precision in simulation state

    const auto& dose = state.doseAsUnit; // dose as unit

    string& unitStr = cache.doseUnitStr;
    string& fullDoseUnitStr = cache.fullDoseUnitStr;
    const bool& unitsEnabled = sim.doseUnitsEnabled;

    double& minProdrugDisplayDose = state.minProdrugDisplayDose;

    // Label displayed adjacent to dose.
    const std::string_view* label = nullptr;

    /* Set dose unit string if it is empty */
    if (unitStr.empty()) {
        unitStr = Convert::unitToString<DOSE_UNIT>(state.doseUnit);
    }

    /* Append unit for dose to end of output string. */
    auto appendUnitFn = [&]() {
        if (!sim.doseUnitsEnabled) return;
        out += ' ';
        out += sim.baseUnitsEnabled ? fullDoseUnitStr : unitStr;
    };

    /* Append default unit to end of output string */
    auto appendDefUnitFn = [&](){
        if (!sim.doseUnitsEnabled) return;
        out += ' ';
        out += sim.baseUnitsEnabled ? MGL_STR : MG_STR;
    };

    /* Create label depending on drug (prodrug if used) phase. */
    auto drugLabelFn = [&]() {
        if (state.hasTmaxed && !state.fullyAbsorbed) {
            label = &elPhaseAbsorbingLabel;
            return;
        }
        label = state.hasTmaxed ? &eliminationPhaseLabel : &absorptionPhaseLabel;
    };

    /* Create string of value with given precision, use sigfigs instead of they are used. */
    auto fmtPrecStr = [&](const double& content, const int& prec) {
        if (sim.sigfigs.has_value()) {
            return formatSigFigs(content, *sim.sigfigs);
        }
        return std::format("{:.{}f}", content, prec);
    };

    /* Set minimum dose at which prodrug can be displayed */
    if (drug.isProdrug && minProdrugDisplayDose == -1) {
        minProdrugDisplayDose = getMinDisplayDose(sim.precision);
    }

    if (drug.isProdrug) {
        out = std::format(
                "active drug content: {}",
                fmtPrecStr(*state.activeDoseAsUnit, statePrec)
        );
        appendUnitFn();

        /* Prodrug multiline text */
        if (!state.fullyAbsorbed || state.drugContent >= minProdrugDisplayDose)
        {
            drugLabelFn();
            altOut = std::format(
                    "prodrug ({}): {}",
                    *label, fmtPrecStr(state.drugContent, sim.precision)
            );

            if (unitsEnabled) {
                altOut += ' ';
                altOut += (sim.isDoseUnitVolume) ? ML_STR : MG_STR;
            }
        }
        // Disable multiline when prodrug amount is too low to be displayed.
        else if (state.isMultiline) {
            state.isMultiline = false;
        }
    }
    else if (drug.isDr) {
        drugLabelFn();
        const auto* drLabel = state.hasDrTmaxed ? &eliminationPhaseLabel :
                                                  &absorptionPhaseLabel;
        if (!state.hasDrReleased) {
            drLabel = &lagPhaseLabel;
        }

        out = std::format(
            "drug content ({}[DR: {}]): {}",
            *label, *drLabel, fmtPrecStr(dose, statePrec)
        );
        appendUnitFn();
    }
    else {
        drugLabelFn();
        out = std::format("drug content ({}): {}", *label, fmtPrecStr(dose, statePrec));
        appendUnitFn();
    }

    // Display excreted.
    if (sim.displayExcreted) {
        out += std::format(" (excreted: {}", fmtPrecStr(state.excreted, sim.precision));
        appendDefUnitFn();
        out += ')';
    }

    // Display max dose achieved.
    if (sim.isMaxStatEnabled) {
        out += std::format(" (max achieved {}", fmtPrecStr(state.maxAchieved, sim.precision));
        appendDefUnitFn();
        out += ')';
    }

    // Display auc.
    if (sim.isAucEnabled) {
        out += std::format(" (AUC: {}", fmtPrecStr(state.auc, sim.precision));

        if (sim.doseUnitsEnabled) {
            appendDefUnitFn();
        }
        else {
            out += " unit";
        }

        out += "\u22C5h/L)";
    }

    // Display effectiveness.
    if (sim.ed50Enabled) {
        out += std::format(
            " (eff. {:.0f}%)",
            state.effectiveness * 100.0f
        );
    }
}
