#include "mainwindow.h"

#include <QApplication>
#include <QGLFormat>
#include <QDebug>
#include <QRect>

#include "rliconfig.h"

#define RLI_THREADS_NUM 6 // Required number of threads in global QThreadPool

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QGLFormat f = QGLFormat::defaultFormat();
  f.setDoubleBuffer(true);
  f.setSampleBuffers(true);
  f.setSamples(4);
  QGLFormat::setDefaultFormat(f);

  if (QThreadPool::globalInstance()->maxThreadCount() < RLI_THREADS_NUM)
    QThreadPool::globalInstance()->setMaxThreadCount(RLI_THREADS_NUM);

  RLIConfig config("config.xml");

  /*
  for (QString size : config.getAvailableScreenSizes()) {
    qDebug() << size;
    const RLILayout* layout = config.getLayoutForSize(size);
    layout->print();
  }
  */

  QRect rect = QApplication::desktop()->screenGeometry();
  qDebug() << "Screen rect: " << rect;


  MainWindow w;
  w.show();
  //w.showMaximized();
  w.showFullScreen();

  return a.exec();
}
