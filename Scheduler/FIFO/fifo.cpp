#include "fifo.h"
#include <algorithm>

FIFOScheduler::FIFOScheduler() : running_job_id(0) {
}

void FIFOScheduler::handle_job_arrival(const Job& job, double current_time) {
    auto it = job_map.find(job.id);
    if (it != job_map.end()) {
        it->second = job;
    } else {
        job_map.insert({job.id, job});
    }
    
    // Enqueue the job to ready queue when it arrives
    enqueue_job(job);
}

void FIFOScheduler::handle_job_completion(uint64_t job_id, double current_time) {
    if (running_job_id == job_id) {
        running_job_id = 0;  
    }
    
    remove_job(job_id);
}

void FIFOScheduler::handle_time_slice_expiration(uint64_t job_id, double current_time) {
    auto it = job_map.find(job_id);
    if (running_job_id == job_id && it != job_map.end()) {
        
        it->second.last_run_time = current_time;
        
        running_job_id = 0;
        enqueue_job(it->second);
    }
}

std::pair<Job*, double> FIFOScheduler::schedule_next(double current_time, double time_slice) {
   
    if (running_job_id != 0) {
        return {nullptr, 0.0};
    }
    
    
    if (ready_queue.empty()) {
        return {nullptr, 0.0};
    }
    
 
    Job& job = ready_queue.front();
    ready_queue.pop_front();
    
   
    job.last_run_time = current_time;
    
    double run_time = std::min(time_slice, job.remaining_cpu);
    
    running_job_id = job.id;
    
    auto it = job_map.find(job.id);
    if (it != job_map.end()) {
        return {&it->second, run_time};
    }
    return {nullptr, 0.0};
}

std::pair<Job*, double> FIFOScheduler::schedule_job(uint64_t job_id, double current_time, double time_slice) {
    if (running_job_id != 0) {
        return {nullptr, 0.0};
    }

    // Find job in ready_queue
    auto it = std::find_if(ready_queue.begin(), ready_queue.end(),
                           [job_id](const Job& j) { return j.id == job_id; });
    if (it == ready_queue.end()) {
        return {nullptr, 0.0};
    }

    Job job = *it; // copy
    // Remove from ready queue
    ready_queue.erase(it);

    job.last_run_time = current_time;
    double run_time = std::min(time_slice, job.remaining_cpu);

    running_job_id = job.id;

    auto map_it = job_map.find(job.id);
    if (map_it != job_map.end()) {
        return {&map_it->second, run_time};
    }
    return {nullptr, 0.0};
}

bool FIFOScheduler::has_ready_jobs() const {
    return !ready_queue.empty();
}

size_t FIFOScheduler::ready_queue_size() const {
    return ready_queue.size();
}

uint64_t FIFOScheduler::get_running_job_id() const {
    return running_job_id;
}

void FIFOScheduler::enqueue_job(const Job& job) {
    auto it = job_map.find(job.id);
    if (it != job_map.end()) {
        it->second = job;
    } else {
        job_map.insert({job.id, job});
    }
    
    bool already_in_queue = false;
    for (const auto& queued_job : ready_queue) {
        if (queued_job.id == job.id) {
            already_in_queue = true;
            break;
        }
    }
    
    if (!already_in_queue) {
        ready_queue.push_back(job);
    }
}

bool FIFOScheduler::update_job_after_run(uint64_t job_id, double run_time) {
    auto it = job_map.find(job_id);
    if (it == job_map.end()) {
        return false;  
    }
    
    Job& job = it->second;
    job.remaining_cpu -= run_time;
    
    // Check if job completed
    if (job.remaining_cpu <= 0.0) {
        job.remaining_cpu = 0.0;  
        running_job_id = 0;  
        return true; 
    }
    
    return false;  
}

void FIFOScheduler::remove_job(uint64_t job_id) {
   
    job_map.erase(job_id);
    
    ready_queue.erase(
        std::remove_if(ready_queue.begin(), ready_queue.end(),
            [job_id](const Job& j) { return j.id == job_id; }),
        ready_queue.end()
    );
}

Job* FIFOScheduler::get_job(uint64_t job_id) {
    auto it = job_map.find(job_id);
    if (it != job_map.end()) {
        return &it->second;
    }
    return nullptr;
}

const Job* FIFOScheduler::get_job(uint64_t job_id) const {
    auto it = job_map.find(job_id);
    if (it != job_map.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<uint64_t> FIFOScheduler::get_ready_job_ids() const {
    std::vector<uint64_t> job_ids;
    job_ids.reserve(ready_queue.size());
    
    for (const auto& job : ready_queue) {
        job_ids.push_back(job.id);
    }
    
    return job_ids;
}