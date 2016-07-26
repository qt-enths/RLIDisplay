#include "mainwindow.h"

#include <QApplication>
#include <QGLFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QGLFormat f = QGLFormat::defaultFormat();
  f.setDoubleBuffer(false);
  f.setSampleBuffers(false);
  f.setSamples(0);
  QGLFormat::setDefaultFormat(f);

  MainWindow w;
  w.show();
  //w.showMaximized();
  w.showFullScreen();

  return a.exec();
}
