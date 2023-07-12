// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#include "common_utils.h"

// STD
#include <iomanip>
#include <iostream>
#if defined(_MSC_VER)
#include <windows.h>
#endif
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
#include <experimental/filesystem>
#else
#include <filesystem>
#endif

// APSI
#include "apsi/log.h"
#include "apsi/psi_params.h"
#include "apsi/util/utils.h"
#include "common/base_clp.h"

using namespace std;
#if defined(__GNUC__) && (__GNUC__ < 8) && !defined(__clang__)
namespace fs = std::experimental::filesystem;
#else
namespace fs = std::filesystem;
#endif
using namespace seal;
using namespace apsi;
using namespace apsi::util;

/**
This only turns on showing colors for Windows.
*/
void prepare_console()
{
#ifndef _MSC_VER
    return; // Nothing to do on Linux.
#else
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole == INVALID_HANDLE_VALUE)
        return;

    DWORD dwMode = 0;
    if (!GetConsoleMode(hConsole, &dwMode))
        return;

    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hConsole, dwMode);
#endif
}

vector<string> generate_timespan_report(
    const vector<Stopwatch::TimespanSummary> &timespans, int max_name_length)
{
    vector<string> report;

    for (const auto &timespan : timespans) {
        stringstream ss;
        ss << "\033[4;36m" << setw(max_name_length) << left << timespan.event_name << "\033[0m: " << endl 
           << "\tInstan*:  " << setw(12) << right << timespan.event_count << "   " << endl;
        if (timespan.event_count == 1) {
            ss << "\tDuration:  " << setw(12) << fixed << setprecision(4) << right << timespan.avg << " ms";
        } else {
            ss << "\tAverage:  " << setw(12) << fixed << setprecision(4) << right << timespan.avg << " ms"
               << "\tDurati*:  " << setw(12) << fixed << setprecision(4) << right << timespan.sum << " ms"
               << endl
               << "\tMinimum:  " << setw(12) << fixed << setprecision(4) << right << timespan.min << " ms"
               << "\tMaximum:  " << setw(12) << fixed << setprecision(4) << right << timespan.max << " ms";
        }

        report.push_back(ss.str());
    }

    return report;
}

vector<string> generate_event_report(
    const vector<Stopwatch::Timepoint> &timepoints, int max_name_length)
{
    vector<string> report;

    Stopwatch::time_unit last = Stopwatch::start_time;
    for (const auto &timepoint : timepoints) {
        stringstream ss;

        int64_t since_start = chrono::duration_cast<chrono::milliseconds>(
                                  timepoint.time_point - Stopwatch::start_time)
                                  .count();
        int64_t since_last =
            chrono::duration_cast<chrono::milliseconds>(timepoint.time_point - last).count();

        ss << setw(max_name_length) << left << timepoint.event_name << ": " << setw(6) << right
           << since_start << "ms since start, " << setw(6) << right << since_last
           << "ms since last single event.";
        last = timepoint.time_point;
        report.push_back(ss.str());
    }

    return report;
}

void print_timing_report(const Stopwatch &stopwatch)
{
    vector<string> timing_report;
    vector<Stopwatch::TimespanSummary> timings;
    stopwatch.get_timespans(timings);

    if (timings.size() > 0) {
        timing_report =
            generate_timespan_report(timings, stopwatch.get_max_timespan_event_name_length());

        APSI_LOG_INFO("Timespan event information");
        for (const auto &timing : timing_report) {
            APSI_LOG_INFO(timing.c_str());
        }
    }

    vector<Stopwatch::Timepoint> timepoints;
    stopwatch.get_events(timepoints);

    if (timepoints.size() > 0) {
        timing_report = generate_event_report(timepoints, stopwatch.get_max_event_name_length());

        APSI_LOG_INFO("Single event information");
        for (const auto &timing : timing_report) {
            APSI_LOG_INFO(timing.c_str());
        }
    }
}

void throw_if_file_invalid(const string &file_name)
{
    fs::path file(file_name);

    if (!fs::exists(file)) {
        APSI_LOG_ERROR("File `" << file.string() << "` does not exist");
        throw logic_error("file does not exist");
    }
    if (!fs::is_regular_file(file)) {
        APSI_LOG_ERROR("File `" << file.string() << "` is not a regular file");
        throw logic_error("invalid file");
    }
}

string nice_byte_count(uint64_t bytes)
{
        stringstream ss;
        if (bytes >= 1024 * 1024) {
            ss << fixed << setprecision(3) << setw(10) << right << static_cast<double>(bytes) / 1024 / 1024 << " MB" << defaultfloat;
        }
        else if (bytes >= 1024) {
            ss << fixed << setprecision(3) << setw(10) << right << static_cast<double>(bytes) / 1024 << " KB" << defaultfloat;
        } 
        else {
            ss << fixed << setprecision(3) << setw(10) << right << static_cast<double>(bytes) << " B" << defaultfloat;
        }
        return ss.str();
}
