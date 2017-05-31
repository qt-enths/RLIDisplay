#include "magnifierengine.h"

MagnifierEngine::MagnifierEngine(const QSize& screen_size, const QMap<QString, QString>& params, QObject* parent) : QObject(parent), QGLFunctions() {
  _initialized = false;
  resize(screen_size, params);
}

MagnifierEngine::~MagnifierEngine() {
  if (_initialized)
    delete _fbo;
}

void MagnifierEngine::resize(const QSize& screen_size, const QMap<QString, QString>& params) {
  _position = QPointF(params["x"].toFloat(), params["y"].toFloat());

  if (_position.x() < 0) _position.setX(_position.x() + screen_size.width());
  if (_position.y() < 0) _position.setX(_position.y() + screen_size.height());

  _size = QSize(params["width"].toInt(), params["height"].toInt());

  if (_initialized) {
    delete _fbo;
    _fbo = new QGLFramebufferObject(_size);
  }
}

bool MagnifierEngine::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);

  _fbo = new QGLFramebufferObject(_size);

  _initialized = true;
  return _initialized;
}

void MagnifierEngine::update() {
  if (!_initialized)
    return;

  glViewport(0, 0, _size.width(), _size.height());

  glDisable(GL_BLEND);

  _fbo->bind();

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, _size.width(), _size.height(), 0, -1, 1 );

  glShadeModel( GL_FLAT );

  glLineWidth(1.f);

  // Draw border
  glBegin(GL_LINES);
    glColor3f(1.f, 1.f, 1.f);
    glVertex2f(0.5f, 0.f);
    glVertex2f(0.5f, _size.height());

    glVertex2f(0.f, 0.5f);
    glVertex2f(_size.width(), 0.5f);

    glVertex2f(_size.width()-0.5f, 0.f);
    glVertex2f(_size.width()-0.5f, _size.height());

    glVertex2f(0.f, _size.height()-0.5f);
    glVertex2f(_size.width(), _size.height()-0.5f);

    glVertex2f(_size.width()/2.0f, 0.f);
    glVertex2f(_size.width()/2.0f, _size.height());

    glVertex2f(0.f, _size.height()/2.0f);
    glVertex2f(_size.width(), _size.height()/2.0f);
  glEnd();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  _fbo->release();
}
