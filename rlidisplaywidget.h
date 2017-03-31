#ifndef RLIDISPLAYWIDGET_H
#define RLIDISPLAYWIDGET_H

#include <QWidget>
#include <QMouseEvent>
#include <QtOpenGL/QGLWidget>
#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

#include "s52/chartmanager.h"

#include "datasources/radarscale.h"

#include "layers/radarengine.h"
#include "layers/infoengine.h"
#include "layers/menuengine.h"
#include "layers/maskengine.h"
#include "layers/chartengine.h"
#include "layers/targetengine.h"
#include "layers/controlsengine.h"
#include "layers/routeengine.h"
#include "layers/magnifierengine.h"

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
  inline MagnifierEngine* magnifierEngine() { return _magnifierEngine; }

  inline void setChartManager(ChartManager* mngr) { _chrt_mngr = mngr; }

  void setWorldCoords(QVector2D coords);
  QVector2D getWorldCoords(void) const;

signals:
  void initialized();

  void cursor_moved(float peleng, float distance, const char* dist_fmt);
  void cursor_moved(QVector2D);

  void per_second();

  void resized(const QSize& s);

  void displayVNDistance(float dist, const char * fmt);
  void displaydBRG(float brg, float crsangle);

  void band_changed(char** band);


public slots:
  // Обработка кнопок
  void onMenuToggled();
  void onConfigMenuToggled();
  void onUpToggled();
  void onDownToggled();
  void onEnterToggled();
  void onCenterShiftToggled();
  void onParallelLinesToggled();
  void onBackToggled();
  void onMagnifierToggled();

  void onVnChanged(float val);
  void onVdChanged(float val);
  // Обработка кнопок

  void onCoordsChanged(const QVector2D& new_coords);
  void onHeadingChanged(float hdg);
  void onBandMenu(const QByteArray band);
  void onScaleChanged(RadarScale scale);

protected slots:
  void mousePressEvent(QMouseEvent* e);
  void mouseMoveEvent(QMouseEvent* e);
  void wheelEvent(QWheelEvent * e);

  void initializeGL();
  void resizeGL(int w, int h);
  void paintGL();

  void new_chart(const QString& name);

  void onStartRouteEdit();
  void onAddRoutePoint();
  void onFinishRouteEdit();

private:
  void fillWithTexture(GLuint texId);

  void moveCoursor(const QPoint &pos, bool repaint = true, RadarScale* curscale = NULL);

  bool _initialized;

  bool _route_edition;
  bool _is_magnifier_visible;

  int  _last_second;
  rli_scale_t _rli_scale;

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

  MagnifierEngine* _magnifierEngine;

  QVector2D _world_coords;
};

#endif // RLIDISPLAYWIDGET_H
