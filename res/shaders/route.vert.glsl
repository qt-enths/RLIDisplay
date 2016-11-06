#version 130

attribute vec2  prev_world_coords;
attribute vec2  curr_world_coords;
attribute vec2  next_world_coords;

uniform vec2 center;
uniform float scale;
uniform float type;

vec2 pix_pos(vec2 coords);
vec2 line_intersection(vec2 p11, vec2 p12, vec2 p21, vec2 p22);

void main() {
  vec2 curr_pix_pos = pix_pos(curr_world_coords);

  if (type == 0) {
    gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(curr_pix_pos, 0, 1);
  } else {
    float shift_dir = 1.f;
    if (type != 1)
      shift_dir = -1.f;

    float farwater_width = 500.f; //meters
    float farwater_width_pix = farwater_width / scale;

    vec2 prev_pix_pos;
    vec2 next_pix_pos;

    vec2 tan_pix_prev;
    vec2 tan_pix_next;

    vec2 norm_pix_prev;
    vec2 norm_pix_next;

    if (prev_world_coords == curr_world_coords) {
      next_pix_pos = pix_pos(next_world_coords);
      tan_pix_next = normalize(next_pix_pos - curr_pix_pos);
      norm_pix_next = vec2(tan_pix_next.y, -tan_pix_next.x);
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(curr_pix_pos + shift_dir*farwater_width_pix*norm_pix_next, 0, 1);
      return;
    }

    if (next_world_coords == curr_world_coords) {
      prev_pix_pos = pix_pos(prev_world_coords);
      tan_pix_prev = normalize(curr_pix_pos - prev_pix_pos);
      norm_pix_prev = vec2(tan_pix_prev.y, -tan_pix_prev.x);
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(curr_pix_pos + shift_dir*farwater_width_pix*norm_pix_prev, 0, 1);
      return;
    }

    prev_pix_pos = pix_pos(prev_world_coords);
    next_pix_pos = pix_pos(next_world_coords);

    tan_pix_prev = normalize(curr_pix_pos - prev_pix_pos);
    tan_pix_next = normalize(next_pix_pos - curr_pix_pos);

    norm_pix_prev = vec2(tan_pix_prev.y, -tan_pix_prev.x);
    norm_pix_next = vec2(tan_pix_next.y, -tan_pix_next.x);

    if (  abs(norm_pix_prev.x - norm_pix_next.x) < 0.0001 && abs(norm_pix_prev.y - norm_pix_next.y) < 0.0001) {
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(curr_pix_pos + shift_dir*farwater_width_pix*norm_pix_next, 0, 1);
    } else {
      vec2 p11 = prev_pix_pos + shift_dir*farwater_width_pix*norm_pix_prev;
      vec2 p12 = curr_pix_pos + shift_dir*farwater_width_pix*norm_pix_prev;
      vec2 p21 = curr_pix_pos + shift_dir*farwater_width_pix*norm_pix_next;
      vec2 p22 = next_pix_pos + shift_dir*farwater_width_pix*norm_pix_next;

      vec2 shifted_pix_pos = line_intersection(p11, p12, p21, p22);
      gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix  * vec4(shifted_pix_pos, 0, 1);
    }
  }
}

vec2 line_intersection(vec2 p11, vec2 p12, vec2 p21, vec2 p22) {
  float d1 = (p11.x*p12.y-p11.y*p12.x);
  float d2 = (p21.x*p22.y-p21.y*p22.x);
  float d3 = (p11.x-p12.x)*(p21.y-p22.y) - (p11.y-p12.y)*(p21.x-p22.x);

  float xt = d1*(p21.x-p22.x) - d2*(p11.x-p12.x);
  float yt = d1*(p21.y-p22.y) - d2*(p11.y-p12.y);

  return vec2(xt/d3, yt/d3);
}

vec2 pix_pos(vec2 coords) {
  float lat_rads = radians(center.x);

  float y_m = -6378137*radians(coords.x - center.x);
  float x_m = 6378137*cos(lat_rads)*radians(coords.y - center.y);

  return vec2(x_m, y_m) / scale;
}
