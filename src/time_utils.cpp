#include <thread>
#include <cassert>
#include <regex>
#include "pch.hpp"
#include "time_utils.hpp"

using std::string;
using system_clock = std::chrono::system_clock;
using chronoMs = std::chrono::milliseconds;
using chronoSeconds = std::chrono::seconds;
using chronoDays = std::chrono::days;
using std::chrono::duration_cast;
using std::chrono::time_point;
using std::chrono::local_time;

const std::string_view dateFormats[] = {
    "%Y%m%d",
    "%Y-%m-%d",
    "%m/%d/%Y",
};

const std::string_view timeFormats[] = {
    "%H%M",
    "%H:%M",
    "%H%M:%S",
    "%H:%M:%S",
    "%I:%M %p",
    "%I:%M:%S %p",
};

std::chrono::duration<double> getEpoch()
{
    auto dur = duration_cast<chronoMs>(system_clock::now().time_since_epoch());
    return dur;
}

/* Return epoch of date and time string, e.g. 20250102 0300 */
std::chrono::duration<double> getDateEpoch(string datetimeStr)
{
    const auto tzone = std::chrono::current_zone();
    local_time<chronoSeconds> localTp;
    chronoSeconds extraSeconds;

    auto spacePos = datetimeStr.find_first_of(' ');

    if (spacePos == string::npos) {
        throw std::invalid_argument("invalid datetime format");
    }

    string dateStr = datetimeStr.substr(0, spacePos);
    string timeStr = datetimeStr.substr(spacePos + 1);

    for (const auto& fmt : dateFormats) {
        std::istringstream ss(dateStr);
        ss >> std::chrono::parse(fmt.data(), localTp);
        if (!ss.fail() && ss.eof())
            break;
    }

    for (const auto& fmt : timeFormats) {
        std::istringstream ss(timeStr);
        ss >> std::chrono::parse(fmt.data(), extraSeconds);
        if (!ss.fail() && ss.eof())
            break;
    }

    // Add seconds from time input to local date epoch.
    localTp += extraSeconds;

    time_point tp = tzone->to_sys(localTp);

    return tp.time_since_epoch();
}

/* Return epoch of a specified time on this day, e.g. 0400 */
std::chrono::duration<double> getTimeEpoch(string timeStr)
{
    chronoSeconds sec;

    std::istringstream ss;

    for (const auto& fmt : timeFormats) {
        ss.str(timeStr);
        ss >> std::chrono::parse(fmt.data(), sec);

        // Break loop if parsing did not fail and eof is true.
        if (!ss.fail() && ss.eof()) {
            break;
        }

        ss.clear();
    }

    const auto zone = std::chrono::current_zone();
    auto local = zone->to_local(system_clock::now());
    auto localDays = std::chrono::floor<chronoDays>(local);
    time_point tp = zone->to_sys(localDays + sec);

    return tp.time_since_epoch();
}

/* Return readable time and date of epoch, e.g. 0600 (2025-04-03)*/
string getTimeAndDateString(std::chrono::duration<double> epoch, bool is12HrFormat)
{
    string fmt = is12HrFormat ? "{:%I:%M %p (%m/%d/%Y)}" :
                                "{:%H%M (%Y-%m-%d)}";

    time_t raw = epoch.count() == -1 ? getEpoch().count() : epoch.count();
    const auto zone = std::chrono::current_zone();
    auto tp = zone->to_local(std::chrono::sys_seconds{chronoSeconds(raw)});

    return std::vformat(fmt, std::make_format_args(tp));
}

std::chrono::duration<double> hhmmToSeconds(string hhmm)
{
    std::regex re{R"((\d{2}):?(\d{2})(?::(\d{2}))?)"};
    std::smatch match;

    if (!std::regex_search(hhmm, match, re)) {
        throw std::invalid_argument("invalid hhmm format");
    }

    int h = stoi(match[1].str());
    int m = stoi(match[2].str());
    int s = match[3].matched ? stoi(match[3].str()) : 0;

    return chronoSeconds(h * 3600 + m * 60 + s);
}

/* Sleep for n milliseconds */
void sleepFor(int ms)
{
    std::this_thread::sleep_for(chronoMs(ms));
}
