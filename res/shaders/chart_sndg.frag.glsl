#version 120

varying vec2	v_texcoords;

uniform sampler2D pattern_tex_id;

void main() {
  gl_FragColor = texture2D(pattern_tex_id, v_texcoords);
}
