#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>

#include "common/rlistrings.h"


#ifndef Q_OS_WIN

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>

int MainWindow::sigintFd[2];


static int setup_unix_signal_handlers() {
  struct sigaction intaction;

  intaction.sa_handler = MainWindow::intSignalHandler;
  sigemptyset(&intaction.sa_mask);
  intaction.sa_flags |= SA_RESTART;

  if(sigaction(SIGINT, &intaction, 0) != 0)
    return 2;

  if(sigaction(SIGTERM, &intaction, 0) != 0)
    return 3;

  return 0;
}

#endif // !Q_OS_WIN

void MainWindow::onClose() {
  close();
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction start";

  ui->setupUi(this);



  config = new RLIConfig("config.xml");

  /*
  for (QString size : config.getAvailableScreenSizes()) {
    qDebug() << size;
    const RLILayout* layout = config.getLayoutForSize(size);
    layout->print();
  }
  */



  connect(ui->wgtRLIControl, SIGNAL(closeApp()), SLOT(onClose()));
  memset(pressedKey, 0, sizeof(pressedKey));

#ifndef Q_OS_WIN
  // Initializing SIGINT handling
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd) == 0)
  {
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
    if(setup_unix_signal_handlers() != 0)
      qDebug() << "Failed to setup SIGINT handler. Ctrl+C will terminate the program without cleaning.";
  }
  else
    qDebug() << "Couldn't create INT socketpair. Ctrl+C will terminate the program without cleaning.";
#endif // !Q_OS_WIN

  _pult_driver = new BoardPultController(this);
  _target_ds = new TargetDataSource();
  _radar_ds = new RadarDataSource();
  _ship_ds = new ShipDataSource();
  _chart_mngr = new ChartManager();

  ui->wgtRLIDisplay->setChartManager(_chart_mngr);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init start";

  QStringList args = qApp->arguments();
  QRegExp     rx("--ampoff=[+|-]?[0-9]+$");
  int argpos = args.indexOf(rx);
  if(argpos >= 0) {
    rx.setPattern("[+|-]?[0-9]+$");
    int offpos = rx.indexIn(args.at(argpos));
    if(offpos >= 0)
        _radar_ds->setAmpsOffset(args.at(argpos).mid(offpos).toInt());
  }

  rx.setPattern("--radar-device");
  argpos = args.indexOf(rx);

  if ((argpos >= 0) && (argpos < args.count() - 1))
    _radar_ds->start(args.at(argpos + 1).toStdString().c_str());
  else {
    rx.setPattern("--use-dump");
    argpos = args.indexOf(rx);
    if(argpos >= 0)
      _radar_ds->start_dump();
    else
      _radar_ds->start();
  }

  rx.setPattern("--nmea-port");
  argpos = args.indexOf(rx);

  if((argpos >= 0) && (argpos < args.count() - 1))
      _nmeaPort = args.at(argpos + 1);

  rx.setPattern("--nmea-imit-file");
  argpos = args.indexOf(rx);
  if ((argpos >= 0) && (argpos < args.count() - 1))
     _nmeaImitfn = args.at(argpos + 1);

  _nmeaprc = new NMEAProcessor(this);


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init finish";

  _gain_ctrl = new ValueBarController(RLIStrings::nGain, QPoint(5, 5), 8, 0, this);
  _gain_ctrl->setMaxValue(255);
  connect(ui->wgtRLIControl, SIGNAL(gainChanged(int)), _gain_ctrl, SLOT(onValueChanged(int)));

  _water_ctrl = new ValueBarController(RLIStrings::nWave, QPoint(5, 5+23+4), 8, 0, this);
  _water_ctrl->setMaxValue(255);
  connect(ui->wgtRLIControl, SIGNAL(waterChanged(int)), _water_ctrl, SLOT(onValueChanged(int)));

  _rain_ctrl = new ValueBarController(RLIStrings::nRain, QPoint(5, 5+2*(23+4)), 8, 0, this);
  _rain_ctrl->setMaxValue(255);
  connect(ui->wgtRLIControl, SIGNAL(rainChanged(int)), _rain_ctrl, SLOT(onValueChanged(int)));

  _apch_ctrl = new ValueBarController(RLIStrings::nAfc, QPoint(5, 5+3*(23+4)), 8, 0, this);
  _lbl5_ctrl = new LabelController(RLIStrings::nPP12p, QRect(5, 5+4*(23+4), 104, 23), "12x14", this);
  _band_lbl_ctrl = new LabelController(RLIStrings::nBandS, QRect(5, 5+5*(23+4), 104, 23), "12x14", this);
  _rdtn_ctrl = new ValueBarController(RLIStrings::nEmsn, QPoint(5+12*8+60+5, 5), 9, -1, this);

  _curs_ctrl = new CursorController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(cursor_moved(float,float, const char *)), _curs_ctrl, SLOT(cursor_moved(float,float, const char *)));

  _clck_ctrl = new ClockController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(per_second()), _clck_ctrl, SLOT(second_changed()));

  _scle_ctrl = new ScaleController(this);

  _lbl1_ctrl = new LabelController(RLIStrings::nNord, QRect(-246-120, 45, 120, 21), "12x14", this);
  _lbl2_ctrl = new LabelController(RLIStrings::nRm, QRect(-246-120, 70, 120, 21), "12x14", this);
  _lbl3_ctrl = new LabelController(RLIStrings::nWstab, QRect(-246-120, 95, 120, 21), "12x14", this);
  _lbl4_ctrl = new LabelController(RLIStrings::nLod, QRect(-246-96, -100, 96, 33), "16x28", this);

  _crse_ctrl = new CourseController(this);
  _pstn_ctrl = new PositionController(this);
  _blnk_ctrl = new BlankController(this);
  _dngr_ctrl = new DangerController(this);
  _tals_ctrl = new TailsController(this);
  _dgdt_ctrl = new DangerDetailsController(this);
  _vctr_ctrl = new VectorController(this);
  _trgs_ctrl = new TargetsController(this);

  _vn_ctrl = new VnController(this);
  _vd_ctrl = new VdController(this);

  connect(_pult_driver, SIGNAL(gain_changed(int)), _gain_ctrl, SLOT(onValueChanged(int)));
  connect(_pult_driver, SIGNAL(wave_changed(int)), _water_ctrl, SLOT(onValueChanged(int)));
  connect(_pult_driver, SIGNAL(rain_changed(int)), _rain_ctrl, SLOT(onValueChanged(int)));

  // gain_slot used only for simulated Control Panel Unit. Must be removed at finish build
  connect(ui->wgtRLIControl, SIGNAL(gainChanged(int)), _radar_ds, SLOT(setGain(int)));

  connect(ui->wgtRLIDisplay, SIGNAL(initialized()), this, SLOT(onRLIWidgetInitialized()));
  startTimer(33);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction finish";
}

MainWindow::~MainWindow() {
  delete _nmeaprc;
  delete _pult_driver;

  delete ui;

  delete config;

  delete _target_ds;
  delete _radar_ds;
  delete _chart_mngr;

  delete _gain_ctrl;
  delete _water_ctrl;
  delete _rain_ctrl;
  delete _apch_ctrl;
  delete _rdtn_ctrl;

  delete _scle_ctrl;

  delete _lbl1_ctrl;
  delete _lbl2_ctrl;
  delete _lbl3_ctrl;
  delete _lbl4_ctrl;
  delete _lbl5_ctrl;
  delete _band_lbl_ctrl;

  delete _crse_ctrl;

  delete _pstn_ctrl;
  delete _blnk_ctrl;
  delete _dngr_ctrl;

  delete _curs_ctrl;
  delete _clck_ctrl;
  delete _tals_ctrl;
  delete _dgdt_ctrl;
  delete _vctr_ctrl;
  delete _trgs_ctrl;

  delete _vn_ctrl;
  delete _vd_ctrl;
}

#ifndef Q_OS_WIN
void MainWindow::intSignalHandler(int sig) {
  ::write(sigintFd[0], &sig, sizeof(sig));
}

void MainWindow::handleSigInt() {
  snInt->setEnabled(false);
  int sig;
  ::read(sigintFd[1], &sig, sizeof(sig));

  // do Qt stuff
  if(sig == SIGINT) {
    close();
    printf("\nSIGINT caught. Waiting for all threads to terminate\n");
  } else if(sig == SIGTERM) {
    close();
    printf("\nSIGTERM caught. Waiting for all threads to terminate\n");
  } else {
    close();
    fprintf(stderr, "\nUnsupported signale %d caught. Waiting for all threads to terminate\n", sig);
  }

  snInt->setEnabled(true);
}
#endif // !Q_OS_WIN


void MainWindow::onRLIWidgetInitialized() {
  setCursor(QCursor(QPixmap("://res/cursors/cross_72dpi_12px_r0_g128_b255.png")));

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Connect radar to datasource";
  connect(_radar_ds, SIGNAL(updateData(uint, uint, GLfloat*))
        , ui->wgtRLIDisplay->radarEngine(), SLOT(updateData(uint, uint, GLfloat*)));

  connect(_radar_ds, SIGNAL(scaleChanged(RadarScale))
        , _scle_ctrl, SLOT(onScaleChanged(RadarScale)));

  connect(_radar_ds, SIGNAL(scaleChanged(RadarScale))
        , ui->wgtRLIDisplay, SLOT(onScaleChanged(RadarScale)));
  ui->wgtRLIDisplay->onScaleChanged(_radar_ds->getCurrentScale());

  connect(ui->wgtRLIDisplay, SIGNAL(displayVNDistance(float, const char *)), _vd_ctrl, SLOT(display_distance(float, const char *)));
  connect(ui->wgtRLIDisplay, SIGNAL(displaydBRG(float, float)), _vn_ctrl, SLOT(display_brg(float, float)));


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup InfoBlocks";

  setupInfoBlock(_gain_ctrl);
  setupInfoBlock(_water_ctrl);
  setupInfoBlock(_rain_ctrl);
  setupInfoBlock(_apch_ctrl);
  setupInfoBlock(_rdtn_ctrl);
  setupInfoBlock(_curs_ctrl);
  setupInfoBlock(_clck_ctrl);
  setupInfoBlock(_pstn_ctrl);
  setupInfoBlock(_blnk_ctrl);
  setupInfoBlock(_crse_ctrl);
  setupInfoBlock(_scle_ctrl);
  setupInfoBlock(_lbl1_ctrl);
  setupInfoBlock(_lbl2_ctrl);
  setupInfoBlock(_lbl3_ctrl);
  setupInfoBlock(_lbl4_ctrl);
  setupInfoBlock(_lbl5_ctrl);
  setupInfoBlock(_band_lbl_ctrl);
  setupInfoBlock(_dngr_ctrl);
  setupInfoBlock(_tals_ctrl);
  setupInfoBlock(_dgdt_ctrl);
  setupInfoBlock(_vctr_ctrl);
  setupInfoBlock(_trgs_ctrl);
  setupInfoBlock(_vn_ctrl);
  setupInfoBlock(_vd_ctrl);

  qRegisterMetaType<RadarTarget>("RadarTarget");

  connect( ui->wgtRLIDisplay->targetEngine(), SIGNAL(targetCountChanged(int))
         , _trgs_ctrl, SLOT(onTargetCountChanged(int)));

  connect( ui->wgtRLIDisplay->targetEngine(), SIGNAL(selectedTargetUpdated(QString, RadarTarget))
         , _trgs_ctrl, SLOT(updateTarget(QString, RadarTarget)));

  connect(_target_ds, SIGNAL(updateTarget(QString, RadarTarget))
         , ui->wgtRLIDisplay->targetEngine(), SLOT(updateTarget(QString, RadarTarget)));


  _target_ds->start();


  connect(_ship_ds, SIGNAL(coordsUpdated(QVector2D))
         , ui->wgtRLIDisplay, SLOT(onCoordsChanged(QVector2D)));

  _ship_ds->start();

  connect(ui->wgtRLIControl, SIGNAL(vdChanged(float)), ui->wgtRLIDisplay, SLOT(onVdChanged(float)));
  connect(ui->wgtRLIControl, SIGNAL(vnChanged(float)), ui->wgtRLIDisplay, SLOT(onVnChanged(float)));


  connect(_chart_mngr, SIGNAL(new_chart_available(QString)), ui->wgtRLIDisplay, SLOT(new_chart(QString)));
  _chart_mngr->loadCharts();

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup menu signals";

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(languageChanged(QByteArray))
         , ui->wgtRLIDisplay->infoEngine(), SLOT(onLanguageChanged(QByteArray)) );

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(languageChanged(QByteArray))
         , ui->wgtRLIDisplay->menuEngine(), SLOT(onLanguageChanged(QByteArray)) );

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(radarBrightnessChanged(int))
         , ui->wgtRLIDisplay->radarEngine(), SLOT(onBrightnessChanged(int)) );


  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(startRouteEdit())
         , ui->wgtRLIDisplay, SLOT(onStartRouteEdit()) );

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(finishRouteEdit())
         , ui->wgtRLIDisplay, SLOT(onFinishRouteEdit()) );

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(loadRoute(int))
         , ui->wgtRLIDisplay->routeEngine(), SLOT(loadFrom(int)) );

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(saveRoute(int))
         , ui->wgtRLIDisplay->routeEngine(), SLOT(saveTo(int)) );


  connect(ui->wgtRLIDisplay->menuEngine(), SIGNAL(simulationChanged(QByteArray))
        , _radar_ds, SLOT(onSimulationChanged(QByteArray)));


  connect(ui->wgtRLIDisplay, SIGNAL(cursor_moved(QVector2D)), _pstn_ctrl, SLOT(pos_changed(QVector2D)));


  ui->wgtRLIDisplay->menuEngine()->onAnalogZeroChanged(_radar_ds->getAmpsOffset());
  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(analogZeroChanged(int)), _radar_ds, SLOT(setAmpsOffset(int)));

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(tailsModeChanged(QByteArray))
         , _target_ds, SLOT(onTailsModeChanged(const QByteArray)));

  connect(_target_ds, SIGNAL(tailsModeChanged(int, int)), _tals_ctrl, SLOT(onTailsModeChanged(int,int)));
  connect(_target_ds, SIGNAL(tailsModeChanged(int, int))
        , ui->wgtRLIDisplay->targetEngine(), SLOT(onTailsModeChanged(int, int)));



  connect(ui->wgtRLIDisplay->menuEngine(), SIGNAL(bandModeChanged(QByteArray)), ui->wgtRLIDisplay, SLOT(onBandMenu(const QByteArray)));

  connect(ui->wgtRLIDisplay, SIGNAL(band_changed(char **)), _band_lbl_ctrl, SLOT(onTextChanged(char**)));

  this->setFocus();

  // Set zero distance for VRM
  ui->wgtRLIDisplay->onVdChanged(-1e6);


  // Start NMEA processor
  connect(_nmeaprc, SIGNAL(updateTarget(QString, RadarTarget)), ui->wgtRLIDisplay->targetEngine(), SLOT(updateTarget(QString, RadarTarget)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), _crse_ctrl, SLOT(course_changed(float)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), _radar_ds, SLOT(updateHeading(float)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), ui->wgtRLIDisplay, SLOT(onHeadingChanged(float)));

  if (_nmeaImitfn.size())
    _nmeaprc->startNMEAImit(_nmeaImitfn, _nmeaPort.toInt());

  if (_nmeaPort.size()) {
    QStringList ports;
    ports.push_back(_nmeaPort);
    _nmeaprc->open_fds(ports);
    _nmeaprc->start();
  }
}

void MainWindow::setupInfoBlock(InfoBlockController* ctrl) {
  InfoBlock* blck = ui->wgtRLIDisplay->infoEngine()->addInfoBlock();
  ctrl->setupBlock(blck, ui->wgtRLIDisplay->size());

  connect(ctrl, SIGNAL(setRect(int, QRect)), blck, SLOT(setRect(int, QRect)));
  connect(ctrl, SIGNAL(setText(int, int, QByteArray)), blck, SLOT(setText(int, int, QByteArray)));
  connect(ui->wgtRLIDisplay, SIGNAL(resized(QSize)), ctrl, SLOT(onResize(QSize)));
}

void MainWindow::resizeEvent(QResizeEvent* e) {
  qDebug() << "Resize event: " << e->size();

  QSize s = e->size();
  if (float(s.height()) / float(s.width()) > 0.7)
    ui->wgtRLIControl->hide();
  else
    ui->wgtRLIControl->show();

  ui->wgtRLIControl->setVisible(config->showButtonPanel());
  QSize availableSize( config->showButtonPanel() ? geometry().width() - ui->wgtRLIControl->width()
                                                 : geometry().width()
                     , geometry().height());
  QString bestSize = config->getSuitableLayoutSize(availableSize);
  QStringList slbestSize = bestSize.split("x");

  ui->spsMainCenter->changeSize(availableSize.width() - slbestSize[0].toInt(), 20);
  ui->wgtRLIDisplay->setGeometry(QRect(0, 0, slbestSize[0].toInt(), slbestSize[1].toInt()));
}

void MainWindow::timerEvent(QTimerEvent*) {
  ui->wgtRLIDisplay->update();
}







int MainWindow::findPressedKey(int key) {
  for (int i = 0; (unsigned int)i < sizeof(pressedKey) / sizeof(pressedKey[0]); i++)
    if (pressedKey[i] == key)
      return i;

  return -1;
}

int MainWindow::savePressedKey(int key) {
  int zeroIdx = -1;
  for (int i = sizeof(pressedKey) / sizeof(pressedKey[0]); i--; ) {
    if (pressedKey[i] == 0)
      zeroIdx = i;
    if (pressedKey[i] == key)
      return i;
  }

  if (zeroIdx == -1)
    zeroIdx = sizeof(pressedKey) / sizeof(pressedKey[0]) - 1;
  pressedKey[zeroIdx] = key;
  return zeroIdx;
}

int MainWindow::countPressedKeys(void) {
  int cnt = 0;

  for (int i = 0; (unsigned int)i < sizeof(pressedKey) / sizeof(pressedKey[0]); i++)
    if(pressedKey != 0)
      cnt++;

  return cnt;
}

void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  int idx = findPressedKey(event->key());
  if (idx != -1)
    pressedKey[idx] = 0;
}


void MainWindow::keyPressEvent(QKeyEvent *event) {
  switch(event->key()) {
  //Под. имп. Помех
  case Qt::Key_S:
    if(_radar_ds)
      _radar_ds->nextHIP(RadarDataSource::HIPC_MAIN);
    break;
  //Меню
  case Qt::Key_W:
    if(findPressedKey(Qt::Key_B) != -1)
      ui->wgtRLIDisplay->onConfigMenuToggled();
    else
      ui->wgtRLIDisplay->onMenuToggled();
    break;
  //Шкала +
  case Qt::Key_Plus:
    _radar_ds->nextScale();
    ui->wgtRLIDisplay->onVdChanged(0);
    break;
  //Шкала -
  case Qt::Key_Minus:
    _radar_ds->prevScale();
    ui->wgtRLIDisplay->onVdChanged(0);
    break;
  //Вынос центра
  case Qt::Key_C: {
      QPoint p = mapFromGlobal(QCursor::pos());
      QMouseEvent * evt = new QMouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      qApp->postEvent(ui->wgtRLIDisplay, evt);
      evt = new QMouseEvent(QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      qApp->postEvent(ui->wgtRLIDisplay, evt);
      ui->wgtRLIDisplay->onCenterShiftToggled();
      break;
  }
  //Скрытое меню
  case Qt::Key_U:
    ui->wgtRLIDisplay->onConfigMenuToggled();
    break;
  //Следы точки
  case Qt::Key_T:
    _target_ds->incrementMode();
    break;
  //Выбор цели
  case Qt::Key_Up:
    ui->wgtRLIDisplay->onUpToggled();
    break;
  //ЛИД / ЛОД
  case Qt::Key_Down:
    ui->wgtRLIDisplay->onDownToggled();
    break;
  //Захват
  case Qt::Key_Enter:
    ui->wgtRLIDisplay->onEnterToggled();
    break;
  //Захват
  case Qt::Key_Return:
    ui->wgtRLIDisplay->onEnterToggled();
    break;
  //Сброс
  case Qt::Key_Escape:
    ui->wgtRLIDisplay->onBackToggled();
    break;
  //Парал. Линии
  case Qt::Key_Backslash:
    ui->wgtRLIDisplay->onParallelLinesToggled();
    break;
  //Электронная лупа
  case Qt::Key_L:
    ui->wgtRLIDisplay->onMagnifierToggled();
    break;
  //Обзор
  case Qt::Key_X:
    break;
  //Узкий / Шир.
  case Qt::Key_Greater:
    break;
  //Накоп. Видео
  case Qt::Key_V:
    break;
  //Сброс АС
  case Qt::Key_Q:
    break;
  //Манёвр
  case Qt::Key_M:
    break;
  //Курс / Север / Курс стаб
  case Qt::Key_H:
    break;
  //ИД / ОД
  case Qt::Key_R:
    break;
  //НКД
  case Qt::Key_D:
    break;
  //Карта (Маршрут)
  case Qt::Key_A:
    break;
  //Выбор
  case Qt::Key_G:
    break;
  //Стоп-кадр
  case Qt::Key_F:
    break;
  //Откл. Звука
  case Qt::Key_B:
    break;
  //Откл. ОК
  case Qt::Key_K:
    break;
  //Вынос ВН/ВД
  case Qt::Key_Slash:
    break;
  }

  QMainWindow::keyPressEvent(event);
  savePressedKey(event->key());
}
