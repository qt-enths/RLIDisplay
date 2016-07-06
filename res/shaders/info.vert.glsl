#version 120

attribute vec2  position;
attribute float order;
attribute float char_val;

uniform vec2  size;

varying float v_char_val;
varying vec2	v_inner_texcoords;

void main() {
  vec2 pos;

  if (order == 0) {
    pos = position;
    v_inner_texcoords = vec2(0, 0);
  } else if (order == 1) {
    pos = position + vec2(0, size.y);
    v_inner_texcoords = vec2(0, 1);
  } else if (order == 2) {
    pos = position + size;
    v_inner_texcoords = vec2(1, 1);
  } else if (order == 3) {
    pos = position + vec2(size.x, 0);
    v_inner_texcoords = vec2(1, 0);
  }

  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(pos, 0, 1);

  v_char_val = char_val;
}
