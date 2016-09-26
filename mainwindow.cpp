#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>

#include "rlistrings.h"
#include "rlicontrolevent.h"

#ifndef Q_OS_WIN

#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <linux/input.h>

#define ABS_AXIS_MUL 1

int MainWindow::sigintFd[2];

static int setup_unix_signal_handlers()
{
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

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction start";

#ifndef Q_OS_WIN
  evdevFd    = -1;
  stopEvdev  = 0;
  memset(pressedKey, 0, sizeof(pressedKey));
#endif // !Q_OS_WIN

  ui->setupUi(this);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init start";

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

  _radar_ds = new RadarDataSource();
  _chart_mngr = new ChartManager();

  ui->wgtRLIDisplay->setChartManager(_chart_mngr);

  QStringList args = qApp->arguments();
  QRegExp     rx("--ampoff=[+|-]?[0-9]+$");
  int argpos = args.indexOf(rx);
  if(argpos >= 0)
  {
      rx.setPattern("[+|-]?[0-9]+$");
      int offpos = rx.indexIn(args.at(argpos));
      if(offpos >= 0)
          _radar_ds->setAmpsOffset(args.at(argpos).mid(offpos).toInt());
  }

  rx.setPattern("--radar-device");
  argpos = args.indexOf(rx);

  if((argpos >= 0) && (argpos < args.count() - 1))
      _radar_ds->start(args.at(argpos + 1).toStdString().c_str());
  else
  {
      rx.setPattern("--use-dump");
      argpos = args.indexOf(rx);
      if(argpos >= 0)
          _radar_ds->start_dump();
      else
          _radar_ds->start();
  }

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
  _rdtn_ctrl = new ValueBarController(RLIStrings::nEmsn, QPoint(5+12*8+60+5, 5), 9, -1, this);

  _curs_ctrl = new CursorController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(cursor_moved(float,float, const char *)), _curs_ctrl, SLOT(cursor_moved(float,float, const char *)));

  _clck_ctrl = new ClockController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(per_second()), _clck_ctrl, SLOT(second_changed()));

  _scle_ctrl = new ScaleController(this);

  _lbl1_ctrl = new LableController(RLIStrings::nNord, QRect(-246-120, 45, 120, 21), "12x14", this);
  _lbl2_ctrl = new LableController(RLIStrings::nRm, QRect(-246-120, 70, 120, 21), "12x14", this);
  _lbl3_ctrl = new LableController(RLIStrings::nWstab, QRect(-246-120, 95, 120, 21), "12x14", this);
  _lbl4_ctrl = new LableController(RLIStrings::nLod, QRect(-246-96, -100, 96, 33), "16x28", this);

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

  _radar_scale = new RadarScale;

  connect(this, SIGNAL(gain_changed(int)), _gain_ctrl, SLOT(onValueChanged(int)));
  connect(this, SIGNAL(wave_changed(int)), _water_ctrl, SLOT(onValueChanged(int)));
  connect(this, SIGNAL(rain_changed(int)), _rain_ctrl, SLOT(onValueChanged(int)));

  // gain_slot used only for simulated Control Panel Unit. Must be removed at finish build
  connect(ui->wgtRLIControl, SIGNAL(gainChanged(int)), this, SLOT(gain_slot(int)));

#ifndef Q_OS_WIN
  setupEvdev("/dev/input/event1");
#endif // !Q_OS_WIN


  connect(ui->wgtRLIDisplay, SIGNAL(initialized()), this, SLOT(onRLIWidgetInitialized()));
  startTimer(33);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction finish";
}

MainWindow::~MainWindow() {
#ifndef Q_OS_WIN
  printf("Waiting for the input event device thread to terminate.\n");
  while(evdevThread.isRunning());
  printf("Input event device thread terminated.\n");
  ::close(evdevFd);
  evdevFd = -1;
#endif // !Q_OS_WIN

  delete ui;

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
void MainWindow::intSignalHandler(int sig)
{
  ::write(sigintFd[0], &sig, sizeof(sig));
}

void MainWindow::handleSigInt()
{
  snInt->setEnabled(false);
  int sig;
  ::read(sigintFd[1], &sig, sizeof(sig));

  // do Qt stuff
  if(sig == SIGINT)
  {
      close();
      printf("\nSIGINT caught. Waiting for all threads to terminate\n");
  }
  else if(sig == SIGTERM)
  {
      close();
      printf("\nSIGTERM caught. Waiting for all threads to terminate\n");
  }
  else
  {
      close();
      fprintf(stderr, "\nUnsupported signale %d caught. Waiting for all threads to terminate\n", sig);
  }

  snInt->setEnabled(true);
}
#endif // !Q_OS_WIN


int MainWindow::findPressedKey(int key)
{
    for(int i = 0; (unsigned int)i < sizeof(pressedKey) / sizeof(pressedKey[0]); i++)
    {
        if(pressedKey[i] == key)
            return i;
    }
    return -1;
}

int MainWindow::savePressedKey(int key)
{
    int zeroIdx = -1;
    for(int i = sizeof(pressedKey) / sizeof(pressedKey[0]); i--; )
    {
        if(pressedKey[i] == 0)
            zeroIdx = i;
        if(pressedKey[i] == key)
            return i;
    }
    if(zeroIdx == -1)
        zeroIdx = sizeof(pressedKey) / sizeof(pressedKey[0]) - 1;
    pressedKey[zeroIdx] = key;
    return zeroIdx;
}

int MainWindow::countPressedKeys(void)
{
    int cnt = 0;

    for(int i = 0; (unsigned int)i < sizeof(pressedKey) / sizeof(pressedKey[0]); i++)
    {
        if(pressedKey != 0)
            cnt++;
    }

    return cnt;
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    int idx = findPressedKey(event->key());
    if(idx != -1)
        pressedKey[idx] = 0;
}


void MainWindow::keyPressEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_S:
        if(_radar_ds)
            _radar_ds->nextHIP(RadarDataSource::HIPC_MAIN);
        break;
    case Qt::Key_W:
    {
        if(findPressedKey(Qt::Key_B) != -1)
        {
            RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ConfigMenu);
            qApp->postEvent(ui->wgtRLIDisplay, e);
        }
        else
        {
            RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Menu);
            qApp->postEvent(ui->wgtRLIDisplay, e);
        }
        break;
    }
    case Qt::Key_Plus:
        if(_radar_scale->nextScale() == 0)
        {
            const rli_scale_t * scale = _radar_scale->getCurScale();
            _radar_ds->setupScale(scale);
            std::pair<QByteArray, QByteArray> s = _radar_scale->getCurScaleText();
            emit scale_changed(s);
            RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::NoButton
                                                   , RLIControlEvent::VD
                                                   , 0);
            qApp->postEvent(ui->wgtRLIDisplay, e);
        }
        break;
    case Qt::Key_Minus:
        if(_radar_scale->prevScale() == 0)
        {
            const rli_scale_t * scale = _radar_scale->getCurScale();
            _radar_ds->setupScale(scale);
            std::pair<QByteArray, QByteArray> s = _radar_scale->getCurScaleText();
            emit scale_changed(s);
            RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::NoButton
                                                   , RLIControlEvent::VD
                                                   , 0);
            qApp->postEvent(ui->wgtRLIDisplay, e);
        }
        break;
    case Qt::Key_C:
    {
        QPoint p = mapFromGlobal(QCursor::pos());
        QMouseEvent * evt = new QMouseEvent(QEvent::MouseButtonPress, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        qApp->postEvent(ui->wgtRLIDisplay, evt);
        evt = new QMouseEvent(QEvent::MouseButtonRelease, p, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        qApp->postEvent(ui->wgtRLIDisplay, evt);
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::CenterShift);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_U:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ConfigMenu);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_Up:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Up);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_Down:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Down);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_Enter:
    case Qt::Key_Return:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Enter);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_Escape:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::Back);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    case Qt::Key_Backslash:
    {
        RLIControlEvent* e = new RLIControlEvent(RLIControlEvent::ParallelLines);
        qApp->postEvent(ui->wgtRLIDisplay, e);
        break;
    }
    default:
        QMainWindow::keyPressEvent(event);
    }
    savePressedKey(event->key());
}

void MainWindow::onRLIWidgetInitialized() {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Connect radar to datasource";
  connect(_radar_ds, SIGNAL(updateData(uint, uint, GLfloat*))
        , ui->wgtRLIDisplay->radarEngine(), SLOT(updateData(uint, uint, GLfloat*)));

  connect(this, SIGNAL(scale_changed(std::pair<QByteArray, QByteArray>))
        , _scle_ctrl, SLOT(scale_changed(std::pair<QByteArray, QByteArray>)));

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
  setupInfoBlock(_dngr_ctrl);
  setupInfoBlock(_tals_ctrl);
  setupInfoBlock(_dgdt_ctrl);
  setupInfoBlock(_vctr_ctrl);
  setupInfoBlock(_trgs_ctrl);
  setupInfoBlock(_vn_ctrl);
  setupInfoBlock(_vd_ctrl);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup rliwidget as control reciever";
  ui->wgtRLIControl->setReciever(ui->wgtRLIDisplay);

  connect(_chart_mngr, SIGNAL(new_chart_available(QString)), ui->wgtRLIDisplay, SLOT(new_chart(QString)));
  _chart_mngr->loadCharts();

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Setup menu signals";
  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(languageChanged(QByteArray))
         , ui->wgtRLIDisplay->infoEngine(), SLOT(onLanguageChanged(QByteArray)));

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(languageChanged(QByteArray))
         , ui->wgtRLIDisplay->menuEngine(), SLOT(onLanguageChanged(QByteArray)));

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(radarBrightnessChanged(int))
         , ui->wgtRLIDisplay->radarEngine(), SLOT(onBrightnessChanged(int)));

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(radarBrightnessChanged(int))
         , ui->wgtRLIDisplay->radarEngine(), SLOT(onBrightnessChanged(int)));

  connect( ui->wgtRLIDisplay->menuEngine(), SIGNAL(onSimChanged(bool))
         , this, SLOT(simulation_slot(bool)));

  ui->wgtRLIDisplay->menuEngine()->setMenuItemIntValue(_radar_ds->getAmpsOffset(), MenuEngine::CONFIG, RLIStrings::nMenu112[RLI_LANG_ENGLISH], RLI_LANG_ENGLISH);
  RLIMenuItem * mnuitem = ui->wgtRLIDisplay->menuEngine()->findItem(MenuEngine::CONFIG, RLIStrings::nMenu112[RLI_LANG_ENGLISH], RLI_LANG_ENGLISH);
  RLIMenuItemInt * mii = dynamic_cast<RLIMenuItemInt *>(mnuitem);
  if(mii)
      connect(mii, SIGNAL(valueChanged(int)), this, SLOT(on_mnuAnalogZeroChanged(int)));
  else
      fprintf(stderr, "Failed to find menu: \'%s\'\n", RLIStrings::nMenu112[RLI_LANG_ENGLISH]);

  this->setFocus();
}

void MainWindow::setupInfoBlock(InfoBlockController* ctrl) {
  InfoBlock* blck = ui->wgtRLIDisplay->infoEngine()->addInfoBlock();
  ctrl->setupBlock(blck, ui->wgtRLIDisplay->size());

  connect(ctrl, SIGNAL(setRect(int, QRect)), blck, SLOT(setRect(int, QRect)));
  connect(ctrl, SIGNAL(setText(int, int, QByteArray)), blck, SLOT(setText(int, int, QByteArray)));
  connect(ui->wgtRLIDisplay, SIGNAL(resized(QSize)), ctrl, SLOT(onResize(QSize)));
}

void MainWindow::resizeEvent(QResizeEvent* e) {
  Q_UNUSED(e);
  QRect rli_geom = ui->wgtRLIDisplay->geometry();

  rli_geom.setWidth(4 * (rli_geom.width() / 4));
  rli_geom.setHeight(3 * (rli_geom.height() / 3));

  if (rli_geom.height() > 3 * (rli_geom.width() / 4))
    rli_geom.setHeight(3 * (rli_geom.width() / 4));
  else
    rli_geom.setWidth(4 * (rli_geom.height() / 3));

  ui->wgtRLIDisplay->setGeometry(rli_geom);
}

void MainWindow::timerEvent(QTimerEvent* e) {
  Q_UNUSED(e);
  const rli_scale_t* scale =  _radar_scale->getCurScale();

  ui->wgtRLIDisplay->setScale(scale->len);
  ui->wgtRLIDisplay->update();
}

void MainWindow::on_btnClose_clicked() {
  close();
}

#ifndef Q_OS_WIN

int MainWindow::setupEvdev(char *evdevfn)
{
    int             res;
    int             fd, ver;
    struct input_id iid;
    char            edname[256];

    if(evdevFd != -1)
    {
        fprintf(stderr, "WARNING: Input event device file is already opened.\n");
        return 1;
    }

    if (evdevThread.isRunning())
        return 2;

    if(evdevfn == NULL)
    {
        fprintf(stderr, "WARNING: Name of an input event device file is given.\n\tGain, Rain and Waves controls are unavailable\n");
        return -1000;
    }

    fd = ::open(evdevfn, O_RDWR);
    if(fd == -1)
    {
        res = errno;
        fprintf(stderr, "WARNING: Failed to open input event device file.\n\tGain, Rain and Waves controls are unavailable\n");
        fprintf(stderr, "Error: %s\n", strerror(res));
        return res;
    }

    res = ::ioctl(fd, EVIOCGVERSION, &ver);
    if(res == -1)
    {
        res = errno;
        fprintf(stderr, "WARNING: Failed to get version of input event device.\n");
        fprintf(stderr, "Error: %s\n", strerror(res));
    }
    else
        printf("Version of input device driver: %d\n", ver);

    res = ::ioctl(fd, EVIOCGID, &iid);
    if(res == -1)
    {
        res = errno;
        fprintf(stderr, "WARNING: Failed to get ID of input event device.\n");
        fprintf(stderr, "Error: %s\n", strerror(res));
    }
    else
        printf("ID of input event device:\n\tbus %04x, vendor %04x, product %04x, version %04x\n", iid.bustype, iid.vendor, iid.product, iid.version);

    res = ::ioctl(fd, EVIOCGNAME(sizeof(edname)), edname);
    if(res == -1)
    {
        res = errno;
        fprintf(stderr, "ERROR: Failed to get the name of the input event device.\n\tGain, Rain and Waves controls are unavailable\n");
        fprintf(stderr, "Error: %s\n", strerror(res));
        ::close(fd);
        return res;
    }
    printf("Input event device name: %s\n", edname);

    QString sname(edname);
    res = sname.indexOf("Pult", 0, Qt::CaseInsensitive);
    if(res == -1)
    {
        res = errno;
        fprintf(stderr, "ERROR: Wrong input event device name. Must be \'Board Pult\'.\n\tGain, Rain and Waves controls are unavailable\n");
        fprintf(stderr, "Error: %s\n", strerror(res));
        ::close(fd);
        return res;
    }

	evdevFd = fd;

    stopEvdev   = 0;
    evdevThread = QtConcurrent::run(this, &MainWindow::evdevHandleThread);
    return 0;
}

void MainWindow::evdevHandleThread()
{
    struct input_event ie;
    int                value;

    if(evdevFd == -1)
    {
        fprintf(stderr, "WARNING: Input event device file is not opened.\n\tGain, Rain and Waves controls are unavailable\n");
        return;
    }
    while(!stopEvdev)
    {
        int res = read(evdevFd, &ie, sizeof(ie));

        if(res != sizeof(ie))
        {
            printf("read returned: %d\n", res);
            continue;
        }
        if(ie.type != EV_ABS)
        {
            printf("type != EV_ABS: %d\n", ie.type);
            continue;
        }
        //----- now the e.code field contains the axis ID, value should be in range [0; 255]
        //

        switch(ie.code)
        {
        case ABS_THROTTLE:
            value = ie.value * ABS_AXIS_MUL;
            value = 255 - value;
            _radar_ds->setGain(value);
            emit gain_changed(value);
            break;
        case ABS_GAS:
            value = ie.value * ABS_AXIS_MUL;
            value = 255 - value;
            emit wave_changed(value);
            break;
        case ABS_BRAKE:
            value = ie.value * ABS_AXIS_MUL;
            value = 255 - value;
            emit rain_changed(value);
            break;
        default:
            printf("EV_ABS: code: %u, value %d\n", (unsigned int) ie.code, ie.value);
        }
    }
}
#endif // !Q_OS_WIN

// gain_slot used only for simulated Control Panel Unit. Must be removed at finish build
void MainWindow::gain_slot(int value)
{
    _radar_ds->setGain(value);
}

void MainWindow::on_mnuAnalogZeroChanged(int val)
{
    _radar_ds->setAmpsOffset(val);
}

void MainWindow::simulation_slot(bool sim)
{
    _radar_ds->simulate(sim);
}
