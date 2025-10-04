#pragma once

#include <chrono>
#include <optional>
#include <string>
#include "drug_info.hpp"
#include "common.hpp"

struct SimulationInfo {
    std::chrono::duration<double> epoch{};

    std::optional<std::string> msg;
    int precision = 0;
    std::optional<int> sigfigs;
    double minDoseAllowed = 0;

    bool is12HrFormat = false;      // display time in 12 hour format?
    bool isAucEnabled = false;
    bool doseUnitsEnabled = false;
    bool isMaxStatEnabled = false;  // should display max concentration achieved?
    bool isDoseUnitVolume = false;  // is the dose unit in volume units?
    bool baseUnitsEnabled = false;
    bool ed50Enabled = false;
    bool displayExcreted = false;

    DrugInfo drugInfo;

    COMP_MODEL compModel = ONE_COMP_MODEL;

    /* Dynamic simulation info. */
    struct State {
        /* Time tracking */
        double elapsed = 0.0; // time since simulation start (in seconds)

        int prec = 0; // current dose unit precision
        DOSE_UNIT doseUnit = DOSE_UNIT_MG;
        BASE_UNIT baseUnit = BASE_UNIT_L;

        /* Drug concentrations */
        double drugContent;
        double doseAsUnit;
        double excreted;
        float effectiveness;

        /* Active drug concentrations */
        std::optional<double> activeDrugContent;
        std::optional<double> activeDoseAsUnit;

        /* Accumulation info */
        double auc = 0; // area under curve
        double maxAchieved; // highest dose achieved (does not count prodrug)

        /* Progress flags */
        bool fullyAbsorbed = true; // is drug fully absorbed?
        bool hasTmaxed = true; // has drug reached cmax?
        bool hasDrReleased = false;
        bool hasDrTmaxed = false;

        /* Thresholds */
        double minDisplayDose = 0.5; // lowest dose that can be displayed
        double minProdrugDisplayDose = -1;

        bool isMultiline = false;
    } state;

    /* Cache information. */
    struct Cache {
        /* Outputs */
        std::string output; // this will be printed
        std::string altOutput;

        /* Unit strings */
        std::string doseUnitStr;
        std::string baseUnitStr;
        std::string fullDoseUnitStr;

        // Combine dose unit and base unit string, e.g. mg/L.
        void updateFullDoseUnitStr() {
            fullDoseUnitStr = doseUnitStr + '/' + baseUnitStr;
        };
    } cache;
};

/*
 * Labels that come before dose in output --
 * use these instead of creating a new string every tick.
*/
const std::string_view absorptionPhaseLabel{"absorption"};
const std::string_view eliminationPhaseLabel{"elimination"};
const std::string_view elPhaseAbsorbingLabel{"elimination, abs"};
const std::string_view lagPhaseLabel{"lag"};

/*
 * String view of abbreviations --
 * use these instead of creating a new string every tick.
*/
const std::string_view MG_STR{"mg"};      // string_view of mg abbreviation
const std::string_view ML_STR{"mL"};
const std::string_view MGL_STR{"mg/L"};   // string_view of mg/L abbreviation
