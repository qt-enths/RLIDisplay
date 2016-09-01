#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>

#include "rlistrings.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction start";
  ui->setupUi(this);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init start";

  _radar_ds = new RadarDataSource();
  _chart_mngr = new ChartManager();

  ui->wgtRLIDisplay->setChartManager(_chart_mngr);

  QStringList args = qApp->arguments();
  QRegExp     rx("--radar-device");
  int argpos = args.indexOf(rx);

  if((argpos >= 0) && (argpos < args.count() - 1))
      _radar_ds->start(args.at(argpos + 1).toStdString().c_str());
  else
      _radar_ds->start();

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "RadarDS init finish";

  _gain_ctrl = new ValueBarController(RLIStrings::nGain, QPoint(5, 5), 8, 0, this);
  connect(ui->wgtRLIControl, SIGNAL(gainChanged(int)), _gain_ctrl, SLOT(onValueChanged(int)));

  _water_ctrl = new ValueBarController(RLIStrings::nWave, QPoint(5, 5+23+4), 8, 0, this);
  connect(ui->wgtRLIControl, SIGNAL(waterChanged(int)), _water_ctrl, SLOT(onValueChanged(int)));

  _rain_ctrl = new ValueBarController(RLIStrings::nRain, QPoint(5, 5+2*(23+4)), 8, 0, this);
  connect(ui->wgtRLIControl, SIGNAL(rainChanged(int)), _rain_ctrl, SLOT(onValueChanged(int)));

  _apch_ctrl = new ValueBarController(RLIStrings::nAfc, QPoint(5, 5+3*(23+4)), 8, 0, this);
  _rdtn_ctrl = new ValueBarController(RLIStrings::nEmsn, QPoint(5+12*8+60+5, 5), 9, -1, this);

  _curs_ctrl = new CursorController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(cursor_moved(float,float)), _curs_ctrl, SLOT(cursor_moved(float,float)));

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

  connect(ui->wgtRLIDisplay, SIGNAL(initialized()), this, SLOT(onRLIWidgetInitialized()));
  startTimer(33);

  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "MainWindow construction finish";
}

MainWindow::~MainWindow() {
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

void MainWindow::onRLIWidgetInitialized() {
  qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Connect radar to datasource";
  connect(_radar_ds, SIGNAL(updateData(uint,uint,/*float*,*/GLubyte*))
        , ui->wgtRLIDisplay->radarEngine(), SLOT(updateData(uint,uint,/*float*,*/GLubyte*)));


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
  ui->wgtRLIDisplay->update();
}

void MainWindow::on_btnClose_clicked() {
  close();
}
