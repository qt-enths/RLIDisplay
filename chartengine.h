#ifndef CHARTENGINE_H
#define CHARTENGINE_H

#include <QVector2D>

#include <QFile>
#include <QDateTime>
#include <QAbstractTableModel>

#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

#include "s52assets.h"
#include "s52chart.h"
#include "s52references.h"

#include "chartlayers.h"
#include "chartshaders.h"


class ChartSettingsModel : public QAbstractTableModel {
    Q_OBJECT
public:
  ChartSettingsModel(const QString& file_path, QObject *parent = 0);
  ~ChartSettingsModel(void);

  int rowCount(const QModelIndex &parent = QModelIndex()) const ;
  int columnCount(const QModelIndex &parent = QModelIndex()) const;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
  Qt::ItemFlags flags(const QModelIndex &index) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
  bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::DisplayRole);

  void load(void);
  void save(void);

  QStringList getLayersDisplayOrder();
  void swapOrders(int i, int j);

  bool isLayerVisible(const QString& layer_name);
  void setLayerVisibility(const QString& layer_name, bool val);

  bool areSoundingsVisible();
  void setSoundingsVisible(bool val);

  float getShallowDepth(void);
  void setShallowDepth(float val);
  float getSafetyDepth(void);
  void setSafetyDepth(float val);
  float getDeepDepth(void);
  void setDeepDepth(float val);

private:
  struct LayerSettings {
    int id;
    QString name;
    QString desc;
    bool visible;
    int order;

    LayerSettings() { id = -1; name = ""; desc = ""; visible = false; order = 1000; }
    LayerSettings(const QString& layer_name) { id = -1; name = layer_name; desc = layer_name; visible = false; order = 1000; }

    bool operator<(const LayerSettings& rhs) const {
      return order < rhs.order;
    }
  };

  void readDepths(QXmlStreamReader* xml);
  void readLayers(QXmlStreamReader* xml);

  QFile file;

  bool display_soundings;
  QMap<QString, LayerSettings> layers_settings;
  QStringList layer_order;

  float shallow_depth;
  float safety_depth;
  float deep_depth;
};




class ChartEngine : protected QGLFunctions {
public:
  explicit ChartEngine();
  virtual ~ChartEngine();

  bool init(S52References* ref, const QGLContext* context);
  void resize(uint radius);

  void setChart(S52Chart* chrt, S52References* ref);
  void update(QVector2D center, float radius, float angle);

  inline GLuint getTextureId() { return _fbo->texture(); }

private:
  bool initialized;
  void clear();

  QDateTime _lastUpdate;
  bool setting_up;

  ChartShaders* shaders;
  ChartSettingsModel* settings;

  QSize canvas;
  QVector2D _center;
  float _radius;
  float _angle;

  QGLFramebufferObject* _fbo;

  S52Assets* assets;
  const QGLContext* ctxt;

  QColor back_color;

  void draw();
  void drawLayers();

  void setAreaLayers(S52Chart* chrt, S52References* ref);
  void setLineLayers(S52Chart* chrt, S52References* ref);
  void setMarkLayers(S52Chart* chrt, S52References* ref);
  void setTextLayers(S52Chart* chrt, S52References* ref);
  void setSndgLayer(S52Chart* chrt, S52References* ref);

  QMap<QString, ChartAreaEngine*>  area_engines;
  QMap<QString, ChartLineEngine*>  line_engines;
  QMap<QString, ChartMarkEngine*>  mark_engines;
  QMap<QString, ChartTextEngine*>  text_engines;
  ChartSndgEngine* sndg_engine;
};

#endif // CHARTENGINE_H
