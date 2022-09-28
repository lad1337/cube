// http://glslsandbox.com/e#72744.0
// Lightning
// By: Brandon Fogerty
// bfogerty at gmail dot com
// xdpixel.com

#ifdef GL_ES
precision mediump float;
#endif

const float count = 3.0;

float Hash(vec2 p, float s) {
    vec3 p2 = vec3(p.xy, 360.0 * abs(sin(s)));
    return fract(sin(dot(p2, vec3(27.1, 61.7, 12.4))) * 273758.5453123);
}

float noise(vec2 p, float s) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f *= f * f * (3.0 - 2.0 * f);

    return mix(mix(Hash(i + vec2(0., 0.), s), Hash(i + vec2(1., 0.), s), f.x),
               mix(Hash(i + vec2(0., 1.), s), Hash(i + vec2(1., 1.), s), f.x), f.y) *
           s;
}

float fbm(vec2 p) {
    float v = 0.0;
    v -= noise(p * .5, 0.35);
    v -= noise(p * 2., 0.25);
    v -= noise(p * 2., 0.25);
    v -= noise(p * 2., 0.25);
    v -= noise(p * 4., 0.125);
    v -= noise(p * 8., 0.0625);
    return v;
}

vec4 render(vec2 p) {
    vec2 uv = p * 6.;

    float timeMult = .2;
    vec3 finalColor = vec3(0.0);
    float factor = uv.x + sin(uv.y + sin(time * 0.6)) + cos(uv.x + time * 0.5) - tan(length(uv) * 0.333);
    for (float i = 1.; i < count; ++i) {
        float t = abs((2. * load + 1.0) / ((1.0 + factor + fbm(uv + time / i)) * (i * 60.0)));
        finalColor += t * vec3(i * 0.1 + upload, 0.3, i * 0.9 + download);
    }

    finalColor *= 30.0 / count;
    return vec4(finalColor, 1.0);
}
