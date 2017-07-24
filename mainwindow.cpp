#include "mainwindow.h"

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

MainWindow::MainWindow(QWidget *parent) : QWidget(parent) {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction start";

  wgtRLI = new RLIDisplayWidget(this);
  wgtButtonPanel = new RLIControlWidget(this);

  connect(wgtButtonPanel, SIGNAL(closeApp()), SLOT(onClose()));

#ifndef Q_OS_WIN
  // Initializing SIGINT handling
  if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd) == 0) {
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
    if(setup_unix_signal_handlers() != 0)
      qDebug() << "Failed to setup SIGINT handler. Ctrl+C will terminate the program without cleaning.";
  } else
    qDebug() << "Couldn't create INT socketpair. Ctrl+C will terminate the program without cleaning.";
#endif // !Q_OS_WIN

  _pult_driver = new BoardPultController(this);
  _target_ds = new TargetDataSource();
  _radar_ds = new RadarDataSource();
  _ship_ds = new ShipDataSource();
  _chart_mngr = new ChartManager();

  wgtRLI->setChartManager(_chart_mngr);

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

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init finish";

  rx.setPattern("--nmea-port");
  argpos = args.indexOf(rx);

  if((argpos >= 0) && (argpos < args.count() - 1))
      _nmeaPort = args.at(argpos + 1);

  rx.setPattern("--nmea-imit-file");
  argpos = args.indexOf(rx);
  if ((argpos >= 0) && (argpos < args.count() - 1))
     _nmeaImitfn = args.at(argpos + 1);

  _nmeaprc = new NMEAProcessor(this);



  _gain_ctrl = new ValueBarController(RLIStrings::nGain, 255, this);
  connect(wgtButtonPanel, SIGNAL(gainChanged(int)), _gain_ctrl, SLOT(onValueChanged(int)));

  _water_ctrl = new ValueBarController(RLIStrings::nWave, 255, this);
  connect(wgtButtonPanel, SIGNAL(waterChanged(int)), _water_ctrl, SLOT(onValueChanged(int)));

  _rain_ctrl = new ValueBarController(RLIStrings::nRain, 255, this);
  connect(wgtButtonPanel, SIGNAL(rainChanged(int)), _rain_ctrl, SLOT(onValueChanged(int)));

  _apch_ctrl = new ValueBarController(RLIStrings::nAfc, 255, this);
  _rdtn_ctrl = new ValueBarController(RLIStrings::nEmsn, 255, this);


  _lbl5_ctrl = new LabelController(RLIStrings::nPP12p, this);
  _band_lbl_ctrl = new LabelController(RLIStrings::nBandS, this);

  _lbl1_ctrl = new LabelController(RLIStrings::nNord, this);
  _lbl2_ctrl = new LabelController(RLIStrings::nRm, this);
  _lbl3_ctrl = new LabelController(RLIStrings::nWstab, this);
  _lbl4_ctrl = new LabelController(RLIStrings::nLod, this);

  _curs_ctrl = new CursorController(this);
  connect(wgtRLI, SIGNAL(cursor_moved(float,float, const char *)), _curs_ctrl, SLOT(cursor_moved(float,float, const char *)));

  _clck_ctrl = new ClockController(this);
  connect(wgtRLI, SIGNAL(per_second()), _clck_ctrl, SLOT(second_changed()));

  _scle_ctrl = new ScaleController(this);

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
  connect(wgtButtonPanel, SIGNAL(gainChanged(int)), _radar_ds, SLOT(setGain(int)));

  connect(wgtRLI, SIGNAL(initialized()), this, SLOT(onRLIWidgetInitialized()));
  startTimer(33);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction finish";
}

MainWindow::~MainWindow() {
  delete _nmeaprc;
  delete _pult_driver;

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
  connect(_radar_ds, SIGNAL(updateData(uint, uint, GLfloat*)), wgtRLI->radarEngine(), SLOT(updateData(uint, uint, GLfloat*)));
  connect(_radar_ds, SIGNAL(scaleChanged(RadarScale)), _scle_ctrl, SLOT(onScaleChanged(RadarScale)));
  connect(_radar_ds, SIGNAL(scaleChanged(RadarScale)), wgtRLI, SLOT(onScaleChanged(RadarScale)));
  wgtRLI->onScaleChanged(_radar_ds->getCurrentScale());

  connect(wgtRLI, SIGNAL(displayVNDistance(float, const char *)), _vd_ctrl, SLOT(display_distance(float, const char *)));
  connect(wgtRLI, SIGNAL(displaydBRG(float, float)), _vn_ctrl, SLOT(display_brg(float, float)));


  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup InfoBlocks";

  const RLILayout* layout = RLIConfig::instance().currentLayout();

  setupInfoBlock(_gain_ctrl, layout->panels["gain"]);
  setupInfoBlock(_water_ctrl, layout->panels["water"]);
  setupInfoBlock(_rain_ctrl, layout->panels["rain"]);
  setupInfoBlock(_apch_ctrl, layout->panels["apch"]);

  setupInfoBlock(_rdtn_ctrl, layout->panels["emission"]);
  setupInfoBlock(_curs_ctrl, layout->panels["cursor"]);
  setupInfoBlock(_clck_ctrl, layout->panels["clock"]);
  setupInfoBlock(_pstn_ctrl, layout->panels["position"]);
  setupInfoBlock(_blnk_ctrl, layout->panels["blank"]);
  setupInfoBlock(_crse_ctrl, layout->panels["course"]);
  setupInfoBlock(_scle_ctrl, layout->panels["scale"]);

  setupInfoBlock(_lbl1_ctrl, layout->panels["label1"]);
  setupInfoBlock(_lbl2_ctrl, layout->panels["label2"]);
  setupInfoBlock(_lbl3_ctrl, layout->panels["label3"]);
  setupInfoBlock(_lbl4_ctrl, layout->panels["label4"]);
  setupInfoBlock(_lbl5_ctrl, layout->panels["label5"]);

  setupInfoBlock(_band_lbl_ctrl, layout->panels["band"]);
  setupInfoBlock(_dngr_ctrl, layout->panels["danger"]);

  setupInfoBlock(_tals_ctrl, layout->panels["tails"]);
  setupInfoBlock(_dgdt_ctrl, layout->panels["danger-details"]);
  setupInfoBlock(_vctr_ctrl, layout->panels["vector"]);
  setupInfoBlock(_trgs_ctrl, layout->panels["targers"]);

  setupInfoBlock(_vn_ctrl, layout->panels["vn"]);
  setupInfoBlock(_vd_ctrl, layout->panels["vd"]);

  qRegisterMetaType<RadarTarget>("RadarTarget");

  connect( wgtRLI->targetEngine(), SIGNAL(targetCountChanged(int))
         , _trgs_ctrl, SLOT(onTargetCountChanged(int)));

  connect( wgtRLI->targetEngine(), SIGNAL(selectedTargetUpdated(QString, RadarTarget))
         , _trgs_ctrl, SLOT(updateTarget(QString, RadarTarget)));

  connect(_target_ds, SIGNAL(updateTarget(QString, RadarTarget))
         , wgtRLI->targetEngine(), SLOT(updateTarget(QString, RadarTarget)));


  _target_ds->start();


  connect(_ship_ds, SIGNAL(coordsUpdated(QVector2D))
         , wgtRLI, SLOT(onCoordsChanged(QVector2D)));

  _ship_ds->start();

  connect(wgtButtonPanel, SIGNAL(vdChanged(float)), wgtRLI, SLOT(onVdChanged(float)));
  connect(wgtButtonPanel, SIGNAL(vnChanged(float)), wgtRLI, SLOT(onVnChanged(float)));


  connect(_chart_mngr, SIGNAL(new_chart_available(QString)), wgtRLI, SLOT(new_chart(QString)));
  _chart_mngr->loadCharts();

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup menu signals";

  connect( wgtRLI->menuEngine(), SIGNAL(languageChanged(QByteArray)), wgtRLI->infoEngine(), SLOT(onLanguageChanged(QByteArray)) );
  connect( wgtRLI->menuEngine(), SIGNAL(languageChanged(QByteArray)), wgtRLI->menuEngine(), SLOT(onLanguageChanged(QByteArray)) );
  connect( wgtRLI->menuEngine(), SIGNAL(radarBrightnessChanged(int)), wgtRLI->radarEngine(), SLOT(onBrightnessChanged(int)) );


  connect( wgtRLI->menuEngine(), SIGNAL(startRouteEdit()), wgtRLI, SLOT(onStartRouteEdit()) );
  connect( wgtRLI->menuEngine(), SIGNAL(finishRouteEdit()), wgtRLI, SLOT(onFinishRouteEdit()) );
  connect( wgtRLI->menuEngine(), SIGNAL(loadRoute(int)), wgtRLI->routeEngine(), SLOT(loadFrom(int)) );
  connect( wgtRLI->menuEngine(), SIGNAL(saveRoute(int)), wgtRLI->routeEngine(), SLOT(saveTo(int)) );


  connect(wgtRLI->menuEngine(), SIGNAL(simulationChanged(QByteArray)), _radar_ds, SLOT(onSimulationChanged(QByteArray)));

  connect(wgtRLI, SIGNAL(cursor_moved(QVector2D)), _pstn_ctrl, SLOT(pos_changed(QVector2D)));


  wgtRLI->menuEngine()->onAnalogZeroChanged(_radar_ds->getAmpsOffset());
  connect( wgtRLI->menuEngine(), SIGNAL(analogZeroChanged(int)), _radar_ds, SLOT(setAmpsOffset(int)));

  connect( wgtRLI->menuEngine(), SIGNAL(tailsModeChanged(QByteArray)), _target_ds, SLOT(onTailsModeChanged(const QByteArray)));

  connect(_target_ds, SIGNAL(tailsModeChanged(int, int)), _tals_ctrl, SLOT(onTailsModeChanged(int,int)));
  connect(_target_ds, SIGNAL(tailsModeChanged(int, int)), wgtRLI->targetEngine(), SLOT(onTailsModeChanged(int, int)));



  connect(wgtRLI->menuEngine(), SIGNAL(bandModeChanged(QByteArray)), wgtRLI, SLOT(onBandMenu(const QByteArray)));

  connect(wgtRLI, SIGNAL(band_changed(char **)), _band_lbl_ctrl, SLOT(onTextChanged(char**)));

  this->setFocus();

  // Set zero distance for VRM
  wgtRLI->onVdChanged(-1e6);


  // Start NMEA processor
  connect(_nmeaprc, SIGNAL(updateTarget(QString, RadarTarget)), wgtRLI->targetEngine(), SLOT(updateTarget(QString, RadarTarget)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), _crse_ctrl, SLOT(course_changed(float)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), _radar_ds, SLOT(updateHeading(float)));
  connect(_nmeaprc, SIGNAL(updateHdgGyro(float)), wgtRLI, SLOT(onHeadingChanged(float)));

  if (_nmeaImitfn.size())
    _nmeaprc->startNMEAImit(_nmeaImitfn, _nmeaPort.toInt());

  if (_nmeaPort.size()) {
    QStringList ports;
    ports.push_back(_nmeaPort);
    _nmeaprc->open_fds(ports);
    _nmeaprc->start();
  }
}

void MainWindow::setupInfoBlock(InfoBlockController* ctrl, const RLIPanelInfo& panelInfo) {
  InfoBlock* blck = wgtRLI->infoEngine()->addInfoBlock();
  ctrl->setupBlock(blck, RLIConfig::instance().currentSize(), panelInfo);

  connect(ctrl, SIGNAL(setRect(int, QRect)), blck, SLOT(setRect(int, QRect)));
  connect(ctrl, SIGNAL(setText(int, int, QByteArray)), blck, SLOT(setText(int, int, QByteArray)));  
}

void MainWindow::resizeEvent(QResizeEvent* e) {
  QSize s = e->size();

  bool showButtonPanel = RLIConfig::instance().showButtonPanel();
  wgtButtonPanel->setVisible(showButtonPanel);
  wgtButtonPanel->move( width() - wgtButtonPanel->width(), 0 );

  QSize availableSize(showButtonPanel ? s.width() - wgtButtonPanel->width() : s.width(), s.height());
  RLIConfig::instance().updateCurrentSize(availableSize);
  QSize curSize = RLIConfig::instance().currentSize();
  wgtRLI->setGeometry(QRect(QPoint(0, 0), curSize));

  const RLILayout* layout = RLIConfig::instance().currentLayout();

  _gain_ctrl->resize(curSize, layout->panels["gain"]);
  _water_ctrl->resize(curSize, layout->panels["water"]);
  _rain_ctrl->resize(curSize, layout->panels["rain"]);
  _apch_ctrl->resize(curSize, layout->panels["apch"]);

  _rdtn_ctrl->resize(curSize, layout->panels["emission"]);
  _curs_ctrl->resize(curSize, layout->panels["cursor"]);
  _clck_ctrl->resize(curSize, layout->panels["clock"]);
  _pstn_ctrl->resize(curSize, layout->panels["position"]);
  _blnk_ctrl->resize(curSize, layout->panels["blank"]);
  _crse_ctrl->resize(curSize, layout->panels["course"]);
  _scle_ctrl->resize(curSize, layout->panels["scale"]);

  _lbl1_ctrl->resize(curSize, layout->panels["label1"]);
  _lbl2_ctrl->resize(curSize, layout->panels["label2"]);
  _lbl3_ctrl->resize(curSize, layout->panels["label3"]);
  _lbl4_ctrl->resize(curSize, layout->panels["label4"]);
  _lbl5_ctrl->resize(curSize, layout->panels["label5"]);

  _band_lbl_ctrl->resize(curSize, layout->panels["band"]);
  _dngr_ctrl->resize(curSize, layout->panels["danger"]);

  _tals_ctrl->resize(curSize, layout->panels["tails"]);
  _dgdt_ctrl->resize(curSize, layout->panels["danger-details"]);
  _vctr_ctrl->resize(curSize, layout->panels["vector"]);
  _trgs_ctrl->resize(curSize, layout->panels["targers"]);

  _vn_ctrl->resize(curSize, layout->panels["vn"]);
  _vd_ctrl->resize(curSize, layout->panels["vd"]);
}

void MainWindow::timerEvent(QTimerEvent*) {
  wgtRLI->update();
}


void MainWindow::keyReleaseEvent(QKeyEvent *event) {
  pressedKeys.remove(event->key());
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
    if(pressedKeys.contains(Qt::Key_B))
      wgtRLI->onConfigMenuToggled();
    else
      wgtRLI->onMenuToggled();
    break;
  //Шкала +
  case Qt::Key_Plus:
    _radar_ds->nextScale();
    wgtRLI->onVdChanged(0);
    break;
  //Шкала -
  case Qt::Key_Minus:
    _radar_ds->prevScale();
    wgtRLI->onVdChanged(0);
    break;
  //Вынос центра
  case Qt::Key_C: {
      QPoint p = mapFromGlobal(QCursor::pos());
      QMouseEvent * evt = new QMouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      qApp->postEvent(wgtRLI, evt);
      evt = new QMouseEvent(QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
      qApp->postEvent(wgtRLI, evt);
      wgtRLI->onCenterShiftToggled();
      break;
  }
  //Скрытое меню
  case Qt::Key_U:
    wgtRLI->onConfigMenuToggled();
    break;
  //Следы точки
  case Qt::Key_T:
    _target_ds->incrementMode();
    break;
  //Выбор цели
  case Qt::Key_Up:
    wgtRLI->onUpToggled();
    break;
  //ЛИД / ЛОД
  case Qt::Key_Down:
    wgtRLI->onDownToggled();
    break;
  //Захват
  case Qt::Key_Enter:
    wgtRLI->onEnterToggled();
    break;
  //Захват
  case Qt::Key_Return:
    wgtRLI->onEnterToggled();
    break;
  //Сброс
  case Qt::Key_Escape:
    wgtRLI->onBackToggled();
    break;
  //Парал. Линии
  case Qt::Key_Backslash:
    wgtRLI->onParallelLinesToggled();
    break;
  //Электронная лупа
  case Qt::Key_L:
    wgtRLI->onMagnifierToggled();
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

  QWidget::keyPressEvent(event);
  pressedKeys.insert(event->key());
}
