#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QTimerEvent>
#include <QSocketNotifier>

#include "rliconfig.h"

#include "s52/chartmanager.h"
#include "datasources/boardpultcontroller.h"
#include "datasources/targetdatasource.h"
#include "datasources/radardatasource.h"
#include "datasources/shipdatasource.h"
#include "datasources/infocontrollers.h"
#include "datasources//radarscale.h"
#include "datasources/nmeaprocessor.h"

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
  static void intSignalHandler(int sig);
#endif // !Q_OS_WIN

protected:
  int findPressedKey(int key);
  int savePressedKey(int key);
  int countPressedKeys(void);

  void keyReleaseEvent(QKeyEvent *event);
  void keyPressEvent(QKeyEvent * event);

public slots:
#ifndef Q_OS_WIN
  void handleSigInt();
#endif // !Q_OS_WIN

private slots:
  void resizeEvent(QResizeEvent* e);
  void timerEvent(QTimerEvent* e);

  void onClose();
  void onRLIWidgetInitialized();

private:
  void setupInfoBlock(InfoBlockController* ctrl);

  RLIConfig* config;

  // Контроллеры инфоблоков
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

  LabelController* _lbl1_ctrl;
  LabelController* _lbl2_ctrl;
  LabelController* _lbl3_ctrl;
  LabelController* _lbl4_ctrl;
  LabelController* _lbl5_ctrl;
  LabelController* _band_lbl_ctrl;

  PositionController* _pstn_ctrl;
  BlankController* _blnk_ctrl;
  TailsController* _tals_ctrl;
  DangerDetailsController* _dgdt_ctrl;
  VectorController* _vctr_ctrl;
  TargetsController* _trgs_ctrl;

  VnController* _vn_ctrl;
  VdController* _vd_ctrl;

  // Контроллер пульта управления (для обработки вращения энкодеров)
  BoardPultController* _pult_driver;

  // Источники данных
  ChartManager* _chart_mngr;
  TargetDataSource* _target_ds;
  RadarDataSource* _radar_ds;
  ShipDataSource* _ship_ds;

  Ui::MainWindow *ui;

  int pressedKey[4];
  QString _nmeaImitfn;
  NMEAProcessor* _nmeaprc;
  QString _nmeaPort;

#ifndef Q_OS_WIN
  static int sigintFd[2];
  QSocketNotifier* snInt;
#endif // !Q_OS_WIN
};

#endif // MAINWINDOW_H
