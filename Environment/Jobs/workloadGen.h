#pragma once

#include <random>
#include <vector>
#include <cstdint>
#include <limits>
#include "job.h"

class JobGenerator {
public:
    JobGenerator(
        uint64_t seed,
        double arrival_rate,   
        double pareto_alpha,    
        double min_job_size     
    );

    std::vector<Job> generate_jobs(double current_time);

private:
    uint64_t next_job_id;

    std::mt19937_64 rng;
    std::exponential_distribution<double> arrival_dist;

    double alpha;
    double min_job_size;
    double next_arrival_time;

    double sample_job_size();
};
