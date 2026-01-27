#include "agent.h"
#include "../../Environment/Simulation/simulation.h"
#include "../../Scheduler/FIFO/fifo.h"
#include <algorithm>
#include <limits>

Agent::Agent() {
}

SimState Agent::get_sim_state(const Simulation& sim) const {
    double current_time = sim.get_current_time();
    double cpu_utilization = sim.get_cpu_utilization();
    
    return SimState(current_time, cpu_utilization);
}

void Agent::action(uint64_t job_id, double time_slice, Simulation& sim) {
    FIFOScheduler& scheduler = sim.get_scheduler();
    
    // Check if CPU is idle
    if (scheduler.get_running_job_id() == 0) {
     
        const Job* job = sim.get_job(job_id);
        if (job != nullptr && scheduler.has_ready_jobs()) {
            // Placeholder 
        }
    }
}

JobSummary Agent::get_job_summary(uint64_t job_id, const Simulation& sim) const {
    const Job* job = sim.get_job(job_id);
    
    if (job == nullptr) {
        // Job not found, return default values
        return JobSummary(0.0, 0, 0.0);
    }
    
    double current_time = sim.get_current_time();
    double wait_time = current_time - job->arrival_time;
    int priority = job->priority;
    double remaining_cpu = job->remaining_cpu;
    
    return JobSummary(wait_time, priority, remaining_cpu);
}

void Agent::reset() {
    // placeholder for resetting internal state if needed
}
