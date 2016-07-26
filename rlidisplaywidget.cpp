#include "rlidisplaywidget.h"

#include <cmath>
#include <qmath.h>
#include <QDebug>
#include <QDateTime>

#include "rlicontrolevent.h"

RLIDisplayWidget::RLIDisplayWidget(QWidget *parent) : QGLWidget(parent) {
  _fonts = new AsmFonts();

  _radarEngine = new RadarEngine(4096, 1024);
  _maskEngine = new MaskEngine(size());
  _maskEngine->setCursorPos(_maskEngine->getCenter());
  _infoEngine = new InfoEngine();
  _menuEngine = new MenuEngine(QSize(12, 14));

  _controlsEngine = new ControlsEngine();
  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  _initialized = false;
}

RLIDisplayWidget::~RLIDisplayWidget() {
  delete _radarEngine;
  delete _maskEngine;
  delete _infoEngine;

  delete _fonts;
}

void RLIDisplayWidget::initializeGL() {
  if (_initialized)
    return;

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "GL init start";
  initializeGLFunctions();

  qDebug() << "Vendor: " << (const char*) glGetString(GL_VENDOR);
  qDebug() << "Renderer: " << (const char*) glGetString(GL_RENDERER);
  qDebug() << "OpenGL: " << (const char*) glGetString(GL_VERSION);
  qDebug() << "Shaders: " << (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION);
  //qDebug() << "Extensions: " << (const char*) glGetString(GL_EXTENSIONS);

  /*GLint ival[2];
  glGetIntegerv(GL_MAX_SAMPLES, ival);
  qDebug() << "Max sample count: " << ival[0];

  glGetIntegerv(GL_MAX_FRAMEBUFFER_LAYERS, ival);
  qDebug() << "Max framebuffer layers: " << ival[0];

  glGetIntegerv(GL_MAX_FRAMEBUFFER_HEIGHT, ival);
  qDebug() << "Max framebuffer height: " << ival[0];

  glGetIntegerv(GL_MAX_FRAMEBUFFER_WIDTH, ival);
  qDebug() << "Max framebuffer width: " << ival[0];

  glGetIntegerv(GL_MAX_FRAMEBUFFER_SAMPLES, ival);
  qDebug() << "Max framebuffer samples: " << ival[0];

  glGetIntegerv(GL_ALIASED_POINT_SIZE_RANGE, ival);
  qDebug() << "Aliased point size range: " << (QString::number(ival[0]) + ":" + QString::number(ival[1])).toLatin1();

  glGetIntegerv(GL_SMOOTH_POINT_SIZE_RANGE, ival);
  qDebug() << "Smooth point size range: " << (QString::number(ival[0]) + ":" + QString::number(ival[1])).toLatin1();

  glGetIntegerv(GL_ALIASED_LINE_WIDTH_RANGE, ival);
  qDebug() << "Aliased line width range: " << (QString::number(ival[0]) + ":" + QString::number(ival[1])).toLatin1();

  glGetIntegerv(GL_SMOOTH_LINE_WIDTH_RANGE, ival);
  qDebug() << "Smooth line width range: " << (QString::number(ival[0]) + ":" + QString::number(ival[1])).toLatin1();

  glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, ival);
  qDebug() << "Max combined texture image units: " << ival[0];*/



  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Fonts loading start";
  _fonts->init(context(), ":/res/fonts");
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Fonts loading finish";

  // Disable depth and stencil buffers (we need only 2D)
  glDisable(GL_STENCIL);
  glEnable(GL_DEPTH);
  glEnable(GL_DEPTH_TEST);

  // Disable multisampling
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

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Radar engine init start";
  if (!_radarEngine->init(context()))
    return;
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Radar engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Mask engine init start";
  if (!_maskEngine->init(context()))
    return;
  _maskEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Mask engine init finish";


  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Info engine init start";
  if (!_infoEngine->init(context()))
    return;
  _infoEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Info engine init finish";

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Info engine init start";
  if (!_menuEngine->init(context()))
    return;
  _menuEngine->setFonts(_fonts);
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Info engine init finish";


  if (!_controlsEngine->init(context()))
    return;

  emit initialized();
  _initialized = true;
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "GL init finish";
}

void RLIDisplayWidget::resizeGL(int w, int h) {
  if (!_initialized)
    return;

  glViewport(0, 0, w, h);

  _maskEngine->resize(QSize(w, h));

  _maskEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCursorPos(_maskEngine->getCenter());
  _controlsEngine->setCenterPos(_maskEngine->getCenter());

  glDisable(GL_BLEND);
  _maskEngine->update();
  glEnable(GL_BLEND);

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

  glDisable(GL_BLEND);
  glDisable(GL_MULTISAMPLE);
  glEnable(GL_DEPTH);
  glEnable(GL_DEPTH_TEST);
  _radarEngine->updateTexture();
  glDisable(GL_DEPTH);
  glDisable(GL_DEPTH_TEST);

  _maskEngine->update();
  _menuEngine->update();
  _infoEngine->update();

  glViewport(0, 0, width(), height());

  glEnable(GL_BLEND);

  // Fill widget with black
  glClearColor(0.33f, 0.33f, 0.33f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Push back the current matrices and go orthographic for background rendering.
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, width(), height(), 0, -1, 1 );

  QPoint center = _controlsEngine->getCenterPos();

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(center.x()+.5f, center.y()+.5f, 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _radarEngine->getTextureId());

  float radar_rad = _radarEngine->getSize() / 2.f;
  int mRadius = _maskEngine->getRadius() + 1;
  QPoint mCenter = _maskEngine->getCenter();

  int l = -mRadius + (mCenter.x() - center.x());
  int r = mRadius + (mCenter.x() - center.x());
  int b = mRadius + (mCenter.y() - center.y());
  int t = -mRadius + (mCenter.y() - center.y());

  float tl = 0.5 - abs(l) / (2.f * radar_rad);
  float tr = 0.5 + abs(r) / (2.f * radar_rad);
  float tb = 0.5 - abs(b) / (2.f * radar_rad);
  float tt = 0.5 + abs(t) / (2.f * radar_rad);

  glBegin(GL_QUADS);
  glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
  glTexCoord2f(tl, tb); glVertex3f(l, b, 0.0f);
  glTexCoord2f(tr, tb); glVertex3f(r, b, 0.0f);
  glTexCoord2f(tr, tt); glVertex3f(r, t, 0.0f);
  glTexCoord2f(tl, tt); glVertex3f(l, t, 0.0f);

  glTexCoord2f(0.0f, 0.0f); glVertex3f(-radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f( radar_rad, radar_rad, 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f( radar_rad,-radar_rad, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(-radar_rad,-radar_rad, 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);

  glEnable(GL_MULTISAMPLE);
  _controlsEngine->draw();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glDisable(GL_MULTISAMPLE);
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
  glTexCoord2f(0.0f, 0.0f); glVertex3f(width() - menuSize.width() - 4, 100 + menuSize.height(), 0.0f);
  glTexCoord2f(1.0f, 0.0f); glVertex3f(width() - 4, 100 + menuSize.height(), 0.0f);
  glTexCoord2f(1.0f, 1.0f); glVertex3f(width() - 4, 100, 0.0f);
  glTexCoord2f(0.0f, 1.0f); glVertex3f(width() - menuSize.width() - 4, 100, 0.0f);
  glEnd();

  glBindTexture(GL_TEXTURE_2D, 0);

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glFlush();
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
    moveCoursor(e->pos());
}

void RLIDisplayWidget::moveCoursor(const QPoint& pos) {
  _controlsEngine->setCursorPos(pos);
  QPointF cen = _controlsEngine->getCenterPos();

  float peleng = 90.0 * qAtan2(pos.x() - cen.x(), - pos.y() + cen.y()) / acos(0);
  if (peleng < 0)
    peleng = 360 + peleng;

  float distance = sqrt(pow(pos.y() - cen.y(), 2) + pow(pos.x() - cen.x(), 2));

  emit cursor_moved(peleng, distance);
}


void RLIDisplayWidget::mouseReleaseEvent(QMouseEvent* e) {
  Q_UNUSED(e);
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
          _maskEngine->setCursorPos(cursor_pos);
          _controlsEngine->setCenterPos(cursor_pos);
          moveCoursor(cursor_pos);
        }
        break;
      case RLIControlEvent::ParallelLines:
        _controlsEngine->setPlVisibility(!_controlsEngine->isPlVisible());
        break;
      case RLIControlEvent::Menu:
        _menuEngine->setVisibility(!_menuEngine->visible());
        break;
      case RLIControlEvent::Up:
        if (_menuEngine->visible())
          _menuEngine->onUp();
        break;
      case RLIControlEvent::Down:
        if (_menuEngine->visible())
          _menuEngine->onDown();
        break;
      case RLIControlEvent::Enter:
        if (_menuEngine->visible())
          _menuEngine->onEnter();
        break;
      case RLIControlEvent::Back:
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
        _controlsEngine->shiftVnP(re->spinnerVal());
        _maskEngine->setAngleShift(_controlsEngine->getVnP());
        break;
      case RLIControlEvent::VD:
        _controlsEngine->shiftVd(re->spinnerVal());
        break;
      default:
        break;
    }
  }

  return QGLWidget::event(e);
}
