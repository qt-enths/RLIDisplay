#include "infocontrollers.h"

#include "rlistrings.h"

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

void InfoBlockController::onLanguageChanged(int lang_id) {
  Q_UNUSED(lang_id);
}

void InfoBlockController::setupBlock(InfoBlock* b, const QSize& size) {
  _block = b;
  initBlock(size);
}

//------------------------------------------------------------------------------

ValueBarController::ValueBarController(char** name, const QPoint& left_top, int title_width, int def_val, QObject* parent) : InfoBlockController(parent) {
  _val_rect_id = -1;
  _val = def_val;
  _title_width = title_width;
  _name = name;
  _left_top = left_top;
}

void ValueBarController::initBlock(const QSize& size) {
  Q_UNUSED(size);

  const QColor txtCol(69, 251, 247);
  const QColor brdCol(35, 255, 103, 255);
  const QColor bckCol(0, 0, 0, 255);

  _block->setGeometry(QRect(_left_top.x(), _left_top.y(), 12*_title_width + 60, 23));
  _block->setBackColor(bckCol);
  _block->setBorder(2, brdCol);

  InfoRect r;
  r.col = brdCol;
  r.rect = QRect(12*_title_width + 6, 0, 2, 23);
  _block->addRect(r);

  r.rect = QRect(12*_title_width + 8, 14, 0, 15);
  _val_rect_id = _block->addRect(r);

  InfoText t;
  t.font_tag = "12x14";
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(_name[i]));

  t.anchor = QPoint(4 + 6*(_title_width - t.str[0].size()), 5);
  t.anchor_left = true;
  t.color = txtCol;
  _ttl_text_id = _block->addText(t);

  t.anchor = QPoint(12*_title_width + 9, 5);
  t.font_tag = "12x14";
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray();
  t.anchor_left = true;
  t.color = txtCol;
  _val_text_id = _block->addText(t);

  onValueChanged(_val);
}

void ValueBarController::onValueChanged(int val) {
  _val = val;

  if (_val_rect_id != -1) {
    if (_val >= 0)
      emit setRect(_val_rect_id, QRect(12*_title_width + 8, 4, val, 15));
    else {
      emit setRect(_val_rect_id, QRect(12*_title_width + 8, 4, 0, 15));

      for (int i = 0; i < RLI_LANG_COUNT; i++)
        emit setText(_val_text_id, i, enc->fromUnicode(dec->toUnicode(RLIStrings::nOff[i])));
    }
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

  _block->setGeometry(QRect(size.width() - 224 - 10, size.height() - 64 - 10, 224, 70));
  _block->setBackColor(bckCol);
  _block->setBorder(2, brdCol);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.anchor_left = true;
  t.color = txtStaticCol;

  t.anchor = QPoint(80, 5);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nMrk[i]));
  _block->addText(t);

  t.anchor = QPoint(4, 32);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nBear[i]));
  _block->addText(t);

  t.anchor = QPoint(4, 52);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nRng[i]));
  _block->addText(t);

  //
  t.font_tag = "12x14";
  t.anchor_left = false;
  t.color = txtDynamicCol;

  t.anchor = QPoint(184, 32);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode("0"));
  _pel_text_id = _block->addText(t);

  t.anchor = QPoint(184, 52);
  _dis_text_id = _block->addText(t);

  t.font_tag = "8x14";
  t.anchor_left = true;
  t.color = txtStaticCol;

  QByteArray str(" ");
  str[0] = static_cast<unsigned char>(248);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nGrad[i]));
  t.anchor = QPoint(186, 32);
  _block->addText(t);

  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nNM[i]));
  t.anchor = QPoint(186, 52);
  _block->addText(t);
}

void CursorController::cursor_moved(float peleng, float distance) {
  QByteArray str[RLI_LANG_COUNT];

  if (_pel_text_id != -1)
    for (int i = 0; i < RLI_LANG_COUNT; i++)
      emit setText(_pel_text_id, i, QString::number(peleng, 'f', 2).left(5).toLocal8Bit());

  if (_dis_text_id != -1)
    for (int i = 0; i < RLI_LANG_COUNT; i++)
      emit setText(_dis_text_id, i, QString::number(distance, 'f', 2).left(5).toLocal8Bit());
}

//------------------------------------------------------------------------------

ClockController::ClockController(QObject* parent) : InfoBlockController(parent) {
  _text_id = -1;
}

void ClockController::initBlock(const QSize& size) {
  const QColor txtStaticCol(69, 251, 247);
  const QColor txtDynamicCol(255, 242, 216);

  const QColor bckCol(0, 0, 0, 0);

  _block->setGeometry(QRect(size.width() - 224 - 10, 64 + 10, 224, 20));
  _block->setBackColor(bckCol);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.anchor_left = true;
  t.color = txtStaticCol;
  t.anchor = QPoint(4, 3);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nTime[i]));
  _block->addText(t);

  t.color = txtDynamicCol;
  t.anchor_left = false;
  t.anchor = QPoint(204, 3);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QTime::currentTime().toString().toLocal8Bit();
  _text_id = _block->addText(t);
}

void ClockController::second_changed() {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_text_id, i, QTime::currentTime().toString().toLocal8Bit());

}
