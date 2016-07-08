#include "menuengine.h"

RLIMenuItemMenu::RLIMenuItemMenu(const QByteArray& name, RLIMenuItemMenu* parent) : RLIMenuItem(name) {
  _type = MENU;
  _parent = parent;
}

RLIMenuItemMenu::~RLIMenuItemMenu() {
  qDeleteAll(_items);
}

RLIMenuItemAction::RLIMenuItemAction(const QByteArray& name) : RLIMenuItem(name) {
  _type = ACTION;
}

RLIMenuItemList::RLIMenuItemList(const QByteArray& name, const QVector<QByteArray>& variants, int def_ind) : RLIMenuItem(name) {
  _type = LIST;
  _variants = variants;
  _index = def_ind;
}

RLIMenuItemInt::RLIMenuItemInt(const QByteArray& name, int min, int max, int def)  : RLIMenuItem(name) {
  _type = INT;
  _min = min;
  _max = max;
  _value = def;
}

RLIMenuItemFloat::RLIMenuItemFloat(const QByteArray& name, float min, float max, float def) : RLIMenuItem(name) {
  _type = FLOAT;
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
  QVector<QByteArray> val_list;

  RLIMenuItemMenu* m0 = new RLIMenuItemMenu(toQByteArray("ГЛАВНОЕ МЕНЮ"), NULL);

  // --------------------------
  RLIMenuItemMenu* m00 = new RLIMenuItemMenu(toQByteArray("ЯРКОСТЬ"), m0);
  m0->add_item(m00);

  RLIMenuItemInt* i000 = new RLIMenuItemInt(toQByteArray("РЛ ВИДЕО"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i000));

  RLIMenuItemInt* i001 = new RLIMenuItemInt(toQByteArray("ИЗМЕРИТ СРЕДСТВА"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i001));

  RLIMenuItemInt* i002 = new RLIMenuItemInt(toQByteArray("МД"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i002));

  RLIMenuItemInt* i003 = new RLIMenuItemInt(toQByteArray("СИМВОЛЫ АРП"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i003));

  RLIMenuItemInt* i004 = new RLIMenuItemInt(toQByteArray("СЛЕДЫ"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i004));

  RLIMenuItemInt* i005 = new RLIMenuItemInt(toQByteArray("ПАНЕЛЬ"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i005));

  RLIMenuItemInt* i006 = new RLIMenuItemInt(toQByteArray("ПУЛЬТ"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i006));

  RLIMenuItemInt* i007 = new RLIMenuItemInt(toQByteArray("ПАНЕЛЬ"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i007));

  RLIMenuItemInt* i008 = new RLIMenuItemInt(toQByteArray("КАРТА"), 0, 255, 255);
  m00->add_item(static_cast<RLIMenuItem*>(i008));

  val_list.append(toQByteArray("ДЕНЬ"));
  val_list.append(toQByteArray("НОЧЬ"));
  RLIMenuItemList* i009 = new RLIMenuItemList(toQByteArray("ДЕНЬ/НОЧЬ"), val_list, 0);
  val_list.clear();
  m00->add_item(static_cast<RLIMenuItem*>(i009));


  // --------------------------
  RLIMenuItemMenu* m01 = new RLIMenuItemMenu(toQByteArray("АВТОПРОКЛАДКА"), m0);
  m0->add_item(m01);

  RLIMenuItemFloat* i010 = new RLIMenuItemFloat(toQByteArray("ОПАСН ДКС миль"), 0.01f, 8.f, 2.f);
  m01->add_item(static_cast<RLIMenuItem*>(i010));

  RLIMenuItemInt* i011 = new RLIMenuItemInt(toQByteArray("ОПАСН ВКС мин"), 5, 60, 30);
  m01->add_item(static_cast<RLIMenuItem*>(i011));

  RLIMenuItemInt* i012 = new RLIMenuItemInt(toQByteArray("ВЕКТОРЫ мин"), 5, 60, 30);
  m01->add_item(static_cast<RLIMenuItem*>(i012));

  val_list.append(toQByteArray("1"));
  val_list.append(toQByteArray("2"));
  val_list.append(toQByteArray("3"));
  val_list.append(toQByteArray("6"));
  val_list.append(toQByteArray("12"));
  RLIMenuItemList* i013 = new RLIMenuItemList(toQByteArray("СЛЕДЫ/ТЧК"), val_list, 3);
  val_list.clear();
  m01->add_item(static_cast<RLIMenuItem*>(i013));

  val_list.append(toQByteArray("ВКЛ"));
  val_list.append(toQByteArray("ОТКЛ"));
  RLIMenuItemList* i014 = new RLIMenuItemList(toQByteArray("НОМЕРА ЦЕЛЕЙ"), val_list, 1);
  val_list.clear();
  m01->add_item(static_cast<RLIMenuItem*>(i014));

  RLIMenuItemAction* i015 = new RLIMenuItemAction(toQByteArray("СБРОС ЗОНЫ"));
  m01->add_item(static_cast<RLIMenuItem*>(i015));

  RLIMenuItemAction* i016 = new RLIMenuItemAction(toQByteArray("УСТАН ЗОНЫ"));
  m01->add_item(static_cast<RLIMenuItem*>(i016));

  val_list.append(toQByteArray("ВОДА"));
  val_list.append(toQByteArray("ГРУНТ"));
  RLIMenuItemList* i017 = new RLIMenuItemList(toQByteArray("ЛИД"), val_list, 0);
  val_list.clear();
  m01->add_item(static_cast<RLIMenuItem*>(i017));


  // --------------------------
  RLIMenuItemMenu* m02 = new RLIMenuItemMenu(toQByteArray("УСТАНОВКИ"), m0);
  m0->add_item(m02);

  RLIMenuItemInt* i020 = new RLIMenuItemInt(toQByteArray("РПЧ"), 0, 255, 5);
  m02->add_item(static_cast<RLIMenuItem*>(i020));

  val_list.append(toQByteArray("МИЛИ"));
  val_list.append(toQByteArray("КМ"));
  RLIMenuItemList* i021 = new RLIMenuItemList(toQByteArray("СМЕНА ВД"), val_list, 0);
  val_list.clear();
  m02->add_item(static_cast<RLIMenuItem*>(i021));

  val_list.append(toQByteArray("ЛАГ"));
  val_list.append(toQByteArray("РУЧ"));
  RLIMenuItemList* i022 = new RLIMenuItemList(toQByteArray("ДАТЧИК СКОР"), val_list, 0);
  val_list.clear();
  m02->add_item(static_cast<RLIMenuItem*>(i022));

  RLIMenuItemFloat* i023 = new RLIMenuItemFloat(toQByteArray("СК  РУЧ  УЗ"), 0.f, 90.f, 5.f);
  m02->add_item(i023);

  val_list.append(toQByteArray("АОЦ"));
  val_list.append(toQByteArray("СНС"));
  val_list.append(toQByteArray("ГК-Л(В)"));
  val_list.append(toQByteArray("ДЛГ"));
  RLIMenuItemList* i024 = new RLIMenuItemList(toQByteArray("ДАТЧИК СТАБ"), val_list, 2);
  val_list.clear();
  m02->add_item(i024);

  RLIMenuItemInt* i025 = new RLIMenuItemInt(toQByteArray("ТАЙМЕР мин"), 0, 90, 0);
  m02->add_item(i025);

  val_list.append(toQByteArray("РУС"));
  val_list.append(toQByteArray("АНГЛ"));
  RLIMenuItemList* i026 = new RLIMenuItemList(toQByteArray("ЯЗЫК"), val_list, 0);
  val_list.clear();
  m02->add_item(i026);

  RLIMenuItemFloat* i027 = new RLIMenuItemFloat(toQByteArray("СОГЛАС ГИРО"), 0.f, 359.9f, 0.f);
  m02->add_item(i027);

  RLIMenuItemInt* i028 = new RLIMenuItemInt(toQByteArray("ОПАС ГЛУБ м"), 1, 100, 1);
  m02->add_item(i028);

  RLIMenuItemInt* i029 = new RLIMenuItemInt(toQByteArray("ЯКОРНАЯ СТ м"), 1, 100, 1);
  m02->add_item(i029);

  val_list.append(toQByteArray("ОТКЛ"));
  RLIMenuItemList* i0210 = new RLIMenuItemList(toQByteArray("ШУМ СТАБ"), val_list, 0);
  val_list.clear();
  m02->add_item(i0210);


  // --------------------------
  RLIMenuItemMenu* m03 = new RLIMenuItemMenu(toQByteArray("КОНТРОЛЬ"), m0);
  m0->add_item(m03);

  RLIMenuItemInt* i030 = new RLIMenuItemInt(toQByteArray("СТРОБ АС"), 1, 2, 1);
  m03->add_item(static_cast<RLIMenuItem*>(i030));

  val_list.append(toQByteArray("ВКЛ"));
  val_list.append(toQByteArray("ОТКЛ"));
  RLIMenuItemList* i031 = new RLIMenuItemList(toQByteArray("ИМИТАЦИЯ ЦЕЛИ"), val_list, 1);
  m03->add_item(static_cast<RLIMenuItem*>(i031));

  RLIMenuItemList* i032 = new RLIMenuItemList(toQByteArray("ЭП"), val_list, 1);
  m03->add_item(static_cast<RLIMenuItem*>(i032));

  RLIMenuItemList* i033 = new RLIMenuItemList(toQByteArray("КИМ"), val_list, 1);
  val_list.clear();
  m03->add_item(static_cast<RLIMenuItem*>(i033));

  RLIMenuItemFloat* i034 = new RLIMenuItemFloat(toQByteArray("ТОК МАГНЕТР"), 0.f, 3.f, 0.f);
  m03->add_item(static_cast<RLIMenuItem*>(i034));

  val_list.append(toQByteArray("ДА"));
  val_list.append(toQByteArray("НЕТ"));
  RLIMenuItemList* i035 = new RLIMenuItemList(toQByteArray("ЗАПУСК И"), val_list, 0);
  m03->add_item(static_cast<RLIMenuItem*>(i035));

  RLIMenuItemList* i036 = new RLIMenuItemList(toQByteArray("РЛ ВИДЕО"), val_list, 1);
  m03->add_item(static_cast<RLIMenuItem*>(i036));

  RLIMenuItemList* i037 = new RLIMenuItemList(toQByteArray("КУА"), val_list, 1);
  m03->add_item(static_cast<RLIMenuItem*>(i037));

  RLIMenuItemList* i038 = new RLIMenuItemList(toQByteArray("ОК"), val_list, 1);
  val_list.clear();
  m03->add_item(static_cast<RLIMenuItem*>(i038));

  // --------------------------
  RLIMenuItemMenu* m04 = new RLIMenuItemMenu(toQByteArray("КАРТА"), m0);
  m0->add_item(m04);

  RLIMenuItemInt* i040 = new RLIMenuItemInt(toQByteArray("ВЫЗОВ КАРТЫ №"), 1, 4, 1);
  m04->add_item(static_cast<RLIMenuItem*>(i040));

  val_list.append(toQByteArray("ОТКЛ"));
  for (int i = 1; i < 11; i++)
    val_list.append(toQByteArray(QString::number(i).toLocal8Bit()));
  RLIMenuItemList* i041 = new RLIMenuItemList(toQByteArray("СЛЕДЫ/ТЧК"), val_list, 0);
  m04->add_item(static_cast<RLIMenuItem*>(i041));

  RLIMenuItemAction* i042 = new RLIMenuItemAction(toQByteArray("-------------------------"));
  m04->add_item(static_cast<RLIMenuItem*>(i042));

  RLIMenuItemInt* i043 = new RLIMenuItemInt(toQByteArray("ВЫЗОВ КАРТЫ №"), 40, 1000, 200);
  m04->add_item(static_cast<RLIMenuItem*>(i043));

  RLIMenuItemAction* i044 = new RLIMenuItemAction(toQByteArray("ЛИНИЯ МАРШРУТА"));
  m04->add_item(static_cast<RLIMenuItem*>(i044));

  RLIMenuItemList* i045 = new RLIMenuItemList(toQByteArray("НОМЕР ТОЧКИ"), val_list, 1);
  val_list.clear();
  m04->add_item(static_cast<RLIMenuItem*>(i045));

  val_list.append(toQByteArray("#"));
  val_list.append(toQByteArray("@"));
  val_list.append(toQByteArray("&"));
  RLIMenuItemList* i046 = new RLIMenuItemList(toQByteArray("СИМВОЛЫ"), val_list, 0);
  val_list.clear();
  m04->add_item(static_cast<RLIMenuItem*>(i046));

  RLIMenuItemInt* i047 = new RLIMenuItemInt(toQByteArray("ЗАПИСЬ КАРТЫ №"), 1, 4, 1);
  m04->add_item(static_cast<RLIMenuItem*>(i047));

  // --------------------------
  RLIMenuItemMenu* m05 = new RLIMenuItemMenu(toQByteArray("ОПОЗНАВАНИЕ"), m0);
  m0->add_item(m05);

  val_list.append(toQByteArray("НЕТ"));
  val_list.append(toQByteArray("СВОЙ"));
  val_list.append(toQByteArray("ЧУЖОЙ"));
  RLIMenuItemList* i050 = new RLIMenuItemList(toQByteArray("ПРИЗНАК"), val_list, 0);
  val_list.clear();
  m05->add_item(static_cast<RLIMenuItem*>(i050));

  val_list.append(toQByteArray("НЕТ"));
  val_list.append(toQByteArray("КРУГ"));
  val_list.append(toQByteArray("СЕКТ"));
  RLIMenuItemList* i051 = new RLIMenuItemList(toQByteArray("ЗАПРОС"), val_list, 0);
  val_list.clear();
  m05->add_item(static_cast<RLIMenuItem*>(i051));

  val_list.append(toQByteArray("ОТКЛ"));
  val_list.append(toQByteArray("ВКЛ"));
  RLIMenuItemList* i052 = new RLIMenuItemList(toQByteArray("ЗАПРЕТ ИЗЛУЧ"), val_list, 0);
  val_list.clear();
  m05->add_item(static_cast<RLIMenuItem*>(i052));

  // --------------------------
  _main_menu = m0;
  _menu = m0;
}

MenuEngine::~MenuEngine() {
  delete _prog;

  if (!_initialized)
    return;

  delete _fbo;
  glDeleteBuffers(INFO_ATTR_COUNT, _vbo_ids);
  delete _main_menu;
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
  if (_menu->item(_selected_line - 1)->type() == RLIMenuItem::MENU) {
    _menu = dynamic_cast<RLIMenuItemMenu*>(_menu->item(_selected_line - 1));
    _selected_line = 1;
  } else
    _selection_active = !_selection_active;

  _need_update = true;
}

void MenuEngine::onBack() {
  if (!_selection_active && _menu->parent() != NULL) {
     int _new_selection = 1;
     for (int i = 0; i < _menu->parent()->item_count(); i++)
       if (_menu->parent()->item(i) == _menu)
         _new_selection = i+1;

     _selected_line = _new_selection;
     _menu = _menu->parent();
     _need_update = true;
  }
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
