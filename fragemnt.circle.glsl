#version 330 core
out vec4 FragColor;
uniform vec2 resolution;
uniform float time;
in vec2 fragCoord;
uniform float p_factor;

float PI = radians(180.);  // So many people hardcode PI by typing out its digits. Why not use this instead?
vec3 circle(vec2 p) {
    float x = cos(time);
    float y = sin(time);
    float d0 = length(p + vec2(0, 0.0));
    float color = sin(10.0 * PI * d0 - time);
    return vec3(clamp(.0, color, 1.0));
}

void main() {
    vec2 p = fragCoord.xy * p_factor;  // - vec2(19.0);
    vec3 color = circle(p);
    FragColor = vec4(color, 1.);
}
