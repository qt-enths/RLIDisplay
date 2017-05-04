#include "rliconfig.h"

#include <QFile>
#include <QDebug>
#include <QXmlStreamReader>


void RLILayout::print() const {
  qDebug() << "circle";
  for (QString attr : circle.keys())
    qDebug() << "\t" << attr << ": " << circle[attr];

  qDebug() << "menu";
  for (QString attr : menu.keys())
    qDebug() << "\t" << attr << ": " << menu[attr];

  qDebug() << "magnifier";
  for (QString attr : magnifier.keys())
    qDebug() << "\t" << attr << ": " << magnifier[attr];

  qDebug() << "panels";
  for (QString panel_name : panels.keys()) {
    qDebug() << "\t" << panel_name;
    for (QString attr : panels[panel_name].keys())
      qDebug() << "\t\t" << attr << ": " << panels[panel_name][attr];
  }
}


RLIConfig::RLIConfig(const QString& filename) {
  QFile file(filename);
  file.open(QFile::ReadOnly);
  QXmlStreamReader* xml = new QXmlStreamReader(&file);

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "layout") {
        QMap<QString, QString> attrs = readXMLAttributes(xml);
        _layouts.insert(attrs["size"], readLayout(xml));
      }
      break;
    default:
      break;
    }
  }

  file.close();
}

RLIConfig::~RLIConfig() {
  qDeleteAll(_layouts);
}

RLILayout* RLIConfig::readLayout(QXmlStreamReader* xml) {
  RLILayout* layout = new RLILayout;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "rli-circle")
        layout->circle = readXMLAttributes(xml);

      if (xml->name() == "menu")
        layout->menu = readXMLAttributes(xml);

      if (xml->name() == "magnifier")
        layout->magnifier = readXMLAttributes(xml);

      if (xml->name() == "panels")
        layout->panels = readPanelLayouts(xml);

      break;
    case QXmlStreamReader::EndElement:
      if (xml->name() == "layout")
        return layout;

      break;
    default:
      break;
    }
  }

  return layout;
}

QMap<QString, QMap<QString, QString>> RLIConfig::readPanelLayouts(QXmlStreamReader* xml) {
  QMap<QString, QMap<QString, QString>> panels;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      panels.insert(xml->name().toString(), readXMLAttributes(xml));

    case QXmlStreamReader::EndElement:
      if (xml->name() == "panels")
        return panels;

      break;
    default:
      break;
    }
  }

  return panels;
}

QMap<QString, QString> RLIConfig::readXMLAttributes(QXmlStreamReader* xml) {
  QMap<QString, QString> attributes;

  for (QXmlStreamAttribute attr : xml->attributes())
    attributes.insert(attr.name().toString(), attr.value().toString());

  return attributes;
}

const QList<QString> RLIConfig::getAvailableScreenSizes() const {
  return _layouts.keys();
}

const RLILayout* RLIConfig::getLayoutForSize(const QString& size) const {
  if (_layouts.contains(size))
    return _layouts[size];
  else
    return NULL;
}
