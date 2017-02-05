#ifndef SHIPDATASOURCE_H
#define SHIPDATASOURCE_H

#include <QVector2D>
#include <QDateTime>
#include <QTimerEvent>

class ShipDataSource : public QObject
{
  Q_OBJECT
public:
  explicit ShipDataSource(QObject *parent = 0);
  virtual ~ShipDataSource();

signals:
  void coordsUpdated(QVector2D coords);

protected slots:
  void timerEvent(QTimerEvent* e);

public slots:
  void start();
  void finish();

private:
  int _timerId;
  QDateTime _startTime;
};

#endif // SHIPDATASOURCE_H
