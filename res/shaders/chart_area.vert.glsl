#version 120

attribute vec2	coords;
attribute float color_index;
attribute vec2	tex_origin;
attribute vec2	tex_dim;

uniform float	north;
uniform vec2	center;
uniform float	scale;
uniform vec2  assetdim;

uniform float	u_color_table[256];
uniform float u_color_index;
uniform vec2  u_tex_origin;
uniform vec2  u_tex_dim;

varying vec3	v_color;
varying vec2	v_tex_dim;
varying vec2	v_tex_orig;
varying vec2	v_inner_texcoords;
varying float v_use_tex_color;

void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(coords.y - center.y);

  // screen position
  vec2 pix_pos = vec2(x_m, y_m) / scale;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pix_pos, 0, 1);

  if (u_tex_dim.r == -1) {
    //0 - 1
    v_inner_texcoords = pix_pos / tex_dim;
    v_tex_dim = tex_dim / assetdim;
    v_tex_orig = tex_origin / assetdim;
  } else {
    //0 - 1
    v_inner_texcoords = pix_pos / u_tex_dim;
    v_tex_dim = u_tex_dim / assetdim;
    v_tex_orig = u_tex_origin / assetdim;
  }

  int color_ind = 0;
  if (u_color_index == -1)
    color_ind = int(color_index);
  else
    color_ind = int(u_color_index);
  v_color = vec3(u_color_table[3*color_ind+0], u_color_table[3*color_ind+1], u_color_table[3*color_ind+2]);

  // !!!! check to SOLID pattern - to replace!!!!
  if (v_tex_orig != vec2(0,0))
    v_use_tex_color = 1;
  else
    v_use_tex_color = 0;
}
