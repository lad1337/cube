#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <fstream>
#include <iostream>
#include <thread>

#include "led-matrix.h"

// Define interval for auto blanking - set to zero to disable
#define BLANKINTERVAL 0

// params
#define LOAD_MIN 1.0
#define LOAD_MAX 10.0
#define DOWNLOAD_MIN 0.0       // in kilobits/s
#define DOWNLOAD_MAX 102400.0  // in kilobits/s
#define UPLOAD_MIN 0.0         // in kilobits/s
#define UPLOAD_MAX 30720.0     // in kilobits/s

// UDP port to listen for status updates
#define PORT 1234

// Animation speed
#define ANIMSTEP 0.5

// Resolution, three panels with 64x64 each.
#define W 192
#define H 64

// That's it up here, but you might also want to change the settings for your RGB matrix in line 462 below
float normalize(float lower, float x, float higher);

// settings
float p_factor = 0.5f;
// lowest, current, max
float load = normalize(LOAD_MIN, LOAD_MIN, LOAD_MAX);
float download = normalize(DOWNLOAD_MIN, DOWNLOAD_MIN, DOWNLOAD_MAX);
float upload = normalize(UPLOAD_MIN, UPLOAD_MIN, UPLOAD_MAX);
bool on = true;  // homekit integration

float normalize(float lower, float x, float higher) { return (x - lower) / (higher - lower); }
float step(float lower, float higher) { return (higher - lower) / 20.0f; }

using rgb_matrix::Canvas;
using rgb_matrix::RGBMatrix;

using namespace rgb_matrix;

float t = 0.f;
float updateTime = -10.0f;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) { interrupt_received = true; }
static const std::string SHADER_HEADER = "#define environment 2\n";

static const EGLint configAttribs[] = {EGL_SURFACE_TYPE,
                                       EGL_PBUFFER_BIT,
                                       EGL_BLUE_SIZE,
                                       8,
                                       EGL_GREEN_SIZE,
                                       8,
                                       EGL_RED_SIZE,
                                       8,
                                       EGL_DEPTH_SIZE,
                                       8,

                                       EGL_SAMPLE_BUFFERS,
                                       1,
                                       EGL_SAMPLES,
                                       4,

                                       EGL_RENDERABLE_TYPE,
                                       EGL_OPENGL_ES2_BIT,
                                       EGL_NONE};

// Width and height of the desired framebuffer
static const EGLint pbufferAttribs[] = {
    EGL_WIDTH, W, EGL_HEIGHT, H, EGL_NONE,
};

static const EGLint contextAttribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};

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

    0.0,     -1.0f, -0.866,  -0.5f,

    0.0f,    0.0f,  0.0f,    -1.0f,

    0.866f,  0.5f,  0.866f,  -0.5f,

    0.0f,    0.0f,  0.866f,  0.5f,

    -0.866f, 0.5f,  0.0f,    1.0f,
};

std::string loadFile(std::string path) {
    std::cout << "loading file: " + path << std::endl;
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    if (content.length() <= 0) std::cout << "WARNING: content of " + path + "is 0" << std::endl;
    return content;
}

static const char *eglGetErrorStr() {
    switch (eglGetError()) {
        case EGL_SUCCESS:
            return "The last function succeeded without error.";
        case EGL_NOT_INITIALIZED:
            return "EGL is not initialized, or could not be initialized, for the "
                   "specified EGL display connection.";
        case EGL_BAD_ACCESS:
            return "EGL cannot access a requested resource (for example a context "
                   "is bound in another thread).";
        case EGL_BAD_ALLOC:
            return "EGL failed to allocate resources for the requested operation.";
        case EGL_BAD_ATTRIBUTE:
            return "An unrecognized attribute or attribute value was passed in the "
                   "attribute list.";
        case EGL_BAD_CONTEXT:
            return "An EGLContext argument does not name a valid EGL rendering "
                   "context.";
        case EGL_BAD_CONFIG:
            return "An EGLConfig argument does not name a valid EGL frame buffer "
                   "configuration.";
        case EGL_BAD_CURRENT_SURFACE:
            return "The current surface of the calling thread is a window, pixel "
                   "buffer or pixmap that is no longer valid.";
        case EGL_BAD_DISPLAY:
            return "An EGLDisplay argument does not name a valid EGL display "
                   "connection.";
        case EGL_BAD_SURFACE:
            return "An EGLSurface argument does not name a valid surface (window, "
                   "pixel buffer or pixmap) configured for GL rendering.";
        case EGL_BAD_MATCH:
            return "Arguments are inconsistent (for example, a valid context "
                   "requires buffers not supplied by a valid surface).";
        case EGL_BAD_PARAMETER:
            return "One or more argument values are invalid.";
        case EGL_BAD_NATIVE_PIXMAP:
            return "A NativePixmapType argument does not refer to a valid native "
                   "pixmap.";
        case EGL_BAD_NATIVE_WINDOW:
            return "A NativeWindowType argument does not refer to a valid native "
                   "window.";
        case EGL_CONTEXT_LOST:
            return "A power management event has occurred. The application must "
                   "destroy all contexts and reinitialise OpenGL ES state and "
                   "objects to continue rendering.";
        default:
            break;
    }
    return "Unknown error!";
}

std::string asString(const std::chrono::system_clock::time_point &tp) {
    // convert to system time:
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::string ts = std::ctime(&t);  // convert to calendar time
    ts.resize(ts.size() - 1);         // skip trailing newline
    return ts;
}

#define BUFLEN 512
void receiveUDP() {
    struct sockaddr_in si_me, si_other;
    int s, i, slen = sizeof(si_other);
    char buf[BUFLEN + 1];

    memset((char *)&si_me, 0, sizeof(si_me));
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000;

    while (!interrupt_received) {
        if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
            usleep(500000);
            printf("Error creating socket.\n");
            continue;
        }

        if (bind(s, (struct sockaddr *)&si_me, sizeof(si_me)) == -1) {
            usleep(500000);
            printf("Error binding socket.\n");
            continue;
        }

        if (setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            usleep(500000);
            printf("Error setting socket timeout.\n");
            continue;
        }

        while (!interrupt_received) {
            int nIn = recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *)&si_other, (socklen_t *)&slen);
            if (nIn <= 0) {
                usleep(0);
                continue;
            }

            updateTime = t;

            buf[nIn] = 0;
            int i = 0;
            int j = 0;
            while (i < nIn) {
                float entry = atof(buf + i);
                // std::cout << "e: " << entry << " @ " << j << std::endl;
                if (entry >= 0) switch (j) {
                        case 0:
                            load = normalize(LOAD_MIN, entry, LOAD_MAX);
                            break;
                        case 1:
                            download = normalize(DOWNLOAD_MIN, entry, DOWNLOAD_MAX);
                            break;
                        case 2:
                            upload = normalize(UPLOAD_MIN, entry, UPLOAD_MAX);
                            break;
                        case 3:
                            on = entry == 1.0f;
                            break;
                        default:
                            break;
                    }
                j++;
                while (buf[i] != ',' && i < nIn) i++;
                i++;
            }
        }

        close(s);
    }
}

bool checkShader(GLuint shader) {
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char errorLog[512];
        glGetShaderInfoLog(shader, 512, NULL, errorLog);
        fprintf(stderr, "Shader compilation failed!\n%s\n", errorLog);
        return false;
    }
    return true;
}
void pixel(int r, int g, int b) {
    float rf = (r / 255.0);
    float gf = (g / 255.0);
    float bf = (b / 255.0);
    int code = (int)std::round(36 * (rf * 5) + 6 * (gf * 5) + (bf * 5) + 16);
    printf("\033[1;48;5;%im \033[0m", code);
}

int main(int argc, char *argv[]) {
    std::string vertexPath = "vertex.original.glsl";
    std::string fragmentPath = "fragment.theroutamod.glsl";
    EGLDisplay display;
    bool debug = false;
    bool print = false;
    int major, minor;
    int desiredWidth, desiredHeight;
    GLuint program, vert, frag, vbo, vbocoord;
    int pos_id, fragcoord_id, load_id, upload_id, download_id, age_id, time_id;

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            std::string option = std::string(argv[i]);
            if (option == "-p")
                print = true;
            else if (option == "-d")
                debug = true;
            else if (option == "--fragment") {
                fragmentPath = std::string(argv[i++]);
            } else if (option == "--vertex") {
                vertexPath = std::string(argv[i++]);
            }
        }
    }

    if ((display = eglGetDisplay(EGL_DEFAULT_DISPLAY)) == EGL_NO_DISPLAY) {
        fprintf(stderr, "Failed to get EGL display! Error: %s\n", eglGetErrorStr());
        return EXIT_FAILURE;
    }

    if (eglInitialize(display, &major, &minor) == EGL_FALSE) {
        fprintf(stderr, "Failed to get EGL version! Error: %s\n", eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    printf("Initialized EGL version: %d.%d\n", major, minor);

    EGLint numConfigs;
    EGLConfig config;
    if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs)) {
        fprintf(stderr, "Failed to get EGL config! Error: %s\n", eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
    if (surface == EGL_NO_SURFACE) {
        fprintf(stderr, "Failed to create EGL surface! Error: %s\n", eglGetErrorStr());
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    eglBindAPI(EGL_OPENGL_API);

    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
    if (context == EGL_NO_CONTEXT) {
        fprintf(stderr, "Failed to create EGL context! Error: %s\n", eglGetErrorStr());
        eglDestroySurface(display, surface);
        eglTerminate(display);
        return EXIT_FAILURE;
    }

    eglMakeCurrent(display, surface, surface, context);

    desiredWidth = pbufferAttribs[1];
    desiredHeight = pbufferAttribs[3];

    glViewport(0, 0, desiredWidth, desiredHeight);

    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);
    printf("GL Viewport size: %dx%d\n", viewport[2], viewport[3]);

    if (desiredWidth != viewport[2] || desiredHeight != viewport[3]) {
        fprintf(stderr,
                "Error! The glViewport/glGetIntegerv are not working! "
                "EGL might be faulty!\n");
    }

    // Clear whole screen (front buffer)
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Shader program
    std::string vertexCode = SHADER_HEADER + loadFile(vertexPath);
    std::string fragmentCode = SHADER_HEADER + loadFile(fragmentPath);
    static const char *cVertexCode = vertexCode.c_str();
    static const char *cFragmentCode = fragmentCode.c_str();

    program = glCreateProgram();
    glUseProgram(program);

    vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &cVertexCode, NULL);
    glCompileShader(vert);
    if (!checkShader(vert)) return EXIT_FAILURE;

    frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &cFragmentCode, NULL);
    glCompileShader(frag);
    if (!checkShader(frag)) return EXIT_FAILURE;

    glAttachShader(program, frag);
    glAttachShader(program, vert);
    glLinkProgram(program);
    // https://www.khronos.org/opengl/wiki/Example/GLSL_Program_Link_Error_Testing
    GLint isLinked = 0;
    glGetProgramiv(program, GL_LINK_STATUS, &isLinked);
    if (isLinked == GL_FALSE) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cout << infoLog << std::endl;
        glDeleteProgram(program);
        return EXIT_FAILURE;
    }
    glUseProgram(program);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(float), vertices, GL_STATIC_DRAW);

    glGenBuffers(1, &vbocoord);
    glBindBuffer(GL_ARRAY_BUFFER, vbocoord);
    glBufferData(GL_ARRAY_BUFFER, 24 * sizeof(float), vcoords, GL_STATIC_DRAW);

    // Get vertex attribute and uniform locations
    pos_id = glGetAttribLocation(program, "aPos");
    fragcoord_id = glGetAttribLocation(program, "coord");

    load_id = glGetUniformLocation(program, "load");
    upload_id = glGetUniformLocation(program, "upload");
    download_id = glGetUniformLocation(program, "download");
    time_id = glGetUniformLocation(program, "time");
    age_id = glGetUniformLocation(program, "age");

    if (debug) {
        std::cout << "Location of position: " << pos_id << std::endl;
        std::cout << "Location of fragecoord: " << fragcoord_id << std::endl;
        std::cout << "Location of load: " << load_id << std::endl;
        std::cout << "Location of upload: " << upload_id << std::endl;
        std::cout << "Location of download: " << download_id << std::endl;
        std::cout << "Location of time: " << time_id << std::endl;
        std::cout << "Location of age: " << age_id << std::endl;
    }

    // Set our vertex data
    glEnableVertexAttribArray(pos_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(pos_id, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    glEnableVertexAttribArray(fragcoord_id);
    glBindBuffer(GL_ARRAY_BUFFER, vbocoord);
    glVertexAttribPointer(fragcoord_id, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void *)0);

    // LED Matrix settings
    RGBMatrix::Options defaults;
    rgb_matrix::RuntimeOptions runtime;
    defaults.hardware_mapping = "adafruit-hat-pwm";
    defaults.led_rgb_sequence = "RGB";
    defaults.pwm_bits = 11;
    defaults.pwm_lsb_nanoseconds = 50;
    defaults.panel_type = "FM6126A";
    defaults.rows = 64;
    defaults.cols = 192;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    //  defaults.brightness = 60;

    runtime.drop_privileges = 0;
    runtime.gpio_slowdown = 1;
    RGBMatrix *matrix = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults, &runtime);
    if (matrix == NULL) return EXIT_FAILURE;
    FrameCanvas *canvas = matrix->CreateFrameCanvas();

    signal(SIGTERM, InterruptHandler);
    signal(SIGINT, InterruptHandler);

    unsigned char *buffer = (unsigned char *)malloc(W * H * 3);

    std::thread networking(receiveUDP);

    int lastTime = (int)std::time(NULL);
    int nbFrames = 0;

    while (!interrupt_received) {
        t += 0.01f;

        if (print) {
            printf("\033c");
            sleep(1);
        }

        float age = float(t - updateTime);
        if (debug) {
            int currentTime = (int)std::time(NULL);
            nbFrames++;
            if ((currentTime - lastTime) >= 1.0) {  // If last prinf() was more than 1 sec ago
                printf("%f ms/frame -> %i/s | time: %f | age: %f | load: %f | down: %f | up: %f | on: %i\n",
                       1000.0 / float(nbFrames), nbFrames, t, age, load, download, upload, on);
                nbFrames = 0;
                lastTime += 1;
            }
        }
        if (!on) {
            usleep(1000);
            continue;
        }

        // Grab the interval since last update
        int quiet = int(t - updateTime);

        if ((quiet > BLANKINTERVAL) && (BLANKINTERVAL > 0)) {
            canvas->Clear();
            canvas = matrix->SwapOnVSync(canvas);
            sleep(5);
        } else {
            glUniform1f(time_id, t);
            glUniform1f(age_id, age);
            glUniform1f(load_id, load);
            glUniform1f(download_id, download);
            glUniform1f(upload_id, upload);

            // ACTION
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 12);
            glReadPixels(0, 0, W, H, GL_RGB, GL_UNSIGNED_BYTE, buffer);
            for (int x = 0; x < W; x++) {
                for (int y = 0; y < H; y++) {
                    int index = 3 * (x + y * W);
                    canvas->SetPixel(x, y, buffer[index], buffer[index + 1], buffer[index + 2]);
                    if (x % 2 == 0 && print) pixel(buffer[index], buffer[index + 1], buffer[index + 2]);
                }
                if (x % 2 == 0 && print) std::cout << std::endl;
            }
            canvas = matrix->SwapOnVSync(canvas);
        }
    }

    networking.join();

    free(buffer);
    canvas->Clear();

    // Cleanup
    eglDestroyContext(display, context);
    eglDestroySurface(display, surface);
    eglTerminate(display);

    return EXIT_SUCCESS;
}
