#include "chartengine.h"

#include <QDateTime>
#include <QXmlStreamReader>

ChartSettingsModel::ChartSettingsModel(const QString& file_path, QObject *parent) : QAbstractTableModel(parent), file(file_path) {
  load();
}

void ChartSettingsModel::load(void) {
  file.open(QFile::ReadOnly);
  layer_order.clear();
  layers_settings.clear();

  QXmlStreamReader* xml = new QXmlStreamReader(&file);
  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "DisplaySoundings")
        display_soundings = (xml->readElementText().trimmed().toLower() == "true");

      if (xml->name() == "Depths")
        readDepths(xml);

      if (xml->name() == "Layers")
        readLayers(xml);

      break;

    default:
      break;
    }
  }

  QList<LayerSettings> settings = layers_settings.values();
  qSort(settings);

  for (int i = 0; i < settings.size(); i++)
    layer_order.push_back(settings[i].name);

  file.close();
}

void ChartSettingsModel::readDepths(QXmlStreamReader* xml) {
  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "Shallow")
        shallow_depth = xml->readElementText().toFloat();

      if (xml->name() == "Safety")
        safety_depth = xml->readElementText().toFloat();

      if (xml->name() == "Deep")
        deep_depth = xml->readElementText().toFloat();

      break;
    case QXmlStreamReader::EndElement:
      if (xml->name() == "Depths")
        return;
      break;
    default:
      break;
    }
  }
}

void ChartSettingsModel::readLayers(QXmlStreamReader* xml) {
  LayerSettings ls;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "Id")
        ls.id = xml->readElementText().toInt();

      if (xml->name() == "Name")
        ls.name = xml->readElementText();

      if (xml->name() == "Descrption")
        ls.desc = xml->readElementText();

      if (xml->name() == "Display")
        ls.visible = (xml->readElementText().trimmed().toLower() == "true");

      if (xml->name() == "Order")
        ls.order = xml->readElementText().toInt();

      break;
    case QXmlStreamReader::EndElement:
      if (xml->name() == "Layer")
        layers_settings.insert(ls.name, ls);

      if (xml->name() == "Layers")
        return;

      break;
    default:
      break;
    }
  }
}

ChartSettingsModel::~ChartSettingsModel(void) {
  save();
}

void ChartSettingsModel::save(void) {
  file.open(QFile::WriteOnly);

  QXmlStreamWriter* xml = new QXmlStreamWriter(&file);
  xml->setAutoFormatting(true);
  xml->writeStartDocument();

  xml->writeStartElement("Settings");

  xml->writeTextElement("DisplaySoundings" , display_soundings ? "True" : "False");

  xml->writeStartElement("Depths");
  xml->writeTextElement("Shallow" , QString::number(shallow_depth));
  xml->writeTextElement("Safety"  , QString::number(safety_depth));
  xml->writeTextElement("Deep"    , QString::number(deep_depth));
  xml->writeEndElement();

  xml->writeStartElement("Layers");

  int order = 1;
  for (int i = 0; i < layer_order.size(); i++) {
    LayerSettings ls = layers_settings[layer_order[i]];
    xml->writeStartElement("Layer");
    xml->writeTextElement("Id"        , QString::number(ls.id));
    xml->writeTextElement("Name"      , ls.name);
    xml->writeTextElement("Descrption", ls.desc);
    xml->writeTextElement("Display"   , ls.visible ? "True" : "False");
    xml->writeTextElement("Order"   , QString::number(order++));
    xml->writeEndElement();
  }
  xml->writeEndElement();

  xml->writeEndElement();

  file.close();
}

QStringList ChartSettingsModel::getLayersDisplayOrder() {
  return layer_order;
}

void ChartSettingsModel::swapOrders(int i, int j) {
  emit layoutAboutToBeChanged();

  if (i >= 0 && j >= 0 && i < layer_order.size() && j < layer_order.size()) {
    QString temp = layer_order[i];
    layer_order[i] = layer_order[j];
    layer_order[j] = temp;
  }

  emit layoutChanged();
}

bool ChartSettingsModel::isLayerVisible(const QString& layer_name) {
  if (layers_settings.contains(layer_name))
    return layers_settings[layer_name].visible;

  return false;
}

void ChartSettingsModel::setLayerVisibility(const QString& layer_name, bool val) {
  layers_settings[layer_name] = LayerSettings(layer_name);
  layers_settings[layer_name].visible = val;
}

float ChartSettingsModel::getShallowDepth(void) {
  return shallow_depth;
}

float ChartSettingsModel::getSafetyDepth(void) {
  return safety_depth;
}

float ChartSettingsModel::getDeepDepth(void) {
  return deep_depth;
}

void ChartSettingsModel::setShallowDepth(float val) {
  shallow_depth = val;
}

void ChartSettingsModel::setSafetyDepth(float val) {
  safety_depth = val;
}

void ChartSettingsModel::setDeepDepth(float val) {
  deep_depth = val;
}

bool ChartSettingsModel::areSoundingsVisible() {
  return display_soundings;
}

void ChartSettingsModel::setSoundingsVisible(bool val) {
  display_soundings = val;
}

int ChartSettingsModel::rowCount(const QModelIndex& /*parent*/) const {
  return layer_order.size();
}

int ChartSettingsModel::columnCount(const QModelIndex& /*parent*/) const {
  return 3;
}

QVariant ChartSettingsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid())
    return QVariant();

  if (index.column() == 0 && role == Qt::DisplayRole)
    return layer_order[index.row()];

  if (index.column() == 1 && role == Qt::CheckStateRole)
    return layers_settings[layer_order[index.row()]].visible ? Qt::Checked : Qt::Unchecked;

  if (index.column() == 2 && role == Qt::DisplayRole)
    return layers_settings[layer_order[index.row()]].desc;

  return QVariant();
}

bool ChartSettingsModel::setData(const QModelIndex &index, const QVariant &value, int role) {
  if (!index.isValid() || index.column() != 1)
    return false;

  //qDebug() << role;

  if (role == Qt::CheckStateRole) {
    layers_settings[layer_order[index.row()]].visible = ((Qt::CheckState)value.toInt() == Qt::Checked);
    return true;
  }

  return false;
}

Qt::ItemFlags ChartSettingsModel::flags(const QModelIndex &index) const {
  switch (index.column()) {
    case 0:
      return Qt::NoItemFlags;
      break;
    case 1:
      return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable;
      break;
    case 2:
      return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
      break;
    default:
      return Qt::NoItemFlags;
      break;
  }
}

QVariant ChartSettingsModel::headerData(int section, Qt::Orientation orientation, int role) const {
  if (role != Qt::DisplayRole)
    return QVariant();

  switch (orientation) {
    case Qt::Vertical:
      return QVariant(section + 1);
      break;
    case Qt::Horizontal:
      switch (section) {
        case 0:
          return QVariant("Name");
          break;
        case 1:
          return QVariant("Visible");
          break;
        case 2:
          return QVariant("Description");
          break;
        default:
          return QVariant();
          break;
      }
      break;
    default:
      return QVariant();
      break;
  }
}


ChartEngine::ChartEngine() {
  sndg_engine = NULL;
  setting_up = false;

  initialized = false;
  _center_shift = QPoint(0, 0);
  _center = QVector2D(0, 0);
  _scale = 10;
  _angle = 0;
  back_color = QColor(127, 127, 127);

  settings = new ChartSettingsModel("res//chart_display_settings.xml");
}

ChartEngine::~ChartEngine() {
  if (sndg_engine != NULL)
    delete sndg_engine;

  QList<QString> layer_names;
  for (int i = 0; i < (layer_names = area_engines.keys()).size(); i++)
    delete area_engines[layer_names[i]];
  for (int i = 0; i < (layer_names = line_engines.keys()).size(); i++)
    delete line_engines[layer_names[i]];
  for (int i = 0; i < (layer_names = mark_engines.keys()).size(); i++)
    delete mark_engines[layer_names[i]];
  for (int i = 0; i < (layer_names = text_engines.keys()).size(); i++)
    delete text_engines[layer_names[i]];

  if (initialized)
    delete _fbo;

  delete shaders;
  delete assets;
}

bool ChartEngine::init(S52References* ref, const QGLContext* context) {
  if (initialized)
    return false;

  initializeGLFunctions(context);
  ctxt = context;

  assets = new S52Assets();
  assets->init(context, ref);

  shaders = new ChartShaders();
  shaders->init(context);

  canvas = QSize(255, 255);
  _fbo = new QGLFramebufferObject(canvas);

  initialized = true;

  draw();

  return true;
}

void ChartEngine::resize(uint radius) {
  canvas = QSize(2*radius+1, 2*radius+1);

  if (!initialized)
      return;

  delete _fbo;
  _fbo = new QGLFramebufferObject(canvas);

  draw();
}

void ChartEngine::setChart(S52Chart* chrt, S52References* ref) {
  if (!initialized)
    return;

  setting_up = true;
  clear();

  back_color = ref->getColor("NODTA");

  setAreaLayers(chrt, ref);
  setLineLayers(chrt, ref);
  setMarkLayers(chrt, ref);
  setTextLayers(chrt, ref);

  setSndgLayer(chrt, ref);
  setting_up = false;

  draw();
}

void ChartEngine::update(QVector2D center, float scale, float angle, QPoint center_shift) {
  bool need_update = (abs(_center.x() - center.x()) > 0.0005
                    || abs(_center.y() - center.y()) > 0.0005
                    || abs(_scale - scale) > 0.005
                    || abs(_angle - angle) > 0.005
                    || abs(_center_shift.x() - center_shift.x()) > 0.005
                    || abs(_center_shift.y() - center_shift.y()) > 0.005);

  if (initialized && !setting_up && need_update) {
    _center = center;
    _scale = scale;
    _angle = angle;
    _center_shift = center_shift;
    draw();
  }
}

void ChartEngine::draw() {
  if (!initialized || setting_up)
      return;

  //qDebug() << QDateTime::currentDateTime().toString("hh:mm:ss zzz") << ": " << "Update chart";

  _fbo->bind();

  glViewport(0, 0, canvas.width(), canvas.height());

  glClearColor(back_color.redF(), back_color.greenF(), back_color.blueF(), 1.f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Push back the current matrices and go orthographic for background rendering.
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, canvas.width(), canvas.height(), 0, -1, 1 );

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(_center_shift.x() + canvas.width()/2.f, _center_shift.y() + canvas.height()/2.f, 0);

  drawLayers();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glFlush();

  _fbo->release();
}


void ChartEngine::clear() {
  if (!initialized)
      return;

  if (sndg_engine != NULL) {
    sndg_engine->clearData();
  }

  QList<QString> layer_names;

  for (int i = 0; i < (layer_names = area_engines.keys()).size(); i++)
    area_engines[layer_names[i]]->clearData();
  for (int i = 0; i < (layer_names = line_engines.keys()).size(); i++)
    line_engines[layer_names[i]]->clearData();
  for (int i = 0; i < (layer_names = mark_engines.keys()).size(); i++)
    mark_engines[layer_names[i]]->clearData();
  for (int i = 0; i < (layer_names = text_engines.keys()).size(); i++)
    text_engines[layer_names[i]]->clearData();
}

void ChartEngine::drawLayers() {
  QStringList displayOrder = settings->getLayersDisplayOrder();
  for (int i = displayOrder.size() - 1; i >= 0; i--)
    if (!settings->isLayerVisible(displayOrder[i]))
      displayOrder.removeAt(i);


  shaders->getChartAreaProgram()->bind();
  for (int i = 0; i < displayOrder.size(); i++) {
    QString s = displayOrder[i];
    if (area_engines.contains(s) && area_engines[s] != NULL)
      area_engines[s]->draw(shaders, _center, _scale, _angle);
  }
  shaders->getChartAreaProgram()->release();


  shaders->getChartLineProgram()->bind();
  for (int i = 0; i < displayOrder.size(); i++) {
    QString s = displayOrder[i];
    if (line_engines.contains(s) && line_engines[s] != NULL)
      line_engines[s]->draw(shaders, _center, _scale, _angle);
  }
  shaders->getChartLineProgram()->release();


  shaders->getChartMarkProgram()->bind();
  for (int i = 0; i < displayOrder.size(); i++) {
    QString s = displayOrder[i];
    if (mark_engines.contains(s) && mark_engines[s] != NULL)
      mark_engines[s]->draw(shaders, _center, _scale, _angle);
  }
  shaders->getChartMarkProgram()->release();

  shaders->getChartTextProgram()->bind();
  for (int i = 0; i < displayOrder.size(); i++) {
    QString s = displayOrder[i];
    if (text_engines.contains(s) && text_engines[s] != NULL)
      text_engines[s]->draw(shaders, _center, _scale, _angle);
  }
  shaders->getChartTextProgram()->release();


  if (sndg_engine != NULL && settings->areSoundingsVisible()) {
    shaders->getChartSndgProgram()->bind();
    sndg_engine->draw(shaders, _center, _scale, _angle);
    shaders->getChartSndgProgram()->release();
  }
}

void ChartEngine::setAreaLayers(S52Chart* chrt, S52References* ref) {
  QList<QString> layer_names = chrt->getAreaLayerNames();

  for (int i = 0; i < layer_names.size(); i++) {
    QString layer_name = layer_names[i];
    S52AreaLayer* layer = chrt->getAreaLayer(layer_name);

    if (!area_engines.contains(layer_name)) {
      area_engines[layer_name] = new ChartAreaEngine();
      area_engines[layer_name]->init(ctxt);
    }

    area_engines[layer_name]->setPatternTexture(assets->getPatternTextureId(ref->getColorScheme())
                                              , assets->getPatternTextureDim(ref->getColorScheme()));
    area_engines[layer_name]->setData(layer, assets, ref);
  }
}

void ChartEngine::setLineLayers(S52Chart* chrt, S52References* ref) {
  QList<QString> layer_names = chrt->getLineLayerNames();

  for (int i = 0; i < layer_names.size(); i++) {
    QString layer_name = layer_names[i];
    S52LineLayer* layer = chrt->getLineLayer(layer_name);

    if (!line_engines.contains(layer_name)) {
      line_engines[layer_name] = new ChartLineEngine();
      line_engines[layer_name]->init(ctxt);
    }

    line_engines[layer_name]->setPatternTexture(assets->getLineTextureId(ref->getColorScheme())
                                              , assets->getLineTextureDim(ref->getColorScheme()));
    line_engines[layer_name]->setData(layer, assets, ref);
  }
}

void ChartEngine::setMarkLayers(S52Chart* chrt, S52References* ref) {
  QList<QString> layer_names = chrt->getMarkLayerNames();

  for (int i = 0; i < layer_names.size(); i++) {
    QString layer_name = layer_names[i];
    S52MarkLayer* layer = chrt->getMarkLayer(layer_name);

    if (!mark_engines.contains(layer_name)) {
      mark_engines[layer_name] = new ChartMarkEngine();
      mark_engines[layer_name]->init(ctxt);
    }

    mark_engines[layer_name]->setPatternTexture(assets->getSymbolTextureId(ref->getColorScheme())
                                              , assets->getSymbolTextureDim(ref->getColorScheme()));
    mark_engines[layer_name]->setData(layer, assets, ref);
  }
}

void ChartEngine::setTextLayers(S52Chart* chrt, S52References* /*ref*/) {
  QList<QString> layer_names = chrt->getTextLayerNames();

  for (int i = 0; i < layer_names.size(); i++) {
    QString layer_name = layer_names[i];
    S52TextLayer* layer = chrt->getTextLayer(layer_name);

    if (!text_engines.contains(layer_name)) {
      text_engines[layer_name] = new ChartTextEngine();
      text_engines[layer_name]->init(ctxt);
    }

    text_engines[layer_name]->setGlyphTexture(assets->getGlyphTextureId());
    text_engines[layer_name]->setData(layer);
  }
}

void ChartEngine::setSndgLayer(S52Chart* chrt, S52References* ref) {
  S52SndgLayer* layer = chrt->getSndgLayer();
  if (layer == NULL)
    return;

  if (sndg_engine == NULL) {
    sndg_engine = new ChartSndgEngine();
    sndg_engine->init(ctxt);
  }

  sndg_engine->setPatternTexture(assets->getSymbolTextureId(ref->getColorScheme())
                               , assets->getSymbolTextureDim(ref->getColorScheme()));
  sndg_engine->setData(layer, assets, ref);
}
