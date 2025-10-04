#pragma once

#include <string>
#include "pch.hpp"
#include "simulation_info.hpp"

/* Functions used by the simulation. */
namespace SimHelper
{
    void validateInit(SimulationInfo&);
    double getMinDisplayDose(int prec);
    void updateCurrentDoses(SimulationInfo&);
    void checkMaxAchieved(SimulationInfo&);
    void checkTmaxState(SimulationInfo&);
    void useFixedPrecision(SimulationInfo&);
    void checkFullyAbsorbed(SimulationInfo&);
    bool isMinDose(SimulationInfo&);
    void updateCache(SimulationInfo&);
    void updateOutput(std::string& out, const SimulationInfo&, const std::string& unit);
}
