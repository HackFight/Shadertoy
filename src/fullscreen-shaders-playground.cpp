#define GLFW_INCLUDE_NONE
#ifdef _WIN32
#include <windows.h>
#endif // _WIN32
#include <glad/glad.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cerrno>
#include <GLFW/glfw3.h>
#include <openglErrorReporting.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <primitives/shapeGenerator.h>



GLuint programID;
GLuint numIndices;
float aspectRatio;
double crntTime = 0.0, prevTime = 0.0, deltaTime = 0.0, runTime = 0.0;



static void error_callback(int error, const char* description)
{
	std::cout << description << "\n";
}

bool CheckShaderStatus(GLuint shaderID)
{
	GLint compileStatus;
	glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);
	if (compileStatus != GL_TRUE)													// Log error if one
	{
		GLint infoLogLength;
		glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* buffer = new GLchar[infoLogLength];

		GLsizei bufferSize;
		glGetShaderInfoLog(shaderID, infoLogLength, &bufferSize, buffer);
		error_callback(0, buffer);

		delete[] buffer;
		return false;
	}
	return true;
}

bool CheckProgramStatus(GLuint programID)
{
	GLint linkStatus;
	glGetProgramiv(programID, GL_LINK_STATUS, &linkStatus);
	if (linkStatus != GL_TRUE)													// Log error if one
	{
		GLint infoLogLength;
		glGetProgramiv(programID, GL_INFO_LOG_LENGTH, &infoLogLength);
		GLchar* buffer = new GLchar[infoLogLength];

		GLsizei bufferSize;
		glGetProgramInfoLog(programID, infoLogLength, &bufferSize, buffer);
		error_callback(0, buffer);

		delete[] buffer;
		return false;
	}
	return true;
}

std::string get_file_contents(const char* filename)
{
	std::ifstream in(filename, std::ios::binary);
	if (in)
	{
		std::string contents;
		in.seekg(0, std::ios::end);
		contents.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&contents[0], contents.size());
		in.close();
		return(contents);
	}
	throw(errno);
}

int main(void)
{
#ifdef _WIN32
#ifdef NDEBUG
	myConsole = GetConsoleWindow();
	ShowWindow(myConsole, 0);
#endif //NDEBUG
#endif // _WIN32


	glfwSetErrorCallback(error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, 1);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
#endif

	GLFWwindow* window = glfwCreateWindow(640, 480, "Shadertoy", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	enableReportGlErrors();

	//glfwSwapInterval(1); //vsync
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); //wireframe
	glEnable(GL_DEPTH_TEST);

	//##### - DATA - #####//
	ShapeData square = ShapeGenerator::MakeSquare();

	GLuint vertexBufferID;
	glGenBuffers(1, &vertexBufferID);
	glBindBuffer(GL_ARRAY_BUFFER, vertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, square.GetVertexBufferSize(), square.vertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (char*)(sizeof(float) * 3));


	GLuint indexBufferID;
	glGenBuffers(1, &indexBufferID);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBufferID);
	numIndices = square.numIndices;
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, square.GetIndexBufferSize(), square.indices, GL_STATIC_DRAW);

	square.CleanUp();																	//Don't forget this!

	//##### - SHADERS - #####//
	GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	std::string vertexCode = get_file_contents(RESOURCES_PATH "vert.shader");
	std::string fragmentCode = get_file_contents(RESOURCES_PATH "frag.shader");

	const char* vertexSource = vertexCode.c_str();
	const char* fragmentSource = fragmentCode.c_str();

	glShaderSource(vertexShaderID, 1, &vertexSource, NULL);							//Make shaders
	glShaderSource(fragmentShaderID, 1, &fragmentSource, NULL);						//

	glCompileShader(vertexShaderID);
	glCompileShader(fragmentShaderID);

	if (!CheckShaderStatus(vertexShaderID) || !CheckShaderStatus(fragmentShaderID)) //If shader compile failed --> Give up
		return -1;

	programID = glCreateProgram();											// C++ compiling shit
	glAttachShader(programID, vertexBufferID);										//
	glAttachShader(programID, fragmentShaderID);									//
	glLinkProgram(programID);														//

	if (!CheckProgramStatus(programID))												//If program linking failed --> Give up (doesn't work on this PC architecture TwT)
		return -1;

	glDeleteShader(vertexBufferID);
	glDeleteShader(fragmentShaderID);	//Clean... don't forget

	glUseProgram(programID);														// Activate shaders/program


	// ### - Main loop - ### //
	while (!glfwWindowShouldClose(window))
	{
		int width = 0, height = 0;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT);

		crntTime = glfwGetTime();
		deltaTime = crntTime - prevTime;
		prevTime = crntTime;
		runTime += deltaTime;


		glm::mat4 trans = glm::mat4(1.0f);
		GLint transfLoc = glGetUniformLocation(programID, "transformMatrix");
		glUniformMatrix4fv(transfLoc, 1, GL_FALSE, glm::value_ptr(trans));

		glm::vec2 iResolution = glm::vec2((float)width, (float)height);
		GLint iResolutionLoc = glGetUniformLocation(programID, "iResolution");
		glUniform2f(iResolutionLoc, iResolution.x, iResolution.y);

		GLint iTimeLoc = glGetUniformLocation(programID, "iTime");
		glUniform1f(iTimeLoc, (float)runTime);


		glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, 0);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glUseProgram(0);
	glDeleteProgram(programID);

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}