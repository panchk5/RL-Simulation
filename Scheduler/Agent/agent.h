#pragma once

#include "../../Environment/Jobs/job.h"
#include <cstdint>
#include <memory>

// Forward declarations
class Simulation;

struct SimState {
    double current_time;
    double cpu_utilization;
    
    SimState(double time, double utilization)
        : current_time(time), cpu_utilization(utilization) {}
};

struct JobSummary {
    double wait_time;
    int priority;
    double remaining_cpu;
    
    JobSummary(double wait, int prio, double remaining)
        : wait_time(wait), priority(prio), remaining_cpu(remaining) {}
};

class Agent {
public:
    Agent();
    virtual ~Agent() = default;

    virtual SimState get_sim_state(const Simulation& sim) const;
    
    virtual void action(uint64_t job_id, double time_slice, Simulation& sim);
    
    virtual JobSummary get_job_summary(uint64_t job_id, const Simulation& sim) const;
    
    virtual void reset();
};
