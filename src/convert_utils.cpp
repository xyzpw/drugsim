#include <regex>
#include <cassert>
#include "pch.hpp"
#include "convert_utils.hpp"
#include "common.hpp"

using std::stof;
using std::stod;
using std::floor;
using std::fmod;
using std::string;
using namespace UnitConverter;

std::pair<double, std::optional<std::string>> parseNumberUnitInput(std::string);

const std::regex NUMBER_UNIT_INPUT_REGEX{"^(\\d*?\\.?\\d+?)(?:\\s?([a-zA-Z/]+?))?$"};
const std::regex reDoseInput{
    "^((?:\\d*?\\.)?\\d+)(?:\\s*?([a-z]+)(?:/?([a-z]+)?))?$",
    std::regex::icase
};

// Detects n/d numbers.
const std::regex numberFracRe{"((?:\\d*?\\.)?\\d+)/((?:\\d*?\\.)?\\d+)"};

/*
 * Return multiplier to convert specified unit to default dose unit,
 * e.g. how many (default units) per specified unit.
 *
 * @note default dose units are mg for mass and mL for volume
*/
double Dose::toDefaultFactor(const DOSE_UNIT& unit)
{
    switch (unit) {
        case DOSE_UNIT_NANOGRAM:
            return 1e-6;
        case DOSE_UNIT_MICROGRAM:
            return 1e-3;
        case DOSE_UNIT_GRAM:
        case DOSE_UNIT_L:
            return 1e+3;
        default:
            return 1;
    }
}

double Dose::toMgPerLiterFactor(const DOSE_UNIT& unit, const BASE_UNIT& base)
{
    return toDefaultFactor(unit) / Base::toLitersFactor(base);
}

double Base::toLitersFactor(const BASE_UNIT& base)
{
    switch (base) {
        case BASE_UNIT_ML:
            return 1e-3;
        default:
            return 1;
    }
}

/* Return multiplier to convert specified unit to seconds. */
double Time::toSecondsFactor(const TIME_UNIT& unit)
{
    switch (unit) {
        case TIME_UNIT_MS:
            return 0.001;
        case TIME_UNIT_MINUTE:
            return 60;
        case TIME_UNIT_HOUR:
            return 3600;
        case TIME_UNIT_DAY:
            return 86400;
        default:
            return 1;
    }
}

/* Time and unit input string to seconds, e.g. "1 h" = 3600.0 */
double timeInputToSeconds(std::string text)
{
    auto input = parseNumberUnitInput(text);

    double sec = input.first;

    if (input.second.has_value()) {
        TIME_UNIT unit = stringToUnit<TIME_UNIT>(*input.second);
        sec *= Time::toSecondsFactor(unit);
    }

    return sec;
}

/* Return parsed dose input. */
ParsedDose parseDoseInput(std::string text)
{
    ParsedDose result;

    // Replace fractions with values.
    setFractionsToDecimal(text);

    std::smatch match;

    if (!std::regex_search(text, match, reDoseInput)) {
        throw std::invalid_argument("invalid dose input");
    }

    result.value = stod(match[1].str());

    // Set dose unit.
    if (match[2].matched) {
        result.doseUnit = stringToUnit<DOSE_UNIT>(match[2].str());
        result.useDoseUnit = true;
    }

    // Set denominator unit.
    if (match[3].matched) {
        result.baseUnit = stringToUnit<BASE_UNIT>(match[3].str());
        result.useBaseUnit = true;
    }

    return result;
}

/*
 * Return pair <number, unit string> based on input.
*/
std::pair<double, std::optional<std::string>> parseNumberUnitInput(std::string text)
{
    std::smatch match;

    setFractionsToDecimal(text);

    if (std::regex_search(text, match, NUMBER_UNIT_INPUT_REGEX)) {
        double num = stod(match[1].str());
        if (!match[2].matched)
            return std::make_pair(num, std::nullopt);
        return std::make_pair(num, match[2].str());
    }

    throw std::runtime_error("invalid number input");
}

/*
 * Changes all fractions to their values, e.g. 1/2 => 0.5
*/
void setFractionsToDecimal(string& text)
{
    std::vector<std::smatch> matches;

    for (std::sregex_iterator it(text.begin(), text.end(), numberFracRe), end;
         it != end; ++it)
    {
        matches.push_back(*it);
    }

    for (auto mIt = matches.rbegin(); mIt != matches.rend(); ++mIt) {
        const std::smatch& m = *mIt;

        double num = stod(m[1].str());
        double den = stod(m[2].str());
        double val = num / den;

        text.replace(m.position(), m.length(), std::to_string(val));
    }
}

/*
 * Change percentage string to decimal string, e.g. "50%" becomes "0.5"
*/
void setPercentagesToDecimal(string& text)
{
    std::vector<std::smatch> matches;
    std::regex re{R"(((?:\d*\.)?\d+)%)"};

    for (std::sregex_iterator it(text.begin(), text.end(), re), end;
         it != end; ++it)
    {
        matches.push_back(*it);
    }

    for (auto it = matches.rbegin(); it != matches.rend(); ++it) {
        const std::smatch& m = *it;

        float val = stof(m[1].str()) * 0.01f;

        std::ostringstream oss;
        oss << std::setprecision(6) << std::noshowpoint << val;

        text.replace(m.position(), m.length(), oss.str());
    }
}

string formatSigFigs(const double& value, const int& sigfigs)
{
    if (value == 0.0) {
        return std::format("{:.{}f}", 0.0, sigfigs - 1);
    }

    int exp = static_cast<int>(std::floor(std::log10(std::fabs(value))));
    double scale = std::pow(10, exp - sigfigs + 1);
    double rounded = std::round(value / scale) * scale;
    int prec = std::max(sigfigs - exp - 1, 0);

    return std::format("{:.{}f}", rounded, prec);
}

/*
 * Convert seconds to readable time.
*/
std::string formatSeconds(float s)
{
    if (s < 1) {
        return std::format("{} ms", floor(s * 1e+3f));
    }

    std::stringstream stream;

    int clockHours = s / 3600;
    int clockMinutes = fmod(s, 3600) / 60;
    int clockSeconds = fmod(s, 60);

    /*
     * @param n: int variable containing time value
     * @param unit: time unit label to be made plural if needed
     * @param com: should add ", " to end of string?
    */
    auto appendFn = [&](const int &n, std::string unit, bool com=false) {
        if (n != 1) unit.append("s");
        if (com) unit.append(", ");
        stream << n << unit;
    };

    if (clockHours > 0) {
        appendFn(clockHours, " hour", true);
    }
    if (s >= 60) {
        appendFn(clockMinutes, " minute", true);
    }
    appendFn(clockSeconds, " second");

    return stream.str();
}

/* Return true if the specified dose unit is a volume unit. */
bool isDoseUnitVolume(const DOSE_UNIT& unit)
{
    if (unit == DOSE_UNIT_ML || unit == DOSE_UNIT_L) {
        return true;
    }
    return false;
}
