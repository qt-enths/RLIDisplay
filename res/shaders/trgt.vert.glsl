#version 130

attribute vec2  world_coords;
attribute float vertex_order;
attribute float heading;
attribute float rotation;
attribute float course;
attribute float speed;

uniform vec2 center;
uniform float scale;
uniform float type;

out vec2 screen_pos;

varying vec2 v_inner_texcoords;
varying float v_type;

void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(world_coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(world_coords.y - center.y);

  // screen position
  vec2 pix_pos = vec2(x_m, y_m) / scale;
  screen_pos = pix_pos;

  float head_rad = radians(heading);
  float cog_rad = radians(course);
  mat2 rot_mat_head = mat2( vec2(cos(head_rad), sin(head_rad)), vec2(-sin(head_rad), cos(head_rad)));
  mat2 rot_mat_cog = mat2( vec2(cos(cog_rad), sin(cog_rad)), vec2(-sin(cog_rad), cos(cog_rad)));

  if (heading < 0)
    rot_mat_head = rot_mat_cog;

  if (type == 0) {
    if (vertex_order == 0) {
      pix_pos = pix_pos + rot_mat_head*vec2(-16, -16);
      v_inner_texcoords = vec2(0, 1);
    } else if (vertex_order == 1) {
      pix_pos = pix_pos + rot_mat_head*vec2(-16, 16);
      v_inner_texcoords = vec2(0, 0);
    } else if (vertex_order == 2) {
      pix_pos = pix_pos + rot_mat_head*vec2(16, 16);
      v_inner_texcoords = vec2(1, 0);
    } else if (vertex_order == 3) {
      pix_pos = pix_pos + rot_mat_head*vec2(16, -16);
      v_inner_texcoords = vec2(1, 1);
    }
  } else if (type == 1 && heading >= 0) {
    if (vertex_order == 1 || vertex_order == 2)
      pix_pos = pix_pos + rot_mat_head*vec2(0, -48);
    if (vertex_order == 3)
      pix_pos = pix_pos + rot_mat_head*vec2(sign(rotation)*8, -48);
  } else if (type == 2) {
    if (vertex_order == 1)
      pix_pos = pix_pos + rot_mat_cog*vec2(0, -speed);
  }

  v_type = type;
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pix_pos, 0, 1);
}
