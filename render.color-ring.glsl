#ifdef GL_ES
precision mediump float;
#endif

vec4 render(vec2 p) {
    float t;
    t = time + 8.0;
    vec2 r = vec2(192, 64);
    o = p - r / 2.;
    o = vec2(length(o) / r.y - .3, atan(o.y, o.x));
    vec4 s = 0.08 * cos(1.5 * vec4(0, 1, 2, 3) + t + o.y + cos(o.y) * cos(8.0)), e = s.yzwx, f = max(o.x - s, e - o.x);
    return dot(clamp(f * r.y, 0., 1.), 80. * (s - e)) * (s - .1) + (e - o.x);
}
