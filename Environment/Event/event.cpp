#include "event.h"
#include "../Jobs/job.h"

EventEngine::EventEngine() {
    events = std::deque<Event>();
}

void EventEngine::add_event(const Event& event) {
    events.push_back(event);
}

Event EventEngine::observe_event() {
    if (events.empty()) {
        return Event(0.0, EventType::NONE, 0);
    }
    Event event = events.front();
    events.pop_front();
    return event;
}

Event EventEngine::create_arrival_event(const Job& job) {
    return Event(job.arrival_time, EventType::JOB_ARRIVAL, job.id);
}

Event EventEngine::create_completion_event(const Job& job, double completion_time) {
    return Event(completion_time, EventType::JOB_COMPLETION, job.id);
}

Event EventEngine::create_time_slice_expiration_event(const Job& job, double expiration_time) {
    return Event(expiration_time, EventType::TIME_SLICE_EXPIRATION, job.id);
}



