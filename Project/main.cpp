/*
Author: Rand Ismaael
Class: ECE4122
Last Date Modified: Dec 2, 2025
Description:
OpenGL main for ECE_UAV test. Creates a window, spawns 15 UAVs (each with its own std::thread), and renders them as with obj file over a field.
*/

#include "ECE_UAV.h"

#include <GLUT/glut.h>

#include <vector>
#include <cmath>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>

/*
    Purpose: Simple struct to hold mesh data
    Inputs: None

*/
struct Mesh
{
    std::vector<float> vertices;
    std::vector<float> normals;
    std::vector<float> texCoords;
};

std::vector<ECE_UAV *> g_uavs;
GLuint g_fieldTexture = 0;
GLuint g_uavTexture = 0;
Mesh g_uavMesh;

// Window size, "football field in a 400 x 400 window"
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 400;

// Forward declarations
void initUAVs();
void drawField();
void drawUAVCube(double x, double y, double z);
void drawUAVMesh(double x, double y, double z);
void displayCallback();
void reshapeCallback(int w, int h);
void timerCallback(int value);
void initOpenGL();
GLuint loadBMP(const char *imagepath);
bool loadOBJ(const char *path, Mesh &mesh);

/*
    Purpose: Initialize UAVs in a 3x5 grid over the field
            "0, 25, 50, 25, 0 yard-lines as shown below by the red dots"
    Inputs: None
    Outputs: None
*/
void initUAVs()
{
    g_uavs.reserve(15);

    double yPositions[5] = {-26.0, -13.0, 0.0, 13.0, 26.0}; // change here for scaling

    int id = 0;
    for (int i = 0; i < 5 && id < 15; ++i)
    {
        double xOffsets[3] = {-13.0, 0.0, 13.0};

        for (int j = 0; j < 3 && id < 15; ++j)
        {
            double x = xOffsets[j];
            double y = yPositions[i];
            double z = 0.0; // on the ground

            ECE_UAV *uav = new ECE_UAV(id, x, y, z, 1.0);
            g_uavs.push_back(uav);
            id++;
        }
    }

    // debug
    //  std::cout << "Total UAVs created: " << g_uavs.size() << "\n";

    // Start UAV threads
    for (auto *uav : g_uavs)
    {
        uav->start();
    }
}

/*
    Purpose: Load a BMP file as an OpenGL texture
             "...file called ff.bmp in the same location as the executable to apply a football field texture to the rectangle."
    Inputs:
        const char *imagepath - path to the BMP image file
    Outputs:
        GLuint - OpenGL texture ID (0 if failed)
*/
GLuint loadBMP(const char *imagepath)
{
    unsigned char header[54];
    unsigned int dataPos;
    unsigned int width, height;
    unsigned int imageSize;
    unsigned char *data;

    FILE *file = fopen(imagepath, "rb");
    if (!file)
    {
        std::cerr << "Image could not be opened: " << imagepath << std::endl;
        return 0;
    }

    if (fread(header, 1, 54, file) != 54)
    {
        std::cerr << "Not a correct BMP file (header read failed)\n";
        fclose(file);
        return 0;
    }

    if (header[0] != 'B' || header[1] != 'M')
    {
        std::cerr << "Not a correct BMP file (missing BM signature)\n";
        fclose(file);
        return 0;
    }

    dataPos = *(int *)&(header[0x0A]);
    imageSize = *(int *)&(header[0x22]);
    width = *(int *)&(header[0x12]);
    height = *(int *)&(header[0x16]);

    if (imageSize == 0)
        imageSize = width * height * 3;
    if (dataPos == 0)
        dataPos = 54;

    data = new unsigned char[imageSize];

    fseek(file, dataPos, SEEK_SET);
    fread(data, 1, imageSize, file);
    fclose(file);

    // make rgb
    for (unsigned int i = 0; i < imageSize; i += 3)
    {
        unsigned char temp = data[i];
        data[i] = data[i + 2];
        data[i + 2] = temp;
    }

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    delete[] data;

    return textureID;
}

/*
    Purpose: Load a simple OBJ file into a Mesh struct
             "place a texture map on a 3D object of your choosing or on one of the included 3D objects."
    Inputs:
        const char *path - path to the OBJ file
        Mesh &mesh - reference to Mesh struct to fill
    Outputs:
        -  bool: true if loaded successfully, false otherwise
*/
bool loadOBJ(const char *path, Mesh &mesh)
{
    std::vector<float> temp_vertices;
    std::vector<float> temp_normals;
    std::vector<float> temp_uvs;

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "Failed to open OBJ file: " << path << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v")
        {
            float x, y, z;
            iss >> x >> y >> z;
            temp_vertices.push_back(x);
            temp_vertices.push_back(y);
            temp_vertices.push_back(z);
        }
        else if (prefix == "vn")
        {
            float x, y, z;
            iss >> x >> y >> z;
            temp_normals.push_back(x);
            temp_normals.push_back(y);
            temp_normals.push_back(z);
        }
        else if (prefix == "vt")
        {
            float u, v;
            iss >> u >> v;
            temp_uvs.push_back(u);
            temp_uvs.push_back(v);
        }
        else if (prefix == "f")
        {
            std::string token;
            std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;

            while (iss >> token)
            {
                unsigned int vertexIndex = 0, uvIndex = 0, normalIndex = 0;

                int slashCount = 0;
                size_t pos = 0;
                while ((pos = token.find('/', pos)) != std::string::npos)
                {
                    slashCount++;
                    pos++;
                }

                if (slashCount == 2)
                {
                    sscanf(token.c_str(), "%u/%u/%u", &vertexIndex, &uvIndex, &normalIndex);
                }
                else if (slashCount == 1)
                {
                    if (token.find("//") != std::string::npos)
                    {
                        sscanf(token.c_str(), "%u//%u", &vertexIndex, &normalIndex);
                    }
                    else
                    {
                        sscanf(token.c_str(), "%u/%u", &vertexIndex, &uvIndex);
                    }
                }
                else
                {
                    sscanf(token.c_str(), "%u", &vertexIndex);
                }

                if (vertexIndex > 0)
                    vertexIndices.push_back(vertexIndex - 1);
                if (uvIndex > 0)
                    uvIndices.push_back(uvIndex - 1);
                if (normalIndex > 0)
                    normalIndices.push_back(normalIndex - 1);
            }

            for (size_t i = 0; i < vertexIndices.size() - 2; i++)
            {
                std::vector<size_t> triangle = {0, i + 1, i + 2};

                for (size_t idx : triangle)
                {
                    if (vertexIndices[idx] < temp_vertices.size() / 3)
                    {
                        mesh.vertices.push_back(temp_vertices[vertexIndices[idx] * 3]);
                        mesh.vertices.push_back(temp_vertices[vertexIndices[idx] * 3 + 1]);
                        mesh.vertices.push_back(temp_vertices[vertexIndices[idx] * 3 + 2]);
                    }

                    if (!normalIndices.empty() && normalIndices[idx] < temp_normals.size() / 3)
                    {
                        mesh.normals.push_back(temp_normals[normalIndices[idx] * 3]);
                        mesh.normals.push_back(temp_normals[normalIndices[idx] * 3 + 1]);
                        mesh.normals.push_back(temp_normals[normalIndices[idx] * 3 + 2]);
                    }

                    if (!uvIndices.empty() && uvIndices[idx] < temp_uvs.size() / 2)
                    {
                        mesh.texCoords.push_back(temp_uvs[uvIndices[idx] * 2]);
                        mesh.texCoords.push_back(temp_uvs[uvIndices[idx] * 2 + 1]);
                    }
                }
            }
        }
    }
    return true;
}

/*
    Purpose: Draw the football field as a textured rectangle
    Inputs: None
    Outputs: None
*/
void drawField()
{
    // Width (x-axis) = 39m, Length (y-axis) = 58.5m
    double fieldWidth = 39.0;  // width
    double fieldLength = 58.5; // length

    double xMin = -fieldWidth / 2.0;  // -19.5
    double xMax = fieldWidth / 2.0;   // 19.5
    double yMin = -fieldLength / 2.0; // -29.25
    double yMax = fieldLength / 2.0;  // 29.25

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    if (g_fieldTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_fieldTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.6f, 0.0f);
    }

    // rotate
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(xMin, yMin, 0.0);
    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(xMax, yMin, 0.0);
    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(xMax, yMax, 0.0);
    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(xMin, yMax, 0.0);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// initial testing
//  void drawUAVCube(double x, double y, double z)
//  {
//      double half = 1.0; // cube half-size

//     glPushMatrix();
//     glTranslated(x, y, z + half); // lift slightly above ground
//     glColor3f(1.0f, 0.0f, 0.0f);  // red UAV
//     glutSolidCube(2.0 * half);
//     glPopMatrix();
// }

/*
    Purpose: Draw the UAV mesh at given position to load obj file
    Inputs:
        double x - x position
        double y - y position
        double z - z position
    Outputs: None
*/
void drawUAVMesh(double x, double y, double z)
{

    glPushMatrix();
    glTranslated(x, y, z + 0.5);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f); // flip it
    if (g_uavTexture != 0)
    {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, g_uavTexture);
        glColor3f(1.0f, 1.0f, 1.0f);
    }
    else
    {
        glDisable(GL_TEXTURE_2D);
        glColor3f(1.0f, 0.0f, 0.0f); // Red
    }

    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < g_uavMesh.vertices.size(); i += 3)
    {
        if (i < g_uavMesh.normals.size())
        {
            glNormal3f(g_uavMesh.normals[i], g_uavMesh.normals[i + 1], g_uavMesh.normals[i + 2]);
        }
        if ((i / 3) * 2 + 1 < g_uavMesh.texCoords.size() && g_uavTexture != 0)
        {
            size_t texIdx = (i / 3) * 2;
            glTexCoord2f(g_uavMesh.texCoords[texIdx], g_uavMesh.texCoords[texIdx + 1]);
        }
        glVertex3f(g_uavMesh.vertices[i], g_uavMesh.vertices[i + 1], g_uavMesh.vertices[i + 2]);
    }
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

/*
    Purpose: Check if all UAVs have been on the sphere surface for 60 seconds
             If so, stop all UAV threads and exit program
             "5) The simulation ends once all of the UAV have come within 10 m of the point, (0, 0, 50
              m), and the UAVs have flown along the surface for 60 seconds."
    Inputs: None
    Outputs:
        bool - true if simulation is done, false otherwise
*/
bool checkSimulationDone()
{
    if (g_uavs.empty())
    {
        return false;
    }

    bool allOnSurface = true;
    bool all60 = true;

    for (auto *uav : g_uavs)
    {
        bool onSurf = false;
        double tSurf = 0.0;
        uav->getSurfaceStatus(onSurf, tSurf);

        if (!onSurf)
        {
            allOnSurface = false;
            all60 = false;
            break;
        }

        if (tSurf < 60.0)
        {
            all60 = false;
        }
    }

    if (allOnSurface && all60)
    {
        // Stop and join all UAV threads
        for (auto *uav : g_uavs)
        {
            uav->stop();
        }
        for (auto *uav : g_uavs)
        {
            uav->join();
        }

        // Free memory
        for (auto *uav : g_uavs)
        {
            delete uav;
        }
        g_uavs.clear();

        // cuz im on mac
        std::exit(0);
    }

    return false;
}

/*
    Purpose: GLUT display callback to render the scene
    Inputs: None
    Outputs: None
*/
void displayCallback()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // CHANGE HERE FOR CAMERA ANGLES
    gluLookAt(
        -40.0, -60.0, 35.0,
        0.0, 0.0, 25.0,
        0.0, 0.0, 1.0);

    drawField();

    // sphere for orbit
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.5f, 0.5f, 1.0f, 0.3f);
    glPushMatrix();
    glTranslated(0.0, 0.0, 50.0);
    glutWireSphere(10.0, 20, 20); // Wireframe sphere, radius 10m
    glPopMatrix();
    glDisable(GL_BLEND);

    // Draw each UAV at its current position
    for (auto *uav : g_uavs)
    {
        double x, y, z;
        uav->getPosition(x, y, z);
        drawUAVMesh(x, y, z);
    }

    glutSwapBuffers();
}

/*
    Purpose: GLUT reshape callback to handle window resizing
    Inputs:
        int w - new window width
        int h - new window height
    Outputs: None
*/
void reshapeCallback(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (double)w / (double)h, 1.0, 500.0);
}

/*
    Purpose: GLUT timer callback to update the scene periodically
    Inputs:
        int value - timer value (not used)
    Outputs: None
*/
void timerCallback(int value)
{
    // Ask GLUT to redraw the scene
    glutPostRedisplay();

    // r we done?
    checkSimulationDone();

    // every 30ms
    glutTimerFunc(30, timerCallback, 0);
}

/*
    Purpose: Initialize OpenGL settings, load textures and UAV mesh
    Inputs: None
    Outputs: None
*/
void initOpenGL()
{
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.2f, 0.5f, 1.0f);

    // bmp file load
    g_fieldTexture = loadBMP("ff.bmp");

    // obj load
    if (loadOBJ("duck-float.obj", g_uavMesh))
    {
        // scale obj
        float minX = 1e10, maxX = -1e10;
        float minY = 1e10, maxY = -1e10;
        float minZ = 1e10, maxZ = -1e10;

        for (size_t i = 0; i < g_uavMesh.vertices.size(); i += 3)
        {
            minX = std::min(minX, g_uavMesh.vertices[i]);
            maxX = std::max(maxX, g_uavMesh.vertices[i]);
            minY = std::min(minY, g_uavMesh.vertices[i + 1]);
            maxY = std::max(maxY, g_uavMesh.vertices[i + 1]);
            minZ = std::min(minZ, g_uavMesh.vertices[i + 2]);
            maxZ = std::max(maxZ, g_uavMesh.vertices[i + 2]);
        }

        float maxDim = std::max({maxX - minX, maxY - minY, maxZ - minZ});
        float scale = 2.0 / maxDim;

        for (size_t i = 0; i < g_uavMesh.vertices.size(); i++)
        {
            g_uavMesh.vertices[i] *= scale;
        }
    }
    else
    {
        std::cerr << "Failed to load UAV model - will use fallback cube\n";
    }

    // Load UAV texture
    g_uavTexture = loadBMP("txtr01.bmp");
}

/*
    Purpose: Main function to set up GLUT, OpenGL, and start the simulation
    Inputs:
        int argc - argument count
        char **argv - argument vector
    Outputs:
        int - exit code
*/
int main(int argc, char **argv)
{
    // init UAVs
    initUAVs();

    // init GLUT / OpenGL
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(WINDOW_WIDTH, WINDOW_HEIGHT);
    glutCreateWindow("ECE 4122 UAV Simulation");

    initOpenGL();

    glutDisplayFunc(displayCallback);
    glutReshapeFunc(reshapeCallback);
    glutTimerFunc(30, timerCallback, 0);

    // main loop
    glutMainLoop();

    return 0;
}