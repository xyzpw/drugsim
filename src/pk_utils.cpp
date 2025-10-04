#include <stdexcept>
#include <cassert>
#include "pch.hpp"
#include "pk_utils.hpp"
#include "simulation_info.hpp"
#include "drug_info.hpp"

using std::exp;
using std::log;

namespace OneComp = PK::OneCompartment;
namespace TwoComp = PK::TwoCompartment;

void throwInvalidArg(std::string text)
{
    throw std::invalid_argument(text);
}

double computeDrugContent(const SimulationInfo& simInfo, double elapsed)
{
    const auto& drug = simInfo.drugInfo;
    const auto& dose = drug.dose;
    const int& model = simInfo.compModel;

    switch (model)
    {
        case ONE_COMP_MODEL:
            return OneComp::computeDrugContent(drug, dose, elapsed);
        case TWO_COMP_MODEL:
            if (drug.isDr) {
                return TwoComp::computeDrugContentDr(drug, dose, elapsed);
            }
            return TwoComp::computeDrugContent(drug, dose, elapsed);
    }

    return 0.0;
}

double OneComp::computeDrugContent(const DrugInfo& drug, double dose, double t)
{
    return drug.dose / drug.vd * exp(-drug.ke * t);
}

double OneComp::computeExcreted(const DrugInfo& drug, const double& t)
{
    const float& fe = drug.excretionFrac;
    const float& vd = drug.vd;
    const double& dose = drug.dose;
    const double& ke = drug.ke;

    return fe * dose / vd * (1 - exp(-ke * t));
}

/* Compute the amount of a metabolite remaining. */
double OneComp::computeMetaboliteContent(const DrugInfo& drug, const double& t)
{
    if (!drug.activeKe.has_value())
        throwInvalidArg("metabolite has no elimination constant");

    const double& ke = drug.ke;
    const double& km = *drug.activeKe;

    double result = drug.dose * *drug.activeFrac;
    result *= exp(-ke * t) / (km - ke) + exp(-km * t) / (ke - km);

    return result / 3600;
}

double OneComp::computeMetaboliteExcreted(const DrugInfo& drug, const double& t)
{
    const double& ke = drug.ke;
    const double& km = *drug.activeKe;
    const float& fe = drug.excretionFrac;

    double result = 1 - exp(-ke * t) - ke/km * (1 - exp(-km * t));
    result *= fe * drug.dose * *drug.activeFrac * ke / (km - ke);

    return result;
}

/*
 * Compute the area under curve for metabolite.
 *
 * @note: Units are in hours.
*/
double OneComp::computeMetaboliteAuc(const DrugInfo& drug, const double& t)
{
    assert(drug.activeKe.has_value());

    const double& ke = drug.ke;
    const double& km = *drug.activeKe;

    if (ke == km) {
        double result = (1 - exp(-ke * t) * (ke * t + 1)) / (ke * ke);
        result *= drug.dose * ke * *drug.activeFrac;

        return result / 3600;
    }

    double result = ke / (km - ke) * drug.dose * *drug.activeFrac;
    result *= (1 - exp(-ke * t)) / ke - (1 - exp(-km * t)) / km;

    return result / 3600;
}

double TwoComp::computeDrugContent(const DrugInfo& drug, double dose, double t)
{
    const double& ka = drug.ka;
    const double& ke = drug.ke;
    const float& vd = drug.vd;
    const float& bio = drug.bioavailability;

    if (ka == ke) {
        return bio * dose * ke / vd * t * exp(-ke * t);
    }

    return (bio * dose * ka) / (vd * (ka - ke)) * (exp(-ke * t) - exp(-ka * t));
}

double TwoComp::computeExcreted(const DrugInfo& drug, const double& t)
{
    const double& dose = drug.dose;
    const float& frac = drug.excretionFrac;
    const float& bio = drug.bioavailability;
    const double& ke = drug.ke;
    const double& ka = drug.ka;

    double result;

    if (drug.isDr) {
        const float& drFrac = *drug.drFrac;
        const float& lag = *drug.drLagtime;

        result = frac * bio * (1 - drFrac) * dose * ka / ((ka - ke) * drug.vd);
        result *= (1 - exp(-ke * t)) - ke/ka * (1 - exp(-ka * t));

        if (t < lag)
            return result;

        double add = frac * bio * (drFrac * dose) * ka / ((ka - ke) * drug.vd);
        add *= (1 - exp(-ke * (t - lag))) - ke/ka * (1 - exp(-ka * (t - lag)));

        return result + add;
    }

    result = frac * bio * dose * ka / ((ka - ke) * drug.vd);
    result *= (1 - exp(-ke * t)) - ke/ka * (1 - exp(-ka * t));

    return result;
}

/*
 * Return the active drug content from the prodrug.
*/
double TwoComp::computeMetaboliteContent(const DrugInfo& drug, const double& t)
{
    if (!drug.activeKe.has_value() || !drug.activeFrac.has_value()) {
        throw std::invalid_argument("active drug from prodrug contains no info");
    }

    const float& bio = drug.bioavailability; // alias because I hate long names
    const float& activeFrac = drug.activeFrac.value();

    std::vector<const double*> rates{&drug.ka, &drug.ke, &drug.activeKe.value()};

    /* https://en.wikipedia.org/wiki/Bateman_equation */
    double sum = 0.0; // sum of iter
    for (auto i = 0u; i < rates.size(); ++i) {
        double num = exp(-*rates[i] * t);
        double den = 1.0;
        for (auto j = 0u; j < rates.size(); ++j) {
            if (j == i)
                continue;
            den *= (*rates[j] - *rates[i]);
        }
        sum += num / den;
    }

    return (drug.dose * activeFrac * bio) * (drug.ka * drug.ke) * sum;
}

double TwoComp::computeMetaboliteExcreted(const DrugInfo& drug, const double& t)
{
    const double& dose = drug.dose;
    const double& activeFrac = *drug.activeFrac;
    const float& bio = drug.bioavailability;
    const double& ka = drug.ka;
    const double& ke = drug.ke;
    const double& km = *drug.activeKe;
    const float& fe = drug.excretionFrac;

    double result = (1 - exp(-ka * t)) / (ka * (ke - ka) * (km - ka));
    result += (1 - exp(-ke * t)) / (ke * (ka - ke) * (km - ke));
    result += (1 - exp(-km * t)) / (km * (ka - km) * (ke - km));
    result *= ka * ke * bio * activeFrac * dose;
    result *= fe * km;

    return result;
}

double TwoComp::computeDrugContentDr(const DrugInfo& drug, double dose, const double& t)
{
    if (!drug.isDr)
        throw std::invalid_argument("dr drug contains no info");

    const double& lag = drug.drLagtime.value();

    double delayedDose = drug.drFrac.value() * dose;

    // Reduce dose value to the IR frac dose.
    dose *= 1.0 - drug.drFrac.value();

    double result = TwoComp::computeDrugContent(drug, dose, t);
    if (t >= lag) {
        result += TwoComp::computeDrugContent(drug, delayedDose, t - lag);
    }

    return result;
}

bool TwoComp::computeIsAbsorbed(const DrugInfo& drug, const double& t)
{
    const double& ka = drug.ka;

    return (1 - exp(-ka * t)) >= ABSORBED_THRESHOLD;
}

/* Return the time it takes to reach peak concentration. */
double TwoComp::computeTmax(const DrugInfo& drug)
{
    const double& ka = drug.ka;
    const double& ke = drug.ke;

    return ka == ke ? 1.0 / ka : log(ka / ke) / (ka - ke);
}

/*
 * Compute area under curve for one compartment model.
 *
 * @note: units are in hours
*/
double OneComp::computeAuc(const DrugInfo& drug, const double& dose, const double& t)
{
    const double& ke = drug.ke;

    return (1 - exp(-ke * t)) / ke * dose / 3600;
}

/*
 * Compute area under curve for two compartment model.
 *
 * @note: units are in hours
*/
double TwoComp::computeAuc(const DrugInfo& drug, const double& dose, const double& t)
{
    const double& ka = drug.ka;
    const double& ke = drug.ke;
    const float& bio = drug.bioavailability;

    if (ka == ke) {
        double result = (1 - exp(-ke * t) * (ke * t + 1)) / (ke * ke);
        result *= dose * bio * ke;

        return result / 3600;
    }

    double result = (1 - exp(-ke * t)) / ke;
    result -= (1 - exp(-ka * t)) / ka;
    result *= bio * dose * ka / (ka - ke);

    return result / 3600;
}

/*
 * Compute area under curve for delayed release drug in two compartment model.
 *
 * @note: units are in hours
*/
double TwoComp::computeAucDr(const DrugInfo& drug, const double& dose, const double& t)
{
    if (!drug.isDr)
        throw std::runtime_error("cannot compute dr auc: drug is not dr");

    // Treat as single dose if delayed has not released.
    if (t < *drug.drLagtime) {
        return TwoComp::computeAuc(drug, dose, t);
    }

    double irDose = dose * (1 - drug.drFrac.value());
    double drT = t - drug.drLagtime.value();

    double result = TwoComp::computeAuc(drug, irDose, t);
    result += TwoComp::computeAuc(drug, dose * drug.drFrac.value(), drT);

    return result;
}

/*
 * Compute area under curve for metabolite in two compartment model.
 *
 * @note: units are in hours
*/
double TwoComp::computeAucMetabolite(const DrugInfo& drug, const double& t)
{
    if (!drug.isProdrug)
        throw std::runtime_error("cannot compute auc for metabolite (no prodrug info)");

    const double& ka = drug.ka;
    const double& ke = drug.ke;
    const double& km = drug.activeKe.value();
    const float& bio = drug.bioavailability;

    double auc = (1 - exp(-ka * t)) / ((ke - ka) * (km - ka) * ka);
    auc += (1 - exp(-ke * t)) / ((ka - ke) * (km - ke) * ke);
    auc += (1 - exp(-km * t)) / ((ka - km) * (ke - km) * km);
    auc *= ka * ke * bio * drug.dose * drug.activeFrac.value();

    return auc / 3600;
}

/*
 * Converts rate constant to half-life or half-life to rate constant.
*/
double PK::convertRateConstant(const double& k)
{
    return log(2) / k;
}

double PK::computeEffectiveness(const double& midpoint, const double& dose)
{
    return 1.0 / (1 + midpoint / dose);
}
