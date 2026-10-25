#define PI 3.141592
#define ORBS 20.

vec4 render(vec2 uv){
  vec4 color = vec4(0.);
  for (float i = 0.; i < ORBS; i++) {
    uv.y -= i / 1000. * (uv.x); 
    uv.x += i / 0.05 * sin(uv.x / 9.32 + iTime) * 0.21 * cos(uv.y / 16.92 + iTime / 3.) * 0.21;
    float t = 5.1 * i * PI / float(ORBS) * (2. + 1.) + iTime / 10.;
    float x = -1. * tan(t);
    float y = sin(t / 3.5795); 
    vec2 p = (115. * vec2(x, y)) / sin(PI * sin(uv.x / 14.28 + iTime / 10.));
    vec3 col = cos(vec3(0, 1, -1) * PI * 2. / 3. + PI * (5. + i / 5.)) * 0.5 + 0.5;
    color += vec4(i / 40. * 55.94 / length(uv - p * 0.9) * col, 3.57);
  }
  color.xyz = pow(color.xyz, vec3(3.57));
  color.w = 1.0;
  return color;
}
