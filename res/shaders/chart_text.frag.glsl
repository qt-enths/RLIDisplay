#version 120

varying vec3   v_color;
varying float  v_char_val;

uniform sampler2D glyph_tex;

void main() {
  vec2 tex_coord = (1.0/16.0) * vec2( mod(v_char_val , 16) + gl_PointCoord.x
                                    , floor(v_char_val / 16) + 1 - gl_PointCoord.y );

  gl_FragColor = texture2D(glyph_tex, tex_coord);

  if (gl_FragColor.rgb == vec3(1, 1, 1))
    gl_FragColor = vec4(gl_FragColor.rgb, 0);
  else
    gl_FragColor.rgb = v_color;
}
