#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <netinet/in.h>
#include <signal.h>
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
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include "led-matrix.h"

// Define interval for auto blanking - set to zero to disable
#define BLANKINTERVAL 0

// UDP port to listen for status updates
#define PORT 1234

// Animation speed
#define ANIMSTEP 0.001f

// Resolution, three panels with 64x64 each.
#define W 192
#define H 64

// That's it up here, but you might also want to change the settings for your RGB matrix in line 462 below
float normalize(float lower, float x, float higher);

// settings
float p_factor = 0.5f;
// lowest, current, max
float dimension1 = 0.0f;
float dimension2 = 0.0f;
float dimension3 = 0.0f;
bool on = true;  // homekit integration
unsigned int brightness = 100;
int debug = 0;
bool debug_network = false;

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

#define BUFLEN 128
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
                float entryf = atof(buf + i);
                int entryi = atoi(buf + i);
                // std::cout << "e: " << entry << " @ " << j << std::endl;
                if (entryi >= 0) switch (j) {
                        case 0:
                            dimension1 = entryf;
                            break;
                        case 1:
                            dimension2 = entryf;
                            break;
                        case 2:
                            dimension3 = entryf;
                            break;
                        case 3:
                            on = entryi == 1;
                            break;
                        case 4:
                            brightness = entryi;
                            break;
                        default:
                            break;
                    }
                j++;
                while (buf[i] != ',' && i < nIn) i++;
                i++;
            }
            if (debug >= 2 && debug_network) {
                printf("dim1: %4.3f | dim2: %4.3f | dim3: %4.3f | on: %i | brightness: %i\n", dimension1, dimension2,
                       dimension3, on, brightness);
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
    std::string fragmentPath = "fragment.template.glsl";
    std::string renderFunctionPath = "render.smoke2.glsl";
    EGLDisplay display;
    bool print = false;
    int major, minor;
    int desiredWidth, desiredHeight;
    GLuint program, vert, frag, vbo, vbocoord;
    int pos_id, fragcoord_id, dimension1_loc, dimension2_loc, dimension3_loc, age_id, time_id;

    if (argc >= 2) {
        for (int i = 1; i < argc; i++) {
            std::string option = std::string(argv[i]);
            if (option == "-p")
                print = true;
            else if (option.rfind("-v", 0) == 0)
                debug = option.length() - 1;
            else if (option == "--fragment") {
                fragmentPath = std::string(argv[i + 1]);
            } else if (option == "--vertex") {
                vertexPath = std::string(argv[i + 1]);
            } else if (option == "--render") {
                renderFunctionPath = std::string(argv[i + 1]);
            } else if (option == "--debug-network") {
                debug_network = true;
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
    if (renderFunctionPath.length() > 0) {
        std::string functionBody = loadFile(renderFunctionPath);
        std::size_t found = fragmentCode.find("// HERE");
        if (found == std::string::npos) {
            std::cout << "not found" << std::endl;
            return EXIT_FAILURE;
        }
        fragmentCode = fragmentCode.insert(found, functionBody);
    }
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
        std::cout << fragmentCode << std::endl;
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

    dimension1_loc = glGetUniformLocation(program, "load");
    dimension2_loc = glGetUniformLocation(program, "upload");
    dimension3_loc = glGetUniformLocation(program, "download");
    time_id = glGetUniformLocation(program, "time");
    age_id = glGetUniformLocation(program, "age");

    if (debug >= 1) {
        printf("Location of position: %d\n", pos_id);
        printf("Location of fragecoord: %d\n", fragcoord_id);
        printf("Location of load: %d\n", dimension1_loc);
        printf("Location of upload: %d\n", dimension2_loc);
        printf("Location of download: %d\n", dimension3_loc);
        printf("Location of time: %d\n", time_id);
        printf("Location of age: %d\n", age_id);

        // printf("load: %d | %f | %d\n", LOAD_MIN, dimension1, LOAD_MAX);
        // printf("down: %d | %f | %d\n", DOWNLOAD_MIN, dimension2, DOWNLOAD_MAX);
        // printf("up:   %d | %f | %d\n", UPLOAD_MIN, dimension3, UPLOAD_MAX);
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
    runtime.daemon = -1;
    defaults.limit_refresh_rate_hz = 120;
    defaults.pwm_bits = 11;
    // defaults.pwm_lsb_nanoseconds = 226; // old default, for raspberry
    defaults.pwm_lsb_nanoseconds = 70;  // no flicker, higher refresh, on dietpi
    defaults.hardware_mapping = "adafruit-hat-pwm";
    defaults.led_rgb_sequence = "BGR";
    defaults.panel_type = "FM6126A";
    defaults.rows = 64;
    defaults.cols = 192;
    defaults.chain_length = 1;
    defaults.parallel = 1;
    if (debug == 2) {
        defaults.show_refresh_rate = true;
    }
    // defaults.brightness = 60;

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

    matrix->StartRefresh();
    bool last_frame_on = true;

    float effective_dimension1 = 0.0f;
    float effective_dimension2 = 0.0f;
    float effective_dimension3 = 0.0f;
    printf("Rendering!\n");
    while (!interrupt_received) {
        t += 0.01f;

        if (print) {
            printf("\033c");
            sleep(1);
        }

        if (dimension1 > effective_dimension1)
            effective_dimension1 += ANIMSTEP;
        else if (dimension1 < effective_dimension1)
            effective_dimension1 -= ANIMSTEP;

        if (dimension2 > effective_dimension2)
            effective_dimension2 += ANIMSTEP;
        else if (dimension2 < effective_dimension2)
            effective_dimension2 -= ANIMSTEP;

        if (dimension3 > effective_dimension3)
            effective_dimension3 += ANIMSTEP;
        else if (dimension3 < effective_dimension3)
            effective_dimension3 -= ANIMSTEP;

        float age = float(t - updateTime);
        if (debug >= 3) {
            int currentTime = (int)std::time(NULL);
            nbFrames++;
            if ((currentTime - lastTime) >= 1.0) {  // If last prinf() was more than 1 sec ago
                printf(
                    "%f ms/frame -> %i/s | time: %4.3f | age: %4.3f | dim1: %4.3f | dim2: %4.3f | dim3: %4.3f | on: "
                    "%d\n",
                    1000.0 / float(nbFrames), nbFrames, t, age, effective_dimension1, effective_dimension2,
                    effective_dimension3, on);
                nbFrames = 0;
                lastTime += 1;
            }
        }
        // skip if we are off
        if (!on) {
            if (last_frame_on) {
                printf("Killing matrix and creating a new one\n");
                canvas->Clear();
                delete matrix;
                matrix = rgb_matrix::CreateMatrixFromFlags(&argc, &argv, &defaults, &runtime);
                if (matrix == NULL) return EXIT_FAILURE;
                canvas = matrix->CreateFrameCanvas();
            }
            last_frame_on = false;
            usleep(1000);
            continue;
        }

        if (!last_frame_on) matrix->StartRefresh();
        last_frame_on = true;

        // Grab the interval since last update
        int quiet = int(t - updateTime);

        if ((quiet > BLANKINTERVAL) && (BLANKINTERVAL > 0)) {
            canvas->Clear();
            canvas = matrix->SwapOnVSync(canvas);
            sleep(5);
        } else {
            matrix->SetBrightness(brightness);
            glUniform1f(time_id, t);
            glUniform1f(age_id, age);
            glUniform1f(dimension1_loc, effective_dimension1);
            glUniform1f(dimension3_loc, effective_dimension2);
            glUniform1f(dimension2_loc, effective_dimension3);

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
