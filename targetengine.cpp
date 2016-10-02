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


void TargetEngine::updateTarget(QString tag, RadarTarget target) {
  if (_targets.contains(tag))
    _targets.insert(tag, target);
  else
    _targets[tag] = target;
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

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COG]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_COG], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_COG]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROT]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_ROT], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_ROT]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SOG]);
  glVertexAttribPointer(_attr_locs[AIS_TRGT_ATTR_SOG], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[AIS_TRGT_ATTR_SOG]);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _asset_texture_id);

  glDrawArrays(GL_QUADS, 0, 3*4);

  glLineWidth(2);

  glBindTexture(GL_TEXTURE_2D, 0);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 1);
  glDrawArrays(GL_LINES, 0, 3*4);

  glPushAttrib(GL_ENABLE_BIT);

  glLineStipple(1, 0xF0F0);
  glEnable(GL_LINE_STIPPLE);

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 2);
  glDrawArrays(GL_LINES, 0, _targets.size()*4);

  glPopAttrib();

  _prog->release();
}

void TargetEngine::initBuffers() {
  std::vector<GLfloat> points;
  std::vector<GLfloat> orders;
  std::vector<GLfloat> cogs;
  std::vector<GLfloat> rots;
  std::vector<GLfloat> sogs;

  QList<QString> keys = _targets.keys();

  for (int trgt = 0; trgt < keys.size(); trgt++) {
    for (int i = 0; i < 4; i++) {
      orders.push_back(i);
      points.push_back(_targets[keys[trgt]].LAT);
      points.push_back(_targets[keys[trgt]].LON);
      cogs.push_back(_targets[keys[trgt]].COG);
      rots.push_back(_targets[keys[trgt]].ROT);
      sogs.push_back(_targets[keys[trgt]].SOG);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, 3*4*2*sizeof(GLfloat), points.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), orders.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COG]);
  glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), cogs.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROT]);
  glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), rots.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SOG]);
  glBufferData(GL_ARRAY_BUFFER, 3*4*sizeof(GLfloat), sogs.data(), GL_DYNAMIC_DRAW);
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
  _attr_locs[AIS_TRGT_ATTR_COG] = _prog->attributeLocation("COG");
  _attr_locs[AIS_TRGT_ATTR_ROT] = _prog->attributeLocation("ROT");
  _attr_locs[AIS_TRGT_ATTR_SOG] = _prog->attributeLocation("SOG");

  _unif_locs[AIS_TRGT_UNIF_CENTER] = _prog->uniformLocation("center");
  _unif_locs[AIS_TRGT_UNIF_SCALE] = _prog->uniformLocation("scale");
  _unif_locs[AIS_TRGT_UNIF_TYPE] = _prog->uniformLocation("type");

  // Restore system locale
  setlocale(LC_ALL, "");

  _prog->release();
}
