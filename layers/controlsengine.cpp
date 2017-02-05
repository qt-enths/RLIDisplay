#include "controlsengine.h"

#include <qmath.h>
#include "../common/rlimath.h"

ControlsEngine::ControlsEngine() : QGLFunctions() {
  _initialized = false;

  _center = QPoint(0, 0);
  _cursor = QPoint(0, 0);
  _visor_shift = QPoint(0, 0);

  _vn_p = 0.f;
  _vn_cu = 0.f;
  _vd = 64.f;

  _oz_max_angle = 35.f;
  _oz_min_angle = 115.f;
  _oz_min_radius = 96.f;
  _oz_max_radius = 192.f;
  _oz_min_radius_step = 1.f;
  _oz_max_radius_step = -1.f;

  _is_pl_visible = false;

  _prog = new QGLShaderProgram();
}

ControlsEngine::~ControlsEngine() {
  delete _prog;

  if (_initialized)
    glDeleteBuffers(CTRL_ATTR_COUNT, vbo_ids);
}

QPointF ControlsEngine::getVdVnIntersection() {
  return QPointF(sin(RLIMath::radians(_vn_cu)) * _vd
                ,-cos(RLIMath::radians(_vn_cu)) * _vd);
}

bool ControlsEngine::init(const QGLContext* context) {
  if (_initialized) return false;

  initializeGLFunctions(context);

  glGenBuffers(CTRL_ATTR_COUNT, vbo_ids);

  initBuffers();
  initShader();

  _initialized = true;
  return _initialized;
}

void ControlsEngine::initBuffers() {
  float* angles = new float[CIRCLE_POINTS+1];

  for (int i = 0; i < CIRCLE_POINTS; i++)
    angles[i] = (static_cast<float>(i) / CIRCLE_POINTS)*360.f;
  angles[CIRCLE_POINTS] = 0.f;

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[CTRL_ATTR_ANGLE]);
  glBufferData(GL_ARRAY_BUFFER, (CIRCLE_POINTS+1)*sizeof(GLfloat), angles, GL_DYNAMIC_DRAW);

  delete[] angles;
}

void ControlsEngine::initShader() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  // OR statements whould be executed one by one until false
  _prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/control.vert.glsl");
  _prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/control.frag.glsl");
  _prog->link();
  _prog->bind();

  // Restore system locale
  setlocale(LC_ALL, "");

  loc_color         = _prog->uniformLocation("color");
  loc_radius        = _prog->uniformLocation("radius");

  loc_angle         = _prog->attributeLocation("angle");

  _prog->release();
}

void ControlsEngine::draw() {
  if (!_initialized) return;

  drawCursor();
  drawVN();
  drawVD();

  if (_is_pl_visible)
    drawPL();

  drawOZ();
}

void ControlsEngine::drawCursor() {
  glShadeModel( GL_SMOOTH );

  glLineWidth(3);
  glColor3f(0.f, .82f, .76f);

  QPoint d = _cursor - _center;

  glBegin(GL_LINES);
  glVertex2f(d.x()-8, d.y());
  glVertex2f(d.x()+8, d.y());
  glVertex2f(d.x(), d.y()-8);
  glVertex2f(d.x(), d.y()+8);
  glEnd();
}

void ControlsEngine::drawVN() {
  glShadeModel( GL_SMOOTH );

  //_vn_cu -= 2;

  float vn_p_rads = RLIMath::radians(_vn_p);
  float vn_cu_rads = RLIMath::radians(_vn_cu);

  glLineWidth(2);
  //glColor3f(0.93f, .62f, .46f);
  glColor3f(0.f, 1.f, 0.f);

  glBegin(GL_LINES);
  glVertex2f(0, 0);
  glVertex2f(1600 * sin(vn_p_rads), -1600 * cos(vn_p_rads));
  glEnd();

  // glPushAttrib is done to return everything to normal after drawing
  //glColor3f(0.83f, .33f, .56f);
  glColor3f(0.f, 1.f, 1.f);
  glPushAttrib(GL_ENABLE_BIT);

  glLineStipple(1, 0xF0F0);
  glEnable(GL_LINE_STIPPLE);

  glMatrixMode( GL_MODELVIEW );
  glTranslatef( _visor_shift.x(), _visor_shift.y(), 0);

  glBegin(GL_LINES);
  glVertex2f(0, 0);
  glVertex2f(1600 * sin(vn_cu_rads), -1600 * cos(vn_cu_rads));
  glEnd();

  glMatrixMode( GL_MODELVIEW );
  glTranslatef( -_visor_shift.x(), -_visor_shift.y(), 0);

  glPopAttrib();
}

void ControlsEngine::drawPL() {
  float vn_p_rads = RLIMath::radians(_vn_p);

  QVector2D tan(sin(vn_p_rads), -cos(vn_p_rads));
  QVector2D norm(-tan.y(), tan.x());

  QVector2D p1 =  _vd*norm;
  QVector2D p2 = -_vd*norm;

  QVector2D p11 = p1 - 1000*tan;
  QVector2D p12 = p1 + 1000*tan;

  QVector2D p21 = p2 - 1000*tan;
  QVector2D p22 = p2 + 1000*tan;

  glShadeModel( GL_SMOOTH );

  glLineWidth(1);
  glColor3f(0.83f, .82f, .76f);

  glBegin(GL_LINES);
  glVertex2f(p11.x(), p11.y());
  glVertex2f(p12.x(), p12.y());
  glVertex2f(p21.x(), p21.y());
  glVertex2f(p22.x(), p22.y());
  glEnd();
}

void ControlsEngine::drawVD() {
  // Start ctrl shader drawing
  _prog->bind();

  // Set uniforms
  // ---------------------------------------------------------------------
  glUniform1f(loc_radius, _vd);
  //glUniform4f(loc_color, 1.f, 0.f, 1.f, 1.f);
  glUniform4f(loc_color, 0.f, 1.f, 1.f, 1.f);
  // ---------------------------------------------------------------------

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[CTRL_ATTR_ANGLE]);
  glVertexAttribPointer(loc_angle, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(loc_angle);

  glMatrixMode( GL_MODELVIEW );
  glTranslatef( _visor_shift.x(), _visor_shift.y(), 0);

  glLineWidth(2);
  glDrawArrays(GL_LINE_STRIP, 0, CIRCLE_POINTS+1);

  glTranslatef( -_visor_shift.x(), -_visor_shift.y(), 0);

  _prog->release();
}

void ControlsEngine::drawOZ() {
  _oz_max_angle++;
  _oz_min_angle--;

  _oz_max_radius += _oz_max_radius_step;
  _oz_min_radius += _oz_min_radius_step;

  if (_oz_max_angle >= 360.f)
    _oz_max_angle = 0.f;

  if (_oz_min_angle <= 0.f)
    _oz_min_angle = 360.f;

  if (_oz_max_radius >= 300.f)
    _oz_max_radius_step *= -1.f;

  if (_oz_max_radius <= 120.f)
    _oz_max_radius_step *= -1.f;

  if (_oz_min_radius >= 120.f)
    _oz_min_radius_step *= -1.f;

  if (_oz_min_radius <= 30.f)
    _oz_min_radius_step *= -1.f;

  float min_angle_rads = RLIMath::radians(_oz_min_angle);
  float max_angle_rads = RLIMath::radians(_oz_max_angle);

  glShadeModel( GL_SMOOTH );

  glLineWidth(2);
  //glColor3f(0.f, 1.f, 1.f);
  static const QColor acqAreaaColor("royalblue");
  glColor3f(acqAreaaColor.redF(), acqAreaaColor.greenF(), acqAreaaColor.blueF());

  glBegin(GL_LINES);
  glVertex2f(_oz_min_radius * sin(min_angle_rads), -_oz_min_radius * cos(min_angle_rads));
  glVertex2f(_oz_max_radius * sin(min_angle_rads), -_oz_max_radius * cos(min_angle_rads));
  glVertex2f(_oz_min_radius * sin(max_angle_rads), -_oz_min_radius * cos(max_angle_rads));
  glVertex2f(_oz_max_radius * sin(max_angle_rads), -_oz_max_radius * cos(max_angle_rads));
  glEnd();

  // Start ctrl shader drawing
  _prog->bind();

  // Set uniforms
  // ---------------------------------------------------------------------
  //glUniform4f(loc_color, 0.f, 1.f, 1.f, 1.f);
  glUniform4f(loc_color, acqAreaaColor.redF(), acqAreaaColor.greenF(), acqAreaaColor.blueF(), 1.f);
  // ---------------------------------------------------------------------

  int min_index = round(_oz_min_angle*CIRCLE_POINTS / 360.f);
  int max_index = round(_oz_max_angle*CIRCLE_POINTS / 360.f);

  glLineWidth(2);

  if (min_index < max_index) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[CTRL_ATTR_ANGLE]);
    glVertexAttribPointer(loc_angle, 1, GL_FLOAT, GL_FALSE, 0, (void*) (min_index*sizeof(float)));
    glEnableVertexAttribArray(loc_angle);

    glUniform1f(loc_radius, _oz_min_radius);
    glDrawArrays(GL_LINE_STRIP, 0, max_index-min_index+1);

    glUniform1f(loc_radius, _oz_max_radius);
    glDrawArrays(GL_LINE_STRIP, 0, max_index-min_index+1);
  } else {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[CTRL_ATTR_ANGLE]);
    glVertexAttribPointer(loc_angle, 1, GL_FLOAT, GL_FALSE, 0, (void*) (min_index*sizeof(float)));
    glEnableVertexAttribArray(loc_angle);

    glUniform1f(loc_radius, _oz_min_radius);
    glDrawArrays(GL_LINE_STRIP, 0, CIRCLE_POINTS-min_index+1);

    glUniform1f(loc_radius, _oz_max_radius);
    glDrawArrays(GL_LINE_STRIP, 0, CIRCLE_POINTS-min_index+1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[CTRL_ATTR_ANGLE]);
    glVertexAttribPointer(loc_angle, 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
    glEnableVertexAttribArray(loc_angle);

    glUniform1f(loc_radius, _oz_min_radius);
    glDrawArrays(GL_LINE_STRIP, 0, max_index+1);

    glUniform1f(loc_radius, _oz_max_radius);
    glDrawArrays(GL_LINE_STRIP, 0, max_index+1);
  }

  _prog->release();
}

