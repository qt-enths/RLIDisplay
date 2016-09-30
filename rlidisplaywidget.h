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
  //inline TargetEngine* targetEngine { return _targetEngine; }

  inline void setChartManager(ChartManager* mngr) { _chrt_mngr = mngr; }

  void setScale(float scale) { _scale = scale; }

signals:
  void initialized();
  void cursor_moved(float peleng, float distance, const char * dist_fmt);
  void per_second();
  void resized(const QSize& s);
  void displayVNDistance(float dist, const char * fmt);
  void displaydBRG(float brg, float crsangle);

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

private:
  void fillWithTexture(GLuint texId);

  void moveCoursor(const QPoint &pos, bool repaint = true);

  bool _initialized;
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
};

#endif // RLIDISPLAYWIDGET_H
