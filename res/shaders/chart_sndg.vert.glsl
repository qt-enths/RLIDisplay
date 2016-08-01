#version 120

// World position lat/lon in degrees
attribute vec2	world_coords;
// Quad vertex order (1 - bottom left, 2 - boottom right, 3 - top right, 4 - top left)
attribute float vertex_order;
attribute float	symbol_order;
attribute float symbol_frac;
attribute float symbol_count;
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
// Radius of chart part to show in meters
uniform float   radius;
// Screen dimensions in pixels
uniform vec2    canvas;

// Pattern texture full size in pixels
uniform vec2  pattern_tex_size;

varying vec2	v_texcoords;

void main() {
  // Display aspect ratio
  float aspect = canvas.x / canvas.y;

  // Position relative to the chart points in meters
  float y_m = 6373*1000*radians(world_coords.x - center.x);
  float x_m = 6373*cos(radians(world_coords.x))*1000*radians(world_coords.y - center.y);

  // OpenGL screen position
  vec2 pos;
  if (aspect > 1) {
    pos = vec2((x_m / radius) / aspect, y_m / radius);
  } else {
    pos = vec2(x_m / radius, (y_m / radius) * aspect);
  }

  // Screen position in pixels
  vec2 pos_pix = ((pos + vec2(1, 1)) / 2) * canvas;
  pos_pix = pos_pix + vec2(symbol_order * 8 - symbol_count * 4, symbol_frac * -4);

  // Variables to store symbol parameters
  vec2 origin, size, pivot;
  origin = symbol_origin;
  size = symbol_size;
  pivot = symbol_pivot + vec2(0.5, 0.5);

  // Quad vertex coordinates in pixels
  vec2 vert_pos_pix;

  if (vertex_order == 0) {
    vert_pos_pix = pos_pix - pivot;
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(0,-1)) / pattern_tex_size;
  } else if (vertex_order == 1) {
    vert_pos_pix = pos_pix - pivot + vec2(size.x, 0);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(1,-1)) / pattern_tex_size;
  } else if (vertex_order == 2) {
    vert_pos_pix = pos_pix - pivot + size;
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(1,0)) / pattern_tex_size;
  } else if (vertex_order == 3) {
    vert_pos_pix = pos_pix - pivot + vec2(0, size.y);
    v_texcoords = (pattern_tex_size*vec2(0, 1) + origin*vec2(1, -1) + size*vec2(0,0)) / pattern_tex_size;
  }

  // Convering quad vertex position back to OpenGL format
  gl_Position = vec4(2 * (vert_pos_pix - canvas/2) / canvas, 0, 1);
}
