#version 120

attribute vec2  world_coords;
attribute float vertex_order;
attribute float COG; // реальный курс судна (true heading – это куда смотрит его нос);
attribute float ROT; // скорость поворота (для рисования вектора поворота);
attribute float SOG; // скорость.

uniform vec2 center;
uniform float scale;
uniform float type;


varying vec2 v_inner_texcoords;
varying float v_type;

void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(world_coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(world_coords.y - center.y);

  // screen position
  vec2 pix_pos = vec2(x_m, y_m) / scale;

  float cog_rad = radians(COG);
  float cog_rot_rad = radians(COG+ROT);
  mat2 rot_mat = mat2( vec2(cos(cog_rad), sin(cog_rad)), vec2(-sin(cog_rad), cos(cog_rad)));
  mat2 rot_mat2 = mat2( vec2(cos(cog_rot_rad), sin(cog_rot_rad)), vec2(-sin(cog_rot_rad), cos(cog_rot_rad)));

  if (type == 0) {
    if (vertex_order == 0) {
      pix_pos = pix_pos + rot_mat*vec2(-16, -16);
      v_inner_texcoords = vec2(0, 1);
    } else if (vertex_order == 1) {
      pix_pos = pix_pos + rot_mat*vec2(-16, 16);
      v_inner_texcoords = vec2(0, 0);
    } else if (vertex_order == 2) {
      pix_pos = pix_pos + rot_mat*vec2(16, 16);
      v_inner_texcoords = vec2(1, 0);
    } else if (vertex_order == 3) {
      pix_pos = pix_pos + rot_mat*vec2(16, -16);
      v_inner_texcoords = vec2(1, 1);
    }
  } else if (type == 1) {
    if (mod(vertex_order, 2) == 0)
      pix_pos = pix_pos + rot_mat*vec2(0, -SOG);
  } else if (type == 2) {
    if (mod(vertex_order, 2) == 0)
      pix_pos = pix_pos + rot_mat2*vec2(0, -SOG);
  }

  v_type = type;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pix_pos, 0, 1);
}
