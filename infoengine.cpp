#include "infoengine.h"

#include <QDebug>
#include <QDateTime>

InfoBlock::InfoBlock(QObject* parent) : QObject(parent) {
  clear();
  _need_update = false;
}

void InfoBlock::clear() {
  _geometry = QRectF(0, 0, 0, 0);
  _back_color = QColor(1, 1, 1, 0);
  _border_color = QColor(1, 1, 1, 0);
  _border_width = 0;

  _texts.clear();
  _rects.clear();
}

InfoEngine::InfoEngine(const QSize& sz, QObject* parent) : QObject(parent), QGLFunctions() {
  _prog = new QGLShaderProgram();
  _size = sz;
  _initialized = false;
}

InfoEngine::~InfoEngine() {
  if (!_initialized)
    return;

  delete _prog;
  delete _fbo;
  qDeleteAll(_blocks);
  glDeleteBuffers(INFO_ATTR_COUNT, _vbo_ids);
}

bool InfoEngine::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);

  _fbo = new QGLFramebufferObject(_size);
  _full_update = true;

  glGenBuffers(INFO_ATTR_COUNT, _vbo_ids);

  if (!initShaders())
    return false;

  _initialized = true;
  return _initialized;
}

void InfoEngine::resize(const QSize& sz) {
  _size = sz;

  if (_initialized) {
    delete _fbo;
    _fbo = new QGLFramebufferObject(_size);
  }

  emit send_resize(_size);
  _full_update = true;
}


InfoBlock* InfoEngine::addInfoBlock() {
  _blocks.push_back(new InfoBlock(this));
  return _blocks[_blocks.size() - 1];
}


void InfoEngine::update() {
  if (!_initialized)
    return;

  _fbo->bind();

  if (_full_update) {
    glClearColor(1.f, 1.f, 1.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT);
  }

  glViewport(0, 0, _size.width(), _size.height());

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, _size.width(), _size.height(), 0, -1, 1 );

  QVector<InfoBlock*>::const_iterator iter;
  for (iter = _blocks.begin(); iter != _blocks.end(); iter++)
    if (_full_update || (*iter)->needUpdate())
      drawBlock((*iter));

  // линейка
  /*for (int i = 0; i < _size.height(); i++) {
    glBegin(GL_LINES);
    if (!(i % 2))
      glColor3f(1.0, 0.0, 0.0);
    if (!((i - 1) % 2))
      glColor3f(0.0, 1.0, 0.0);

    glVertex2f(0.5, i + 0.5);
    glVertex2f(10 + 0.5, i + 0.5);

    if (!((i / 5) % 2))
      glColor3f(0.2, 0.2, 0.2);
    if (!(((i / 5) - 1) % 2))
      glColor3f(1.0, 1.0, 1.0);

    glVertex2f(0, i);
    glVertex2f(6, i);
    glEnd();
    glEnd();
  }*/

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  _fbo->release();
  _full_update = false;

  glFlush();
}

void InfoEngine::drawBlock(InfoBlock* b) {
  QColor bckCol = b->getBackColor();
  QColor brdCol = b->getBorderColor();
  int    brdWid = b->getBorderWidth();
  QRectF  geom   = b->getGeometry();

  glShadeModel( GL_FLAT );

  // Draw background and border
  glDisable(GL_BLEND);

  if (brdWid >= 1) {
    drawRect(geom, brdCol);
    QRectF back(geom.x() + brdWid, geom.y() + brdWid, geom.width() - 2 * brdWid, geom.height() - 2 * brdWid);
    drawRect(back, bckCol);
  } else {
    drawRect(geom, bckCol);
  }

  glEnable(GL_BLEND);

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glTranslatef(geom.x(), geom.y(), -1) ;

  for (int i = 0; i < b->getRectCount(); i++)
    drawRect(b->getRect(i).rect, b->getRect(i).col);

  _prog->bind();

  for (int i = 0; i < b->getTextCount(); i++)
    drawText(b->getText(i));

  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

  _prog->release();

  b->discardUpdate();
}

void InfoEngine::drawText(const InfoText& text) {
  GLuint tex_id = _fonts->getTextureId(text.font_tag);
  QSize font_size = _fonts->getSize(text.font_tag);

  QVector<float> pos, ord, chars;
  QPointF anchor = text.anchor;
  if (!text.anchor_left)
    anchor -= QPointF(text.chars.size() * font_size.width(), 0);

  for (int i = 0; i < text.chars.size(); i++) {
    for (int j = 0; j < 4; j++) {
      QPointF lefttop = anchor + QPointF(i * font_size.width(), 0);
      pos.push_back(lefttop.x());
      pos.push_back(lefttop.y());
      ord.push_back(j);
      chars.push_back(text.chars[i]);
    }
  }

  glUniform2f(_uniform_locs[INFO_UNIFORM_SIZE], font_size.width(), font_size.height());
  glUniform4f(_uniform_locs[INFO_UNIFORM_COLOR], text.color.redF(), text.color.greenF(), text.color.blueF(), 1.f);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[INFO_ATTR_POSITION]);
  glBufferData(GL_ARRAY_BUFFER, pos.size()*sizeof(GLfloat), pos.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(_attr_locs[INFO_ATTR_POSITION], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[INFO_ATTR_POSITION]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[INFO_ATTR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, ord.size()*sizeof(GLfloat), ord.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(_attr_locs[INFO_ATTR_ORDER], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[INFO_ATTR_ORDER]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[INFO_ATTR_CHAR_VAL]);
  glBufferData(GL_ARRAY_BUFFER, chars.size()*sizeof(GLfloat), chars.data(), GL_STATIC_DRAW);
  glVertexAttribPointer(_attr_locs[INFO_ATTR_CHAR_VAL], 1, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[INFO_ATTR_CHAR_VAL]);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_id);

  glDrawArrays(GL_QUADS, 0, ord.size());
  glBindTexture(GL_TEXTURE_2D, 0);
}

void InfoEngine::drawRect(const QRectF& rect, const QColor& col) {
  glBegin(GL_QUADS);
  glColor4f(col.redF(), col.greenF(), col.blueF(), col.alphaF());
  glVertex2f(rect.x(), rect.y());
  glVertex2f(rect.x(), rect.y() + rect.height());
  glVertex2f(rect.x() + rect.width() , rect.y() + rect.height());
  glVertex2f(rect.x() + rect.width() , rect.y());
  glEnd();
}


bool InfoEngine::initShaders() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  // OR statements whould be executed one by one until false
  if (!_prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/info.vert.glsl")
   || !_prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/info.frag.glsl")
   || !_prog->link()
   || !_prog->bind())
      return false;

  // Restore system locale
  setlocale(LC_ALL, "");

  _attr_locs[INFO_ATTR_POSITION]  = _prog->attributeLocation("position");
  _attr_locs[INFO_ATTR_ORDER]     = _prog->attributeLocation("order");
  _attr_locs[INFO_ATTR_CHAR_VAL]  = _prog->attributeLocation("char_val");

  _uniform_locs[INFO_UNIFORM_COLOR]  = _prog->uniformLocation("color");
  _uniform_locs[INFO_UNIFORM_SIZE]  = _prog->uniformLocation("size");

  _prog->release();

  return true;
}
