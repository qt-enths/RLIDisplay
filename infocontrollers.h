#ifndef INFOCONTROLLERS_H
#define INFOCONTROLLERS_H

#include <QObject>
#include <QRectF>
#include <QTextEncoder>
#include <QTextDecoder>

#include "infoengine.h"

class InfoBlockController : public QObject {
  Q_OBJECT
public:
  explicit InfoBlockController(QObject* parent = 0);
  virtual void setupBlock(InfoBlock* b, const QSize& size);

public slots:
  virtual void onResize(const QSize& size);
  virtual void onLanguageChanged(int lang_id);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

protected:
  virtual void initBlock(const QSize& size) = 0;

  InfoBlock* _block;
  QTextEncoder* enc;
  QTextDecoder* dec;
};



class ValueBarController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ValueBarController(char** name, const QPoint& left_top, int title_width, int def_val, QObject* parent = 0);

public slots:
  void onValueChanged(int val);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _val;

  char** _name;
  int _title_width;
  QPoint _left_top;

  int _ttl_text_id;
  int _val_rect_id;
  int _val_text_id;
};



class ScaleController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ScaleController(QObject* parent = 0);

public slots:
  void scale_changed(std::pair<int, int> scale);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _scl1_text_id;
  int _scl2_text_id;
  int _unit_text_id;
};



class RegimeController : public InfoBlockController {
  Q_OBJECT
public:
  explicit RegimeController(QObject* parent = 0);

public slots:

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);
};



class CourseController : public InfoBlockController {
  Q_OBJECT
public:
  explicit CourseController(QObject* parent = 0);

public slots:
  void course_changed(float course);
  void speed_changed(float speed);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _crs_text_id;
  int _spd_text_id;
};



class PositionController : public InfoBlockController {
  Q_OBJECT
public:
  explicit PositionController(QObject* parent = 0);

public slots:
  void pos_changed(std::pair<float, float> pos);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _lat_text_id;
  int _lon_text_id;
};



class BlankController : public InfoBlockController {
  Q_OBJECT
public:
  explicit BlankController(QObject* parent = 0);

public slots:

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);
};



class CursorController : public InfoBlockController {
  Q_OBJECT
public:
  explicit CursorController(QObject* parent = 0);

public slots:
  void cursor_moved(float peleng, float distance);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _pel_text_id;
  int _dis_text_id;
};



class ClockController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ClockController(QObject* parent = 0);

public slots:
  void second_changed();

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);
  int _text_id;
};


#endif // INFOCONTROLLERS_H
