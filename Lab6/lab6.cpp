/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: 11/25/2025
Description:
Estimate definite integrals w/ open MPI (Monte Carlo).
*/

#include <mpi.h>
#include <iostream>
#include <random>
#include <cstring>   // for std::strcmp
#include <cstdlib>   // for std::atoi, std::atoll
#include <cmath>     // for std::exp
#include <ctime>     // time()

/*
    Calculate integrand, depends on which problem we are solving.
    Input:
        x - random sample in [0,1]
        P - which integral to compute (1 or 2)
    Output:
        g(x) value for the selected integral
*/
double integral(double x, int P) 
{
    // Integral 1
    if (P == 1) 
    { 
        return x * x;
    // Integral 2
    } 
    else if (P == 2) 
    {
        return std::exp(-x * x);
    }
    return 0.0; // default case (should not happen)
}

/*
    Parse command line arguments to get P and N_total.
    Only rank 0 does this; other ranks will receive values via Bcast.
*/
void parseCommandLine(int argc, char* argv[],
                      int world_rank,
                      int &P, long long &N_total) 
{
    if (world_rank != 0) 
    {
        // if it's 0, parse command line arguments so we only do it once, P and N_total via Bcast
        return;
    }

    if (argc != 5) 
    {
        std::cerr << "Usage: " << argv[0]
                  << " -P <1|2> -N <num_samples>\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    P = 0; //will reset with 1 or 2 based input
    N_total = 0; //numberq of samples

    for (int i = 1; i < argc; ++i) 
    {
        if (std::strcmp(argv[i], "-P") == 0 && i + 1 < argc) 
        {
            P = std::atoi(argv[++i]);
        } 
        else if (std::strcmp(argv[i], "-N") == 0 && i + 1 < argc) 
        {
            N_total = std::atoll(argv[++i]);
        }
    }

    if ((P != 1 && P != 2) || N_total <= 0) {
        std::cerr << "Error: P must be 1 or 2, and N must be > 0.\n";
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}


/*
    Main function: MPI program to estimate integrals with Monte Carlo.
*/
int main(int argc, char* argv[]) 
{
    MPI_Init(&argc, &argv);

    int world_size, world_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

    int P; //which integral
    long long N_total; 
    parseCommandLine(argc, argv, world_rank, P, N_total);

    // Broadcast P and N_total to all ranks (collective communication)
    MPI_Bcast(&P, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&N_total, 1, MPI_LONG_LONG, 0, MPI_COMM_WORLD);

    // how many samples this rank will handle
    long long base = N_total / world_size;
    long long rem  = N_total % world_size;
    long long local_N = base + (world_rank < rem ? 1 : 0);

    // random number generator
    std::mt19937_64 rng;
    // diff seeds for each rank
    rng.seed(static_cast<unsigned long long>(time(nullptr)) + world_rank);
    std::uniform_real_distribution<double> dist(0.0, 1.0);

    // monte carlo
    double local_sum = 0.0;
    for (long long i = 0; i < local_N; i++) 
    {
        double x = dist(rng);
        local_sum += integral(x, P);
    }

    // if rank 0, gather results from all ranks
    double global_sum = 0.0;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE,
               MPI_SUM, 0, MPI_COMM_WORLD);

    if (world_rank == 0) 
    {
        // Average over N_total samples
        double estimate = global_sum / static_cast<double>(N_total);
        std::cout << "The estimate for integral " << P
                  << " is " << estimate << std::endl;
        std::cout << "Bye!" << std::endl;
    }

    MPI_Finalize();
    return 0;
}
