#include "infocontrollers.h"
#include "../mainwindow.h"

#include "../common/rlistrings.h"
#include "targetdatasource.h"

#include <QTime>
#include <QDebug>
#include <QApplication>



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

void InfoBlockController::resize(const QSize& size, const QMap<QString, QString>& params) {
  if (_block == NULL)
    return;

  _block->clear();
  setupBlock(_block, size, params);
}

void InfoBlockController::onLanguageChanged(int lang_id) {
  Q_UNUSED(lang_id);
}

void InfoBlockController::setupBlock(InfoBlock* b, const QSize& screen_size, const QMap<QString, QString>& params) {
  _block = b;

  QPoint leftTop(params["x"].toInt(), params["y"].toInt());
  QSize size(params["width"].toInt(), params["height"].toInt());

  if (leftTop.x() < 0) leftTop.setX(leftTop.x() + screen_size.width() - size.width());
  if (leftTop.y() < 0) leftTop.setY(leftTop.y() + screen_size.height() - size.height());

  //if (leftTop.x() < 0) leftTop.setX(leftTop.x() + screen_size.width());
  //if (leftTop.y() < 0) leftTop.setY(leftTop.y() + screen_size.height());

  _block->setGeometry(QRect(leftTop, size));

  initBlock(params);
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

ValueBarController::ValueBarController(char** name, int max_val, QObject* parent) : InfoBlockController(parent) {
  _val_rect_id = -1;
  _val = 0;

  _maxval = max_val;
  _name = name;
}

void ValueBarController::initBlock(const QMap<QString, QString>& params) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  int border_width = _block->getBorderWidth();
  int block_width = _block->getGeometry().width();
  int block_height = _block->getGeometry().height();
  _bar_width = params["bar_width"].toInt();
  QString font1 = params["font"];

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(block_width - _bar_width - 2*border_width, 0, 1, block_height);
  _block->addRect(r);

  r.rect = QRect(block_width - _bar_width - border_width, 3, 0, block_height - 2*3);
  _val_rect_id = _block->addRect(r);

  InfoText t;
  t.font_tag = font1;
  setInfoTextStr(t, _name);
  t.rect = QRect(0, 0, block_width - _bar_width - border_width, block_height);
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_STATIC_COLOR;
  _ttl_text_id = _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  t.rect = QRect(block_width - _bar_width - 1, 0, _bar_width, block_height);
  setInfoTextBts(t, QByteArray());
  _val_text_id = _block->addText(t);

  onValueChanged(_val);
}

void ValueBarController::onValueChanged(int val) {
  _val = val;

  int border_width = _block->getBorderWidth();
  int block_width = _block->getGeometry().width();
  int block_height = _block->getGeometry().height();

  if (_val_rect_id != -1) {
    if (_val >= 0)
      emit setRect(_val_rect_id, QRect(block_width - _bar_width - border_width, 3, (_val*_bar_width) / _maxval, block_height - 2*3));
    else {
      emit setRect(_val_rect_id, QRect(block_width - _bar_width - border_width, 3, 0, block_height - 2*3));

      for (int i = 0; i < RLI_LANG_COUNT; i++)
        emit setText(_val_text_id, i, enc->fromUnicode(dec->toUnicode(RLIStrings::nOff[i])));
    }
  }
}

//------------------------------------------------------------------------------


LabelController::LabelController(char** text, QObject* parent)
  : InfoBlockController(parent) {
  _text_id = -1;
  _text = text;
}

void LabelController::onTextChanged(char** text) {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_text_id, i, enc->fromUnicode(dec->toUnicode(text[i])));
}

void LabelController::initBlock(const QMap<QString, QString>& params) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  QRect geom = _block->getGeometry();

  t.font_tag = params["font"];
  t.allign = INFOTEXT_ALLIGN_CENTER;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  t.rect = QRect(0, 0, geom.width(), geom.height());
  setInfoTextStr(t, _text);
  _text_id = _block->addText(t);
}


//------------------------------------------------------------------------------

ScaleController::ScaleController(QObject* parent) : InfoBlockController(parent) {
  _scl1_text_id = -1;
  _scl2_text_id = -1;
  _unit_text_id = -1;
}

void ScaleController::onScaleChanged(RadarScale scale) {
  std::pair<QByteArray, QByteArray> s = scale.getCurScaleText();

  emit setText(0, 0, s.first);
  emit setText(2, 0, s.second);
  emit setText(0, 1, s.first);
  emit setText(2, 1, s.second);
}

void ScaleController::initBlock(const QMap<QString, QString>& params) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  std::pair<QByteArray, QByteArray> s("0.125", "0.125");

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
  //Q_UNUSED(course);
  //printf("Slot: %s. Hdg: %f\n", __func__, course);
  if(_crs_text_id == -1)
    return;
  QString str;
  str.setNum(course, 'f', 1);
  for(int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_crs_text_id, i, str.toLocal8Bit());
}

void CourseController::speed_changed(float speed) {
  Q_UNUSED(speed);
}

void CourseController::initBlock(const QMap<QString, QString>& params) {
  QSize font_size(12, 14);

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

void PositionController::initBlock(const QMap<QString, QString>& params) {
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

void BlankController::initBlock(const QMap<QString, QString>& params) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);
}

//------------------------------------------------------------------------------

DangerController::DangerController(QObject* parent) : InfoBlockController(parent) {
}

void DangerController::initBlock(const QMap<QString, QString>& params) {
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
  _mode_text_id = -1;
  _min_text_id = -1;
  _minutes     = 0;
}

void TailsController::onTailsModeChanged(int mode, int minutes) {
  _minutes = minutes;

  if (mode == TargetDataSource::TAILMODE_OFF)
    _minutes = 0;

  QByteArray smode;
  QString mins;
  for (int i = 0; i < RLI_LANG_COUNT; i++) {
    smode = (mode == TargetDataSource::TAILMODE_DOTS) ? RLIStrings::nDot[i] : RLIStrings::nTrl[i];

    if (_minutes <= 0)
      mins.sprintf("%s", RLIStrings::nOff[i]);
    else
      mins.sprintf("%d", _minutes);

    _block->setText(_mode_text_id, i, enc->fromUnicode(dec->toUnicode(smode)));
    _block->setText(_min_text_id, i, enc->fromUnicode(dec->toUnicode(mins.toLocal8Bit())));
  }
}

void TailsController::initBlock(const QMap<QString, QString>& params) {
  char * minsarray[2];

  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  t.font_tag = "12x14";
  t.allign = INFOTEXT_ALLIGN_LEFT;
  t.color = INFO_TEXT_STATIC_COLOR;

  t.rect = QRect(4, 4, 0, 14);
  setInfoTextStr(t, RLIStrings::nTrl);
  _mode_text_id = _block->addText(t);

  t.allign = INFOTEXT_ALLIGN_RIGHT;
  t.color = INFO_TEXT_DYNAMIC_COLOR;

  QString mins;
  for (int i = 0; i < RLI_LANG_COUNT; i++)
  {
      if(_minutes <= 0)
          mins.sprintf("%s", RLIStrings::nOff[i]);
      else
          mins.sprintf("%d", _minutes);
      minsarray[i] = mins.toLocal8Bit().data();
  }
  t.rect = QRect(180, 4, 0, 14);
  setInfoTextStr(t, minsarray);
  //setInfoTextStr(t, RLIStrings::nOff);
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

void DangerDetailsController::initBlock(const QMap<QString, QString>& params) {
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

void VectorController::initBlock(const QMap<QString, QString>& params) {
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
  _trg_text_id = -1;
  _cnt_text_id = -1;

  _cog_text_id = -1;
  _sog_text_id = -1;

  _count = 0;
}

void TargetsController::onTargetCountChanged(int count) {
  if (_cnt_text_id == -1)
    return;

  _count = count;
  QByteArray cnt = QString::number(count).toLatin1();

  emit setText(_cnt_text_id, RLI_LANG_ENGLISH, cnt);
  emit setText(_cnt_text_id, RLI_LANG_RUSSIAN, cnt);
}

void TargetsController::updateTarget(const QString& tag, const RadarTarget& trgt) {
  if (_trg_text_id == -1)
    return;

  _target = trgt;
  QByteArray taga = tag.toLatin1();
  QByteArray coga = QString::number(trgt.CourseOverGround).left(6).toLatin1();
  QByteArray soga = QString::number(trgt.SpeedOverGround).left(6).toLatin1();

  emit setText(_trg_text_id, RLI_LANG_ENGLISH, taga);
  emit setText(_trg_text_id, RLI_LANG_RUSSIAN, taga);
  emit setText(_cog_text_id, RLI_LANG_ENGLISH, coga);
  emit setText(_cog_text_id, RLI_LANG_RUSSIAN, coga);
  emit setText(_sog_text_id, RLI_LANG_ENGLISH, soga);
  emit setText(_sog_text_id, RLI_LANG_RUSSIAN, soga);
}

void TargetsController::initBlock(const QMap<QString, QString>& params) {
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
  setInfoTextBts(t, QString::number(_count).toLatin1());
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
  setInfoTextBts(t, QString::number(_target.CourseOverGround).left(6).toLatin1());
  _cog_text_id = _block->addText(t);

  t.rect = QRect(180, 18+5+3*18, 0, 14);
  setInfoTextBts(t, QString::number(_target.SpeedOverGround).left(6).toLatin1());
  _sog_text_id =_block->addText(t);

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

void CursorController::initBlock(const QMap<QString, QString>& params) {
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

void VnController::initBlock(const QMap<QString, QString>& params) {
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

void VdController::initBlock(const QMap<QString, QString>& params) {
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

void ClockController::initBlock(const QMap<QString, QString>& params) {
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
