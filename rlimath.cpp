#include "rlimath.h"

QVector2D RLIMath::pos_to_coords(QVector2D center, QPointF center_pos, QPointF pos, float scale) {
  QPointF metric_pos = (pos - center_pos) * scale;

  float lat_rads = radians(center.x());

  float lat = degrees(-metric_pos.y() / EARTH_RADIUS) + center.x();
  float lon = degrees(metric_pos.x() / (EARTH_RADIUS * cos(lat_rads))) + center.y();

  return QVector2D(lat, lon);
}

QPointF RLIMath::coords_to_pos(QVector2D center, QVector2D coords, QPointF center_pos, float scale) {
  float lat_rads = radians(center.x());

  float y_m = -EARTH_RADIUS*radians(coords.x() - center.x());
  float x_m = EARTH_RADIUS*cos(lat_rads)*radians(coords.y() - center.y());

  QPointF pix_pos(x_m / scale, y_m / scale);
  return pix_pos + center_pos;
}

double RLIMath::geo_distance(double lat1, double lon1, double lat2, double lon2) {
  double lat2_rads = radians(lat2);
  double y = -EARTH_RADIUS*radians(lat1 - lat2);
  double x = EARTH_RADIUS*cos(lat2_rads)*radians(lon1 - lon2);
  return sqrt(x*x+y*y);
}

double RLIMath::geo_distance(QVector2D coords1, QVector2D coords2) {
  return geo_distance(coords1.x(), coords1.y(), coords2.x(), coords2.y());
}
