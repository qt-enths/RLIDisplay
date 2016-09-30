#version 120

// World position lat/lon in degrees
attribute vec2	world_coords;
// Character int value and order in text line
attribute float	char_order;
attribute float	char_val;

// Angle to north
uniform float   north;
// Chart center position lat/lon in degrees
uniform vec2    center;
// meters / pixel
uniform float   scale;

// Pattern texture full size in pixels
uniform vec2  pattern_tex_size;
uniform vec3  color;

varying vec3   v_color;
varying float  v_char_val;


void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(world_coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(world_coords.y - center.y);

  // screen position
  vec2 pos_pix = vec2(x_m, y_m) / scale;


  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pos_pix.x + char_order * 12.0, pos_pix.y, 0, 1);
  v_char_val = char_val;
  //v_color = color;
  v_color = vec3(0, 0, 0);
}
