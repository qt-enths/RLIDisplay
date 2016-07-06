#version 120

uniform vec4  color;

varying float v_char_val;
varying vec2	v_inner_texcoords;

uniform sampler2D glyph_tex;

void main() {
  if (v_char_val != -1) {
    vec2 tex_coord = (1.0/16.0) * vec2( mod(v_char_val , 16) + v_inner_texcoords.x
                                      , (16 - floor(v_char_val / 16)) - v_inner_texcoords.y );
    vec4 tex_color = texture2D(glyph_tex, tex_coord);

    if (tex_color.r > .5f)
      gl_FragColor = vec4(1.f, 1.f, 1.f, 0.0f);
    else
      gl_FragColor = color;
  } else {
    gl_FragColor = color;
  }
}
