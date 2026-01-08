#include "simulation.h"
#include "../Event/event.h"
#include "../Jobs/workloadGen.h"
#include "../Jobs/job.h"
#include <vector>


Simulation::Simulation() 
    : event_engine(), 
      job_generator(42, 0.5, 1.5, 1.0),
      fifo_scheduler(),
      current_time(0.0) {
}

void Simulation::run() {
    // Generate jobs up to current time
    std::vector<Job> jobs = job_generator.generate_jobs(current_time);
    
    // Convert jobs to arrival events and add them to the event engine
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