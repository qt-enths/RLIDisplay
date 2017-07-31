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

void InfoBlockController::resize(const QSize& size, const RLIPanelInfo& panelInfo) {
  if (_block == NULL)
    return;

  _block->clear();
  setupBlock(_block, size, panelInfo);
}

void InfoBlockController::onLanguageChanged(int lang_id) {
  Q_UNUSED(lang_id);
}

void InfoBlockController::setupBlock(InfoBlock* b, const QSize& screen_size, const RLIPanelInfo& panelInfo) {
  _block = b;

  QPoint leftTop(panelInfo.params["x"].toInt(), panelInfo.params["y"].toInt());
  QSize size(panelInfo.params["width"].toInt(), panelInfo.params["height"].toInt());

  if (leftTop.x() < 0) leftTop.setX(leftTop.x() + screen_size.width() - size.width());
  if (leftTop.y() < 0) leftTop.setY(leftTop.y() + screen_size.height() - size.height());

  //if (leftTop.x() < 0) leftTop.setX(leftTop.x() + screen_size.width());
  //if (leftTop.y() < 0) leftTop.setY(leftTop.y() + screen_size.height());

  _block->setGeometry(QRect(leftTop, size));

  initBlock(panelInfo);
}

void InfoBlockController::setInfoTextParams(InfoText& t, QMap<QString, QString> params) {
  t.font_tag = params["font"];
  t.allign = allignFromString(params["allign"]);
  t.rect = rectFromString(params["rect"]);
}

void InfoBlockController::setInfoTextParams(InfoText& t, const RLIPanelTableInfo& tblInfo, int row, int col) {
  int top = tblInfo.params["top"].toInt();
  int height = tblInfo.params["row_height"].toInt();
  int left = tblInfo.columns[col]["left"].toInt();
  int width = tblInfo.columns[col]["width"].toInt();

  t.font_tag = tblInfo.columns[col]["font"];
  t.allign = allignFromString(tblInfo.columns[col]["allign"]);
  t.rect = QRect(left, top+row*height, width, height);
}

void InfoBlockController::setInfoTextStr(InfoText& t, char** str) {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = enc->fromUnicode(dec->toUnicode(str[i]));
}

void InfoBlockController::setInfoTextBts(InfoText& t, QByteArray str) {
  for (int i = 0; i < RLI_LANG_COUNT; i++)
    t.str[i] = str;
}

TextAllign InfoBlockController::allignFromString(const QString& s) {
  if (s == "center")
    return INFOTEXT_ALLIGN_CENTER;

  if (s == "left")
    return INFOTEXT_ALLIGN_LEFT;

  if (s == "right")
    return INFOTEXT_ALLIGN_RIGHT;

  return INFOTEXT_ALLIGN_CENTER;
}

QRect InfoBlockController::rectFromString(const QString& s) {
  QStringList l = s.split("x");
  return QRect(l[0].toInt(), l[1].toInt(), l[2].toInt(), l[3].toInt());
}

//------------------------------------------------------------------------------

ValueBarController::ValueBarController(char** name, int max_val, QObject* parent) : InfoBlockController(parent) {
  _val_rect_id = -1;
  _val = 0;

  _maxval = max_val;
  _name = name;
}

void ValueBarController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  int border_width = _block->getBorderWidth();
  int block_width = _block->getGeometry().width();
  int block_height = _block->getGeometry().height();
  _bar_width = panelInfo.params["bar_width"].toInt();
  QString font = panelInfo.params["font"];

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = QRect(block_width - _bar_width - 2*border_width, 0, 1, block_height);
  _block->addRect(r);

  r.rect = QRect(block_width - _bar_width - border_width, 3, 0, block_height - 2*3);
  _val_rect_id = _block->addRect(r);

  InfoText t;
  t.font_tag = font;
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

void LabelController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  QRect geom = _block->getGeometry();

  t.font_tag = panelInfo.params["font"];
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

void ScaleController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["scale1"]);
  setInfoTextBts(t, QByteArray("0.125"));
  _block->addText(t);

  setInfoTextParams(t, panelInfo.texts["slash"]);
  setInfoTextBts(t, QByteArray("/"));
  _block->addText(t);

  setInfoTextParams(t, panelInfo.texts["scale2"]);
  setInfoTextBts(t, QByteArray("0.025"));
  _block->addText(t);

  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = rectFromString(panelInfo.rects["splitter"]["rect"]);
  _block->addRect(r);

  t.color = INFO_TEXT_STATIC_COLOR;

  setInfoTextParams(t, panelInfo.texts["units"]);
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

void CourseController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nGiro);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nMspd);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("0"));
  _crs_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextBts(t, QByteArray("0"));
  _spd_text_id =_block->addText(t);


  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 2);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);
}

//------------------------------------------------------------------------------

PositionController::PositionController(QObject* parent) : InfoBlockController(parent) {
  _lat_text_id = -1;
  _lon_text_id = -1;
}

void PositionController::pos_changed(QVector2D coords) {
  QByteArray lat = QString::number(coords.x()).left(7).toLatin1();
  QByteArray lon = QString::number(coords.y()).left(7).toLatin1();

  emit setText(_lat_text_id, RLI_LANG_ENGLISH, lat);
  emit setText(_lon_text_id, RLI_LANG_ENGLISH, lon);
  emit setText(_lat_text_id, RLI_LANG_RUSSIAN, lat);
  emit setText(_lon_text_id, RLI_LANG_RUSSIAN, lon);
}

void PositionController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nLat);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nLon);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextStr(t, RLIStrings::nBlank);
  _lat_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextStr(t, RLIStrings::nBlank);
  _lon_text_id =_block->addText(t);
}

//------------------------------------------------------------------------------

BlankController::BlankController(QObject* parent) : InfoBlockController(parent) {
}

void BlankController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  Q_UNUSED(panelInfo);
}

//------------------------------------------------------------------------------

DangerController::DangerController(QObject* parent) : InfoBlockController(parent) {
}

void DangerController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_TEXT_DYNAMIC_COLOR);

  InfoText t;

  t.color = INFO_BACKGRD_COLOR;
  setInfoTextParams(t, panelInfo.texts["label"]);
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

void TailsController::initBlock(const RLIPanelInfo& panelInfo) {
  char * minsarray[2];

  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nTrl);
  _mode_text_id = _block->addText(t);

  QString mins;
  for (int i = 0; i < RLI_LANG_COUNT; i++) {
    if(_minutes <= 0)
       mins.sprintf("%s", RLIStrings::nOff[i]);
    else
       mins.sprintf("%d", _minutes);

    minsarray[i] = mins.toLocal8Bit().data();
  }

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextStr(t, minsarray);
  _min_text_id =_block->addText(t);

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

DangerDetailsController::DangerDetailsController(QObject* parent) : InfoBlockController(parent) {
  _dks_text_id = -1;
  _vks_text_id = -1;
}

void DangerDetailsController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  InfoText t;

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nCPA);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nVks);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("0"));
  _dks_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextBts(t, QByteArray("0"));
  _vks_text_id =_block->addText(t);


  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 2);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

VectorController::VectorController(QObject* parent) : InfoBlockController(parent) {

}

void VectorController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  InfoText t;

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nVec);
  _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("20"));
  _block->addText(t);

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
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

void TargetsController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  // Header
  InfoRect r;
  r.col = INFO_BORDER_COLOR;
  r.rect = rectFromString(panelInfo.rects["header-bottom"]["rect"]);
  _block->addRect(r);

  r.rect = rectFromString(panelInfo.rects["header-middle"]["rect"]);
  _block->addRect(r);

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["header"]);
  setInfoTextStr(t, RLIStrings::nTrg);
  _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["current-target"]);
  setInfoTextBts(t, QByteArray("1"));
  _trg_text_id = _block->addText(t);

  setInfoTextParams(t, panelInfo.texts["target-count"]);
  setInfoTextBts(t, QString::number(_count).toLatin1());
  _cnt_text_id = _block->addText(t);


  // Table
  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nBear);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nRng);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 2, 0);
  setInfoTextStr(t, RLIStrings::nCrsW);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 3, 0);
  setInfoTextStr(t, RLIStrings::nSpdW);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 4, 0);
  setInfoTextStr(t, RLIStrings::nTcpa);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 5, 0);
  setInfoTextStr(t, RLIStrings::nTtcpa);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 6, 0);
  setInfoTextStr(t, RLIStrings::nDcc);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 7, 0);
  setInfoTextStr(t, RLIStrings::nTcc);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 2, 1);
  setInfoTextBts(t, QString::number(_target.CourseOverGround).left(6).toLatin1());
  _cog_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 3, 1);
  setInfoTextBts(t, QString::number(_target.SpeedOverGround).left(6).toLatin1());
  _sog_text_id =_block->addText(t);

  setInfoTextParams(t, tblInfo, 4, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 5, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 6, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 7, 1);
  setInfoTextBts(t, QByteArray("0"));
  _block->addText(t);


  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 2);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 2, 2);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 3, 2);
  setInfoTextStr(t, RLIStrings::nKts);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 4, 2);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 5, 2);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 6, 2);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 7, 2);
  setInfoTextStr(t, RLIStrings::nMin);
  _block->addText(t);
}

//------------------------------------------------------------------------------

CursorController::CursorController(QObject* parent) : InfoBlockController(parent) {
  _pel_text_id = -1;
  _dis_text_id = -1;
}

void CursorController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["header"]);
  setInfoTextStr(t, RLIStrings::nMrk);
  _block->addText(t);


  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nBear);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nRng);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("0"));
  _pel_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextBts(t, QByteArray("0"));
  _dis_text_id = _block->addText(t);


  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 2);
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

void VnController::display_brg(float brg, float crsangle) {
  //QByteArray str;
  QString s;
  if (_p_text_id != -1) {
    for (int i = 0; i < RLI_LANG_COUNT; i++) {
      s.sprintf("%.1f", brg);
      //str = s.toStdString().c_str();
      emit setText(_p_text_id, i, s.toStdString().c_str());
    }
  }

  if (_cu_text_id != -1) {
    bool starboard = true;
    bool oncourse  = false;

    if (crsangle < 0) {
      crsangle *= -1;
      starboard = false; // portside
    } else if((crsangle == 0) || (crsangle == 180))
      oncourse = true;

    s.sprintf("%.1f", crsangle);
    //str = s.toStdString().c_str();
    for (int i = 0; i < RLI_LANG_COUNT; i++) {
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

void VnController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["header"]);
  setInfoTextStr(t, RLIStrings::nEbl);
  _block->addText(t);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 0);
  setInfoTextStr(t, RLIStrings::nVN);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 0);
  setInfoTextStr(t, RLIStrings::nCu);
  _block->addText(t);


  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 1);
  setInfoTextBts(t, QByteArray("0.0"));
  _p_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 1);
  setInfoTextBts(t, QByteArray("0.0"));
  _cu_text_id = _block->addText(t);


  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, tblInfo, 0, 2);
  setInfoTextStr(t, RLIStrings::nGrad);
  _block->addText(t);

  setInfoTextParams(t, tblInfo, 1, 2);
  setInfoTextStr(t, RLIStrings::nGradLb);
  _board_ptr_id = _block->addText(t);
}

//------------------------------------------------------------------------------

VdController::VdController(QObject* parent) : InfoBlockController(parent) {
  _vd_text_id = -1;
}

void VdController::display_distance(float dist, const char * fmt) {
    QString s;
    if (fmt)
      s.sprintf(fmt, dist);
    else
      s.sprintf("%5.1f", dist);

    emit setText(_vd_text_id, 0, s.toLocal8Bit());
    emit setText(_vd_text_id, 1, s.toLocal8Bit());
}

void VdController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_BACKGRD_COLOR);
  _block->setBorder(1, INFO_BORDER_COLOR);

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["header"]);
  setInfoTextStr(t, RLIStrings::nVrm);
  _block->addText(t);

  RLIPanelTableInfo tblInfo = panelInfo.tables["table"];

  setInfoTextParams(t, tblInfo, 0, 0);
  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextBts(t, QByteArray("0.00"));
  _vd_text_id = _block->addText(t);

  setInfoTextParams(t, tblInfo, 0, 1);
  t.color = INFO_TEXT_STATIC_COLOR;
  t.rect = QRect(4+2*12+5*12+2+6, 29, 0, 14);
  setInfoTextStr(t, RLIStrings::nNM);
  _block->addText(t);
}

//------------------------------------------------------------------------------

ClockController::ClockController(QObject* parent) : InfoBlockController(parent) {
  _text_id = -1;
}

void ClockController::initBlock(const RLIPanelInfo& panelInfo) {
  _block->setBackColor(INFO_TRANSPARENT_COLOR);

  InfoText t;
  t.color = INFO_TEXT_STATIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["label"]);
  setInfoTextStr(t, RLIStrings::nTmInfo);
  _block->addText(t);

  t.color = INFO_TEXT_DYNAMIC_COLOR;
  setInfoTextParams(t, panelInfo.texts["time"]);
  setInfoTextBts(t, QTime::currentTime().toString().toLocal8Bit());
  _text_id = _block->addText(t);
}

void ClockController::second_changed() {
  if (_text_id == -1)
    return;

  for (int i = 0; i < RLI_LANG_COUNT; i++)
    emit setText(_text_id, i, QTime::currentTime().toString().toLocal8Bit());
}
