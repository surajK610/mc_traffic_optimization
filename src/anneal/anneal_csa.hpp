#ifndef ANNEAL_CSA
#define ANNEAL_CSA

#include <cmath>
#include <omp.h>
#include <vector>
#include "context.hpp"
#include <sys/time.h>

namespace CSA {

class SolverCoupled : public BaseSolver
{
public:
    int m = 4; // number of threads
    int max_iters = 1000000;
    float tgen_initial = 0.01;
    float tgen_schedule = 0.99999;
    float tacc_initial = 0.9;
    float tacc_schedule = 0.01;
    float desired_variance = 0.99;

    // Timing variables for each operation
    long total_fx_time = 0.0;
    int fx_count = 0;
    long total_step_time = 0.0;
    int step_count = 0;
    long total_param_time = 0.0;
    int param_count = 0;


    SolverMultipleST() {  };
    
    void writeTimingsToCSV(const std::string& filename) {
        std::ofstream file;
        file.open(filename);

        file << "Operation,Total Time (microseconds),Count\n";

        file << "fx," << total_fx_time << "," << fx_count << "\n";
        file << "step," << total_step_time << "," << step_count << "\n";
        file << "parameter updates," << total_param_time << "," << param_count << "\n";

        file.close();
    }


    inline int minimize(
        int n, // number of dimensions
        double* x,
        double (*fx)(void*, double*),
        void (*step)(void*, double* y, const double*, float tgen),
        void (*progress)(void*,
                         double cost,
                         float tgen,
                         float tacc,
                         int opt_id,
                         int iter),
        void* instance)
    {
        double fx0 = fx(instance, x);

        // Initialize shared values.
        SharedStates shared_states(this->m, n, x, fx0);
        float tacc = this->tacc_initial;
        float tgen = this->tgen_initial;
        float tmp, sum_a, prob_var, gamma = m;

        omp_lock_t lock;
        omp_init_lock(&lock);

        #pragma omp parallel shared(n, shared_states, tacc, tgen, gamma) num_threads(this->m)
        {
            int k, opt_id = omp_get_thread_num();

            double max_cost = shared_states[0].cost;
            double cost;
            std::vector<double> y(n, double(0));
            float unif, prob;

            #pragma omp for
            for (int iter = 0; iter < this->max_iters; ++iter) {

                gettimeofday(&start, 0);
                step(instance, y.data(), shared_states[opt_id].x.data(), tgen);
                gettimeofday(&end, 0);
                total_step_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
                step_count++;

                gettimeofday(&start, 0);
                cost = fx(instance, y.data());
                gettimeofday(&end, 0);
                total_fx_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
                fx_count++;

                gettimeofday(&start, 0);
                if (cost < shared_states[opt_id].cost) {
                    omp_set_lock(&lock);

                    if (cost < shared_states[opt_id].best_cost) {
                        shared_states[opt_id].best_cost = cost;
                        shared_states[opt_id].best_x = y;
                        if (progress != nullptr)
                            progress(instance, cost, tgen, tacc, opt_id, iter);
                    }

                    shared_states[opt_id].step(y, cost);
                    omp_unset_lock(&lock);
                } else {

                    unif = drand48();
                    prob = std::exp((shared_states[opt_id].cost - max_cost) / tacc) / gamma;
                    if (prob > unif) {
                        omp_set_lock(&lock);
                        shared_states[opt_id].step(y, cost);
                        omp_unset_lock(&lock);
                    }
                }

                if (omp_test_lock(&lock)) {
                    max_cost = shared_states[0].cost;
                    for (k = 0; k < this->m; ++k)
                        if (shared_states[k].cost > max_cost)
                            max_cost = shared_states[k].cost;

                    gamma = sum_a = 0.;
                    for (k = 0; k < this->m; ++k) {
                        tmp = (shared_states[k].cost - max_cost) / tacc;
                        gamma += std::exp(tmp);
                        sum_a += std::exp(2.0 * tmp);
                    }
                    prob_var = (this->m * (sum_a / (gamma * gamma)) - 1.) /
                               (this->m * this->m);

                    if (prob_var > this->desired_variance)
                        tacc += this->tacc_schedule * tacc;
                    else
                        tacc -= this->tacc_schedule * tacc;
                    tgen = this->tgen_schedule * tgen;

                    omp_unset_lock(&lock);
                    gettimeofday(&end, 0);
                    total_param_time += (end.tv_sec - start.tv_sec) * 1000000 + (end.tv_usec - start.tv_usec);
                    param_count++;
                }
            }
        }

        int best_ind = 0;
        double best_cost = shared_states[0].best_cost;
        for (int k = 0; k < this->m; ++k) {
            if (shared_states[k].best_cost < best_cost) {
                best_cost = shared_states[k].best_cost;
                best_ind = k;
            }
        }
        Context best_state = shared_states[best_ind];
        for (int i = 0; i < n; ++i)
            x[i] = best_state.best_x[i];

        // Clean up.
        omp_destroy_lock(&lock);
        writeTimingsToCSV("outputs/timings_csa.csv");

        return 0;
    }
};  // class SolverCoupled
}  // namespace ANNEAL_OMP
#endif

