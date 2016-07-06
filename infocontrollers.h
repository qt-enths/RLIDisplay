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

signals:
  void setRect(int rectId, const QRectF& r);
  void setText(int textId, const QByteArray& c);

protected:
  virtual void initBlock(const QSize& size) = 0;

  InfoBlock* _block;
  QTextEncoder* enc;
  QTextDecoder* dec;
};


class GainController : public InfoBlockController {
  Q_OBJECT
public:
  explicit GainController(QObject* parent = 0);

public slots:
  void onGainChanged(int val);

signals:
  void setRect(int rectId, const QRectF& r);
  void setText(int textId, const QByteArray& c);

private:
  void initBlock(const QSize& size);
  int _gain_rect_id;
};



class CursorController : public InfoBlockController {
  Q_OBJECT
public:
  explicit CursorController(QObject* parent = 0);

public slots:
  void cursor_moved(float peleng, float distance);

signals:
  void setRect(int rectId, const QRectF& r);
  void setText(int textId, const QByteArray& c);

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
  void setRect(int rectId, const QRectF& r);
  void setText(int textId, const QByteArray& c);

private:
  void initBlock(const QSize& size);
  int _text_id;
};


#endif // INFOCONTROLLERS_H
