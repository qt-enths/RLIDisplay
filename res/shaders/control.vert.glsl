#version 120

attribute float angle;

uniform float radius;

void main() {
  float phi = radians(angle);
  vec2 pix_pos = vec2(radius * sin(phi), -radius * cos(phi));
  gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(pix_pos, 0, 1);
}
