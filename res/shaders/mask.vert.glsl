#version 120

attribute float angle;
attribute float char_val;
attribute float order;
attribute float shift;

uniform float angle_shift;
uniform float	circle_radius;
uniform vec2  circle_pos;
uniform vec2  cursor_pos;

varying float v_char_val;

vec2 intersectRayCircle(vec2 o, float r, vec2 p, float phi);

void main() {
  if (char_val == -1) {

    if (order == 0) {
      gl_Position = gl_ProjectionMatrix * vec4(cursor_pos, 0, 1);
    } else {
      float phi = radians(angle) + radians(angle_shift);
      vec2 p2 = intersectRayCircle(circle_pos, circle_radius + shift, cursor_pos, phi);
      gl_Position = gl_ProjectionMatrix * vec4(p2, 0, 1);
    }

  } else {

    float phi = radians(angle) + radians(angle_shift);
    vec2 tp = intersectRayCircle(circle_pos, circle_radius + 14, cursor_pos, phi);

    int char_count = 1;
    if (angle > 9) char_count = 2;
    if (angle > 99) char_count = 3;    

    vec2 mark_shift;

    if (abs(tp.y - circle_pos.y) < 10)
      mark_shift.y = 0.f;
    else
      if (tp.y > circle_pos.y)
        mark_shift.y = 4.f;
      else
        mark_shift.y = -4.f;

    if (abs(tp.x - circle_pos.x) < 10)
      mark_shift.x = - (char_count - 1) * 4.f;
    else
      if (tp.x > circle_pos.x)
        mark_shift.x = 4.f;
      else
        mark_shift.x = 4.f - char_count * 8.f;

    vec2 p2 = vec2( tp.x + 8 * order + mark_shift.x, tp.y + mark_shift.y);
    gl_Position = gl_ProjectionMatrix * vec4(p2, 0, 1);

  }

  v_char_val = char_val;
}

// o, r - circle's center and radius
// p, phi - ray's anchor and angle
vec2 intersectRayCircle(vec2 o, float r, vec2 p, float phi) {
  float dy = p.y-o.y;
  float dx = p.x-o.x;
  float r2 = r*r;

  vec2 p1, p2;
  float tp, tp2;
  float a, b, c, d;

  // Split solving intersection equation to prevent too big tan
  if (abs(sin(phi)) > 0.5) {
    tp = tan(phi - radians(90));
    tp2 = tp*tp;

    a = 1.0 + tp2;
    b = 2.0 * (tp*dy - o.x - tp2*p.x);
    c = dy*dy + o.x*o.x + (tp*p.x)*(tp*p.x) - 2*tp*p.x*dy - r2;

    d = b*b - 4*a*c;

    if (d < 0) {
      return o;
    } else {
      p1.x = (-b + sqrt(d)) / (2.0 * a);
      p2.x = (-b - sqrt(d)) / (2.0 * a);

      p1.y = p.y + tp * (p1.x - p.x);
      p2.y = p.y + tp * (p2.x - p.x);

      // To the right from hole center, choose
      if (sin(phi) * (p1.x - p.x) > 0)
        return p1;
      else
        return p2;
    }
  } else {
    tp = tan(-phi);
    tp2 = tp*tp;

    a = 1.0 + tp2;
    b = 2.0 * (tp*dx - o.y - tp2*p.y);
    c = dx*dx + o.y*o.y + (tp*p.y)*(tp*p.y) - 2*tp*p.y*dx - r2;

    d = b*b - 4*a*c;

    if (d < 0) {
      return o;
    } else {
      p1.y = (-b + sqrt(d)) / (2.0 * a);
      p2.y = (-b - sqrt(d)) / (2.0 * a);
      p1.x = p.x + tp * (p1.y - p.y);
      p2.x = p.x + tp * (p2.y - p.y);

      // To the right from hole center, choose
      if (cos(phi) * (p1.y - p.y) < 0)
        return p1;
      else
        return p2;
    }
  }
}
