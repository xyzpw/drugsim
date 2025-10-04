#include <cstdlib>
#include <cassert>
#include "pch.hpp"
#include "argparser.hpp"
#include "arg_constants.hpp"

using std::string;
using std::size_t;

void ArgParser::displayHelp()
{
    std::stringstream stream;
    stream << std::left;

    stream << "usage:\n\n";

    const int spaces = 32;

    for (const auto& it : args)
    {
        string line = "  " + it.meta.flag;

        if (it.meta.flag.length() > 2 && !it.meta.param.empty())
            line += '=';

        line += it.meta.param;

        if (line.length() >= spaces) {
            line += '\n';
            line += string(spaces, ' ');
        } else {
            line += string(spaces - line.length(), ' ');
        }

        line += it.meta.desc;

        stream << line << std::endl;
    }

    std::cout << stream.str() << std::endl;
}

void ArgParser::addArg(const Args::Metadata meta)
{
    args.push_back({.meta=meta});
}

/* Sort args in alphabetical order. */
void ArgParser::sortArgs()
{
    std::sort(args.begin(), args.end(), [&](auto& a, auto& b) {
            return a.meta.flag < b.meta.flag;
    });
}

/* Check if arg has been added to parser. */
bool ArgParser::argExists(std::string name) const
{
    for (const auto& it : args) {
        if (it.meta.flag == name) {
            return true;
        }
    }
    return false;
}

ArgParser::Arg& ArgParser::getArg(const Args::Metadata meta)
{
    for (auto& it : args) {
        if (meta.flag == it.meta.flag)
            return it;
    }

    throw std::invalid_argument("arg '" + meta.flag + "' does not exist");
}

ArgParser::Arg& ArgParser::getArgByFlag(std::string flag)
{
    for (auto& it : args) {
        if (it.meta.flag == flag)
            return it;
    }

    throw std::invalid_argument("no arg with name '" + flag + "' exist");
}

/* Check if arg was used. */
bool ArgParser::isArgUsed(Args::Metadata arg) const
{
    for (const auto& it : args) {
        if (it.meta.flag == arg.flag && it.value.has_value())
            return true;
    }
    return false;
}

void ArgParser::parse(int argc, char** argv)
{
    if (argc == 1)
        return;

    auto setVal = [&](const Args::Metadata meta, const std::string& val) {
        for (auto& it : args) { if (it.meta.flag == meta.flag) it.value = val; }
    };

    const Arg* currentArg = nullptr;

    for (int i = 1; i < argc; ++i)
    {
        std::string val = argv[i];
        bool isLast = i + 1 == argc;
        bool nextExists = i + 2 == argc;
        bool isNextArg = nextExists && argExists(argv[i + 1]);
        bool isShort = val.at(0) == '-' && val.at(1) != '-' && val.length() > 2;

        if (val == "--help" || val == "-h") {
            displayHelp();
            std::exit(0);
        }

        size_t flagEndPos = isShort ? 1 : val.find_first_of('=') - 1;
        string flag = val.substr(0, flagEndPos + 1);

        bool exists = argExists(flag);
        if (exists) {
            currentArg = &getArgByFlag(flag);
        }

        bool hasParam = exists && !currentArg->meta.param.empty();

        if (exists && isShort) {
            setVal(currentArg->meta, val.substr(2));
            continue;
        }
        else if (exists && val.find('=') != string::npos) {
            setVal(currentArg->meta, val.substr(flagEndPos + 2));
            continue;
        }

        if (exists && hasParam && (isNextArg || isLast)) {
            throw std::invalid_argument("flag '" + val + "' requires an argument");
        }

        if (exists && !hasParam) {
            setVal(currentArg->meta, "true");
        }
        else if (exists && hasParam) {
            setVal(currentArg->meta, argv[i + 1]);
        }
    }
}
