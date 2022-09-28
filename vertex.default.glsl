attribute vec3 pos;
attribute vec2 coord;
in uniform vec2 coord;

void main() {
    fragCoord = coord;
    gl_Position = vec4(pos, 1.0);
}
