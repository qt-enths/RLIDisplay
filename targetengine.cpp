#include "targetengine.h"

#include "rlimath.h"

TargetEngine::TargetEngine(QObject* parent) : QObject(parent), QGLFunctions() {
  _initialized = false;
  _tailsTime   = 0;
  _selected = "1";

  connect(&_tailsTimer, SIGNAL(timeout()), SLOT(onTailsTimer()));
  if(_tailsTime > 0)
      _tailsTimer.start((_tailsTime * 60 * 1000) / TRG_TAIL_NUM);
}

TargetEngine::~TargetEngine() {
  if (_initialized) {
    delete _prog;
    glDeleteBuffers(AIS_TRGT_ATTR_COUNT, _vbo_ids);
    glDeleteBuffers(1, &_tbo_id);
    glDeleteTextures(1, &_asset_texture_id);
  }
}


void TargetEngine::trySelect(QVector2D cursorCoords, float scale) {
  QList<QString> tags = _targets.keys();
  for (int i = 0; i < tags.size(); i++) {
    if (tags[i] == _selected)
      continue;

    QVector2D target_coords(_targets[tags[i]].Latitude, _targets[tags[i]].Longtitude);
    QPointF dist_to_target = RLIMath::coords_to_pos(cursorCoords, target_coords, QPoint(0, 0), scale);
    if (QVector2D(dist_to_target).length() < 16) {
      _selected = tags[i];
      emit selectedTargetUpdated(_selected, _targets[_selected]);
      return;
    }
  }
}

#include <QDateTime>

void TargetEngine::onTailsTimer() {
  _trgtsMutex.lock();
  //qDebug() << QDateTime::currentDateTime() << ": " << "onTailsTimer";

  QList<QString> tags = _targets.keys();
  for (int i = 0; i < tags.count(); i++) {
    QString tag = tags[i];
    _tails[tag].push_back(QVector2D(_targets[tag].Latitude,_targets[tag].Longtitude));

    if (_tails[tag].size() > TRG_TAIL_NUM)
      _tails[tag].removeFirst();
  }

  _trgtsMutex.unlock();
  //qDebug() << _tails[tags[0]].size();
}

void TargetEngine::onTailsTimeChanged(int minutes)
{
    if(_tailsTimer.isActive())
        _tailsTimer.stop();
    _tailsTime = minutes;
    if(_tailsTime <= 0)
    {
        _trgtsMutex.lock();
        //qDebug() << QDateTime::currentDateTime() << ": " << "onTailsTimer";

        QList<QString> tags = _targets.keys();
        for (int i = 0; i < tags.count(); i++) {
          QString tag = tags[i];
          _tails[tag].push_back(QVector2D(_targets[tag].Latitude,_targets[tag].Longtitude));

          if (_tails[tag].size() > TRG_TAIL_NUM)
            _tails[tag].removeFirst();
        }

        _trgtsMutex.unlock();
    }
    else
        _tailsTimer.start((_tailsTime * 60 * 1000) / TRG_TAIL_NUM);
}

void TargetEngine::updateTarget(QString tag, RadarTarget target) {
  _trgtsMutex.lock();

  if (!_targets.contains(tag)) {
    _targets.insert(tag, target);
    _tails.insert(tag, QList<QVector2D>());    
    emit targetCountChanged(_targets.size());
  } else {
    _targets[tag] = target;
    if (tag == _selected)
      emit selectedTargetUpdated(tag, target);
  }

  _trgtsMutex.unlock();
}


void TargetEngine::deleteTarget(QString tag) {
  _trgtsMutex.lock();

  if (_targets.contains(tag)) {
    _targets.remove(tag);
    _tails.remove(tag);
    emit targetCountChanged(_targets.size());

    if (tag == _selected) {
      emit selectedTargetUpdated(tag, RadarTarget());
      _selected = "";
    }
  }

  _trgtsMutex.unlock();
}


bool TargetEngine::init(const QGLContext* context) {
  Q_UNUSED(context);

  if (_initialized) return false;

  initializeGLFunctions(context);

  _prog = new QGLShaderProgram();

  glGenBuffers(1, &_tbo_id);
  glGenBuffers(AIS_TRGT_ATTR_COUNT, _vbo_ids);

  initShader();

  initTexture("res//textures//targets//target.png", &_asset_texture_id);
  initTexture("res//textures//targets//selection.png", &_selection_texture_id);

  _initialized = true;
  return _initialized;
}


void TargetEngine::initShader() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  _prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/trgt.vert.glsl");
  _prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/trgt.frag.glsl");

  //const GLchar* feedbackVaryings[] = { "screen_pos" };
  //glTransformFeedbackVaryings(_prog->programId(), 1, feedbackVaryings, GL_SEPARATE_ATTRIBS);

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

  _prog->release();

  // Restore system locale
  setlocale(LC_ALL, "");
}


void TargetEngine::draw(QVector2D world_coords, float scale) {
  _trgtsMutex.lock();

  _prog->bind();

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_SCALE], scale);
  glUniform2f(_unif_locs[AIS_TRGT_UNIF_CENTER], world_coords.x(), world_coords.y());

  initBuffersTrgts("");
  bindBuffers();

  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 0);

  // Transform feedback part
  // Start
  /*glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER , _tbo_id);
  glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER , _targets.size()*4*2*sizeof(GLfloat), nullptr, GL_DYNAMIC_READ);
  glEnable(GL_RASTERIZER_DISCARD);
  glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _tbo_id);
  glBeginTransformFeedback(GL_POINTS);
  glDrawArrays(GL_POINTS, 0,  _targets.size()*4);
  glEndTransformFeedback();
  glFlush();

  GLfloat* feedback = new GLfloat[_targets.size()*4*2];
  glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0, _targets.size()*4*2*sizeof(GLfloat), feedback);

  qDebug() << "feedback";
  qDebug() << world_coords;
  qDebug() << scale;
  for (int i = 0; i < _targets.size(); i++) {
    QVector2D center(_targets[_targets.keys()[i]].Latitude, _targets[_targets.keys()[i]].Longtitude);
    qDebug() << center;

    float lat_rads = radians(world_coords.x());

    float y_m = -6378137*radians(center.x() - world_coords.x());
    float x_m = 6378137*cos(lat_rads)*radians(center.y() - world_coords.y());

    // screen position
    QVector2D pix_pos = QVector2D(x_m, y_m) / scale;
    qDebug() << pix_pos;

    qDebug() << feedback[8*i+0] << ": " << feedback[8*i+1];
    qDebug() << feedback[8*i+2] << ": " << feedback[8*i+3];
    qDebug() << feedback[8*i+4] << ": " << feedback[8*i+5];
    qDebug() << feedback[8*i+6] << ": " << feedback[8*i+7];
  }

  glDisable(GL_RASTERIZER_DISCARD);
  // End
  // Transform feedback part
  */


  // Draw target marks
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, _asset_texture_id);
  glDrawArrays(GL_QUADS, 0,  _targets.size()*4);
  glBindTexture(GL_TEXTURE_2D, 0);

  glLineWidth(2);

  // Draw target headings
  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 1);
  glDrawArrays(GL_LINES, 0,  _targets.size()*4);

  // Draw target courses
  glPushAttrib(GL_ENABLE_BIT);
  glLineStipple(1, 0xF0F0);
  glEnable(GL_LINE_STIPPLE);
  glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 2);
  glDrawArrays(GL_LINES, 0, _targets.size()*4);
  glPopAttrib();

  glPointSize(5);

  // Draw tails++
  if(_tailsTime)
  {
      int pCount = initBuffersTails();
      bindBuffers();
      glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 3);
      glDrawArrays(GL_POINTS, 0, pCount);
  }


  if (_selected != "" && _targets.contains(_selected)) {
    initBuffersTrgts(_selected);
    bindBuffers();

    glUniform1f(_unif_locs[AIS_TRGT_UNIF_TYPE], 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, _selection_texture_id);
    glDrawArrays(GL_QUADS, 0,  4);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  _prog->release();

  _trgtsMutex.unlock();
}


void TargetEngine::bindBuffers() {
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
}


int TargetEngine::initBuffersTails() {
  std::vector<GLfloat> point;

  QList<QString> keys = _tails.keys();

  for (int trgt = 0; trgt < keys.size(); trgt++) {
    for (int i = 0; i < _tails[keys[trgt]].size(); i++) {
      point.push_back(_tails[keys[trgt]][i].x());
      point.push_back(_tails[keys[trgt]][i].y());
    }
  }

  int point_count = point.size()/2;

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, point_count*2*sizeof(GLfloat), point.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, point_count*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_HEADING]);
  glBufferData(GL_ARRAY_BUFFER, point_count*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROTATION]);
  glBufferData(GL_ARRAY_BUFFER, point_count*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COURSE]);
  glBufferData(GL_ARRAY_BUFFER, point_count*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SPEED]);
  glBufferData(GL_ARRAY_BUFFER, point_count*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);

  return point_count;
}


void TargetEngine::initBuffersTrgts(QString tag) {
  std::vector<GLfloat> point, order, heading, rotation, course, speed;

  QList<QString> keys = _targets.keys();

  if (tag == "") {
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
  } else {
    for (int i = 0; i < 4; i++) {
     order.push_back(i);
      point.push_back(_targets[tag].Latitude);
      point.push_back(_targets[tag].Longtitude);
      heading.push_back(0);
      rotation.push_back(0);
      course.push_back(0);
      speed.push_back(0);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, point.size()*sizeof(GLfloat), point.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, order.size()*sizeof(GLfloat), order.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_HEADING]);
  glBufferData(GL_ARRAY_BUFFER, heading.size()*sizeof(GLfloat), heading.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_ROTATION]);
  glBufferData(GL_ARRAY_BUFFER, rotation.size()*sizeof(GLfloat), rotation.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_COURSE]);
  glBufferData(GL_ARRAY_BUFFER, course.size()*sizeof(GLfloat), course.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[AIS_TRGT_ATTR_SPEED]);
  glBufferData(GL_ARRAY_BUFFER, speed.size()*sizeof(GLfloat), speed.data(), GL_DYNAMIC_DRAW);
}

void TargetEngine::initTexture(QString path, GLuint* tex_id) {
  glGenTextures(1, tex_id);

  glBindTexture(GL_TEXTURE_2D, *tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  QImage img(path);
  img = QGLWidget::convertToGLFormat(img);

  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height()
               , 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}

int TargetEngine::getTailsTime(void)
{
    return _tailsTime;
}
