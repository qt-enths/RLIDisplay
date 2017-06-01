#include "rlidisplaywidget.h"
#include "mainwindow.h"

#include <cmath>
#include <qmath.h>
#include <QDebug>
#include <QDateTime>
#include <QApplication>

#include "common/rlimath.h"

RLIDisplayWidget::RLIDisplayWidget(QWidget *parent) : QGLWidget(parent) {
  _world_coords = QVector2D(12.5000f, -81.6000f);
  _fonts = new AsmFonts();

  QSize screen_size = RLIConfig::instance().currentSize();
  const RLILayout* layout = RLIConfig::instance().currentLayout();

  _maskEngine = new MaskEngine(size(), layout->circle, this);
  _chartEngine = new ChartEngine();
  _radarEngine = new RadarEngine(8192, 800);
  _infoEngine = new InfoEngine();

  _menuEngine = new MenuEngine(screen_size, layout->menu, this);
  _magnifierEngine = new MagnifierEngine(screen_size, layout->magnifier, this);

  _targetEngine = new TargetEngine();
  _routeEngine = new RouteEngine();

  _menuEngine->setRouteEngine(_routeEngine);

  _controlsEngine = new ControlsEngine();
  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  _initialized = false;
  _route_edition = false;
  _is_magnifier_visible = false;
}

RLIDisplayWidget::~RLIDisplayWidget() {
  delete _chartEngine;
  delete _radarEngine;
  delete _maskEngine;
  delete _menuEngine;
  delete _infoEngine;

  delete _targetEngine;
  delete _controlsEngine;
  delete _routeEngine;

  delete _magnifierEngine;

  delete _fonts;
}

void RLIDisplayWidget::onStartRouteEdit() {
  _routeEngine->clearCurrentRoute();
  _routeEngine->addPointToCurrent(_world_coords);
  _route_edition = true;
}

void RLIDisplayWidget::onScaleChanged(RadarScale scale) {
  _rli_scale = *scale.current;
}

void RLIDisplayWidget::onAddRoutePoint() {
  QPointF pos = _controlsEngine->getVdVnIntersection();
  float scale = (_rli_scale.len*1852.f) / _maskEngine->getRadius();
  QVector2D last_route_point = _routeEngine->getLastPoint();
  /*QPointF pos = _controlsEngine->getCursorPos();
  QPointF cen = _controlsEngine->getCenterPos();
  QVector2D cursor_coords = RLIMath::pos_to_coords(_world_coords, cen, pos, scale);*/
  QVector2D cursor_coords = RLIMath::pos_to_coords(last_route_point, QPoint(0, 0), pos, scale);
  _routeEngine->addPointToCurrent(cursor_coords);
}

void RLIDisplayWidget::onFinishRouteEdit() {
  _route_edition = false;
}

void RLIDisplayWidget::onCoordsChanged(const QVector2D& new_coords) {
  _world_coords = new_coords;
}

void RLIDisplayWidget::onHeadingChanged(float hdg) {
  _controlsEngine->setVnP(hdg);
  _maskEngine->setAngleShift(hdg);

  if (hdg >= 0 && hdg <= 360) {
    _radarEngine->shiftNorth(static_cast<uint>((hdg*_radarEngine->pelengCount())/360.f));
  }
}


void RLIDisplayWidget::onBandMenu(const QByteArray band) {
  const char* pband = band.data();
  int bandnum = sizeof(RLIStrings::bandArray) / sizeof(RLIStrings::bandArray[0][0]) / 2;

  for(int i = 0; i < bandnum; i++)
    for(int j = 0; j < RLI_LANG_COUNT; j++)
      if(strcmp(pband, RLIStrings::bandArray[i][j]) == 0)
        switch(i) {
        case 0:
          emit band_changed(RLIStrings::nBandX);
          break;
        case 1:
          emit band_changed(RLIStrings::nBandS);
          break;
        case 2:
          emit band_changed(RLIStrings::nBandK);
          break;
        }
}


void RLIDisplayWidget::new_chart(const QString& name) {
  if (!_initialized)
    return;

  if (name == "CO200008.000")
    _chartEngine->setChart(_chrt_mngr->getChart(name), _chrt_mngr->refs());
}

void RLIDisplayWidget::initializeGL() {
  if (_initialized)
    return;

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "GL init start";
  initializeGLFunctions();

  qDebug() << "Vendor: " << (const char*) glGetString(GL_VENDOR);
  qDebug() << "Renderer: " << (const char*) glGetString(GL_RENDERER);
  qDebug() << "OpenGL: " << (const char*) glGetString(GL_VERSION);
  qDebug() << "Shaders: " << (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
  //qDebug() << "Extensions: " << (const char*) glGetString(GL_EXTENSIONS);


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Fonts loading start";
  _fonts->init(context(), ":/res/fonts");
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Fonts loading finish";

  // Disable depth and stencil buffers (we need only 2D)
  glDisable(GL_STENCIL);
  glEnable(GL_DEPTH);
  glEnable(GL_DEPTH_TEST);

  // Enable multisampling
  glEnable(GL_MULTISAMPLE);

  // Disable Lighting
  glDisable(GL_LIGHTING);

  // Enable textures
  glEnable(GL_TEXTURE_2D);

  // Enable alpha blending
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  // Enable point sprites
  glEnable(GL_POINT_SPRITE);
  glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

  // Enable smooth geometry
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_POLYGON_SMOOTH);

  glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
  glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Chart engine init start";
  if (!_chartEngine->init(_chrt_mngr->refs(), context()))
    return;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Chart engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Radar engine init start";
  if (!_radarEngine->init(context()))
    return;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Radar engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Mask engine init start";
  if (!_maskEngine->init(context()))
    return;
  _maskEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Mask engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Info engine init start";
  if (!_infoEngine->init(context()))
    return;
  _infoEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Info engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Menu engine init start";
  if (!_menuEngine->init(context()))
    return;
  _menuEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Menu engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Target engine init start";
  if (!_targetEngine->init(context()))
    return;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Target engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Route engine init start";
  if (!_routeEngine->init(context()))
    return;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Route engine init finish";


  if (!_controlsEngine->init(context()))
    return;

  if (!_magnifierEngine->init(context()))
    return;

  setMouseTracking(true);

  emit initialized();
  _initialized = true;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "GL init finish";
}

void RLIDisplayWidget::resizeGL(int w, int h) {
  Q_UNUSED(w);
  Q_UNUSED(h);

  if (!_initialized)
    return;

  QSize screen_size = RLIConfig::instance().currentSize();
  const RLILayout* layout = RLIConfig::instance().currentLayout();

  _maskEngine->resize(RLIConfig::instance().currentSize(), layout->circle);
  _maskEngine->update();

  _menuEngine->resize(screen_size, layout->menu);
  _magnifierEngine->resize(screen_size, layout->magnifier);

  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  _radarEngine->resizeTexture(_maskEngine->getRadius());
  _chartEngine->resize(_maskEngine->getRadius());
}

void RLIDisplayWidget::paintGL() {
  if (!_initialized)
    return;

  int curr_second = QTime::currentTime().second();
  if (curr_second != _last_second) {
    _last_second = curr_second;

    emit per_second();
  }

  glViewport(0, 0, width(), height());

  // Fill widget with black
  glClearColor(0.33f, 0.33f, 0.33f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Push back the current matrices and go orthographic for background rendering.
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, width(), height(), 0, -1, 1 );

  QPoint hole_center = _maskEngine->getCenter();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(hole_center.x()+.5f, hole_center.y()+.5f, 0);

  float radar_rad = _radarEngine->getSize() / 2.f;

  QRectF radarRect(-radar_rad, -radar_rad, 2*radar_rad, 2*radar_rad);

  fillRectWithTexture(radarRect, _chartEngine->getTextureId());
  fillRectWithTexture(radarRect, _radarEngine->getTextureId());

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  QPoint center = _controlsEngine->getCenterPos();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(center.x()+.5f, center.y()+.5f, 0);

  //_scale - радиус дырки в маске в пикселях
  // scale в update - метров/пиксель
  float scale = (_rli_scale.len*1852.f) / _maskEngine->getRadius();

  if (_route_edition) {
    QVector2D _route_last = _routeEngine->getLastPoint();
    QPointF v_pos = RLIMath::coords_to_pos(_world_coords, _route_last, QPoint(0, 0), scale);
    _controlsEngine->setVisorShift(v_pos.toPoint());
  } else {
    _controlsEngine->setVisorShift(QPoint(0, 0));
  }

  glEnable(GL_BLEND);

  _controlsEngine->draw();

  _targetEngine->draw(_world_coords, scale);
  _routeEngine->draw(_world_coords, scale);

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  fillWithTexture(_maskEngine->getTextureId());


  for (int i = 0; i < _infoEngine->getBlockCount(); i++)
    fillRectWithTexture(_infoEngine->getBlockGeometry(i), _infoEngine->getBlockTextId(i));


  QRectF menuRect(_menuEngine->position(), _menuEngine->size());
  fillRectWithTexture(menuRect, _menuEngine->getTextureId());

  if (_is_magnifier_visible) {
    QRectF magRect(_magnifierEngine->position(), _magnifierEngine->size());
    fillRectWithTexture(magRect, _magnifierEngine->getTextureId());
  }

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();


  _radarEngine->updateTexture();

  _maskEngine->update();

  _menuEngine->update();
  _magnifierEngine->update();

  _infoEngine->update();

  glEnable(GL_BLEND);
  _chartEngine->update(_world_coords, scale, 0.f, center-hole_center);


  glFlush();
}

void RLIDisplayWidget::fillRectWithTexture(const QRectF& rect, GLuint texId) {
  glShadeModel( GL_FLAT );

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texId);

  // Draw The Quad
  glBegin(GL_QUADS);
  glColor3f(1.f, 1.f, 1.f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(rect.left(), rect.bottom(), 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(rect.right(), rect.bottom(), 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(rect.right(), rect.top(), 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(rect.left(), rect.top(), 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);
}

void RLIDisplayWidget::fillWithTexture(GLuint texId) {
  fillRectWithTexture(geometry(), texId);
}

void RLIDisplayWidget::mousePressEvent(QMouseEvent* e) {
  if (QLineF(e->pos() , _maskEngine->getCenter()).length() < _maskEngine->getRadius())
    moveCoursor(e->pos());
}

void RLIDisplayWidget::mouseMoveEvent(QMouseEvent* e) {
  if (QLineF(e->pos() , _maskEngine->getCenter()).length() < _maskEngine->getRadius())
    moveCoursor(e->pos(), false);
}

void RLIDisplayWidget::moveCoursor(const QPoint& pos, bool repaint, RadarScale* curscale) {
  QPointF cen = _controlsEngine->getCenterPos();
  float scale = (_rli_scale.len*1852.f) / _maskEngine->getRadius();
  QVector2D cursor_coords = RLIMath::pos_to_coords(QVector2D(12.5000f, -81.6000f), cen, pos, scale);
  emit cursor_moved(cursor_coords);

  const char * dist_fmt = NULL;
  if(repaint)
    _controlsEngine->setCursorPos(pos);

  float peleng = 90.0 * qAtan2(pos.x() - cen.x(), - pos.y() + cen.y()) / acos(0);
  if (peleng < 0)
    peleng = 360 + peleng;

  float distance = sqrt(pow(pos.y() - cen.y(), 2) + pow(pos.x() - cen.x(), 2));

  float ratio = 1;
  if(curscale) {
    const rli_scale_t * scale = curscale->getCurScale();
    if(scale) {
      ratio = scale->len / maskEngine()->getRadius();
      dist_fmt = scale->val_fmt;
    }
  }
  distance *= ratio;

  emit cursor_moved(peleng, distance, dist_fmt);
}


void RLIDisplayWidget::wheelEvent(QWheelEvent * e) {
  int numDegrees = e->delta() / 8;
  int numSteps = numDegrees / 15;

  if (e->orientation() == Qt::Horizontal)
    onVnChanged((float)numSteps / 10.0);
  else
    onVnChanged((float)numSteps);

  e->accept();
}


void RLIDisplayWidget::onMenuToggled() {
  if (_is_magnifier_visible)
    return;

  if (_menuEngine->state() == MenuEngine::MAIN)
    _menuEngine->setState(MenuEngine::DISABLED);
  else
    _menuEngine->setState(MenuEngine::MAIN);
}

void RLIDisplayWidget::onConfigMenuToggled() {
  if (_is_magnifier_visible)
    return;

  if (_menuEngine->state() == MenuEngine::CONFIG)
    _menuEngine->setState(MenuEngine::DISABLED);
  else
    _menuEngine->setState(MenuEngine::CONFIG);
}

void RLIDisplayWidget::onMagnifierToggled() {
  if (!_menuEngine->visible())
    _is_magnifier_visible = !_is_magnifier_visible;
}

void RLIDisplayWidget::onUpToggled() {
  if (_route_edition) {
    onAddRoutePoint();
    return;
  }

  if (_menuEngine->visible())
    _menuEngine->onUp();
}

void RLIDisplayWidget::onDownToggled() {
  if (_menuEngine->visible())
    _menuEngine->onDown();
}

void RLIDisplayWidget::onEnterToggled() {
  if (_menuEngine->visible()) {
    _menuEngine->onEnter();
  } else {
    QPointF pos = _controlsEngine->getCursorPos();
    QPointF cen = _controlsEngine->getCenterPos();
    float scale = (_rli_scale.len*1852.f) / _maskEngine->getRadius();
    QVector2D cursor_coords = RLIMath::pos_to_coords(_world_coords, cen, pos, scale);
    _targetEngine->trySelect(cursor_coords, scale);
  }
}

void RLIDisplayWidget::onCenterShiftToggled() {
  QPoint cursor_pos = _controlsEngine->getCursorPos();
  if (QLineF(cursor_pos, _maskEngine->getCenter()).length() < _maskEngine->getRadius()) {
    if(_controlsEngine->getCenterPos() != _maskEngine->getCenter())
        cursor_pos = _maskEngine->getCenter();
    _maskEngine->setCursorPos(cursor_pos);
    _controlsEngine->setCenterPos(cursor_pos);

    QPoint hole_center = _maskEngine->getCenter();
    _radarEngine->shiftCenter(cursor_pos-hole_center);
    moveCoursor(cursor_pos);
  }
}

void RLIDisplayWidget::onParallelLinesToggled() {
  _controlsEngine->setPlVisibility(!_controlsEngine->isPlVisible());
}

void RLIDisplayWidget::onBackToggled() {
  if (_route_edition) {
    _routeEngine->removePointFromCurrent();
    return;
  }

  if (_menuEngine->visible())
    _menuEngine->onBack();
}

void RLIDisplayWidget::onVnChanged(float val) {
  _controlsEngine->shiftVnCu(val); //shiftVnP(re->spinnerVal());
  //_maskEngine->setAngleShift(_controlsEngine->getVnP());
  float brg = _controlsEngine->getVnCu();
  if(brg < 0)
      brg = 360.0 + brg;

  float crsangle;
  float hdg = _controlsEngine->getVnP();
  float counterhdg;
  if(hdg <= 180)
    counterhdg = hdg + 180;
  else
    counterhdg = 180 - hdg;
  if((brg >= hdg) && (brg <= counterhdg))
    crsangle = brg - hdg;
  else
    crsangle = (-1) * (360 - brg + hdg);
  emit displaydBRG(brg, crsangle);
}

void RLIDisplayWidget::onVdChanged(float val) {
  _controlsEngine->shiftVd(val);

  float ratio = 1;
  const char* fmt = NULL;

  ratio = _rli_scale.len / maskEngine()->getRadius();
  fmt = _rli_scale.val_fmt;

  emit displayVNDistance(_controlsEngine->getVd() * ratio, fmt);
}

void RLIDisplayWidget::setWorldCoords(QVector2D coords) {
  _world_coords = coords;
}

QVector2D RLIDisplayWidget::getWorldCoords(void) const {
  return _world_coords;
}
