#include "workloadGen.h"
#include "job.h"
#include <cmath>
#include <limits>

JobGenerator::JobGenerator(
    uint64_t seed,
    double arrival_rate,
    double pareto_alpha,
    double min_job_size_)
    : next_job_id(0),
      rng(seed),
      arrival_dist(arrival_rate),
      alpha(pareto_alpha),
      min_job_size(min_job_size_),
      next_arrival_time(0.0)
{
}

std::vector<Job> JobGenerator::generate_jobs(double current_time) {
    std::vector<Job> jobs;

    while (next_arrival_time <= current_time) {
        double arrival_time = next_arrival_time;

        Job j(
            next_job_id++,
            sample_job_size(),
            arrival_time,
            0, // priority
            std::numeric_limits<double>::infinity()
        );

        jobs.push_back(j);

        next_arrival_time += arrival_dist(rng);
    }

    return jobs;
}

double JobGenerator::sample_job_size() {
    std::uniform_real_distribution<double> uniform(0.0, 1.0);
    double u = uniform(rng);
    return min_job_size * std::pow(1.0 - u, -1.0 / alpha);
}