/*
COMP 371 - assignment 2

written by:
 Marc El Haddad     id: 40231208
 Ahmad Al Habbal    id: 40261029
 Mostafa Mohamed    id: 40201893
 Joseph Keshishian  id: 40297447
*/

#include <iostream>
#include <list>
#include <algorithm>
#include <cmath>
#include <vector>
#include <fstream>
#include <sstream>

#define GLEW_STATIC 1
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

using namespace glm;
using namespace std;

// === GLOBALS ===
vec3 cameraPos = vec3(0.0f, 6.0f, 18.0f);  // farther back and higher
vec3 cameraFront = vec3(0.0f, 0.0f, -1.0f);
vec3 cameraUp = vec3(0.0f, 1.0f, 0.0f);
float cameraYaw = -90.0f, cameraPitch = 0.0f;
double lastX = 400, lastY = 300;
bool firstMouse = true;

bool showTexture = true;
bool tKeyPressed = false; //T Pressing Feature
// Change the Colors 
bool pauseLight = false;
bool lKeyPressed = false;

vec3 lightColor = vec3(1.0f); // start white
bool mKeyPressed = false;
int lightColorIndex = 0;
vec3 lightColors[] = {
    vec3(1.0f, 1.0f, 1.0f), // white
    vec3(1.0f, 0.0f, 0.0f), // red
    vec3(0.0f, 1.0f, 0.0f), // green
    vec3(0.0f, 0.0f, 1.0f)  // blue
};
// SHADOW MAPPING
const GLuint SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
GLuint depthMapFBO, depthMap;
GLuint depthShader;
mat4 lightSpaceMatrix;

// === STRUCTURES ===
struct TexturedVertex {
    vec3 position;
    vec3 normal;
    vec2 uv;
};

struct Mesh {
    GLuint vao, vbo, ebo;
    GLsizei count;
};

// === HELPERS ===
string loadShaderSource(const char* filepath) {
    ifstream file(filepath);
    stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

GLuint compileShaderFromFile(const char* filepath, GLenum type) {
    string code = loadShaderSource(filepath);
    const char* src = code.c_str();
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char info[512];
        glGetShaderInfoLog(shader, 512, nullptr, info);
        cerr << "Shader compile error (" << filepath << "): " << info << endl;
    }
    return shader;
}

GLuint createShaderProgramFromFiles(const char* vertexPath, const char* fragmentPath) {
    GLuint vs = compileShaderFromFile(vertexPath, GL_VERTEX_SHADER);
    GLuint fs = compileShaderFromFile(fragmentPath, GL_FRAGMENT_SHADER);
    GLuint program = glCreateProgram();
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return program;
}

Mesh loadOBJ(const char* path) {
    vector<vec3> positions, normals;
    vector<vec2> texCoords;
    vector<TexturedVertex> vertices;
    vector<GLuint> indices;
    ifstream file(path);
    string line;
    while (getline(file, line)) {
        istringstream iss(line);
        string type;
        iss >> type;
        if (type == "v") {
            vec3 v; iss >> v.x >> v.y >> v.z;
            positions.push_back(v);
        } else if (type == "vt") {
            vec2 uv; iss >> uv.x >> uv.y;
            texCoords.push_back(uv);
        } else if (type == "vn") {
            vec3 n; iss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        } else if (type == "f") {
            GLuint idx[3];
            for (int i = 0; i < 3; ++i) {
                string v; iss >> v;
                sscanf(v.c_str(), "%u/%u/%u", &idx[0], &idx[1], &idx[2]);
                TexturedVertex vert;
                vert.position = positions[idx[0] - 1];
                vert.uv = texCoords[idx[1] - 1];
                vert.normal = normals[idx[2] - 1];
                vertices.push_back(vert);
                indices.push_back(indices.size());
            }
        }
    }

    Mesh mesh;
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TexturedVertex), vertices.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)sizeof(vec3));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(TexturedVertex), (void*)(2 * sizeof(vec3)));
    glEnableVertexAttribArray(2);

    mesh.count = indices.size();
    return mesh;
}

GLuint loadTexture(const char* path) {
    int w, h, ch;
    unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
    if (!data) {
        cerr << "Failed to load texture: " << path << endl;
        return 0;
    }
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLenum format = (ch == 4) ? GL_RGBA : GL_RGB;
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);
    return tex;
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
    cameraYaw += xoffset;
    cameraPitch += yoffset;
cameraPitch = ::glm::clamp(cameraPitch, -89.0f, 89.0f);    vec3 direction;
    direction.x = cos(radians(cameraYaw)) * cos(radians(cameraPitch));
    direction.y = sin(radians(cameraPitch));
    direction.z = sin(radians(cameraYaw)) * cos(radians(cameraPitch));
    cameraFront = normalize(direction);
}

void processInput(GLFWwindow* window, float deltaTime) {
    float speed = 2.5f * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= speed * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= normalize(cross(cameraFront, cameraUp)) * speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += normalize(cross(cameraFront, cameraUp)) * speed;

    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS && !tKeyPressed) {
        showTexture = !showTexture;
        tKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_T) == GLFW_RELEASE) {
        tKeyPressed = false;
    }
      // L key to pause/unpause light
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS && !lKeyPressed) {
        pauseLight = !pauseLight;
        lKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_RELEASE) {
        lKeyPressed = false;
    }

    // M key to cycle light colors
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && !mKeyPressed) {
        lightColorIndex = (lightColorIndex + 1) % 4;
        lightColor = lightColors[lightColorIndex];
        mKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_RELEASE) {
        mKeyPressed = false;
    }

}

int main() {
    glfwInit();
    GLFWwindow* window = glfwCreateWindow(800, 600, "COMP371 A2", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glewInit();
    glEnable(GL_DEPTH_TEST);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    GLuint shader = createShaderProgramFromFiles("src/phong.vert", "src/phong.frag");
    depthShader = createShaderProgramFromFiles("src/shadow_depth.vert", "src/shadow_depth.frag");

Mesh model = loadOBJ("models/diamond.obj");
    Mesh moon = loadOBJ("models/moon.obj");
    GLuint tex = loadTexture("textures/13770019-sprite_l.jpg");
    GLuint moonTex = loadTexture("textures/moon-background-photo.jpg");


    // Depth framebuffer
    glGenFramebuffers(1, &depthMapFBO);
    glGenTextures(1, &depthMap);
    glBindTexture(GL_TEXTURE_2D, depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    float borderColor[] = {1.0, 1.0, 1.0, 1.0};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    mat4 projection = perspective(radians(60.0f), 800.f / 600.f, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window)) {
        static float last = glfwGetTime();
        float now = glfwGetTime();
        float dt = now - last;
        last = now;

        processInput(window, dt);

        mat4 view = lookAt(cameraPos, cameraPos + cameraFront, cameraUp);

        
      mat4 modelMat = mat4(1.0f);
modelMat = translate(modelMat, vec3(0.0f, 3.0f, 0.0f));
modelMat = rotate(modelMat, now, vec3(0, 1, 0)); // spin around Y
modelMat = scale(modelMat, vec3(1.8f));

      mat4 modelMatMoon = mat4(1.0f);
modelMatMoon = rotate(modelMatMoon, now * 1.5f, vec3(0, 1, 0));         // Orbit around body
modelMatMoon = translate(modelMatMoon, vec3(4.0f, 5.0f, 0.0f));
modelMatMoon = rotate(modelMatMoon, now * 3.0f, vec3(0, 1, 0));        // Spin on axis
modelMatMoon = scale(modelMatMoon, vec3(0.6f));              // Slightly bigger      


static float lightAngle = 0.0f;
if (!pauseLight) {
    lightAngle += dt;   // advances only when not paused
}
vec3 lightPos = vec3(6.0f * cos(lightAngle), 5.0f, 6.0f * sin(lightAngle));

mat4 lightProjection = perspective(radians(60.0f), 1.0f, 1.0f, 20.0f);
        mat4 lightView = lookAt(lightPos, vec3(0.0f), vec3(0, 1, 0));
        lightSpaceMatrix = lightProjection * lightView;

        // First Pass: shadow map
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glUseProgram(depthShader);
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, value_ptr(modelMat));
        glBindVertexArray(model.vao);
        glDrawElements(GL_TRIANGLES, model.count, GL_UNSIGNED_INT, 0);
        glUniformMatrix4fv(glGetUniformLocation(depthShader, "model"), 1, GL_FALSE, value_ptr(modelMatMoon));
        glBindVertexArray(moon.vao);
        glDrawElements(GL_TRIANGLES, moon.count, GL_UNSIGNED_INT, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Second pass: normal render
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(shader, "lightSpaceMatrix"), 1, GL_FALSE, value_ptr(lightSpaceMatrix));
        glUniform3fv(glGetUniformLocation(shader, "lightColor"), 1, value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(shader, "viewPos"), 1, value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(shader, "lightPos"), 1, value_ptr(lightPos));
glUniform3fv(glGetUniformLocation(shader, "lightColor"), 1, value_ptr(lightColor));
        glUniform1i(glGetUniformLocation(shader, "showTexture"), showTexture ? 1 : 0);


        if (showTexture) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
        } else {
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(glGetUniformLocation(shader, "shadowMap"), 1);

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(modelMat));
        glBindVertexArray(model.vao);
        glDrawElements(GL_TRIANGLES, model.count, GL_UNSIGNED_INT, 0);

        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, value_ptr(modelMatMoon));

if (showTexture) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, moonTex);
    glUniform1i(glGetUniformLocation(shader, "texture1"), 0);
} else {
    glBindTexture(GL_TEXTURE_2D, 0);
}

glBindVertexArray(moon.vao);
glDrawElements(GL_TRIANGLES, moon.count, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
