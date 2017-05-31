#ifndef MAGNIFIERENGINE_H
#define MAGNIFIERENGINE_H

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>

class MagnifierEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit MagnifierEngine  (const QSize& screen_size, const QMap<QString, QString>& params, QObject* parent = 0);
  virtual ~MagnifierEngine  ();

  inline QPointF position() { return _position; }
  inline QSize size() { return _size; }
  inline GLuint getTextureId() { return _fbo->texture(); }

  bool init     (const QGLContext* context);
  void resize   (const QSize& screen_size, const QMap<QString, QString>& params);

public slots:
  void update();

private:
  bool _initialized;

  QSize _size;
  QPointF _position;

  QGLFramebufferObject* _fbo;
};

#endif // MAGNIFIERENGINE_H
