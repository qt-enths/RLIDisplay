#version 130

attribute vec2  world_coords;

uniform vec2 center;
uniform float scale;

void main() {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(world_coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(world_coords.y - center.y);

  // screen position
  vec2 pix_pos = vec2(x_m, y_m) / scale;

  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(pix_pos, 0, 1);
}
