#include "s52assets.h"

#include <QDebug>

#include <QDir>
#include <QGLWidget>
#include <QStringList>

#include <QFontDatabase>
#include <QTextCodec>
#include <QPixmap>
#include <QPainter>
#include <QRectF>


S52Assets::S52Assets() : _initialized(false) {
}

S52Assets::~S52Assets() {
  glDeleteTextures(1, &glyph_tex_id);

  for (int i = 0; i < pattern_tex_ids.keys().size(); i++)
    glDeleteTextures(1, &pattern_tex_ids[pattern_tex_ids.keys()[i]]);

  for (int i = 0; i < line_tex_ids.keys().size(); i++)
    glDeleteTextures(1, &line_tex_ids[line_tex_ids.keys()[i]]);

  for (int i = 0; i < symbol_tex_ids.keys().size(); i++)
    glDeleteTextures(1, &symbol_tex_ids[symbol_tex_ids.keys()[i]]);
}

void S52Assets::init(const QGLContext* context, S52References* ref) {
  initializeGLFunctions(context);

  initGlyphTexture();
  initPatternTextures(ref);
  initLineTextures(ref);
  initSymbolTextures(ref);
}

void S52Assets::initGlyphTexture() {
  if (glyph_tex_id == 0)
    glGenTextures(1, &glyph_tex_id);

  glBindTexture(GL_TEXTURE_2D, glyph_tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  // 32x32 pix for every glyph, 256=16x16 glyphs =>
  // overall texture should by (32x16)x(32x16) = 512x512
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 32*16, 32*16, 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);

  QImage img;
  QPixmap pixmap(32, 32);
  QPainter painter(&pixmap);
  painter.setRenderHint(QPainter::TextAntialiasing);
  QRectF bound(0.f, 0.f, 32.f, 32.f);

  int id = QFontDatabase::addApplicationFont("res//fonts//Helvetica.ttf");
  QString family = QFontDatabase::applicationFontFamilies(id).at(0);
  painter.setFont(QFont(family, 14, 14));

  painter.setPen(Qt::black);
  painter.setBrush(Qt::black);

  char char_table[256];
  QString uchars;

  QTextDecoder* dec = QTextCodec::codecForName("cp1251")->makeDecoder();
  for (int i = 0; i < 256; i++)
    char_table[i] = static_cast<char>(i);
  uchars = dec->toUnicode(char_table, 256);

  for (int i = 0; i < 256; i++) {
    pixmap.fill(Qt::white);
    painter.drawText(bound, Qt::AlignCenter, uchars.at(i));
    img = QGLWidget::convertToGLFormat(pixmap.toImage());

    int xoffset = i % 16;
    int yoffset = i / 16;

    glTexSubImage2D(GL_TEXTURE_2D, 0, 32*xoffset, 32*yoffset, 32, 32, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
  }

  glGenerateMipmap(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, 0);
}



void S52Assets::initPatternTextures(S52References* ref) {
  QStringList color_schemes = ref->getColorSchemeNames();

  GLuint* tex_ids = new GLuint[color_schemes.size()];
  glGenTextures(color_schemes.size(), tex_ids);

  for (int k = 0; k < color_schemes.size(); k++) {
    pattern_tex_ids.insert(color_schemes[k], tex_ids[k]);

    glBindTexture(GL_TEXTURE_2D, tex_ids[k]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    QDir simple_tex_dir("res//textures//patterns//simple");
    simple_tex_dir.setNameFilters(QStringList() << "*.png");
    QStringList simple_tex_list = simple_tex_dir.entryList();

    QDir color_tex_dir("res//textures//patterns//" + color_schemes[k].toLower());
    color_tex_dir.setNameFilters(QStringList() << "*.png");
    QStringList color_tex_list = color_tex_dir.entryList();

    int pattern_tex_count = simple_tex_list.size() + color_tex_list.size();

    // Templates are 128x128
    int cols = 8;
    int rows = (pattern_tex_count - 1) / cols + 1;

    QVector2D pattern_tex_dim;
    pattern_tex_dim.setX(128*cols);
    pattern_tex_dim.setY(128*rows);
    pattern_tex_dims.insert(color_schemes[k], pattern_tex_dim);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, pattern_tex_dim.x(), pattern_tex_dim.y()
               , 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);

    QString tex_file_name;
    QImage img_tex;
    QMap<QString, QVector2D> pattern_ind_map;
    QMap<QString, QVector2D> pattern_dim_map;

    for (int i = 0; i < pattern_tex_count; i++) {
      int tex_row = i / cols;
      int tex_col = i % cols;

      if (i < simple_tex_list.size()) {
        tex_file_name = simple_tex_list[i];
        img_tex = QImage(simple_tex_dir.filePath(tex_file_name));
      } else {
        tex_file_name = color_tex_list[i - simple_tex_list.size()];
        img_tex = QImage(color_tex_dir.filePath(tex_file_name));
      }

      img_tex = QGLWidget::convertToGLFormat(img_tex);
      pattern_ind_map.insert(tex_file_name.replace(".png", ""), QVector2D(128*tex_col, 128*tex_row));
      pattern_dim_map.insert(tex_file_name.replace(".png", ""), QVector2D(img_tex.width(), img_tex.height()));

      glTexSubImage2D(GL_TEXTURE_2D, 0, tex_col*128, tex_row*128, img_tex.width(), img_tex.height()
                    , GL_RGBA, GL_UNSIGNED_BYTE, img_tex.bits());
    }
    pattern_ind_maps.insert(color_schemes[k], pattern_ind_map);
    pattern_dim_maps.insert(color_schemes[k], pattern_dim_map);

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}

void S52Assets::initLineTextures(S52References* ref) {
  QStringList color_schemes = ref->getColorSchemeNames();

  GLuint* tex_ids = new GLuint[color_schemes.size()];
  glGenTextures(color_schemes.size(), tex_ids);

  for (int k = 0; k < color_schemes.size(); k++) {
    line_tex_ids.insert(color_schemes[k], tex_ids[k]);

    glBindTexture(GL_TEXTURE_2D, tex_ids[k]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    QDir simple_tex_dir("res//textures//lines//simple");
    simple_tex_dir.setNameFilters(QStringList() << "*.png");
    QStringList simple_tex_list = simple_tex_dir.entryList();

    QDir color_tex_dir("res//textures//lines//" + color_schemes[k].toLower());
    color_tex_dir.setNameFilters(QStringList() << "*.png");
    QStringList color_tex_list = color_tex_dir.entryList();

    int line_tex_count = simple_tex_list.size() + color_tex_list.size();

    QVector2D line_tex_dim;
    line_tex_dim.setX(128);
    line_tex_dim.setY(32*line_tex_count);
    line_tex_dims.insert(color_schemes[k], line_tex_dim);


    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, line_tex_dim.x(), line_tex_dim.y()
               , 0, GL_RGBA, GL_UNSIGNED_BYTE, (void*)0);

    QString tex_file_name;
    QImage img_tex;
    QMap<QString, QVector2D> line_ind_map;
    QMap<QString, QVector2D> line_dim_map;

    for (int i = 0; i < line_tex_count; i++) {
      if (i < simple_tex_list.size()) {
        tex_file_name = simple_tex_list[i];
        img_tex = QImage(simple_tex_dir.filePath(tex_file_name));
      } else {
        tex_file_name = color_tex_list[i - simple_tex_list.size()];
        img_tex = QImage(color_tex_dir.filePath(tex_file_name));
      }

      img_tex = QGLWidget::convertToGLFormat(img_tex);
      line_ind_map.insert(tex_file_name.replace(".png", ""), QVector2D(0, 32*i));
      line_dim_map.insert(tex_file_name.replace(".png", ""), QVector2D(img_tex.width(), img_tex.height()));

      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, i*32, img_tex.width(), img_tex.height()
                    , GL_RGBA, GL_UNSIGNED_BYTE, img_tex.bits());
    }

    line_ind_maps.insert(color_schemes[k], line_ind_map);
    line_dim_maps.insert(color_schemes[k], line_dim_map);

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}


void S52Assets::initSymbolTextures(S52References* ref) {
  QStringList color_schemes = ref->getColorSchemeNames();

  GLuint symbol_tex_id;
  for (int k = 0; k < color_schemes.size(); k++) {
    QString file_name = ref->getGraphicsFileName(color_schemes[k]);
    graphic_files.insert(color_schemes[k], file_name);

    if (symbol_tex_ids.contains(file_name))
      continue;

    glGenTextures(1, &symbol_tex_id);
    symbol_tex_ids.insert(file_name, symbol_tex_id);

    glBindTexture(GL_TEXTURE_2D, symbol_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    QImage img("res//textures//symbols//" + file_name);
    img = QGLWidget::convertToGLFormat(img);
    symbol_tex_dims.insert(file_name, QVector2D(img.width(), img.height()));

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height()
                 , 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
}
