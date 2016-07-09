#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QTimerEvent>

#include "radardatasource.h"
#include "infocontrollers.h"

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

private slots:
  void resizeEvent(QResizeEvent* e);
  void timerEvent(QTimerEvent* e);

  void onRLIWidgetInitialized();

  void on_btnClose_clicked();

private:
  void setupInfoBlock(InfoBlockController* ctrl);

  ValueBarController* _gain_ctrl;
  ValueBarController* _water_ctrl;
  ValueBarController* _rain_ctrl;
  ValueBarController* _apch_ctrl;
  ValueBarController* _radiation_ctrl;

  CursorController* _curs_ctrl;
  ClockController* _clck_ctrl;

  RadarDataSource* _radar_ds;

  Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
