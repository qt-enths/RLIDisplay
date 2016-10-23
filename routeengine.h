#ifndef ROUTEENGINE_H
#define ROUTEENGINE_H

#include <QList>
#include <QMutex>
#include <QVector2D>
#include <QGLShaderProgram>
#include <QGLFunctions>

class RouteEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit RouteEngine(QObject* parent = 0);
  virtual ~RouteEngine();

  bool init(const QGLContext* context);
  void draw(QVector2D world_coords, float scale);

public slots:

protected slots:

private:
  bool _initialized;

  void initShader();
  int loadBuffers(int routeIndex);

  QMutex _routesMutex;

  QGLShaderProgram* _prog;
  QList<QList<QVector2D> > _routes;

  // -----------------------------------------------
  enum { ROUTE_ATTR_COORDS = 0
       , ROUTE_ATTR_COUNT = 1 } ;
  enum { ROUTE_UNIF_CENTER = 0
       , ROUTE_UNIF_SCALE = 1
       , ROUTE_UNIF_COUNT = 2 } ;

  GLuint _vbo_ids[ROUTE_ATTR_COUNT];
  GLuint _attr_locs[ROUTE_ATTR_COUNT];
  GLuint _unif_locs[ROUTE_UNIF_COUNT];
};

#endif // ROUTEENGINE_H
