//#version 330 core
// mod of https://there.oughta.be/an/led-cube shader
// environment: 0 = sandbox, 1 = desktop, 2 = raspberry pi
#ifndef environment
precision mediump float;
//#define simulation // activate to change load etc automatically
#define environment 0
#endif

uniform float time;
#if environment == 1
out vec4 FragColor;
in vec2 fragCoord;
#elif environment == 0
uniform vec2 resolution;
#else
varying vec2 fragCoord;
#endif
#if environment == 1
uniform float p_factor;
#else
float p_factor = 0.5;
#endif

void main(void) {
#if environment != 0
    vec2 p = fragCoord.xy * p_factor;
#else
    vec2 p = (gl_FragCoord.xy - 0.5 * resolution.xy) / resolution.y;
#endif

    vec3 final_color = vec3(0.0);
    final_color.r = sin(p.x);
    final_color.g = sin(p.y);

#if environment == 1
    FragColor = vec4(final_color, 1.0);
#else
    gl_FragColor = vec4(final_color, 1.0);
#endif
}
