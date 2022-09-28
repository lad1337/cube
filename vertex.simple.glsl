#version 330 core
layout(location = 0) in vec3 pos;
// Values that stay constant for the whole mesh.
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 coord;
out vec2 fragCoord;

void main() {
    fragCoord = coord;
    gl_Position = projection * view * model * vec4(pos, 1.0f);
}
