#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

// Window dimensions
const GLuint WIDTH = 1200, HEIGHT = 800;

// Camera variables
glm::vec3 cameraPos = glm::vec3(0.0f, 20.0f, 50.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float yaw = -90.0f, pitch = 0.0f;
float cameraSpeed = 10.0f;

// Time variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;
float animationTime = 0.0f;

// Input handling
bool keys[1024];
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void processInput(GLFWwindow* window);

// Mouse variables
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Shader source code
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 viewPos;
uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 objectColor;
uniform float emissionStrength;
uniform vec3 emissionColor;

void main()
{
    // Ambient lighting
    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor;
    
    // Diffuse lighting
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;
    
    // Specular lighting
    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor;
    
    // Emission (for glowing effects)
    vec3 emission = emissionStrength * emissionColor;
    
    vec3 result = (ambient + diffuse + specular) * objectColor + emission;
    FragColor = vec4(result, 1.0);
}
)";

// Utility functions
GLuint compileShader(const char* source, GLenum type) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cout << "Shader compilation failed: " << infoLog << std::endl;
    }
    return shader;
}

GLuint createShaderProgram() {
    GLuint vertexShader = compileShader(vertexShaderSource, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderSource, GL_FRAGMENT_SHADER);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

// 3D Object classes
struct Building {
    glm::vec3 position;
    glm::vec3 scale;
    glm::vec3 color;
    float emissionStrength;
    glm::vec3 emissionColor;
    
    Building(glm::vec3 pos, glm::vec3 sc, glm::vec3 col, float emission = 0.0f, glm::vec3 emCol = glm::vec3(0.0f)) 
        : position(pos), scale(sc), color(col), emissionStrength(emission), emissionColor(emCol) {}
};

struct Vehicle {
    glm::vec3 position;
    glm::vec3 direction;
    float speed;
    glm::vec3 color;
    float pathRadius;
    float pathAngle;
    
    Vehicle(glm::vec3 pos, glm::vec3 dir, float sp, glm::vec3 col, float radius = 30.0f) 
        : position(pos), direction(dir), speed(sp), color(col), pathRadius(radius), pathAngle(0.0f) {}
    
    void update(float deltaTime) {
        pathAngle += speed * deltaTime;
        position.x = pathRadius * cos(pathAngle);
        position.z = pathRadius * sin(pathAngle);
    }
};

struct Billboard {
    glm::vec3 position;
    float rotation;
    float rotationSpeed;
    glm::vec3 color;
    
    Billboard(glm::vec3 pos, float rotSpeed, glm::vec3 col) 
        : position(pos), rotation(0.0f), rotationSpeed(rotSpeed), color(col) {}
    
    void update(float deltaTime) {
        rotation += rotationSpeed * deltaTime;
    }
};

// Cube vertices with normals and texture coordinates
float cubeVertices[] = {
    // positions          // normals           // texture coords
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
     0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,

    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   1.0f, 1.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f,  0.0f,  0.0f, 1.0f,   0.0f, 0.0f,

    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
    -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,

    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
     0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
     0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
     0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
     0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
    -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
};

class FuturisticCity {
private:
    GLuint VBO, VAO;
    GLuint shaderProgram;
    std::vector<Building> buildings;
    std::vector<Vehicle> vehicles;
    std::vector<Billboard> billboards;
    std::random_device rd;
    std::mt19937 gen;
    
public:
    FuturisticCity() : gen(rd()) {
        setupBuffers();
        shaderProgram = createShaderProgram();
        generateCity();
    }
    
    void setupBuffers() {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        
        glBindVertexArray(VAO);
        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
        
        // Position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        
        // Normal attribute
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Texture coordinate attribute
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);
    }
    
    void generateCity() {
        std::uniform_real_distribution<float> posDist(-50.0f, 50.0f);
        std::uniform_real_distribution<float> heightDist(5.0f, 40.0f);
        std::uniform_real_distribution<float> widthDist(2.0f, 8.0f);
        std::uniform_real_distribution<float> colorDist(0.1f, 0.9f);
        
        // Generate buildings
        for (int i = 0; i < 50; i++) {
            float x = posDist(gen);
            float z = posDist(gen);
            float height = heightDist(gen);
            float width = widthDist(gen);
            
            glm::vec3 pos(x, height / 2.0f, z);
            glm::vec3 scale(width, height, width);
            
            // Cyberpunk color palette
            glm::vec3 color;
            float colorChoice = colorDist(gen);
            if (colorChoice < 0.3f) {
                color = glm::vec3(0.2f, 0.2f, 0.8f); // Blue
            } else if (colorChoice < 0.6f) {
                color = glm::vec3(0.8f, 0.2f, 0.8f); // Magenta
            } else {
                color = glm::vec3(0.2f, 0.8f, 0.8f); // Cyan
            }
            
            float emission = (height > 20.0f) ? 0.3f : 0.1f;
            buildings.push_back(Building(pos, scale, color, emission, color * 0.5f));
        }
        
        // Generate flying vehicles
        for (int i = 0; i < 8; i++) {
            float height = 15.0f + i * 3.0f;
            float radius = 20.0f + i * 5.0f;
            float speed = 0.5f + (i % 3) * 0.3f;
            
            glm::vec3 pos(radius, height, 0.0f);
            glm::vec3 dir(0.0f, 0.0f, 1.0f);
            glm::vec3 color(1.0f, 0.8f, 0.2f); // Golden headlights
            
            Vehicle vehicle(pos, dir, speed, color, radius);
            vehicle.pathAngle = i * (2.0f * M_PI / 8.0f); // Distribute evenly
            vehicles.push_back(vehicle);
        }
        
        // Generate holographic billboards
        for (int i = 0; i < 15; i++) {
            float x = posDist(gen);
            float z = posDist(gen);
            float y = 20.0f + i * 2.0f;
            
            glm::vec3 pos(x, y, z);
            float rotSpeed = 30.0f + (i % 3) * 20.0f;
            glm::vec3 color(0.0f, 1.0f, 0.5f); // Holographic green
            
            billboards.push_back(Billboard(pos, rotSpeed, color));
        }
    }
    
    void update(float deltaTime) {
        animationTime += deltaTime;
        
        // Update vehicles
        for (auto& vehicle : vehicles) {
            vehicle.update(deltaTime);
        }
        
        // Update billboards
        for (auto& billboard : billboards) {
            billboard.update(deltaTime);
        }
    }
    
    void render(glm::mat4 view, glm::mat4 projection) {
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        
        // Set uniforms
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(shaderProgram, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3f(glGetUniformLocation(shaderProgram, "lightPos"), 0.0f, 50.0f, 0.0f);
        glUniform3f(glGetUniformLocation(shaderProgram, "lightColor"), 0.3f, 0.3f, 0.7f);
        
        // Render buildings
        for (const auto& building : buildings) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, building.position);
            model = glm::scale(model, building.scale);
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(building.color));
            glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), building.emissionStrength);
            glUniform3fv(glGetUniformLocation(shaderProgram, "emissionColor"), 1, glm::value_ptr(building.emissionColor));
            
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
        // Render vehicles
        for (const auto& vehicle : vehicles) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, vehicle.position);
            model = glm::scale(model, glm::vec3(1.5f, 0.5f, 3.0f));
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(vehicle.color));
            glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), 0.8f);
            glUniform3fv(glGetUniformLocation(shaderProgram, "emissionColor"), 1, glm::value_ptr(vehicle.color));
            
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
        
        // Render billboards
        for (const auto& billboard : billboards) {
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, billboard.position);
            model = glm::rotate(model, glm::radians(billboard.rotation), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(3.0f, 2.0f, 0.1f));
            
            glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(glGetUniformLocation(shaderProgram, "objectColor"), 1, glm::value_ptr(billboard.color));
            glUniform1f(glGetUniformLocation(shaderProgram, "emissionStrength"), 0.9f);
            glUniform3fv(glGetUniformLocation(shaderProgram, "emissionColor"), 1, glm::value_ptr(billboard.color));
            
            glDrawArrays(GL_TRIANGLES, 0, 36);
        }
    }
    
    ~FuturisticCity() {
        glDeleteVertexArrays(1, &VAO);
        glDeleteBuffers(1, &VBO);
        glDeleteProgram(shaderProgram);
    }
};

// Global city instance
FuturisticCity* city = nullptr;

int main() {
    // Initialize GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    // Create window
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Futuristic City at Night", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    // Set callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    
    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return -1;
    }
    
    // Configure OpenGL
    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create city
    city = new FuturisticCity();
    
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        // Calculate deltaTime
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // Process input
        processInput(window);
        glfwPollEvents();
        
        // Update
        city->update(deltaTime);
        
        // Render
        glClearColor(0.05f, 0.05f, 0.15f, 1.0f); // Dark night sky
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Create matrices
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH / (float)HEIGHT, 0.1f, 200.0f);
        
        // Render city
        city->render(view, projection);
        
        glfwSwapBuffers(window);
    }
    
    // Clean up
    delete city;
    glfwTerminate();
    return 0;
}

// Input handling functions
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    
    if (key >= 0 && key < 1024) {
        if (action == GLFW_PRESS)
            keys[key] = true;
        else if (action == GLFW_RELEASE)
            keys[key] = false;
    }
}

void processInput(GLFWwindow* window) {
    float velocity = cameraSpeed * deltaTime;
    
    if (keys[GLFW_KEY_W])
        cameraPos += velocity * cameraFront;
    if (keys[GLFW_KEY_S])
        cameraPos -= velocity * cameraFront;
    if (keys[GLFW_KEY_A])
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (keys[GLFW_KEY_D])
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * velocity;
    if (keys[GLFW_KEY_SPACE])
        cameraPos += velocity * cameraUp;
    if (keys[GLFW_KEY_LEFT_SHIFT])
        cameraPos -= velocity * cameraUp;
    
    // Speed control
    if (keys[GLFW_KEY_LEFT_CONTROL])
        cameraSpeed = 25.0f;
    else
        cameraSpeed = 10.0f;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }
    
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    
    yaw += xoffset;
    pitch += yoffset;
    
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;
    
    glm::vec3 direction;
    direction.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    direction.y = sin(glm::radians(pitch));
    direction.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(direction);
}
