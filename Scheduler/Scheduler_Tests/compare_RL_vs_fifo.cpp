#include "../Agent/ppo_agent.h"
#include "../../Environment/Simulation/simulation.h"
#include "../../Environment/Jobs/job.h"
#include "../../Environment/Event/event.h"
#include "../../Scheduler/FIFO/fifo.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <numeric>
#include <limits>
#include <cmath>
#include <random>

struct SimulationMetrics {
    double total_time;
    int jobs_completed;
    double avg_wait_time;
    double avg_completion_time;
    double max_wait_time;
    double cpu_utilization;
    double total_reward;  // For PPO agent
    std::vector<uint64_t> job_order;  // Order in which jobs were scheduled
    
    SimulationMetrics() 
        : total_time(0.0), jobs_completed(0), avg_wait_time(0.0),
          avg_completion_time(0.0), max_wait_time(0.0), cpu_utilization(0.0),
          total_reward(0.0) {}
};

struct JobCompletion {
    uint64_t job_id;
    double arrival_time;
    double completion_time;
    double wait_time;
    int priority;
    
    JobCompletion(uint64_t id, double arrival, double completion, int prio)
        : job_id(id), arrival_time(arrival), completion_time(completion),
          wait_time(completion - arrival), priority(prio) {}
};

SimulationMetrics run_fifo_simulation(const std::vector<Job>& jobs, double max_time, double time_slice) {
    SimulationMetrics metrics;
    FIFOScheduler scheduler;
    EventEngine event_engine;
    
    std::vector<JobCompletion> completions;
    double current_time = 0.0;
    double total_cpu_time = 0.0;
    
    // Add all jobs to scheduler and create events
    for (const auto& job : jobs) {
        scheduler.handle_job_arrival(job, job.arrival_time);
        Event arrival = event_engine.create_arrival_event(job);
        event_engine.add_event(arrival);
    }
    
    // Run simulation
    while (current_time < max_time) {
        Event next_event = event_engine.observe_event();
        if (next_event.event_type == EventType::NONE) {
            break;
        }
        
        current_time = next_event.time;
        
        // Handle events
        if (next_event.event_type == EventType::JOB_ARRIVAL) {
            // Already handled
        } else if (next_event.event_type == EventType::TIME_SLICE_EXPIRATION) {
            scheduler.handle_time_slice_expiration(next_event.job_id, current_time);
        } else if (next_event.event_type == EventType::JOB_COMPLETION) {
            scheduler.handle_job_completion(next_event.job_id, current_time);
        }
        
        // Schedule next job if CPU is idle
        if (scheduler.get_running_job_id() == 0 && scheduler.has_ready_jobs()) {
            auto result = scheduler.schedule_next(current_time, time_slice);
            
            if (result.first != nullptr) {
                Job* job = result.first;
                double run_time = result.second;
                total_cpu_time += run_time;
                
                double completion_time = current_time + run_time;
                bool completed = scheduler.update_job_after_run(job->id, run_time);
                
                if (completed) {
                    // Store job info before removing
                    uint64_t job_id = job->id;
                    double arrival = job->arrival_time;
                    int priority = job->priority;
                    
                    scheduler.handle_job_completion(job->id, completion_time);
                    completions.push_back(JobCompletion(job_id, arrival, completion_time, priority));
                    metrics.job_order.push_back(job_id);
                    metrics.jobs_completed++;
                    
                    Event completion = event_engine.create_completion_event(*job, completion_time);
                    event_engine.add_event(completion);
                } else {
                    Event expiration = event_engine.create_time_slice_expiration_event(
                        *job, completion_time);
                    event_engine.add_event(expiration);
                }
            }
        }
    }
    
    metrics.total_time = current_time;
    
    // Calculate metrics
    if (!completions.empty()) {
        double total_wait = 0.0;
        double max_wait = 0.0;
        
        for (const auto& comp : completions) {
            total_wait += comp.wait_time;
            max_wait = std::max(max_wait, comp.wait_time);
        }
        
        metrics.avg_wait_time = total_wait / completions.size();
        metrics.max_wait_time = max_wait;
        
        double total_completion = 0.0;
        for (const auto& comp : completions) {
            total_completion += comp.completion_time - comp.arrival_time;
        }
        metrics.avg_completion_time = total_completion / completions.size();
    }
    
    if (current_time > 0) {
        metrics.cpu_utilization = (total_cpu_time / current_time) * 100.0;
    }
    
    return metrics;
}

SimulationMetrics run_ppo_simulation(const std::vector<Job>& jobs, double max_time, double time_slice, bool use_trained = false) {
    SimulationMetrics metrics;
    EventEngine event_engine;
    PPOAgent agent;
    
    // Load trained model if available
    if (use_trained) {
        agent.load_model("ppo_model.txt");
        std::cout << "  Loaded trained PPO model\n";
    }
    
    Simulation sim;  // Use actual Simulation class
    
    std::vector<JobCompletion> completions;
    double current_time = 0.0;
    double total_cpu_time = 0.0;
    
    // Add all jobs to simulation's scheduler and create events
    FIFOScheduler& scheduler = sim.get_scheduler();
    for (const auto& job : jobs) {
        scheduler.handle_job_arrival(job, job.arrival_time);
        Event arrival = event_engine.create_arrival_event(job);
        event_engine.add_event(arrival);
    }
    
    // Run simulation
    while (current_time < max_time) {
        Event next_event = event_engine.observe_event();
        if (next_event.event_type == EventType::NONE) {
            break;
        }
        
        current_time = next_event.time;
        
        // Note: We can't directly set Simulation's time, so we'll work around this
        // by using the scheduler directly and manually tracking state
        
        // Handle events
        if (next_event.event_type == EventType::JOB_ARRIVAL) {
            // Already handled
        } else if (next_event.event_type == EventType::TIME_SLICE_EXPIRATION) {
            scheduler.handle_time_slice_expiration(next_event.job_id, current_time);
        } else if (next_event.event_type == EventType::JOB_COMPLETION) {
            scheduler.handle_job_completion(next_event.job_id, current_time);
        }
        
        // Use PPO agent to select action if CPU is idle
        if (scheduler.get_running_job_id() == 0 && scheduler.has_ready_jobs()) {
            // Use agent to select action (agent will use sim's scheduler)
            std::pair<uint64_t, double> action = agent.select_action(sim);
            uint64_t selected_job_id = action.first;
            double selected_time_slice = action.second;
            
            // Validate selected action
            std::vector<uint64_t> available_jobs = scheduler.get_ready_job_ids();
            if (selected_job_id == 0 || 
                std::find(available_jobs.begin(), available_jobs.end(), selected_job_id) 
                == available_jobs.end()) {
                // Agent selected invalid action, use FIFO fallback
                if (!available_jobs.empty()) {
                    selected_job_id = available_jobs[0];
                    selected_time_slice = time_slice;
                } else {
                    continue;
                }
            }
            
            // Schedule the selected job using scheduler (try specific job then fallback)
            std::pair<Job*, double> schedule_result = scheduler.schedule_job(selected_job_id, current_time, selected_time_slice);
            if (schedule_result.first == nullptr) {
                schedule_result = scheduler.schedule_next(current_time, selected_time_slice);
            }

            if (schedule_result.first != nullptr) {
                Job* job = schedule_result.first;
                double run_time = schedule_result.second;
                total_cpu_time += run_time;
                
                double completion_time = current_time + run_time;
                bool completed = scheduler.update_job_after_run(job->id, run_time);
                
                // Store job info before potential removal
                uint64_t job_id = job->id;
                double arrival = job->arrival_time;
                int priority = job->priority;
                
                // Calculate reward and record experience using agent
                // We'll manually create states since we can't easily sync Simulation time
                SimState state_before(sim.get_current_time(), sim.get_cpu_utilization());
                double reward = agent.calculate_reward(sim, job_id, run_time);
                metrics.total_reward += reward;
                
                // Record experience
                SimState state_after(sim.get_current_time(), sim.get_cpu_utilization());
                agent.record_experience(state_before, job_id, run_time, reward, state_after, false, sim);
                
                if (completed) {
                    scheduler.handle_job_completion(job_id, completion_time);
                    completions.push_back(JobCompletion(job_id, arrival, completion_time, priority));
                    metrics.job_order.push_back(job_id);
                    metrics.jobs_completed++;
                    
                    Event completion = event_engine.create_completion_event(*job, completion_time);
                    event_engine.add_event(completion);
                } else {
                    Event expiration = event_engine.create_time_slice_expiration_event(
                        *job, completion_time);
                    event_engine.add_event(expiration);
                }
            }
        }
    }
    
    metrics.total_time = current_time;
    
    // Calculate metrics
    if (!completions.empty()) {
        double total_wait = 0.0;
        double max_wait = 0.0;
        
        for (const auto& comp : completions) {
            total_wait += comp.wait_time;
            max_wait = std::max(max_wait, comp.wait_time);
        }
        
        metrics.avg_wait_time = total_wait / completions.size();
        metrics.max_wait_time = max_wait;
        
        double total_completion = 0.0;
        for (const auto& comp : completions) {
            total_completion += comp.completion_time - comp.arrival_time;
        }
        metrics.avg_completion_time = total_completion / completions.size();
    }
    
    if (current_time > 0) {
        metrics.cpu_utilization = (total_cpu_time / current_time) * 100.0;
    }
    
    return metrics;
}

void print_metrics(const std::string& name, const SimulationMetrics& metrics) {
    std::cout << "\n" << name << " Metrics:\n";
    std::cout << "  Total simulation time: " << std::fixed << std::setprecision(2) 
              << metrics.total_time << "\n";
    std::cout << "  Jobs completed: " << metrics.jobs_completed << "\n";
    std::cout << "  Average wait time: " << metrics.avg_wait_time << "\n";
    std::cout << "  Average completion time: " << metrics.avg_completion_time << "\n";
    std::cout << "  Max wait time: " << metrics.max_wait_time << "\n";
    std::cout << "  CPU utilization: " << metrics.cpu_utilization << "%\n";
    if (name == "PPO Agent") {
        std::cout << "  Total reward: " << metrics.total_reward << "\n";
    }
    if (!metrics.job_order.empty()) {
        std::cout << "  Job completion order: ";
        for (size_t i = 0; i < metrics.job_order.size(); ++i) {
            std::cout << metrics.job_order[i];
            if (i < metrics.job_order.size() - 1) std::cout << " -> ";
        }
        std::cout << "\n";
    }
}

void compare_schedulers() {
    std::cout << "PPO Agent vs FIFO Scheduler Comparison\n";
    std::cout << "======================================\n\n";
    
    // Create a more challenging, randomized job set (20 jobs)
    std::vector<Job> test_jobs;
    double max_time = 200.0;
    double time_slice = 1.0;

    const int num_jobs = 20;
    // Reproducible RNG for consistent runs; change seed to vary experiments
    std::mt19937 rng(12345);
    std::uniform_real_distribution<double> arrival_dist(0.0, max_time * 0.5);
    std::uniform_int_distribution<int> cpu_dist(1, 20); // longer jobs up to 20 units
    std::uniform_int_distribution<int> prio_dist(1, 10);
    std::uniform_real_distribution<double> deadline_scale(1.5, 4.0);

    for (uint64_t i = 1; i <= (uint64_t)num_jobs; ++i) {
        double remaining_cpu = static_cast<double>(cpu_dist(rng));
        double arrival = arrival_dist(rng);
        int priority = prio_dist(rng);
        double deadline = arrival + remaining_cpu * deadline_scale(rng) + (i % 5) * 2.0;
        test_jobs.push_back(Job(i, remaining_cpu, arrival, priority, deadline));
    }
    
    std::cout << "Test Configuration:\n";
    std::cout << "  Number of jobs: " << test_jobs.size() << "\n";
    std::cout << "  Max simulation time: " << max_time << "\n";
    std::cout << "  Time slice: " << time_slice << "\n\n";
    
    std::cout << "Job Details:\n";
    for (const auto& job : test_jobs) {
        std::cout << "  Job " << job.id << ": remaining_cpu=" << job.remaining_cpu 
                  << ", priority=" << job.priority 
                  << ", arrival=" << job.arrival_time << "\n";
    }
    
    // Run FIFO simulation
    std::cout << "\nRunning FIFO Scheduler simulation...\n";
    SimulationMetrics fifo_metrics = run_fifo_simulation(test_jobs, max_time, time_slice);
    print_metrics("FIFO Scheduler", fifo_metrics);
    
    // Run PPO simulation (untrained)
    std::cout << "\nRunning PPO Agent simulation (untrained)...\n";
    SimulationMetrics ppo_metrics_untrained = run_ppo_simulation(test_jobs, max_time, time_slice, false);
    print_metrics("PPO Agent (Untrained)", ppo_metrics_untrained);
    
    // Run PPO simulation (trained)
    std::cout << "\nRunning PPO Agent simulation (trained)...\n";
    SimulationMetrics ppo_metrics_trained = run_ppo_simulation(test_jobs, max_time, time_slice, true);
    print_metrics("PPO Agent (Trained)", ppo_metrics_trained);
    
    // Comparison
    std::cout << "\n" << std::string(50, '=') << "\n";
    std::cout << "Comparison Summary:\n";
    std::cout << std::string(50, '=') << "\n";
    
    // Compare FIFO vs Trained PPO
    if (fifo_metrics.avg_wait_time > 0 && ppo_metrics_trained.avg_wait_time > 0) {
        double wait_improvement = ((fifo_metrics.avg_wait_time - ppo_metrics_trained.avg_wait_time) 
                                   / fifo_metrics.avg_wait_time) * 100.0;
        std::cout << "Average Wait Time:\n";
        std::cout << "  FIFO: " << std::fixed << std::setprecision(2) 
                  << fifo_metrics.avg_wait_time << "\n";
        std::cout << "  PPO (Untrained): " << ppo_metrics_untrained.avg_wait_time << "\n";
        std::cout << "  PPO (Trained):   " << ppo_metrics_trained.avg_wait_time << "\n";
        std::cout << "  Improvement: " << std::setprecision(1) << wait_improvement << "%\n\n";
    }
    
    if (fifo_metrics.max_wait_time > 0 && ppo_metrics_trained.max_wait_time > 0) {
        double max_wait_improvement = ((fifo_metrics.max_wait_time - ppo_metrics_trained.max_wait_time) 
                                       / fifo_metrics.max_wait_time) * 100.0;
        std::cout << "Max Wait Time:\n";
        std::cout << "  FIFO: " << std::fixed << std::setprecision(2) 
                  << fifo_metrics.max_wait_time << "\n";
        std::cout << "  PPO (Untrained): " << ppo_metrics_untrained.max_wait_time << "\n";
        std::cout << "  PPO (Trained):   " << ppo_metrics_trained.max_wait_time << "\n";
        std::cout << "  Improvement: " << std::setprecision(1) << max_wait_improvement << "%\n\n";
    }
    
    std::cout << "CPU Utilization:\n";
    std::cout << "  FIFO: " << std::fixed << std::setprecision(2) 
              << fifo_metrics.cpu_utilization << "%\n";
    std::cout << "  PPO (Untrained): " << ppo_metrics_untrained.cpu_utilization << "%\n";
    std::cout << "  PPO (Trained):   " << ppo_metrics_trained.cpu_utilization << "%\n\n";
    
    std::cout << "Jobs Completed:\n";
    std::cout << "  FIFO: " << fifo_metrics.jobs_completed << "\n";
    std::cout << "  PPO (Untrained): " << ppo_metrics_untrained.jobs_completed << "\n";
    std::cout << "  PPO (Trained):   " << ppo_metrics_trained.jobs_completed << "\n\n";
}

int main() {
    compare_schedulers();
    return 0;
}
