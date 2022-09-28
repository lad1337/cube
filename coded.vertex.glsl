attribute vec3 pos;
attribute vec2 coord;
varying vec2 fragCoord;
void main() {
    fragCoord = coord;
    gl_Position = vec4(pos, 1.0);
}
