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
  RLIConfig(const QString& filename);
  ~RLIConfig(void);

  inline bool showButtonPanel() const { return _showButtonPanel; }

  const QList<QString> getAvailableScreenSizes(void) const;
  const RLILayout* getLayoutForSize(const QString& sz) const;
  const QString getSuitableLayoutSize(const QSize& sz) const;

private:
  QMap<QString, QString> readXMLAttributes(QXmlStreamReader* xml);
  RLILayout* readLayout(QXmlStreamReader* xml);
  QMap<QString, QMap<QString, QString>> readPanelLayouts(QXmlStreamReader* xml);

  QMap<QString, RLILayout*> _layouts;
  bool _showButtonPanel;
};

#endif // RLICONFIG_H
