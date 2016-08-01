#version 120

attribute vec2	coords1;
attribute vec2	coords2;
attribute float dist;
attribute float order;
attribute vec2	tex_orig;
attribute vec2	tex_dim;
attribute float color_index;

uniform float	north;
uniform vec2	center;
uniform float	radius;
uniform vec2	canvas;
uniform vec2  assetdim;

uniform float	u_color_table[256];

uniform float u_color_index;
uniform vec2  u_tex_origin;
uniform vec2  u_tex_dim;

varying vec3	v_color;
varying vec2	v_tex_dim;
varying vec2	v_tex_orig;
varying vec2	v_texcoords;
varying vec2	v_inner_texcoords;
varying float v_use_tex_color;

void main() {
  float aspect = canvas.x / canvas.y;
  float pix_per_meter;

  float lat1_rads = radians(coords1.x);
  float lat2_rads = radians(coords2.x);

  float y1_m = 6372795.f*radians(coords1.x - center.x);
  float x1_m = 6372795.f*cos(lat1_rads)*radians(coords1.y - center.y);

  float y2_m = 6372795.f*radians(coords2.x - center.x);
  float x2_m = 6372795.f*cos(lat2_rads)*radians(coords2.y - center.y);

  vec2 pos1, pos2;

  // opengl screen position
  if (aspect > 1) {
    pos1 = vec2((x1_m / radius) / aspect, y1_m / radius);
    pos2 = vec2((x2_m / radius) / aspect, y2_m / radius);
    pix_per_meter = (canvas.y / 2) / radius;
  } else {
    pos1 = vec2(x1_m / radius, (y1_m / radius) * aspect);
    pos2 = vec2(x1_m / radius, (y1_m / radius) * aspect);
    pix_per_meter = (canvas.x / 2) / radius;
  }

  // screen position in pixels
  vec2 pos1_pix = ((pos1 + vec2(1, 1)) / 2) * canvas;
  vec2 pos2_pix = ((pos2 + vec2(1, 1)) / 2) * canvas;

  vec2 tan_pix = pos2_pix - pos1_pix;
  float len_pix = sqrt(tan_pix.x * tan_pix.x + tan_pix.y * tan_pix.y);
  vec2 unit_tan_pix = normalize(tan_pix);

  if (u_tex_dim.r == -1) {
    v_tex_dim = tex_dim;
    v_tex_orig = tex_orig;
  } else {
    v_tex_dim = u_tex_dim;
    v_tex_orig = u_tex_origin;
  }

  vec2 norm_pix = (v_tex_dim.y / 2) * vec2(unit_tan_pix.y, -unit_tan_pix.x);
  vec2 norm = 2.0 * (norm_pix / canvas);

  int color_ind = 0;
  if (u_color_index == -1)
    color_ind = int(color_index);
  else
    color_ind = int(u_color_index);
  v_color = vec3(u_color_table[3*color_ind+0], u_color_table[3*color_ind+1], u_color_table[3*color_ind+2]);

  v_texcoords = assetdim;
  float dist_pix = dist * pix_per_meter;

  if (order == 0) {
    gl_Position = vec4(pos1 - norm, 0, 1);
    v_inner_texcoords = vec2(dist_pix/v_tex_dim.x, 0);
  } else if (order == 1) {
    gl_Position = vec4(pos2 - norm, 0, 1);
    v_inner_texcoords = vec2((dist_pix + len_pix)/v_tex_dim.x, 0);
  } else if (order == 2) {
    gl_Position = vec4(pos2 + norm, 0, 1);
    v_inner_texcoords = vec2((dist_pix + len_pix)/v_tex_dim.x, 1);
  } else if (order == 3) {
    gl_Position = vec4(pos1 + norm, 0, 1);
    v_inner_texcoords = vec2(dist_pix/v_tex_dim.x, 1);
  }

  if (tex_orig.y < 96)
    v_use_tex_color = 1;
  else
    v_use_tex_color = 0;
}
