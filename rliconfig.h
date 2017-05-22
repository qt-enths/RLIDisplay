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


class RLIConfig {
public:
  static RLIConfig& Instance() {
      static RLIConfig config("config.xml");
      return config;
  }

  inline bool showButtonPanel() const { return _showButtonPanel; }

  const QList<QString> getAvailableScreenSizes(void) const;
  const RLILayout* getLayoutForSize(const QString& sz) const;
  const QString getSuitableLayoutSize(const QSize& sz) const;

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
  QString _defaultSize;
  bool _showButtonPanel;
};

#endif // RLICONFIG_H
