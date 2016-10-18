#include "s52chart.h"

#include <QDebug>
#include <ogrsf_frmts.h>

#include "triangulate.h"
#include "rlimath.h"

using namespace RLIMath;

#define EQUAL_EPS 0.00000001


S52Chart::S52Chart(char* file_name, S52References* ref) {
  isOk = false;
  _ref = ref;
  sndg_layer = NULL;
  //qDebug() << file_name;

  // Open OGR data source
  RegisterOGRS57();
  OGRDataSource* poDS = OGRSFDriverRegistrar::Open( file_name, FALSE, NULL );

  // Failed to open chart
  if (poDS == NULL) {
    return;
  }

  // iterate through
  for( int iLayer = 0; iLayer < poDS->GetLayerCount(); iLayer++ ) {
    OGRLayer* poLayer = poDS->GetLayer(iLayer);
    QString layer_name = poLayer->GetName();
    //qDebug() << layer_name;

    if (layer_name == "M_COVR") {
      OGREnvelope oExt;
      if (!(poLayer->GetExtent(&oExt, TRUE) == OGRERR_NONE))
        return;

      min_lat = static_cast<float>(oExt.MinY);
      max_lat = static_cast<float>(oExt.MaxY);
      min_lon = static_cast<float>(oExt.MinX);
      max_lon = static_cast<float>(oExt.MaxX);
    }

    poLayer->ResetReading();
    if (layer_name == "SOUNDG") {
      if (!readSoundingLayer(poLayer))
        return;
     
      continue;
    }

    if (layer_name == "LNDARE"
     || layer_name == "CANALS"
     || layer_name == "LAKARE"
     || layer_name == "LNDRGN"
     || layer_name == "SEAARE") {
      readTextLayer(poLayer);
    }

    poLayer->ResetReading();
    if (!readLayer(poLayer)) {
      //qDebug() << "read failed";
      return;
    }
  }

  isOk = true;
}


S52Chart::~S52Chart() {
  clear();
}

bool S52Chart::readTextLayer(OGRLayer* poLayer) {
  OGRFeature* poFeature = NULL;
  QString name;
  OGRPoint centroid;

  S52TextLayer* layer = new S52TextLayer();

  while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
    QString name = poFeature->GetFieldAsString("OBJNAM");

    if (name.trimmed().length() > 0) {
      poFeature->StealGeometry()->Centroid(&centroid);
      layer->points.push_back(centroid.getY());
      layer->points.push_back(centroid.getX());
      layer->texts.push_back(name.trimmed());
    }

    OGRFeature::DestroyFeature( poFeature );
  }

  text_layers.insert(QString(poLayer->GetName()), layer);

  return true;
}

S52SndgLayer* S52Chart::getSndgLayer() {
  return sndg_layer;
}

bool S52Chart::readSoundingLayer(OGRLayer* poLayer) {
  OGRFeature* poFeature = NULL;

  if (sndg_layer != NULL)
    return false;

  sndg_layer = new S52SndgLayer();

  while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
    for (int i = 0; i < poFeature->GetGeomFieldCount(); i++) {
      OGRGeometry* geom = poFeature->GetGeomFieldRef(i);
      OGRwkbGeometryType geom_type = geom->getGeometryType();

      if (geom_type == wkbMultiPoint25D) {
        OGRMultiPoint* mp = (OGRMultiPoint*) geom;

        for (int j = 0; j < mp->getNumGeometries(); j++) {
          OGRPoint *p = (OGRPoint *) mp->getGeometryRef(j);
          sndg_layer->points.push_back(p->getY());
          sndg_layer->points.push_back(p->getX());
          sndg_layer->depths.push_back(p->getZ());
        }
      }
    }

    OGRFeature::DestroyFeature( poFeature );
  }

  return true;
}

QList<QString> S52Chart::getAreaLayerNames() {
  return area_layers.keys();
}

QList<QString> S52Chart::getLineLayerNames() {
  return line_layers.keys();
}

QList<QString> S52Chart::getMarkLayerNames() {
  return mark_layers.keys();
}

QList<QString> S52Chart::getTextLayerNames() {
  return text_layers.keys();
}

S52AreaLayer* S52Chart::getAreaLayer(QString name) {
  return area_layers[name];
}

S52LineLayer* S52Chart::getLineLayer(QString name) {
  return line_layers[name];
}

S52MarkLayer* S52Chart::getMarkLayer(QString name) {
  return mark_layers[name];
}

S52TextLayer* S52Chart::getTextLayer(QString name) {
  return text_layers[name];
}

float S52Chart::getMinLat() {
  return min_lat;
}

float S52Chart::getMaxLat() {
  return max_lat;
}

float S52Chart::getMinLon() {
  return min_lon;
}

float S52Chart::getMaxLon() {
  return max_lon;
}


void S52Chart::clear() {
  for (int i = 0; i < area_layers.keys().size(); i++)
    delete area_layers[area_layers.keys()[i]];

  for (int i = 0; i < line_layers.keys().size(); i++)
    delete line_layers[line_layers.keys()[i]];

  for (int i = 0; i < mark_layers.keys().size(); i++)
    delete mark_layers[mark_layers.keys()[i]];

  delete sndg_layer;
}

bool S52Chart::readLayer(OGRLayer* poLayer) {
  QString layer_name = QString(poLayer->GetName());
  //qDebug() << layer_name;

  OGRFeature* poFeature = NULL;

  S52AreaLayer* area_layer;
  S52LineLayer* line_layer;
  S52MarkLayer* mark_layer;

  if (area_layers.contains(layer_name))
    area_layer = area_layers[layer_name];
  else {
    area_layer = new S52AreaLayer();

    area_layer->is_color_uniform = isAreaColorUniform(layer_name);
    area_layer->is_pattern_uniform = isAreaPatternUniform(layer_name);
    area_layer->pattern_ref = "-";
    area_layer->color_ind = -1;
  }

  if (line_layers.contains(layer_name))
    line_layer = line_layers[layer_name];
  else {
    line_layer = new S52LineLayer();

    line_layer->is_color_uniform = isLineColorUniform(layer_name);
    line_layer->is_pattern_uniform = isLinePatternUniform(layer_name);
    line_layer->pattern_ref = "-";
    line_layer->color_ind = -1;
  }

  if (mark_layers.contains(layer_name))
    mark_layer = mark_layers[layer_name];
  else {
    mark_layer = new S52MarkLayer();

    mark_layer->is_symbol_uniform = isMarkSymbolUniform(layer_name);
    mark_layer->symbol_ref = "-";
  }

  while( (poFeature = poLayer->GetNextFeature()) != NULL ) {
    for (int i = 0; i < poFeature->GetGeomFieldCount(); i++) {
      OGRGeometry* geom = poFeature->GetGeomFieldRef(i);
      OGRwkbGeometryType geom_type = geom->getGeometryType();

      if (geom_type == wkbPolygon) {
        fillLineParams(layer_name, line_layer, poFeature);
        line_layer->start_inds.push_back(line_layer->points.size());

        if (!readOGRPolygon((OGRPolygon*) geom, line_layer->points, line_layer->distances, true))
          return false;

        fillAreaParams(layer_name, area_layer, poFeature);
        area_layer->start_inds.push_back(area_layer->triangles.size());

        if (!readOGRPolygon((OGRPolygon*) geom, area_layer->triangles, area_layer->triangles, false))
          return false;
      }

      if (geom_type == wkbLineString) {
        fillLineParams(layer_name, line_layer, poFeature);
        line_layer->start_inds.push_back(line_layer->points.size());

        if (!readOGRLine((OGRLineString*) geom, line_layer->points, line_layer->distances))
          return false;
      }

      if (geom_type == wkbPoint) {
        fillMarkParams(layer_name, mark_layer, poFeature);
        OGRPoint* p = (OGRPoint*) geom;
        mark_layer->points.push_back(p->getY());
        mark_layer->points.push_back(p->getX());
      }
    }

    OGRFeature::DestroyFeature( poFeature );
  }

  if (area_layer->triangles.size() > 0)
    area_layers[layer_name] = area_layer;

  if (line_layer->points.size() > 0)
    line_layers[layer_name] = line_layer;

  if (mark_layer->points.size() > 0)
    mark_layers[layer_name] = mark_layer;

  return true;
}

void S52Chart::fillLineParams(QString& layer_name, S52LineLayer* layer, OGRFeature* poFeature) {
  if (layer->is_pattern_uniform && layer->pattern_ref == "-")
    layer->pattern_ref = getLinePatternRef(layer_name, NULL);
  else if (!layer->is_pattern_uniform)
    layer->pattern_refs.push_back(getLineColorRef(layer_name, poFeature));

  if (layer->is_color_uniform && layer->color_ind == -1)
    layer->color_ind = _ref->getColorIndex(getAreaColorRef(layer_name, NULL));
  else if (!layer->is_color_uniform)
    layer->color_inds.push_back(_ref->getColorIndex(getAreaColorRef(layer_name, poFeature)));
}

void S52Chart::fillAreaParams(QString& layer_name, S52AreaLayer* layer, OGRFeature* poFeature) {
  if (layer->is_pattern_uniform && layer->pattern_ref == "-")
    layer->pattern_ref = getAreaPatternRef(layer_name, NULL);
  else if (!layer->is_pattern_uniform)
    layer->pattern_refs.push_back(getAreaColorRef(layer_name, poFeature));

  if (layer->is_color_uniform && layer->color_ind == -1)
    layer->color_ind = _ref->getColorIndex(getAreaColorRef(layer_name, NULL));
  else if (!layer->is_color_uniform)
    layer->color_inds.push_back(_ref->getColorIndex(getAreaColorRef(layer_name, poFeature)));
}

void S52Chart::fillMarkParams(QString& layer_name, S52MarkLayer* layer, OGRFeature* poFeature) {
  if (layer->is_symbol_uniform && layer->symbol_ref == "-")
    layer->symbol_ref = getMarkSymbolRef(layer_name, NULL);
  else if (!layer->is_symbol_uniform)
    layer->symbol_refs.push_back(getMarkSymbolRef(layer_name, poFeature));
}

// !!!!
// TODO: add hole support
bool S52Chart::readOGRPolygon(OGRPolygon* poGeom, std::vector<float> &points, std::vector<float> &distances, bool contour) {
  if (contour) {
    return readOGRLine((OGRLineString*)poGeom->getExteriorRing(), points, distances);
  }

  // Make a quick sanity check of the polygon coherence
  // ----------------------------------------------------------------
  bool b_ok = true;
  OGRLineString *tls = poGeom->getExteriorRing();

  if(!tls) {
    b_ok = false;
  }
  else {
    int tnpta  = poGeom->getExteriorRing()->getNumPoints();
    if(tnpta < 3)
      b_ok = false;
  }

  for( int iir = 0; iir < poGeom->getNumInteriorRings(); iir++) {
    int tnptr = poGeom->getInteriorRing(iir)->getNumPoints();
    if(tnptr < 3)
      b_ok = false;
  }

  if( !b_ok )
    return false;
  // ----------------------------------------------------------------

  int iir, ip;

  // PolyGeo BBox
  OGREnvelope Envelope;
  poGeom->getEnvelope(&Envelope);
  // xmin = Envelope.MinX;
  // ymin = Envelope.MinY;
  // xmax = Envelope.MaxX;
  // ymax = Envelope.MaxY;


  // Get total number of contours
  int m_ncnt = 1;                             // always exterior ring
  int nint = poGeom->getNumInteriorRings();   // interior rings
  m_ncnt += nint;

  // Allocate cntr array
  int *cntr = (int *)malloc(m_ncnt * sizeof(int));

  // Get total number of points(vertices)
  int npta  = poGeom->getExteriorRing()->getNumPoints();
  npta += 2;                                  // fluff

  for( iir=0 ; iir < nint ; iir++) {
    int nptr = poGeom->getInteriorRing(iir)->getNumPoints();
    npta += nptr + 2;
  }

  point_t *geoPt = (point_t*)calloc((npta + 1) * sizeof(point_t), 1);     // vertex array

  // Create input structures

  // Exterior Ring
  int npte  = poGeom->getExteriorRing()->getNumPoints();
  cntr[0] = npte;

  point_t *ppt = geoPt;
  ppt->x = 0.; ppt->y = 0.;
  ppt++;                            // vertex 0 is unused

  //  Check and account for winding direction of ring
  bool cw = !(poGeom->getExteriorRing()->isClockwise() == 0);

  double x0, y0, x, y;
  OGRPoint p;

  if(cw) {
    poGeom->getExteriorRing()->getPoint(0, &p);
    x0 = p.getX();
    y0 = p.getY();
  } else {
    poGeom->getExteriorRing()->getPoint(npte-1, &p);
    x0 = p.getX();
    y0 = p.getY();
  }

  // Transcribe points to vertex array, in proper order with no duplicates
  for(ip = 0; ip < npte; ip++) {
    int pidx;
    if(cw)
      pidx = npte - ip - 1;
    else
      pidx = ip;

    poGeom->getExteriorRing()->getPoint(pidx, &p);
    x = p.getX();
    y = p.getY();

    if((fabs(x - x0) > EQUAL_EPS) || (fabs(y - y0) > EQUAL_EPS)) {
      ppt->x = x;
      ppt->y = y;
      ppt++;
    } else
      cntr[0]--;

    x0 = x;
    y0 = y;
  }

  // Now the interior contours
  for(iir = 0; iir < nint; iir++) {
    int npti = poGeom->getInteriorRing(iir)->getNumPoints();
    cntr[iir + 1] = npti;

    // Check and account for winding direction of ring
    bool cw = !(poGeom->getInteriorRing(iir)->isClockwise() == 0);
    if(!cw) {
      poGeom->getInteriorRing(iir)->getPoint(0, &p);
      x0 = p.getX();
      y0 = p.getY();
    } else {
      poGeom->getInteriorRing(iir)->getPoint(npti-1, &p);
      x0 = p.getX();
      y0 = p.getY();
    }

    // Transcribe points to vertex array, in proper order with no duplicates
    for(int ip = 0 ; ip < npti ; ip++) {
      OGRPoint p;
      int pidx;

      if(!cw)                               // interior contours must be cw
        pidx = npti - ip - 1;
      else
        pidx = ip;

      poGeom->getInteriorRing(iir)->getPoint(pidx, &p);
      x = p.getX();
      y = p.getY();

      if((fabs(x - x0) > EQUAL_EPS) || (fabs(y - y0) > EQUAL_EPS)) {
        ppt->x = x;
        ppt->y = y;
        ppt++;
      } else
        cntr[iir+1]--;

      x0 = x;
      y0 = y;
    }
  }

  polyout* polys = triangulate_polygon(m_ncnt, cntr, (double (*)[2])geoPt);
  polyout* pck = polys;

  while(pck != NULL) {
    if(pck->is_valid) {
      int *ivs = pck->vertex_index_list;

      if (pck->nvert != 3)
        continue;

      for(int i = 0 ; i < 3 ; i++) {
        int ivp = ivs[i];
        points.push_back(geoPt[ivp].y);
        points.push_back(geoPt[ivp].x);
      }
    }

    pck = (polyout *)pck->poly_next;
  }

  if (polys != NULL)
    delete polys;
  return true;
}

bool S52Chart::readOGRLine(OGRLineString* poGeom, std::vector<float> &points, std::vector<float> &distances) {
  int point_count = poGeom->getNumPoints();

  if(point_count < 2)
    return false;

  OGRPoint p;
  for( int i = 0; i < point_count; i++ ) {
    poGeom->getPoint(i, &p);

    points.push_back(static_cast<float>(p.getY()));
    points.push_back(static_cast<float>(p.getX()));

    if (i != 0) {
      distances.push_back(geo_distance(points[points.size() - 4], points[points.size() - 3]
                                      ,points[points.size() - 2], points[points.size() - 1]));
    }
    else {
      distances.push_back(0);
    }
  }

  return true;
}

QString S52Chart::getAreaColorRef(QString& layer_name, OGRFeature* poFeature) {
  if (layer_name == "LNDARE"
   || layer_name == "LNDRGN"
   || layer_name == "SLCONS")
    return "LANDA";

  if (layer_name == "M_COVR")
    return "NODTA";

  if (layer_name == "FAIRWY")
    return "DEPMS";

  if (layer_name == "RIVERS" ||
      layer_name == "LAKARE")
    return "DEPVS";

  if (layer_name == "DEPARE") {
    // TODO: avoid hardcode
    // shallow_contour_val
    double shc = 2.0;
    // safety_contour_val
    double sfc = 20.0;
    // deep_contour_val
    double dpc = 30.0;

    QString pattern = "DEPIT";

    if (poFeature == NULL)
      return pattern;

    double dep_range_min = poFeature->GetFieldAsDouble("DRVAL1");
    double dep_range_max = poFeature->GetFieldAsDouble("DRVAL2");

    if (dep_range_min >= 0 && dep_range_max > 0)
      pattern = "DEPVS";
    if (dep_range_min >= shc && dep_range_max > shc)
      pattern = "DEPMS";
    if (dep_range_min >= sfc && dep_range_max > sfc)
      pattern = "DEPMD";
    if (dep_range_min >= dpc && dep_range_max > dpc)
      pattern = "DEPDW";

    return pattern;
  }

  return "CHBLK";
}

QString S52Chart::getAreaPatternRef(QString& layer_name, OGRFeature* /*poFeature*/) {
  if (layer_name == "LNDARE" ||
      layer_name == "DEPARE" ||
      layer_name == "RIVERS" ||
      layer_name == "LAKARE")
    return "SOLID";

  if (layer_name == "BUAARE") {
    return "RCKLDG01";
  }

  //Default
  return "SOLID";
}


QString S52Chart::getLineColorRef(QString& layer_name, OGRFeature* /*poFeature*/) {
  if (layer_name == "DEPARE" || layer_name == "DEPCNT")
    return "DEPSC";

  if (layer_name == "RIVERS")
    return "CHBLK";

  if (layer_name == "WATTUR")
    return "LITRD";

  //Default
  return "CHBLK";
}

QString S52Chart::getLinePatternRef(QString& layer_name, OGRFeature* /*poFeature*/) {
  if (layer_name == "RIVERS"
   || layer_name == "LAKARE")
    return "DOTTED";

  if (layer_name == "FAIRWY")
    return "PLNRTE03";

  if (layer_name == "DEPCNT" ||
      layer_name == "DEPARE" ||
      layer_name == "WATTUR")
    return "SOLID";

  //Default
  return "SOLID";
}

QString S52Chart::getMarkSymbolRef(QString& layer_name, OGRFeature* /*poFeature*/) {
  if (layer_name == "WATTUR")
    return "WATTUR02";

  if (layer_name == "WRECKS")
    return "WRECKS05";

  if (layer_name == "BOYINB")
    return "BOYINB01";

  if (layer_name == "RDOSTA")
    return "RDOSTA02";

  if (layer_name == "LIGHTS")
    return "LIGHTS94";

   if (layer_name == "LNDMRK")
     return "MSTCON14";

   if (layer_name == "UWTROC")
     return "UWTROC03";

   if (layer_name == "OBSTRN")
     return "ISODGR51";

  return "QUESMRK1";
}

bool S52Chart::isAreaColorUniform(QString& layer_name) {
  if (layer_name == "DEPARE")
      return false;

  //Default
  return true;
}

bool S52Chart::isLineColorUniform(QString& /*layer_name*/) {
  //Default
  return true;
}

bool S52Chart::isAreaPatternUniform(QString& /*layer_name*/) {
  //Default
  return true;
}

bool S52Chart::isLinePatternUniform(QString& /*layer_name*/) {
  //Default
  return true;
}

bool S52Chart::isMarkSymbolUniform(QString& /*layer_name*/) {
  //Default
  return true;
}

