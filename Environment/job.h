#pragma once
#include <cstdint>

struct Job {
    uint64_t id;
    double remaining_cpu;
    double arrival_time;
    double last_run_time;
    int priority;
    double deadline;

    Job(uint64_t id_,
        double remaining_cpu_,
        double arrival_time_,
        int priority_,
        double deadline_)
        : id(id_),
          remaining_cpu(remaining_cpu_),
          arrival_time(arrival_time_),
          last_run_time(arrival_time_), 
          priority(priority_),
          deadline(deadline_) {}
};