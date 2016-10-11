#include "targetengine.h"

TargetEngine::TargetEngine() : QGLFunctions() {
  _initialized = false;

  _prog = new QGLShaderProgram();
}

TargetEngine::~TargetEngine() {
  delete _prog;

  if (_initialized) {
    glDeleteBuffers(AIS_TRGT_ATTR_COUNT, _vbo_ids);
    glDeleteTextures(1, &_asset_texture_id);
  }
}


void TargetEngine::trySelect(QPoint cursorPos) {
  Q_UNUSED(cursorPos);
}


void TargetEngine::updateTarget(QString tag, RadarTarget target) {
  _trgtsMutex.lock();

  if (_targets.contains(tag))
    _targets.insert(tag, target);
  else
    _targets[tag] = target;

  _trgtsMutex.unlock();
}


void TargetEngine::deleteTarget(QString tag) {
  _trgtsMutex.lock();

  if (_targets.contains(tag))
    _targets.remove(tag);

  _trgtsMutex.unlock();
}


bool TargetEngine::init(const QGLContext* context) {
  if (_initialized) return false;

  initializeGLFunctions(context);

  glGenBuffers(AIS_TRGT_ATTR_COUNT, _vbo_ids);

  initBuffers();
  initShader();
  initTexture();

  _initialized = true;
  return _initialized;
}

void TargetEngine::draw(QVector2D world_coords, float scale) {
  _trgtsMutex.lock();

  _prog->bind();

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_SCALE], scale);
  glUniform2f(_unif_locs[AIS_TRGT_UNIF_CENTER], world_coords.x(), world_coords.y());

  initBuffers();

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COORDS]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_COORDS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_COORDS]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ORDER]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_ORDER], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_ORDER]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_HEADING]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_HEADING], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_HEADING]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROTATION]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_ROTATION], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_ROTATION]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COURSE]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_COURSE], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_COURSE]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SPEED]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_SPEED], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_SPEED]);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _asset_texture_id);

  glDrawArrays(GL_QUADS, 0,  _targets.size()*4);

  glLineWidth(2);

  glBindTexture(GL_TEXTURE_2D, 0);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 1);
  glDrawArrays(GL_LINES, 0,  _targets.size()*4);

  glPushAttrib(GL_ENABLE_BIT);

  glLineStipple(1, 0xF0F0);
  glEnable(GL_LINE_STIPPLE);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 2);
  glDrawArrays(GL_LINES, 0, _targets.size()*4);

  glPopAttrib();

  _prog->release();

  _trgtsMutex.unlock();
}

void TargetEngine::initBuffers() {
  std::vector<GLfloat> point;
  std::vector<GLfloat> order;
  std::vector<GLfloat> heading;
  std::vector<GLfloat> rotation;
  std::vector<GLfloat> course;
  std::vector<GLfloat> speed;

  QList<QString> keys = _targets.keys();

  for (int trgt = 0; trgt < keys.size(); trgt++) {
    for (int i = 0; i < 4; i++) {
      order.push_back(i);
      point.push_back(_targets[keys[trgt]].Latitude);
      point.push_back(_targets[keys[trgt]].Longtitude);
      heading.push_back(_targets[keys[trgt]].Heading);
      rotation.push_back(_targets[keys[trgt]].Rotation);
      course.push_back(_targets[keys[trgt]].CourseOverGround);
      speed.push_back(_targets[keys[trgt]].SpeedOverGround);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*2*sizeof(GLfloat), point.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*sizeof(GLfloat), order.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_HEADING]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*sizeof(GLfloat), heading.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROTATION]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*sizeof(GLfloat), rotation.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COURSE]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*sizeof(GLfloat), course.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SPEED]);
  glBufferData(GL_ARRAY_BUFFER, keys.size()*4*sizeof(GLfloat), speed.data(), GL_DYNAMIC_DRAW);
}

void TargetEngine::initTexture() {
  glGenTextures(1, &_asset_texture_id);

  glBindTexture(GL_TEXTURE_2D, _asset_texture_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  QImage img("res//textures//targets//target.png");
  img = QGLWidget::convertToGLFormat(img);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height()
               , 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void TargetEngine::initShader() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  // OR statements whould be executed one by one until false
  _prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/trgt.vert.glsl");
  _prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/trgt.frag.glsl");
  _prog->link();
  _prog->bind();

  _attr_locs[AIS_TRGT_ATTR_COORDS] = _prog->attributeLocation("world_coords");
  _attr_locs[AIS_TRGT_ATTR_ORDER] = _prog->attributeLocation("vertex_order");
  _attr_locs[AIS_TRGT_ATTR_HEADING] = _prog->attributeLocation("heading");
  _attr_locs[AIS_TRGT_ATTR_ROTATION] = _prog->attributeLocation("rotation");
  _attr_locs[AIS_TRGT_ATTR_COURSE] = _prog->attributeLocation("course");
  _attr_locs[AIS_TRGT_ATTR_SPEED] = _prog->attributeLocation("speed");

  _unif_locs[AIS_TRGT_UNIF_CENTER] = _prog->uniformLocation("center");
  _unif_locs[AIS_TRGT_UNIF_SCALE] = _prog->uniformLocation("scale");
  _unif_locs[AIS_TRGT_UNIF_TYPE] = _prog->uniformLocation("type");

  // Restore system locale
  setlocale(LC_ALL, "");

  _prog->release();
}
