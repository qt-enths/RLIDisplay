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

void RLIControlWidget::on_sldGain_valueChanged(int value) {
  emit gainChanged(static_cast<u_int32_t>(value));
}

void RLIControlWidget::on_sldWater_valueChanged(int value) {
  emit waterChanged(value);
}

void RLIControlWidget::on_sldRain_valueChanged(int value) {
  emit rainChanged(value);
}

void RLIControlWidget::on_btnClose_clicked() {
  emit closeApp();
}



void RLIControlWidget::postKeyEvent(int key) {
  QKeyEvent* e = new QKeyEvent(QEvent::KeyPress, key, Qt::NoModifier);
  qApp->postEvent(parent(), e);
}

/* ---- */
//Обзор
void RLIControlWidget::on_btnRLI1_clicked() {
  postKeyEvent(Qt::Key_X);
}

//Узкий / Шир.
void RLIControlWidget::on_btnRLI2_clicked() {
  postKeyEvent(Qt::Key_Greater);
}

//Накоп. Видео
void RLIControlWidget::on_btnRLI3_clicked() {
  postKeyEvent(Qt::Key_V);
}

//Подав. имп. Помех
void RLIControlWidget::on_btnRLI4_clicked() {
  postKeyEvent(Qt::Key_S);
}
/* ---- */

/* ---- */
//Сброс
void RLIControlWidget::on_btnTrace1_clicked() {
  postKeyEvent(Qt::Key_Escape);
}

//Следы точки
void RLIControlWidget::on_btnTrace2_clicked() {
  postKeyEvent(Qt::Key_T);
}

//Манёвр
void RLIControlWidget::on_btnTrace3_clicked() {
  postKeyEvent(Qt::Key_M);
}

//Выбор цели
void RLIControlWidget::on_btnTrace4_clicked() {
  postKeyEvent(Qt::Key_Up);
}

//ЛИД / ЛОД
void RLIControlWidget::on_btnTrace5_clicked() {
  postKeyEvent(Qt::Key_Down);
}

//Захват
void RLIControlWidget::on_btnTrace6_clicked() {
  postKeyEvent(Qt::Key_Enter);
}
/* ---- */

/* ---- */
//Меню
void RLIControlWidget::on_btnMenu_clicked() {
  postKeyEvent(Qt::Key_W);
}

//Скрытое меню
void RLIControlWidget::on_btnConfigMenu_clicked() {
  postKeyEvent(Qt::Key_U);
}

//Электронная лупа
void RLIControlWidget::on_btnMagnifier_clicked() {
  postKeyEvent(Qt::Key_L);
}
/* ---- */


/* ---- */
//Курс / Север / Курс стаб
void RLIControlWidget::on_btnMode1_clicked() {
  postKeyEvent(Qt::Key_H);
}

//ИД / ОД
void RLIControlWidget::on_btnMode2_clicked() {
  postKeyEvent(Qt::Key_R);
}

//Вынос центра
void RLIControlWidget::on_btnMode3_clicked() {
  postKeyEvent(Qt::Key_C);
}

//Шкала -
void RLIControlWidget::on_btnModeDec_clicked() {
  postKeyEvent(Qt::Key_Minus);
}

//Шкала +
void RLIControlWidget::on_btnModeInc_clicked() {
  postKeyEvent(Qt::Key_Plus);
}
/* ---- */

/* ---- */
void RLIControlWidget::on_btnOnOff1_clicked() {

}

//Карта (Маршрут)
void RLIControlWidget::on_btnOnOff2_clicked() {
  postKeyEvent(Qt::Key_A);
}

//Парал. Линии
void RLIControlWidget::on_btnOnOff3_clicked() {
  postKeyEvent(Qt::Key_Backslash);
}

//Откл. Звука
void RLIControlWidget::on_btnOnOff4_clicked() {
  postKeyEvent(Qt::Key_B);
}

//Откл. ОК
void RLIControlWidget::on_btnOnOff5_clicked() {
  postKeyEvent(Qt::Key_K);
}

//Вынос ВН/ВД
void RLIControlWidget::on_btnOnOff6_clicked() {
  postKeyEvent(Qt::Key_Slash);
}
/* ---- */



/*
+ Обзор                       X
+ Узкий / Шир.                >
+ Накоп. Видео                V
+ Под. имп. Помех             S
+ Сброс                       ESC
Сброс АС                    Q
+ Следы точки                 T
+ Манёвр                      M
+ ЛИД / ЛОД                   Стрелка вниз
+ Курс / Север / Курс стаб    H
+ ИД / ОД                     R
+ Вынос центра                C
НКД                         D
+ Карта (Маршрут)             A
+ Меню                        W
Выбор                       G
+ Скрытое меню                U
Стоп-кадр                   F
+ Шкала +                     +
+ Шкала -                     -
+ Откл. Звука                 B
+ Откл. ОК                    K
+ Вынос ВН/ВД                 /
+ Электронная лупа            L
+ Выбор цели                  Стрелка вверх
+ Захват                      ENTER
+ Парал. Линии                \
*/
