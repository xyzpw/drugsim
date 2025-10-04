#pragma once

#include <string>
#include <vector>
#include <optional>
#include "arg_constants.hpp"

struct ArgParser {
    struct Arg {
        Args::Metadata meta;
        std::optional<std::string> value; // value of the arg given by the user
    };

    std::vector<Arg> args;

    void displayHelp();
    void addArg(const Args::Metadata);
    void sortArgs();
    bool argExists(std::string name) const;
    Arg& getArg(const Args::Metadata);
    Arg& getArgByFlag(std::string flag);
    bool isArgUsed(Args::Metadata) const;
    void parse(int argc, char** argv);
};
