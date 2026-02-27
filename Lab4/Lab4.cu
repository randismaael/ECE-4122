/*
Author: Rand Ismaael
Class: ECE4122 
Last Date Modified: 11/03/2025
Description:
    This program simulates the 2D heat equation on a square plate using the Jacobi method.
    It initializes a grid with boundary conditions at 20 degrees celsius, performs a specified number of iterations
    to update the temperature values, and writes the final temperature distribution to a CSV file.
*/

//sources used:
//https://www.youtube.com/watch?v=bR2SEe8W3Ig
//https://docs.nvidia.com/cuda/cuda-c-programming-guide/
//https://www.youtube.com/watch?v=86FAWCzIe_4

#include <cuda_runtime.h>
#include <algorithm>   // for max, min
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>

/* 
Convert 2D indices to 1D index
    * i  = row index
    * j  = column index
    * n1 = total side length INCLUDING boundaries = N + 2
*/
__host__ __device__
inline int arr(int i, int j, int n1)
{
    return i * n1 + j; // turn 2d into 1d for gpu memory
}

struct Args 
{
    int N = 256;
    int I = 10000;
    bool quit = false;
};

/* 
Parse command line arguments
    * -N <int> : grid size
    * -I <int> : number of iterations
    * -q       : quit immediately
*/
Args parseArgs(int argc, char** argv)
{
    Args args;
    for (int i = 1; i < argc; ++i) 
    {
        std::string arg = argv[i];
        if (arg == "-N" && i + 1 < argc) 
        {
            args.N = std::stoi(argv[++i]);
        } 
        else if (arg == "-I" && i + 1 < argc) 
        {
            args.I = std::stoi(argv[++i]);
        } 
        else if (arg == "-q") 
        {
            args.quit = true;
        } 
        else 
        {
            std::cerr << "Unknown argument: " << arg << std::endl;
        }
    }
    if (args.N < 2)
    {
        args.N = 2; //minimum grid size
    }

    if (args.I < 1)
    {
        args.I = 1; //at least 1 iteration
    }
    return args;
}


/* 
One Jacobi iteration
    * p      = current grid of points
    * p_next = next grid
    * N      = interior size (N x N)
    * n1     = total side length INCLUDING boundaries = N + 2  (per spec)
 */
__global__
void jacobiIteration(const double* __restrict__ p, double* __restrict__ p_next, int N, int n1)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    int j = blockIdx.y * blockDim.y + threadIdx.y;

    if (i >= n1 || j >= n1) 
    {
        return; //boundary check
    }

    //make sure edges don't change
    if (i == 0 || j == 0 || i == n1 - 1 || j == n1 - 1) 
    {
        p_next[arr(i, j, n1)] = p[arr(i, j, n1)];
        return;
    }

    // Jacobi update
    double up = p[arr(i-1, j, n1)];
    double down = p[arr(i+1, j, n1)];
    double right = p[arr(i, j+1, n1)];
    double left = p[arr(i, j-1, n1)];

    p_next[arr(i, j, n1)] = 0.25 * (up + down + right + left);
}

/* 
Initialize plate on HOST:
    * - all edges 20 C
    * - 100 C segment in middle of the top edge
 */
void plate(std::vector<double>& H, int N, double cold = 20.0, double hot = 100.0)
{
    int n1 = N + 2; //account for edges
    H.assign(n1 * n1, 0.0);

    for (int j = 0; j < n1; ++j) 
    {
        H[arr(0,     j, n1)] = cold;
        H[arr(n1-1,  j, n1)] = cold;
    }

    for (int i = 0; i < n1; ++i) 
    {
        H[arr(i, 0,     n1)] = cold;
        H[arr(i, n1-1,  n1)] = cold;
    }

    int hotLength = std::max(1, N / 10);
    int hotStart = 1 + std::max(0, (N - hotLength) / 2);
    int hotEnd = std::min(n1 - 2, hotStart + hotLength - 1);

    for (int j = hotStart; j <= hotEnd; ++j) 
    {
        H[arr(0, j, n1)] = hot;
    }

}

/* 
Write 2D grid to CSV file
    * path = output file path
    * H    = grid data
    * N    = interior size 
 * Source: https://dev.to/arepp23/how-to-write-to-a-csv-file-in-c-1l5b
 */
void writeCSV(const char* path, const std::vector<double>& H, int N)
{
    FILE* file;

    file = fopen(path, "w");
    if (!file) { fprintf(stderr, "Failed to open %s\n", path); return; }
    int n1 = N + 2; //account for edges

    for (int i = 0; i < n1; ++i)
    {
        for (int j = 0; j < n1; ++j)
        {
            fprintf(file, "%.6f", H[arr(i, j, n1)]);
            if (j + 1 < n1)
            {
                fprintf(file, ",");
            }
        }
        fprintf(file, "\n");
    }
    fclose(file);
}

/*
Main function that parses args, runs Jacobi iterations on GPU, and writes final result to CSV
* Args:
*  -N <int> : grid size 
*  -I <int> : number of iterations 
*  -q       : quit immediately 
*/
int main(int argc, char** argv)
{
    Args args = parseArgs(argc, argv);
    //quit if -q flag is set
    if (args.quit) 
    {
        return 0; 
    }

    int N = args.N;
    int I = args.I;
    int n1 = N + 2; //account for edges
    size_t size = (size_t)n1 * (size_t)n1 * sizeof(double);

    std::vector<double> h_p(n1 * n1);
    std::vector<double> h_p_next(n1 * n1); //host copies

    plate(h_p, N); //adds boundaries

    double* d_p;
    double* d_p_next;
    //device memory
    cudaMalloc(&d_p, size);
    cudaMalloc(&d_p_next, size);

    //copy from host to device
    cudaMemcpy(d_p, h_p.data(), size, cudaMemcpyHostToDevice); 
    cudaMemcpy(d_p_next, h_p.data(), size, cudaMemcpyHostToDevice); 

    dim3 blockSize(16, 16);
    dim3 gridSize((n1 + blockSize.x - 1) / blockSize.x, (n1 + blockSize.y - 1) / blockSize.y);

    //start and stop for timer
    cudaEvent_t evStart, evStop;
    cudaEventCreate(&evStart);
    cudaEventCreate(&evStop);
    cudaEventRecord(evStart);

    //actual iterations
    for (int iter = 0; iter < I; ++iter)
    {
        jacobiIteration<<<gridSize, blockSize>>>(d_p, d_p_next, N, n1);
        std::swap(d_p, d_p_next);
    }

    cudaEventRecord(evStop);
    cudaEventSynchronize(evStop);
    float ms = 0.0f;
    cudaEventElapsedTime(&ms, evStart, evStop);
    printf("Elapsed (ms): %.3f\n", ms); //timer output

    cudaMemcpy(h_p.data(), d_p, size, cudaMemcpyDeviceToHost); //copy back to host

    writeCSV("finalTemperatures.csv", h_p, N);

    cudaEventDestroy(evStart);
    cudaEventDestroy(evStop);
    cudaFree(d_p);
    cudaFree(d_p_next);

    return 0;
}
