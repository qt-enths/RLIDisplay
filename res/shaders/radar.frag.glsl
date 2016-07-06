#version 130

varying float norm_amp;

uniform vec3 palette[16];
uniform float threshold;

void main() {
  float alpha;

  if (norm_amp >= threshold)
    alpha = 1.0;
  else
    alpha = 0.0;

  gl_FragColor = vec4(palette[int(norm_amp)] / 255.0, alpha);
}
