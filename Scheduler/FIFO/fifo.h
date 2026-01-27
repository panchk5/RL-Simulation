#pragma once

#include "../../Environment/Jobs/job.h"
#include "../../Environment/Event/event.h"
#include <deque>
#include <unordered_map>
#include <vector>
#include <cstdint>

class FIFOScheduler {
public:
    FIFOScheduler();
    
    // Handle different event types
    void handle_job_arrival(const Job& job, double current_time);
    void handle_job_completion(uint64_t job_id, double current_time);
    void handle_time_slice_expiration(uint64_t job_id, double current_time);
    
    // Schedule next job from ready queue
    // Returns the job to run and the time slice (or remaining CPU if less)
    // Returns nullptr if no job available
    std::pair<Job*, double> schedule_next(double current_time, double time_slice);
    // Schedule a specific job by ID if it's in the ready queue
    std::pair<Job*, double> schedule_job(uint64_t job_id, double current_time, double time_slice);
    
    // Check if there are jobs in ready queue
    bool has_ready_jobs() const;
    
    // Get the number of jobs in ready queue
    size_t ready_queue_size() const;
    
    // Get currently running job ID (0 if none)
    uint64_t get_running_job_id() const;
    
    // Update job after running (decrements remaining_cpu)
    // Returns true if job completed, false otherwise
    bool update_job_after_run(uint64_t job_id, double run_time);
    
    // Get job by ID (for Agent API)
    Job* get_job(uint64_t job_id);
    const Job* get_job(uint64_t job_id) const;
    
    // Get IDs of jobs in ready queue (for Agent API)
    std::vector<uint64_t> get_ready_job_ids() const;
    
private:
    std::deque<Job> ready_queue;  // FIFO queue of ready jobs
    std::unordered_map<uint64_t, Job> job_map;  // Map of all jobs by ID
    uint64_t running_job_id;  // ID of currently running job (0 if none)
    
    // Helper to add job to ready queue
    void enqueue_job(const Job& job);
    
    // Helper to remove job from system
    void remove_job(uint64_t job_id);
};

