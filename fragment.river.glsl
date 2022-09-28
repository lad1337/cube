#version 330 core
// http://glslsandbox.com/e#72133.1
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
out vec4 FragColor;
in vec2 fragCoord;

uniform float time;
uniform vec2 resolution;
uniform float p_factor;
uniform float load;

float PI = radians(180.);  // So many people hardcode PI by typing out its digits. Why not use this instead?

void main() {
    // vec2 p = fragCoord.xy / vec2(max(resolution.x, resolution.y)) * 1.5;
    vec2 p = fragCoord.xy * p_factor;  // - vec2(19.0);
    float t = time / 2.5;

    float l = 0.0;
    for (float i = 1.0; i < 2.0; i++) {
        p.x += 0.1 / i * cos(i * 8.0 * p.y + t + sin(t / 75.0));
        p.y += 0.1 / i * sin(i * 12.0 * p.x + t + cos(t / 120.0));
        l = length(vec2(0, p.y + sin(p.x * PI * i * ((sin(t / 3.0) + cos(t / 2.0)) * 0.25 + 0.5))));
    }

    float g = 1.0 - pow(l, 0.9);
    // a.x 0->-5 makes black red: use for load
    // a.y -23->-2 makes torcoise thicker: use for download
    vec3 a = vec3(load * -5.0, -22.0, 150.0);
    vec3 b = vec3(0.0, -80.0, 0.0);
    vec3 c = vec3(0.0, -400.0, -800.0);

    // factor behind g-> higher makes color line thicker
    vec3 color = mix(a, mix(b, c, g / 2.0), g * 5.0);
    FragColor = vec4(color, 1.0);
}
