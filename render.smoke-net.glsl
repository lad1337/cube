

#define ss(a, b, t) smoothstep(a, b, t)

float distLine(vec2 p, vec2 a, vec2 b) {  // ?
    vec2 pa = p - a;
    vec2 ba = b - a;
    float t = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);

    return length(pa - ba * t);
}

float n21(vec2 p) {
    p = fract(p * vec2(233.12, 852.53));
    p += dot(p, p + 23.53);

    return fract(p.x * p.y);
}

vec2 n22(vec2 p) {
    float n = n21(p);
    return vec2(n, n21(p + n));
}

vec2 getPos(vec2 id, vec2 offset) {
    vec2 n = n22(id + offset) * time;

    return offset + sin(n) * 0.4;
}

float line(vec2 p, vec2 a, vec2 b) {
    float d = distLine(p, a, b);
    float m = ss(0.03, 0.01, d);
    float d2 = length(a - b);

    m *= ss(1.6, .9, d2) + ss(.05, .03, abs(d2 - .75));

    return m;
}

float layer(vec2 uv) {
    vec2 gv = fract(uv) - 0.5;
    vec2 id = floor(uv);
    float result;

    vec2 center = getPos(id, vec2(0., 0.));

    for (float x = -1.0; x <= 1.0; x++) {
        for (float y = -1.0; y <= 1.0; y++) {
            vec2 point = getPos(id, vec2(x, y));

            result += line(gv, center, point);

            vec2 j = (point - gv) * 12.;
            float sparkle = 1. / dot(j, j);

            sparkle *= sin(time + fract(point.x) * 10.) * .5 + .5;

            result += sparkle;
        }
    }

    result += line(gv, getPos(id, vec2(-1, 0)), getPos(id, vec2(0, 1)));
    result += line(gv, getPos(id, vec2(0, 1)), getPos(id, vec2(1, 0)));
    result += line(gv, getPos(id, vec2(1, 0)), getPos(id, vec2(0, -1)));
    result += line(gv, getPos(id, vec2(0, -1)), getPos(id, vec2(-1, 0)));

    return result;
}

vec4 render_net(vec2 p) {
    // vec2 uv = (gl_FragCoord.xy - .5 * resolution.xy) / resolution.y;  // ?
    vec2 uv = p * 1.6;
    vec3 col = vec3(0.0);
    uv *= 5.;
    float l1 = layer(uv);
    col = vec3(l1);
    return vec4(col, 1.0);
}

float random(in vec2 point) {
    return fract(100.0 * sin(point.x + fract(100.0 * sin(point.y))));  // http://www.matteo-basei.it/noise
}

float noise(in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = random(i);
    float b = random(i + vec2(1., 0.));
    float c = random(i + vec2(0., 1.));
    float d = random(i + vec2(1., 1.));

    vec2 u = f * f * (3. - 2. * f);

    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

#define octaves 5

float fbm(in vec2 p) {
    float value = 0.;
    float freq = 1.;
    float amp = .5;

    for (int i = 0; i < octaves; i++) {
        value += amp * (noise((p - vec2(1.)) * freq));
        freq *= 1.9;
        amp *= .6;
    }

    return value;
}

float pattern(in vec2 p) {
    vec2 offset = vec2(-.5);

    vec2 aPos = vec2(sin(time * .05), sin(time * .1)) * 6.;
    vec2 aScale = vec2(3.);
    float a = fbm(p * aScale + aPos);

    vec2 bPos = vec2(sin(time * .1), sin(time * .1)) * 1.;
    vec2 bScale = vec2(.5);
    float b = fbm((p + a) * bScale + bPos);

    vec2 cPos = vec2(-.6, -.5) + vec2(sin(-time * .01), sin(time * .1)) * 2.;
    vec2 cScale = vec2(2.);
    float c = fbm((p + b) * cScale + cPos);

    return c;
}

vec3 palette(in float t) {
    vec3 a = vec3(.5 * (1. - load), .5 * (1. - download), .5 * (1. - upload));
    vec3 b = vec3(.5, .5, .5);

    vec3 c = vec3(1., 1., 1.);
    vec3 d = vec3(0., .2, 0);

    return a + b * cos(6.28318 * (c * t + d));
}

vec4 render_smoke(vec2 p) {
    float value = pow(pattern(p), 2.);
    vec3 color = palette(value);

    return vec4(color, 1.);
}

vec4 render(vec2 p) { return render_smoke(p) + vec4(render_net(p).rgb * 0.5, 1.0); }

