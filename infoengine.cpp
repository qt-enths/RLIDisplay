#include "infoengine.h"

#include <QDebug>
#include <QDateTime>

InfoBlock::InfoBlock(QObject* parent) : QObject(parent) {
  clear();
  _need_update = false;
  _initialized = false;
}

InfoBlock::~InfoBlock() {
  delete _fbo;
}

void InfoBlock::setRect(int rectId, const QRect& r) {
  if (rectId < _rects.size())
    _rects[rectId].rect = r;

  _need_update = true;
}

void InfoBlock::setText(int textId, int lang_id, const QByteArray& str) {
  if (textId < _texts.size())
    _texts[textId].str[lang_id] = str;

  _need_update = true;
}

bool InfoBlock::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);

  _fbo = new QGLFramebufferObject(_geometry.size());

  _initialized = true;
  return _initialized;
}

void InfoBlock::clear() {
  _geometry = QRect(0, 0, 1, 1);
  _back_color = QColor(1, 1, 1, 0);
  _border_color = QColor(1, 1, 1, 0);
  _border_width = 0;

  _texts.clear();
  _rects.clear();
}

void InfoBlock::setGeometry(const QRect& r) {
  _geometry = r;
  _need_update = true;

  if (_initialized) {
    delete _fbo;
    _fbo = new QGLFramebufferObject(_geometry.size());
  }
}




InfoEngine::InfoEngine(QObject* parent) : QObject(parent), QGLFunctions() {
  _prog = new QGLShaderProgram();
  _initialized = false;

  _lang = RLI_LANG_RUSSIAN;

  enc = QTextCodec::codecForName("cp866")->makeEncoder();
  dec = QTextCodec::codecForName("UTF8")->makeDecoder();
  dec1 = QTextCodec::codecForName("cp866")->makeDecoder();
}

InfoEngine::~InfoEngine() {
  if (!_initialized)
    return;

  delete _prog;
  qDeleteAll(_blocks);
  glDeleteBuffers(INFO_ATTR_COUNT, _vbo_ids);
}

bool InfoEngine::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);
  _context = context;

  _full_update = true;

  glGenBuffers(INFO_ATTR_COUNT, _vbo_ids);

  if (!initShaders())
    return false;

  _initialized = true;
  return _initialized;
}

InfoBlock* InfoEngine::addInfoBlock() {
  _blocks.push_back(new InfoBlock(this));
  _blocks[_blocks.size() - 1]->init(_context);
  return _blocks[_blocks.size() - 1];
}




void InfoEngine::onLanguageChanged(const QByteArray& lang) {
  QString lang_str = dec1->toUnicode(lang);

  if (_lang == RLI_LANG_RUSSIAN && (lang_str == dec->toUnicode(RLIStrings::nEng[RLI_LANG_RUSSIAN])
                             || lang_str == dec->toUnicode(RLIStrings::nEng[RLI_LANG_ENGLISH]))) {
      _lang = RLI_LANG_ENGLISH;
      _full_update = true;
  }

  if (_lang == RLI_LANG_ENGLISH && (lang_str == dec->toUnicode(RLIStrings::nRus[RLI_LANG_ENGLISH])
                             || lang_str == dec->toUnicode(RLIStrings::nRus[RLI_LANG_RUSSIAN]))) {
      _lang = RLI_LANG_RUSSIAN;
      _full_update = true;
  }
}

void InfoEngine::update() {
  if (!_initialized)
    return;

  QVector<InfoBlock*>::const_iterator iter;
  for (iter = _blocks.begin(); iter != _blocks.end(); iter++) {
    InfoBlock* block = (*iter);
    if (_full_update || block->needUpdate()) {
      block->fbo()->bind();

      glDisable(GL_BLEND);
      glDisable(GL_DEPTH);

      QRect geom = block->getGeometry();

      glViewport(0, 0, geom.width(), geom.height());

      glMatrixMode( GL_PROJECTION );
      glPushMatrix();
      glLoadIdentity();
      glOrtho(0, geom.width(), geom.height(), 0, -1, 1 );

      updateBlock(block);

      glMatrixMode( GL_PROJECTION );
      glPopMatrix();

      block->fbo()->release();
    }
  }

  _full_update = false;
  glFlush();
}

void InfoEngine::updateBlock(InfoBlock* b) {
  QColor bckCol = b->getBackColor();
  QColor brdCol = b->getBorderColor();
  int    brdWid = b->getBorderWidth();
  QRect  geom   = QRect(0, 0, b->getGeometry().width(), b->getGeometry().height());

  glShadeModel( GL_FLAT );

  // Draw background and border

  if (brdWid >= 1) {
    drawRect(geom, brdCol);
    QRect back(geom.x() + brdWid, geom.y() + brdWid, geom.width() - 2 * brdWid, geom.height() - 2 * brdWid);
    drawRect(back, bckCol);
  } else {
    drawRect(geom, bckCol);
  }

  glEnable(GL_BLEND);

  for (int i = 0; i < b->getRectCount(); i++)
    drawRect(b->getRect(i).rect, b->getRect(i).col);

  _prog->bind();

  for (int i = 0; i < b->getTextCount(); i++)
    drawText(b->getText(i));

  _prog->release();

  b->discardUpdate();
}

void InfoEngine::drawText(const InfoText& text) {
  GLuint tex_id = _fonts->getTextureId(text.font_tag);
  QSize font_size = _fonts->getSize(text.font_tag);

  std::vector<GLfloat> pos;
  std::vector<GLfloat> ord, chars;

  QPoint anchor;
  switch (text.allign) {
    case INFOTEXT_ALLIGN_LEFT:
      anchor = text.rect.topLeft();
      break;
    case INFOTEXT_ALLIGN_RIGHT:
      anchor = text.rect.topRight() - QPoint(font_size.width()*text.str[_lang].size(), 0);
      break;
    case INFOTEXT_ALLIGN_CENTER:
      anchor = text.rect.center() - QPoint((font_size.width()*text.str[_lang].size()) / 2, font_size.height() / 2);
      break;
    default:
      anchor = text.rect.topLeft();
      break;
  }

  for (int i = 0; i < text.str[_lang].size(); i++) {
    for (int j = 0; j < 4; j++) {
      QPoint lefttop = anchor + QPoint(i * font_size.width(), 0);

      pos.push_back(lefttop.x());
      pos.push_back(lefttop.y());
      ord.push_back(j);
      chars.push_back(text.str[_lang][i]);
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


void InfoEngine::drawRect(const QRect& rect, const QColor& col) {
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
