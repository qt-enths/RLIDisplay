#ifndef INFOENGINE_H
#define INFOENGINE_H

#include <QRectF>
#include <QVector>
#include <QDebug>

#include <QTextEncoder>
#include <QTextDecoder>

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>

#include "asmfonts.h"
#include "rlistrings.h"

struct InfoRect {
  QColor col;
  QRect  rect;
};

struct InfoText {
  QByteArray str[LANG_COUNT];
  QString font_tag;
  QColor color;
  QPoint anchor;
  bool anchor_left;

  InfoText() { anchor_left = true; }
};




class InfoBlock : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit InfoBlock(QObject* parent = 0);
  virtual ~InfoBlock();

  bool init(const QGLContext* context);

  void clear();

  void setGeometry(const QRect& r);
  inline const QRect& getGeometry()         { return _geometry; }
  void setBackColor(const QColor& c)        { _back_color = c; _need_update = true; }
  inline const QColor& getBackColor()       { return _back_color; }
  void setBorder(int w, const QColor& c)    { _border_width = w; _border_color = c; _need_update = true; }
  inline const QColor& getBorderColor()     { return _border_color; }
  inline int getBorderWidth()               { return _border_width; }

  inline bool needUpdate()                  { return _need_update; }
  inline void discardUpdate()               { _need_update = false; }

  int addRect(const InfoRect& t)            { _rects.push_back(t); _need_update = true; return _rects.size() - 1; }
  int addText(const InfoText& t)            { _texts.push_back(t); _need_update = true; return _texts.size() - 1; }

  inline int getRectCount()                 { return _rects.size(); }
  inline const InfoRect& getRect(int i)     { return _rects[i]; }
  inline int getTextCount()                 { return _texts.size(); }
  inline const InfoText& getText(int i)     { return _texts[i]; }

  inline GLuint getTextureId()              { return _fbo->texture(); }
  QGLFramebufferObject* fbo()               { return _fbo; }

public slots:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  bool _initialized;

  QVector<InfoRect> _rects;
  QVector<InfoText> _texts;

  QRect   _geometry;
  QColor  _back_color;
  QColor  _border_color;
  int     _border_width;
  bool    _need_update;

  // Framebuffer vars
  QGLFramebufferObject* _fbo;
};




class InfoEngine : public QObject, protected QGLFunctions  {
  Q_OBJECT
public:
  explicit InfoEngine   (QObject* parent = 0);
  virtual ~InfoEngine   ();

  bool init(const QGLContext* context);
  inline void setFonts(AsmFonts* fonts) { _fonts = fonts; }

  inline int getBlockCount() { return _blocks.size(); }
  inline int getBlockTextId(int b_id) { return _blocks[b_id]->getTextureId(); }
  inline const QRect& getBlockGeometry(int b_id) { return _blocks[b_id]->getGeometry(); }

  InfoBlock* addInfoBlock();

public slots:
  void update();
  void onLanguageChanged(const QByteArray& lang);

private:
  void updateBlock(InfoBlock* b);
  inline void drawText(const InfoText& text);
  inline void drawRect(const QRect& rect, const QColor& col);

  AsmFonts* _fonts;
  RLILang _lang;

  bool _initialized;
  bool _full_update;

  QVector<InfoBlock*> _blocks;

  bool initShaders();

  const QGLContext* _context;

  // Info shader program
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

  QTextEncoder* enc;
  QTextDecoder* dec;
  QTextDecoder* dec1;
};

#endif // INFOENGINE_H
