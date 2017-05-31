#ifndef MASKENGINE_H
#define MASKENGINE_H

#define CIRCLE_RAY_COUNT 720
#define MARK_RAY_COUNT 36

#include <QVector>
#include <QMap>

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLFramebufferObjectFormat>
#include <QtOpenGL/QGLShaderProgram>

#include "asmfonts.h"

class MaskEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit MaskEngine (const QSize& sz, const QMap<QString, QString>& params, QObject* parent = 0);
  virtual ~MaskEngine ();

  bool init(const QGLContext* context);
  void resize(const QSize& sz, const QMap<QString, QString>& params);

  inline void setAngleShift (float s)           { _angle_shift = s; }
  inline void setCursorPos  (const QPoint& p)   { _cursor_pos = p; }
  inline void setFonts      (AsmFonts* fonts)   { _fonts = fonts; }

  inline GLuint  getTextureId   ()   { return _fbo->texture(); }
  inline QPoint  getCenter      ()   { return _hole_center; }
  inline uint    getRadius      ()   { return _hole_radius; }
  inline float   getAngleShift  ()   { return _angle_shift; }

public slots:
  void update();

private:
  bool _initialized;

  void initBuffers();
  bool initShaders();

  void initLineBuffers();
  void initTextBuffers();
  void initHoleBuffers();

  void bindBuffers(GLuint* vbo_ids);
  void setBuffers(GLuint* vbo_ids, int count, GLfloat* angles, GLfloat* chars, GLfloat* orders, GLfloat* shifts);

  AsmFonts* _fonts;

  float     _angle_shift;
  float     _hole_radius;

  QPoint    _hole_center;
  QPoint    _cursor_pos;

  QSize     _size;

  // Framebuffer vars
  QGLFramebufferObject* _fbo;

  // Mask shader programs
  QGLShaderProgram* _prog;

  // -----------------------------------------------
  enum { MARK_ATTR_ANGLE = 0, MARK_ATTR_CHAR_VAL = 1, MARK_ATTR_ORDER = 2, MARK_ATTR_SHIFT = 3, MARK_ATTR_COUNT = 4 } ;

  GLuint vbo_ids_mark  [MARK_ATTR_COUNT];

  GLuint vbo_ids_text  [MARK_ATTR_COUNT];
  int _text_point_count;

  GLuint vbo_ids_hole  [MARK_ATTR_COUNT];
  int _hole_point_count;

  // Mark line shader uniform locations
  GLuint  loc_angle_shift;
  GLuint  loc_circle_radius;
  GLuint  loc_circle_pos;
  GLuint  loc_cursor_pos;
  GLuint  loc_color;
  GLuint  loc_font_size;
  // Mark line shader attributes locations
  GLuint  loc_angle;
  GLuint  loc_char_val;
  GLuint  loc_order;
  GLuint  loc_shift;
};

#endif // MASKENGINE_H
