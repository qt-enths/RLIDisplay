#ifndef INFOCONTROLLERS_H
#define INFOCONTROLLERS_H

#include <QObject>
#include <QRectF>
#include <QTextEncoder>
#include <QTextDecoder>

#include "infoengine.h"
#include "targetengine.h"

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
  void setInfoTextStr(InfoText& t, char** str);
  void setInfoTextBts(InfoText& t, QByteArray str);

  InfoBlock* _block;
  QTextEncoder* enc;
  QTextDecoder* dec;
};



class ValueBarController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ValueBarController(char** name, const QPoint& left_top, int title_width, int def_val, QObject* parent = 0);

  void setMaxValue(int val);

public slots:
  void onValueChanged(int val);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _val;
  int _maxval;

  char** _name;
  int _title_width;
  QPoint _left_top;

  int _ttl_text_id;
  int _val_rect_id;
  int _val_text_id;
};



class LableController : public InfoBlockController {
  Q_OBJECT
public:
  explicit LableController(char** text, const QRect& r, QString font_tag, QObject* parent = 0);

public slots:
  void onTextChanged(char** text);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  QString _font_tag;
  QRect _geom;
  char** _text;
  int _text_id;
};





class ScaleController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ScaleController(QObject* parent = 0);

public slots:
  void scale_changed(std::pair<QByteArray, QByteArray> scale);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _scl1_text_id;
  int _scl2_text_id;
  int _unit_text_id;
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
  void pos_changed(QVector2D pos);

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



class DangerController : public InfoBlockController {
  Q_OBJECT
public:
  explicit DangerController(QObject* parent = 0);

public slots:

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);
};



class TailsController : public InfoBlockController {
  Q_OBJECT
public:
  explicit TailsController(QObject* parent = 0);

  enum
  {
      TAILMODE_FIRST  = 0,
      TAILMODE_OFF    = 0,
      TAILMODE_RADAR  = 1,
      TAILMODE_DOTS   = 2,
      TAILMODE_LAST   = 2
  };

public slots:
    void tails_count_changed(int count);
    void tails_changed(int mode, const QByteArray count);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _mode_text_id;
  int _min_text_id;
  int _minutes;
};



class DangerDetailsController : public InfoBlockController {
  Q_OBJECT
public:
  explicit DangerDetailsController(QObject* parent = 0);

public slots:

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _dks_text_id;
  int _vks_text_id;
};



class VectorController : public InfoBlockController {
  Q_OBJECT
public:
  explicit VectorController(QObject* parent = 0);

public slots:

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);
};



class TargetsController : public InfoBlockController {
  Q_OBJECT
public:
  explicit TargetsController(QObject* parent = 0);

public slots:
  void onTargetCountChanged(int count);
  void updateTarget(const QString& tag, const RadarTarget& trgt);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _count;
  RadarTarget _target;

  int _trg_text_id;
  int _cnt_text_id;
  int _cog_text_id;
  int _sog_text_id;
};



class CursorController : public InfoBlockController {
  Q_OBJECT
public:
  explicit CursorController(QObject* parent = 0);

public slots:
  void cursor_moved(float peleng, float distance, const char * dist_fmt);

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



class VnController : public InfoBlockController {
  Q_OBJECT
public:
  explicit VnController(QObject* parent = 0);

public slots:
  void display_brg(float brg, float crsangle);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _p_text_id;
  int _cu_text_id;
  int _board_ptr_id;
};



class VdController : public InfoBlockController {
  Q_OBJECT
public:
  explicit VdController(QObject* parent = 0);

public slots:
  void display_distance(float dist, const char * fmt);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const QSize& size);

  int _vd_text_id;
};

#endif // INFOCONTROLLERS_H
