#ifndef TARGETENGINE_H
#define TARGETENGINE_H

#include <QPoint>
#include <QMutex>
#include <QList>
#include <QTimer>
#include <QVector2D>
#include <QGLShaderProgram>
#include <QGLFunctions>


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
  explicit TargetEngine(QObject* parent = 0);
  virtual ~TargetEngine();

  bool init(const QGLContext* context);
  void draw(QVector2D world_coords, float scale);

  int getTailsTime(void);
  int getCurrentIndex();

signals:
  void targetCountChanged(int count);
  void selectedTargetUpdated(const QString& tag, const RadarTarget& trgt);

public slots:
  void onTailsTimer();
  void onTailsTimeChanged(int minutes);
  void trySelect(QVector2D cursorCoords, float scale);
  void deleteTarget(QString tag);
  void updateTarget(QString tag, RadarTarget target);

private:
  void bindBuffers();
  void initBuffersTrgts(QString tag);
  int initBuffersTails();

  void initShader();
  void initTexture(QString path, GLuint* tex_id);

  QMutex _trgtsMutex;
  QString _selected;
  QMap<QString, RadarTarget> _targets;

  QTimer _tailsTimer;
  QMap<QString, QList<QVector2D> > _tails;
  int _tailsTime; // Maximum tails time in minutes

  enum {TRG_TAIL_NUM = 4};

  bool _initialized;

  // Mask shader programs
  GLuint _asset_texture_id;
  GLuint _selection_texture_id;
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

  GLuint _tbo_id;
  GLuint _vbo_ids[AIS_TRGT_ATTR_COUNT];
  GLuint _attr_locs[AIS_TRGT_ATTR_COUNT];
  GLuint _unif_locs[AIS_TRGT_UNIF_COUNT];
};

#endif // TARGETENGINE_H
