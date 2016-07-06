#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "MainWindow construction start";
  ui->setupUi(this);

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "RadarDS init start";

  _radar_ds = new RadarDataSource();

  QStringList args = qApp->arguments();
  QRegExp     rx("--radar-device");
  int argpos = args.indexOf(rx);

  if((argpos >= 0) && (argpos < args.count() - 1))
      _radar_ds->start(args.at(argpos + 1).toStdString().c_str());
  else
      _radar_ds->start();

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "RadarDS init finish";

  _gain_ctrl = new GainController(this);
  connect(ui->wgtRLIControl, SIGNAL(gainChanged(int)), _gain_ctrl, SLOT(onGainChanged(int)));

  _curs_ctrl = new CursorController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(cursor_moved(float,float)), _curs_ctrl, SLOT(cursor_moved(float,float)));

  _clck_ctrl = new ClockController(this);
  connect(ui->wgtRLIDisplay, SIGNAL(per_second()), _clck_ctrl, SLOT(second_changed()));

  connect(ui->wgtRLIDisplay, SIGNAL(initialized()), this, SLOT(onRLIWidgetInitialized()));
  startTimer(25);

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "MainWindow construction finish";
}

MainWindow::~MainWindow() {
  delete ui;
  delete _radar_ds;

  delete _gain_ctrl;
  delete _curs_ctrl;
  delete _clck_ctrl;
}

void MainWindow::onRLIWidgetInitialized() {
  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Connect radar to datasource";
  connect(_radar_ds, SIGNAL(updateData(uint,uint,float*,float*))
        , ui->wgtRLIDisplay->radarEngine(), SLOT(updateData(uint,uint,float*,float*)));


  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Setup gain block";
  setupInfoBlock(_gain_ctrl);

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Setup cursor block";
  setupInfoBlock(_curs_ctrl);

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Setup clock block";
  setupInfoBlock(_clck_ctrl);

  qDebug() << QDateTime::currentDateTime().toString("hh:MM:ss zzz") << ": " << "Setup rliwidget as control reciever";
  ui->wgtRLIControl->setReciever(ui->wgtRLIDisplay);
}

void MainWindow::setupInfoBlock(InfoBlockController* ctrl) {
  InfoBlock* blck = ui->wgtRLIDisplay->infoEngine()->addInfoBlock();
  ctrl->setupBlock(blck, ui->wgtRLIDisplay->size());

  connect(ctrl, SIGNAL(setRect(int,QRectF)), blck, SLOT(setRect(int,QRectF)));
  connect(ctrl, SIGNAL(setText(int,QByteArray)), blck, SLOT(setText(int,QByteArray)));
  connect(ui->wgtRLIDisplay->infoEngine(), SIGNAL(send_resize(QSize)), ctrl, SLOT(onResize(QSize)));
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
