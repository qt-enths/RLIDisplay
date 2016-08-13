#include "infocontrollers.h"

#include "rlistrings.h"

#include <QTime>
#include <QDebug>

//#define cGROUND 20      // общий фон /*14*/{0x38,0x38,0x38}
//#define cRULER 47       // линейка   /*2F*/{0x40,0xFC,0x00}
//#define cINFORMATION 14 // данные    /*0E*/{0xFC,0xFC,0x54}
//#define cNOT 12         // отрицание /*0C*/{0xFC,0x54,0x54}
//#define cOCIFR 48       // оцифровка /*30*/{0x00,0xFC,0x00}

//#define cHEAD 52       // надписи	 /*34*/{0x00,0xFC,0xFC}
//#define cINFGROUND 0	 // фон вывода данных /*00*/{0x00,0x00,0x00}
//#define cBLINKTEXT 39  // цвет мигающего текста /*27*/{0xFC,0x00,0x40}
//#define cMSGERR 44	 // фон сообщения         /*2C*/{0xFC,0xFC,0x00}
//#define cBLANKITEM 28	 // пропускаемый пункт меню /*1C*/{0xB4,0xB4,0xB4}

const QColor INFO_TEXT_STATIC_COLOR(0x00,0xFC,0xFC);
const QColor INFO_TEXT_DYNAMIC_COLOR(0xFC,0xFC,0x54);

const QColor INFO_BORDER_COLOR(0x40,0xFC,0x00);
const QColor INFO_BACKGRD_COLOR(0x00,0x00,0x00);


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

  _block->setGeometry(QRect(_left_top.x(), _left_top.y(), 12*_title_width + 60, 23));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(4+12*_title_width+2, 0, 2, 23);
  _block->addRect(r);

  r.rect = QRect(4+12*_title_width+2, 14, 0, 15);
  _val_rect_id = _block->addRect(r);

  InfoText t;
  t.font_tag = "12x14";
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(_name[i]));

  t.rect = QRect(4, 6, 12*_title_width+2, 14);
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;
  _ttl_text_id = _block->addText(t);

  t.rect = QRect(4+12*_title_width+4+2, 6, 12*4, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray();
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


LabelController::LabelController(char** text, int width, const QPoint& anchor, bool anchor_left, QObject* parent)
  : InfoBlockController(parent) {
  _text_id = -1;
  _text = text;
  _anchor = anchor;
  _anchor_left = anchor_left;
  _width = width;
}

void LabelController::onTextChanged(char** text) {
  Q_UNUSED(text);
}

void LabelController::initBlock(const QSize& size) {
  QRect geometry;

  if (_anchor_left)
    geometry = QRect(_anchor.x(), _anchor.y(), _width, 4+14+4);
  else
    geometry = QRect(size.width() -_anchor.x()-_width, _anchor.y(), _width, 4+14+4);

  _block->setGeometry(geometry);
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(0, 5, _width, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(_text[i]));;
  _text_id = _block->addText(t);
}


//------------------------------------------------------------------------------

ScaleController::ScaleController(QObject* parent) : InfoBlockController(parent) {
  _scl1_text_id = -1;
  _scl2_text_id = -1;
  _unit_text_id = -1;
}

void ScaleController::scale_changed(std::pair<int, int> scale) {
  Q_UNUSED(scale);
}

void ScaleController::initBlock(const QSize& size) {
  _block->setGeometry(QRect( size.width() - 19*12-4*2 - 5 - 5 - 236, 5, 236, 4+28+4));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "16x28";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(6, 5, 5*16, 28);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray("0.125");
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_LEFT;

  t.rect = QRect(6+5*16, 5, 16, 28);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray("/");
  _block->addText(t);

  t.font_tag = "14x14";

  t.rect = QRect(6+6*16, 5+12, 5*14, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray("0.025");
  _block->addText(t);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(6+6*16+5*14+2, 0, 2, 4+28+4);
  _block->addRect(r);

  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(6+6*16+5*14+2+2+2, 5+7, 4*14, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nNM[i]));
  _block->addText(t);
}

//------------------------------------------------------------------------------

CourseController::CourseController(QObject* parent) : InfoBlockController(parent) {
  _crs_text_id = -1;
  _spd_text_id = -1;
}

void CourseController::course_changed(float course) {
  Q_UNUSED(course);
}

void CourseController::speed_changed(float speed) {
  Q_UNUSED(speed);
}

void CourseController::initBlock(const QSize& size) {
  QSize font_size(12, 14);

  _block->setGeometry(QRect( size.width() - 19*font_size.width()-4*2 - 5, 5
                           , 19*font_size.width()+4*2, 2*(6+font_size.height()) + 2));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 5, 224-8, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nGiro[i]));
  _block->addText(t);

  t.rect = QRect(4, 23, 224-8, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nMspd[i]));
  _block->addText(t);


  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 5, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray("0");
  _crs_text_id = _block->addText(t);

  t.rect = QRect(180, 23, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QByteArray("0");
  _spd_text_id =_block->addText(t);


  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 5, 32, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nGrad[i]));
  _block->addText(t);

  t.rect = QRect(186, 23, 32, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nNM[i]));
  _block->addText(t);
}

//------------------------------------------------------------------------------

PositionController::PositionController(QObject* parent) : InfoBlockController(parent) {
  _lat_text_id = -1;
  _lon_text_id = -1;
}

void PositionController::pos_changed(std::pair<float, float> pos) {
  Q_UNUSED(pos);
}

void PositionController::initBlock(const QSize& size) {
  QSize font_size(12, 14);

  _block->setGeometry(QRect( size.width() - 19*font_size.width()-4*2 - 5, 5 + 2*(6+font_size.height()) + 2 + 4
                           , 19*font_size.width()+4*2, 2*(6+font_size.height()) + 2));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 5, 224-8, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nLat[i]));
  _block->addText(t);

  t.rect = QRect(4, 23, 224-8, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nLon[i]));
  _block->addText(t);
}

//------------------------------------------------------------------------------

BlankController::BlankController(QObject* parent) : InfoBlockController(parent) {
}

void BlankController::initBlock(const QSize& size) {
  QSize font_size(12, 14);

  _block->setGeometry(QRect( size.width() - 19*font_size.width()-4*2 - 5, 5 + 2*(2*(6+font_size.height()) + 2 + 4)
                           , 19*font_size.width()+4*2, 2*(6+font_size.height()) + 2));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);
}

//------------------------------------------------------------------------------

CursorController::CursorController(QObject* parent) : InfoBlockController(parent) {
  _pel_text_id = -1;
  _dis_text_id = -1;
}

void CursorController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 224 - 5, size.height() - 64 - 5, 224, 64));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(2, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(0, 6, 224, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nMrk[i]));
  _block->addText(t);

  t.rect = QRect(4, 26, 224-8, 14);
  t.allign = INFOTEXT_ALLIGN_LEFT;
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nBear[i]));
  _block->addText(t);

  t.rect = QRect(4, 46, 224-8, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nRng[i]));
  _block->addText(t);

  //
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 26, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode("0"));
  _pel_text_id = _block->addText(t);

  t.rect = QRect(180, 46, 0, 14);
  _dis_text_id = _block->addText(t);

  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 26, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nGrad[i]));
  _block->addText(t);

  t.rect = QRect(186, 46, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nNM[i]));
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
  _block->setGeometry(QRect(size.width() - 224 - 10, 144 + 10, 224, 20));
  _block->setBackColor(INFO_BACKGRD_COLOR);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.color = INFO_TEXT_STATIC_COLOR;
  t.rect = QRect(4, 4, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(RLIStrings::nTime[i]));
  _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.rect = QRect(224-4, 4, 0, 14);
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = QTime::currentTime().toString().toLocal8Bit();
  _text_id = _block->addText(t);
}

void ClockController::second_changed() {
  if (_text_id == -1)
    return;

  for (int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_text_id, i, QTime::currentTime().toString().toLocal8Bit());
}
