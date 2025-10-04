#pragma once

#include <string>
#include <array>
#include <map>

inline std::string ARG_TIME_PARAM = "<time>[ unit]";
inline std::string ARG_DATE_PARAM = "<4+YMMDD hhmm[:ss]>";

inline std::string ARG_TIME_DESC = "time at administration (accepts 12-hour format)";
inline std::string ARG_DATE_DESC = "date and time of drug administration (accepts USA format)";
inline std::string ARG_ELAPSED_DESC = "time elapsed since drug was given";
inline std::string ARG_DOSE_DESC = "dose of drug administered";
inline std::string ARG_COUNT_DESC = "number of doses administered";
inline std::string ARG_T12_DESC = "elimination half-life";
inline std::string ARG_T12ABS_DESC = "absorption half-life";
inline std::string ARG_PRECISION_DESC = "decimal precision (default: 0)";
inline std::string ARG_LAGTIME_DESC = "time for drug to reach systemic circulation";
inline std::string ARG_ROA_DESC = "route of administration";
inline std::string ARG_BIO_DESC = "bioavailability";
inline std::string ARG_MAX_DESC = "display max concentration achieved";
inline std::string ARG_MIN_DESC = "minimum dose allowed to be displayed";
inline std::string ARG_PRODRUG_DESC = "fraction of prodrug converts into active drug";
inline std::string ARG_T12M_DESC = "half-life of metabolite";
inline std::string ARG_DR_DESC = "time until delayed dose is released";
inline std::string ARG_DR_FRAC_DESC = "fraction of dose is delayed form";
inline std::string ARG_VOLUME_DESC = "volume of distribution in liters";
inline std::string ARG_ED50_DESC = "dose required to obtain half effectiveness";

namespace Args
{
    struct Metadata { std::string flag; std::string param; std::string desc; };

    inline const Metadata TIME = {"--time", "<hhmm[:ss]>", ARG_TIME_DESC};
    inline const Metadata DATE = {"--date", ARG_DATE_PARAM, ARG_DATE_DESC};
    inline const Metadata ELAPSED = {"--elapsed", "<hhmm[:ss]|time unit>", ARG_ELAPSED_DESC};
    inline const Metadata DOSE = {"--dose", "<dose>[ unit]", ARG_DOSE_DESC};
    inline const Metadata COUNT = {"--count", "<n>", ARG_COUNT_DESC};
    inline const Metadata T12 = {"--t12", ARG_TIME_PARAM, ARG_T12_DESC};
    inline const Metadata T12ABS = {"--t12abs", ARG_TIME_PARAM, ARG_T12ABS_DESC};
    inline const Metadata PRECISION = {"-p", "<n>", ARG_PRECISION_DESC};
    inline const Metadata SIGFIGS = {"--sigfigs", "<n>", "sigfigs to round results to"};
    inline const Metadata LAGTIME = {"--lagtime", ARG_TIME_PARAM, ARG_LAGTIME_DESC};
    inline const Metadata ROA = {"--roa", "<roa>", ARG_ROA_DESC};
    inline const Metadata BIOAVAILABILITY = {"-F", "<decimal>", ARG_BIO_DESC};
    inline const Metadata PRODRUG = {"--prodrug", "<decimal>", ARG_PRODRUG_DESC};
    inline const Metadata T12M = {"--t12m", ARG_TIME_PARAM, ARG_T12M_DESC};
    inline const Metadata MAX = {"--max", "", ARG_MAX_DESC};
    inline const Metadata MIN = {"--min", "<dose>[ unit]", ARG_MIN_DESC};
    inline const Metadata DR = {"--dr", ARG_TIME_PARAM, ARG_DR_DESC};
    inline const Metadata DR_FRAC = {"--dr-frac", "<decimal>", ARG_DR_FRAC_DESC};
    inline const Metadata MSG = {"--msg", "<msg>", "custom start message"};
    inline const Metadata ARG_FILE = {"--file", "<name>", "custom file config"};
    inline const Metadata AUC = {"--auc", "", "display area under curve"};
    inline const Metadata VOLUME = {"--volume", "<n>", ARG_VOLUME_DESC};
    inline const Metadata ED50 = {"--ed50", "<dose>[ unit]", ARG_ED50_DESC};
    inline const Metadata EXCRETION = {"--excretion", "<decimal>", "fraction of drug excreted unchanged"};
}

/* All commands available. */
inline constexpr std::array<const Args::Metadata*, 24> globalArgs=
{
    &Args::TIME,
    &Args::DATE,
    &Args::ELAPSED,
    &Args::DOSE,
    &Args::COUNT,
    &Args::T12,
    &Args::T12ABS,
    &Args::PRECISION,
    &Args::LAGTIME,
    &Args::ROA,
    &Args::BIOAVAILABILITY,
    &Args::MAX,
    &Args::MIN,
    &Args::PRODRUG,
    &Args::T12M,
    &Args::DR,
    &Args::DR_FRAC,
    &Args::MSG,
    &Args::ARG_FILE,
    &Args::AUC,
    &Args::VOLUME,
    &Args::ED50,
    &Args::EXCRETION,
    &Args::SIGFIGS,
};

/* Args associated with their config param, e.g. {arg, str} = {"dose": "25 mg"} */
inline const std::map<const Args::Metadata*, std::string> configArgs
{
    {&Args::DOSE, "dose"},
    {&Args::COUNT, "count"},
    {&Args::T12, "t12"},
    {&Args::T12ABS, "t12abs"},
    {&Args::PRECISION, "precision"},
    {&Args::LAGTIME, "lagtime"},
    {&Args::ROA, "roa"},
    {&Args::BIOAVAILABILITY, "bioavailability"},
    {&Args::MAX, "max"},
    {&Args::MIN, "min"},
    {&Args::PRODRUG, "prodrug"},
    {&Args::T12M, "t12m"},
    {&Args::DR, "dr"},
    {&Args::DR_FRAC, "dr-frac"},
    {&Args::MSG, "msg"},
    {&Args::VOLUME, "volume"},
    {&Args::ED50, "ed50"},
    {&Args::EXCRETION, "excretion"},
    {&Args::SIGFIGS, "sigfigs"},
};
