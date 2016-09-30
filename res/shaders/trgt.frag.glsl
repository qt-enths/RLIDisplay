#version 120

uniform sampler2D tex;

varying vec2 v_inner_texcoords;
varying float v_type;

void main() {
  if (v_type == 0)
    gl_FragColor = texture2D(tex, v_inner_texcoords);
  else
    gl_FragColor = vec4(0.93, 0.17, 0.21, 1.00);
}
