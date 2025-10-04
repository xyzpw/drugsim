#pragma once

#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <stdexcept>
#include <boost/algorithm/string/predicate.hpp>

// Fraction of drug absorbed to be considered complete.
constexpr float ABSORBED_THRESHOLD = 0.98f;

/* Ansi codes */
const inline std::string ANSI_UP = "\x1b[1A";     // move cursor up
const inline std::string ANSI_DOWN = "\x1b[1B";   // move cursor down
const inline std::string ANSI_CLEAR = "\x1b[2K";  // clear line

enum DOSE_UNIT {
    DOSE_UNIT_MG,
    DOSE_UNIT_NANOGRAM,
    DOSE_UNIT_NANOMOLAR,
    DOSE_UNIT_MICROGRAM,
    DOSE_UNIT_MICROMOLAR,
    DOSE_UNIT_GRAM,
    DOSE_UNIT_ML,
    DOSE_UNIT_L,
};

enum BASE_UNIT {
    BASE_UNIT_L,
    BASE_UNIT_ML,
    BASE_UNIT_KG,
};

enum ROA_TYPE {
    ROA_TYPE_IV,
    ROA_TYPE_ORAL,
    ROA_TYPE_INHALATION,
    ROA_TYPE_INTRANASAL,
    ROA_TYPE_SL,
};

enum TIME_UNIT {
    TIME_UNIT_SECOND,
    TIME_UNIT_MS,
    TIME_UNIT_MINUTE,
    TIME_UNIT_HOUR,
    TIME_UNIT_DAY,
};

// Pharmacokinetics compartment model.
enum COMP_MODEL {
    ONE_COMP_MODEL,
    TWO_COMP_MODEL,
};

/* Check if vector contains a specific element.
 * @note: strings are case-insensitive.*/
template <typename T>
inline bool vectorContains(const std::vector<T>& v, const T& value)
{
    if constexpr (std::is_same_v<T, std::string>) {
        auto it = std::find_if(v.begin(), v.end(), [&](const std::string& s) {
                return boost::iequals(s, value);
        });

        return it != v.end();
    }

    return std::find(v.begin(), v.end(), value) != v.end();
}

inline const std::unordered_map<DOSE_UNIT, std::vector<std::string>>
doseUnitToStringMap
{
    {DOSE_UNIT_MG, {"mg", "milligram", "milligrams"}},
    {DOSE_UNIT_NANOGRAM, {"ng", "nanogram", "nanograms"}},
    {DOSE_UNIT_MICROGRAM, {"mcg", "ug", "microgram", "micrograms"}},
    {DOSE_UNIT_GRAM, {"g", "gram", "grams"}},
    {DOSE_UNIT_NANOMOLAR, {"nM", "nMol", "nanomolar", "nanomolars"}},
    {DOSE_UNIT_MICROMOLAR, {"uM", "uMol", "micromolar", "micromolars"}},
    {DOSE_UNIT_ML, {"mL", "milliliter", "milliliters"}},
    {DOSE_UNIT_L, {"L", "liter", "liters"}},
};

inline const std::unordered_map<BASE_UNIT, std::vector<std::string>>
volumeUnitToStringMap
{
    {BASE_UNIT_L, {"L", "liter", "liters"}},
    {BASE_UNIT_ML, {"mL", "milliliter", "milliliters"}},
    {BASE_UNIT_KG, {"kg", "kilogram", "kilograms"}},
};

inline const std::unordered_map<TIME_UNIT, std::vector<std::string>>
timeUnitToStringMap
{
    {TIME_UNIT_SECOND, {"s", "sec", "second", "seconds"}},
    {TIME_UNIT_MS, {"ms", "millisecond", "milliseconds"}},
    {TIME_UNIT_MINUTE, {"m", "min", "minute", "minutes"}},
    {TIME_UNIT_HOUR, {"h", "hr", "hrs", "hour", "hours"}},
    {TIME_UNIT_DAY, {"d", "day", "days"}},
};

inline const std::unordered_map<ROA_TYPE, std::vector<std::string>> roaToStringMap
{
    {ROA_TYPE_IV, {"iv"}},
    {ROA_TYPE_ORAL, {"oral", "po"}},
    {ROA_TYPE_INHALATION, {"inhalation", "inhale", "inhaled", "smoke", "smoked"}},
    {ROA_TYPE_INTRANASAL, {"intranasal"}},
    {ROA_TYPE_SL, {"sl", "sublingual"}},
};

inline const std::unordered_map<ROA_TYPE, COMP_MODEL> roaToCompModelMap
{
    {ROA_TYPE_IV, ONE_COMP_MODEL},
    {ROA_TYPE_ORAL, TWO_COMP_MODEL},
    {ROA_TYPE_INTRANASAL, TWO_COMP_MODEL},
    {ROA_TYPE_INHALATION, TWO_COMP_MODEL},
    {ROA_TYPE_SL, TWO_COMP_MODEL},
};

template <typename T>
inline const std::unordered_map<T, std::vector<std::string>>& getUnitStringMap()
{
    if constexpr (std::is_same_v<T, DOSE_UNIT>) return doseUnitToStringMap;
    else if constexpr (std::is_same_v<T, BASE_UNIT>) return volumeUnitToStringMap;
    else if constexpr (std::is_same_v<T, TIME_UNIT>) return timeUnitToStringMap;
    else if constexpr (std::is_same_v<T, ROA_TYPE>) return roaToStringMap;

    throw std::invalid_argument("unsupported unit type");
}
