// Include GLFW
#include <GLFW/glfw3.h>
extern GLFWwindow *window; // The "extern" keyword here is to access the variable "window" declared in tutorialXXX.cpp. This is a hack to keep the tutorials simple. Please avoid this.

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include "controls.hpp"

glm::mat4 ViewMatrix;
glm::mat4 ProjectionMatrix;

glm::mat4 getViewMatrix()
{
	return ViewMatrix;
}
glm::mat4 getProjectionMatrix()
{
	return ProjectionMatrix;
}

// Initial position : on +Z
glm::vec3 position = glm::vec3(0.0f, 0.0f, 12.0f);
// Initial horizontal angle : toward -Z
float horizontalAngle = 3.14f;
// Initial vertical angle : none
float verticalAngle = -0.22f;
// Initial Field of View
float initialFoV = 60.0f;

float speed = 2.0f; // 3 units / second
float mouseSpeed = 0.00005f;

static float orbitPitch = -0.22f;
static float orbitRadius = 12.0f;
static const glm::vec3 orbitTarget(0.0f, 0.0f, 0.0f);

void computeMatricesFromInputs()
{

	// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Get mouse position
	static bool firstMouse = true;
	static double lastX = 512.0, lastY = 384.0;
	double xpos, ypos;
	glfwGetCursorPos(window, &xpos, &ypos);
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}
	double dx = xpos - lastX;
	double dy = ypos - lastY;
	lastX = xpos;
	lastY = ypos;

	// Compute new orientation
	horizontalAngle -= mouseSpeed * float(dx);
	orbitPitch -= mouseSpeed * float(dy);

	// Clamp pitch to avoid gimbal lock
	orbitPitch = glm::clamp(orbitPitch, -1.5f, 1.5f);

	// Zoom in
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
		orbitRadius -= speed * deltaTime;

	// Zoom out
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
		orbitRadius += speed * deltaTime;

	// Rotate right
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
		horizontalAngle -= 0.8f * deltaTime;

	// Rotate left
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
		horizontalAngle += 0.8f * deltaTime;

	// Rotate camera up
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
		orbitPitch += 0.8f * deltaTime;

	// Rotate camera down
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
		orbitPitch -= 0.8f * deltaTime;

	// Rotate camera right
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		horizontalAngle -= 0.8f * deltaTime;

	// Rotate camera left
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
		horizontalAngle += 0.8f * deltaTime;

	// Prevent zooming too close
	orbitRadius = glm::max(orbitRadius, 1.0f);

	// Calculate camera position in orbit
	glm::vec3 camPos = orbitTarget + glm::vec3(
										 orbitRadius * cosf(orbitPitch) * sinf(horizontalAngle),
										 orbitRadius * sinf(orbitPitch),
										 orbitRadius * cosf(orbitPitch) * cosf(horizontalAngle));

	// Calculate camera orientation
	glm::vec3 direction = glm::normalize(orbitTarget - camPos);
	glm::vec3 right = glm::normalize(glm::cross(direction, glm::vec3(0, 1, 0)));
	glm::vec3 up = glm::normalize(glm::cross(right, direction));
	position = camPos;

	float FoV = initialFoV;

	// Projection matrix : 60° Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
	ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
	// Camera matrix
	ViewMatrix = glm::lookAt(
		position,			  // Camera is here
		position + direction, // and looks here : at the same position, plus "direction"
		up					  // Head is up (set to 0,-1,0 to look upside-down)
	);

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;
}