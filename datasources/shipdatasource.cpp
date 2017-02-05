#include "shipdatasource.h"

#include <qmath.h>

ShipDataSource::ShipDataSource(QObject *parent) : QObject(parent) {
  _timerId = -1;
}

ShipDataSource::~ShipDataSource() {
  finish();
}

void ShipDataSource::timerEvent(QTimerEvent* e) {
  Q_UNUSED(e);

  QDateTime now = QDateTime::currentDateTime();

  float lon =  -81.6000f + 0.2f * sin(_startTime.msecsTo(now)/400000.f);
  float lat =  12.5000f + 0.2f * cos(_startTime.msecsTo(now)/400000.f);
  QVector2D coords(lat, lon);

  emit coordsUpdated(coords);
}

void ShipDataSource::start() {
  if (_timerId != -1)
    return;

  _startTime = QDateTime::currentDateTime();
  _timerId = startTimer(500);

}

void ShipDataSource::finish() {
  if (_timerId == -1)
    return;

  killTimer(_timerId);
  _timerId = -1;

}
