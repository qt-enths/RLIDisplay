#ifndef INFOENGINE_H
#define INFOENGINE_H

#include <QRectF>
#include <QVector>
#include <QDebug>

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>

#include "asmfonts.h"

struct InfoRect {
  QColor col;
  QRectF  rect;
};


struct InfoText {
  QByteArray chars;
  QString font_tag;
  QColor color;
  QPointF anchor;
  bool anchor_left;

  InfoText() { anchor_left = true; }
};


class InfoBlock : public QObject {
  Q_OBJECT
public:
  explicit InfoBlock(QObject* parent = 0);

  void clear();

  void setGeometry(const QRectF& r) { _geometry = r; _need_update = true; }
  inline const QRectF& getGeometry() { return _geometry; }
  void setBackColor(const QColor& c) { _back_color = c; _need_update = true; }
  inline const QColor& getBackColor() { return _back_color; }
  void setBorder(int w, const QColor& c) { _border_width = w; _border_color = c; _need_update = true; }
  inline const QColor& getBorderColor() { return _border_color; }
  inline int getBorderWidth() { return _border_width; }

  inline bool needUpdate() { return _need_update; }
  inline void discardUpdate() { _need_update = false; }

  int addRect(const InfoRect& t) { _rects.push_back(t); _need_update = true; return _rects.size() - 1; }
  int addText(const InfoText& t) { _texts.push_back(t); _need_update = true; return _texts.size() - 1; }

  inline int getRectCount() { return _rects.size(); }
  inline const InfoRect& getRect(int i) { return _rects[i]; }
  inline int getTextCount() { return _texts.size(); }
  inline const InfoText& getText(int i) { return _texts[i]; }

public slots:
  void setRect(int rectId, const QRectF& r) { if (rectId < _rects.size()) _rects[rectId].rect = r; _need_update = true; }
  void setText(int textId, const QByteArray& c) { if (textId < _texts.size()) _texts[textId].chars = c; _need_update = true; }

private:
  QVector<InfoRect> _rects;
  QVector<InfoText> _texts;

  QRectF   _geometry;
  QColor  _back_color;
  QColor  _border_color;
  int     _border_width;
  bool    _need_update;
};


class InfoEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit InfoEngine   (const QSize& sz, QObject* parent = 0);
  virtual ~InfoEngine   ();

  bool init             (const QGLContext* context);
  void resize           (const QSize& sz);

  inline void setFonts(AsmFonts* fonts) { _fonts = fonts; }

  inline GLuint getTextureId() { return _fbo->texture(); }
  InfoBlock* addInfoBlock();

signals:
  void send_resize      (const QSize& size);

public slots:
  void update();

private:
  void drawBlock(InfoBlock* b);
  inline void drawText(const InfoText& text);
  inline void drawRect(const QRectF& rect, const QColor& col);

  AsmFonts* _fonts;

  bool _initialized;
  bool _full_update;

  QVector<InfoBlock*> _blocks;
  QSize _size;

  // Framebuffer vars
  QGLFramebufferObject* _fbo;

  bool initShaders();

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
};

#endif // INFOENGINE_H
