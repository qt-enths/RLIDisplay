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

signals:
  void updateTarget(QString tag, RadarTarget target);

protected slots:
  void timerEvent(QTimerEvent* e);

public slots:
  void start();
  void finish();

private:
  int _timerId;
  QDateTime _startTime;
  QVector<RadarTarget> _targets;
};

#endif // TARGETDATASOURCE_H
