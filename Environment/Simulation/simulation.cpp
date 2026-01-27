#include "simulation.h"
#include "../Event/event.h"
#include "../Jobs/workloadGen.h"
#include "../Jobs/job.h"
#include <vector>


Simulation::Simulation() 
    : event_engine(), 
      job_generator(42, 0.5, 1.5, 1.0),
      fifo_scheduler(),
      current_time(0.0),
      total_cpu_time_used(0.0),
      simulation_start_time(0.0) {
}

double Simulation::get_cpu_utilization() const {
    if (current_time <= simulation_start_time) {
        return 0.0;
    }
    double elapsed_time = current_time - simulation_start_time;
    return (total_cpu_time_used / elapsed_time) * 100.0;  // Percentage
}

Job* Simulation::get_job(uint64_t job_id) {
    return fifo_scheduler.get_job(job_id);
}

const Job* Simulation::get_job(uint64_t job_id) const {
    return fifo_scheduler.get_job(job_id);
}

void Simulation::run() {
    std::vector<Job> jobs = job_generator.generate_jobs(current_time);
    
    for (const auto& job : jobs) {
        Event arrival_event = event_engine.create_arrival_event(job);
        event_engine.add_event(arrival_event);
    }

    // agent loop
    while (true) {
        Event next_event = event_engine.observe_event();
        current_time = next_event.time;
        // TODO: implement agent logic
        fifo_scheduler.schedule_next(current_time, 1.0);
    }
}