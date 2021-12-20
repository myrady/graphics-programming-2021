#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

#include <vector>
#include <chrono>

#include "shader.h"
#include "camera.h"
#include "model.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"


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


// function declarations
// ---------------------
void loadFloorTexture();
void drawCar();
void drawFloor();
void drawRain();
void createParticles();

// glfw and input functions
// ------------------------
void processInput(GLFWwindow* window);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void cursor_input_callback(GLFWwindow* window, double posX, double posY);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// screen settings
// ---------------
const unsigned int SCR_WIDTH = 800; //1280;
const unsigned int SCR_HEIGHT = 600;//720;

// global variables used for rendering
// -----------------------------------
Shader* carShader;
Shader* floorShader;
Shader* rainShader;
SceneObject particlesObj;
Model* carPaint;
Model* carBody;
Model* carInterior;
Model* carLight;
Model* carWindow;
Model* carWheel;
Model* floorModel;
unsigned int floorTextureId;
Camera camera(glm::vec3(0.0f, 16.6f, 15.0f));            //------------CAMERA POS


// global variables used for control
// ---------------------------------
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
float deltaTime;

float boxSize = 30.f;
bool isSnow = false;
float currentTime;
const unsigned int vertexBufferSize = 2000, particleSize = 3, sizeOfFloat = 4;
// parameters that can be set in our GUI
// -------------------------------------
struct Config {
                //-----------LIGHT COLOUR---------------
    // ambient light
    glm::vec3 ambientLightColor = {1.0f, 0.9f, 0.9f};
    float ambientLightIntensity = 0.925f;

    // light 1
    glm::vec3 lightPosition = {10.2f, 20.5f, 0.8f};
    glm::vec3 lightColor = {1.0f, 0.5f, 1.0f};
    float lightIntensity = 0.75f;
                //--------------------------------------
    // material
    float specularExponent = 80.0f;
    float ambientOcclusionMix = 1.0f;

    // attenuation (c0, c1 and c2 on the slides)
    float attenuationC0 = 0.25;
    float attenuationC1 = 0.1;
    float attenuationC2 = 0.1;

    // TODO exercise 9.2 scale config variable
    // floor texture mode
    unsigned int wrapSetting = GL_REPEAT;
    unsigned int minFilterSetting = GL_LINEAR_MIPMAP_LINEAR;
    unsigned int magFilterSetting = GL_LINEAR;

} config;



//---DEBUG
void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    std::cout << "ERROR";
}



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

    // ----------------------------------------------------------------------------------------------------

    // floor texture
    glGenTextures(1, &floorTextureId);
    loadFloorTexture();

    rainShader = new Shader("shaders/rain.vert", "shaders/rain.frag");
    carShader = new Shader("shaders/car_shader.vert", "shaders/car_shader.frag");
    floorShader = new Shader("shaders/floor_Shader.vert", "shaders/floor_Shader.frag");
	carPaint = new Model("car/Paint_LOD0.obj");
	carBody = new Model("car/Body_LOD0.obj");
	carLight = new Model("car/Light_LOD0.obj");
	carInterior = new Model("car/Interior_LOD0.obj");
	carWindow = new Model("car/Windows_LOD0.obj");
	carWheel = new Model("car/Wheel_LOD0.obj");
	floorModel = new Model("floor/flooor.obj");

    // set up the z-buffer
    glDepthRange(-1,1); // make the NDC a right handed coordinate system, with the camera pointing towards -z
    glEnable(GL_DEPTH_TEST); // turn on z-buffer depth test
    glDepthFunc(GL_LESS); // draws fragments that are closer to the screen in NDC

    //-----DEBUG-------
    glEnable(GL_DEBUG_OUTPUT);
    //glDebugMessageCallback(MessageCallback, 0);


    // IMGUI init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");


	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    createParticles();      //---raining

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

        glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        drawFloor();
        drawCar();
        drawRain();


        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

	//delete carModel;
	delete floorModel;
	delete carWindow;
	delete carPaint;
	delete carInterior;
	delete carLight;
	delete carBody;
    delete carWheel;
    delete floorShader;
    delete carShader;

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// ------------------------------------
// TODO EXERCISE 9.1 LOAD FLOOR TEXTURE
// ------------------------------------

void loadFloorTexture(){
    // TODO this is mostly a copy and paste of the function 'TextureFromFile' in the 'model.h' file
    //  however, you should use the min/mag/wrap settings that you can control in the user interface
    //  and load the texture 'floor/checkboard_texture.png'
    int width, height, nrComponents;
    unsigned char *data = stbi_load("floor/FloorAlbedo.jpg", &width, &height, &nrComponents, 0);
    if (data) {
        GLenum format = GL_RGB;

        glBindTexture(GL_TEXTURE_2D, floorTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, config.wrapSetting);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, config.wrapSetting);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, config.minFilterSetting);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, config.magFilterSetting);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load" << std::endl;
        stbi_image_free(data);
    }
}

// --------------
// DRAW FUNCTIONS
// --------------

void drawRain() {
    glEnable(GL_BLEND);
    rainShader->use();

    static glm::mat4 prevViewProjection = glm::mat4(1.0f);
    // offset of the rain volume
    glm::vec3 forwardOffset = glm::normalize(camera.getForward()) * (boxSize / 2.f);

    static glm::vec3 velocity[14] = {glm::vec3(0.0f, +10.0f, 0.0f),          //----- if + then goes up
                                    glm::vec3(0.0f, +7.0f, 0.0f),
                                    glm::vec3(0.0f, +11.0f, 0.0f),
                                    glm::vec3(0.0f, +8.0f, 0.0f)};
    static glm::vec3 wind[14] = {glm::vec3(0.85f, 0.0f, 0.0f),
                                glm::vec3(0.00f, 0.0f, 0.7f),
                                glm::vec3(0.55f, 0.0f, 1.3f),
                                glm::vec3(2.11f, 0.0f, 0.0f)};
    static glm::vec3 randomOffset[14] = {glm::vec3(0.10f, 0.2f, 0.3f),
                                        glm::vec3(0.22f, 0.3f, 0.5f),
                                        glm::vec3(0.55f, 0.0f, 1.3f),
                                        glm::vec3(2.11f, 0.0f, 0.0f)};

    for (int i = 0; i < 14; i++) {
        glm::vec3 gravityOffset = velocity[i] * currentTime * (isSnow ? 0.2f : 0.60f);       // rain velocity
        glm::vec3 windOffset = wind[i] * currentTime;
        glm::vec3 offsets = gravityOffset + windOffset + randomOffset[i];
        glm::vec3 inverseVelocity = (-velocity[i] - wind[i]) * 0.025f;
        offsets -= camera.getPosition() + forwardOffset + boxSize / 2.f;
        offsets = glm::mod(offsets, boxSize);

        // set uniform parameters
        rainShader->setVec3("offsets", offsets);
        rainShader->setVec3("inverseVelocity", inverseVelocity);
        rainShader->setVec3("forwardOffset", forwardOffset);
        rainShader->setFloat("boxSize", boxSize);

        rainShader->setBool("renderLines", !isSnow);
        //glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        rainShader->setMat4("viewProj", camera.getViewProjection());    //projection    camera.getViewProjection()
        rainShader->setMat4("prevViewProj", prevViewProjection);

        rainShader->setVec3("cameraPosition", camera.getPosition());

        // draw command
        particlesObj.drawSceneObject(isSnow ? GL_POINTS : GL_LINES);
    }
    // keep current view projection for the next frame
    prevViewProjection = camera.getViewProjection();
}


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



    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
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


void drawCar(){
    carShader->use();
    // light uniforms
    carShader->setVec3("ambientLightColor", config.ambientLightColor * config.ambientLightIntensity);
    carShader->setVec3("lightPosition", config.lightPosition);
    carShader->setVec3("lightColor", config.lightColor * config.lightIntensity);

    // material uniforms
    carShader->setFloat("ambientOcclusionMix", config.ambientOcclusionMix);
    carShader->setFloat("specularExponent", config.specularExponent);

    // attenuation uniforms
    carShader->setFloat("attenuationC0", config.attenuationC0);
    carShader->setFloat("attenuationC1", config.attenuationC1);
    carShader->setFloat("attenuationC2", config.attenuationC2);


    // camera parameters
    glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 viewProjection = projection * view;

    // set projection matrix uniform
    carShader->setMat4("projection", projection);

    // draw wheel
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, 1.39));
    carShader->setMat4("model", model);
    glm::mat4 invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::translate(glm::mat4(1.0f), glm::vec3(-.7432, .328, -1.296));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, 1.296));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw wheel
    model = glm::rotate(glm::mat4(1.0f), glm::pi<float>(), glm::vec3(0.0, 1.0, 0.0));
    model = glm::translate(model, glm::vec3(-.7432, .328, -1.39));
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carWheel->Draw(*carShader);

    // draw the rest of the car
    model = glm::mat4(1.0f);
    carShader->setMat4("model", model);
    invTranspose = glm::inverse(glm::transpose(view * model));
    carShader->setMat4("invTranspMV", invTranspose);
    carShader->setMat4("view", view);
    carBody->Draw(*carShader);
    carInterior->Draw(*carShader);
    carPaint->Draw(*carShader);
    carLight->Draw(*carShader);
    glEnable(GL_BLEND);
    carWindow->Draw(*carShader);
    glDisable(GL_BLEND);

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








// ---------------
// INPUT FUNCTIONS
// ---------------

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