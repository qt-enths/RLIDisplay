#ifndef TARGETENGINE_H
#define TARGETENGINE_H

#include <QPoint>
#include <QMutex>
#include <QVector2D>
#include <QGLFunctions>
#include <QGLShaderProgram>

struct RadarTarget {
public:
  RadarTarget() {
    Lost = false;
    Latitude = 0;
    Longtitude = 0;
    Heading = 0;
    Rotation = 0;
    CourseOverGround = 0;
    SpeedOverGround = 0;
  }

  RadarTarget(const RadarTarget& o) {
    Lost = o.Lost;
    Latitude = o.Latitude;
    Longtitude = o.Longtitude;
    Heading = o.Heading;
    Rotation = o.Rotation;
    CourseOverGround = o.CourseOverGround;
    SpeedOverGround = o.SpeedOverGround;
  }

  ~RadarTarget() { }

  bool Lost;
  float Latitude, Longtitude;
  float Heading, Rotation;
  float CourseOverGround, SpeedOverGround;
};

Q_DECLARE_METATYPE(RadarTarget)


class TargetEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit TargetEngine();
  virtual ~TargetEngine();

  bool init(const QGLContext* context);
  void draw(QVector2D world_coords, float scale);

public slots:
  void trySelect(QPoint cursorPos);
  void deleteTarget(QString tag);
  void updateTarget(QString tag, RadarTarget target);

private:
  void initBuffers();
  void initShader();
  void initTexture();

  QMutex _trgtsMutex;
  QString _selected;
  QMap<QString, RadarTarget> _targets;
  bool _initialized;

  // Mask shader programs
  GLuint _asset_texture_id;
  QGLShaderProgram* _prog;

  // -----------------------------------------------
  enum { AIS_TRGT_ATTR_COORDS = 0
       , AIS_TRGT_ATTR_ORDER = 1
       , AIS_TRGT_ATTR_HEADING = 2
       , AIS_TRGT_ATTR_COURSE = 3
       , AIS_TRGT_ATTR_ROTATION = 4
       , AIS_TRGT_ATTR_SPEED = 5
       , AIS_TRGT_ATTR_COUNT = 6 } ;
  enum { AIS_TRGT_UNIF_CENTER = 0
       , AIS_TRGT_UNIF_SCALE = 1
       , AIS_TRGT_UNIF_TYPE = 2
       , AIS_TRGT_UNIF_COUNT = 3 } ;

  GLuint _vbo_ids[AIS_TRGT_ATTR_COUNT];
  GLuint _attr_locs[AIS_TRGT_ATTR_COUNT];
  GLuint _unif_locs[AIS_TRGT_UNIF_COUNT];
};

#endif // TARGETENGINE_H
