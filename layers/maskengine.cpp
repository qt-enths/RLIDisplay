#include "maskengine.h"

#include <cmath>
#include <numeric>

static double const PI = acos(-1);

MaskEngine::MaskEngine(const QSize& sz) {
  _initialized = false;

  _prog = new QGLShaderProgram();

  resize(sz);

  _angle_shift = 0;
  _text_point_count = 0;
  _hole_point_count = 362;
}

MaskEngine::~MaskEngine() {
  if (!_initialized)
    return;

  glDeleteBuffers(MARK_ATTR_COUNT, vbo_ids_mark);
  glDeleteBuffers(MARK_ATTR_COUNT, vbo_ids_text);
  glDeleteBuffers(MARK_ATTR_COUNT, vbo_ids_hole);

  delete _fbo;

  delete _prog;
}

bool MaskEngine::init(const QGLContext* context) {
  if (_initialized) return false;

  initializeGLFunctions(context);

  _fbo = new QGLFramebufferObject(_size.width(), _size.height());

  glGenBuffers(MARK_ATTR_COUNT, vbo_ids_mark);
  glGenBuffers(MARK_ATTR_COUNT, vbo_ids_text);
  glGenBuffers(MARK_ATTR_COUNT, vbo_ids_hole);

  initBuffers();

  if (!initShaders()) return false;

  update();

  _initialized = true;
  return _initialized;
}


void MaskEngine::resize(const QSize& sz) {
  _size = sz;
  _hole_radius = 470.f / 1024.f * _size.height();
  _hole_center = QPoint(470.f / 1024.f * _size.height() + 34.0, _size.height() / 2.f);
  _cursor_pos = _hole_center;

  if (_initialized) {
    delete _fbo;
    _fbo = new QGLFramebufferObject(_size.width(), _size.height());
  }
}

void MaskEngine::bindBuffers(GLuint* vbo_ids) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ANGLE]);
  glVertexAttribPointer(loc_angle, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(loc_angle);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_CHAR_VAL]);
  glVertexAttribPointer(loc_char_val, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(loc_char_val);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ORDER]);
  glVertexAttribPointer(loc_order, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(loc_order);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_SHIFT]);
  glVertexAttribPointer(loc_shift, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(loc_shift);
}

void MaskEngine::update() {
  if (!_initialized)
    return;

  glViewport(0, 0, _size.width(), _size.height());

  _fbo->bind();

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, _size.width(), _size.height(), 0, -1, 1 );

  glShadeModel( GL_SMOOTH );
  // Draw background
  glBegin(GL_QUADS);
  glColor3f(.04f, .05f, .04f);
  glVertex2f(static_cast<float>(_size.width()), 0.f);
  glVertex2f(0.f, 0.f);
  glColor3f(.32f, .34f, .30f);
  glVertex2f(0.f, static_cast<float>(_size.height()));
  glVertex2f(static_cast<float>(_size.width()), static_cast<float>(_size.height()));
  glEnd();

  // Start mark shader drawing
  _prog->bind();

  // Set uniforms
  // ---------------------------------------------------------------------
  glUniform1f(loc_angle_shift, _angle_shift);
  glUniform1f(loc_circle_radius, _hole_radius);
  glUniform2f(loc_circle_pos, _hole_center.x(), _hole_center.y());
  glUniform2f(loc_cursor_pos, _cursor_pos.x(), _cursor_pos.y());
  glUniform4f(loc_color, 0.f, 1.f, 0.f, 1.f);
  // ---------------------------------------------------------------------

  // Draw line marks
  // ---------------------------------------------------------------------
  bindBuffers(vbo_ids_mark);
  glLineWidth(1);
  glDrawArrays(GL_LINES, 0, 2*360);
  // ---------------------------------------------------------------------


  // Draw text mark
  // ---------------------------------------------------------------------
  QSize font_size = _fonts->getSize("8x8");
  GLuint tex_id = _fonts->getTextureId("8x8");

  bindBuffers(vbo_ids_text);

  glUniform2f(loc_font_size, font_size.width(), font_size.height());
  glPointSize(qMax(font_size.width(), font_size.height()));

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_id);

  glDrawArrays(GL_POINTS, 0, _text_point_count);
  glBindTexture(GL_TEXTURE_2D, 0);
  // ---------------------------------------------------------------------

  glDisable(GL_BLEND);

  // Draw hole
  // ---------------------------------------------------------------------
  glUniform4f(loc_color, 1.f, 1.f, 1.f, 0.f);
  glUniform2f(loc_cursor_pos, _hole_center.x(), _hole_center.y());
  bindBuffers(vbo_ids_hole);
  glDrawArrays(GL_POLYGON, 0, _hole_point_count);
  // ---------------------------------------------------------------------

  _prog->release();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glFlush();

  _fbo->release();
}

bool MaskEngine::initShaders() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  // OR statements whould be executed one by one until false
  if (!_prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/mask.vert.glsl")
   || !_prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/mask.frag.glsl")
   || !_prog->link()
   || !_prog->bind())
      return false;

  // Restore system locale
  setlocale(LC_ALL, "");

  loc_angle_shift   = _prog->uniformLocation("angle_shift");
  loc_circle_radius = _prog->uniformLocation("circle_radius");
  loc_circle_pos    = _prog->uniformLocation("circle_pos");
  loc_cursor_pos    = _prog->uniformLocation("cursor_pos");
  loc_color         = _prog->uniformLocation("color");
  loc_font_size     = _prog->uniformLocation("font_size");

  loc_angle         = _prog->attributeLocation("angle");
  loc_char_val      = _prog->attributeLocation("char_val");
  loc_order         = _prog->attributeLocation("order");
  loc_shift         = _prog->attributeLocation("shift");

  _prog->release();

  return true;
}

void MaskEngine::initLineBuffers() {
  GLfloat angle[2*360];
  GLfloat chars[2*360];
  GLfloat order[2*360];
  GLfloat shift[2*360];

  for (int a = 0; a < 360; a++) {
    angle[2*a+0] = angle[2*a+1] = a;
    order[2*a+0] = 0;
    order[2*a+1] = 1;
    chars[2*a+0] = chars[2*a+1] = 0;

    char s = 2;
    if (a%5  == 0) s =  4;
    if (a%10 == 0) s =  7;
    if (a%30 == 0) s = 12;

    shift[2*a+0] = shift[2*a+1] = s;
  }

  setBuffers(vbo_ids_mark, 2*360, angle, chars, order, shift);
}

void MaskEngine::initTextBuffers() {
  QVector<GLfloat> angles;
  QVector<GLfloat> chars;
  QVector<GLfloat> orders;
  QVector<GLfloat> shifts;

  _text_point_count = 0;

  for (int i = 0; i < 360; i += 10) {
    QByteArray tm = QString::number(i).toLatin1();

    for (int l = 0; l < tm.size(); l++) {
      angles.push_back(i);
      chars.push_back(tm[l]);
      orders.push_back(l);
      shifts.push_back(14);
      _text_point_count++;
    }
  }

  setBuffers(vbo_ids_text, _text_point_count, angles.data(), chars.data(), orders.data(), shifts.data());
}

void MaskEngine::initHoleBuffers() {
  QVector<GLfloat> angle;
  QVector<GLfloat> chars, order, shift;

  for (int a = 0; a < _hole_point_count; a++) {
    angle.push_back((static_cast<float>(a) / (_hole_point_count - 1)) * 360);
    order.push_back(1);
    chars.push_back(0);
    shift.push_back(0);
  }

  setBuffers(vbo_ids_hole, _hole_point_count, angle.data(), chars.data(), order.data(), shift.data());
}

void MaskEngine::setBuffers(GLuint* vbo_ids, int count, GLfloat* angles, GLfloat* chars, GLfloat* orders, GLfloat* shifts) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ANGLE]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), angles, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_CHAR_VAL]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), chars, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), orders, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_SHIFT]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), shifts, GL_STATIC_DRAW);
}


void MaskEngine::initBuffers() {
  initLineBuffers();
  initTextBuffers();
  initHoleBuffers();
}
