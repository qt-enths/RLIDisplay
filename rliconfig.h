#ifndef RLICONFIG_H
#define RLICONFIG_H

#include <QString>
#include <QSize>
#include <QMap>

class QXmlStreamReader;

struct RLILayout {
  QMap<QString, QString> circle;
  QMap<QString, QString> menu;
  QMap<QString, QString> magnifier;

  QMap<QString, QMap<QString, QString>> panels;

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
  QMap<QString, QMap<QString, QString>> readPanelLayouts(QXmlStreamReader* xml);

  QMap<QString, RLILayout*> _layouts;
  QString _currentSize;
  QString _defaultSize;
  bool _showButtonPanel;
};

#endif // RLICONFIG_H
