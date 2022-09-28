#version 330 core
out vec4 FragColor;

uniform vec3 color;
uniform mat4 mvpF;

void main() {
    vec3 cColor;
    if (0.0 == color.x) {
        cColor = vec3(1.0, 0.0, 0.0);
    } else {
        cColor = color;
    }
    if (mvpF[0].x == 0.0) {
        cColor.x = 0.5;
    }
    FragColor = vec4(cColor, 1.0);
}
