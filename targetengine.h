#ifndef TARGETENGINE_H
#define TARGETENGINE_H

#include <QPoint>
#include <QMutex>
#include <QVector2D>
#include <QGLFunctions>
#include <QGLShaderProgram>

class RadarTarget {
public:
  RadarTarget() : LAT(0), LON(0), COG(0), ROT(0), SOG(0) { }
  RadarTarget(const RadarTarget& o) : LAT(o.LAT), LON(o.LON), COG(o.COG), ROT(o.ROT), SOG(o.SOG) { }
  ~RadarTarget() { }
  float LAT, LON, COG, ROT, SOG;
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
  void updateTarget(QString tag, RadarTarget target);

private:
  void initBuffers();
  void initShader();
  void initTexture();

  QMutex _trgtsMutex;
  QMap<QString, RadarTarget> _targets;
  bool _initialized;

  // Mask shader programs
  GLuint _asset_texture_id;
  QGLShaderProgram* _prog;

  // -----------------------------------------------
  enum { AIS_TRGT_ATTR_COORDS = 0
       , AIS_TRGT_ATTR_ORDER = 1
       , AIS_TRGT_ATTR_COG = 2
       , AIS_TRGT_ATTR_ROT = 3
       , AIS_TRGT_ATTR_SOG = 4
       , AIS_TRGT_ATTR_COUNT = 5 } ;
  enum { AIS_TRGT_UNIF_CENTER = 0
       , AIS_TRGT_UNIF_SCALE = 1
       , AIS_TRGT_UNIF_TYPE = 2
       , AIS_TRGT_UNIF_COUNT = 3 } ;

  GLuint _vbo_ids[AIS_TRGT_ATTR_COUNT];
  GLuint _attr_locs[AIS_TRGT_ATTR_COUNT];
  GLuint _unif_locs[AIS_TRGT_UNIF_COUNT];
};

#endif // TARGETENGINE_H
