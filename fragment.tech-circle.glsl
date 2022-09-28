#version 330 core
// http://glslsandbox.com/e#72327.0
out vec4 FragColor;
in vec2 fragCoord;

precision mediump float;
uniform float time;
uniform vec2 resolution;
uniform float p_factor;
uniform float load;

#define PI 3.14
#define PI2 2.0 * PI

float circle(vec2 uv, vec2 o, float r) { return clamp(abs(length((uv - o)) - r), 0., 1.); }

float segment(vec2 uv, vec2 p1, vec2 p2, float r) {
    vec2 d = p2 - p1;
    float t = clamp(dot(uv - p1, d) / max(0.001, d.x * d.x + d.y * d.y), 0., 1.);
    vec2 proj = p1 + t * (p2 - p1);
    return clamp(abs(length(proj - uv) - r), 0., 1.);
}

float drawCircles(vec2 uv) {
    float v = 0.;
    vec2 ar = vec2(0.5, 0.5);
    v += pow(1. - circle(uv, vec2(0.), 0.6), 32.);
    v += pow(1. - circle(uv, vec2(0.), 0.45), 128.);
    v += pow(1. - circle(uv, vec2(0.), 0.3), 64.);
    for (int i = 0; i < 4; ++i) {
        float x = cos(float(i) * 0.25 * PI2 + time);
        float y = sin(float(i) * 0.25 * PI2 + time);
        v += pow(1. - segment(uv, 0.3 * vec2(x, y), 0.63 * vec2(x, y), 0.001), 64.);
    }
    for (int i = 0; i < 4; ++i) {
        float x = cos(float(i) * 0.25 * PI2 + PI * 0.25 + time);
        float y = sin(float(i) * 0.25 * PI2 + PI * 0.25 + time);
        v += pow(1. - segment(uv, 0.25 * vec2(x, y), 0.6 * vec2(x, y), 0.001), 128.);
        v += pow(1. - circle(uv, 0.235 * vec2(x, y), 0.01), 256.);
    }
    float az = acos(dot(normalize(uv), vec2(0., 1.)));
    v += (sin(4. * az + PI * 1.5 + time * 4.0) > 0. ? 1. : 0.) * pow(1. - circle(uv, vec2(0.0), 0.57), 64.);
    for (int i = 0; i < 6; ++i) {
        float x = cos(float(i) * 0.125 * PI2 + PI * 0.125 + time);
        float y = sin(float(i) * 0.125 * PI2 + PI * 0.125 + time);
        v += pow(1. - segment(uv, 0.63 * vec2(x, y), 0.57 * vec2(x, y), 0.001), 256.);
    }
    az = acos(dot(normalize(uv), vec2(0., 1.)));
    v += (cos(4. * az + time * 16.0) > 0. ? 1. : 0.) * pow(1. - circle(uv, vec2(0.), 0.33), 128.);
    for (int i = 0; i < 8; ++i) {
        float x = cos(float(i) * 0.125 * PI2 + PI * 0.125 - time);
        float y = sin(float(i) * 0.125 * PI2 + PI * 0.125 - time);
        v += pow(1. - segment(uv, 0.36 * vec2(x, y), 0.3 * vec2(x, y), 0.001), 256.);
    }
    for (int i = 0; i < 4; ++i) {
        float x = cos(float(i) * 0.25 * PI2 + PI * 0.135 - time);
        float y = sin(float(i) * 0.25 * PI2 + PI * 0.135 - time);
        v += pow(1. - circle(uv, 0.585 * vec2(x, y), 0.01), 256.);
        x = cos(float(i) * 0.25 * PI2 + PI * 0.365);
        y = sin(float(i) * 0.25 * PI2 + PI * 0.365);
        v += pow(1. - circle(uv, 0.585 * vec2(x, y), 0.01), 256.);
    }
    for (int i = 0; i < 4; ++i) {
        float x = cos(float(i) * 0.25 * PI2 + PI * 0.11 - time);
        float y = sin(float(i) * 0.25 * PI2 + PI * 0.11 - time);
        v += pow(1. - circle(uv, 0.315 * vec2(x, y), 0.01), 256.);
        x = cos(float(i) * 0.25 * PI2 + PI * 0.39 - time);
        y = sin(float(i) * 0.25 * PI2 + PI * 0.385 - time);
        v += pow(1. - circle(uv, 0.315 * vec2(x, y), 0.01), 256.);
    }
    return v;
}

void main(void) {
    vec2 uv = fragCoord.xy / resolution.xy;
    uv.x *= resolution.x / resolution.y;

    // the vec2 at the front is the position
    // the vec2 at the back is the size
    vec2 cuv = (uv - vec2(1.0, 0.5)) * vec2(4.0, 4.0);

    // the vec4 is the actual color!
    FragColor = vec4(1., 1.0, 0.5, 0.6) * drawCircles(cuv);
}
