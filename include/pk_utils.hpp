#pragma once

#include "simulation_info.hpp"

namespace PK
{
    namespace OneCompartment
    {
        double computeDrugContent(const DrugInfo&, double dose, double t);
        double computeExcreted(const DrugInfo& drug, const double& t);
        double computeMetaboliteContent(const DrugInfo&, const double& t);
        double computeMetaboliteExcreted(const DrugInfo&, const double& t);
        double computeAuc(const DrugInfo&, const double& dose, const double& t);
        double computeMetaboliteAuc(const DrugInfo& dose, const double& t);
    }

    namespace TwoCompartment
    {
        double computeDrugContent(const DrugInfo&, double dose, double t);
        double computeExcreted(const DrugInfo& drug, const double& t);
        double computeDrugContentDr(const DrugInfo&, double dose, const double& t);
        double computeMetaboliteContent(const DrugInfo&, const double& t);
        double computeMetaboliteExcreted(const DrugInfo&, const double& t);
        double computeAuc(const DrugInfo&, const double& dose, const double& t);
        double computeAucDr(const DrugInfo&, const double& dose, const double& t);
        double computeAucMetabolite(const DrugInfo&, const double& t);
        bool computeIsAbsorbed(const DrugInfo&, const double& t);
        double computeTmax(const DrugInfo&);
    }

    double convertRateConstant(const double& k);
    double computeEffectiveness(const double& midpoint, const double& dose);
}

double computeDrugContent(const SimulationInfo& simInfo, double elapsed);
