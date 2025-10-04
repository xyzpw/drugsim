#pragma once

#include <string>
#include "common.hpp"

namespace UnitConverter
{
    namespace Dose {
        double toDefaultFactor(const DOSE_UNIT& unit);
        double toMgPerLiterFactor(const DOSE_UNIT& unit, const BASE_UNIT& base);
    }

    namespace Base {
        double toLitersFactor(const BASE_UNIT& base);
    }

    namespace Time {
        double toSecondsFactor(const TIME_UNIT& unit);
    }

    template <typename T>
    std::string unitToString(const T& unit)
    {
        const auto& map = getUnitStringMap<T>().at(unit);
        return *std::next(map.begin(), 0);
    }

    template <typename T>
    T stringToUnit(const std::string text)
    {
        const auto& map = getUnitStringMap<T>();

        for (const auto& it : map) {
            if (vectorContains(it.second, text))
                return it.first;
        }

        throw std::invalid_argument("could not convert string to unit");
    }
};

struct ParsedDose {
    double value = 0;
    DOSE_UNIT doseUnit = DOSE_UNIT_MG;
    BASE_UNIT baseUnit = BASE_UNIT_L;
    bool useDoseUnit = false;
    bool useBaseUnit = false;
};

double timeInputToSeconds(std::string text);
ParsedDose parseDoseInput(std::string text);
void setFractionsToDecimal(std::string& text);
void setPercentagesToDecimal(std::string& text);
std::string formatSigFigs(const double& value, const int& sigfigs);
std::string formatSeconds(float s);
bool isDoseUnitVolume(const DOSE_UNIT& unit);
