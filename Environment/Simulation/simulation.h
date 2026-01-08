#pragma once

#include "../Event/event.h"
#include "../Jobs/workloadGen.h"
#include "../../Scheduler/FIFO/fifo.h"

class Simulation {
public:
    Simulation();
    void run();
    
private:
    EventEngine event_engine;
    JobGenerator job_generator;
    FIFOScheduler fifo_scheduler;
    double current_time;
};

