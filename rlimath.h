#ifndef RLIMATH_H
#define RLIMATH_H

#include <qmath.h>
#include <QVector2D>
#include <QPoint>


namespace RLIMath {
  const double PI = 2 * asin(1);
  const double EARTH_RADIUS = 6378137.0;

  inline double radians(double deg) { return (deg / 180.f) * PI; }
  inline double degrees(double rad) { return (rad / PI) * 180.f; }

  QVector2D pos_to_coords(QVector2D center, QPointF center_pos, QPointF pos, float scale);
  QPointF coords_to_pos(QVector2D center, QVector2D coords, QPointF center_pos, float scale);

  double geo_distance(double lat1, double lon1, double lat2, double lon2);
  double geo_distance(QVector2D coords1, QVector2D coords2);

}
#endif // RLIMATH_H
