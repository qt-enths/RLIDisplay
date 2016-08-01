#include "s52references.h"

#include <QFile>
#include <QDebug>
#include <QXmlStreamReader>


S52References::S52References(QString file_name) {
  QFile file(file_name);
  file.open(QFile::ReadOnly);
  QXmlStreamReader* xml = new QXmlStreamReader(&file);

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "color-tables")
        readColorTables(xml);

      if (xml->name() == "lookups")
        readLookUps(xml);

      if (xml->name() == "line-styles")
        readLineStyles(xml);

      if (xml->name() == "patterns")
        readPatterns(xml);

      if (xml->name() == "symbols")
        readSymbols(xml);

      break;

    default:
      break;
    }
  }

  //print();

  file.close();
  fillColorTables2();
}

int S52References::getColorIndex(const QString& color_ref) {
  if (color_indices.contains(color_ref))
    return color_indices[color_ref];

  return -1;
}

std::vector<float> S52References::getColorTable() {
  return color_tables2[color_scheme];
}

void S52References::fillColorTables2() {
  int current_index = 0;
  QList<QString> table_names = color_tables.keys();

  // Loop 1, filling color_indices
  for (int i = 0; i < table_names.size(); i++) {
    QString table_ref = table_names[i];
    QList<QString> color_names = color_tables[table_ref]->colors.keys();

    for (int j = 0; j < color_names.size(); j++)
      if (!color_indices.contains(color_names[j]))
          color_indices.insert(color_names[j], current_index++);
  }

  QList<QString> color_names = color_indices.keys();
  // Loop 2, filling color_tables2
  for (int i = 0; i < table_names.size(); i++) {
    QString table_ref = table_names[i];
    ColorTable* tbl = color_tables[table_ref];
    std::vector<float> color_vec(3*color_indices.size());

    for (int j = 0; j < color_names.size(); j++) {
      int index = color_indices[color_names[j]];

      if (tbl->colors.contains(color_names[j])) {
        color_vec[3*index+0] = tbl->colors[color_names[j]].redF();
        color_vec[3*index+1] = tbl->colors[color_names[j]].greenF();
        color_vec[3*index+2] = tbl->colors[color_names[j]].blueF();
      } else {
        color_vec[3*index+0] = 0;
        color_vec[3*index+1] = 0;
        color_vec[3*index+2] = 0;
      }
    }

    color_tables2.insert(table_ref, color_vec);
  }

  return;
}

const QVector2D S52References::getSymbolIndex(const QString& symbol_ref) {
  if (symbols.contains(symbol_ref))
    return symbols[symbol_ref]->bitmap.graphics_location;
  else
    return QVector2D(0, 0);
}

const QVector2D S52References::getSymbolDim(const QString& symbol_ref) {
  if (symbols.contains(symbol_ref))
    return symbols[symbol_ref]->bitmap.size;
  else
    return QVector2D(0, 0);
}

const QVector2D S52References::getSymbolPivot(const QString& symbol_ref) {
  if (symbols.contains(symbol_ref))
    return symbols[symbol_ref]->bitmap.pivot;
  else
    return QVector2D(0, 0);
}

void S52References::print(void) {
  qDebug() << "";
  qDebug() << "Color Tables";
  qDebug() << "------------------------";
  qDebug() << "";
  for (int i = 0; i < color_tables.keys().size(); i++) {
    QString table_key =  color_tables.keys()[i];
    ColorTable* ct = color_tables[table_key];

    qDebug() << ct->name;
    qDebug() << ct->graphics_file;
    for (int j = 0; j < ct->colors.keys().size(); j++) {
      QString color_key =  ct->colors.keys()[j];
      QColor c = ct->colors[color_key];
      qDebug() << color_key << " (" << c.redF() << ", "
               << c.greenF() << ", " << c.blueF() << ")";
    }

    qDebug() << "";
  }

  qDebug() << "";
  qDebug() << "Lookups";
  qDebug() << "------------------------";
  qDebug() << "";
  for (int i = 0; i < lookups.keys().size(); i++) {
    int table_key =  lookups.keys()[i];
    LookUp* lp = lookups[table_key];

    qDebug() << lp->name;
    qDebug() << "id: " << lp->id;
    qDebug() << "rcid: " << lp->rcid;
    qDebug() << "type: " << lp->type;
    qDebug() << "disp_prio: " << lp->disp_prio;
    qDebug() << "radar_prio: " << lp->radar_prio;
    qDebug() << "table_name: " << lp->table_name;
    qDebug() << "instruction: " << lp->instruction;
    qDebug() << "display_cat: " << lp->display_cat;
    qDebug() << "comment: " << lp->comment;
    qDebug() << "attr_refs: " << lp->attr_refs;
    qDebug() << "";
  }

  qDebug() << "";
  qDebug() << "Line styles";
  qDebug() << "------------------------";
  qDebug() << "";
  for (int i = 0; i < line_styles.keys().size(); i++) {
    int table_key =  line_styles.keys()[i];
    LineStyle* ls = line_styles[table_key];

    qDebug() << ls->name;
    qDebug() << "rcid: " << ls->rcid;
    qDebug() << "description: " << ls->description;
    qDebug() << "color_ref: " << ls->color_ref;
    qDebug() << "vector size: " << ls->vector.size;
    qDebug() << "vector distance: " << ls->vector.distance;
    qDebug() << "vector origin: " << ls->vector.origin;
    qDebug() << "vector pivot: " << ls->vector.pivot;
    qDebug() << "vector hpgl: " << ls->vector.hpgl;

    qDebug() << "";
  }

  qDebug() << "";
  qDebug() << "Patterns";
  qDebug() << "------------------------";
  qDebug() << "";
  for (int i = 0; i < patterns.keys().size(); i++) {
    int table_key =  patterns.keys()[i];
    Pattern* pt = patterns[table_key];

    qDebug() << pt->name;
    qDebug() << "rcid: " << pt->rcid;
    qDebug() << "description: " << pt->description;
    qDebug() << "color_ref: " << pt->color_ref;
    qDebug() << "definition: " << pt->definition;
    qDebug() << "filltype: " << pt->filltype;
    qDebug() << "spacing: " << pt->spacing;
    qDebug() << "vector size: " << pt->vector.size;
    qDebug() << "vector distance: " << pt->vector.distance;
    qDebug() << "vector origin: " << pt->vector.origin;
    qDebug() << "vector pivot: " << pt->vector.pivot;
    qDebug() << "vector hpgl: " << pt->vector.hpgl;

    qDebug() << "";
  }

  qDebug() << "";
  qDebug() << "Symbols";
  qDebug() << "------------------------";
  qDebug() << "";
  for (int i = 0; i < symbols.keys().size(); i++) {
    QString table_key =  symbols.keys()[i];
    Symbol* sl = symbols[table_key];

    qDebug() << sl->name;
    qDebug() << "rcid: " << sl->rcid;
    qDebug() << "description: " << sl->description;
    qDebug() << "color_ref: " << sl->color_ref;
    qDebug() << "definition: " << sl->definition;

    qDebug() << "vector size: " << sl->vector.size;
    qDebug() << "vector distance: " << sl->vector.distance;
    qDebug() << "vector origin: " << sl->vector.origin;
    qDebug() << "vector pivot: " << sl->vector.pivot;
    qDebug() << "vector hpgl: " << sl->vector.hpgl;

    qDebug() << "bitmap size: " << sl->bitmap.size;
    qDebug() << "bitmap distance: " << sl->bitmap.distance;
    qDebug() << "bitmap origin: " << sl->bitmap.origin;
    qDebug() << "bitmap pivot: " << sl->bitmap.pivot;
    qDebug() << "bitmap graphics loc: " << sl->bitmap.graphics_location;

    qDebug() << "";
  }
}


S52References::~S52References() {
  for (int i = 0; i < color_tables.size(); i++)
    delete color_tables[color_tables.keys()[i]];

  for (int i = 0; i < lookups.size(); i++)
    delete lookups[lookups.keys()[i]];

  for (int i = 0; i < line_styles.size(); i++)
    delete line_styles[line_styles.keys()[i]];

  for (int i = 0; i < patterns.size(); i++)
    delete patterns[patterns.keys()[i]];

  for (int i = 0; i < symbols.size(); i++)
    delete symbols[symbols.keys()[i]];
}


void S52References::setColorScheme(const QString& name) {
  if (color_tables.contains(name))
    color_scheme = name;
}

QString S52References::getColorScheme() {
  return color_scheme;
}

QStringList S52References::getColorSchemeNames() {
    return QStringList(color_tables.keys());
}

QString S52References::getGraphicsFileName(const QString& scheme) {
  return color_tables[scheme]->graphics_file;
}


const QColor S52References::getColor(const QString& color_ref) {
  ColorTable* col_tbl = color_tables[color_scheme];

  if (col_tbl->colors.contains(color_ref)) {
    return col_tbl->colors[color_ref];
  } else {
    return QColor(255, 255, 255, 0);
  }
}


void S52References::readColorTables(QXmlStreamReader* xml) {
  ColorTable* ct = NULL;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "color-table") {
        ct = new ColorTable();
        ct->name = xml->attributes().value("name").toString();
        break;
      }

      if (ct != NULL && xml->name() == "graphics-file")
        ct->graphics_file = xml->attributes().value("name").toString();

      if (ct != NULL && xml->name() == "color")
        ct->colors.insert(xml->attributes().value("name").toString(),
                          QColor(xml->attributes().value("r").toString().toInt()
                               , xml->attributes().value("g").toString().toInt()
                               , xml->attributes().value("b").toString().toInt()));

      break;
    case QXmlStreamReader::EndElement:
      if (ct != NULL && xml->name() == "color-table")
        color_tables.insert(ct->name, ct);

      if (xml->name() == "color-tables")
        return;

      break;
    default:
      break;
    }
  }
}


void S52References::readLookUps(QXmlStreamReader* xml) {
  LookUp* lp = NULL;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "lookup") {
        lp = new LookUp();
        lp->id = xml->attributes().value("id").toString().toInt();
        lp->rcid = xml->attributes().value("RCID").toString().toInt();
        lp->name = xml->attributes().value("name").toString();
        break;
      }

      if (lp != NULL && xml->name() == "type")
        lp->type = xml->readElementText();

      if (lp != NULL && xml->name() == "disp-prio")
        lp->disp_prio = xml->readElementText();

      if (lp != NULL && xml->name() == "radar-prio")
        lp->radar_prio = xml->readElementText();

      if (lp != NULL && xml->name() == "table-name")
        lp->table_name = xml->readElementText();

      if (lp != NULL && xml->name() == "attrib-code")
        lp->attr_refs.append(xml->readElementText());

      if (lp != NULL && xml->name() == "instruction")
        lp->instruction = xml->readElementText();

      if (lp != NULL && xml->name() == "display-cat")
        lp->display_cat = xml->readElementText();

      if (lp != NULL && xml->name() == "comment")
        lp->comment = xml->readElementText();

      break;
    case QXmlStreamReader::EndElement:
      if (lp != NULL && xml->name() == "lookup")
        lookups.insert(lp->rcid, lp);

      if (xml->name() == "lookups")
        return;

      break;
    default:
      break;
    }
  }
}


void S52References::readLineStyles(QXmlStreamReader* xml) {
  LineStyle* ls = NULL;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "line-style") {
        ls = new LineStyle();
        ls->rcid = xml->attributes().value("RCID").toString().toInt();
        break;
      }

      if (ls != NULL && xml->name() == "name")
        ls->name = xml->readElementText();

      if (ls != NULL && xml->name() == "description")
        ls->description = xml->readElementText();

      if (ls != NULL && xml->name() == "color-ref")
        ls->color_ref = xml->readElementText();

      if (xml->name() == "vector")
        ls->vector.size = QVector2D(xml->attributes().value("width").toString().toInt()
                          , xml->attributes().value("height").toString().toInt());

      if (xml->name() == "distance")
        ls->vector.distance = QVector2D(xml->attributes().value("min").toString().toInt()
                              , xml->attributes().value("max").toString().toInt());

      if (xml->name() == "pivot")
        ls->vector.pivot = QVector2D(xml->attributes().value("x").toString().toInt()
                           , xml->attributes().value("y").toString().toInt());

      if (xml->name() == "origin")
        ls->vector.origin = QVector2D(xml->attributes().value("x").toString().toInt()
                            , xml->attributes().value("y").toString().toInt());

      if (xml->name() == "HPGL")
        ls->vector.hpgl = xml->readElementText();

      break;
    case QXmlStreamReader::EndElement:
      if (ls != NULL && xml->name() == "line-style")
        line_styles.insert(ls->rcid, ls);

      if (xml->name() == "line-styles")
        return;

      break;
    default:
      break;
    }
  }
}

void S52References::readPatterns(QXmlStreamReader* xml) {
  Pattern* pn = NULL;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "pattern") {
        pn = new Pattern();
        pn->rcid = xml->attributes().value("RCID").toString().toInt();
        break;
      }

      if (pn != NULL && xml->name() == "name")
        pn->name = xml->readElementText();

      if (pn != NULL && xml->name() == "definition")
        pn->definition = xml->readElementText();

      if (pn != NULL && xml->name() == "filltype")
        pn->filltype = xml->readElementText();

      if (pn != NULL && xml->name() == "spacing")
        pn->spacing = xml->readElementText();

      if (pn != NULL && xml->name() == "description")
        pn->description = xml->readElementText();

      if (pn != NULL && xml->name() == "color-ref")
        pn->color_ref = xml->readElementText();

      if (xml->name() == "vector")
        pn->vector.size = QVector2D(xml->attributes().value("width").toString().toInt()
                          , xml->attributes().value("height").toString().toInt());

      if (xml->name() == "distance")
        pn->vector.distance = QVector2D(xml->attributes().value("min").toString().toInt()
                              , xml->attributes().value("max").toString().toInt());

      if (xml->name() == "pivot")
        pn->vector.pivot = QVector2D(xml->attributes().value("x").toString().toInt()
                           , xml->attributes().value("y").toString().toInt());

      if (xml->name() == "origin")
        pn->vector.origin = QVector2D(xml->attributes().value("x").toString().toInt()
                            , xml->attributes().value("y").toString().toInt());

      if (xml->name() == "HPGL")
        pn->vector.hpgl = xml->readElementText();

      break;
    case QXmlStreamReader::EndElement:
      if (pn != NULL && xml->name() == "pattern")
        patterns.insert(pn->rcid, pn);

      if (xml->name() == "patterns")
        return;

      break;
    default:
      break;
    }
  }
}

void S52References::readSymbols(QXmlStreamReader* xml) {
  Symbol* sb = NULL;
  bool vector_part_flag = false;

  while (!xml->atEnd()) {
    switch (xml->readNext()) {
    case QXmlStreamReader::StartElement:
      if (xml->name() == "symbol") {
        sb = new Symbol();
        sb->rcid = xml->attributes().value("RCID").toString().toInt();
        break;
      }

      if (sb != NULL && xml->name() == "name")
        sb->name = xml->readElementText();

      if (sb != NULL && xml->name() == "definition")
        sb->definition = xml->readElementText();

      if (sb != NULL && xml->name() == "description")
        sb->description = xml->readElementText();

      if (sb != NULL && xml->name() == "color-ref")
        sb->color_ref = xml->readElementText();

      if (xml->name() == "vector") {
        sb->vector.size = QVector2D(xml->attributes().value("width").toString().toInt()
                                  , xml->attributes().value("height").toString().toInt());
        vector_part_flag = true;
      }

      if (xml->name() == "bitmap") {
        sb->bitmap.size = QVector2D(xml->attributes().value("width").toString().toInt()
                                    , xml->attributes().value("height").toString().toInt());
        vector_part_flag = false;
      }


      if (xml->name() == "distance") {
        if (vector_part_flag)
          sb->vector.distance = QVector2D(xml->attributes().value("min").toString().toInt()
                                        , xml->attributes().value("max").toString().toInt());
        else
          sb->bitmap.distance = QVector2D(xml->attributes().value("min").toString().toInt()
                                        , xml->attributes().value("max").toString().toInt());
      }
      
      if (xml->name() == "pivot") {
        if (vector_part_flag)
          sb->vector.pivot = QVector2D(xml->attributes().value("x").toString().toInt()
                                     , xml->attributes().value("y").toString().toInt());
        else
          sb->bitmap.pivot = QVector2D(xml->attributes().value("x").toString().toInt()
                                     , xml->attributes().value("y").toString().toInt());
      }

      if (xml->name() == "origin") {
        if (vector_part_flag)
          sb->vector.origin = QVector2D(xml->attributes().value("x").toString().toInt()
                                      , xml->attributes().value("y").toString().toInt());
        else
          sb->bitmap.origin = QVector2D(xml->attributes().value("x").toString().toInt()
                                      , xml->attributes().value("y").toString().toInt());
      }

      if (xml->name() == "HPGL")
        sb->vector.hpgl = xml->readElementText();

      if (xml->name() == "graphics-location")
        sb->bitmap.graphics_location = QVector2D(xml->attributes().value("x").toString().toInt()
                                               , xml->attributes().value("y").toString().toInt());

      break;
    case QXmlStreamReader::EndElement:
      if (sb != NULL && xml->name() == "symbol") {
        sb->bitmap.pivot.setY(sb->bitmap.size.y() - sb->bitmap.pivot.y());
        symbols.insert(sb->name, sb);

      }

      if (xml->name() == "symbols")
        return;

      break;
    default:
      break;
    }
  }
}



