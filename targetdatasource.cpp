#include "targetdatasource.h"

#include <qmath.h>


TargetDataSource::TargetDataSource(QObject *parent) : QObject(parent) {
  _timerId = -1;

  RadarTarget trgt;

  trgt.LAT = 12.4200f;
  trgt.LON = -81.4900f;
  trgt.COG = 37.f;
  trgt.ROT = 10.f;
  trgt.SOG = 80.f;

  _targets.push_back(trgt);

  trgt.LAT = 12.4000f;
  trgt.LON = -81.7500f;
  trgt.COG = 123.f;
  trgt.ROT = 20.f;
  trgt.SOG = 50.f;

  _targets.push_back(trgt);

  trgt.LAT = 12.6000f;
  trgt.LON = -81.7300f;
  trgt.COG = 286.f;
  trgt.ROT = 10.f;
  trgt.SOG = 100.f;

  _targets.push_back(trgt);
}

TargetDataSource::~TargetDataSource() {
  finish();
}

void TargetDataSource::start() {
  if (_timerId != -1)
    return;

  _startTime = QDateTime::currentDateTime();
  _timerId = startTimer(20);
}

void TargetDataSource::finish() {
  if (_timerId == -1)
    return;

  killTimer(_timerId);
  _timerId = -1;
}

const float PI = 3.14159265359f;

void TargetDataSource::timerEvent(QTimerEvent* e) {
  Q_UNUSED(e);

  QDateTime now = QDateTime::currentDateTime();

  for (int i = 0; i < 3; i++) {
    emit updateTarget(QString::number(i+1), _targets[i]);
    _targets[i].LON += 0.0005f * (i+1) * sin(_startTime.msecsTo(now)/(1000.f*(i+1)) + i);
    _targets[i].LAT += 0.0005f * (i+1) * cos(_startTime.msecsTo(now)/(1000.f*(i+1)) + i);
    _targets[i].COG = int(180 * (_startTime.msecsTo(now)/(1000.f*(i+1)) + i) / PI) % 360;
  }
}
