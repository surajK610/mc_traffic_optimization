// schwefel_cuda.hpp file

#ifndef SCHWEFEL_CUDA
#define SCHWEFEL_CUDA

#include "schwefel_cuda.hpp"
#include <cmath>
#include <stdio.h>
#include <iostream>
#include <typeinfo>
#include <limits>
#include <cuda.h>
#include <curand.h>
#include <curand_kernel.h>
#include <cuda_runtime.h>

const int DIM = 10;

__global__ void setup_kernel(curandState *state, unsigned long seed) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x;  
    if (idx < DIM)
      curand_init(seed, idx, 0, &state[idx]);          
}

__global__ void f_kernel(double* x, double* result) {
    int idx = threadIdx.x + blockIdx.x * blockDim.x; 
    if (idx < DIM) {
      result[idx] = 500 * x[idx] * sin(sqrt(fabs(500 * x[idx])));
    }
}

__global__ void step_kernel(double* y, const double* x, float tgen, curandState *state) {
  int idx = threadIdx.x + blockIdx.x * blockDim.x; 
  if (idx < DIM) {
    curandState localState = state[idx];         
    double randVal = curand_uniform(&localState); 
    y[idx] = fmod(x[idx] + tgen * tan(M_PI * (randVal - 0.5)), 1.0);
    state[idx] = localState;                      
  }
}

double f_c(void* instance, double* x) {
  double *d_x, *d_result;
  double* h_result = new double[DIM];

  cudaMalloc((void**) &d_x, DIM * sizeof(double));
  cudaMalloc((void**) &d_result, DIM * sizeof(double));
  cudaMemcpy(d_x, x, DIM * sizeof(double), cudaMemcpyHostToDevice);

  dim3 blockSize(256, 1, 1);
  dim3 numBlocks((DIM + blockSize.x - 1) / blockSize.x, 1, 1);
  f_kernel<<<numBlocks, blockSize>>>(d_x, d_result);
  cudaMemcpy(h_result, d_result, DIM * sizeof(double), cudaMemcpyDeviceToHost);

  double sum = 0.;
  for (int i = 0; i < DIM; ++i)
      sum += h_result[i];

  delete[] h_result;
  cudaFree(d_x);
  cudaFree(d_result);

  return 418.9829 * DIM - sum;
}

void step_c(void* instance, double* y, const double* x, float tgen, curandState *dev_states) {
  double *d_x, *d_y;

  cudaMalloc((void**) &d_x, DIM * sizeof(double));
  cudaMalloc((void**) &d_y, DIM * sizeof(double));
  cudaMemcpy(d_x, x, DIM * sizeof(double), cudaMemcpyHostToDevice);

  dim3 blockSize(256, 1, 1);
  dim3 numBlocks((DIM + blockSize.x - 1) / blockSize.x, 1, 1);
  step_kernel<<<numBlocks, blockSize>>>(d_y, d_x, tgen, dev_states);
  
  cudaMemcpy(y, d_y, DIM * sizeof(double), cudaMemcpyDeviceToHost);

  cudaFree(d_x);
  cudaFree(d_y);
}

#endif