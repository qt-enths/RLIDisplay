#include "maskengine.h"

#include <cmath>
#include <numeric>

static double const PI = acos(-1);

MaskEngine::MaskEngine(const QSize& sz) {
  _initialized = false;

  _prog = new QGLShaderProgram();

  format.setAttachment(QGLFramebufferObject::CombinedDepthStencil);
  format.setMipmap(true);
  format.setSamples(1);
  format.setTextureTarget(GL_TEXTURE_2D);
  format.setInternalTextureFormat(GL_RGBA8);

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
  delete render_fbo;
  delete texture_fbo;

  delete _prog;
}

bool MaskEngine::init(const QGLContext* context) {
  if (_initialized) return false;

  initializeGLFunctions(context);

  render_fbo = new QGLFramebufferObject(_size.width(), _size.height(), format);
  texture_fbo = new QGLFramebufferObject(_size.width(), _size.height());

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
    delete render_fbo;
    delete texture_fbo;
    render_fbo = new QGLFramebufferObject(_size.width(), _size.height(), format);
    texture_fbo = new QGLFramebufferObject(_size.width(), _size.height());
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

  render_fbo->bind();

  glEnable(GL_MULTISAMPLE);

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

  //glEnableVertexAttribArray(loc_point);
  // ---------------------------------------------------------------------

  glDisable(GL_MULTISAMPLE);

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

  render_fbo->release();

  QRect rect(0, 0, render_fbo->width(), render_fbo->height());
  QGLFramebufferObject::blitFramebuffer(texture_fbo, rect, render_fbo, rect);

  glFlush();
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
  float angle[2*360];
  float chars[2*360];
  float order[2*360];
  float shift[2*360];

  for (int a = 0; a < 360; a++) {
    angle[2*a+0] = angle[2*a+1] = a;
    order[2*a+0] = 0.f;
    order[2*a+1] = 1.f;
    chars[2*a+0] = chars[2*a+1] = -1.f;

    float s = 1.5f;
    if (a%5  == 0) s =  4.f;
    if (a%10 == 0) s =  7.f;
    if (a%30 == 0) s = 12.f;

    shift[2*a+0] = shift[2*a+1] = s;
  }

  setBuffers(vbo_ids_mark, 2*360, angle, chars, order, shift);
}

void MaskEngine::initTextBuffers() {
  QVector<float> angles;
  QVector<float> chars;
  QVector<float> orders;
  QVector<float> shifts;

  _text_point_count = 0;

  for (int i = 0; i < 360; i += 10) {
    QByteArray tm = QString::number(i).toLatin1();

    //qDebug() << i;

    for (int l = 0; l < tm.size(); l++) {
      angles.push_back(i);
      chars.push_back(tm[l]);

      //qDebug() << static_cast<int>(tm[l]);

      orders.push_back(l);
      shifts.push_back(14);
      _text_point_count++;
    }
  }

  setBuffers(vbo_ids_text, _text_point_count, angles.data(), chars.data(), orders.data(), shifts.data());
}

void MaskEngine::initHoleBuffers() {
  QVector<float> angle, chars, order, shift;

  for (int a = 0; a < _hole_point_count; a++) {
    angle.push_back((static_cast<float>(a) / (_hole_point_count - 1)) * 360);
    order.push_back(1);
    chars.push_back(-1);
    shift.push_back(-0.5f);
  }

  setBuffers(vbo_ids_hole, _hole_point_count, angle.data(), chars.data(), order.data(), shift.data());
}

void MaskEngine::setBuffers(GLuint* vbo_ids, int count, GLfloat* angles, GLfloat* chars, GLfloat* orders, GLfloat* shifts) {
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ANGLE]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), angles, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_CHAR_VAL]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), chars, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), orders, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTR_SHIFT]);
  glBufferData(GL_ARRAY_BUFFER, count*sizeof(GLfloat), shifts, GL_DYNAMIC_DRAW);
}


void MaskEngine::initBuffers() {
  initLineBuffers();
  initTextBuffers();
  initHoleBuffers();
}
