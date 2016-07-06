#include "menuengine.h"

RLIMenuItemMenu::RLIMenuItemMenu(const QByteArray& name, RLIMenuItemMenu* parent) : RLIMenuItem(name) {
  _parent = parent;
}

RLIMenuItemMenu::~RLIMenuItemMenu() {
  qDeleteAll(_items);
}

RLIMenuItemList::RLIMenuItemList(const QByteArray& name, const QVector<QByteArray>& variants, int def_ind) : RLIMenuItem(name) {
  _variants = variants;
  _index = def_ind;
}

RLIMenuItemInt::RLIMenuItemInt(const QByteArray& name, int min, int max, int def)  : RLIMenuItem(name) {
  _min = min;
  _max = max;
  _value = def;
}

RLIMenuItemFloat::RLIMenuItemFloat(const QByteArray& name, float min, float max, float def) : RLIMenuItem(name) {
  _min = min;
  _max = max;
  _value = def;
  _step = 0.2f;
}

MenuEngine::MenuEngine(const QSize& font_size, QObject* parent) : QObject(parent), QGLFunctions() {
  _prog = new QGLShaderProgram();
  _enc = QTextCodec::codecForName("cp866")->makeEncoder();
  _dec = QTextCodec::codecForName("UTF8")->makeDecoder();

  resize(font_size);

  _initialized = false;
  _selected_line = 1;
  _selection_active = false;
  _enabled = false;

  initMenuTree();
}

void MenuEngine::initMenuTree() {
  RLIMenuItemMenu* m1 = new RLIMenuItemMenu(toQByteArray("УСТАНОВКИ"), NULL);

  RLIMenuItemInt* i10 = new RLIMenuItemInt(toQByteArray("РПЧ"), 0, 255, 5);
  m1->add_item(static_cast<RLIMenuItem*>(i10));

  QVector<QByteArray> val_list;
  val_list.append(toQByteArray("МИЛИ"));
  val_list.append(toQByteArray("КМ"));
  RLIMenuItemList* i11 = new RLIMenuItemList(toQByteArray("СМЕНА ВД"), val_list, 0);
  val_list.clear();
  m1->add_item(static_cast<RLIMenuItem*>(i11));

  val_list.append(toQByteArray("ЛАГ"));
  val_list.append(toQByteArray("РУЧ"));
  RLIMenuItemList* i12 = new RLIMenuItemList(toQByteArray("ДАТЧИК СКОР"), val_list, 0);
  val_list.clear();
  m1->add_item(static_cast<RLIMenuItem*>(i12));

  RLIMenuItemFloat* i13 = new RLIMenuItemFloat(toQByteArray("СК  РУЧ  УЗ"), 0.f, 90.f, 5.f);
  m1->add_item(i13);

  val_list.append(toQByteArray("АОЦ"));
  val_list.append(toQByteArray("СНС"));
  val_list.append(toQByteArray("ГК-Л(В)"));
  val_list.append(toQByteArray("ДЛГ"));
  RLIMenuItemList* i14 = new RLIMenuItemList(toQByteArray("ДАТЧИК СТАБ"), val_list, 2);
  val_list.clear();
  m1->add_item(i14);

  RLIMenuItemInt* i15 = new RLIMenuItemInt(toQByteArray("ТАЙМЕР мин"), 0, 90, 0);
  m1->add_item(i15);

  val_list.append(toQByteArray("РУС"));
  val_list.append(toQByteArray("АНГЛ"));
  RLIMenuItemList* i16 = new RLIMenuItemList(toQByteArray("ЯЗЫК"), val_list, 0);
  val_list.clear();
  m1->add_item(i16);

  RLIMenuItemFloat* i17 = new RLIMenuItemFloat(toQByteArray("СОГЛАС ГИРО"), 0.f, 359.9f, 0.f);
  m1->add_item(i17);

  RLIMenuItemInt* i18 = new RLIMenuItemInt(toQByteArray("ОПАС ГЛУБ м"), 1, 100, 1);
  m1->add_item(i18);

  RLIMenuItemInt* i19 = new RLIMenuItemInt(toQByteArray("ЯКОРНАЯ СТ м"), 1, 100, 1);
  m1->add_item(i19);

  val_list.append(toQByteArray("ОТКЛ"));
  RLIMenuItemList* i110 = new RLIMenuItemList(toQByteArray("ШУМ СТАБ"), val_list, 0);
  val_list.clear();
  m1->add_item(i110);

  _menu = m1;
}

MenuEngine::~MenuEngine() {
  delete _prog;

  if (!_initialized)
    return;

  delete _fbo;
  glDeleteBuffers(INFO_ATTR_COUNT, _vbo_ids);
}

void MenuEngine::setVisibility(bool val) {
  _enabled = val;
  _need_update = true;
}

void MenuEngine::onUp() {
  if (!_enabled)
    return;

  if (!_selection_active) {
    if (_selected_line > 1)
      _selected_line--;
  } else {
    _menu->item(_selected_line - 1)->up();
  }

  _need_update = true;
}

void MenuEngine::onDown() {
  if (!_enabled)
    return;

  if (!_selection_active) {
    if (_selected_line < _menu->item_count())
      _selected_line++;
  } else {
    _menu->item(_selected_line - 1)->down();
  }

  _need_update = true;
}

void MenuEngine::onEnter() {
  _selection_active = !_selection_active;
  _need_update = true;
}

bool MenuEngine::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);

  _fbo = new QGLFramebufferObject(_size);
  _need_update = true;

  glGenBuffers(INFO_ATTR_COUNT, _vbo_ids);

  if (!initShader())
    return false;

  _initialized = true;
  return _initialized;
}

void MenuEngine::resize(const QSize& font_size) {
  // 13 lines + 14 spaces between lines
  // 19 symbols per line + 2 paddings
  _size = QSize(19*font_size.width()+4*2, 13*(6+font_size.height()) + 4*2 - 6.f);
  _font_tag = QString::number(font_size.width()) + "x" + QString::number(font_size.height());
  _need_update = true;
}

void MenuEngine::update() {
  if (!_initialized || !_need_update)
    return;

  glViewport(0, 0, _size.width(), _size.height());

  glDisable(GL_BLEND);

  _fbo->bind();

  glClearColor(0.f, 0.f, 0.f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, _size.width(), _size.height(), 0, -1, 1 );

  glShadeModel( GL_FLAT );

  glLineWidth(2.f);

  // Draw border
  glBegin(GL_LINE_LOOP);
  glColor3f(35.f/255.f, 255.f/255.f, 103.f/255.f);
  glVertex2f(1.f, 1.f);
  glVertex2f(1.f, _size.height() - 1.f);
  glVertex2f(_size.width() - 1.f, _size.height() - 1.f);
  glVertex2f(_size.width() - 1.f, 1.f);
  glEnd();

  if (_enabled) {
    QSize font_size = _fonts->getSize(_font_tag);

    glBegin(GL_LINES);
    glColor3f(35.f/255.f, 255.f/255.f, 103.f/255.f);

    // Header separator
    glVertex2f(0.f, 6.f + font_size.height());
    glVertex2f(_size.width(), 6.f + font_size.height());
    // Footer separator
    glVertex2f(0.f, 2.f + 12*(6+font_size.height()));
    glVertex2f(_size.width(), 2.f + 12*(6+font_size.height()));
    glEnd();

    glEnable(GL_BLEND);

    _prog->bind();

    drawText(_menu->name(), 0, ALIGN_CENTER, QColor(69, 251, 247));

    for (int i = 0; i < _menu->item_count(); i++) {
      drawText(_menu->item(i)->name(), i+1, ALIGN_LEFT, QColor(69, 251, 247));
      drawText(_menu->item(i)->value(), i+1, ALIGN_RIGHT, QColor(255, 242, 216));
    }

    _prog->release();

    drawSelection();
  }

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  _fbo->release();

  _need_update = false;
}

void MenuEngine::drawSelection() {
  glShadeModel( GL_FLAT );

  QSize font_size = _fonts->getSize(_font_tag);

  glLineWidth(2.f);

  glBegin(GL_LINE_LOOP);
  if (_selection_active)
    glColor3f(255.f/255.f, 242.f/255.f, 216.f/255.f);
  else
    glColor3f(69.f/255.f, 251.f/255.f, 247.f/255.f);

  // Left border
  glVertex2f(3.f, 2.f + _selected_line*(6+font_size.height()));
  glVertex2f(3.f, (_selected_line+1)*(6+font_size.height()));
  glVertex2f(_size.width() - 3.f, (_selected_line+1)*(6+font_size.height()));
  glVertex2f(_size.width() - 3.f, 2.f + _selected_line*(6+font_size.height()));

  glEnd();
}

void MenuEngine::drawText(const QByteArray& text, int line, TextAllignement align, const QColor& col) {
  GLuint tex_id = _fonts->getTextureId(_font_tag);
  QSize font_size = _fonts->getSize(_font_tag);

  QVector<float> pos, ord, chars;
  QPoint anchor = QPoint(0, 4 + (font_size.height() + 6) * line);

  switch (align) {
    case ALIGN_CENTER:
      anchor.setX(_size.width()/2 - text.size()*font_size.width()/2);
      break;
    case ALIGN_LEFT:
      anchor.setX(4);
      break;
    case ALIGN_RIGHT:
      anchor.setX(_size.width() - 4 - text.size()*font_size.width());
      break;
    default:
      return;
  }

  // From text to vertex data
  for (int i = 0; i < text.size(); i++) {
    for (int j = 0; j < 4; j++) {
      QPointF lefttop = anchor + QPointF(i * font_size.width(), 0);
      pos.push_back(lefttop.x());
      pos.push_back(lefttop.y());
      ord.push_back(j);
      chars.push_back(text[i]);
    }
  }

  // Set shader uniforms
  glUniform2f(_uniform_locs[INFO_UNIFORM_SIZE], font_size.width(), font_size.height());
  glUniform4f(_uniform_locs[INFO_UNIFORM_COLOR], col.redF(), col.greenF(), col.blueF(), 1.f);

  // Push vertex data to VBOs, lind them toshader attributes
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

  // Bind fonts texture
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex_id);

  glDrawArrays(GL_QUADS, 0, ord.size());
  glBindTexture(GL_TEXTURE_2D, 0);
}


bool MenuEngine::initShader() {
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
