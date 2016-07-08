#ifndef MENUENGINE_H
#define MENUENGINE_H

#include <QDebug>
#include <QByteArray>
#include <QTextEncoder>
#include <QTextDecoder>

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>

#include "asmfonts.h"

class RLIMenuItem {
public:
  RLIMenuItem() { _name = QByteArray(); }
  RLIMenuItem(const QByteArray& name) : _name(name) {}

  virtual ~RLIMenuItem() {  }

  virtual QByteArray name() { return _name; }
  virtual QByteArray value() { return QByteArray(); }

  virtual void up() { }
  virtual void down() { }

  enum RLI_MENU_ITEM_TYPE { MENU, LIST, INT, FLOAT, ACTION};

  virtual RLI_MENU_ITEM_TYPE type() { return _type; }

protected:
  RLI_MENU_ITEM_TYPE _type;

private:
  QByteArray _name;
};

class RLIMenuItemAction : public RLIMenuItem {
public:
  RLIMenuItemAction(const QByteArray& name);
  ~RLIMenuItemAction() {}

private:
};


class RLIMenuItemMenu : public RLIMenuItem {
public:
  RLIMenuItemMenu(const QByteArray& name, RLIMenuItemMenu* parent);
  ~RLIMenuItemMenu();

  inline QByteArray value() { return QByteArray(); }

  inline RLIMenuItemMenu* parent() { return _parent; }
  inline RLIMenuItem* item(int i) { return _items[i]; }
  inline int item_count() { return _items.size(); }
  inline void add_item(RLIMenuItem* i) { _items.push_back(i); }

private:
  RLIMenuItemMenu* _parent;
  QVector<RLIMenuItem*> _items;
};

class RLIMenuItemList : public RLIMenuItem {
public:
  RLIMenuItemList(const QByteArray& name, const QVector<QByteArray>& variants, int def_ind);
  ~RLIMenuItemList() {}

  inline QByteArray value() { return _variants[_index]; }

  inline void up() { if (_index < _variants.size() - 1) _index++; }
  inline void down() { if (_index > 0) _index--; }

private:
  int _index;
  QVector<QByteArray> _variants;
};

class RLIMenuItemInt : public RLIMenuItem {
public:
  RLIMenuItemInt(const QByteArray& name, int min, int max, int def);
  ~RLIMenuItemInt() {}

  inline QByteArray value() { return QString::number(_value).toLatin1(); }

  inline void up() { if (_value < _max) _value++; }
  inline void down() { if (_value > _min) _value--; }

private:
  int _value;
  int _min, _max;
};

class RLIMenuItemFloat : public RLIMenuItem {
public:
  RLIMenuItemFloat(const QByteArray& name, float min, float max, float def);
  ~RLIMenuItemFloat() { }

  inline QByteArray value() { return QString::number(_value).left(5).toLatin1(); }

  inline void up() { if (_value + _step < _max) _value += _step; }
  inline void down() { if (_value - _step > _min) _value -= _step; }

private:
  float _value;
  float _step;
  float _min, _max;
};


class MenuEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit MenuEngine  (const QSize& font_size, QObject* parent = 0);
  virtual ~MenuEngine  ();

  inline QSize size() { return _size; }
  inline GLuint getTextureId() { return _fbo->texture(); }

  inline void setFonts(AsmFonts* fonts) { _fonts = fonts; }

  bool init     (const QGLContext* context);
  void resize   (const QSize& font_size);

  inline bool visible() { return _enabled; }
  inline QByteArray toQByteArray(const char* str) { return _enc->fromUnicode(_dec->toUnicode(str)); }

signals:

public slots:
  void setVisibility(bool val);
  void update();

  void onUp();
  void onDown();
  void onEnter();
  void onBack();

private:
  void initMenuTree();
  bool initShader();

  enum TextAllignement {
    ALIGN_LEFT, ALIGN_CENTER, ALIGN_RIGHT
  };

  void drawSelection();
  void drawText(const QByteArray& text, int line, TextAllignement align, const QColor& col);

  bool _initialized;
  bool _need_update;

  bool _enabled;

  QSize _size;
  QString _font_tag;

  RLIMenuItemMenu* _menu;

  RLIMenuItemMenu* _main_menu;

  int _selected_line;
  bool _selection_active;

  AsmFonts* _fonts;
  QTextEncoder* _enc;
  QTextDecoder* _dec;

  QGLFramebufferObject* _fbo;
  QGLShaderProgram* _prog;

  // -----------------------------------------------
  enum { INFO_ATTR_POSITION = 0
       , INFO_ATTR_ORDER = 1
       , INFO_ATTR_CHAR_VAL = 2
       , INFO_ATTR_COUNT = 3 } ;
  enum { INFO_UNIFORM_COLOR = 0
       , INFO_UNIFORM_SIZE = 1
       , INFO_UNIFORM_COUNT = 2 } ;

  GLuint _vbo_ids[INFO_ATTR_COUNT];
  GLuint _attr_locs[INFO_ATTR_COUNT];
  GLuint _uniform_locs[INFO_UNIFORM_COUNT];
};

#endif // MENUENGINE_H
