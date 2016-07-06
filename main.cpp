#include "mainwindow.h"

#include <QApplication>
#include <QGLFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);

  QGLFormat f = QGLFormat::defaultFormat();
  f.setDoubleBuffer(false );
  f.setSampleBuffers(true);
  f.setSamples(8);
  QGLFormat::setDefaultFormat(f);

  MainWindow w;
  w.show();
  w.showMaximized();

  return a.exec();
}
