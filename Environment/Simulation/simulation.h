#pragma once

#include "../Event/event.h"
#include "../Jobs/workloadGen.h"
#include "../../Scheduler/FIFO/fifo.h"

class Simulation {
public:
    Simulation();
    void run();
    
    // Agent API: Get simulation state
    double get_current_time() const { return current_time; }
    double get_cpu_utilization() const;
    
    // Agent API: Get scheduler access
    FIFOScheduler& get_scheduler() { return fifo_scheduler; }
    const FIFOScheduler& get_scheduler() const { return fifo_scheduler; }
    
    // Agent API: Get job by ID (from scheduler)
    Job* get_job(uint64_t job_id);
    const Job* get_job(uint64_t job_id) const;
    
private:
    EventEngine event_engine;
    JobGenerator job_generator;
    FIFOScheduler fifo_scheduler;
    double current_time;
    double total_cpu_time_used;  // Track for utilization calculation
    double simulation_start_time;
};

