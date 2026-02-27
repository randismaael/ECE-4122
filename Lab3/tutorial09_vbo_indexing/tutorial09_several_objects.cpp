// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>
GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/constants.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>
#include <common/objloader.hpp>
#include <common/vboindexer.hpp>
#include <cfloat> //for helper

// helper function to get box outline vertices
struct AABB
{
	glm::vec3 minv, maxv;
};

// vertex struct for ground quad
struct GV
{
	glm::vec3 p;
	glm::vec2 uv;
	glm::vec3 n;
};

static AABB computeAABB(const std::vector<glm::vec3> &v)
{
	AABB b;
	b.minv = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	b.maxv = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	for (size_t i = 0; i < v.size(); ++i)
	{
		const glm::vec3 &p = v[i];
		b.minv = glm::min(b.minv, p); // componentwise min  (x,y,z)
		b.maxv = glm::max(b.maxv, p); // componentwise max  (x,y,z)
	}
	return b;
}

int main(void)
{
	// Initialize GLFW
	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		getchar();
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make macOS happy; should not be needed
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow(1024, 768, "Tutorial 09 - Rendering several models", NULL, NULL);
	if (window == NULL)
	{
		fprintf(stderr, "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
		getchar();
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "Failed to initialize GLEW\n");
		getchar();
		glfwTerminate();
		return -1;
	}

	glfwSwapInterval(1); // vsync to avoid VNC choking

	// input
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// stop mouse
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	glfwPollEvents();

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	// Hide the mouse and enable unlimited movement
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set the mouse at the center of the screen
	glfwPollEvents();
	// glfwSetCursorPos(window, 1024/2, 768/2); messes w/ mouse

	// Dark blue background
	glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it is closer to the camera than the former one
	glDepthFunc(GL_LESS);

	// Cull triangles which normal is not towards the camera
	glEnable(GL_CULL_FACE);

	GLuint VertexArrayID;
	glGenVertexArrays(1, &VertexArrayID);
	glBindVertexArray(VertexArrayID);

	// Create and compile our GLSL program from the shaders
	GLuint programID = LoadShaders("tutorial09_vbo_indexing/StandardShading.vertexshader", "tutorial09_vbo_indexing/StandardShading.fragmentshader");

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = glGetUniformLocation(programID, "MVP");
	GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
	GLuint ModelMatrixID = glGetUniformLocation(programID, "M");

	// Load the texture
	GLuint Texture = loadDDS("tutorial09_vbo_indexing/uvmap.DDS");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID = glGetUniformLocation(programID, "myTextureSampler");

	GLint uEnableDS = glGetUniformLocation(programID, "uEnableDS");
	int enableDS = 1;

	// Read our .obj file
	std::vector<glm::vec3> vertices;
	std::vector<glm::vec2> uvs;
	std::vector<glm::vec3> normals;
	bool res = loadOBJ("tutorial09_vbo_indexing/suzanne.obj", vertices, uvs, normals);

	std::vector<unsigned short> indices;
	std::vector<glm::vec3> indexed_vertices;
	std::vector<glm::vec2> indexed_uvs;
	std::vector<glm::vec3> indexed_normals;
	indexVBO(vertices, uvs, normals, indices, indexed_vertices, indexed_uvs, indexed_normals);

	// for ring placement, AABB
	AABB box = computeAABB(vertices);
	float s = 1.0f;

	// post-scale half width in X
	float halfWidth = 0.5f * (box.maxv.x - box.minv.x) * s;
	float R = halfWidth / sinf(glm::pi<float>() / 8.0f);

	// chins on ground, lowest is z=0
	float minZ = box.minv.z * s;
	float chinLift = -minZ;

	// 8 models
	glm::mat4 modelMatrices[8];
	{
		for (int i = 0; i < 8; ++i)
		{
			float ang = glm::radians(45.0f * i);
			glm::mat4 M(1.0f);

			M = glm::rotate(M, ang, glm::vec3(0, 0, 1));
			M = glm::translate(M, glm::vec3(R, 0, 0));

			// look forward
			// switch from -z to +x
			M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 1, 0));
			M = glm::rotate(M, glm::radians(180.0f), glm::vec3(0, 1, 0));

			// chin on z=0 and apply uniform scale
			M = glm::translate(M, glm::vec3(0, 0, chinLift));
			M = glm::scale(M, glm::vec3(s));

			modelMatrices[i] = M;
		}
	}

	// Load it into a VBO

	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_vertices.size() * sizeof(glm::vec3), &indexed_vertices[0], GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_uvs.size() * sizeof(glm::vec2), &indexed_uvs[0], GL_STATIC_DRAW);

	GLuint normalbuffer;
	glGenBuffers(1, &normalbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
	glBufferData(GL_ARRAY_BUFFER, indexed_normals.size() * sizeof(glm::vec3), &indexed_normals[0], GL_STATIC_DRAW);

	// Generate a buffer for the indices as well
	GLuint elementbuffer;
	glGenBuffers(1, &elementbuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), &indices[0], GL_STATIC_DRAW);

	// ground quad
	float gsize = R * 1.4f; // slightly larger than ring
	GV gVerts[4];
	gVerts[0].p = glm::vec3(-gsize, -gsize, 0);
	gVerts[0].uv = glm::vec2(0, 0);
	gVerts[0].n = glm::vec3(0, 0, 1);
	gVerts[1].p = glm::vec3(gsize, -gsize, 0);
	gVerts[1].uv = glm::vec2(1, 0);
	gVerts[1].n = glm::vec3(0, 0, 1);
	gVerts[2].p = glm::vec3(gsize, gsize, 0);
	gVerts[2].uv = glm::vec2(1, 1);
	gVerts[2].n = glm::vec3(0, 0, 1);
	gVerts[3].p = glm::vec3(-gsize, gsize, 0);
	gVerts[3].uv = glm::vec2(0, 1);
	gVerts[3].n = glm::vec3(0, 0, 1);

	unsigned short gIdx[6] = {0, 1, 2, 0, 2, 3};
	GLuint gVAO, gVBO, gEBO;
	glGenVertexArrays(1, &gVAO);
	glBindVertexArray(gVAO);
	glGenBuffers(1, &gVBO);
	glBindBuffer(GL_ARRAY_BUFFER, gVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gVerts), gVerts, GL_STATIC_DRAW);
	glGenBuffers(1, &gEBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gIdx), gIdx, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(GV), (void *)offsetof(GV, p));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(GV), (void *)offsetof(GV, uv));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(GV), (void *)offsetof(GV, n));
	glEnableVertexAttribArray(2);
	glBindVertexArray(0);

	// Get a handle for our "LightPosition" uniform
	glUseProgram(programID);
	GLuint LightID = glGetUniformLocation(programID, "LightPosition_worldspace");

	// For speed computation
	double lastTime = glfwGetTime();
	int nbFrames = 0;

	do
	{

		// Measure speed
		double currentTime = glfwGetTime();
		nbFrames++;
		if (currentTime - lastTime >= 1.0)
		{ // If last prinf() was more than 1sec ago
			// printf and reset
			printf("%f ms/frame\n", 1000.0 / double(nbFrames));
			nbFrames = 0;
			lastTime += 1.0;
		}

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();

		// ////// Start of the rendering of the first object //////

		// // Use our shader
		// glUseProgram(programID);

		// glm::vec3 lightPos = glm::vec3(4,4,4);
		// glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		// glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]); // This one doesn't change between objects, so this can be done once for all objects that use "programID"

		// glm::mat4 ModelMatrix1 = glm::mat4(1.0);
		// glm::mat4 MVP1 = ProjectionMatrix * ViewMatrix * ModelMatrix1;

		// // Send our transformation to the currently bound shader,
		// // in the "MVP" uniform
		// glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP1[0][0]);
		// glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix1[0][0]);

		// // Bind our texture in Texture Unit 0
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_2D, Texture);
		// // Set our "myTextureSampler" sampler to use Texture Unit 0
		// glUniform1i(TextureID, 0);

		// // 1rst attribute buffer : vertices
		// glEnableVertexAttribArray(0);
		// glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		// glVertexAttribPointer(
		// 	0,                  // attribute
		// 	3,                  // size
		// 	GL_FLOAT,           // type
		// 	GL_FALSE,           // normalized?
		// 	0,                  // stride
		// 	(void*)0            // array buffer offset
		// );

		// // 2nd attribute buffer : UVs
		// glEnableVertexAttribArray(1);
		// glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		// glVertexAttribPointer(
		// 	1,                                // attribute
		// 	2,                                // size
		// 	GL_FLOAT,                         // type
		// 	GL_FALSE,                         // normalized?
		// 	0,                                // stride
		// 	(void*)0                          // array buffer offset
		// );

		// // 3rd attribute buffer : normals
		// glEnableVertexAttribArray(2);
		// glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		// glVertexAttribPointer(
		// 	2,                                // attribute
		// 	3,                                // size
		// 	GL_FLOAT,                         // type
		// 	GL_FALSE,                         // normalized?
		// 	0,                                // stride
		// 	(void*)0                          // array buffer offset
		// );

		// // Index buffer
		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// // Draw the triangles !
		// glDrawElements(
		// 	GL_TRIANGLES,      // mode
		// 	indices.size(),    // count
		// 	GL_UNSIGNED_SHORT,   // type
		// 	(void*)0           // element array buffer offset
		// );

		////// End of rendering of the first object //////
		////// Start of the rendering of the second object //////

		// In our very specific case, the 2 objects use the same shader.
		// So it's useless to re-bind the "programID" shader, since it's already the current one.
		// glUseProgram(programID);

		// Similarly : don't re-set the light position and camera matrix in programID,
		// it's still valid !
		// *** You would have to do it if you used another shader ! ***
		// glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		// glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]); // This one doesn't change between objects, so this can be done once for all objects that use "programID"

		// Again : this is already done, but this only works because we use the same shader.
		//// Bind our texture in Texture Unit 0
		// glActiveTexture(GL_TEXTURE0);
		// glBindTexture(GL_TEXTURE_2D, Texture);
		//// Set our "myTextureSampler" sampler to use Texture Unit 0
		// glUniform1i(TextureID, 0);

		// // BUT the Model matrix is different (and the MVP too)
		// glm::mat4 ModelMatrix2 = glm::mat4(1.0);
		// ModelMatrix2 = glm::translate(ModelMatrix2, glm::vec3(2.0f, 0.0f, 0.0f));
		// glm::mat4 MVP2 = ProjectionMatrix * ViewMatrix * ModelMatrix2;

		// // Send our transformation to the currently bound shader,
		// // in the "MVP" uniform
		// glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP2[0][0]);
		// glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix2[0][0]);

		// // The rest is exactly the same as the first object

		// // 1rst attribute buffer : vertices
		// glEnableVertexAttribArray(0);
		// glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		// glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// // 2nd attribute buffer : UVs
		// glEnableVertexAttribArray(1);
		// glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		// glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// // 3rd attribute buffer : normals
		// glEnableVertexAttribArray(2);
		// glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		// glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

		// // Index buffer
		// glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// // Draw the triangles !
		// glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_SHORT, (void*)0);

		// ////// End of rendering of the second object //////

		glUseProgram(programID);

		// for L
		static int lastL = GLFW_RELEASE;
		int nowL = glfwGetKey(window, GLFW_KEY_L);
		if (nowL == GLFW_PRESS && lastL == GLFW_RELEASE)
			enableDS = 1 - enableDS;
		lastL = nowL;
		glUniform1i(uEnableDS, enableDS);

		glm::vec3 lightPos = glm::vec3(4, 4, 4);
		glUniform3f(LightID, lightPos.x, lightPos.y, lightPos.z);
		glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &ViewMatrix[0][0]);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Texture);
		glUniform1i(TextureID, 0);

		glBindVertexArray(VertexArrayID); // bind monkey
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void *)0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, normalbuffer);
		glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, elementbuffer);

		// we need to render 8 objects now
		// draw 8
		for (int i = 0; i < 8; ++i)
		{
			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * modelMatrices[i];
			glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &modelMatrices[i][0][0]);
			glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, (void *)0);
		}

		// z=0 ground
		glBindVertexArray(gVAO);
		glm::mat4 Mg(1.0f);
		glm::mat4 MVPg = ProjectionMatrix * ViewMatrix * Mg;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVPg[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Mg[0][0]);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void *)0);
		glBindVertexArray(0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0);

	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteBuffers(1, &normalbuffer);
	glDeleteBuffers(1, &elementbuffer);

	glDeleteBuffers(1, &gVBO);
	glDeleteBuffers(1, &gEBO);
	glDeleteVertexArrays(1, &gVAO);

	glDeleteProgram(programID);
	glDeleteTextures(1, &Texture);
	glDeleteVertexArrays(1, &VertexArrayID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
