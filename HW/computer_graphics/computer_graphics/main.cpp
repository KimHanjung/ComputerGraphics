// InClass09
//      Mouse: Arcball manipulation
//      Keyboard: 'r' - reset arcball

#include <GL/glew.h> 
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <cmath>
#include <time.h>
#include <shader.h>
#include <Model.h>
#include <text.h>
#include <cube.h>
#include <plane.h>
#include <arcball.h>
#include "link.h"
#include <keyframe.h>

using namespace std;

// Function Prototypes
GLFWwindow *glAllInit();
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
void mouse_button_callback(GLFWwindow *window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow *window, double x, double y);
unsigned int loadTexture(const char *path);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void render();
int check_in(int index, float x, float z);
void initLinks();
void drawLinks(Link *root, float t, glm::mat4 cmodel, Shader *shader);
void initKeyframes();
void updateAnimData();

// Global variables
GLFWwindow *mainWindow = NULL;
Shader *shader = NULL;
Shader *shaderSingleColor = NULL;
Shader *modelShader = NULL;
Shader *textShader = NULL;
Shader *globalShader = NULL;
Shader *lampShader = NULL;
unsigned int SCR_WIDTH = 600;
unsigned int SCR_HEIGHT = 600;
Link *root;
Cube *cube;
Cube *lamp;
Model *ourModel;
Text *text = NULL;
Plane *plane;
glm::mat4 projection, view, model;
float cube_pos[10][2];
bool finish = false;

// for camera
glm::vec3 cameraOrigPos(0.0f, 15.0f, 15.0f);
glm::vec3 cameraPos;
glm::vec3 camTarget(0.0f, 0.0f, 0.0f);
glm::vec3 camUp(0.0f, 1.0f, 0.0f);

glm::vec3 modelPan(0.0f, 0.0f, 0.0f);
float modelPanStep = 0.2f;


// for lighting
glm::vec3 lightSize(0.1f, 0.1f, 0.1f);
glm::vec3 spotlightPos(0.0f, 10.0f, 0.0f);
glm::vec3 spotlightDirection = -spotlightPos;


// for lighting2
glm::vec3 lightPos(20.0f, 20.0f, 20.0f);
glm::vec3 lightColor(1.0f, 1.0f, 1.0f);
float ambientStrenth = 0.5f;
glm::vec3 objectColor(1.0f, 0.5f, 0.31f);


// for arcball
float arcballSpeed = 0.1f;
static Arcball camArcBall(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);

// for texture
unsigned int cubeTexture;
unsigned int floorTexture;

// for outline
float outlineScale = 1.1f;

// for animation
enum RenderMode { INIT, ANIM };
RenderMode renderMode;                  // current rendering mode
float beginT;                           // animation beginning time (in sec)
float timeT;                            // current time (in sec)
const float animEndTime = 5.0f;         // ending time of animation (in sec)
float xTrans, yTrans, zTrans;           // current translation factors
float xAngle, yAngle, zAngle;           // current rotation factors
KeyFraming xTKF(4), yTKF(4), zTKF(4);   // translation keyframes
KeyFraming xRKF(4), yRKF(4), zRKF(4);   // rotation keyframes

int main()
{
	mainWindow = glAllInit();
	srand(time(NULL));
	for (int i = 0; i < 10; i++)
	{
		do {
			cube_pos[i][0] = 5 - rand() % 11;
			cube_pos[i][1] = 5 - rand() % 11;
		} while ((check_in(i, cube_pos[i][0], cube_pos[i][1]) == 1) && (!((cube_pos[i][0] >= -0.5 && cube_pos[i][0] <= 0.5) && (cube_pos[i][1] >= -0.5 && cube_pos[i][1] <= 0.5))));
	}

	// shader loading and compile (by calling the constructor)
	shader = new Shader("2.stencil_testing.vs", "2.stencil_testing.fs");
	shaderSingleColor = new Shader("2.stencil_testing.vs", "2.stencil_single_color.fs");
	modelShader = new Shader("res/shaders/modelLoading.vs", "res/shaders/modelLoading.frag");
	textShader = new Shader("text.vs", "text.frag");

	// shader loading and compile (by calling the constructor)
	globalShader = new Shader("basic_lighting.vs", "basic_lighting.fs");

	// lighting parameters to fragment shader
	globalShader->use();
	globalShader->setVec3("lightColor", lightColor);
	globalShader->setVec3("objectColor", objectColor);
	globalShader->setVec3("lightPos", lightPos);
	globalShader->setFloat("ambientStrenth", ambientStrenth);

	// shader loading and compile (by calling the constructor)
	lampShader = new Shader("lamp.vs", "lamp.fs");


	// projection matrix
	projection = glm::perspective(glm::radians(45.0f),
		(float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
	globalShader->setMat4("projection", projection);
	shader->use();
	shader->setMat4("projection", projection);
	shaderSingleColor->use();
	shaderSingleColor->setMat4("projection", projection);
	modelShader->use();
	modelShader->setMat4("projection", projection);

	//modeling & text
	ourModel = new Model((GLchar *)"res/models/bomb/Bomb.obj");
	text = new Text("fonts/arial.ttf", textShader, SCR_WIDTH, SCR_HEIGHT);

	// set defaul camera position
	cameraPos = cameraOrigPos;

	// load textures
	// -------------
	cubeTexture = loadTexture("marble.jpg");
	floorTexture = loadTexture("metal.png");

	shader->use();
	shader->setInt("texture1", 0);

	// transfer texture id to fragment shader
	shader->use();
	shader->setInt("material.diffuse", 0);
	shader->setInt("material.specular", 1);
	shader->setFloat("material.shininess", 32);

	shader->setVec3("viewPos", cameraPos);

	// transfer lighting parameters to fragment shader

	// directional light
	shader->setVec3("dirLight.direction", -0.2f, -1.0f, -0.3f);
	shader->setVec3("dirLight.ambient", 0.05, 0.05f, 0.05f);
	shader->setVec3("dirLight.diffuse", 0.6f, 0.6f, 0.6f);
	shader->setVec3("dirLight.specular", 0.8f, 0.8f, 0.8f);

	// spotLight
	shader->setVec3("spotLight.position", spotlightPos);
	//shader->setVec3("spotLight.direction", spotlightDirection);
	shader->setVec3("spotLight.ambient", 1.0f, 1.0f, 1.0f);
	shader->setVec3("spotLight.diffuse", 1.0f, 1.0f, 1.0f);
	shader->setVec3("spotLight.specular", 1.0f, 1.0f, 1.0f);
	shader->setFloat("spotLight.constant", 1.0f);
	shader->setFloat("spotLight.linear", 0.09);
	shader->setFloat("spotLight.quadratic", 0.032);
	shader->setFloat("spotLight.cutOff", glm::cos(glm::radians(3.5f)));
	shader->setFloat("spotLight.outerCutOff", glm::cos(glm::radians(7.5f)));

	// create a cube and a plane
	cube = new Cube();
	plane = new Plane();

	// initialize animation data
	initLinks();
	timeT = 0.0f;
	renderMode = INIT;

	initKeyframes();
	updateAnimData();

	while (!glfwWindowShouldClose(mainWindow)) {
		render();
		glfwPollEvents();
	}

	glfwTerminate();
	return 0;
}


void initKeyframes() {

	// x-translation keyframes
	xTKF.setKey(0, 0, 0.0);
	xTKF.setKey(1, 1.5, -2.0);
	xTKF.setKey(2, 3.0, 2.0);
	xTKF.setKey(3, animEndTime, 0.0);

	// y-translation keyframes
	yTKF.setKey(0, 0, 0.0);
	yTKF.setKey(1, 1.5, -2.0);
	yTKF.setKey(2, 3.0, 2.0);
	yTKF.setKey(3, animEndTime, 0.0);

	// z-translation keyframes
	zTKF.setKey(0, 0, 0.0);
	zTKF.setKey(1, 1.5, -2.0);
	zTKF.setKey(2, 3.0, 2.0);
	zTKF.setKey(3, animEndTime, 0.0);

	// x-rotation keyframes
	xRKF.setKey(0, 0, 0.0);
	xRKF.setKey(1, 1.5, 20.0);
	xRKF.setKey(2, 3.0, 80.0);
	xRKF.setKey(3, animEndTime, 0.0);

	// y-rotation keyframes
	yRKF.setKey(0, 0, 0.0);
	yRKF.setKey(1, 1.5, -30.0);
	yRKF.setKey(2, 3.0, 50.0);
	yRKF.setKey(3, animEndTime, 0.0);

	// z-rotation keyframes
	zRKF.setKey(0, 0, 0.0);
	zRKF.setKey(1, 1.5, 90.0);
	zRKF.setKey(2, 3.0, 180.0);
	zRKF.setKey(3, animEndTime, 200.0);
}

void updateAnimData() {
	if (timeT > animEndTime) {
		timeT = animEndTime;
	}
	xTrans = xTKF.getValLinear(timeT);
	yTrans = yTKF.getValLinear(timeT);
	zTrans = zTKF.getValLinear(timeT);
	xAngle = xRKF.getValLinear(timeT);
	yAngle = yRKF.getValLinear(timeT);
	zAngle = zRKF.getValLinear(timeT);
}

void initLinks()
{
	//Link(string name, glm::vec3 color, bool isRoot, int nChild,
	//     glm::vec3 size,
	//     glm::vec3 offset,
	//     glm::vec3 trans1,
	//     glm::vec3 trans2,
	//     glm::vec3 rot1,       //angles in degree
	//     glm::vec3 rot2)       //angles in degree

	// root link: yellow
	root = new Link("ROOT", glm::vec3(1.0, 1.0, 0.0), true, 1,
		glm::vec3(0.5, 3.0, 1.0),   // size
		glm::vec3(0.0, 0.0, 0.0),   // offset
		glm::vec3(0.0, 0.0, 0.0),   // trans1 w.r.t. origin (because root)
		glm::vec3(0.0, 0.0, 0.0),   // trans2 w.r.t. origin (because root)
		glm::vec3(0.0, 0.0, 0.0),   // no rotation
		glm::vec3(0.0, 0.0, 0.0));  // no rotation

									// left upper arm: red
	root->child[0] = new Link("LEFT_ARM_UPPER", glm::vec3(1.0, 0.0, 0.0), false, 1,
		glm::vec3(0.5, 3.6, 1.0),  // size
		glm::vec3(0.0, 3.0, 0.0),  // offset
		glm::vec3(0.0, 0.0, 0.0),  // trans1
		glm::vec3(-0.3, 2.8, 0.0),  // trans2
		glm::vec3(0.0, 0.0, 0.0),  // rotation about parent
		glm::vec3(0.0, 0.0, -150.0));

	// left low arm: orange
	root->child[0]->child[0] = new Link("LEFT_ARM_LOWER", glm::vec3(1.0, 0.5, 0.0), false, 0,
		glm::vec3(0.5, 3.0, 1.0),  // size
		glm::vec3(0.0, 6.3, 0.0),  // offset
		glm::vec3(0.0, 0.0, 0.0),
		glm::vec3(2.0, 9.0, 0.0),
		glm::vec3(0.0, 0.0, 0.0),
		glm::vec3(0.0, 0.0, 150.0));

}

void drawLinks(Link *clink, float t, glm::mat4 cmodel, Shader *shader)
{

	if (t > 1.0) t = 1.0f;

	glm::mat4 thisMat = glm::mat4(1.0f);

	// accumulate the parent's transformation
	thisMat = thisMat * cmodel;

	// if root, interpolates the translation
	glm::vec3 ctrans = glm::mix(clink->trans1, clink->trans2, t);
	thisMat = glm::translate(thisMat, ctrans);

	// interpolates the rotation
	//glm::quat q = glm::slerp(clink->q1, clink->q2, t);
	glm::vec3 euler = glm::mix(clink->rot1, clink->rot2, t);
	glm::quat q = glm::quat(euler);

	glm::mat4 crot = q.operator glm::mat4x4();

	thisMat = thisMat * crot;

	// render the link
	shader->use();
	shader->setMat4("model", thisMat);
	clink->draw(shader);

	// recursively call the drawLinks for the children
	for (int i = 0; i < clink->nChild; i++) {
		drawLinks(clink->child[i], t, thisMat, shader);
	}

}


// render
void render() {

	shader->use();
	shader->setVec3("spotLight.direction", modelPan + spotlightDirection);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

	ourModel->meshes[0].vertices[0].Normal;
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_STENCIL_TEST);

	// send view to shader
	view = glm::lookAt(cameraPos, camTarget, camUp);
	view = view * camArcBall.createRotationMatrix();
	shader->use();
	shader->setMat4("view", view);
	shaderSingleColor->use();
	shaderSingleColor->setMat4("view", view);
	modelShader->use();
	modelShader->setMat4("view", view);
	shader->use();
	if (finish == false)
	{
		// drawing a floor

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, floorTexture);
		model = glm::mat4(1.0);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(0.0f, -0.5f, 0.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(15.0f, 15.0f, 0.0f));
		shader->setMat4("model", model);
		plane->draw(shader);


		//drawing bomb

		modelShader->use();
		glm::mat4 model(1.0);

		model = glm::translate(model, modelPan);
		model = glm::scale(model, glm::vec3(1.5f, 1.5f, 1.5f));

		modelShader->setMat4("model", model);

		ourModel->Draw(modelShader);
		// drawing textured cubes

		shader->use();

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, cubeTexture);

		model = glm::mat4(1.0);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[0][0], 0.0f, cube_pos[0][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[1][0], 0.0f, cube_pos[1][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[2][0], 0.0f, cube_pos[2][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[3][0], 0.0f, cube_pos[3][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[4][0], 0.0f, cube_pos[4][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[5][0], 0.0f, cube_pos[5][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[6][0], 0.0f, cube_pos[6][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[7][0], 0.0f, cube_pos[7][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[8][0], 0.0f, cube_pos[8][1]));
		shader->setMat4("model", model);
		cube->draw(shader);


		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_ALWAYS, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		glStencilMask(0xFF);
		glClear(GL_STENCIL_BUFFER_BIT);

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[9][0], 0.0f, cube_pos[9][1]));
		shader->setMat4("model", model);
		cube->draw(shader);

		// drawing yellow cubes

		glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glStencilMask(0x00);


		shaderSingleColor->use();

		model = glm::mat4(1.0f);
		//model = glm::translate(model, modelPan);
		model = glm::translate(model, glm::vec3(cube_pos[9][0], 0.0f, cube_pos[9][1]));
		model = glm::scale(model, glm::vec3(1.1f, 1.1f, 1.1f));
		shaderSingleColor->setMat4("model", model);
		cube->draw(shaderSingleColor);


		glDisable(GL_STENCIL_TEST);
	}
	if ((check_in(10, modelPan.x - 0.25, modelPan.z - 0.25) == 1) || (check_in(10, modelPan.x - 0.25, modelPan.z + 0.25) == 1)
		|| (check_in(10, modelPan.x + 0.25, modelPan.z - 0.25) == 1) || (check_in(10, modelPan.x + 0.25, modelPan.z + 0.25) == 1))
	{
		textShader->use();
		finish = true;
		if ((check_in(9, modelPan.x - 0.25, modelPan.z - 0.25) == 1) || (check_in(9, modelPan.x - 0.25, modelPan.z + 0.25) == 1)
			|| (check_in(9, modelPan.x + 0.25, modelPan.z - 0.25) == 1) || (check_in(9, modelPan.x + 0.25, modelPan.z + 0.25) == 1))
		{
			text->RenderText("Fail", 370.0f, 570.0f, 0.5f, glm::vec3(0.3, 0.7f, 0.9f));

			// view matrix to fragment shader
			globalShader->use();
			globalShader->setMat4("view", view);

			// links
			model = glm::mat4(1.0f);
			if (renderMode == ANIM) {
				float cTime = (float)glfwGetTime(); // current time
				timeT = cTime - beginT;

				drawLinks(root, timeT / animEndTime, model, globalShader);
			}

		}
		else
		{
			text->RenderText("Success", 25.0f, 25.0f, 1.0f, glm::vec3(0.5, 0.8f, 0.2f));
			if (renderMode == ANIM) {
				float cTime = (float)glfwGetTime(); // current time
				timeT = cTime - beginT;
				updateAnimData();
			}

			// view matrix to fragment shader
			globalShader->use();
			globalShader->setMat4("view", view);

			// cube object
			model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(xTrans, yTrans, zTrans));
			glm::vec3 eulerAngles(glm::radians(xAngle), glm::radians(yAngle), glm::radians(zAngle));
			glm::quat q(eulerAngles);
			glm::mat4 rotMatrix = q.operator glm::mat4x4();
			model = model * rotMatrix;
			globalShader->setMat4("model", model);
			cube->draw(globalShader);
		}
	}


	glfwSwapBuffers(mainWindow);
}

// glAllInit();
GLFWwindow *glAllInit()
{
	GLFWwindow *window;

	// glfw: initialize and configure
	if (!glfwInit()) {
		printf("GLFW initialisation failed!");
		glfwTerminate();
		exit(-1);
	}
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// glfw window creation
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "hw2", NULL, NULL);
	if (window == NULL) {
		cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		exit(-1);
	}
	glfwMakeContextCurrent(window);
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// OpenGL states
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	// Set OpenGL options
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// Allow modern extension features
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK) {
		cout << "GLEW initialisation failed!" << endl;
		glfwDestroyWindow(window);
		glfwTerminate();
		exit(-1);
	}

	return window;
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;
		else {
			format = GL_RGBA;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}


// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and
	// height will be significantly larger than specified on retina displays.
	glViewport(0, 0, width, height);
	SCR_WIDTH = width;
	SCR_HEIGHT = height;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
	else if (key == GLFW_KEY_R && action == GLFW_PRESS) {

		if (finish == true || renderMode == INIT)
		{
			beginT = glfwGetTime();
			renderMode = ANIM;
		}
		else {  // renderMode == ANIM
			camArcBall.init(SCR_WIDTH, SCR_HEIGHT, arcballSpeed, true, true);
			cameraPos = cameraOrigPos;
			modelPan[0] = modelPan[1] = modelPan[2] = 0.0f;
			renderMode = INIT;
			timeT = 0.0f;
		}
	}
	else if (key == GLFW_KEY_LEFT && finish == false) {
		modelPan[0] -= modelPanStep;
	}
	else if (key == GLFW_KEY_RIGHT && finish == false) {
		modelPan[0] += modelPanStep;
	}
	else if (key == GLFW_KEY_DOWN && finish == false) {
		modelPan[2] += modelPanStep;
	}
	else if (key == GLFW_KEY_UP && finish == false) {
		modelPan[2] -= modelPanStep;
	}
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods) {
	camArcBall.mouseButtonCallback(window, button, action, mods);
}

void cursor_position_callback(GLFWwindow *window, double x, double y) {
	camArcBall.cursorCallback(window, x, y);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
	cameraPos[2] -= (yoffset * 0.5);
	cameraPos[1] -= (yoffset * 0.5);
}

int check_in(int index, float x, float z)
{
	for (int i = 0; i < index; i++)
	{
		if ((cube_pos[i][0] - 0.5 <= x) && (cube_pos[i][0] + 0.5 >= x) && (cube_pos[i][1] - 0.5 <= z) && (cube_pos[i][1] + 0.5 >= z))
			return 1;
	}

	return 0;
}