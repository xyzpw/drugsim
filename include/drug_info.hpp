#pragma once

#include <optional>
#include "common.hpp"

struct DrugInfo {
    ROA_TYPE roa = ROA_TYPE_IV;
    float vd = 1;                 // volume of distribution
    double dose = -1;             // dose in milligrams
    double lagtime = 0;           // lagtime in seconds
    double ka = -1;               // absorption constant in seconds
    double ke = -1;               // elimination constant in seconds
    double tmax = 0;              // time to reach peak concentration
    float bioavailability = 1.0f;
    double ed50 = -1;
    float excretionFrac = 1.0f;   // fraction excreted unchanged

    /* If prodrug is used, these values are for the active drug. */
    bool isProdrug = false;
    std::optional<double> activeKe;
    std::optional<float> activeFrac;

    /* If delayed release is used, these values are for the second dose. */
    bool isDr = false;
    std::optional<float> drFrac;
    std::optional<float> drLagtime;
};
