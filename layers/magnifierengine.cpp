#include "magnifierengine.h"

MagnifierEngine::MagnifierEngine(QObject* parent) : QObject(parent), QGLFunctions() {
  _initialized = false;
  _size = QSize(224, 224);
}

MagnifierEngine::~MagnifierEngine() {
  if (_initialized)
    delete _fbo;
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
