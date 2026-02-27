/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: 10/12/2025
Description:
Explores different parallelization strategies (single threading, multithreading, or openMP parallel) using Conway's Game of Life.
*/

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <cctype>

//force to compile even in no sfml
#ifndef HEADLESS
  #include <SFML/Graphics.hpp>
#endif

//safety against no openmp
#ifdef _OPENMP
  #include <omp.h>
#endif

/* Class: Cfg
    Description: stores configuration parameters read from command line arguments
    Inputs:
      n: number of threads (must be larger than 2)
      c: cell size (cells are square, c >=1)
      x: window width
      y: window height
      t: mode, can be SEQ, THRD, or OMP, default is THRD

*/
struct Cfg {
  int n = 8;      // is the number of threads (must be larger than 2)
  int c = 5;      // “cell size” with cells being square (c >=1)
  int x = 800;    // window width
  int y = 600;    // window height
  std::string t = "THRD"; // mode: SEQ, THRD, or OMP, defaiult THRD
};

/* Function: read
    Description: reads command line arguments (like /Lab2 -c 5 -x 800 -y 600 -t OMP) and populates a Cfg struct
    Inputs:
      argc: argument count from main
      argv: argument vector from main
    Outputs:
      Cfg struct populated with values from command line or defaults

*/
Cfg read(int argc, char** argv){ 
  Cfg a;
  for (int i=1;i<argc;i++){
    std::string f=argv[i];
    if (f=="-n" && i+1<argc) a.n = std::atoi(argv[++i]);
    else if (f=="-c" && i+1<argc) a.c = std::atoi(argv[++i]);
    else if (f=="-x" && i+1<argc) a.x = std::atoi(argv[++i]);
    else if (f=="-y" && i+1<argc) a.y = std::atoi(argv[++i]);
    else if (f=="-t" && i+1<argc) a.t = argv[++i];
  }
  for (auto& ch : a.t) ch = std::toupper(ch);
  if (a.t=="SEQ") a.n = 1;           // if SEQ, then n = 1
  if (a.t!="SEQ" && a.n<2) a.n = 2;  //  n>=2 if not seq
  if (a.c<1) a.c = 1;                // cell size >=1
  return a;
}

// cell logic

using Cell = uint8_t;
/* Function: idx
    Description: converts 2D coordinates to 1D index
    Inputs:
      r: row
      c: column
      cols: total number of columns in the grid
    Outputs:
      1D index corresponding to (row, column)
*/
inline size_t idx(int r, int c, int cols)
    { 
        return (size_t)r * cols + c; 
    }

/* Function: countAliveNeighbors
    Description: counts alive neighbors of cell at (r, c) in grid
    Inputs:
      grid: 1D vector of grid
      r: row of the cell
      c: column of the cell
      rows: total number of rows in the grid
      cols: total number of columns in the grid
    Outputs:
      number of alive neighbors 
*/
static inline int countAliveNeighbors(const std::vector<Cell>& grid, int r, int c, int rows, int cols) 
    {
    int alive = 0;
    for (int delta_r = -1; delta_r <= 1; ++delta_r)
        {
            for (int delta_c = -1; delta_c <= 1; ++delta_c)
            {
                if (delta_r == 0 && delta_c == 0) continue; // skip center cell
                int neighbor_r = r + delta_r;
                int neighbor_c = c + delta_c;
                if (neighbor_r >= 0 && neighbor_r < rows && neighbor_c >= 0 && neighbor_c < cols)
                {
                    alive += grid[idx(neighbor_r, neighbor_c, cols)]; // add 1 if alive
                }
            }
        }
        return alive;
    }

/* Function: updateSequential
    Description: updates the grid to the next generation sequentially (SEQ)
    Inputs:
      current: 1D vector of current grid state
      next: 1D vector to store next grid state
      rows: total number of rows in the grid
      cols: total number of columns in the grid
    Outputs:
      next vector populated with next generation's states
*/
static void updateSequential(const std::vector<Cell>& current, std::vector<Cell>& next, int rows, int cols)
{
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            int alive = countAliveNeighbors(current, r, c, rows, cols); //get # of alive neighbors
            Cell curr = current[idx(r, c, cols)]; //current cell alive or dead
            if (curr) //if alive
            {
                next[idx(r, c, cols)] = (alive == 2 || alive == 3) ? 1u : 0u; // lives w/ 2 or 3 neighbors
            }
            else //if dead
            {
                next[idx(r, c, cols)] = (alive == 3) ? 1u : 0u; //revives w/ 3 neighbors
            }
        }
    }
}

// multithreading logic, same logic as SEQ
/* Function: updateThreaded
    Description: updates the grid to the next generation using multiple threads (THRD)
    Inputs:
      current: 1D vector of current grid state
      next: 1D vector to store next grid state
      rows: total number of rows in the grid
      cols: total number of columns in the grid
      threads: number of threads to use
    Outputs:
      next vector populated with next generation's states
*/
static void workerSection(const std::vector<Cell>& current, std::vector<Cell>& next, int rows, int cols, int r0, int r1)
{
    for (int r = r0; r < r1; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            int alive = countAliveNeighbors(current, r, c, rows, cols); //get # of alive neighbors
            Cell curr = current[idx(r, c, cols)]; //current cell alive or dead
            if (curr) //if alive
            {
                next[idx(r, c, cols)] = (alive == 2 || alive == 3) ? 1u : 0u; // lives w/ 2 or 3 neighbors
            }
            else //if dead
            {
                next[idx(r, c, cols)] = (alive == 3) ? 1u : 0u; //revives w/ 3 neighbors
            }
        }
    }
}

/* Function: updateThreaded
    Description: updates the grid to the next generation using multiple threads (THRD)
    Inputs:
      current: 1D vector of current grid state
      next: 1D vector to store next grid state
      rows: total number of rows in the grid
      cols: total number of columns in the grid
      threads: number of threads to use
    Outputs:
      next vector populated with next generation's states
*/
static void updateThreaded(const std::vector<Cell>& current, std::vector<Cell>& next, int rows, int cols, int threads)
{
    if (threads < 2) //safety
    {
        updateSequential(current, next, rows, cols);
        return;
    }
    std::vector<std::thread> pool;
    pool.reserve(threads);
    int base = (rows + threads - 1) / threads; //rows per thread

    for (int t = 0; t < threads; ++t)
    {
        int r0 = t * base;
        int r1 = std::min(r0 + base, rows);
        if (r0 >= rows) break; //no more rows to process
        pool.emplace_back(workerSection, std::cref(current), std::ref(next), rows, cols, r0, r1);
    }
    for (auto& th : pool) th.join();
}

/* Function: updateOMP
    Description: updates the grid to the next generation using OpenMP parallelization (OMP)
    Inputs:
      current: 1D vector of current grid state
      next: 1D vector to store next grid state
      rows: total number of rows in the grid
      cols: total number of columns in the grid
      threads: number of threads to use
    Outputs:
      next vector populated with next generation's states
*/
static void updateOMP(const std::vector<Cell>& current, std::vector<Cell>& next, int rows, int cols, int threads)
{
#ifdef _OPENMP
    #pragma omp parallel for schedule(static) num_threads(threads)
    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            int alive = countAliveNeighbors(current, r, c, rows, cols); //get # of alive neighbors
            Cell curr = current[idx(r, c, cols)]; //current cell alive or dead
            if (curr) //if alive
            {
                next[idx(r, c, cols)] = (alive == 2 || alive == 3) ? 1u : 0u; // lives w/ 2 or 3 neighbors
            }
            else //if dead
            {
                next[idx(r, c, cols)] = (alive == 3) ? 1u : 0u; //revives w/ 3 neighbors
            }
        }
    }
#else
    updateSequential(current, next, rows, cols);
    (void)threads;
#endif
}

/* Function: main
    Description: main function to run the Game of Life simulation
    Inputs:
      argc: argument count from command line
      argv: argument vector from command line
    Outputs:
      runs the simulation and displays results in a window or headless mode
*/
int main(int argc, char* argv[])
{
    Cfg cfg = read(argc, argv); //read command line arguments
    int activeThreads = (cfg.t == "SEQ") ? 1 : cfg.n;

    //window size 
    const int rows = std::max(1, cfg.y / cfg.c); 
    const int cols = std::max(1, cfg.x / cfg.c);
    std::vector<Cell> current(rows * cols, 0), next(rows * cols, 0);

    //using bernoulli distribution to randomly populate grid
    std::mt19937 rng(static_cast<uint32_t>(
        std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    std::bernoulli_distribution aliveDist(0.5);

    for (int r = 0; r < rows; ++r)
    {
        for (int c = 0; c < cols; ++c)
        {
            current[idx(r, c, cols)] = aliveDist(rng) ? 1u : 0u;
        }
    }

    //only print every 100 generations
    using clock = std::chrono::high_resolution_clock;
    int genCounter = 0;
    auto batchStart = clock::now();

#ifndef HEADLESS
    // GRAPHICS loop 
    sf::RenderWindow window(sf::VideoMode(cfg.x, cfg.y), "Conway's Game of Life (Lab 2)");
    sf::RectangleShape cellShape(sf::Vector2f(static_cast<float>(cfg.c), static_cast<float>(cfg.c))); // draw cell
    cellShape.setFillColor(sf::Color::White);
    window.setVerticalSyncEnabled(true);

    //close w/ escape or window close
    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
            if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape)
            {
                window.close();
            }
        }

        if (genCounter == 0)
        {
            batchStart = clock::now();
        }

        if (cfg.t == "SEQ")
        {
            updateSequential(current, next, rows, cols);
        }
        else if (cfg.t == "THRD")
        {
            updateThreaded(current, next, rows, cols, activeThreads);
        }
        else
        {
            updateOMP(current, next, rows, cols, activeThreads);
        }

        std::swap(current, next);

        window.clear(sf::Color::Black);
        for (int r = 0; r < rows; ++r)
        {
            for (int c = 0; c < cols; ++c)
            {
                if (current[idx(r, c, cols)])
                {
                    cellShape.setPosition(
                        static_cast<float>(c * cfg.c),
                        static_cast<float>(r * cfg.c)
                    );
                    window.draw(cellShape);
                }
            }
        }
        window.display();

        //count generations and print time every 100
        genCounter++;
        if (genCounter == 100)
        {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                clock::now() - batchStart
            ).count();

            //print
            if (cfg.t == "SEQ")
            {
                std::cout << "100 generations took " << us
                          << " microseconds with single thread.\n";
            }
            else if (cfg.t == "THRD")
            {
                std::cout << "100 generations took " << us
                          << " microseconds with " << activeThreads
                          << " std::threads.\n";
            }
            else
            {
                std::cout << "100 generations took " << us
                          << " microseconds with " << activeThreads
                          << " OMP threads.\n";
            }

            genCounter = 0; //for next set
        }
    }
#else
    // headless to avoid compiler issues
    const int totalGenerations = 1000;
    batchStart = clock::now();

    for (int g = 0; g < totalGenerations; ++g)
    {
        if (genCounter == 0)
        {
            batchStart = clock::now();
        }

        if (cfg.t == "SEQ")
        {
            updateSequential(current, next, rows, cols);
        }
        else if (cfg.t == "THRD")
        {
            updateThreaded(current, next, rows, cols, activeThreads);
        }
        else
        {
            updateOMP(current, next, rows, cols, activeThreads);
        }

        std::swap(current, next);

        genCounter++;
        if (genCounter == 100)
        {
            auto us = std::chrono::duration_cast<std::chrono::microseconds>(
                clock::now() - batchStart
            ).count();

            if (cfg.t == "SEQ")
            {
                std::cout << "100 generations took " << us
                          << " microseconds with single thread.\n";
            }
            else if (cfg.t == "THRD")
            {
                std::cout << "100 generations took " << us
                          << " microseconds with " << activeThreads
                          << " std::threads.\n";
            }
            else
            {
                std::cout << "100 generations took " << us
                          << " microseconds with " << activeThreads
                          << " OMP threads.\n";
            }

            genCounter = 0;
        }
    }
#endif

    return 0;
}
