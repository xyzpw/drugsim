#include "simulation_info.hpp"
#include "simulation.hpp"
#include "argparser.hpp"
#include "input_handler.hpp"
#include "arg_constants.hpp"

void setupArgs(ArgParser&);

int main(int argc, char* argv[])
{
    ArgParser parser;
    SimulationInfo simInfo;

    setupArgs(parser);
    parser.parse(argc, argv);
    handleInput(parser, simInfo);

    startSimulation(simInfo);

    return 0;
}

void setupArgs(ArgParser& parser)
{
    for (const auto& it : globalArgs) {
        parser.addArg(*it);
    }
    parser.sortArgs();
}
