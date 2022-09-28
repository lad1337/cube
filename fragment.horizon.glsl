//#version 330 core
// mod of https://there.oughta.be/an/led-cube shader
// environment: 0 = sandbox, 1 = desktop, 2 = raspberry pi
#ifndef environment
precision mediump float;
//#define simulation // activate to change load etc automatically
#define environment 0
#endif

// code with errors to check the preprocessor
/*
#if environment == 2
raspi;
#elif environment == 1
mac;
#elif environment == 0
sandbox;
#else
wtf;
#endif
*/

uniform float time;
#if environment == 1
out vec4 FragColor;
in vec2 fragCoord;
#elif environment == 0
uniform vec2 resolution;
#else
varying vec2 fragCoord;
#endif

#if environment > 0
// all expected to the within 0.0 & 1.0
uniform float load;
uniform float download;
uniform float upload;
// last time data was updated in seconds
uniform float age;
#else
// all expected to the within 0.0 & 1.0
float load = 0.1;
float download = .0;
float upload = .0;
// last time data was updated in seconds
float age = .2;
#endif
#if environment == 1
uniform float p_factor;
#else
float p_factor = 0.5;
#endif
mat2 rot(float angle) {
    float c = cos(-angle);
    float s = sin(-angle);
    return mat2(c, -s, s, c);
}

vec4 render(vec2 p) {
    vec2 uv = p;
    vec3 dir = normalize(vec3(uv, 1.0));
    vec3 pos = vec3(0, 0, -3.);
    vec3 col = vec3(1.0);
    float t = 0.0;

    float tt = 2. + (sin(time) * 0.8) * (download + 1.);
    float sz = mix(.15, 0.4 * (1. / load), tt);
    vec3 N = vec3(0.0, 1.0, 0.0);
    N.xy *= rot(time * .1);
    N = normalize(N) * sz;

    t = -(5.0 - dot(dir, pos)) / dot(dir, N);
    if (t > 0.0) col = vec3(1, 2, 3) * t * 0.005;
    t = -(5.0 - dot(dir, pos)) / dot(dir, -N);
    if (t > 0.0) col = vec3(2, 1, 3.5) * t * 0.005;

    return vec4(clamp(col * 4.0, vec3(0.0), vec3(1.0)), 1.0);
}

void main(void) {
#if environment != 0
    vec2 p = fragCoord.xy * p_factor;
#else
    vec2 p = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
#endif

#ifdef simulation
    load = abs(sin(time * 0.2));
    download = abs(sin(time * 0.1));
    upload = abs(sin(time * 0.01));
#endif

#if environment == 1
    FragColor = render(p);
#else
    gl_FragColor = render(p);
#endif
}
