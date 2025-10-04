#pragma once

#include <chrono>
#include <string>

std::chrono::duration<double> getEpoch();
std::chrono::duration<double> getTimeEpoch(std::string timeStr);
std::chrono::duration<double> getDateEpoch(std::string datetimeStr);
std::string getTimeAndDateString(std::chrono::duration<double> epoch=
                                 std::chrono::duration<double>(-1),
                                 bool is12HrFormat=false);
std::chrono::duration<double> hhmmToSeconds(std::string hhmm);
void swapTimeFormat(std::string& text);
void sleepFor(int ms);
