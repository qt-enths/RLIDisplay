#include "infocontrollers.h"
#include "mainwindow.h"

#include "rlistrings.h"

#include <QTime>
#include <QDebug>
#include <QApplication>

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

static const QColor INFO_TEXT_STATIC_COLOR(0x00,0xFC,0xFC);
static const QColor INFO_TEXT_DYNAMIC_COLOR(0xFC,0xFC,0x54);

static const QColor INFO_TRANSPARENT_COLOR(0xFF,0xFF,0xFF,0x00);
static const QColor INFO_BORDER_COLOR(0x40,0xFC,0x00);
static const QColor INFO_BACKGRD_COLOR(0x00,0x00,0x00);


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

void InfoBlockController::setInfoTextStr(InfoText& t, char** str) {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(str[i]));
}

void InfoBlockController::setInfoTextBts(InfoText& t, QByteArray str) {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = str;
}

//------------------------------------------------------------------------------

ValueBarController::ValueBarController(char** name, const QPoint& left_top, int title_width, int def_val, QObject* parent) : InfoBlockController(parent) {
  _val_rect_id = -1;
  _val = def_val;
  _maxval = title_width * 12 + 2;
  _title_width = title_width;
  _name = name;
  _left_top = left_top;
}

void ValueBarController::initBlock(const QSize& size) {
  Q_UNUSED(size);

  _block->setGeometry(QRect(_left_top.x(), _left_top.y(), 12*_title_width + 60, 23));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(4+12*_title_width+2, 0, 2, 23);
  _block->addRect(r);

  r.rect = QRect(4+12*_title_width+2, 14, 0, 15);
  _val_rect_id = _block->addRect(r);

  InfoText t;
  t.font_tag = "12x14";
  setInfoTextStr(t, _name);
  t.rect = QRect(4, 6, 12*_title_width+2, 14);
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;
  _ttl_text_id = _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  t.rect = QRect(4+12*_title_width+4+2, 6, 12*4, 14);
  setInfoTextBts(t, QByteArray());
  _val_text_id = _block->addText(t);

  onValueChanged(_val);
}

void ValueBarController::onValueChanged(int val) {
  _val = val;

  if (_val_rect_id != -1) {
    if (_val >= 0)
      emit setRect(_val_rect_id, QRect(12*_title_width + 8, 4, val * 50 / _maxval, 15));
    else {
      emit setRect(_val_rect_id, QRect(12*_title_width + 8, 4, 0, 15));

      for (int i = 0; i < RLI_LANG_COUNT; i++)
        emit setText(_val_text_id, i, enc->fromUnicode(dec->toUnicode(RLIStrings::nOff[i])));
    }
  }
}

void ValueBarController::setMaxValue(int val)
{
    _maxval = val;
}

//------------------------------------------------------------------------------


LableController::LableController(char** text, const QRect& geom, QString font_tag, QObject* parent)
  : InfoBlockController(parent) {
  _text_id = -1;
  _font_tag = font_tag;
  _text = text;
  _geom = geom;
}

void LableController::onTextChanged(char** text) {
  Q_UNUSED(text);
}

void LableController::initBlock(const QSize& size) {
  int x, y;

  if (_geom.x() < 0)
    x = size.width() + _geom.x();
  else
    x = _geom.x();

  if (_geom.y() < 0)
    y = size.height() + _geom.y();
  else
    y = _geom.y();

  _block->setGeometry(QRect(x, y, _geom.width(), _geom.height()));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = _font_tag;
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(1, 1, _geom.width(), _geom.height());
  setInfoTextStr(t, _text);
  _text_id = _block->addText(t);
}


//------------------------------------------------------------------------------

ScaleController::ScaleController(QObject* parent) : InfoBlockController(parent) {
  _scl1_text_id = -1;
  _scl2_text_id = -1;
  _unit_text_id = -1;
}

void ScaleController::scale_changed(std::pair<QByteArray, QByteArray> scale) {
    emit setText(0, 0, scale.first);
    emit setText(2, 0, scale.second);
    emit setText(0, 1, scale.first);
    emit setText(2, 1, scale.second);
}

void ScaleController::initBlock(const QSize& size) {
  _block->setGeometry(QRect( size.width() - 19*12-4*2 - 5 - 5 - 236, 5, 236, 4+28+4));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  std::pair<QByteArray, QByteArray> s;
  MainWindow * mainWnd = dynamic_cast<MainWindow *>(parent());
  if(mainWnd)
      s = mainWnd->getRadarScale()->getCurScaleText();

  t.font_tag = "16x28";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(6, 5, 5*16, 28);
  if(s.first.size())
      setInfoTextBts(t, s.first);
  else
      setInfoTextBts(t, QByteArray("0.125"));
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_LEFT;

  t.rect = QRect(6+5*16, 5, 16, 28);
  setInfoTextBts(t, QByteArray("/"));
  _block->addText(t);

  t.font_tag = "14x14";

  t.rect = QRect(6+6*16, 5+12, 5*14, 14);
  if(s.second.size())
      setInfoTextBts(t, s.second);
  else
      setInfoTextBts(t, QByteArray("0.025"));
  _block->addText(t);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(6+6*16+5*14+2, 0, 2, 4+28+4);
  _block->addRect(r);

  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(6+6*16+5*14+2+2+2, 5+7, 4*14, 14);
  setInfoTextStr(t, RLIStrings::nNM);
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

  _block->setGeometry(QRect( size.width() - 236 - 5, 5, 236, 38));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 236, 14);
  setInfoTextStr(t, RLIStrings::nGiro);
  _block->addText(t);

  t.rect = QRect(4, 21, 236, 14);
  setInfoTextStr(t, RLIStrings::nMspd);
  _block->addText(t);


  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 4, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _crs_text_id = _block->addText(t);

  t.rect = QRect(180, 21, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _spd_text_id =_block->addText(t);


  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 4, 32, 14);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  t.rect = QRect(186, 21, 32, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);
}

//------------------------------------------------------------------------------

PositionController::PositionController(QObject* parent) : InfoBlockController(parent) {
  _lat_text_id = -1;
  _lon_text_id = -1;
}

void PositionController::pos_changed(QVector2D coords) {
  //qDebug() << coords;

  QByteArray lat = QString::number(coords.x()).left(7).toLatin1();
  QByteArray lon = QString::number(coords.y()).left(7).toLatin1();

  emit setText(_lat_text_id, RLI_LANG_ENGLISH, lat);
  emit setText(_lon_text_id, RLI_LANG_ENGLISH, lon);
  emit setText(_lat_text_id, RLI_LANG_RUSSIAN, lat);
  emit setText(_lon_text_id, RLI_LANG_RUSSIAN, lon);
}

void PositionController::initBlock(const QSize& size) {
  QSize font_size(12, 14);

  _block->setGeometry(QRect( size.width() - 236 - 5, 5+38+4, 236, 38));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 230, 14);
  setInfoTextStr(t, RLIStrings::nLat);
  _block->addText(t);

  t.rect = QRect(4, 21, 230, 14);
  setInfoTextStr(t, RLIStrings::nLon);
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(4, 4, 230, 14);
  setInfoTextStr(t, RLIStrings::nBlank);
  _lat_text_id = _block->addText(t);

  t.rect = QRect(4, 21, 230, 14);
  setInfoTextStr(t, RLIStrings::nBlank);
  _lon_text_id =_block->addText(t);
}

//------------------------------------------------------------------------------

BlankController::BlankController(QObject* parent) : InfoBlockController(parent) {
}

void BlankController::initBlock(const QSize& size) {
  QSize font_size(12, 14);

  _block->setGeometry(QRect( size.width() - 236 - 5, 5+38+4+38+4, 236, 38));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);
}

//------------------------------------------------------------------------------

DangerController::DangerController(QObject* parent) : InfoBlockController(parent) {
}

void DangerController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 236 - 5, size.height() - 350 - 5, 236, 20));
  _block->setBackColor(INFO_TEXT_DYNAMIC_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_BACKGRD_COLOR;
  t.rect = QRect(1, 1, 236, 20);
  setInfoTextStr(t, RLIStrings::nDng);
  //_block->addText(t);
}

//------------------------------------------------------------------------------

TailsController::TailsController(QObject* parent) : InfoBlockController(parent) {
  _min_text_id = -1;
}

void TailsController::tails_count_changed(int count) {
  Q_UNUSED(count);
}

void TailsController::initBlock(const QSize& size) {
  _block->setGeometry(QRect( size.width() - 236 - 5, size.height() - 323 - 5, 236, 21));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nTrl);
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nOff);
  _min_text_id =_block->addText(t);

  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

DangerDetailsController::DangerDetailsController(QObject* parent) : InfoBlockController(parent) {
  _dks_text_id = -1;
  _vks_text_id = -1;
}

void DangerDetailsController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 236 - 5, size.height() - 298 - 5, 236, 38));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nCPA);
  _block->addText(t);

  t.rect = QRect(4, 21, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nVks);
  _block->addText(t);


  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 4, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _dks_text_id = _block->addText(t);

  t.rect = QRect(180, 21, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _vks_text_id =_block->addText(t);


  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 4, 32, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  t.rect = QRect(186, 21, 32, 14);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

VectorController::VectorController(QObject* parent) : InfoBlockController(parent) {

}

void VectorController::initBlock(const QSize& size) {
  _block->setGeometry(QRect( size.width() - 236 - 5, size.height() - 256 - 5, 236, 21));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nVec);
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 4, 0, 14);
  setInfoTextBts(t, QByteArray("20"));
  _block->addText(t);

  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

TargetsController::TargetsController(QObject* parent) : InfoBlockController(parent) {
}

void TargetsController::initBlock(const QSize& size) {
  _block->setGeometry(QRect( size.width() - 236 - 5, size.height() - 229 - 5, 236, 167));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(0, 18, 236, 1);
  _block->addRect(r);

  r.rect = QRect(118, 0, 1, 18);
  _block->addRect(r);

  InfoText t;
  // Header
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 3, 0, 14);
  setInfoTextStr(t, RLIStrings::nTrg);
  _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(114, 3, 0, 14);
  setInfoTextBts(t, QByteArray("1"));
  _trg_text_id = _block->addText(t);

  t.rect = QRect(230, 3, 0, 14);
  setInfoTextBts(t, QByteArray("1"));
  _cnt_text_id = _block->addText(t);



  // Table
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 18+5, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nBear);
  _block->addText(t);

  t.rect = QRect(4, 18+5+18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nRng);
  _block->addText(t);

  t.rect = QRect(4, 18+5+2*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nCrsW);
  _block->addText(t);

  t.rect = QRect(4, 18+5+3*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nSpdW);
  _block->addText(t);

  t.rect = QRect(4, 18+5+4*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nTcpa);
  _block->addText(t);

  t.rect = QRect(4, 18+5+5*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nTtcpa);
  _block->addText(t);

  t.rect = QRect(4, 18+5+6*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nDcc);
  _block->addText(t);

  t.rect = QRect(4, 18+5+7*18, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nTcc);
  _block->addText(t);



  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 18+5+0*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+1*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+2*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+3*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+4*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+5*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+6*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  t.rect = QRect(180, 18+5+7*18, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);


  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 18+5+0*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  t.rect = QRect(186, 18+5+1*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  t.rect = QRect(186, 18+5+2*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  t.rect = QRect(186, 18+5+3*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nKts);
  _block->addText(t);

  t.rect = QRect(186, 18+5+4*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  t.rect = QRect(186, 18+5+5*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);

  t.rect = QRect(186, 18+5+6*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  t.rect = QRect(186, 18+5+7*18, 32, 14);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

CursorController::CursorController(QObject* parent) : InfoBlockController(parent) {
  _pel_text_id = -1;
  _dis_text_id = -1;
}

void CursorController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 236 - 5, size.height() - 64, 236, 60));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(0, 6, 224, 14);
  setInfoTextStr(t, RLIStrings::nMrk);
  _block->addText(t);

  t.rect = QRect(4, 24, 224-8, 14);
  t.allign = INFOTEXT_ALLIGN_LEFT;
  setInfoTextStr(t, RLIStrings::nBear);
  _block->addText(t);

  t.rect = QRect(4, 42, 224-8, 14);
  setInfoTextStr(t, RLIStrings::nRng);
  _block->addText(t);

  //
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(180, 24, 0, 14);
  setInfoTextBts(t, QByteArray("0"));
  _pel_text_id = _block->addText(t);

  t.rect = QRect(180, 42, 0, 14);
  _dis_text_id = _block->addText(t);

  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(186, 24, 0, 14);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  t.rect = QRect(186, 42, 0, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);
}

void CursorController::cursor_moved(float peleng, float distance, const char * dist_fmt) {
  QByteArray str[RLI_LANG_COUNT];

  if (_pel_text_id != -1)
    for (int i = 0; i < RLI_LANG_COUNT; i++)
      emit setText(_pel_text_id, i, QString::number(peleng, 'f', 2).left(5).toLocal8Bit());

  if (_dis_text_id != -1)
  {
    for (int i = 0; i < RLI_LANG_COUNT; i++)
    {
      if(dist_fmt)
      {
          QString s;
          s.sprintf(dist_fmt, distance);
          emit setText(_dis_text_id, i, s.toLocal8Bit());
      }
      else
          emit setText(_dis_text_id, i, QString::number(distance, 'f', 2).left(5).toLocal8Bit());
    }
  }
}

//------------------------------------------------------------------------------

VnController::VnController(QObject* parent) : InfoBlockController(parent) {
  _p_text_id     = -1;
  _cu_text_id    = -1;
  _board_ptr_id  = -1;
}

void VnController::display_brg(float brg, float crsangle)
{
  //QByteArray str;
  QString s;
  if (_p_text_id != -1)
  {
    for (int i = 0; i < RLI_LANG_COUNT; i++)
    {
        s.sprintf("%.1f", brg);
        //str = s.toStdString().c_str();

        emit setText(_p_text_id, i, s.toStdString().c_str());
    }
  }

  if (_cu_text_id != -1)
  {
    bool starboard = true;
    bool oncourse  = false;
    if(crsangle < 0)
    {
        crsangle *= -1;
        starboard = false; // portside
    }
    else if((crsangle == 0) || (crsangle == 180))
        oncourse = true;
    s.sprintf("%.1f", crsangle);
    //str = s.toStdString().c_str();
    for (int i = 0; i < RLI_LANG_COUNT; i++)
    {
        emit setText(_cu_text_id, i, s.toStdString().c_str());
        if(_board_ptr_id == -1)
            continue;
        QByteArray brdptr;
        if(oncourse)
            brdptr = " ";
        else if(starboard)
            brdptr = enc->fromUnicode(dec->toUnicode(RLIStrings::nGradRb[i]));
        else
            brdptr = enc->fromUnicode(dec->toUnicode(RLIStrings::nGradLb[i]));
        emit setText(_board_ptr_id, i, brdptr);
    }
  }
}

void VnController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(5, size.height() - 70, 140, 66));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(0, 6, 136, 14);
  setInfoTextStr(t, RLIStrings::nEbl);
  _block->addText(t);

  t.rect = QRect(4, 28, 132, 14);
  t.allign = INFOTEXT_ALLIGN_LEFT;
  setInfoTextStr(t, RLIStrings::nVN);
  _block->addText(t);

  t.rect = QRect(4, 46, 132, 14);
  setInfoTextStr(t, RLIStrings::nCu);
  _block->addText(t);

  //
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(4+2*12+5*12+6, 28, 0, 14);
  setInfoTextBts(t, QByteArray("0.0"));
  _p_text_id = _block->addText(t);

  t.rect = QRect(4+2*12+5*12+6, 46, 0, 14);
  setInfoTextBts(t, QByteArray("0.0"));
  _cu_text_id = _block->addText(t);

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4+2*12+5*12+2+2, 28, 0, 14);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  t.rect = QRect(4+2*12+5*12+2+2, 46, 0, 14);
  setInfoTextStr(t, RLIStrings::nGradLb);
  _board_ptr_id = _block->addText(t);
}

//------------------------------------------------------------------------------

VdController::VdController(QObject* parent) : InfoBlockController(parent) {
  _vd_text_id = -1;
}

void VdController::display_distance(float dist, const char * fmt)
{
    QString s;
    if(fmt)
        s.sprintf(fmt, dist);
    else
        s.sprintf("%5.1f", dist);
    emit setText(_vd_text_id, 0, s.toLocal8Bit());
    emit setText(_vd_text_id, 1, s.toLocal8Bit());
}

void VdController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 5 - 236 - 5 - 140, size.height() - 56, 140, 52));
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(0, 6, 136, 14);
  setInfoTextStr(t, RLIStrings::nVrm);
  _block->addText(t);

  //
  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(4+2*12+5*12+6, 28, 0, 14);
  setInfoTextBts(t, QByteArray("0.00"));
  _vd_text_id = _block->addText(t);

  t.font_tag = "8x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4+2*12+5*12+2+6, 29, 0, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);
}

//------------------------------------------------------------------------------

ClockController::ClockController(QObject* parent) : InfoBlockController(parent) {
  _text_id = -1;
}

void ClockController::initBlock(const QSize& size) {
  _block->setGeometry(QRect(size.width() - 224 - 10, 128, 224, 20));
  _block->setBackColor(INFO_TRANSPARENT_COLOR);

  InfoText t;

  //
  t.font_tag = "12x14";
  t.color = INFO_TEXT_STATIC_COLOR;
  t.rect = QRect(4, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nTmInfo);
  _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.rect = QRect(224-4, 4, 0, 14);
  setInfoTextBts(t, QTime::currentTime().toString().toLocal8Bit());
  _text_id = _block->addText(t);
}

void ClockController::second_changed() {
  if (_text_id == -1)
    return;

  for (int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_text_id, i, QTime::currentTime().toString().toLocal8Bit());
}
