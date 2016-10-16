#ifndef S52CHART_H
#define S52CHART_H

#include <QMap>
#include <QRectF>
#include <QString>

#include <vector>
#include "s52references.h"

class OGRLayer;
class OGRPoint;
class OGRFeature;
class OGRPolygon;
class OGRLineString;


struct S52AreaLayer {
  bool is_pattern_uniform;
  bool is_color_uniform;

  QString pattern_ref;
  //QString color_ref;
  int color_ind;
  // layer i-th area s52 pattern name
  std::vector<QString> pattern_refs;
  // layer i-th area s52 color token
  //std::vector<QString> color_refs;
  std::vector<float> color_inds;
  // layer i-th area triangles start index
  std::vector<int>    start_inds;
  // sequence of coords representing triangulated polygon
  std::vector<float>  triangles;
};

struct S52LineLayer {
  bool is_pattern_uniform;
  bool is_color_uniform;

  QString pattern_ref;
  //QString color_ref;
  int color_ind;
  // layer i-th line s52 pattern name
  std::vector<QString> pattern_refs;
  // layer i-th line s52 color token
  //std::vector<QString> color_refs;
  std::vector<float> color_inds;
  // layer i-th line points start index
  std::vector<int>    start_inds;
  // sequence of coords representing polylines
  std::vector<float>  points;
  // length of the line up to current point
  std::vector<float>  distances;
};

struct S52MarkLayer {
  bool is_symbol_uniform;
  QString symbol_ref;
  // layer i-th point s52 symbol name
  std::vector<QString> symbol_refs;
  // sequence of point coords (lat, lon)
  std::vector<float> points;
};

struct S52TextLayer {
  // text marks
  std::vector<QString> texts;
  // sequence of point coords (lat, lon)
  std::vector<float> points;
};

// Sounding values
struct S52SndgLayer {
  // Depths
  std::vector<double> depths;
  // Sounding value point coords (lat, lon)
  std::vector<float> points;
};


class S52Chart {
public:
  S52Chart(char* file_name, S52References* ref);
  ~S52Chart();

  QList<QString> getAreaLayerNames();
  QList<QString> getLineLayerNames();
  QList<QString> getMarkLayerNames();
  QList<QString> getTextLayerNames();

  S52AreaLayer* getAreaLayer(QString name);
  S52LineLayer* getLineLayer(QString name);
  S52MarkLayer* getMarkLayer(QString name);
  S52TextLayer* getTextLayer(QString name);
  S52SndgLayer* getSndgLayer();

  float getMinLat();
  float getMaxLat();
  float getMinLon();
  float getMaxLon();

private:
  bool isOk;
  S52References* _ref;
  void clear();

  // Chart dimension
  float min_lat, max_lat;
  float min_lon, max_lon;

  // Layered presentation of the chart
  QMap<QString, S52AreaLayer*> area_layers;
  QMap<QString, S52LineLayer*> line_layers;
  QMap<QString, S52MarkLayer*> mark_layers;
  QMap<QString, S52TextLayer*> text_layers;
  S52SndgLayer* sndg_layer;

  // Reads OGRLayer, appends presented layers to one or more layer maps
  bool readLayer(OGRLayer* poLayer);
  bool readSoundingLayer(OGRLayer* poLayer);
  bool readTextLayer(OGRLayer* poLayer);

  // Reading and tesselating OGRPolygon, append result to triangles
  bool readOGRPolygon(OGRPolygon* poGeom, std::vector<float> &points, std::vector<float> &distances, bool contour);
  // Reading OGRLine, append result to points
  bool readOGRLine(OGRLineString* poGeom, std::vector<float> &points, std::vector<float> &distances);

  void fillLineParams(QString& layer_name, S52LineLayer* layer, OGRFeature* poFeature);
  void fillAreaParams(QString& layer_name, S52AreaLayer* layer, OGRFeature* poFeature);
  void fillMarkParams(QString& layer_name, S52MarkLayer* layer, OGRFeature* poFeature);

  QString getAreaColorRef(QString& layer_name, OGRFeature* poFeature);
  QString getAreaPatternRef(QString& layer_name, OGRFeature* poFeature);

  QString getLineColorRef(QString& layer_name, OGRFeature* poFeature);
  QString getLinePatternRef(QString& layer_name, OGRFeature* poFeature);

  QString getMarkSymbolRef(QString& layer_name, OGRFeature* poFeature);

  bool isAreaColorUniform(QString& layer_name);
  bool isAreaPatternUniform(QString& layer_name);

  bool isLineColorUniform(QString& layer_name);
  bool isLinePatternUniform(QString& layer_name);

  bool isMarkSymbolUniform(QString& layer_name);
};

#endif // S52CHART_H
