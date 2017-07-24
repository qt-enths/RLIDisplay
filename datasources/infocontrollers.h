#ifndef INFOCONTROLLERS_H
#define INFOCONTROLLERS_H

#include <QObject>
#include <QRectF>
#include <QTextEncoder>
#include <QTextDecoder>

#include "../rliconfig.h"

#include "../datasources/radarscale.h"
#include "../layers/infoengine.h"
#include "../layers/targetengine.h"

class InfoBlockController : public QObject {
  Q_OBJECT
public:
  explicit InfoBlockController(QObject* parent = 0);
  virtual void setupBlock(InfoBlock* b, const QSize& size, const RLIPanelInfo& panelInfo);

public slots:
  virtual void resize(const QSize& size, const RLIPanelInfo& panelInfo);
  virtual void onLanguageChanged(int lang_id);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

protected:
  void geometrySetup();

  virtual void initBlock(const RLIPanelInfo& panelInfo) = 0;
  void setInfoTextStr(InfoText& t, char** str);
  void setInfoTextBts(InfoText& t, QByteArray str);

  TextAllign allignFromString(const QString& s);
  QRect rectFromString(const QString& s);

  InfoBlock* _block;
  QTextEncoder* enc;
  QTextDecoder* dec;
};



class ValueBarController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ValueBarController(char** name, int max_val, QObject* parent = 0);

public slots:
  void onValueChanged(int val);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const RLIPanelInfo& panelInfo);

  int _val;
  int _maxval;
  int _bar_width;
  char** _name;

  int _ttl_text_id;
  int _val_rect_id;
  int _val_text_id;
};



class LabelController : public InfoBlockController {
  Q_OBJECT
public:
  explicit LabelController(char** text, QObject* parent = 0);

public slots:
  void onTextChanged(char** text);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const RLIPanelInfo& panelInfo);

  char** _text;
  int _text_id;
};





class ScaleController : public InfoBlockController {
  Q_OBJECT
public:
  explicit ScaleController(QObject* parent = 0);

public slots:
  void onScaleChanged(RadarScale scale);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);
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
  void initBlock(const RLIPanelInfo& panelInfo);
};



class TailsController : public InfoBlockController {
  Q_OBJECT
public:
  explicit TailsController(QObject* parent = 0);

public slots:
  void onTailsModeChanged(int mode, int minutes);

signals:
  void setRect(int rectId, const QRect& r);
  void setText(int textId, int lang_id, const QByteArray& str);

private:
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);
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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);
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
  void initBlock(const RLIPanelInfo& panelInfo);

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
  void initBlock(const RLIPanelInfo& panelInfo);

  int _vd_text_id;
};

#endif // INFOCONTROLLERS_H
