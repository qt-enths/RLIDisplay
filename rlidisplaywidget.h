#ifndef RLIDISPLAYWIDGET_H
#define RLIDISPLAYWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

#include "radarengine.h"
#include "infoengine.h"
#include "menuengine.h"
#include "maskengine.h"
#include "controlsengine.h"

class RLIDisplayWidget : public QGLWidget, protected QGLFunctions
{
  Q_OBJECT
public:
  explicit RLIDisplayWidget(QWidget *parent = 0);
  ~RLIDisplayWidget();

  inline RadarEngine* radarEngine() { return _radarEngine; }
  inline MaskEngine* maskEngine() { return _maskEngine; }
  inline InfoEngine* infoEngine() { return _infoEngine; }
  inline MenuEngine* menuEngine() { return _menuEngine; }

signals:
  void initialized();
  void cursor_moved(float peleng, float distance);
  void per_second();
  void resized(const QSize& s);

protected slots:
  void mousePressEvent(QMouseEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void mouseReleaseEvent(QMouseEvent* e);

  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

  bool event(QEvent* e);

private:
  void fillWithTexture(GLuint texId);

  void moveCoursor(const QPoint& pos);

  bool _initialized;
  int  _last_second;

  AsmFonts* _fonts;

  RadarEngine* _radarEngine;
  InfoEngine* _infoEngine;
  MaskEngine* _maskEngine;
  MenuEngine* _menuEngine;

  ControlsEngine* _controlsEngine;
};

#endif // RLIDISPLAYWIDGET_H
