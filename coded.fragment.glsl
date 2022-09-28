const int CORES = 12;
const float T1 = 40.0;
const float T2 = 60.0;
const float T3 = 80.0;
uniform float temperature;
uniform float cpu[CORES];
uniform float age;
uniform float time;
varying vec2 fragCoord;

vec3 mcolor = vec3(0.1, 0.6, 0.4);
vec3 mcolorwarm = vec3(0.5, 0.5, 0.1);
vec3 mcolorhot = vec3(0.6, 0.2, 0.1);
vec3 ccolor = vec3(0.7, 1.0, 0.9);
vec3 ccolorwarm = vec3(1.0, 1.0, 0.6);
vec3 ccolorhot = vec3(1.0, 1.0, 1.0);
float phi;
float cpuf = 0.0;
const int num_x = 5;
const int num_y = 5;
float w = 600.0;
float h = 600.0;

vec4 draw_ball(int i, int j, float temperature) {
    float t = time * 0.2 + (temperature / 10.0);
    float x = w / 2.0 * (1.0 + cos(1.5 * t + float(2 * i + 4 * j)));
    float y = h / 2.0 * (1.0 + sin(2.3 * t + float(3 * i + 4 * j)));
    float size = 3.0 - 2.0 * sin(t);
    vec2 pos = vec2(x, y);
    float dist = length(gl_FragCoord.xy - pos);
    float intensity = pow(size / (dist / 2.0), 2.0);
    vec4 color = vec4(0.0);
    color.r = 0.5 + cos(t * float(i));
    color.g = 0.5 + sin(t * float(j));
    color.b = 0.5 + sin(float(j));
    return color * intensity;
}

float circle(vec2 uv, float w0, float width) {
    float f = length(uv) + (sin(normalize(uv).y * 5.0 + time * 2.0) -
                            sin(normalize(uv).x * 5.0 + time * 2.0)) /
                               100.0;
    float w = width + width * cpuf * 0.1;
    return smoothstep(w0 - w, w0, f) - smoothstep(w0, w0 + w, f);
}

void main() {
    vec2 coords = fragCoord.xy * 0.5;
    float phi = (atan(coords.y, coords.x) + 3.1415926538) / 3.1415926538 *
                float(CORES) * 0.5;
    cpuf += clamp(1.0 - abs(phi - 0.0), 0.0, 1.0) * cpu[0];
    cpuf += clamp(1.0 - abs(phi - 1.0), 0.0, 1.0) * cpu[1];
    cpuf += clamp(1.0 - abs(phi - 2.0), 0.0, 1.0) * cpu[2];
    cpuf += clamp(1.0 - abs(phi - 3.0), 0.0, 1.0) * cpu[3];
    cpuf += clamp(1.0 - abs(phi - 4.0), 0.0, 1.0) * cpu[4];
    cpuf += clamp(1.0 - abs(phi - 5.0), 0.0, 1.0) * cpu[5];
    cpuf += clamp(1.0 - abs(phi - 6.0), 0.0, 1.0) * cpu[6];
    cpuf += clamp(1.0 - abs(phi - 7.0), 0.0, 1.0) * cpu[7];
    cpuf += clamp(1.0 - abs(phi - 8.0), 0.0, 1.0) * cpu[8];
    cpuf += clamp(1.0 - abs(phi - 9.0), 0.0, 1.0) * cpu[9];
    cpuf += clamp(1.0 - abs(phi - 10.0), 0.0, 1.0) * cpu[10];
    cpuf += clamp(1.0 - abs(phi - 11.0), 0.0, 1.0) * cpu[11];
    cpuf += clamp(1.0 - abs(phi - 12.0), 0.0, 1.0) * cpu[0];
    vec2 p = fragCoord.xy * 0.5 * 10.0 - vec2(19.0);
    vec2 i = p;
    float c = 1.0;
    float inten = 0.05;
    for (int n = 0; n < 8; n++) {
        float t = time * (0.7 - (0.2 / float(n + 1)));
        i = p + vec2(cos(t - i.x) + sin(t + i.y), sin(t - i.y) + cos(t + i.x));
        c += 1.0 / length(vec2(p.x / (2.0 * sin(i.x + t) / inten),
                               p.y / (cos(i.y + t) / inten)));
    }
    c /= 8.0;
    c = 1.5 - sqrt(pow(c, 2.0));
    mcolor.g = clamp(coords.x, 0.0, 1.0);
    mcolor = smoothstep(T2, T1, temperature) * mcolor +
             smoothstep(T1, T2, temperature) * smoothstep(T3, T2, temperature) *
                 mcolorwarm +
             smoothstep(T2, T3, temperature) * mcolorhot;
    ccolor = smoothstep(50.0, 0.0, cpuf) * ccolor +
             smoothstep(0.0, 50.0, cpuf) * smoothstep(100.0, 50.0, cpuf) *
                 ccolorwarm +
             smoothstep(50.0, 100.0, cpuf) * ccolorhot;
    ccolor *= circle(coords, 0.25, 0.01);

    vec3 outcolor = mcolor * c * c * c * c + ccolor;
    vec3 grayXfer = vec3(0.3, 0.59, 0.11);
    vec3 gray = vec3(dot(grayXfer, outcolor));

    vec4 balls = vec4(0.0);
    for (int i = 0; i < num_x; ++i) {
        for (int j = 0; j < num_y; ++j) {
            balls += draw_ball(i, j, temperature);
        }
    }
    gl_FragColor =
        vec4(mix(outcolor, gray, smoothstep(5.0, 10.0, age)), 1.0) + balls;
}
