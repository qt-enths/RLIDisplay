#include "rliconfig.h"

#include <QFile>
#include <QDebug>
#include <QXmlStreamReader>
#include <QStringList>
#include <QRegExp>

const QRegExp SIZE_RE = QRegExp("^\\d{3,4}x\\d{3,4}$");

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
    for (QString attr : panels[panel_name].params.keys())
      qDebug() << "\t\t" << attr << ": " << panels[panel_name].params[attr];
  }
}


RLIConfig::RLIConfig(const QString& filename) {
  _showButtonPanel = false;

  QFile file(filename);
  file.open(QFile::ReadOnly);
  QXmlStreamReader* xml = new QXmlStreamReader(&file);

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "use-button-pannel")
        _showButtonPanel = (xml->readElementText() == "true");

      if (xml->name() == "default-layout") {
        _defaultSize = xml->readElementText();
        _currentSize = _defaultSize;
      }

      if (xml->name() == "layout") {
        QMap<QString, QString> attrs = readXMLAttributes(xml);

        if (attrs.contains("size") && SIZE_RE.exactMatch(attrs["size"]))
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

QMap<QString, RLIPanelInfo> RLIConfig::readPanelLayouts(QXmlStreamReader* xml) {
  QMap<QString, RLIPanelInfo> panels;
  RLIPanelInfo current_panel;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "panel") {
        current_panel.clear();
        current_panel.params = readXMLAttributes(xml);
      }

      if (xml->name() == "text") {
        auto attrs = readXMLAttributes(xml);
        current_panel.texts.insert(attrs["name"], attrs);
      }

      if (xml->name() == "rect") {
        auto attrs = readXMLAttributes(xml);
        current_panel.rects.insert(attrs["name"], attrs);
      }

      if (xml->name() == "table") {
        auto attrs = readXMLAttributes(xml);
        current_panel.tables.insert(attrs["name"], attrs);
      }

      break;
    case QXmlStreamReader::EndElement:
      if (xml->name() == "panel")
        panels.insert(current_panel.params["name"], current_panel);

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


const QSize RLIConfig::currentSize() const {
  QStringList slsize = _currentSize.split("x");
  return QSize(slsize[0].toInt(), slsize[1].toInt());
}


void RLIConfig::updateCurrentSize(const QSize& screen_size) {
  QString best = "";
  int max_area = 0;

  for (QString ssize : _layouts.keys()) {
    QStringList slsize = ssize.split("x");
    QSize size(slsize[0].toInt(), slsize[1].toInt());

    if (size.width() <= screen_size.width() && size.height() <= screen_size.height()) {
      float area = size.width() * size.height();

      if (area > max_area) {
        max_area = area;
        best = ssize;
      }
    }
  }

  if (best == "")
    _currentSize = _defaultSize;
  else
    _currentSize = best;
}
