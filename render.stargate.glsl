// http://glslsandbox.com/e#45918.0
#ifdef GL_ES
precision highp float;
#endif

#define PI 3.14159265359

float random(float n) { return fract(abs(sin(n * 55.753) * 367.34)); }

mat2 rotate2d(float angle) { return mat2(cos(angle), -sin(angle), sin(angle), cos(angle)); }

vec4 render(vec2 p) {
    vec2 uv = p * 2.0;

    uv *= rotate2d(time * 0.1);  // time * 0.2

    float direction = -1.0;
    float speed = time * direction * 1.6;
    float distanceFromCenter = length(uv);

    float meteorAngle = atan(uv.y, uv.x) * (180.0 / PI);

    float flooredAngle = floor(meteorAngle);
    float randomAngle = pow(random(flooredAngle), 0.2);
    float t = speed + randomAngle;

    float lightsCountOffset = 0.9;
    float adist = randomAngle / distanceFromCenter * lightsCountOffset;
    float dist = t + adist;
    float meteorDirection = (direction < 0.0) ? -1.0 : 0.0;
    dist = abs(fract(dist) + meteorDirection);

    float lightLength = 200.0;
    float meteor = (5.0 / dist) * cos(sin(speed)) / lightLength;
    meteor *= distanceFromCenter * 2.0;

    vec3 color = vec3(0.);
    color.gb += meteor;

    return vec4(color, 1.0);
}
