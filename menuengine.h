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
#include "rlistrings.h"

class RLIMenuItem : public QObject {
  Q_OBJECT
public:
  RLIMenuItem(QObject* parent = 0);
  RLIMenuItem(char** name, QObject* parent = 0);

  virtual ~RLIMenuItem() {  }

  virtual QByteArray name(int lang_id) { return _name[lang_id]; }
  virtual QByteArray value() { return QByteArray(); }

  virtual void up() { }
  virtual void down() { }
  virtual void action() { }

  enum RLI_MENU_ITEM_TYPE { MENU, LIST, INT, FLOAT, ACTION};

  virtual RLI_MENU_ITEM_TYPE type() { return _type; }

protected:
  RLI_MENU_ITEM_TYPE _type;

private:
  QTextEncoder* _enc;
  QTextDecoder* _dec;

  QByteArray _name[RLI_LANG_COUNT];
};


class RLIMenuItemAction : public RLIMenuItem {
  Q_OBJECT
public:
  RLIMenuItemAction(char** name, QObject* parent = 0);
  ~RLIMenuItemAction() {}

  void action();

signals:
  void triggered();

private:
};


class RLIMenuItemMenu : public RLIMenuItem {
public:
  RLIMenuItemMenu(char** name, RLIMenuItemMenu* parent);
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
  Q_OBJECT
public:
  RLIMenuItemList(char** name, const QVector<QByteArray>& variants, int def_ind, QObject* parent = 0);
  ~RLIMenuItemList() {}

  inline QByteArray value() { return _variants[_index]; }

  void up();
  void down();

signals:
  void onValueChanged(const QByteArray);

private:
  int _index;
  QVector<QByteArray> _variants;
};


class RLIMenuItemInt : public RLIMenuItem {
public:
  RLIMenuItemInt(char** name, int min, int max, int def);
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
  RLIMenuItemFloat(char** name, float min, float max, float def);
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
  void languageChanged(const QByteArray& lang);

public slots:
  void setVisibility(bool val);
  void onLanguageChanged(const QByteArray& lang);

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
  RLILang _lang;
  QTextEncoder* _enc;
  QTextDecoder* _dec;
  QTextDecoder* _dec1;

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
