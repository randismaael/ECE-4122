/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: Dec 2, 2025
Description:
ECE_UAV class implementation. Handles UAV movement logic in its each own thread.
*/

#include "ECE_UAV.h"
#include <chrono>
#include <thread>
#include <cmath>
#include <vector>

// for collision detection
extern std::vector<ECE_UAV *> g_uavs;

// Constructor
ECE_UAV::ECE_UAV(int id, double xInit, double yInit, double zInit, double massInit)
    : id(id), x(xInit), y(yInit), z(zInit),
      vx(0.0), vy(0.0), vz(0.0),
      ax(0.0), ay(0.0), az(0.0),
      m(massInit),
      running(false),
      isCenter(false),
      onSurface(false),
      timeonSurface(0.0),
      xIntegral(0.0), yIntegral(0.0), zIntegral(0.0),
      prevErrorX(0.0), prevErrorY(0.0), prevErrorZ(0.0),
      orbitInitialized(false),
      orbitAxisX(0.0), orbitAxisY(0.0), orbitAxisZ(0.0)

{
}

// Destructor
ECE_UAV::~ECE_UAV()
{
    stop();
    join();
}

/*
    Purpose: Start the UAV thread
    Inputs: None
    Outputs: None
*/
void ECE_UAV::start()
{
    if (!running.load())
    {
        running.store(true);
        uavThread = std::thread(threadFunction, this); // start trhead
    }
}

/*
    Purpose: Join the UAV thread
    Inputs: None
    Outputs: None
*/
void ECE_UAV::join()
{
    if (uavThread.joinable())
    {
        uavThread.join();
    }
}

/*
    Purpose: Get the UAV position
    Inputs:
        double &xOut - x position
        double &yOut - y position
        double &zOut - z position
    Outputs: None
*/
void ECE_UAV::getPosition(double &xOut, double &yOut, double &zOut) const
{
    std::lock_guard<std::mutex> lock(mtx);
    xOut = x;
    yOut = y;
    zOut = z;
}

/*
    Purpose: Getter for the UAV velocity
    Inputs:
        double &vxOut - x velocity
        double &vyOut - y velocity
        double &vzOut - z velocity
    Outputs: None
*/
void ECE_UAV::getVelocity(double &vxOut, double &vyOut, double &vzOut) const
{
    std::lock_guard<std::mutex> lock(mtx);
    vxOut = vx;
    vyOut = vy;
    vzOut = vz;
}

/*
    Purpose: Setter for the UAV position
    Inputs:
        double xInit - x position
        double yInit - y position
        double zInit - z position
    Outputs: None
*/
void ECE_UAV::setPosition(double xInit, double yInit, double zInit)
{
    std::lock_guard<std::mutex> lock(mtx);
    x = xInit;
    y = yInit;
    z = zInit;
}

/*
    Purpose: Stop the UAV thread
    Inputs: None
    Outputs: None
*/
void ECE_UAV::stop()
{
    running.store(false);
}

/*
    Purpose: Check if the UAV thread is running
    Inputs: None
    Outputs:
        -bool: true if need to stop
*/
bool ECE_UAV::needStop() const
{
    return !running.load(); // if true, need to stop
}

// "Each UAV has a mass of 1 kg and is able to generate a single force vector with a
// total maximum magnitude of 20 N in any direction."
/*
    Purpose: Clamp force vector to max magnitude
    Inputs:
        double &fx - x force
        double &fy - y force
        double &fz - z force
        double maxMag - maximum magnitude
    Outputs: None
*/
static void clamp(double &fx, double &fy, double &fz, double maxMag)
{
    double mag = std::sqrt(fx * fx + fy * fy + fz * fz);
    if (mag > maxMag && mag > 0.0)
    {
        double scale = maxMag / mag;
        fx *= scale;
        fy *= scale;
        fz *= scale;
    }
}

// "13) If the bounding boxes of two UAVs come within 1 cm of each other an elastic collision
// occurs. For simplicity we are going to model the UAVs as point objects and the UAVs
// will just swap velocity vectors for the next time step"

/*
    Purpose: Check for collisions between UAVs and swap velocities if within 1 cm
    Inputs:
        ECE_UAV *pUAV - pointer to the UAV to check collisions
    Outputs: None
*/
static void checkCollisions(ECE_UAV *pUAV)
{
    const double collisionDist = 0.01; // 1 cm

    double myX, myY, myZ, myVx, myVy, myVz;
    {
        std::lock_guard<std::mutex> lock(pUAV->mtx);
        myX = pUAV->x;
        myY = pUAV->y;
        myZ = pUAV->z;
        myVx = pUAV->vx;
        myVy = pUAV->vy;
        myVz = pUAV->vz;
    }

    for (auto *other : g_uavs)
    {
        if (other == pUAV || other == nullptr)
        {
            continue;
        }

        // use ID so no double check
        if (pUAV->getID() > other->getID())
        {
            continue;
        }

        double otherX, otherY, otherZ, otherVx, otherVy, otherVz;
        {
            std::lock_guard<std::mutex> lock(other->mtx);
            otherX = other->x;
            otherY = other->y;
            otherZ = other->z;
            otherVx = other->vx;
            otherVy = other->vy;
            otherVz = other->vz;
        }

        double dx = myX - otherX;
        double dy = myY - otherY;
        double dz = myZ - otherZ;
        double dist = std::sqrt(dx * dx + dy * dy + dz * dz);

        // "If within 1cm, swap velocities"
        if (dist < collisionDist)
        {
            {
                std::lock_guard<std::mutex> lock(pUAV->mtx);
                pUAV->vx = otherVx;
                pUAV->vy = otherVy;
                pUAV->vz = otherVz;
            }
            {
                std::lock_guard<std::mutex> lock(other->mtx);
                other->vx = myVx;
                other->vy = myVy;
                other->vz = myVz;
            }

            // once
            break;
        }
    }
}

/*
    Purpose: Getter for the UAV mass
    Inputs: None
    Outputs:
        -double: mass of UAV
*/
double ECE_UAV::getMass() const
{
    return m;
}

/*
    Purpose: Getter for the UAV ID
    Inputs: None
    Outputs:
        -int: ID of UAV
*/
int ECE_UAV::getID() const
{
    return id;
}

// basically for main
/*
    Purpose: Get the UAV surface status
    Inputs:
        bool &onSurfaceOut - if on surface
        double &timeOnSurfaceOut - time on surface
    Outputs: None
*/
void ECE_UAV::getSurfaceStatus(bool &onSurfaceOut, double &timeOnSurfaceOut) const
{
    std::lock_guard<std::mutex> lock(mtx);
    onSurfaceOut = onSurface;
    timeOnSurfaceOut = timeonSurface;
}

/*
    Purpose: Main UAV thread function. Handles UAV movement logic and updates.
    Inputs:
        ECE_UAV *pUAV - pointer to the UAV
    Outputs: None
*/
void threadFunction(ECE_UAV *pUAV)
{
    using namespace std::chrono_literals;

    if (pUAV == nullptr)
    {
        return;
    }

    const double dt = 0.01; // 10ms
    const double gravity = 10.0;
    const double maxForce = 20.0; // clamp to 20N

    // (0,0,50)
    const double targetX = 0.0;
    const double targetY = 0.0;
    const double targetZ = 50.0;

    //  10 m while attempting to maintain a speed between 2 to 10 m/s.
    const double sphereR = 10.0;
    const double minSpeed = 2.0;
    const double maxSpeed = 10.0;

    double duration = 0.0;

    auto lastWallTime = std::chrono::high_resolution_clock::now();

    while (!pUAV->needStop())
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastWallTime);
        if (elapsed < 10ms)
        {
            std::this_thread::sleep_for(10ms - elapsed);
            now = std::chrono::high_resolution_clock::now();
        }
        lastWallTime = now;
        duration += dt;

        {
            std::lock_guard<std::mutex> lock(pUAV->mtx);

            // sit for 5s
            // "2) The UAVs remain on the ground for 5 seconds after the beginning of the simulation. "
            if (duration < 5.0)
            {
                pUAV->vx = 0.0;
                pUAV->vy = 0.0;
                pUAV->vz = 0.0;

                pUAV->ax = 0.0;
                pUAV->ay = 0.0;
                pUAV->az = 0.0;

                continue;
            }

            // fly to center

            // curr
            double x = pUAV->x;
            double y = pUAV->y;
            double z = pUAV->z;
            double vx = pUAV->vx;
            double vy = pUAV->vy;
            double vz = pUAV->vz;

            // center to UAV
            double dx = x - targetX;
            double dy = y - targetY;
            double dz = z - targetZ;
            double distToCenter = std::sqrt(dx * dx + dy * dy + dz * dz);

            // MOVING TOWARD SPHERE
            // "3) After the initial 5 seconds the UAVs then launch from the ground and go towards the
            // point (0, 0, 50 m) above the ground with a maximum velocity of 2 m/s"
            if (!pUAV->onSurface && distToCenter > sphereR)
            {
                // target - curr
                double xError = targetX - x;
                double yError = targetY - y;
                double zError = targetZ - z;

                // distance to target
                if (distToCenter < 10.0)
                {
                    pUAV->isCenter = true; // remember we reached the center region
                }

                // PID calcs derived from PID_Sim.cpp
                const double Kp = 4.0;
                const double Ki = 0.2;
                const double Kd = 2.0;

                pUAV->xIntegral += xError * dt;
                double xDerivative = (xError - pUAV->prevErrorX) / dt;
                double fx = Kp * xError + Ki * pUAV->xIntegral + Kd * xDerivative;
                pUAV->prevErrorX = xError;

                pUAV->yIntegral += yError * dt;
                double yDerivative = (yError - pUAV->prevErrorY) / dt;
                double fy = Kp * yError + Ki * pUAV->yIntegral + Kd * yDerivative;
                pUAV->prevErrorY = yError;

                pUAV->zIntegral += zError * dt;
                double zDerivative = (zError - pUAV->prevErrorZ) / dt;
                pUAV->prevErrorZ = zError;

                // gravity for z
                double zGravity = gravity * pUAV->getMass(); // positive up
                double fz = Kp * zError + Ki * pUAV->zIntegral + Kd * zDerivative + zGravity;

                // clamp 20N
                clamp(fx, fy, fz, maxForce);

                // get acceleration
                double mass = pUAV->getMass();
                double ax = fx / mass;
                double ay = fy / mass;
                double az = fz / mass;

                // velocity
                vx += ax * dt;
                vy += ay * dt;
                vz += az * dt;

                //"with a maximum velocity of 2 m/s" so make sure it is 2
                double speed = std::sqrt(vx * vx + vy * vy + vz * vz);
                if (speed > 2.0)
                {
                    double s = 2.0 / speed;
                    vx *= s;
                    vy *= s;
                    vz *= s;
                }

                // position
                x += vx * dt + 0.5 * ax * dt * dt;
                y += vy * dt + 0.5 * ay * dt * dt;
                z += vz * dt + 0.5 * az * dt * dt;

                // save
                pUAV->ax = ax;
                pUAV->ay = ay;
                pUAV->az = az;

                pUAV->vx = vx;
                pUAV->vy = vy;
                pUAV->vz = vz;

                pUAV->x = x;
                pUAV->y = y;
                pUAV->z = z;

                // just entered the 10 m region?
                if (distToCenter <= sphereR)
                {
                    pUAV->isCenter = true;
                    pUAV->onSurface = true;
                    pUAV->timeonSurface = 0.0;
                }

                continue; // still approaching
            }

            // ON SPHERE
            if (!pUAV->onSurface)
            {
                pUAV->onSurface = true;
                pUAV->timeonSurface = 0.0;
            }

            // curr to center
            dx = x - targetX;
            dy = y - targetY;
            dz = z - targetZ;
            double dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist < 1e-6)
            {
                // if at center, move to sphere surface
                double angle = (pUAV->getID() * 2.0 * M_PI) / 15.0; // even
                dx = sphereR * std::cos(angle);
                dy = sphereR * std::sin(angle);
                dz = 0.0;
                dist = sphereR;
            }
            double scale = sphereR / dist;
            dx *= scale;
            dy *= scale;
            dz *= scale;
            x = targetX + dx;
            y = targetY + dy;
            z = targetZ + dz;

            // rad unit vector
            double ux = dx / sphereR;
            double uy = dy / sphereR;
            double uz = dz / sphereR;

            // start orbit
            if (!pUAV->orbitInitialized)
            {
                // use Fibbonaci on the IDs to "randomize"
                int uavID = pUAV->getID();
                double phi = M_PI * (3.0 - std::sqrt(5.0)); // Golden angle in radians

                double y_axis = 1.0 - (uavID / 7.5);
                double radius = std::sqrt(1.0 - y_axis * y_axis);
                double theta = phi * uavID;

                pUAV->orbitAxisX = std::cos(theta) * radius;
                pUAV->orbitAxisY = y_axis;
                pUAV->orbitAxisZ = std::sin(theta) * radius;

                // Normalize
                double axisMag = std::sqrt(pUAV->orbitAxisX * pUAV->orbitAxisX +
                                           pUAV->orbitAxisY * pUAV->orbitAxisY +
                                           pUAV->orbitAxisZ * pUAV->orbitAxisZ);
                if (axisMag > 1e-6)
                {
                    pUAV->orbitAxisX /= axisMag;
                    pUAV->orbitAxisY /= axisMag;
                    pUAV->orbitAxisZ /= axisMag;
                }

                pUAV->orbitInitialized = true;

                // init tangent vector (this is for velocity dir)
                double tx = pUAV->orbitAxisY * uz - pUAV->orbitAxisZ * uy;
                double ty = pUAV->orbitAxisZ * ux - pUAV->orbitAxisX * uz;
                double tz = pUAV->orbitAxisX * uy - pUAV->orbitAxisY * ux;
                double tmag = std::sqrt(tx * tx + ty * ty + tz * tz);

                // If parallel, choose a different tangent
                if (tmag < 1e-6)
                {
                    double hx = 0.0, hy = 0.0, hz = 1.0;
                    if (std::fabs(pUAV->orbitAxisZ) > 0.9)
                    {
                        hx = 1.0;
                        hy = 0.0;
                        hz = 0.0;
                    }
                    tx = pUAV->orbitAxisY * hz - pUAV->orbitAxisZ * hy;
                    ty = pUAV->orbitAxisZ * hx - pUAV->orbitAxisX * hz;
                    tz = pUAV->orbitAxisX * hy - pUAV->orbitAxisY * hx;
                    tmag = std::sqrt(tx * tx + ty * ty + tz * tz);
                }

                if (tmag > 1e-6)
                {
                    tx /= tmag;
                    ty /= tmag;
                    tz /= tmag;
                }

                // initialize random speeds
                double desiredSpeed = 2.0 + (uavID % 9); // "2 to 10 m/s"
                if (desiredSpeed < minSpeed)
                    desiredSpeed = minSpeed;
                if (desiredSpeed > maxSpeed)
                    desiredSpeed = maxSpeed;

                vx = tx * desiredSpeed;
                vy = ty * desiredSpeed;
                vz = tz * desiredSpeed;
            }

            // stay in orbit
            double oax = pUAV->orbitAxisX;
            double oay = pUAV->orbitAxisY;
            double oaz = pUAV->orbitAxisZ;

            // for velocity direction
            double tx = oay * uz - oaz * uy;
            double ty = oaz * ux - oax * uz;
            double tz = oax * uy - oay * ux;
            double tmag = std::sqrt(tx * tx + ty * ty + tz * tz);
            if (tmag < 1e-6)
            {
                // just in case they r parallel
                tx = -uy;
                ty = ux;
                tz = 0.0;
                tmag = std::sqrt(tx * tx + ty * ty);
            }
            if (tmag > 1e-6)
            {
                tx /= tmag;
                ty /= tmag;
                tz /= tmag;
            }

            // make sure speed between 2 and 10 m/s
            double desiredSpeed = 2.0 + (pUAV->getID() % 9);
            if (desiredSpeed < minSpeed)
                desiredSpeed = minSpeed;
            if (desiredSpeed > maxSpeed)
                desiredSpeed = maxSpeed;

            // Save old velocity for acceleration calc
            double vxOld = pUAV->vx;
            double vyOld = pUAV->vy;
            double vzOld = pUAV->vz;

            vx = tx * desiredSpeed;
            vy = ty * desiredSpeed;
            vz = tz * desiredSpeed;

            // integrate position
            x += vx * dt;
            y += vy * dt;
            z += vz * dt;

            // Re-project to sphere surface
            dx = x - targetX;
            dy = y - targetY;
            dz = z - targetZ;
            dist = std::sqrt(dx * dx + dy * dy + dz * dz);
            if (dist > 1e-6)
            {
                scale = sphereR / dist;
                dx *= scale;
                dy *= scale;
                dz *= scale;
                x = targetX + dx;
                y = targetY + dy;
                z = targetZ + dz;
            }

            // acceleration calc
            double ax = (vx - vxOld) / dt;
            double ay = (vy - vyOld) / dt;
            double az = (vz - vzOld) / dt;

            // save
            pUAV->ax = ax;
            pUAV->ay = ay;
            pUAV->az = az;

            pUAV->vx = vx;
            pUAV->vy = vy;
            pUAV->vz = vz;

            pUAV->x = x;
            pUAV->y = y;
            pUAV->z = z;

            // track time on sphere surface cuz we stop at 60s
            pUAV->timeonSurface += dt;
        }

        // avoid collisions
        checkCollisions(pUAV);
    }
}