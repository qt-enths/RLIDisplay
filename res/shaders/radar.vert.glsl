#version 120

attribute vec2 pos;
attribute float first;

attribute float amp;
//attribute float div;

uniform float clear;

varying float norm_amp;


void main() {
  //float namp = amp/div;

  if (clear == 0) {
    //gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(pos, -namp/255, 1);
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(pos, -amp/255, 1);
  } else {
    if (first == 1)
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(pos, 1, 1);
    else
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * vec4(0, 0, 0, 0);
  }

  //norm_amp = int(namp / 16.0);
  norm_amp = int(amp / 16.0);
}
