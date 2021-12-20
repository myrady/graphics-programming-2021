#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/gtc/matrix_access.hpp>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "skybox.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

//-----------------
#include "firstpersoncamera.h"
//-----------------//-----------------//-----------------//-----------------

struct SceneObject{
    unsigned int VAO;
    unsigned int vertexCount;
    bool indexedObject = true;
    void drawSceneObject(std::uint16_t glMode){
        glBindVertexArray(VAO);
        if(indexedObject)
            glDrawElements(glMode, vertexCount, GL_UNSIGNED_INT, nullptr);
        else
            glDrawArrays(glMode, 0, vertexCount);
    }
};
//-----------------//-----------------//-----------------//-----------------
//-----------------//-----------------//-----------------//-----------------

void setup();
void drawObjects();
void createParticles();
unsigned int createArrayBuffer(const std::vector<float> &array);
unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array);
unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices);
void processInput(GLFWwindow* window);
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);

// function declarations
// ---------------------
void drawTrees(Model* type, float x, float y, float z);
void drawWater();
void drawCrystals(float x, float y, float z);
//void drawFloor();
void drawSky();
unsigned int initSkyboxBuffers();
unsigned int loadCubemap(vector<std::string> faces);

//-----------------//-----------------//-----------------//-----------------
void renderScene(GLFWwindow* window);
void drawScene(Shader *shader, bool isShadowPass = false);
void drawSkybox();

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);

//-----------------//-----------------//-----------------//-----------------
// screen settings
// ---------------
const unsigned int SCR_WIDTH = 800, SCR_HEIGHT = 600;
const unsigned int SHADOW_WIDTH = 1280, SHADOW_HEIGHT = 720;
const unsigned int vertexBufferSize = 200, particleSize = 3, sizeOfFloat = 4;
//-----------------//-----------------//-----------------//-----------------

// global variables used for rendering
// -----------------------------------
Shader* sceneShader;
Shader* simpleDepthShader;
unsigned int depthMap, depthMapFBO;


SceneObject floorObj;
SceneObject particlesObj;
//---------rain-----------
Shader* rainShader;
Shader* solidShader;
//--------floor------------
Shader* floorShader;
Model* floorModel;
unsigned int floorTextureId;
Model* waterModel;
//---------sky------------
Shader* skyboxShader;
unsigned int skyboxVAO; // skybox handle
unsigned int cubemapTexture; // skybox texture handle
//---------stuffs---------
//Shader* waterShader;
Shader* bloomShader;
//Shader* treeShader;
Model* crystalModel;
Model* treeModel;
Model* firModel;
// build and compile shaders
// -------------------------
Shader* Bloom; //("shaders/bloom.vert", "shaders/shader.frag");
Shader* shaderLight; //("shaders/bloom.vert", "shaders/bloom.frag");
Shader* shaderBlur; //("shaders/blur.vert", "shaders/blur.frag");
Shader* shaderBloomFinal; //("shaders/bloom_final.vert", "shaders/bloom_final.frag");


// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;

float currentTime;
float deltaTime;

bool isSnow = false;
float boxSize = 30.f;


FirstPersonCamera fpCam(glm::radians(70.0f), SCR_WIDTH, SCR_HEIGHT, .1f, 110.0f, glm::vec3(0, 10.6f, 0), glm::vec3(0, 0, -1));
Camera camera(glm::vec3(0.0f, 10.6f, 0.0f));

//-----------------//-----------------//-----------------//-----------------

// parameters that can be set in our GUI
// -------------------------------------
struct Config {
    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 0.9f, 0.9f};
    float ambientLightIntensity = 0.925f;

    // light 1
    glm::vec3 lightPosition = {10.2f, 20.5f, 0.8f};
    glm::vec3 lightDirection = {2.7f, -1.0f, 0.7f};
    glm::vec3 lightColor = {1.0f, 0.5f, 1.0f};
    float lightIntensity = 0.75f;

    // light 2
    float attenuationConstant = 0.2f;
    float attenuationLinear = 0.5f;
    float attenuationQuadratic = 1.0f;
    float specularOffset = 0.5f;

    // material
    float specularExponent = 27.0f;
    float ambientOcclusionMix = 1.0f;
    float normalMappingMix = 1.0f;
    float reflectionMix = 0.15f;
    int orthoTangentSpace = 0;

    // shadow parameters
    bool softShadows = false;
    float shadowBias = 0.07f;
    float shadowMapSize = 5.0f;
    float shadowMapDepthRange = 20.0f;

    // attenuation (c0, c1 and c2 on the slides)
    float attenuationC0 = 0.25;
    float attenuationC1 = 0.1;
    float attenuationC2 = 0.1;

    float reflectionFactor = 0.5f;
    float n2 = 1.0f;

} config;





//---DEBUG
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cout << "ERROR";
}
//-----------------//-----------------//-----------------//-----------------




int main()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "PVBetti (vipi)", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, cursor_input_callback);
	glfwSetScrollCallback(window, scroll_callback);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    //-----DEBUG-------
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);
    // ---------------------------------------

// setup mesh objects
    // ---------------------------------------
    setup();



// ---------------------------------------
    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
    // init skybox
    vector<std::string> faces {
                    "skybox/right.tga",
                    "skybox/left.tga",
                    "skybox/top.tga",
                    "skybox/bottom.tga",
                    "skybox/front.tga",
                    "skybox/back.tga"
            };
    cubemapTexture = Skybox::loadCubemap(faces);
    skyboxVAO = Skybox::initSkyboxBuffers();
// ---------------------------------------



    // configure depth map FBO
    // -----------------------

    // create depth texture
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // if you replace GL_LINEAR with GL_NEAREST you will see pixelation in the borders of the shadow
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    // attach depth texture as FBO's depth buffer
    glGenFramebuffers(1, &depthMapFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

// ---------------------------------------
    // set up the z-buffer
    glDepthRange(0,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    // IMGUI init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    glEnable(GL_BLEND); //-old
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_CULL_FACE); //-new
// ---------------------------------------

    // configure (floating point) framebuffers
    // ---------------------------------------
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO);
    // create 2 floating point color buffers (1 for normal rendering, other for brightness threshold values)
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        // attach texture to framebuffer
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, colorBuffers[i], 0);
    }
    // create and attach depth buffer (renderbuffer)
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);
    // tell OpenGL which color attachments we'll use (of this framebuffer) for rendering
    unsigned int attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);
    // finally check if framebuffer is complete
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // ping-pong-framebuffer for blurring
    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // we clamp to the edge as the blur filter would otherwise sample repeated texture values!
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);
        // also check if framebuffers are complete (no need for depth buffer)
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            std::cout << "Framebuffer not complete!" << std::endl;
    }

// ---------------------------------------



    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

    // render loop
    while (!glfwWindowShouldClose(window))
    {
        // update current time
        auto frameStart = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> appTime = frameStart - begin;
        currentTime = appTime.count();


        static float lastFrame = 0.0f;
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // clear buffers
        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        renderScene(window);


// ---------DRAW---------
        //glEnable(GL_BLEND);
        drawTrees(firModel, -50.0, 5.2, -45.296);
        drawTrees(firModel, -50.0, 5.2, -55.296);
        drawTrees(firModel, -60.0, 5.2, -75.296);
        drawTrees(firModel, -30.0, 7.5, -55.296);
        drawTrees(firModel, -10.0, 7.5, -65.296);
        drawTrees(firModel, 20.0, 6.2, -65.296);

        drawTrees(treeModel, -40.0, 6.2, -45.296);
        drawTrees(treeModel,55.0, 7.5, -20.296);
        drawTrees(treeModel, -40.0, 0.2, 40.296);
        drawTrees(treeModel, -60.0, 6.2, 45.296);
        drawTrees(treeModel, -60.0, 5.5, 25.296);
        drawTrees(treeModel, -55.0, 3.5, -20.296);
        drawTrees(treeModel,65.0, 14.5, -60.296);
        drawTrees(treeModel, -60.0, 9.2, 70.296);
        drawTrees(treeModel, 60.0, 8.2, 45.296);
        drawTrees(treeModel, 60.0, 3.0, 25.296);


        drawCrystals(-10.7432, 7.0, -55.296);
        drawCrystals(-20.7432, 5.0, -50.296);
        drawCrystals(-30.0, 1.5, -35.296);

        drawCrystals(-10.7432, 6.0, 25.296);
        drawCrystals(-60.7432, 5.0, 15.296);
        drawCrystals(40.0, 4.0, 2.296);

        drawObjects();
        drawWater( );

        //glDisable(GL_BLEND);
// ---------

        // show the frame buffer
        glfwSwapBuffers(window);
        glfwPollEvents();

        // control render loop frequency
        std::chrono::duration<float> elapsed = std::chrono::high_resolution_clock::now()-frameStart;
        while (loopInterval > elapsed.count()) {
            elapsed = std::chrono::high_resolution_clock::now() - frameStart;
        }
    }

    //--------Cleanup---------------
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	//delete carModel;
	delete floorModel;
    delete sceneShader;
    delete skyboxShader;
    delete simpleDepthShader;
    delete firModel;
    delete treeModel;
    delete crystalModel;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

//-----------------//-----------------//-----------------//-----------------
//-----------------//-----------------//-----------------//-----------------
//-----------------//-----------------//-----------------//-----------------

void setup(){

    sceneShader = new Shader("shaders/shader.vert", "shaders/shader.frag");
    simpleDepthShader = new Shader("shaders/shadowmapping_depth.vert", "shaders/shadowmapping_depth.frag");

    // initialize shaders
    solidShader = new Shader("shaders/water.vert", "shaders/water.frag");
    rainShader = new Shader("shaders/rain.vert", "shaders/rain.frag");

    //-----NEW STUFFS----
    floorShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    floorModel = new Model("exam/flo.obj");     //flooor
    treeModel = new Model("exam/stuffs/tree1.obj");
    firModel = new Model("exam/stuffs/tree2.obj");
    crystalModel = new Model("exam/stuffs/cry.obj");
    waterModel = new Model("exam/water.obj");
    //-----SHADERS-----
    //bloomShader = new Shader("shaders/bloom.vert", "shaders/bloom.frag");
    //treeShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    //waterShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    Bloom = new Shader("shaders/bloom.vert", "shaders/shader.frag");
    shaderLight = new Shader("shaders/bloom.vert", "shaders/bloom.frag");
    shaderBlur = new Shader("shaders/blur.vert", "shaders/blur.frag");
    shaderBloomFinal = new Shader("shaders/bloom_final.vert", "shaders/bloom_final.frag");
    //--------------------

something
    createParticles();
}
//----

void drawTrees(Model* type, float x, float y, float z){
    Shader* shader = new Shader("shaders/tree_shader.vert", "shaders/tree_shader.frag");
    shader->use();

    shader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    shader->setVec3("lightPosition", config.lightPosition);
    shader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    shader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    shader->setFloat("attenuationC0", config.attenuationC0);
    shader->setFloat("attenuationC1", config.attenuationC1);
    shader->setFloat("attenuationC2", config.attenuationC2);

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;


    // set projection matrix uniform
    shader->setMat4("projection", projection);
    shader->setVec3("viewPosition", camera.Position);
    shader->setMat4("view", view);

    glActiveTexture(GL_TEXTURE0);
    shader->setInt("texture_diffuse1", 0);
    glBindTexture(GL_TEXTURE_2D, floorTextureId);
    // notice that we overwrite the value of one of the uniform variables to set a different floor color
    shader->setVec3("reflectionColor", .2, .5, .2);
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(1.f, 1.f, 1.f));
    model = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
    shader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    shader->setMat4("invTranspMV", invTranspose);
    shader->setMat4("view", view);

    type->Draw(*shader);
}


//-TODO----SHADER-----
void drawCrystals(float x, float y, float z){
    Shader* shader = new Shader("shaders/tree_shader.vert", "shaders/tree_shader.frag");
    shader->use();


    shader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    shader->setVec3("lightPosition", config.lightPosition);
    shader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    shader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    shader->setFloat("attenuationC0", config.attenuationC0);
    shader->setFloat("attenuationC1", config.attenuationC1);
    shader->setFloat("attenuationC2", config.attenuationC2);


    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);
    shader->setVec3("viewPosition", camera.Position);
    shader->setMat4("view", view);


    glActiveTexture(GL_TEXTURE0);
    shader->setInt("texture_diffuse1", 0);//0
    glBindTexture(GL_TEXTURE_2D, floorTextureId);
    // notice that we overwrite the value of one of the uniform variables to set a different floor color
    shader->setVec3("reflectionColor", .2, .5, .2);
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(1.5f, 1.5f, 1.5f));
    model = glm::translate(glm::mat4(1.0f), glm::vec3(x,y,z));
    shader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    shader->setMat4("invTranspMV", invTranspose);
    shader->setMat4("view", view);



    crystalModel->Draw(*shader);
}



//---------------------------------------------

void drawWater(){
    solidShader->use();

///----
    solidShader->setFloat("reflectionFactor", config.reflectionFactor);
    solidShader->setFloat("n2", config.n2);

    solidShader->setInt("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
///----
// light uniforms
    solidShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    solidShader->setVec3("lightDirection", config.lightDirection);
    solidShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    solidShader->setFloat("specularExponent", config.specularExponent);
    solidShader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    solidShader->setFloat("normalMappingMix", config.normalMappingMix);
    solidShader->setFloat("reflectionMix", config.reflectionMix);
    solidShader->setFloat("specularExponent", config.specularExponent);


    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    solidShader->setMat4("projection", projection);
    solidShader->setVec3("viewPosition", camera.Position);
    solidShader->setMat4("view", view);
//----

    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(7.5f, 7.5f, 7.5f));
    glm::mat4 whirl = model * glm::rotate(currentTime * -1.0f, glm::vec3(0.0,1.0,0.0)) *
                          glm::rotate(glm::half_pi<float>(), glm::vec3(0.0,1.0,0.0));
    solidShader->setMat4("model", whirl);
    solidShader->setMat4("modelInvT", glm::inverse(glm::transpose(model)));

///----
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    waterModel->Draw(*solidShader);
    glDisable(GL_BLEND);
}

//---------------------------------------------


//----PARTICLE--------
void drawObjects(){
    // declaration of static variable used to store the last view projection
    static glm::mat4 prevViewProjection = glm::mat4(1.0f);

    // draw particles
    glEnable(GL_BLEND);
    rainShader->use();

    // offset of the rain volume
    glm::vec3 forwardOffset = glm::normalize(fpCam.getForward()) * (boxSize/2.f);   //fpCam

    static glm::vec3 velocity[4] = {glm::vec3(0.0f, +10.0f, 0.0f),      // falls up
                                    glm::vec3(0.0f,  +7.0f, 0.0f),
                                    glm::vec3(0.0f, +11.0f, 0.0f),
                                    glm::vec3(0.0f,  +8.0f, 0.0f)};
    static glm::vec3 wind[4] = {glm::vec3(0.85f, 0.0f, 0.0f),
                                glm::vec3(0.00f, 0.0f, 0.7f),
                                glm::vec3(0.55f, 0.0f, 1.3f),
                                glm::vec3(2.11f, 0.0f, 0.0f)};
    static glm::vec3 randomOffset[4] = {glm::vec3(0.10f, 0.2f, 0.3f),
                                        glm::vec3(0.22f, 0.3f, 0.5f),
                                        glm::vec3(0.55f, 0.0f, 1.3f),
                                        glm::vec3(2.11f, 0.0f, 0.0f)};

    for(int i = 0; i < 4; i++) {
        glm::vec3 gravityOffset = velocity[i] * currentTime * (isSnow ? 0.2f : 1.0f);
        glm::vec3 windOffset = wind[i] * currentTime;
        glm::vec3 offsets = gravityOffset + windOffset + randomOffset[i];
        glm::vec3 inverseVelocity = (-velocity[i] - wind[i]) * 0.025f;
        offsets -= fpCam.getPosition() + forwardOffset + boxSize / 2.f; //fpCam
        offsets = glm::mod(offsets, boxSize);

        // set uniform parameters
        rainShader->setVec3("offsets", offsets);
        rainShader->setVec3("inverseVelocity", inverseVelocity);
        rainShader->setVec3("forwardOffset", forwardOffset);
        rainShader->setFloat("boxSize", boxSize);

        rainShader->setBool("renderLines", !isSnow);
        rainShader->setMat4("viewProj", fpCam.getViewProjection());  //fpCam
        rainShader->setMat4("prevViewProj", prevViewProjection);

        rainShader->setVec3("cameraPosition", fpCam.getPosition());  //fpCam

        // draw command
        particlesObj.drawSceneObject(isSnow ? GL_POINTS: GL_LINES);

    }
    // keep current view projection for the next frame
    prevViewProjection = fpCam.getViewProjection(); //fpCam

}

void createParticles(){
    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // initialize particle buffer, set all values to 0
    std::vector<float> data(vertexBufferSize * particleSize);
    auto max_rand = (float) (RAND_MAX);
    for(unsigned int i = 0; i < data.size(); i+=6) {
        data[i] = data[i+3] =(float(rand()) / max_rand) * boxSize;
        data[i+1] = data[i+4] = (float(rand()) / max_rand) * boxSize;
        data[i+2] = data[i+5] = (float(rand()) / max_rand) * boxSize;
    }

    // copy to openGL controlled memory
    glBufferData(GL_ARRAY_BUFFER, vertexBufferSize * particleSize * sizeOfFloat, &data[0], GL_DYNAMIC_DRAW);

    int posSize = 3; // each position has x,y and z
    GLuint vertexLocation = glGetAttribLocation(rainShader->ID, "initPosition");
    glEnableVertexAttribArray(vertexLocation);
    glVertexAttribPointer(vertexLocation, posSize, GL_FLOAT, GL_FALSE, particleSize * sizeOfFloat, nullptr);

    particlesObj.VAO = VAO;
    particlesObj.vertexCount = vertexBufferSize;
    particlesObj.indexedObject = false;
}


//---------------------------------------------
void renderScene(GLFWwindow* window){
    // render depth of scene to texture from the light's perspective
    // -------------------------------------------------------------

    // We use an ortographic projection since it is a directional light.
    // left, right, bottom, top, near and far values define the 3D volume relative to
    // the light position and direction that will be rendered to produce the depth texture.
    // Geometry outside of this range will not be considered when computing shadows.
    float near_plane = 1.0f;
    float half = config.shadowMapSize/2.0f;
    glm::mat4 lightProjection = glm::ortho(-half, half, -half, half, near_plane, near_plane + config.shadowMapDepthRange);
    glm::mat4 lightView = glm::lookAt(config.lightPosition, config.lightPosition+config.lightDirection, glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 lightSpaceMatrix = lightProjection * lightView;

    // setup depth shader
    simpleDepthShader->use();
    simpleDepthShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);

    // setup framebuffer size
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);

    // bind our depth texture to the frame buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);

    // clear the depth texture/depth buffer
    glClear(GL_DEPTH_BUFFER_BIT);

    // draw scene from the light's perspective into the depth texture
    drawScene(simpleDepthShader, true);

    // unbind the depth texture from the frame buffer, now we can render to the screen (frame buffer) again
    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    // render the scene and use the depth from the light's perspective to compute shadows
    // --------------------------------------------------------------

    // reset the render window size
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    // clear the frame buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // draw the skybox
    drawSkybox();

    // setup scene shader
    sceneShader->use();

    // shadow uniforms
    sceneShader->setMat4("lightSpaceMatrix", lightSpaceMatrix);
    sceneShader->setInt("shadowMap", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    sceneShader->setFloat("shadowBias", config.shadowBias * 0.01f);
    sceneShader->setBool("softShadows", config.softShadows);

    // light uniforms
    sceneShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    sceneShader->setVec3("lightDirection", config.lightDirection);
    sceneShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    sceneShader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    sceneShader->setFloat("normalMappingMix", config.normalMappingMix);
    sceneShader->setFloat("reflectionMix", config.reflectionMix);
    sceneShader->setFloat("specularExponent", config.specularExponent);

    // set up skybox texture uniform
    sceneShader->setInt("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

    // draw the scene
    drawScene(sceneShader, false);
}
//---------------------------------------------

//---------------------------------------------
// draw the scene geometry          //------------------DRAWFLOOR----------------
void drawScene(Shader *shader, bool isShadowPass){

    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);
    shader->setVec3("viewPosition", camera.Position);
    shader->setMat4("view", view);

    // draw floor,
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(5.f, 5.f, 5.f));
    shader->setMat4("model", model);
    shader->setMat3("modelInvTra", glm::inverse(glm::transpose(glm::mat3(model))));
    shader->setMat4("view", view);
    floorModel->Draw(*shader);

    if(isShadowPass)
        return;

}
//---------------------------------------------


void drawSkybox(){
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();

    // draw skybox as last
    glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
    skyboxShader->use();
    skyboxShader->setMat4("projection", projection);
    skyboxShader->setMat4("view", view);
    skyboxShader->setInt("skybox", 0);
    // skybox cube
    glBindVertexArray(skyboxVAO);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glDepthFunc(GL_LESS); // set depth function back to default
}



//-----------------//-----------------//-----------------//-----------------

//-----------------//-----------------//-----------------//-----------------
//-----------------//-----------------//-----------------//-----------------
//-----------------//-----------------//-----------------//-----------------


unsigned int createVertexArray(const std::vector<float> &positions, const std::vector<float> &colors, const std::vector<unsigned int> &indices){
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    // bind vertex array object
    glBindVertexArray(VAO);

    // set vertex shader attribute "pos"
    createArrayBuffer(positions); // creates and bind  the VBO
    int posAttributeLocation = glGetAttribLocation(solidShader->ID, "pos");
    glEnableVertexAttribArray(posAttributeLocation);
    glVertexAttribPointer(posAttributeLocation, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

    // set vertex shader attribute "color"
    createArrayBuffer(colors); // creates and bind the VBO
    int colorAttributeLocation = glGetAttribLocation(solidShader->ID, "color");
    glEnableVertexAttribArray(colorAttributeLocation);
    glVertexAttribPointer(colorAttributeLocation, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

    // creates and bind the EBO
    createElementArrayBuffer(indices);

    return VAO;
}



unsigned int createArrayBuffer(const std::vector<float> &array){
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, array.size() * sizeof(GLfloat), &array[0], GL_STATIC_DRAW);

    return VBO;
}

unsigned int createElementArrayBuffer(const std::vector<unsigned int> &array){
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, array.size() * sizeof(unsigned int), &array[0], GL_STATIC_DRAW);

    return EBO;
}


//-----------------//-----------------//-----------------//-----------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

	// movement commands
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);


    // camera movement
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS){
        fpCam.moveCamera(glm::vec3(0,0,-1), .02f);  //fpCam
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS){
        fpCam.moveCamera(glm::vec3(0,0,1), .02f);   //fpCam
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        fpCam.moveCamera(glm::vec3(-1,0,0), 0.02f); //fpCam
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS){
        // vector perpendicular to camera forward and Y-axis
        fpCam.moveCamera(glm::vec3(1,0,0), 0.02f);  //fpCam
    }



    // 1 for snow and 2 for rain
    if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS){
        isSnow = false;
    }
    if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS){
        isSnow = true;
    }

}


void cursor_input_callback(GLFWwindow* window, double posX, double posY){

	// camera rotation
    static bool firstMouse = true;
    if (firstMouse)
    {
        lastX = posX;
        lastY = posY;
        firstMouse = false;
    }

    float xoffset = posX - lastX;
    float yoffset = lastY - posY; // reversed since y-coordinates go from bottom to top

    lastX = posX;
    lastY = posY;


    int screenW, screenH;
    glm::vec2 cursorPosition(0.0f);
   // cursorInRange(posX, posY, screenW, screenH, -1.0f, 1.0f, cursorPosition.x, cursorPosition.y);
  //  fpCam.rotateCamera(cursorPosition, .02f);       //fpCam

    camera.ProcessMouseMovement(xoffset, yoffset);
}


// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(yoffset);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}