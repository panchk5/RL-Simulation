#pragma once
#include <cstdint>
#include <vector>
#include <deque>

enum class EventType {
    JOB_ARRIVAL,
    JOB_COMPLETION,
    TIME_SLICE_EXPIRATION,
    NONE
};

struct Event {
    double time;
    EventType event_type;
    uint64_t job_id;

    Event(double time_, EventType event_type_, uint64_t job_id_)
        : time(time_),
          event_type(event_type_),
          job_id(job_id_) {}
};

struct Job;

class EventEngine {
public:
    EventEngine();
    
    void add_event(const Event& event);
    Event observe_event();
    
    // Methods to create events from jobs
    Event create_arrival_event(const Job& job);
    Event create_completion_event(const Job& job, double completion_time);
    Event create_time_slice_expiration_event(const Job& job, double expiration_time);

private:
    std::deque<Event> events;
};

