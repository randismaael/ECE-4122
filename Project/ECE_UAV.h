/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: Dec 2, 2025
Description:
ECE_UAV class header. Defines the ECE_UAV class which represents a UAV with position, velocity, acceleration, mass, and threading capabilities.
*/

// "Create a class called ECE_UAV that has member variables to contain the mass, (x, y, z)
// position, (vx, vy, yz) velocities, and (ax, ay, az) accelerations of the UAV.  The ECE_UAV
// class also contains a member variable of type std::thread.  A member function start()
// causes the thread member variable to run an external function called threadFunction
//  void threadFunction(ECE_UAV* pUAV);""

#pragma once
#include <thread>
#include <mutex>
#include <atomic>

class ECE_UAV; // Forward declaration to use in threadFunction
void threadFunction(ECE_UAV *pUAV);

class ECE_UAV
{
public:
    ECE_UAV(int id, double xInit, double yInit, double zInit, double massInit);
    ~ECE_UAV(); // Destructor to clean up thread

    void start(); // Start the UAV thread
    void join();  // Join the UAV thread

    // getters
    void getPosition(double &xOut, double &yOut, double &zOut) const;
    void getVelocity(double &vxOut, double &vyOut, double &vzOut) const;
    double getMass() const;
    int getID() const;
    void getSurfaceStatus(bool &onSurfaceOut, double &timeOnSurfaceOut) const; // to know if on surface and time

    // setters
    void setPosition(double xInit, double yInit, double zInit);

    void stop();           // Stop the UAV thread
    bool needStop() const; // Check if the UAV thread is running

    // Member variables so threadFunction can access them and update
    double x;
    double y;
    double z;
    double vx;
    double vy;
    double vz;
    double ax;
    double ay;
    double az;

    double xIntegral, yIntegral, zIntegral;    // integrals
    double prevErrorX, prevErrorY, prevErrorZ; // previous_errors

    double orbitAxisX;
    double orbitAxisY;
    double orbitAxisZ;

    // since "The UAVs travel along the same orbit unless colliding."
    // double orbitAngle;
    bool orbitInitialized;

    bool isCenter;        // if UAV id at (0,0,50)
    bool onSurface;       // while in a 10 m radius of sphere
    double timeonSurface; // time spent on surface

    mutable std::mutex mtx; // so we can lock for other threads

private:
    int id; // UAV ID
    double m;

    std::thread uavThread; // to know which one is running
    std::atomic<bool> running;
};
