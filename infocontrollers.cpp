#include "infocontrollers.h"

#include <QTime>
#include <QDebug>

InfoBlockController::InfoBlockController(QObject* parent) : QObject(parent) {
  _block = NULL;
  enc = QTextCodec::codecForName("cp866")->makeEncoder();
  dec = QTextCodec::codecForName("UTF8")->makeDecoder();
}

void InfoBlockController::onResize(const QSize& size) {
  if (_block == NULL)
    return;

  _block->clear();
  initBlock(size);
}

void InfoBlockController::setupBlock(InfoBlock* b, const QSize& size) {
  _block = b;
  initBlock(size);
}

//------------------------------------------------------------------------------

GainController::GainController(QObject* parent) : InfoBlockController(parent) {
  _gain_rect_id = -1;
}

void GainController::initBlock(const QSize& size) {
  Q_UNUSED(size);

  const QColor txtCol(69, 251, 247);
  const QColor brdCol(35, 255, 103, 255);
  const QColor bckCol(0, 0, 0, 255);

  _block->setGeometry(QRectF(10, 10, 12*8 + 50, 23));
  _block->setBackColor(bckCol);
  _block->setBorder(2, brdCol);

  InfoRect r;
  r.col = brdCol;
  r.rect = QRectF(12*8 + 6, 0, 2, 23);
  _block->addRect(r);

  r.rect = QRectF(12*8 + 8, 14, 0, 15);
  _gain_rect_id = _block->addRect(r);

  InfoText t;
  t.anchor = QPointF(4, 5);
  t.font_tag = "12x14";

  t.chars = enc->fromUnicode(dec->toUnicode("УСИЛЕНИЕ"));

  t.anchor_left = true;
  t.color = txtCol;
  _block->addText(t);
}

void GainController::onGainChanged(int val) {
  if (_gain_rect_id != -1) {
    emit setRect(_gain_rect_id, QRectF(12*8 + 8, 4, val, 15));
  }
}

//------------------------------------------------------------------------------

CursorController::CursorController(QObject* parent) : InfoBlockController(parent) {
  _pel_text_id = -1;
  _dis_text_id = -1;
}

void CursorController::initBlock(const QSize& size) {
  const QColor txtStaticCol(69, 251, 247);
  const QColor txtDynamicCol(255, 242, 216);

  const QColor brdCol(35, 255, 103, 255);
  const QColor bckCol(0, 0, 0, 255);

  _block->setGeometry(QRectF(size.width() - 224 - 10, size.height() - 64 - 10, 224, 70));
  _block->setBackColor(bckCol);
  _block->setBorder(2, brdCol);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.anchor_left = true;
  t.color = txtStaticCol;

  t.anchor = QPointF(80, 5);
  t.chars = enc->fromUnicode(dec->toUnicode("КУРСОР"));
  _block->addText(t);

  t.anchor = QPointF(4, 32);
  t.chars = enc->fromUnicode(dec->toUnicode("ПЕЛЕНГ"));
  _block->addText(t);

  t.anchor = QPointF(4, 52);
  t.chars = enc->fromUnicode(dec->toUnicode("ДАЛЬНОСТЬ"));
  _block->addText(t);

  //
  t.anchor_left = false;
  t.color = txtDynamicCol;

  t.anchor = QPointF(184, 32);
  t.chars = enc->fromUnicode(dec->toUnicode("0"));
  _pel_text_id = _block->addText(t);

  t.anchor = QPointF(184, 52);
  t.chars = enc->fromUnicode(dec->toUnicode("0"));
  _dis_text_id = _block->addText(t);

  //
  t.font_tag = "8x14";
  t.anchor_left = true;
  t.color = txtStaticCol;

  QByteArray str(" ");
  str[0] = static_cast<unsigned char>(248);
  t.chars = str;
  t.anchor = QPoint(186, 32);
  _block->addText(t);

  t.chars = enc->fromUnicode(dec->toUnicode("миль"));
  t.anchor = QPoint(186, 52);
  _block->addText(t);
}

void CursorController::cursor_moved(float peleng, float distance) {
  if (_pel_text_id != -1)
    emit setText(_pel_text_id, QString::number(peleng, 'f', 2).left(5).toLocal8Bit());

  if (_dis_text_id != -1)
    emit setText(_dis_text_id, QString::number(distance, 'f', 2).left(5).toLocal8Bit());
}

//------------------------------------------------------------------------------

ClockController::ClockController(QObject* parent) : InfoBlockController(parent) {
  _text_id = -1;
}

void ClockController::initBlock(const QSize& size) {
  const QColor txtStaticCol(69, 251, 247);
  const QColor txtDynamicCol(255, 242, 216);

  const QColor bckCol(0, 0, 0, 0);

  _block->setGeometry(QRectF(size.width() - 224 - 10, 64 + 10, 224, 20));
  _block->setBackColor(bckCol);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.anchor_left = true;
  t.color = txtStaticCol;
  t.anchor = QPointF(4, 4);
  t.chars = enc->fromUnicode(dec->toUnicode("ВРЕМЯ"));
  _block->addText(t);

  t.color = txtDynamicCol;
  t.anchor_left = false;
  t.anchor = QPointF(204, 3);
  t.chars = QTime::currentTime().toString().toLocal8Bit();
  _text_id = _block->addText(t);
}

void ClockController::second_changed() {
  emit setText(_text_id, QTime::currentTime().toString().toLocal8Bit());
}
