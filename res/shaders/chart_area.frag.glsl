#version 120

varying vec3	v_color;
varying vec2	v_tex_dim;
varying vec2	v_tex_orig;
varying vec2	v_inner_texcoords;
varying float v_use_tex_color;

uniform sampler2D pattern_tex;

void main() {
  vec2 texcoords = v_tex_orig + (fract(v_inner_texcoords) * v_tex_dim);
  vec4 tex_color = texture2D(pattern_tex, texcoords);

  if (tex_color.a > 0 && v_use_tex_color < 0.5)
    gl_FragColor = vec4(v_color, 1);
  else
    gl_FragColor = tex_color;
}
