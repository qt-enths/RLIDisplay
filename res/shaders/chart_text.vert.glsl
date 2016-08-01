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
// Radius of chart part to show in meters
uniform float   radius;
// Screen dimensions in pixels
uniform vec2    canvas;

// Pattern texture full size in pixels
uniform vec2  pattern_tex_size;
uniform vec3  color;

varying vec3   v_color;
varying float  v_char_val;


void main() {
  // Display aspect ratio
  float aspect = canvas.x / canvas.y;

  // Position relative to the chart points in meters
  float y_m = 6373*1000*radians(world_coords.x - center.x);
  float x_m = 6373*cos(radians(world_coords.x))*1000*radians(world_coords.y - center.y);

  vec2 pos;

  // screen position
  if (aspect > 1)
    pos = vec2((x_m / radius) / aspect, y_m / radius);
  else
    pos = vec2(x_m / radius, (y_m / radius) * aspect);


  gl_Position = vec4(pos.x + char_order * (24.0/canvas.x), pos.y, 0, 1);
  v_char_val = char_val;
  //v_color = color;
  v_color = vec3(0, 0, 0);
}
