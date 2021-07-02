/* Defined BEFORE OpenGL and GLUT includes to avoid deprecation messages */
#define GL_SILENCE_DEPRECATION
/* Ask for an OpenGL Core Context */
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <stdio.h>

#include <fstream>
#include <iostream>

bool for_display = true;
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);
float normalize(float lower, float x, float higher);

// settings
const unsigned int SCR_WIDTH = 3 * 64;
const unsigned int SCR_HEIGHT = 1 * 64;
const float RESOLUTION[2] = {float(SCR_WIDTH), float(SCR_HEIGHT)};

float p_factor = 0.5f;
// lowest, current, max
float dimension1[3] = {1.0f, 3.0f, 8.0f};
float dimension2[3] = {0.0f, 80.0f, 5000.0f};
float dimension3[3] = {0.0f, 80.0f, 500.0f};

std::string loadFile(std::string path) {
    std::cout << "loading file: " + path << std::endl;
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    if (content.length() <= 0) std::cout << "WARNING: content of " + path + "is 0" << std::endl;
    return content;
}

float normalize(float lower, float x, float higher) { return (x - lower) / (higher - lower); }
float step(float lower, float higher) { return (higher - lower) / 20.0f; }
float normalized_load() { return normalize(dimension1[0], dimension1[1], dimension1[2]); }
float normalized_download() { return normalize(dimension2[0], dimension2[1], dimension2[2]); }
float normalized_upload() { return normalize(dimension3[0], dimension3[1], dimension3[2]); }

static const std::string SHADER_HEADER = "#version 330 core\n#define environment 1\n";

static const GLfloat vertices[] = {
    -1.0f,
    -1.0f,
    0.0f,
    -1.0f,
    1.0f,
    0.0f,

    -0.33333333333f,
    -1.0f,
    0.0f,
    -0.33333333333f,
    1.0f,
    0.0f,

    -0.33333333333f,
    -1.0f,
    0.0f,
    -0.33333333333f,
    1.0f,
    0.0f,

    0.33333333333f,
    -1.0f,
    0.0f,
    0.33333333333f,
    1.0f,
    0.0f,

    0.33333333333f,
    -1.0f,
    0.0f,
    0.33333333333f,
    1.0f,
    0.0f,

    1.0f,
    -1.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
};

static const GLfloat vcoords[] = {
    0.0f,    0.0f,  -0.866f, 0.5f,

    0.0f,    -1.0f, -0.866f, -0.5f,

    0.0f,    0.0f,  0.0f,    -1.0f,

    0.866f,  0.5f,  0.866f,  -0.5f,

    0.0f,    0.0f,  0.866f,  0.5f,

    -0.866f, 0.5f,  0.0f,    1.0f,
};

void set_fragment_xy(bool for_display) {
    if (for_display) {
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
        std::cout << "Mode: normal screen" << std::endl;
    } else {
        glBufferData(GL_ARRAY_BUFFER, sizeof(vcoords), vcoords, GL_STATIC_DRAW);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);
        std::cout << "Mode: rgb matrix on the cube" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    std::string vertexPath = "vertex.original.glsl";
    std::string fragmentPath = "fragemnt.original.glsl";
    if (argc >= 2) {
        fragmentPath = argv[1];
        if (argc >= 3) vertexPath = argv[2];
    }
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetKeyCallback(window, key_callback);

    // build and compile our shader program
    // ------------------------------------
    std::string vertexCode = SHADER_HEADER + loadFile(vertexPath);
    std::string fragmentCode = SHADER_HEADER + loadFile(fragmentPath);
    static const char *cVertexCode = vertexCode.c_str();
    static const char *cFragmentShaderCode = fragmentCode.c_str();
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &cVertexCode, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 1;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &cFragmentShaderCode, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 1;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 1;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int VBO, VAO, VBO2;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &VBO2);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBO2);
    set_fragment_xy(for_display);
    glEnableVertexAttribArray(1);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound
    // vertex buffer object so afterwards we can safely unbind
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the
    // VAO; keep the EBO bound. glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely
    // happens. Modifying other VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs
    // (nor VBOs) when it's not directly necessary.
    // glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // render loop
    // -----------
    int load_id = glGetUniformLocation(shaderProgram, "load");
    int upload_id = glGetUniformLocation(shaderProgram, "upload");
    int download_id = glGetUniformLocation(shaderProgram, "download");

    int time_id = glGetUniformLocation(shaderProgram, "time");
    int age_id = glGetUniformLocation(shaderProgram, "age");
    int resolution_id = glGetUniformLocation(shaderProgram, "resolution");
    int p_factor_id = glGetUniformLocation(shaderProgram, "p_factor");
    std::cout << "Location of p_factor: " << p_factor_id << std::endl;

    float t;
    glBindVertexArray(VAO);

    int lastTime = (int)std::time(NULL);
    int nbFrames = 0;
    while (!glfwWindowShouldClose(window)) {
        int currentTime = (int)std::time(NULL);
        nbFrames++;
        if ((currentTime - lastTime) >= 1.0) {  // If last prinf() was more than 1 sec ago
            // printf and reset timer
            printf("%f ms/frame\n", 1000.0 / float(nbFrames));
            nbFrames = 0;
            lastTime += 1;
        }
        // input
        // -----
        // processInput(window);
        t += 0.01f;
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

        // render
        // ------
        glUniform2fv(resolution_id, 1, RESOLUTION);
        glUniform1f(p_factor_id, p_factor);

        glClear(GL_COLOR_BUFFER_BIT);
        glUniform1f(time_id, t);
        glUniform1f(age_id, 2.0f);

        glUniform1f(load_id, normalized_load());
        glUniform1f(upload_id, normalized_upload());
        glUniform1f(download_id, normalized_download());

        // draw our first triangle
        glUseProgram(shaderProgram);
        // do so to keep things a bit more organized
        //        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 12);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &VBO2);
    glDeleteProgram(shaderProgram);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS) {
        for_display = !for_display;
        set_fragment_xy(for_display);
    }
};

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (action == GLFW_RELEASE) return;  // only handle press events
    if (key == GLFW_KEY_M) {
        for_display = !for_display;
        set_fragment_xy(for_display);
    }
    if (key == GLFW_KEY_K) {
        // up
        p_factor += 0.1f;
    }
    if (key == GLFW_KEY_J) {
        // down
        p_factor -= 0.1f;
    }
    if (key == GLFW_KEY_H) {
        // lower load
        dimension1[1] -= step(dimension1[0], dimension1[2]);
    }
    if (key == GLFW_KEY_L) {
        // higher load
        dimension1[1] += step(dimension1[0], dimension1[2]);
    }
    std::cout << "p_factor: " << p_factor << " load: " << dimension1[1] << "(" << normalized_load() << ")" << std::endl;
}

