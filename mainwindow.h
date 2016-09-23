#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QSocketNotifier>

#include "chartmanager.h"
#include "radardatasource.h"
#include "infocontrollers.h"
#include "radarscale.h"

namespace Ui {
  class MainWindow;
}

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = 0);
  ~MainWindow();

#ifndef Q_OS_WIN
  static void intSignalHandler(int unused);

  RadarScale * _radar_scale;

protected:
  int findPressedKey(int key);
  int savePressedKey(int key);
  int countPressedKeys(void);
  void keyReleaseEvent(QKeyEvent *event);
  void keyPressEvent(QKeyEvent * event);

  QFuture<void> evdevThread;
  int evdevFd;
  int stopEvdev;
  int setupEvdev(char *evdevfn);
  void evdevHandleThread();

public slots:
  void handleSigInt();
#endif // !Q_OS_WIN
  void gain_slot(int value); // Used only for simulated Control Panel Unit. Must be removed at finish build

signals:
  void scale_changed(std::pair<QByteArray, QByteArray> scale);
  void gain_changed(int value);
  void rain_changed(int value);
  void wave_changed(int value);

private slots:
  void resizeEvent(QResizeEvent* e);
  void timerEvent(QTimerEvent* e);

  void onRLIWidgetInitialized();

  void on_btnClose_clicked();

public slots:
  void on_mnuAnalogZeroChanged(int val);

private:
  void setupInfoBlock(InfoBlockController* ctrl);

  ValueBarController* _gain_ctrl;
  ValueBarController* _water_ctrl;
  ValueBarController* _rain_ctrl;
  ValueBarController* _apch_ctrl;
  ValueBarController* _rdtn_ctrl;

  CursorController* _curs_ctrl;
  ClockController* _clck_ctrl;

  ScaleController* _scle_ctrl;
  CourseController* _crse_ctrl;
  DangerController* _dngr_ctrl;

  LableController* _lbl1_ctrl;
  LableController* _lbl2_ctrl;
  LableController* _lbl3_ctrl;
  LableController* _lbl4_ctrl;

  PositionController* _pstn_ctrl;
  BlankController* _blnk_ctrl;
  TailsController* _tals_ctrl;
  DangerDetailsController* _dgdt_ctrl;
  VectorController* _vctr_ctrl;
  TargetsController* _trgs_ctrl;

  VnController* _vn_ctrl;
  VdController* _vd_ctrl;

  ChartManager* _chart_mngr;
  RadarDataSource* _radar_ds;

  Ui::MainWindow *ui;

  int pressedKey[4];

#ifndef Q_OS_WIN
  static int sigintFd[2];
  QSocketNotifier *snInt;
#endif // !Q_OS_WIN
};

#endif // MAINWINDOW_H
