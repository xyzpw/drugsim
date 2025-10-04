#include "pch.hpp"
#include "simulation.hpp"
#include "simulation_helper.hpp"
#include "time_utils.hpp"
#include "convert_utils.hpp"

using std::getchar;
using std::string;

const int tickIntervalMs = 50;

void startLag(SimulationInfo&);
void printStartupText(SimulationInfo&);
void displayOutput(SimulationInfo&);

/* Ansi codes with carriage return before them */
const string lineReset = '\r' + ANSI_CLEAR;
const string lineDown = '\r' + ANSI_DOWN;
const string lineUp = '\r' + ANSI_UP;

void startSimulation(SimulationInfo& simInfo)
{
    using namespace SimHelper;

    /* Startup stuff */
    validateInit(simInfo);
    printStartupText(simInfo);

    // Start lagtime if needed.
    startLag(simInfo);

    /* Aliases to prevent ugly code */
    auto& drug = simInfo.drugInfo;
    auto& simState = simInfo.state;

    /* Get time since drug reached systemic circulation */
    auto getElapsed = [&]() { return (getEpoch() - simInfo.epoch).count(); };

    double& elapsed = simState.elapsed;

    while (true)
    {
        elapsed = getElapsed();

        /* Update doses. */
        updateCurrentDoses(simInfo);

        /* Has delayed release started? */
        if (drug.isDr && !simState.hasDrReleased && elapsed >= drug.drLagtime.value()) {
            simState.hasDrReleased = true;
        }

        checkMaxAchieved(simInfo);

        useFixedPrecision(simInfo);

        updateCache(simInfo);

        displayOutput(simInfo);
        std::flush(std::cout);

        // If the drug is not considered absorbed, check again.
        checkFullyAbsorbed(simInfo);

        // Set drug tmax state to true if drug has reached tmax.
        checkTmaxState(simInfo);

        // Break the loop if the minimum dose has been reached or all numbers are 0.
        if (simState.fullyAbsorbed && isMinDose(simInfo)) {
            break;
        }

        sleepFor(tickIntervalMs); // small delay before repeating
    }

    elapsed = getElapsed();

    // Elapsed time including lagtime.
    float tComplete = elapsed + drug.lagtime;

    std::cout << "\n\nCompletion after " << formatSeconds(tComplete) << '.'
              << " Press enter to exit.";
    getchar();

    std::cout << "\n";
}

void startLag(SimulationInfo& sim)
{
    if (sim.drugInfo.lagtime <= 0)
        return;

    auto& drug = sim.drugInfo;

    double duration = (drug.lagtime + sim.epoch.count()) - getEpoch().count();

    const string label = "lagtime: ";

    while (duration > 0)
    {
        std::cout << label << formatSeconds(duration) << std::flush;

        sleepFor(tickIntervalMs);

        duration -= tickIntervalMs * 1e-3;

        std::cout << lineReset << std::flush;
    }

    sim.epoch += std::chrono::duration<double>(drug.lagtime);
}

/* Print text which is suppose to appear before simulation begins. */
void printStartupText(SimulationInfo& sim)
{
    if (sim.msg.has_value()) {
        std::cout << '\n' << sim.msg.value() << '\n';
    }

    std::cout << "\ntime at administration: "
              << getTimeAndDateString(sim.epoch, sim.is12HrFormat) << "\n\n";

    // Add extra line for more space to display other lines.
    if (sim.state.isMultiline) {
        std::cout << '\n';
    }
}

void displayOutput(SimulationInfo& sim)
{
    const bool& isMulti = sim.state.isMultiline;
    const auto& output = sim.cache.output;
    auto& altOutput = sim.cache.altOutput;

    if (isMulti) {
        // Move line up, reset, and print output.
        std::cout << lineUp << lineReset << output;

        // Move line down, reset, and print second output.
        std::cout << lineDown << lineReset << altOutput;

        return;
    }
    else if (!altOutput.empty()) {
        altOutput.clear();
        std::cout << lineReset << lineUp;
    }

    // Reset line and print output.
    std::cout << lineReset << output;
}
