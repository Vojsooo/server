/*
 * Copyright (c) 2026
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include "../StdAfx.h"

#include "ember_system_info.h"

#include <algorithm>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>

#ifdef _WIN32
#include <windows.h>
#include <psapi.h>
#else
#include <unistd.h>
#endif

namespace caspar { namespace protocol { namespace ember {
namespace {

double bytes_to_megabytes(double bytes) { return bytes / (1024.0 * 1024.0); }

#ifdef _WIN32
unsigned long long to_uint64(const FILETIME& file_time)
{
    ULARGE_INTEGER value;
    value.LowPart  = file_time.dwLowDateTime;
    value.HighPart = file_time.dwHighDateTime;
    return value.QuadPart;
}
#else
bool read_proc_stat(unsigned long long& total, unsigned long long& busy)
{
    std::ifstream input("/proc/stat");
    if (!input)
        return false;

    std::string cpu;
    input >> cpu;
    if (cpu != "cpu")
        return false;

    std::vector<unsigned long long> values;
    unsigned long long              value = 0;
    while (input >> value)
        values.push_back(value);

    if (values.size() < 4)
        return false;

    total = 0;
    for (const auto item : values)
        total += item;

    const auto idle   = values.size() > 3 ? values[3] : 0;
    const auto iowait = values.size() > 4 ? values[4] : 0;
    busy              = total - idle - iowait;
    return true;
}

bool read_proc_process_ticks(unsigned long long& total_ticks)
{
    std::ifstream input("/proc/self/stat");
    if (!input)
        return false;

    std::string line;
    std::getline(input, line);
    const auto close_paren = line.rfind(')');
    if (close_paren == std::string::npos || close_paren + 2 >= line.size())
        return false;

    std::istringstream stream(line.substr(close_paren + 2));
    std::string        token;
    int                field = 3;
    unsigned long long utime = 0;
    unsigned long long stime = 0;

    while (stream >> token) {
        if (field == 14)
            utime = std::strtoull(token.c_str(), nullptr, 10);
        else if (field == 15) {
            stime = std::strtoull(token.c_str(), nullptr, 10);
            break;
        }
        ++field;
    }

    total_ticks = utime + stime;
    return true;
}

bool read_proc_self_statm(double page_size, double& resident_mb)
{
    std::ifstream input("/proc/self/statm");
    if (!input)
        return false;

    unsigned long long resident_pages = 0;
    input.ignore(std::numeric_limits<std::streamsize>::max(), ' ');
    input >> resident_pages;
    if (!input)
        return false;

    resident_mb = bytes_to_megabytes(static_cast<double>(resident_pages) * page_size);
    return true;
}

bool read_meminfo(double& total_mb, double& available_mb)
{
    std::ifstream input("/proc/meminfo");
    if (!input)
        return false;

    std::string       key;
    unsigned long long value = 0;
    std::string       unit;
    bool              found_total = false;
    bool              found_available = false;

    while (input >> key >> value >> unit) {
        if (key == "MemTotal:") {
            total_mb   = static_cast<double>(value) / 1024.0;
            found_total = true;
        } else if (key == "MemAvailable:") {
            available_mb   = static_cast<double>(value) / 1024.0;
            found_available = true;
        }

        if (found_total && found_available)
            return true;
    }

    return false;
}
#endif

} // namespace

ember_system_info_sampler::ember_system_info_sampler()
{
#ifndef _WIN32
    page_size_ = sysconf(_SC_PAGESIZE);
#endif
}

system_snapshot ember_system_info_sampler::sample() const
{
    std::lock_guard<std::mutex> lock(mutex_);

    system_snapshot snapshot;

#ifdef _WIN32
    FILETIME idle_time;
    FILETIME kernel_time;
    FILETIME user_time;
    const auto previous_system_total = previous_system_kernel_ + previous_system_user_;
    if (GetSystemTimes(&idle_time, &kernel_time, &user_time)) {
        const auto current_idle   = to_uint64(idle_time);
        const auto current_kernel = to_uint64(kernel_time);
        const auto current_user   = to_uint64(user_time);

        if (initialized_) {
            const auto total_delta = (current_kernel - previous_system_kernel_) + (current_user - previous_system_user_);
            const auto idle_delta  = current_idle - previous_system_idle_;
            if (total_delta > 0)
                snapshot.system_cpu_pct =
                    std::max(0.0, std::min(100.0, static_cast<double>(total_delta - idle_delta) * 100.0 /
                                                     static_cast<double>(total_delta)));
        }

        previous_system_idle_   = current_idle;
        previous_system_kernel_ = current_kernel;
        previous_system_user_   = current_user;
    }

    FILETIME creation_time;
    FILETIME exit_time;
    FILETIME process_kernel_time;
    FILETIME process_user_time;
    if (GetProcessTimes(GetCurrentProcess(), &creation_time, &exit_time, &process_kernel_time, &process_user_time)) {
        const auto current_process_total = to_uint64(process_kernel_time) + to_uint64(process_user_time);
        const auto current_system_total  = previous_system_kernel_ + previous_system_user_;

        if (initialized_ && current_system_total > previous_system_total) {
            const auto system_delta  = current_system_total - previous_system_total;
            const auto process_delta = current_process_total - previous_process_total_;
            snapshot.process_cpu_pct =
                std::max(0.0, std::min(100.0, static_cast<double>(process_delta) * 100.0 / static_cast<double>(system_delta)));
        }

        previous_process_total_ = current_process_total;
    }

    PROCESS_MEMORY_COUNTERS_EX counters;
    std::memset(&counters, 0, sizeof(counters));
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(
            GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters), sizeof(counters))) {
        snapshot.process_resident_mb = bytes_to_megabytes(static_cast<double>(counters.WorkingSetSize));
    }

    MEMORYSTATUSEX memory_status;
    std::memset(&memory_status, 0, sizeof(memory_status));
    memory_status.dwLength = sizeof(memory_status);
    if (GlobalMemoryStatusEx(&memory_status)) {
        snapshot.system_total_memory_mb     = bytes_to_megabytes(static_cast<double>(memory_status.ullTotalPhys));
        snapshot.system_available_memory_mb = bytes_to_megabytes(static_cast<double>(memory_status.ullAvailPhys));
    }
#else
    unsigned long long current_system_total = 0;
    unsigned long long current_system_busy  = 0;
    const auto         previous_system_total = previous_system_total_;
    if (read_proc_stat(current_system_total, current_system_busy) && initialized_ && current_system_total > previous_system_total_) {
        const auto total_delta = current_system_total - previous_system_total_;
        const auto busy_delta  = current_system_busy - previous_system_busy_;
        snapshot.system_cpu_pct =
            std::max(0.0, std::min(100.0, static_cast<double>(busy_delta) * 100.0 / static_cast<double>(total_delta)));
    }

    previous_system_total_ = current_system_total;
    previous_system_busy_  = current_system_busy;

    unsigned long long current_process_ticks = 0;
    if (read_proc_process_ticks(current_process_ticks) && initialized_ && current_system_total > previous_system_total) {
        const auto total_delta   = current_system_total - previous_system_total;
        const auto process_delta = current_process_ticks - previous_process_total_;
        if (total_delta > 0) {
            snapshot.process_cpu_pct =
                std::max(0.0, std::min(100.0, static_cast<double>(process_delta) * 100.0 / static_cast<double>(total_delta)));
        }
    }
    previous_process_total_ = current_process_ticks;

    double resident_mb = 0.0;
    if (page_size_ > 0 && read_proc_self_statm(static_cast<double>(page_size_), resident_mb))
        snapshot.process_resident_mb = resident_mb;

    read_meminfo(snapshot.system_total_memory_mb, snapshot.system_available_memory_mb);
#endif

    if (snapshot.system_total_memory_mb > 0.0) {
        snapshot.system_used_memory_pct =
            std::max(0.0,
                     std::min(100.0,
                              ((snapshot.system_total_memory_mb - snapshot.system_available_memory_mb) * 100.0) /
                                  snapshot.system_total_memory_mb));
    }

    initialized_ = true;
    return snapshot;
}

}}} // namespace caspar::protocol::ember
