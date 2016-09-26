#version 120

// World position lat/lon in degrees
attribute vec2	world_coords;
// Quad vertex order (1 - bottom left, 2 - boottom right, 3 - top right, 4 - top left)
attribute float vertex_order;
// Chart symbol position in pattern texture
attribute vec2	symbol_origin;
// Chart symbol size in pixels
attribute vec2	symbol_size;
// Chart symbol pivot point
attribute vec2	symbol_pivot;

// Angle to north
uniform float   north;
// Chart center position lat/lon in degrees
uniform vec2    center;
// meters/pixel
uniform float   scale;

// Pattern texture full size in pixels
uniform vec2  pattern_tex_size;

// Uniform replacements for some attributes are to be used if all
// the points in a layer should be displayed with the same symbol
uniform vec2	u_symbol_origin;
uniform vec2	u_symbol_size;
uniform vec2	u_symbol_pivot;

varying vec2	v_texcoords;
varying vec4	v_color;

void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(world_coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(world_coords.y - center.y);

  // screen position
  vec2 pos_pix = vec2(x_m, y_m) / scale;

  // Variables to store symbol parameters
  vec2 origin, size, pivot;
  if (u_symbol_origin.x == -1) {
    origin = symbol_origin;
    size = symbol_size;
    pivot = symbol_pivot;// + vec2(0.5, 0.5);
  } else {
    origin = u_symbol_origin;
    size = u_symbol_size;
    pivot = u_symbol_pivot;// + vec2(0.5, 0.5);
  }

  // Quad vertex coordinates in pixels
  if (vertex_order == 0) {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pos_pix + vec2(-pivot.x, pivot.y), 0, 1);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(0,-1)) / pattern_tex_size;
  } else if (vertex_order == 1) {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pos_pix + vec2(-pivot.x, -(size.y-pivot.y)), 0, 1);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(0,0)) / pattern_tex_size;
  } else if (vertex_order == 2) {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pos_pix + vec2(size.x-pivot.x, -(size.y-pivot.y)), 0, 1);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(1,0)) / pattern_tex_size;
  } else if (vertex_order == 3) {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pos_pix + vec2(size.x-pivot.x, pivot.y), 0, 1);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(1,-1)) / pattern_tex_size;
  }
}
