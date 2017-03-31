#ifndef MAGNIFIERENGINE_H
#define MAGNIFIERENGINE_H

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>

class MagnifierEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:

  explicit MagnifierEngine  (QObject* parent = 0);
  virtual ~MagnifierEngine  ();

  inline QSize size() { return _size; }
  inline GLuint getTextureId() { return _fbo->texture(); }

  bool init     (const QGLContext* context);

public slots:
  void update();

private:
  bool _initialized;

  QSize _size;

  QGLFramebufferObject* _fbo;
};

#endif // MAGNIFIERENGINE_H
