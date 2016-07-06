#include "rlicontrolwidget.h"
#include "ui_rlicontrolwidget.h"

#include <QDebug>
#include <QApplication>

#include "rlicontrolevent.h"

#define TRIGGERED_SLIDER_MIN -90
#define TRIGGERED_SLIDER_MAX 90
#define TRIGGERED_SLIDER_DEFAULT 0

RLIControlWidget::RLIControlWidget(QWidget *parent) : QWidget(parent), ui(new Ui::RLIControlWidget) {
  ui->setupUi(this);

  ui->sldGain->setMinimum(0);
  ui->sldGain->setMaximum(40);
  ui->sldGain->setValue(0);

  ui->sldVN->setMinimum(TRIGGERED_SLIDER_MIN);
  ui->sldVN->setMaximum(TRIGGERED_SLIDER_MAX);
  ui->sldVN->setValue(TRIGGERED_SLIDER_DEFAULT);

  ui->sldVD->setMinimum(TRIGGERED_SLIDER_MIN);
  ui->sldVD->setMaximum(TRIGGERED_SLIDER_MAX);
  ui->sldVD->setValue(TRIGGERED_SLIDER_DEFAULT);

  _vn_pos = TRIGGERED_SLIDER_DEFAULT;
  _vd_pos = TRIGGERED_SLIDER_DEFAULT;

  _reciever = NULL;
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
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::NoButton
                                           , RLIControlEvent::VN
                                           , pos - _vn_pos);
    qApp->postEvent(_reciever, e);
    _vn_pos = pos;
  }
}

void RLIControlWidget::on_sldVD_sliderMoved(int pos) {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::NoButton
                                           , RLIControlEvent::VD
                                           , pos - _vd_pos);
    qApp->postEvent(_reciever, e);
    _vd_pos = pos;
  }
}

void RLIControlWidget::on_btnModeDec_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ButtonMinus);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnModeInc_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ButtonPlus);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnMode3_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::CenterShift);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_sldGain_valueChanged(int value) {
  emit gainChanged(value);
}

void RLIControlWidget::on_btnOnOff3_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ParallelLines);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnMenu_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Menu);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnTrace4_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Up);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnTrace5_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Down);
    qApp->postEvent(_reciever, e);
  }
}

void RLIControlWidget::on_btnTrace6_clicked() {
  if (_reciever != NULL) {
    RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Enter);
    qApp->postEvent(_reciever, e);
  }
}
