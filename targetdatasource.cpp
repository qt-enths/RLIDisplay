#include "targetdatasource.h"

#include <qmath.h>


TargetDataSource::TargetDataSource(QObject *parent) : QObject(parent) {
  _timerId = -1;

  RadarTarget trgt;

  trgt.Lost = false;
  trgt.Latitude = 12.4200f;
  trgt.Longtitude = -81.4900f;
  trgt.Heading = 37.f;
  trgt.Rotation = 20.f;
  trgt.CourseOverGround = 37.f;
  trgt.SpeedOverGround = 180.f;

  _targets.push_back(trgt);

  trgt.Latitude = 12.4000f;
  trgt.Longtitude = -81.7500f;
  trgt.Heading = 123.f;
  trgt.Rotation = -20.f;
  trgt.CourseOverGround = 123.f;
  trgt.SpeedOverGround = 80.f;

  _targets.push_back(trgt);

  trgt.Latitude = 12.6000f;
  trgt.Longtitude = -81.7300f;
  trgt.Heading = 286.f;
  trgt.Rotation = 0.f;
  trgt.CourseOverGround = 286.f;
  trgt.SpeedOverGround = 140.f;

  _targets.push_back(trgt);

  trgt.Latitude = 12.3000f;
  trgt.Longtitude = -81.7300f;
  trgt.Heading = -1.f;
  trgt.Rotation = 0.f;
  trgt.CourseOverGround = 286.f;
  trgt.SpeedOverGround = 140.f;

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

  for (int i = 0; i < _targets.size(); i++) {
    emit updateTarget(QString::number(i+1), _targets[i]);
    _targets[i].Longtitude += 0.0005f * (i+1) * sin(_startTime.msecsTo(now)/(1000.f*(i+1)) + i);
    _targets[i].Latitude += 0.0005f * (i+1) * cos(_startTime.msecsTo(now)/(1000.f*(i+1)) + i);
    _targets[i].CourseOverGround = int(360 + 180 * (_startTime.msecsTo(now)/(1000.f*(i+1)) + i) / PI) % 360;
    if (_targets[i].Heading != -1)
      _targets[i].Heading = int(360 + _targets[i].CourseOverGround - _targets[i].Rotation) % 360;
  }
}
