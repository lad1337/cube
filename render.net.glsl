
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

vec4 render(vec2 p) {
    // vec2 uv = (gl_FragCoord.xy - .5 * resolution.xy) / resolution.y;  // ?
    vec2 uv = p * 1.6;
    vec3 col = vec3(0.0);
    uv *= 5.;
    float l1 = layer(uv);
    col = vec3(l1);
    return vec4(col, 1.0);
}
