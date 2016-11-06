#ifndef RLIDISPLAYWIDGET_H
#define RLIDISPLAYWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

#include "chartmanager.h"

#include "radarengine.h"
#include "infoengine.h"
#include "menuengine.h"
#include "maskengine.h"
#include "chartengine.h"
#include "targetengine.h"
#include "controlsengine.h"
#include "routeengine.h"

class RLIDisplayWidget : public QGLWidget, protected QGLFunctions
{
  Q_OBJECT
public:
  explicit RLIDisplayWidget(QWidget *parent = 0);
  ~RLIDisplayWidget();

  inline ChartEngine* chartEngine() { return _chartEngine; }
  inline RadarEngine* radarEngine() { return _radarEngine; }
  inline MaskEngine* maskEngine() { return _maskEngine; }
  inline InfoEngine* infoEngine() { return _infoEngine; }
  inline MenuEngine* menuEngine() { return _menuEngine; }
  inline RouteEngine* routeEngine() { return _routeEngine; }
  inline TargetEngine* targetEngine() { return _targetEngine; }

  inline void setChartManager(ChartManager* mngr) { _chrt_mngr = mngr; }

  void setScale(float scale) { _scale = scale; }

  void setWorldCoords(QVector2D coords);
  QVector2D getWorldCoords(void) const;

signals:
  void initialized();
  void cursor_moved(float peleng, float distance, const char * dist_fmt);
  void cursor_moved(QVector2D);
  void per_second();
  void resized(const QSize& s);
  void displayVNDistance(float dist, const char * fmt);
  void displaydBRG(float brg, float crsangle);

public slots:
  void onCoordsChanged(const QVector2D& new_coords);

protected slots:
  void mousePressEvent(QMouseEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void mouseReleaseEvent(QMouseEvent* e);
  void wheelEvent(QWheelEvent * e);

  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

  bool event(QEvent* e);

  void new_chart(const QString& name);

  void onStartRouteEdit();
  void onAddRoutePoint();
  void onFinishRouteEdit();

private:
  void fillWithTexture(GLuint texId);

  void moveCoursor(const QPoint &pos, bool repaint = true);

  bool _initialized;

  bool _route_edition;

  int  _last_second;
  float _scale;

  AsmFonts* _fonts;
  ChartManager* _chrt_mngr;

  ChartEngine* _chartEngine;
  RadarEngine* _radarEngine;
  InfoEngine* _infoEngine;
  MaskEngine* _maskEngine;
  MenuEngine* _menuEngine;

  TargetEngine* _targetEngine;
  ControlsEngine* _controlsEngine;
  RouteEngine* _routeEngine;

  QVector2D _world_coords;
};

#endif // RLIDISPLAYWIDGET_H
