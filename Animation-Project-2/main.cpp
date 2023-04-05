#include "OpenGL.h"
#include "sCamera.h"
#include "cMeshInfo/cMeshInfo.h"
#include "Draw Mesh/DrawMesh.h"
#include "Draw Bounding Box/DrawBoundingBox.h"

#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "cModelFileLoader/cModelFileLoader.h"
#include "cShaderManager/cShaderManager.h"
#include "cVAOManager/cVAOManager.h"
#include "cLightManager/cLightManager.h"
#include "cBasicTextureManager/cBasicTextureManager.h"
#include "cAnimationManager/cAnimationManager.h"
#include "cFBO/cFBO.h"

#include <iostream>
#include <sstream>
#include <fstream>
#include <vector>
#include <chrono>

#include <stdlib.h>
#include <stdio.h>

GLFWwindow* window;
GLint mvp_location = 0;
GLuint shaderID = 0;

cModelFileLoader* modelLoader;
cVAOManager* VAOMan;
cBasicTextureManager* TextureMan;
cLightManager* LightMan;
cFBO* FrameBuffer;

cAnimationManager AnimeMan;

sModelDrawInfo player_obj;

cMeshInfo* full_screen_quad;
cMeshInfo* skybox_sphere_mesh;
cMeshInfo* player_mesh;

cMeshInfo* bulb_mesh;
cLight* pointLight;

unsigned int readIndex = 0;
int object_index = 0;
int elapsed_frames = 0;
int index = 0;

bool enableMouse = false;
bool useFBO = false;
bool mouseClick = false;
bool fullScreen = false;

float collectiveSpeed = 1.f;
float temp = 0.f;

std::vector <std::string> meshFiles;
std::vector <cMeshInfo*> meshArray;

void ReadFromFile(std::string filePath);
void LoadTextures(void);
void ManageLights(void);
float RandomFloat(float a, float b);
void LoadPlyFilesIntoVAO(void);
void RenderToFBO(GLFWwindow* window, sCamera* camera, GLuint eyeLocationLocation, 
                 GLuint viewLocation, GLuint projectionLocation,
                 GLuint modelLocaction, GLuint modelInverseLocation,
                 GLuint BoneMatricesLocation[4], GLuint BoneRotationMatricesLocation[4]);
glm::mat4 CreateModelMatrix(const glm::mat4& parentModelMatrix,
    const glm::vec3& translate, const glm::quat& rotate);

void LoadAnimations();

const glm::vec3 origin = glm::vec3(0);
const glm::mat4 matIdentity = glm::mat4(1.0f);
const glm::vec3 upVector = glm::vec3(0.f, 1.f, 0.f);

glm::vec3 m_Bone1Position;
glm::vec3 m_Bone2Position;
glm::vec3 m_Bone3Position;
glm::quat m_Bone1Rotation;
glm::quat m_Bone2Rotation;
glm::quat m_Bone3Rotation;

// attenuation on all lights
glm::vec4 constLightAtten = glm::vec4(1.0f);

double g_prevTime, g_currentTime, g_elapsedTime;

enum eEditMode
{
    MOVING_CAMERA,
    MOVING_LIGHT,
    MOVING_SELECTED_OBJECT,
    TAKE_CONTROL
};

// Camera moved into a different struct
// glm::vec3 cameraEye = glm::vec3(-280.0, 140.0, -700.0);
// glm::vec3 cameraTarget = glm::vec3(0.f, 0.f, 0.f);

sCamera* camera;

glm::vec3 initCamera;

eEditMode theEditMode = MOVING_CAMERA;

float yaw = 0.f;
float pitch = 0.f;
float fov = 45.f;

// mouse state
bool firstMouse = true;
float lastX = 800.f / 2.f;
float lastY = 600.f / 2.f;

float beginTime = 0.f;
float currentTime = 0.f;
float timeDiff = 0.f;
int frameCount = 0;

// Pre-existing light, independent of the scene lighting
float ambientLight = 1.f;

static void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
    {
        theEditMode = MOVING_CAMERA;
    }
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        theEditMode = MOVING_SELECTED_OBJECT;
    }
    if (key == GLFW_KEY_M && action == GLFW_PRESS) 
    {
        theEditMode = MOVING_LIGHT;
    }
    if (key == GLFW_KEY_F && action == GLFW_PRESS) 
    {
        theEditMode = TAKE_CONTROL;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) {
        collectiveSpeed = 1.f;
    }
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) {
        collectiveSpeed = 2.f;
    }
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) {
        collectiveSpeed = 3.f;
    }
    if (key == GLFW_KEY_4 && action == GLFW_PRESS) {
        collectiveSpeed = 4.f;
    }
    if (key == GLFW_KEY_5 && action == GLFW_PRESS) {
        collectiveSpeed = 5.f;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
        temp = collectiveSpeed;
        collectiveSpeed = 0.f;
    }
    if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE) {
        collectiveSpeed = temp;
    }
    if (key == GLFW_KEY_GRAVE_ACCENT && action == GLFW_RELEASE) {
        collectiveSpeed *= -1.f;
    }

    // Wireframe
    if (key == GLFW_KEY_X && action == GLFW_PRESS) {
        for (int i = 0; i < meshArray.size(); i++) {
            meshArray[i]->isWireframe = !meshArray[i]->isWireframe;
        }
    }
    
    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_PRESS) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
    if (key == GLFW_KEY_LEFT_ALT && action == GLFW_RELEASE) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    /* 
    *    updates translation of all objects in the scene based on changes made to scene 
    *    description files, resets everything if no changes were made
    */
    if (key == GLFW_KEY_U && action == GLFW_PRESS) {
        ReadSceneDescription(meshArray);
        camera->position = initCamera;
    }
    // Enable mouse look
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        enableMouse = !enableMouse;
    }

    switch (theEditMode)
    {
        case MOVING_CAMERA:
        {
            const float CAMERA_MOVE_SPEED = 10.f;
            if (key == GLFW_KEY_A)     // Left
            {
                camera->StrafeLeft();
            }
            if (key == GLFW_KEY_D)     // Right
            {
                camera->StrafeRight();
            }
            if (key == GLFW_KEY_W)     // Forward
            {
                camera->MoveForward();
            }
            if (key == GLFW_KEY_S)     // Backwards
            {
                camera->MoveBackward();
            }
            if (key == GLFW_KEY_Q)     // Up
            {
                camera->MoveUp();
            }
            if (key == GLFW_KEY_E)     // Down
            {
                camera->MoveDown();
            }
        }
        break;
        case MOVING_SELECTED_OBJECT:
        {
            const float OBJECT_MOVE_SPEED = 1.f;
            if (key == GLFW_KEY_A)     // Left
            {
                meshArray[object_index]->position.x -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_D)     // Right
            {
                meshArray[object_index]->position.x += OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_W)     // Forward
            {
                meshArray[object_index]->position.z += OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_S)     // Backwards
            {
                meshArray[object_index]->position.z -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_Q)     // Down
            {
                meshArray[object_index]->position.y -= OBJECT_MOVE_SPEED;
            }
            if (key == GLFW_KEY_E)     // Up
            {
                meshArray[object_index]->position.y += OBJECT_MOVE_SPEED;
            }

            // cycle through objects in the scene
            if (key == GLFW_KEY_1 && action == GLFW_PRESS)
            {
                object_index++;
                if (object_index > meshArray.size() - 1) {
                    object_index = 0;
                }
                if (!enableMouse) camera->target = meshArray[object_index]->position;
            }
            if (key == GLFW_KEY_2 && action == GLFW_PRESS)
            {
                object_index--;
                if (object_index < 0) {
                    object_index = meshArray.size() - 1;
                }
                if (!enableMouse) camera->target = meshArray[object_index]->position;
            }    
        }
        break;
        case MOVING_LIGHT:
        {
            if (key == GLFW_KEY_1 && action == GLFW_PRESS)
            {
                constLightAtten *= glm::vec4(1, 1.e+1, 1, 1);
            }
            if (key == GLFW_KEY_2 && action == GLFW_PRESS)
            {
                constLightAtten *= glm::vec4(1, 1.e-1, 1, 1);
            }
            if (key == GLFW_KEY_3 && action == GLFW_PRESS)
            {
                constLightAtten *= glm::vec4(1, 1, 1.e+1, 1);
            }
            if (key == GLFW_KEY_4 && action == GLFW_PRESS)
            {
                constLightAtten *= glm::vec4(1, 1, 1.e-1, 1);
            }

            if (key == GLFW_KEY_W && action == GLFW_PRESS)
            {
                ambientLight *= 1.e+1;
                LightMan->SetAmbientLightAmount(ambientLight);
            }
            if (key == GLFW_KEY_S && action == GLFW_PRESS)
            {
                ambientLight *= 1.e-1;
                LightMan->SetAmbientLightAmount(ambientLight);
            }
        }
        case TAKE_CONTROL:
        {
            constexpr float PLAYER_MOVE_SPEED = 5.f;
            glm::vec3 strafeDirection = glm::cross(camera->target, upVector);
            player_mesh->velocity.y = 0.f;

            if (action == GLFW_PRESS) {
                if (key == GLFW_KEY_W) {
                    player_mesh->velocity.z = PLAYER_MOVE_SPEED;
                    player_mesh->rotation = glm::quat(glm::vec3(0.0f, glm::radians(180.0f), 0.0f));
                }
                if (key == GLFW_KEY_S) {
                    player_mesh->velocity.z = -PLAYER_MOVE_SPEED;
                    player_mesh->rotation = glm::quat(glm::vec3(0.0f, glm::radians(0.0f), 0.0f));
                }
                if (key == GLFW_KEY_A) {
                    player_mesh->velocity.x = PLAYER_MOVE_SPEED;
                    player_mesh->rotation = glm::quat(glm::vec3(0.0f, glm::radians(-90.0f), 0.0f));
                }
                if (key == GLFW_KEY_D) {
                    player_mesh->velocity.x = -PLAYER_MOVE_SPEED;
                    player_mesh->rotation = glm::quat(glm::vec3(0.0f, glm::radians(90.0f), 0.0f));
                }
                if (key == GLFW_KEY_Q) {
                    player_mesh->velocity.y = PLAYER_MOVE_SPEED;
                }
                if (key == GLFW_KEY_E) {
                    player_mesh->velocity.y = -PLAYER_MOVE_SPEED;
                }
            }
            else if (action == GLFW_RELEASE) {
                player_mesh->KillAllForces();
            }
        }
        break;
    }
}

static void MouseCallBack(GLFWwindow* window, double xposition, double yposition) {

    if (firstMouse) {
        lastX = xposition;
        lastY = yposition;
        firstMouse = false;
    }

    float xoffset = xposition - lastX;
    float yoffset = lastY - yposition;  // reversed since y coordinates go from bottom to up
    lastX = xposition;
    lastY = yposition;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // prevent perspective from getting flipped by capping it
    if (pitch > 89.f) {
        pitch = 89.f;
    }
    if (pitch < -89.f) {
        pitch = -89.f;
    }

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    if (enableMouse) {
        camera->target = glm::normalize(front);
    }
}

static void ScrollCallBack(GLFWwindow* window, double xoffset, double yoffset) {
    if (fov >= 1.f && fov <= 45.f) {
        fov -= yoffset;
    }
    if (fov <= 1.f) {
        fov = 1.f;
    }
    if (fov >= 45.f) {
        fov = 45.f;
    }
}

void Initialize() {

    std::cout.flush();

    if (!glfwInit()) {
        std::cerr << "GLFW init failed." << std::endl;
        glfwTerminate();
        return;
    }

    const char* glsl_version = "#version 420";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    // glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    if (!fullScreen) {
        // windowed mode
        window = glfwCreateWindow(1366, 768, "Man", NULL, NULL);
    }
    else {     
        GLFWmonitor* currentMonitor = glfwGetPrimaryMonitor();

        const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);

        glfwWindowHint(GLFW_RED_BITS, mode->redBits);
        glfwWindowHint(GLFW_GREEN_BITS, mode->greenBits);
        glfwWindowHint(GLFW_BLUE_BITS, mode->blueBits);
        glfwWindowHint(GLFW_REFRESH_RATE, mode->refreshRate);

        // fullscreen support based on current monitor
        window = glfwCreateWindow(mode->width, mode->height, "Man", currentMonitor, NULL);
    }
    
    if (!window) {
        std::cerr << "Window creation failed." << std::endl;
        glfwTerminate();
        return;
    }

    glfwSetWindowAspectRatio(window, 16, 9);

    // keyboard callback
    glfwSetKeyCallback(window, KeyCallback);

    // mouse and scroll callback
    glfwSetCursorPosCallback(window, MouseCallBack);
    glfwSetScrollCallback(window, ScrollCallBack);

    // capture mouse input
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glfwSetErrorCallback(ErrorCallback);

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)(glfwGetProcAddress))) {
        std::cerr << "Error: unable to obtain pocess address." << std::endl;
        return;
    }
    glfwSwapInterval(1); //vsync

    camera = new sCamera();

    // Init camera object
    // camera->position = glm::vec3(-530, 2500.0, -675.0);
    camera->position = glm::vec3(-280.0, 140.0, -700.0);
    camera->target = glm::vec3(0.f, 0.f, 1.f);

    // Init imgui for crosshair
    // crosshair.Initialize(window, glsl_version);
}

void Render() {

    // FBO
    std::cout << "\nCreating FBO..." << std::endl;
    FrameBuffer = new cFBO();

    useFBO = false;

    if (useFBO) {
        int screenWidth = 0;
        int screenHeight = 0;

        glfwGetFramebufferSize(window, &screenWidth, &screenHeight);

        std::string fboErrorString = "";
        if (!FrameBuffer->init(screenWidth, screenHeight, fboErrorString)) {
            std::cout << "FBO creation error: " << fboErrorString << std::endl;
        }
        else {
            std::cout << "FBO created." << std::endl;
        }
    }

    // Shader Manager
    std::cout << "\nCompiling shaders..." << std::endl;
    cShaderManager* shadyMan = new cShaderManager();

    cShader vertexShader;
    cShader geometryShader;
    cShader fragmentShader;

    vertexShader.fileName = "../assets/shaders/vertexShader.glsl";
    geometryShader.fileName = "../assets/shaders/geometryShader_passthrough.glsl";
    fragmentShader.fileName = "../assets/shaders/fragmentShader.glsl";

    if (!shadyMan->createProgramFromFile("ShadyProgram", vertexShader, geometryShader, fragmentShader)) {
        std::cout << "Error: Shader program failed to compile." << std::endl;
        std::cout << shadyMan->getLastError();
        return;
    }
    else {
        std::cout << "Shaders compiled." << std::endl;
    }

    initCamera = camera->position;

    shaderID = shadyMan->getIDFromFriendlyName("ShadyProgram");
    glUseProgram(shaderID);

    // Load asset paths from external file
    ReadFromFile("./File Stream/readFile.txt");

    // Load animations
    LoadAnimations();

    // Load the ply model
    modelLoader = new cModelFileLoader();

    // VAO Manager
    VAOMan = new cVAOManager();

    // Scene
    std::cout << "\nLoading assets..." << std::endl;

    // Load all ply files
    LoadPlyFilesIntoVAO();

    // Lights
    LightMan = new cLightManager();

    // Pre-existing light, independent of the scene lighting
    ambientLight = 0.5f;
    LightMan->SetAmbientLightAmount(ambientLight);

    constLightAtten = glm::vec4(0.1f, 2.5e-5f, 2.5e-6f, 1.0f);

    // The actual light
    pointLight = LightMan->AddLight(glm::vec4(0.f, 0.f, 0.f, 1.f));
    pointLight->diffuse = glm::vec4(1.f, 1.f, 1.f, 1.f);
    pointLight->specular = glm::vec4(1.f, 1.f, 1.f, 1.f);
    pointLight->direction = glm::vec4(1.f, 1.f, 1.f, 1.f);
    pointLight->atten = constLightAtten;
    pointLight->param1 = glm::vec4(0.f, 50.f, 50.f, 1.f);
    pointLight->param2 = glm::vec4(1.f, 0.f, 0.f, 1.f);

    // The model to be drawn where the light exists
    bulb_mesh = new cMeshInfo();
    bulb_mesh->meshName = "bulb";
    bulb_mesh->friendlyName = "bulb";
    meshArray.push_back(bulb_mesh);

    glm::vec4 terrainColor = glm::vec4(0.45f, 0.45f, 0.45f, 1.f);

    // Scene
    // Mesh Models
    cMeshInfo* terrain_mesh = new cMeshInfo();
    terrain_mesh->meshName = "terrain";
    terrain_mesh->friendlyName = "terrain";
    terrain_mesh->RGBAColour = terrainColor;
    terrain_mesh->useRGBAColour = true;
    terrain_mesh->isVisible = false;
    terrain_mesh->doNotLight = false;
    meshArray.push_back(terrain_mesh);

    cMeshInfo* flat_plain = new cMeshInfo();
    flat_plain->meshName = "flat_plain";
    flat_plain->friendlyName = "flat_plain";
    flat_plain->RGBAColour = terrainColor;
    flat_plain->useRGBAColour = true;
    flat_plain->hasTexture = false;
    flat_plain->textures[0] = "";
    flat_plain->textureRatios[0] = 1.f;
    flat_plain->doNotLight = false;
    flat_plain->isVisible = true;
    meshArray.push_back(flat_plain);

    player_mesh = new cMeshInfo();
    player_mesh->meshName = "steve";
    player_mesh->friendlyName = "player";
    player_mesh->doNotLight = true;
    player_mesh->isVisible = true;
    player_mesh->useRGBAColour = true;
    player_mesh->RGBAColour = glm::vec4(50, 30, 0, 1);
    player_mesh->hasTexture = false;
    player_mesh->textures[0] = "man.bmp";
    player_mesh->textureRatios[0] = 1.f;

    player_mesh->animation.AnimationTime = 5.f;
    player_mesh->animation.IsLooping = true;
    player_mesh->animation.IsPlaying = true;
    player_mesh->isAnimated = true;
    player_mesh->animation.AnimationType = "Animation0";
    player_mesh->animation.Speed = collectiveSpeed;

    player_mesh->HasBones = true;
    player_mesh->BoneModelMatrices[0] = glm::mat4(1.f);	// identity matrix

    meshArray.push_back(player_mesh);

    // Utility
    skybox_sphere_mesh = new cMeshInfo();
    skybox_sphere_mesh->meshName = "skybox_sphere";
    skybox_sphere_mesh->friendlyName = "skybox_sphere";
    skybox_sphere_mesh->isSkyBoxMesh = true;

    full_screen_quad = new cMeshInfo();
    full_screen_quad->meshName = "fullScreenQuad";
    full_screen_quad->friendlyName = "fullScreenQuad";
    full_screen_quad->doNotLight = false;

    // all textures loaded here
    LoadTextures();

    // reads scene descripion files for positioning and other info
    ReadSceneDescription(meshArray);
}

void Update(float dt) {

    // Cull back facing triangles
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);

    // Depth test
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // MVP
    glm::mat4x4 model;

    GLuint modelLocaction = glGetUniformLocation(shaderID, "Model");
    GLuint viewLocation = glGetUniformLocation(shaderID, "View");
    GLuint projectionLocation = glGetUniformLocation(shaderID, "Projection");
    GLuint modelInverseLocation = glGetUniformLocation(shaderID, "ModelInverse");

    GLuint BoneMatricesLocation[4];
    BoneMatricesLocation[0] = glGetUniformLocation(shaderID, "BoneMatrices[0]");
    BoneMatricesLocation[1] = glGetUniformLocation(shaderID, "BoneMatrices[1]");
    BoneMatricesLocation[2] = glGetUniformLocation(shaderID, "BoneMatrices[2]");
    BoneMatricesLocation[3] = glGetUniformLocation(shaderID, "BoneMatrices[3]");
    
    GLuint BoneRotationMatricesLocation[4];
    BoneRotationMatricesLocation[0] = glGetUniformLocation(shaderID, "BoneRotationMatrices[0]");
    BoneRotationMatricesLocation[1] = glGetUniformLocation(shaderID, "BoneRotationMatrices[1]");
    BoneRotationMatricesLocation[2] = glGetUniformLocation(shaderID, "BoneRotationMatrices[2]");
    BoneRotationMatricesLocation[3] = glGetUniformLocation(shaderID, "BoneRotationMatrices[3]");
    
    // Lighting
    // ManageLights();
    LightMan->LoadLightUniformLocations(shaderID);
    LightMan->CopyLightInformationToShader(shaderID);
    
    pointLight->position = glm::vec4(bulb_mesh->position, 1.f);
    pointLight->atten = constLightAtten;

    float ratio;
    int width, height;

    // Bind the Frame Buffer
    if (useFBO) {
        glBindFramebuffer(GL_FRAMEBUFFER, FrameBuffer->ID);
        FrameBuffer->clearBuffers(true, true);
    }

    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;
    glViewport(0, 0, width, height);

    // mouse support
    if (enableMouse) {
        camera->view = glm::lookAt(camera->position, camera->position + camera->target, upVector);
        camera->projection = glm::perspective(glm::radians(fov), ratio, 0.1f, 10000.f);
    }
    else {
        camera->view = glm::lookAt(camera->position, camera->target, upVector);
        camera->projection = glm::perspective(0.6f, ratio, 0.1f, 10000.f);
    }

    GLint eyeLocationLocation = glGetUniformLocation(shaderID, "eyeLocation");
    glUniform4f(eyeLocationLocation, camera->position.x, camera->position.y, camera->position.z, 1.f);

    model = glm::mat4(1.f);

    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(camera->view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(camera->projection));

    g_prevTime = g_currentTime;
    g_currentTime = glfwGetTime();
    g_elapsedTime = g_currentTime - g_prevTime;

    if (theEditMode == TAKE_CONTROL) {
        camera->position = player_mesh->position - glm::vec3(0.f, -10.f, 100.f);
        // bulb_mesh->position = player_mesh->position - glm::vec3(0.f, -100.f, 75.f);

        // Set the camera to always follow the agent
        if (!enableMouse) {
            camera->target = player_mesh->position;
        }
    }

    player_mesh->BoneModelMatrices[0] = glm::translate(glm::mat4(1.0f), m_Bone1Position);
    player_mesh->BoneModelMatrices[1] = glm::translate(player_mesh->BoneModelMatrices[0], m_Bone2Position);
    player_mesh->BoneModelMatrices[2] = glm::translate(player_mesh->BoneModelMatrices[1], glm::vec3(0.f));
    player_mesh->BoneModelMatrices[3] = glm::translate(player_mesh->BoneModelMatrices[2], glm::vec3(0.f));

    // Draw scene meshes
    for (int i = 0; i < meshArray.size(); i++) {

        cMeshInfo* currentMesh = meshArray[i];

        if (currentMesh->isAnimated)
        {
            currentMesh->animation.Speed = collectiveSpeed;

            // picks up the easing based on the position
            if (currentMesh->animation.AnimationType == "Animation0") {
                switch (currentMesh->currentEasing)
                {
                case EaseIn:
                    currentMesh->RGBAColour = glm::vec4(100.f, 0.f, 0.f, 1.f);
                    break;
                case EaseOut:
                    currentMesh->RGBAColour = glm::vec4(100.f, 100.f, 0.f, 1.f);
                    break;
                case EaseInOut:
                    currentMesh->RGBAColour = glm::vec4(0.f, 100.f, 0.f, 1.f);
                    break;
                default:
                    currentMesh->RGBAColour = glm::vec4(100.f, 100.f, 100.f, 1.f);
                    break;
                }
            }
        }

        // Draw all the meshes pushed onto the vector
        DrawMesh(currentMesh,                   // theMesh
                 model,                         // Model Matrix
                 shaderID,                      // Compiled Shader ID
                 TextureMan,                    // Instance of the Texture Manager
                 VAOMan,                        // Instance of the VAO Manager
                 camera,                        // Instance of the struct Camera
                 modelLocaction,                // UL for model matrix
                 modelInverseLocation,          // UL for transpose of model matrix
                 BoneMatricesLocation,          // UL for bone matrices
                 BoneRotationMatricesLocation); // UL for bone rotation matrices
    }

    // Draw the skybox
    DrawMesh(skybox_sphere_mesh, matIdentity, shaderID, 
        TextureMan, VAOMan, camera, modelLocaction, modelInverseLocation,
        BoneMatricesLocation, BoneRotationMatricesLocation);

    // Redirect output to an offscreen quad
    if (useFBO)
    {
        RenderToFBO(window, camera, eyeLocationLocation, 
            viewLocation, projectionLocation,
            modelLocaction, modelInverseLocation, BoneMatricesLocation, BoneRotationMatricesLocation);
    }

    glfwSwapBuffers(window);
    glfwPollEvents();

    currentTime = glfwGetTime();
    timeDiff = currentTime - beginTime;
    frameCount++;

    //const GLubyte* vendor = glad_glGetString(GL_VENDOR); // Returns the vendor
    const GLubyte* renderer = glad_glGetString(GL_RENDERER); // Returns a hint to the model

    // Set window title
    if (timeDiff >= 1.f / 30.f) {
        std::string frameRate = std::to_string((1.f / timeDiff) * frameCount);
        std::string frameTime = std::to_string((timeDiff / frameCount) * 1000);

        std::stringstream ss;
        if (theEditMode == MOVING_SELECTED_OBJECT) {
            ss << " Camera: " << "(" << camera->position.x << ", " << camera->position.y << ", " << camera->position.z << ")"
               << "    GPU: " << renderer
               << "    FPS: " << frameRate << " ms: " << frameTime
               << "    Target: Index = " << object_index << ", MeshName: " << meshArray[object_index]->friendlyName << ", Position: (" << meshArray[object_index]->position.x << ", " << meshArray[object_index]->position.y << ", " << meshArray[object_index]->position.z << ")"
               ;
        }
        else if (theEditMode == MOVING_LIGHT) {
            ss << " Camera: " << "(" << camera->position.x << ", " << camera->position.y << ", " << camera->position.z << ")"
                << "    GPU: " << renderer
                << "    FPS: " << frameRate << " ms: " << frameTime
                << "    Light Atten: " << "(" << constLightAtten.x << ", " << constLightAtten.y << ", " << constLightAtten.z << ")"
                << "    Ambient Light: " << ambientLight
                ;
        }
        else {
            ss << " Camera: " << "(" << camera->position.x << ", " << camera->position.y << ", " << camera->position.z << ")"
               << "    GPU: " << renderer
               << "    FPS: " << frameRate << " ms: " << frameTime
               ;
        }   

        // Set window title
        glfwSetWindowTitle(window, ss.str().c_str());

        beginTime = currentTime;
        frameCount = 0;
    }

    AnimeMan.Update(meshArray, 0.1f);
}

void Shutdown() {

    glfwDestroyWindow(window);
    glfwTerminate();

    window = nullptr;
    delete window;

    exit(EXIT_SUCCESS);
}

void ReadFromFile(std::string filePath) {

    std::ifstream readFile;

    readFile.open(filePath);

    if (!readFile.is_open()) {
        readFile.close();
        return;
    }

    std::string input0;

    while (readFile >> input0) {
        meshFiles.push_back(input0);
        readIndex++;
    }

    readFile.close();
}

// All lights managed here
void ManageLights() {
    
    GLint PositionLocation = glGetUniformLocation(shaderID, "sLightsArray[0].position");
    GLint DiffuseLocation = glGetUniformLocation(shaderID, "sLightsArray[0].diffuse");
    GLint SpecularLocation = glGetUniformLocation(shaderID, "sLightsArray[0].specular");
    GLint AttenLocation = glGetUniformLocation(shaderID, "sLightsArray[0].atten");
    GLint DirectionLocation = glGetUniformLocation(shaderID, "sLightsArray[0].direction");
    GLint Param1Location = glGetUniformLocation(shaderID, "sLightsArray[0].param1");
    GLint Param2Location = glGetUniformLocation(shaderID, "sLightsArray[0].param2");

    //glm::vec3 lightPosition0 = meshArray[1]->position;
    glm::vec3 lightPosition0 = meshArray[0]->position;
    glUniform4f(PositionLocation, lightPosition0.x, lightPosition0.y, lightPosition0.z, 1.0f);
    //glUniform4f(PositionLocation, 0.f, 0.f, 0.f, 1.0f);
    glUniform4f(DiffuseLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(SpecularLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(AttenLocation, 0.1f, 0.5f, 0.1f, 1.f);
    glUniform4f(DirectionLocation, 1.f, 1.f, 1.f, 1.f);
    glUniform4f(Param1Location, 0.f, 0.f, 0.f, 1.f); //x = Light Type
    glUniform4f(Param2Location, 1.f, 0.f, 0.f, 1.f); //x = Light on/off
}

void LoadTextures() {

    std::cout << "\nLoading textures...";

    std::string errorString = "";
    TextureMan = new cBasicTextureManager();

    TextureMan->SetBasePath("../assets/textures");

    // skybox/cubemap textures
    std::string skyboxName = "NightSky";
    if (TextureMan->CreateCubeTextureFromBMPFiles(skyboxName,
        "SpaceBox_right1_posX.bmp",
        "SpaceBox_left2_negX.bmp",
        "SpaceBox_top3_posY.bmp",
        "SpaceBox_bottom4_negY.bmp",
        "SpaceBox_front5_posZ.bmp",
        "SpaceBox_back6_negZ.bmp",
        true, errorString))
    {
        std::cout << "\nLoaded skybox textures: " << skyboxName << std::endl;
    }
    else
    {
        std::cout << "\nError: failed to load skybox because " << errorString;
    }
    
    if (TextureMan->Create2DTextureFromBMPFile("man.bmp"))
    {
        std::cout << "Loaded man texture." << std::endl;
    }
    else
    {
        std::cout << "Error: failed to load man texture.";
    }
    
    if (TextureMan->Create2DTextureFromBMPFile("crosshair.bmp"))
    {
        std::cout << "Loaded crosshair texture." << std::endl;
    }
    else
    {
        std::cout << "Error: failed to load crosshair texture.";
    }
}

float RandomFloat(float a, float b) {

    float random = ((float)rand()) / (float)RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void RenderToFBO(GLFWwindow* window, sCamera* camera,
                 GLuint eyeLocationLocation, GLuint viewLocation, GLuint projectionLocation,
                 GLuint modelLocaction, GLuint modelInverseLocation, 
                 GLuint BoneMatricesLocation[4], GLuint BoneRotationMatricesLocation[4])
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glm::vec3 exCameraEye = camera->position;
    glm::vec3 exCameraLookAt = camera->target;

    camera->position = glm::vec3(0.0f, 0.0f, -6.0f);
    camera->target = glm::vec3(0.0f, 0.0f, 0.0f);

    float ratio;
    int width, height;

    glfwGetFramebufferSize(window, &width, &height);
    ratio = width / (float)height;

    GLint FBOsizeLocation = glGetUniformLocation(shaderID, "FBO_size");
    GLint screenSizeLocation = glGetUniformLocation(shaderID, "screen_size");

    glUniform2f(FBOsizeLocation, (GLfloat)FrameBuffer->width, (GLfloat)FrameBuffer->height);
    glUniform2f(screenSizeLocation, (GLfloat)width, (GLfloat)height);

    camera->projection = glm::perspective(0.6f, ratio, 0.1f, 100.f);

    glViewport(0, 0, width, height);

    camera->view = glm::lookAt(camera->position, camera->target, upVector);

    // Set eyelocation again
    glUniform4f(eyeLocationLocation, camera->position.x, camera->position.y, camera->position.z, 1.f);
    glUniformMatrix4fv(viewLocation, 1, GL_FALSE, glm::value_ptr(camera->view));
    glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, glm::value_ptr(camera->projection));

    // Drawing
    GLint bIsFullScreenQuadLocation = glGetUniformLocation(shaderID, "bIsFullScreenQuad");
    glUniform1f(bIsFullScreenQuadLocation, (GLfloat)GL_TRUE);

    // Texture on the quad
    GLuint texture21Unit = 21;			// Picked 21 because it's not being used
    glActiveTexture(texture21Unit + GL_TEXTURE0);	// GL_TEXTURE0 = 33984
    glBindTexture(GL_TEXTURE_2D, FrameBuffer->colourTexture_0_ID);

    GLint FBO_TextureLocation = glGetUniformLocation(shaderID, "FBO_Texture");
    glUniform1i(FBO_TextureLocation, texture21Unit);

    // Crosshair Texture
    GLuint crosshairTextureID = TextureMan->getTextureIDFromName("crosshair.bmp");

    GLuint texture15Unit = 15;
    glActiveTexture(texture15Unit + GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, crosshairTextureID);

    GLint crosshairTextureLocation = glGetUniformLocation(shaderID, "crosshair_texture");
    glUniform1i(crosshairTextureLocation, texture15Unit);

    full_screen_quad->SetUniformScale(10.f);
    full_screen_quad->isVisible = true;

    // Draw the quad
    DrawMesh(full_screen_quad,
        matIdentity,
        shaderID,
        TextureMan,
        VAOMan,
        camera,
        modelLocaction,
        modelInverseLocation,
        BoneMatricesLocation,
        BoneRotationMatricesLocation);

    camera->position = exCameraEye;
    camera->target = exCameraLookAt;

    glUniform1f(bIsFullScreenQuadLocation, (GLfloat)GL_FALSE);
}

void LoadPlyFilesIntoVAO(void)
{
    sModelDrawInfo bulb;
    modelLoader->LoadModel(meshFiles[0], bulb);
    if (!VAOMan->LoadModelIntoVAO("bulb", bulb, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }

    sModelDrawInfo flat_plain_obj;
    modelLoader->LoadModel(meshFiles[9], flat_plain_obj);
    if (!VAOMan->LoadModelIntoVAO("flat_plain", flat_plain_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }

    sModelDrawInfo wall_cube;
    modelLoader->LoadModel(meshFiles[2], wall_cube);
    if (!VAOMan->LoadModelIntoVAO("wall_cube", wall_cube, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    
    sModelDrawInfo pyramid;
    modelLoader->LoadModel(meshFiles[11], pyramid);
    if (!VAOMan->LoadModelIntoVAO("pyramid", pyramid, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }

    sModelDrawInfo steve;
    modelLoader->LoadModel(meshFiles[8], steve, true);
    if (!VAOMan->LoadModelIntoVAO("steve", steve, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
    
    // 2-sided full screen quad aligned to x-y axis
    sModelDrawInfo fullScreenQuad;
    modelLoader->LoadModel(meshFiles[10], fullScreenQuad);
    if (!VAOMan->LoadModelIntoVAO("fullScreenQuad", fullScreenQuad, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }

    // skybox sphere with inverted normals
    sModelDrawInfo skybox_sphere_obj;
    modelLoader->LoadModel(meshFiles[6], skybox_sphere_obj);
    if (!VAOMan->LoadModelIntoVAO("skybox_sphere", skybox_sphere_obj, shaderID)) {
        std::cerr << "Could not load model into VAO" << std::endl;
    }
}

glm::mat4 CreateModelMatrix(const glm::mat4& parentModelMatrix, const glm::vec3& translate, const glm::quat& rotate)
{
    glm::mat4 TranslationMatrix = glm::translate(glm::mat4(1.f), translate);
    glm::mat4 RotationMatrix = glm::mat4_cast(rotate);
    return parentModelMatrix * TranslationMatrix * RotationMatrix;
}

// All animations loaded here
void LoadAnimations() {
    // A more position & rotation focused animation
    AnimationData animation0;
    animation0.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(75.0f, 12.0f, 0.0f), 0.f, EaseIn));
    animation0.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(0.0f, 12.0f, -75.f), 5.f, EaseIn));
    animation0.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(-75.0f, 12.0f, 0.0f), 10.f, EaseInOut));
    animation0.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(0.0f, 12.0f, 75.0f), 15.f, EaseOut));
    animation0.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(75.0f, 12.0f, 0.0f), 20.f, EaseOut));
    animation0.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(1), 0.0f, None));
    animation0.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(1, 0, 0, 0), 0.0f, true, EaseIn));
    animation0.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0, -1, 0), 5.0f, true, EaseIn));
    animation0.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(-1, 0, 0, 0), 10.0f, true, EaseInOut));
    animation0.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0, 1, 0), 15.0f, true, EaseOut));
    animation0.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(1, 0, 0, 0), 20.0f, true, EaseOut));
    animation0.NullKeyFrames.push_back(NullKeyFrame("\nAnimation0: The zero_null frame was called.\n", 0.f));
    animation0.Duration = 20.f;
    AnimeMan.LoadAnimation("Animation0", animation0);

    // A more scale focused animation
    AnimationData animation1;
    animation1.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(20.0f, 5.0f, 20.0f), 0.f, EaseInOut));
    animation1.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(20.0f, 10.0f, -20.0f), 20.f, EaseInOut));
    animation1.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(20.0f, 5.0f, 20.0f), 40.f, EaseInOut));
    animation1.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(1, 0, 0, 0), 0.f, true, EaseIn));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(1), 0.0f, EaseIn));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(2), 5.0f, EaseIn));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(3), 10.0f, EaseIn));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(4), 15.0f, EaseInOut));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(5), 20.0f, EaseInOut));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(4), 25.0f, EaseInOut));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(3), 30.0f, EaseOut));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(2), 35.0f, EaseOut));
    animation1.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(1), 40.0f, EaseOut));
    animation1.NullKeyFrames.push_back(NullKeyFrame("\nAnimation1: The zero_null frame was called.\n", 0.f));
    animation1.Duration = 40.f;
    AnimeMan.LoadAnimation("Animation1", animation1);

    // A more rotation focused animation
    AnimationData animation2;
    animation2.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(-20.0f, 5.0f, 0.0f), 0.f, None));
    animation2.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(-30.0f, 5.0f, 0.0f), 20.f, None));
    animation2.PositionKeyFrames.push_back(PositionKeyFrame(glm::vec3(-20.0f, 5.0f, 0.0f), 40.f, None));
    animation2.ScaleKeyFrames.push_back(ScaleKeyFrame(glm::vec3(1), 0.0f, EaseIn));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0, 0, 0), 0.f, true, EaseIn));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.01f, 0, 0), 5.f, true, EaseIn));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.02f, 0, 0), 10.f, true, EaseIn));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.03f, 0, 0), 15.f, true, EaseInOut));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.04f, 0, 0), 20.f, true, EaseInOut));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.03f, 0, 0), 25.f, true, EaseInOut));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.02f, 0, 0), 30.f, true, EaseOut));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0.01f, 0, 0), 35.f, true, EaseOut));
    animation2.RotationKeyFrames.push_back(RotationKeyFrame(glm::quat(0, 0, 0, 0), 40.f, true, EaseOut));

    animation2.NullKeyFrames.push_back(NullKeyFrame("\nAnimation2: The zero_null frame was called.\n", 0.f));
    animation2.Duration = 40.f;
    AnimeMan.LoadAnimation("Animation2", animation2);

}
int main(int argc, char** argv) 
{
    Initialize();
    Render();
    
    while (!glfwWindowShouldClose(window)) {
        Update(g_elapsedTime);
    }
    
    Shutdown();
    
    return 0;
}