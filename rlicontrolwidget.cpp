#include "rlicontrolwidget.h"
#include "ui_rlicontrolwidget.h"

#include <QDebug>
#include <QKeyEvent>
#include <QApplication>

#include <stdint.h>
typedef uint32_t u_int32_t;


#define TRIGGERED_SLIDER_MIN -90
#define TRIGGERED_SLIDER_MAX 90
#define TRIGGERED_SLIDER_DEFAULT 0

RLIControlWidget::RLIControlWidget(QWidget *parent) : QWidget(parent), ui(new Ui::RLIControlWidget) {
  ui->setupUi(this);

  ui->sldGain->setMinimum(0);
  ui->sldGain->setMaximum(255);
  ui->sldGain->setValue(0);

  ui->sldWater->setMinimum(0);
  ui->sldWater->setMaximum(255);
  ui->sldWater->setValue(0);

  ui->sldRain->setMinimum(0);
  ui->sldRain->setMaximum(255);
  ui->sldRain->setValue(0);

  ui->sldVN->setMinimum(TRIGGERED_SLIDER_MIN);
  ui->sldVN->setMaximum(TRIGGERED_SLIDER_MAX);
  ui->sldVN->setValue(TRIGGERED_SLIDER_DEFAULT);

  ui->sldVD->setMinimum(TRIGGERED_SLIDER_MIN);
  ui->sldVD->setMaximum(TRIGGERED_SLIDER_MAX);
  ui->sldVD->setValue(TRIGGERED_SLIDER_DEFAULT);

  _vn_pos = TRIGGERED_SLIDER_DEFAULT;
  _vd_pos = TRIGGERED_SLIDER_DEFAULT;

  setFocusPolicy(Qt::NoFocus);
}

RLIControlWidget::~RLIControlWidget() {
  delete ui;
}

// TODO: send vn and vd changed event
void RLIControlWidget::on_sldVN_sliderReleased() {
  //qDebug() << "VN Diff: " << ui->sldVN->value() - TRIGGERED_SLIDER_DEFAULT;
  ui->sldVN->setValue(TRIGGERED_SLIDER_DEFAULT);
  _vn_pos = TRIGGERED_SLIDER_DEFAULT;
}

void RLIControlWidget::on_sldVD_sliderReleased() {
  //qDebug() << "VD Diff: " << ui->sldVD->value() - TRIGGERED_SLIDER_DEFAULT;
  ui->sldVD->setValue(TRIGGERED_SLIDER_DEFAULT);
  _vd_pos = TRIGGERED_SLIDER_DEFAULT;
}

void RLIControlWidget::on_sldVN_sliderMoved(int pos) {
  emit vnChanged(static_cast<float>(pos - _vn_pos));
  _vn_pos = pos;
}

void RLIControlWidget::on_sldVD_sliderMoved(int pos) {
  emit vdChanged(static_cast<float>(pos - _vd_pos));
  _vd_pos = pos;
}

void RLIControlWidget::on_btnModeDec_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Minus, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnModeInc_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Plus, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnMode3_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_sldGain_valueChanged(int value) {
  emit gainChanged(static_cast<u_int32_t>(value));
}

void RLIControlWidget::on_sldWater_valueChanged(int value) {
  emit waterChanged(value);
}

void RLIControlWidget::on_sldRain_valueChanged(int value) {
  emit rainChanged(value);
}

void RLIControlWidget::on_btnOnOff3_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Backslash, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnMenu_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_W, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnTrace1_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnTrace4_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnTrace5_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnTrace6_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnConfigMenu_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_U, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

void RLIControlWidget::on_btnClose_clicked() {
  emit closeApp();
}

void RLIControlWidget::on_btnMagnifier_clicked() {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, Qt::Key_L, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}
