// main.cpp
#include "graph/graph.h"
#include "qubo/qubo.h"
#include <cmath>
#include <iostream>

#include <csa.hpp>


#define PI 3.14159265358979323846264338327
#define DIM 10


// This defines the Schwefel function. The pointer `instance` can be used to
// pass data to `f`, such as the dimension.
// In this case we don't need to pass anything, so it will be given as NULL.
double f(void* instance, double* x)
{
    double sum = 0.;
    for (int i = 0; i < DIM; ++i)
        sum += 500 * x[i] * std::sin(std::sqrt(std::fabs(500 * x[i])));
    return 418.9829 * DIM - sum;
}

// This function will take a random step from `x` to `y`. The value `tgen`,
// the "generation temperature", determines the variance of the distribution of
// the step. `tgen` will decrease according to fixed a schedule throughout the
// annealing process, which corresponds to a decrease in the variance of steps.
void step(void* instance, double* y, const double* x, float tgen)
{
    int i;
    double tmp;
    for (i = 0; i < DIM; ++i) {
        tmp = std::fmod(x[i] + tgen * std::tan(PI * (drand48() - 0.5)), 1.);
        y[i] = tmp;
    }
}

// This will receive progress updates from the CSA process and print updates
// to the terminal.
void progress(
    void* instance, double cost, float tgen, float tacc, int opt_id, int iter)
{
    printf(
        "bestcost=%1.3e \t tgen=%1.3e \t tacc=%1.3e \t thread=%d\n",
        cost,
        tgen,
        tacc,
        opt_id);
    return ;
}


int main()
{
    srand(0);

    double* x = new double[DIM];
    for (int i = 0; i < DIM; ++i)
        x[i] = drand48();
    double cost = f(nullptr, x);
    printf("Initial cost: %f\n", cost);

    CSA::Solver<double, double> solver;
    solver.m = 2;

    solver.minimize(DIM, x, f, step, progress, nullptr);

    cost = f(nullptr, x);
    printf("Best cost: %f\nx =\n", cost);
    for (int i = 0; i < DIM; ++i)
        std::cout << 500 * x[i] << " ";
    std::cout << std::endl;

    delete[] x;

    return EXIT_SUCCESS;
}