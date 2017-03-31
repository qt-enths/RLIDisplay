#include "mainwindow.h"

#include <QApplication>
#include <QGLFormat>

#define RLI_THREADS_NUM 6 // Required number of threads in global QThreadPool

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);

  QGLFormat f = QGLFormat::defaultFormat();
  f.setDoubleBuffer(true);
  f.setSampleBuffers(true);
  f.setSamples(16);
  QGLFormat::setDefaultFormat(f);

  if (QThreadPool::globalInstance()->maxThreadCount() < RLI_THREADS_NUM)
    QThreadPool::globalInstance()->setMaxThreadCount(RLI_THREADS_NUM);

  MainWindow w;
  w.show();
  //w.showMaximized();
  w.showFullScreen();

  return a.exec();
}
