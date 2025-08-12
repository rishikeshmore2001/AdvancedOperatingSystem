#include <cuda_runtime.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

__global__ void reduce_max_kernel(double *g_idata, double *g_odata, int n) {
    extern __shared__ double sdata[];
    int tid = threadIdx.x;
    int i = blockIdx.x * blockDim.x + threadIdx.x;

    // Load data into shared memory
    sdata[tid] = (i < n) ? g_idata[i] : -1e9;
    __syncthreads();

    // Perform reduction in shared memory
    for (int s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            if (sdata[tid + s] > sdata[tid]) {
                sdata[tid] = sdata[tid + s];
            }
        }
        __syncthreads();
    }

    // Write result for this block to global memory
    if (tid == 0) g_odata[blockIdx.x] = sdata[0];
}

// Internal: perform max reduction on GPU
double find_max_gpu(double *h_arr, int size) {
    double *d_in, *d_out;
    double max_val = -1e9;
    int threads = 256;
    int blocks = (size + threads - 1) / threads;
    int out_size = blocks;

    // Allocate device memory
    cudaMalloc(&d_in, size * sizeof(double));
    cudaMalloc(&d_out, blocks * sizeof(double));

    // Copy data to device
    cudaMemcpy(d_in, h_arr, size * sizeof(double), cudaMemcpyHostToDevice);

    // First reduction pass
    reduce_max_kernel<<<blocks, threads, threads * sizeof(double)>>>(d_in, d_out, size);
    cudaDeviceSynchronize();

    // Continue reducing until one block remains
    while (out_size > 1) {
        int new_blocks = (out_size + threads - 1) / threads;
        reduce_max_kernel<<<new_blocks, threads, threads * sizeof(double)>>>(d_out, d_out, out_size);
        cudaDeviceSynchronize();
        out_size = new_blocks;
    }

    // Copy final result back to host
    cudaMemcpy(&max_val, d_out, sizeof(double), cudaMemcpyDeviceToHost);

    // Free memory
    cudaFree(d_in);
    cudaFree(d_out);

    return max_val;
}

// RPC-callable: generate data + return max from GPU
extern "C"
double getmax_cpu(int N, double M, int seed) {
    int size = 1 << N;  // 2^N
    double *arr = (double *)malloc(size * sizeof(double));

    if (!arr) {
        fprintf(stderr, "Host allocation failed\n");
        return -1.0;
    }

    // Initialize array with exponential distribution
    srand(seed);
    for (int i = 0; i < size; ++i) {
        double U = (double)rand() / RAND_MAX;
        arr[i] = -M * log(1.0 - U);  // Exponential distribution
    }

    // Launch GPU kernel to compute max
    double result = find_max_gpu(arr, size);

    free(arr);
    return result;
}
