#ifndef RLICONFIG_H
#define RLICONFIG_H

#include <QString>
#include <QSize>
#include <QMap>

class QXmlStreamReader;

struct RLIPanelTableInfo {
  QMap<QString, QString> params;
  QMap<QString, QMap<QString, QString>> columns;

  inline void clear() {
    params.clear();
    columns.clear();
  }
};

struct RLIPanelInfo {
  QMap<QString, QString> params;
  QMap<QString, QMap<QString, QString>> texts;
  QMap<QString, QMap<QString, QString>> rects;
  QMap<QString, RLIPanelTableInfo> tables;

  inline void clear() {
    params.clear();
    texts.clear();
    rects.clear();
    tables.clear();
  }
};

struct RLILayout {
  QMap<QString, QString> circle;
  QMap<QString, QString> menu;
  QMap<QString, QString> magnifier;
  QMap<QString, RLIPanelInfo> panels;

  void print() const;
};


#include <QDebug>

class RLIConfig {
public:
  static RLIConfig& instance() {
    static RLIConfig config("config.xml");    
    return config;
  }

  inline bool showButtonPanel() const { return _showButtonPanel; }

  inline const RLILayout* currentLayout() const { return _layouts[_currentSize]; }
  const QSize currentSize(void) const;
  void updateCurrentSize(const QSize& screen_size);

private:
  RLIConfig(const QString& filename);
  ~RLIConfig(void);

  // Singleton
  RLIConfig(RLIConfig const&) = delete;
  RLIConfig& operator= (RLIConfig const&) = delete;

  QMap<QString, QString> readXMLAttributes(QXmlStreamReader* xml);
  RLILayout* readLayout(QXmlStreamReader* xml);
  RLIPanelTableInfo readTableInfo(QXmlStreamReader* xml);
  QMap<QString, RLIPanelInfo> readPanelLayouts(QXmlStreamReader* xml);

  QMap<QString, RLILayout*> _layouts;
  QString _currentSize;
  QString _defaultSize;
  bool _showButtonPanel;
};

#endif // RLICONFIG_H
