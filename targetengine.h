#ifndef TARGETENGINE_H
#define TARGETENGINE_H

#include <QPoint>
#include <QVector2D>
#include <QGLFunctions>
#include <QGLShaderProgram>

struct AISTarget {
  float LAT, LON, COG, ROT, SOG;
};

class TargetEngine : protected QGLFunctions {
public:
  explicit TargetEngine();
  virtual ~TargetEngine();

  bool init(const QGLContext* context);
  void draw(QVector2D world_coords, float scale);

private:
  void initBuffers();
  void initShader();
  void initTexture();

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
