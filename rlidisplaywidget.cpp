#include "rlidisplaywidget.h"
#include "mainwindow.h"

#include <cmath>
#include <qmath.h>
#include <QDebug>
#include <QDateTime>
#include <QApplication>

#include "rlimath.h"
#include "rlicontrolevent.h"

RLIDisplayWidget::RLIDisplayWidget(QWidget *parent) : QGLWidget(parent) {
  _world_coords = QVector2D(12.5000f, -81.6000f);
  _fonts = new AsmFonts();

  _chartEngine = new ChartEngine();
  _radarEngine = new RadarEngine(4096, 800);
  _maskEngine = new MaskEngine(size());
  _maskEngine->setCursorPos(_maskEngine->getCenter());
  _infoEngine = new InfoEngine();
  _menuEngine = new MenuEngine(QSize(12, 14));
  _targetEngine = new TargetEngine();
  _routeEngine = new RouteEngine();

  _controlsEngine = new ControlsEngine();
  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  _initialized = false;
  _route_edition = false;
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

  delete _fonts;
}

void RLIDisplayWidget::onStartRouteEdit() {
  _routeEngine->clearCurrentRoute();
  _routeEngine->addPointToCurrent(_world_coords);
  _route_edition = true;
}

void RLIDisplayWidget::onAddRoutePoint() {
  QPointF pos = _controlsEngine->getVdVnIntersection();
  float scale = (_scale*1852.f) / _maskEngine->getRadius();
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

  // Disable multisampling
  glDisable(GL_MULTISAMPLE);

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

  setMouseTracking(true);

  emit initialized();
  _initialized = true;
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "GL init finish";
}

void RLIDisplayWidget::resizeGL(int w, int h) {
  if (!_initialized)
    return;

  glViewport(0, 0, w, h);

  _maskEngine->resize(QSize(w, h));

  _maskEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  _radarEngine->resizeTexture(_maskEngine->getRadius());
  _chartEngine->resize(_maskEngine->getRadius());

  emit resized(QSize(w, h));
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


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _chartEngine->getTextureId());

  glBegin(GL_QUADS);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( radar_rad,-radar_rad, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-radar_rad,-radar_rad, 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);


  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _radarEngine->getTextureId());

  glBegin(GL_QUADS);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(-radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( radar_rad,-radar_rad, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-radar_rad,-radar_rad, 0.0f);
  glEnd();


  glBindTexture(GL_TEXTURE_2D, 0);

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  QPoint center = _controlsEngine->getCenterPos();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(center.x()+.5f, center.y()+.5f, 0);

  //_scale - радиус дырки в маске в пикселях
  // scale в update - метров/пиксель
  float scale = (_scale*1852.f) / _maskEngine->getRadius();


  if (_route_edition) {
    QVector2D _route_last = _routeEngine->getLastPoint();
    QPointF v_pos = RLIMath::coords_to_pos(_world_coords, _route_last, QPoint(0, 0), scale);
    _controlsEngine->setVisorShift(v_pos.toPoint());
  } else {
    _controlsEngine->setVisorShift(QPoint(0, 0));
  }

  _controlsEngine->draw();




  _targetEngine->draw(_world_coords, scale);
  _routeEngine->draw(_world_coords, scale);

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();


  fillWithTexture(_maskEngine->getTextureId());


  for (int i = 0; i < _infoEngine->getBlockCount(); i++) {
    QRectF blockRect = _infoEngine->getBlockGeometry(i);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _infoEngine->getBlockTextId(i));

    glBegin(GL_QUADS);
    glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(blockRect.left(),  blockRect.bottom(), 0.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(blockRect.right(), blockRect.bottom(), 0.0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(blockRect.right(), blockRect.top(), 0.0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(blockRect.left(),  blockRect.top(), 0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _menuEngine->getTextureId());

  QSize menuSize = _menuEngine->size();

  glBegin(GL_QUADS);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(width() - menuSize.width() - 5, 149 + menuSize.height(), 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(width() - 5, 149 + menuSize.height(), 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(width() - 5, 149, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(width() - menuSize.width() - 5, 149, 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glFlush();


  glDisable(GL_BLEND);
  glEnable(GL_DEPTH);
  glEnable(GL_DEPTH_TEST);
  _radarEngine->updateTexture();
  glDisable(GL_DEPTH);
  glDisable(GL_DEPTH_TEST);

  _maskEngine->update();
  _menuEngine->update();
  _infoEngine->update();

  glEnable(GL_BLEND);
  _chartEngine->update(_world_coords, scale, 0.f, center-hole_center);
}



void RLIDisplayWidget::fillWithTexture(GLuint texId) {
  glShadeModel( GL_FLAT );
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texId);

  // Draw The Quad
  glBegin(GL_QUADS);
  glColor3f(1.f, 1.f, 1.f);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(0.0f, float(height()), 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( float(width()), float(height()), 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( float(width()), 0.0f, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(0.0f, 0.0f, 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);
}

void RLIDisplayWidget::mousePressEvent(QMouseEvent* e) {
  if (QLineF(e->pos() , _maskEngine->getCenter()).length() < _maskEngine->getRadius())
    moveCoursor(e->pos());
}

void RLIDisplayWidget::mouseMoveEvent(QMouseEvent* e) {
  if (QLineF(e->pos() , _maskEngine->getCenter()).length() < _maskEngine->getRadius())
    moveCoursor(e->pos(), false);
}

void RLIDisplayWidget::moveCoursor(const QPoint& pos, bool repaint) {
  QPointF cen = _controlsEngine->getCenterPos();
  float scale = (_scale*1852.f) / _maskEngine->getRadius();
  QVector2D cursor_coords = RLIMath::pos_to_coords(QVector2D(12.5000f, -81.6000f), cen, pos, scale);
  emit cursor_moved(cursor_coords);


  const char * dist_fmt = NULL;
  if(repaint)
    _controlsEngine->setCursorPos(pos);

  float peleng = 90.0 * qAtan2(pos.x() - cen.x(), - pos.y() + cen.y()) / acos(0);
  if (peleng < 0)
    peleng = 360 + peleng;

  float distance = sqrt(pow(pos.y() - cen.y(), 2) + pow(pos.x() - cen.x(), 2));

  foreach(QWidget * w, QApplication::topLevelWidgets()) {
    MainWindow * mainWnd = dynamic_cast<MainWindow *>(w);
    if(mainWnd) {
      float ratio = 1;
      RadarScale * curscale = mainWnd->getRadarScale();
      if(curscale) {
        const rli_scale_t * scale = curscale->getCurScale();
        if(scale) {
          ratio = scale->len / maskEngine()->getRadius();
          dist_fmt = scale->val_fmt;
        }
      }
      distance *= ratio;
      break;
    }
  }

  emit cursor_moved(peleng, distance, dist_fmt);
}


void RLIDisplayWidget::mouseReleaseEvent(QMouseEvent* e) {
  Q_UNUSED(e);
}

void RLIDisplayWidget::wheelEvent(QWheelEvent * e)
{
    int numDegrees = e->delta() / 8;
    int numSteps = numDegrees / 15;

    if (e->orientation() == Qt::Horizontal) {
        RLIControlEvent* evt = new RLIControlEvent(RLIControlEvent::NoButton
                                               , RLIControlEvent::VN
                                               , (float)numSteps / 10.0);
        qApp->postEvent(this, evt);
    } else {
        RLIControlEvent* evt = new RLIControlEvent(RLIControlEvent::NoButton
                                               , RLIControlEvent::VD
                                               , numSteps);
        qApp->postEvent(this, evt);
        //vd_pos = pos;
    }
    e->accept();
}

bool RLIDisplayWidget::event(QEvent* e) {
  if (e->type() == QEvent::User + 111) {
    RLIControlEvent* re = static_cast<RLIControlEvent*>(e);
    QPoint cursor_pos;

    switch (re->button()) {
      case RLIControlEvent::NoButton:
        break;
      case RLIControlEvent::ButtonMinus:
        break;
      case RLIControlEvent::ButtonPlus:
        break;
      case RLIControlEvent::CenterShift:
        cursor_pos = _controlsEngine->getCursorPos();
        if (QLineF(cursor_pos, _maskEngine->getCenter()).length() < _maskEngine->getRadius()) {
          if(_controlsEngine->getCenterPos() != _maskEngine->getCenter())
              cursor_pos = _maskEngine->getCenter();
          _maskEngine->setCursorPos(cursor_pos);
          _controlsEngine->setCenterPos(cursor_pos);

          QPoint hole_center = _maskEngine->getCenter();
          _radarEngine->shiftCenter(cursor_pos-hole_center);
          moveCoursor(cursor_pos);
        }
        break;
      case RLIControlEvent::ParallelLines:
        _controlsEngine->setPlVisibility(!_controlsEngine->isPlVisible());
        break;
      case RLIControlEvent::Menu:
        if (_menuEngine->state() == MenuEngine::MAIN)
          _menuEngine->setState(MenuEngine::DISABLED);
        else
          _menuEngine->setState(MenuEngine::MAIN);
        break;
    case RLIControlEvent::ConfigMenu:
      if (_menuEngine->state() == MenuEngine::CONFIG)
        _menuEngine->setState(MenuEngine::DISABLED);
      else
        _menuEngine->setState(MenuEngine::CONFIG);
      break;
      case RLIControlEvent::Up:
        if (_route_edition) {
          onAddRoutePoint();
          break;
        }

        if (_menuEngine->visible())
          _menuEngine->onUp();
        break;
      case RLIControlEvent::Down:
        if (_menuEngine->visible())
          _menuEngine->onDown();
        break;
      case RLIControlEvent::Enter:
        if (_menuEngine->visible()) {
          _menuEngine->onEnter();
        } else {
          QPointF pos = _controlsEngine->getCursorPos();
          QPointF cen = _controlsEngine->getCenterPos();
          float scale = (_scale*1852.f) / _maskEngine->getRadius();
          QVector2D cursor_coords = RLIMath::pos_to_coords(_world_coords, cen, pos, scale);
          _targetEngine->trySelect(cursor_coords, scale);
        }
        break;
      case RLIControlEvent::Back:
        if (_route_edition) {
          _routeEngine->removePointFromCurrent();
          break;
        }

        if (_menuEngine->visible())
          _menuEngine->onBack();
        break;
      default:
        break;
    }

    switch (re->spinner()) {
      case RLIControlEvent::NoSpinner:
        break;
      case RLIControlEvent::VN:
    {
        _controlsEngine->shiftVnCu(re->spinnerVal()); //shiftVnP(re->spinnerVal());
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
        break;
    }
      case RLIControlEvent::VD:
        _controlsEngine->shiftVd(re->spinnerVal());
        foreach(QWidget * w, QApplication::topLevelWidgets())
        {
            MainWindow * mainWnd = dynamic_cast<MainWindow *>(w);
            if(mainWnd)
            {
                float ratio = 1;
                const char * fmt = NULL;
                RadarScale * curscale = mainWnd->getRadarScale();
                if(curscale)
                {
                    const rli_scale_t * scale = curscale->getCurScale();
                    if(scale)
                    {
                        ratio = scale->len / maskEngine()->getRadius();
                        fmt = scale->val_fmt;
                    }
                }
                emit displayVNDistance(_controlsEngine->getVd() * ratio, fmt);
                break;
            }
        }

        break;
      default:
        break;
    }
  }

  return QGLWidget::event(e);
}

void RLIDisplayWidget::setWorldCoords(QVector2D coords)
{
    _world_coords = coords;
}

QVector2D RLIDisplayWidget::getWorldCoords(void) const
{
    return _world_coords;
}
