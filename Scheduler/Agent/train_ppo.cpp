#include "ppo_agent.h"
#include "../../Environment/Jobs/job.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <limits>
#include <thread>
#include <mutex>
#include <fstream>
#include <sstream>
#include <chrono>

// Helper: average multiple model files saved by PPOAgent::save_model
bool average_models(const std::vector<std::string>& paths, const std::string& out_path) {
    if (paths.empty()) return false;

    struct NetSpec { size_t in, out; } policy_spec{0,0}, value_spec{0,0};
    std::vector<std::vector<std::vector<double>>> policy_weights; // [worker][out][in]
    std::vector<std::vector<double>> policy_biases; // [worker][out]
    std::vector<std::vector<std::vector<double>>> value_weights;
    std::vector<std::vector<double>> value_biases;

    for (const auto& p : paths) {
        std::ifstream f(p);
        if (!f.is_open()) return false;
        std::string tag;
        f >> tag;
        if (tag != "POLICY") return false;
        size_t in_size, out_size;
        f >> in_size >> out_size;
        if (policy_weights.empty()) {
            policy_spec.in = in_size; policy_spec.out = out_size;
        } else {
            if (policy_spec.in != in_size || policy_spec.out != out_size) return false;
        }

        std::vector<std::vector<double>> pw(out_size, std::vector<double>(in_size));
        std::vector<double> pb(out_size);
        for (size_t i = 0; i < out_size; ++i) {
            for (size_t j = 0; j < in_size; ++j) f >> pw[i][j];
            f >> pb[i];
        }

        f >> tag; // VALUE
        if (tag != "VALUE") return false;
        size_t vin, vout; f >> vin >> vout;
        if (value_weights.empty()) {
            value_spec.in = vin; value_spec.out = vout;
        } else {
            if (value_spec.in != vin || value_spec.out != vout) return false;
        }
        std::vector<std::vector<double>> vw(vout, std::vector<double>(vin));
        std::vector<double> vb(vout);
        for (size_t i = 0; i < vout; ++i) {
            for (size_t j = 0; j < vin; ++j) f >> vw[i][j];
            f >> vb[i];
        }

        policy_weights.push_back(pw);
        policy_biases.push_back(pb);
        value_weights.push_back(vw);
        value_biases.push_back(vb);
    }

    size_t W = policy_weights.size();
    // average
    std::vector<std::vector<double>> avg_pb(policy_spec.out, std::vector<double>(1));
    std::vector<std::vector<std::vector<double>>> avg_pw(policy_spec.out, std::vector<std::vector<double>>(policy_spec.in, std::vector<double>(1)));
    for (size_t o = 0; o < policy_spec.out; ++o) {
        for (size_t i = 0; i < policy_spec.in; ++i) {
            double s = 0.0;
            for (size_t w = 0; w < W; ++w) s += policy_weights[w][o][i];
            avg_pw[o][i][0] = s / W;
        }
    }
    for (size_t o = 0; o < policy_spec.out; ++o) {
        double s = 0.0;
        for (size_t w = 0; w < W; ++w) s += policy_biases[w][o];
        avg_pb[o][0] = s / W;
    }

    // value averages
    std::vector<std::vector<std::vector<double>>> avg_vw(value_spec.out, std::vector<std::vector<double>>(value_spec.in, std::vector<double>(1)));
    std::vector<std::vector<double>> avg_vb(value_spec.out, std::vector<double>(1));
    for (size_t o = 0; o < value_spec.out; ++o) {
        for (size_t i = 0; i < value_spec.in; ++i) {
            double s = 0.0;
            for (size_t w = 0; w < W; ++w) s += value_weights[w][o][i];
            avg_vw[o][i][0] = s / W;
        }
    }
    for (size_t o = 0; o < value_spec.out; ++o) {
        double s = 0.0;
        for (size_t w = 0; w < W; ++w) s += value_biases[w][o];
        avg_vb[o][0] = s / W;
    }

    // write averaged model in same format as PPOAgent::save_model
    std::ofstream out(out_path);
    if (!out.is_open()) return false;
    out << "POLICY\n";
    out << policy_spec.in << " " << policy_spec.out << "\n";
    for (size_t i = 0; i < policy_spec.out; ++i) {
        for (size_t j = 0; j < policy_spec.in; ++j) {
            out << avg_pw[i][j][0] << " ";
        }
        out << "\n";
        out << avg_pb[i][0] << "\n";
    }
    out << "VALUE\n";
    out << value_spec.in << " " << value_spec.out << "\n";
    for (size_t i = 0; i < value_spec.out; ++i) {
        for (size_t j = 0; j < value_spec.in; ++j) {
            out << avg_vw[i][j][0] << " ";
        }
        out << "\n";
        out << avg_vb[i][0] << "\n";
    }
    out.close();
    return true;
}

int main() {
    std::cout << "PPO Agent Training\n";
    std::cout << "==================\n\n";
    
    std::vector<Job> training_jobs;

    // Generate 20 jobs with varying CPU requirements and priorities.
    // This increases scenario complexity for training.
    for (uint64_t i = 1; i <= 20; ++i) {
        double remaining_cpu = (double)((i % 5) + 1) * 2.0; // 2,4,6,8,10
        double arrival = 0.0;
        int priority = (int)((i % 10) + 1); // priorities 1..10
        double deadline = remaining_cpu * 5.0 + 10.0; // simple deadline heuristic
        training_jobs.push_back(Job(i, remaining_cpu, arrival, priority, deadline));
    }

    double max_time = 200.0;
    double time_slice = 1.0;
    int num_episodes = 3000;
    
    std::cout << "Training Configuration:\n";
    std::cout << "  Episodes: " << num_episodes << "\n";
    std::cout << "  Max time per episode: " << max_time << "\n";
    std::cout << "  Time slice: " << time_slice << "\n";
    std::cout << "  Number of jobs: " << training_jobs.size() << "\n\n";
    
    // Distributed training with shared parameters (simple in-process parameter server)
    std::cout << "Starting distributed training with shared parameters (4 workers)...\n\n";
    int num_workers = 4;
    int episodes_per_worker = num_episodes / num_workers;
    int sync_chunk = 50; // each worker trains this many episodes before syncing

    // Initialize shared parameters from a seed agent
    PPOAgent seed_agent;
    std::vector<std::vector<double>> shared_policy_w;
    std::vector<double> shared_policy_b;
    std::vector<std::vector<double>> shared_value_w;
    std::vector<double> shared_value_b;
    seed_agent.get_policy_params(shared_policy_w, shared_policy_b);
    seed_agent.get_value_params(shared_value_w, shared_value_b);

    std::mutex param_mutex;

    auto worker_fn = [&](int wid) {
        PPOAgent local_agent;

        int episodes_done = 0;
        while (episodes_done < episodes_per_worker) {
            // Pull shared params
            {
                std::lock_guard<std::mutex> lg(param_mutex);
                local_agent.set_policy_params(shared_policy_w, shared_policy_b);
                local_agent.set_value_params(shared_value_w, shared_value_b);
            }

            int chunk = std::min(sync_chunk, episodes_per_worker - episodes_done);
            local_agent.train(chunk, training_jobs, max_time, time_slice);
            episodes_done += chunk;

            // Push local params back by averaging into shared params
            std::vector<std::vector<double>> local_pw; std::vector<double> local_pb;
            std::vector<std::vector<double>> local_vw; std::vector<double> local_vb;
            local_agent.get_policy_params(local_pw, local_pb);
            local_agent.get_value_params(local_vw, local_vb);

            {
                std::lock_guard<std::mutex> lg(param_mutex);
                // simple averaging: shared = 0.5 * shared + 0.5 * local
                for (size_t o = 0; o < shared_policy_w.size(); ++o) {
                    for (size_t i = 0; i < shared_policy_w[o].size(); ++i) {
                        shared_policy_w[o][i] = 0.5 * shared_policy_w[o][i] + 0.5 * local_pw[o][i];
                    }
                    shared_policy_b[o] = 0.5 * shared_policy_b[o] + 0.5 * local_pb[o];
                }
                for (size_t o = 0; o < shared_value_w.size(); ++o) {
                    for (size_t i = 0; i < shared_value_w[o].size(); ++i) {
                        shared_value_w[o][i] = 0.5 * shared_value_w[o][i] + 0.5 * local_vw[o][i];
                    }
                    shared_value_b[o] = 0.5 * shared_value_b[o] + 0.5 * local_vb[o];
                }
            }
        }
        // Worker done
    };

    std::vector<std::thread> workers2;
    for (int w = 0; w < num_workers; ++w) workers2.emplace_back(worker_fn, w);
    for (auto &t : workers2) t.join();

    // Save shared model
    PPOAgent final_agent;
    final_agent.set_policy_params(shared_policy_w, shared_policy_b);
    final_agent.set_value_params(shared_value_w, shared_value_b);
    final_agent.save_model("ppo_model.txt");
    std::cout << "Saved shared model to ppo_model.txt\n";

    std::cout << "\nDistributed training (shared params) complete!\n";
    return 0;
}
