#ifndef TARGETDATASOURCE_H
#define TARGETDATASOURCE_H

#include "../layers/targetengine.h"

#include <QDateTime>
#include <QTimerEvent>


class TargetDataSource : public QObject
{
  Q_OBJECT
public:
  explicit TargetDataSource(QObject *parent = 0);
  virtual ~TargetDataSource();

  enum {
    TAILMODE_FIRST  = 0,
    TAILMODE_OFF    = 0,
    TAILMODE_RADAR  = 1,
    TAILMODE_DOTS   = 2,
    TAILMODE_LAST   = 2
  };

  void incrementMode();

signals:
  void updateTarget(QString tag, RadarTarget target);
  void tailsModeChanged(int mode, int minutes);

protected slots:
  void timerEvent(QTimerEvent* e);

public slots:
  void start();
  void finish();

  void onTailsModeChanged(const QByteArray mode);

private:
  int _timerId;
  QDateTime _startTime;

  int tail_mode;
  int tail_minutes;

  QVector<RadarTarget> _targets;
};

#endif // TARGETDATASOURCE_H
