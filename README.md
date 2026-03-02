# ECE 4122 – Systems Programming & Multithreaded Simulation

Course labs and final project completed during ECE 4122 at Georgia Institute of Technology.

This repository contains systems-oriented programming assignments and a final multithreaded 3D UAV simulation implemented in C/C++.

---

## Course Focus

The course emphasizes:

- Systems-level programming concepts  
- Memory management and pointer usage  
- Multithreading using `std::thread`  
- Physics-based simulation  
- OpenGL rendering  
- Modular and self-documenting code design  
- Performance-aware implementation  

---

## Repository Structure

```
Lab0/
Lab1/
Lab2/
Lab3/
Lab4/
Lab5/
Lab6/
Project/
```

Each lab builds foundational systems programming skills, culminating in a full-scale simulation project.

---

## Final Project – GaTech Buzzy Bowl UAV Simulation

The final project implements a real-time 3D UAV halftime show simulation using multithreading and OpenGL.

### Overview

- Simulates 15 UAVs positioned on a football field  
- Each UAV is controlled by its own thread (15 worker threads + 1 rendering thread)  
- Physics updates occur every 10 ms  
- Rendering updates occur every 30 ms  
- UAV motion governed by Newton’s Second Law:

  **F = m a**

- Gravity and applied force vectors determine acceleration  
- UAVs ascend to a target altitude and transition to motion along a virtual 10 m radius sphere  
- Elastic collision handling implemented between UAVs  

### PID-Based Flight Control

To maintain spherical flight paths, a PID (Proportional–Integral–Derivative) controller was implemented for each UAV:

- Proportional term corrects instantaneous positional error  
- Integral term reduces steady-state drift  
- Derivative term dampens oscillations and stabilizes motion  
- Gains tuned experimentally to balance responsiveness and stability  

This allowed the UAVs to remain constrained to the surface of the sphere while maintaining realistic velocity limits.

### Key Concepts Implemented

- Multithreaded architecture (16 total threads)  
- Thread-managed object state updates  
- Kinematic equations of motion  
- Real-time OpenGL rendering  
- Object-oriented UAV class design  
- PID control implementation  
- Collision detection and velocity swapping  
- Time-stepped simulation loop  

### Demo Video

https://youtu.be/m0lO1hTf3j0

---

## Build Instructions

This project was developed using CMake.

To build:

```
mkdir build
cd build
cmake ..
make
```

Run the executable from the build directory.

Some labs may require OpenGL or SFML depending on assignment requirements.

---

## Academic Integrity Notice

Coursework uploaded with instructor permission.  
Please do not reuse or redistribute this material for academic submissions.