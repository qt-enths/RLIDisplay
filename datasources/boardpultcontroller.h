#ifndef BOARDPULTCONTROLLER_H
#define BOARDPULTCONTROLLER_H

#include <QObject>

#if QT_VERSION >= 0x050000
    #include <QtConcurrent/QtConcurrentRun>
#else
    #include <QtConcurrentRun>
#endif

class BoardPultController : public QObject {
  Q_OBJECT
public:
  explicit BoardPultController(QObject *parent = 0);
  ~BoardPultController();

signals:
  void gain_changed(int value);
  void rain_changed(int value);
  void wave_changed(int value);

public slots:

private:
#ifndef Q_OS_WIN
  QFuture<void> evdevThread;
  int evdevFd;
  int stopEvdev;

  int setupEvdev(char *evdevfn);
  void evdevHandleThread();
#endif
};

#endif // BOARDPULTCONTROLLER_H
