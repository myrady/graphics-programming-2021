#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader.h"
#include "glmutils.h"

#include "primitives.h"
#include "firstpersoncamera.h"


//-----------------
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "model.h"
#include "camera.h"
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
void drawCrystals(float x, float y, float z, GLFWwindow* window);
void drawFloor();
void drawSky();
unsigned int initSkyboxBuffers();
unsigned int loadCubemap(vector<std::string> faces);

//-----------------//-----------------//-----------------//-----------------

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const unsigned int vertexBufferSize = 200, particleSize = 3, sizeOfFloat = 4;
unsigned int depthMap, depthMapFBO;
const unsigned int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;

//-----------------//-----------------//-----------------//-----------------

// global variables used for rendering
//SceneObject cubeObj;
SceneObject floorObj;
SceneObject particlesObj;

//---------rain-----------
Shader* rainShader;
Shader* solidShader;
//--------floor------------
Shader* floorShader;
Model* floorModel;
unsigned int floorTextureId;
//---------sky------------
Shader* skyboxShader;
unsigned int skyboxVAO; // skybox handle
unsigned int cubemapTexture; // skybox texture handle
//---------stuffs---------
//Shader* waterShader;
//Shader* bloomShader;
//Shader* treeShader;
Model* crystalModel;
Model* treeModel;
Model* firModel;


float currentTime;
float deltaTime;

bool isSnow = false;
float boxSize = 30.f;

FirstPersonCamera fpCam(glm::radians(70.0f), SCR_WIDTH, SCR_HEIGHT, .1f, 110.0f, glm::vec3(0, 10.6f, 0), glm::vec3(0, 0, -1));
//Camera camera(glm::vec3(0.0f, 16.6f, 15.0f));            //------------CAMERA POS

//-----------------//-----------------//-----------------//-----------------

struct Config {
    //-----------LIGHT COLOUR---------------
    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 0.9f, 0.9f};
    float ambientLightIntensity = 0.925f;

    // light 1
    glm::vec3 lightPosition = {10.2f, 20.5f, 0.8f};
    glm::vec3 lightDirection = {2.7f, -1.0f, 0.7f};
    glm::vec3 lightColor = {1.0f, 0.5f, 1.0f};
    float lightIntensity = 0.75f;
    //--------------------------------------
    // material
    float specularExponent = 80.0f;
    float ambientOcclusionMix = 0.80f;
    float normalMappingMix = 0.80f;
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
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // setup mesh objects
    // ---------------------------------------
    setup();


    //-----DEBUG-------
    //glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);
    // ---------------------------------------

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
    glDepthRange(-1,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    // ---------------------------------------
    //--IMGUI init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    //--Setup Dear ImGui style
    ImGui::StyleColorsDark();

    //--Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
    // ---------------------------------------


    skyboxShader = new Shader("shaders/skybox.vert", "shaders/skybox.frag");
    // init skybox
    vector<std::string> faces
            {
                    "skybox/right.tga",
                    "skybox/left.tga",
                    "skybox/top.tga",
                    "skybox/bottom.tga",
                    "skybox/front.tga",
                    "skybox/back.tga"
            };
    cubemapTexture = loadCubemap(faces);
    skyboxVAO = initSkyboxBuffers();





    // render loop
    // -----------
    // render every loopInterval seconds
    float loopInterval = 0.02f;
    auto begin = std::chrono::high_resolution_clock::now();

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

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);       // ---------sky

        // notice that we also need to clear the depth buffer (aka z-buffer) every new frame
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

// ---------DRAW---------
        //glEnable(GL_BLEND);
        drawTrees(firModel, -50.0, 5.2, -45.296);
       // drawTrees(firModel, -50.0, 5.2, -55.296);
        drawTrees(firModel, -60.0, 5.2, -75.296);
      //  drawTrees(firModel, -30.0, 7.5, -55.296);
        drawTrees(firModel, -10.0, 7.5, -65.296);
        drawTrees(firModel, 20.0, 6.2, -65.296);

        drawTrees(treeModel, -40.0, 6.2, -45.296);
        drawTrees(treeModel,55.0, 7.5, -20.296);
     //   drawTrees(treeModel, -40.0, 0.2, 40.296);
        drawTrees(treeModel, -60.0, 6.2, 45.296);
     //   drawTrees(treeModel, -60.0, 5.5, 25.296);
        drawTrees(treeModel, -55.0, 3.5, -20.296);
    //    drawTrees(treeModel,65.0, 14.5, -60.296);
     //   drawTrees(treeModel, -60.0, 9.2, 70.296);
        drawTrees(treeModel, 60.0, 8.2, 45.296);
        drawTrees(treeModel, 60.0, 3.0, 25.296);


        drawCrystals(-10.7432, 7.0, -55.296, window);
        drawCrystals(-20.7432, 5.0, -50.296, window);
        drawCrystals(-30.0, 1.5, -35.296, window);

        drawCrystals(-10.7432, 6.0, 25.296, window);
        drawCrystals(-60.7432, 5.0, 15.296, window);
        drawCrystals(40.0, 4.0, 2.296, window);

        drawFloor();
        drawObjects();
        drawSky();
        //glDisable(GL_BLEND);
// ---------
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
    delete floorModel;
    delete floorShader;
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

// init the VAO of the skybox
// --------------------------
unsigned int initSkyboxBuffers(){
    // triangles forming the six faces of a cube
    // note that the camera is placed inside of the cube, so the winding order
    // is selected to make the triangles visible from the inside
    float skyboxVertices[108]  {
            // x, y and z coordinates
            -1.0f, 1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, -1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 1.0f,
            -1.0f, -1.0f, 1.0f,

            -1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, -1.0f,
            1.0f, 1.0f, 1.0f,
            1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, 1.0f,
            -1.0f, 1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, 1.0f,
            1.0f, -1.0f, 1.0f
    };

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);

    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    return skyboxVAO;
}

// -------------------------------------------------------
unsigned int loadCubemap(vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrComponents;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}


void drawSky(){
    skyboxShader->use();

    // light uniforms
    skyboxShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    skyboxShader->setVec3("lightPosition", config.lightPosition);
    skyboxShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    skyboxShader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    skyboxShader->setFloat("attenuationC0", config.attenuationC0);
    skyboxShader->setFloat("attenuationC1", config.attenuationC1);
    skyboxShader->setFloat("attenuationC2", config.attenuationC2);

    // TODO exercise 9.2 send uvScale to the shader as a uniform variable

    // camera parameters                                        //zoom
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = fpCam.GetViewMatrix();        //camera
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    skyboxShader->setMat4("projection", projection);


    // set up skybox texture
    skyboxShader->setInt("skybox", 4);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

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

    // TODO exercise 9.2 send uvScale to the shader as a uniform variable

    // camera parameters                                        //zoom
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = fpCam.GetViewMatrix();        //camera
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);

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



void drawCrystals(float x, float y, float z, GLFWwindow* window){
    Shader* shader = new Shader("shaders/tree_shader.vert", "shaders/tree_shader.frag");
    shader->use();



//-----------------------------------------
    // light uniforms
    shader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    shader->setVec3("lightDirection", config.lightDirection);
    shader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    shader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    shader->setFloat("normalMappingMix", config.normalMappingMix);
    shader->setFloat("reflectionMix", config.reflectionMix);
    shader->setFloat("specularExponent", config.specularExponent);

    // TODO exercise 9.2 send uvScale to the shader as a uniform variable

    // camera parameters                                        //zoom
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = fpCam.GetViewMatrix();        //camera
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    shader->setMat4("projection", projection);

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


//-----------------//-----------------//-----------------//-----------------

void drawFloor(){




    floorShader->use();
    // light uniforms
    floorShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    floorShader->setVec3("lightPosition", config.lightPosition);
    floorShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    floorShader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    floorShader->setFloat("attenuationC0", config.attenuationC0);
    floorShader->setFloat("attenuationC1", config.attenuationC1);
    floorShader->setFloat("attenuationC2", config.attenuationC2);

    // TODO exercise 9.2 send uvScale to the shader as a uniform variable

    // camera parameters                                        //zoom
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = fpCam.GetViewMatrix();        //camera
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    floorShader->setMat4("projection", projection);

    glActiveTexture(GL_TEXTURE0);
    floorShader->setInt("texture_diffuse1", 0);
    glBindTexture(GL_TEXTURE_2D, floorTextureId);
    // draw floor,
    // notice that we overwrite the value of one of the uniform variables to set a different floor color
    floorShader->setVec3("reflectionColor", .2, .5, .2);
    glm::mat4 model = glm::scale(glm::mat4(1.0), glm::vec3(5.f, 5.f, 5.f));
    floorShader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    floorShader->setMat4("invTranspMV", invTranspose);
    floorShader->setMat4("view", view);

    floorModel->Draw(*floorShader);
}



//-----------------//-----------------//-----------------//-----------------
            //----PARTICLE--------
void drawObjects(){
    // declaration of static variable used to store the last view projection
    static glm::mat4 prevViewProjection = glm::mat4(1.0f);

    // draw ground and boxes
    glDisable(GL_BLEND);
    solidShader->use();
    glm::mat4 viewProjection = fpCam.getViewProjection();  //fpCam

    // draw floor (the floor was built so that it does not need to be transformed)
    solidShader->setMat4("model", viewProjection);
    floorObj.drawSceneObject(GL_TRIANGLES);



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


//-----------------//-----------------//-----------------//-----------------




void setup(){
    // initialize shaders
    solidShader = new Shader("shader.vert", "shader.frag");
    rainShader = new Shader("rain.vert", "rain.frag");

    //-----NEW STUFFS----
    floorShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    floorModel = new Model("exam/flo.obj");     //flooor
    treeModel = new Model("exam/stuffs/tree1.obj");
    firModel = new Model("exam/stuffs/tree2.obj");
    crystalModel = new Model("exam/stuffs/cry.obj");
    //-----SHADERS-----
    //bloomShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    //treeShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    //waterShader = new Shader("shaders/floor_shader.vert", "shaders/floor_shader.frag");
    //--------------------

    // load floor mesh into openGL
    floorObj.VAO = createVertexArray(floorVertices, floorColors, floorIndices);
    floorObj.vertexCount = floorIndices.size();
    floorObj.indexedObject = true;


    createParticles();
}

//-----------------//-----------------//-----------------//-----------------



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

// min and max parameters
void cursorInRange(float screenX, float screenY, int screenW, int screenH, float min, float max, float &x, float &y){
    float sum = max - min;
    float xInRange = (float) screenX / (float) screenW * sum - sum/2.0f;
    float yInRange = (float) screenY / (float) screenH * sum - sum/2.0f;
    x = xInRange;
    y = -yInRange; // flip screen space y axis
}

void cursor_input_callback(GLFWwindow* window, double posX, double posY){
    int screenW, screenH;

    // get cursor position and scale it down to a smaller range
    glfwGetWindowSize(window, &screenW, &screenH);
    glm::vec2 cursorPosition(0.0f);
    cursorInRange(posX, posY, screenW, screenH, -1.0f, 1.0f, cursorPosition.x, cursorPosition.y);
    fpCam.rotateCamera(cursorPosition, .02f);       //fpCam

}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

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

// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}