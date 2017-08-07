#include "asmfonts.h"

#include <QPixmap>
#include <QDebug>
#include <QDir>

AsmFonts::AsmFonts(QObject *parent) : QObject(parent), QGLFunctions() {
}

AsmFonts::~AsmFonts() {
  QList<QString>::const_iterator iter;
  for (int i = 0; i < _tex_ids.count(); i++)
    glDeleteTextures(1, &_tex_ids[_tex_ids.keys()[i]]);
}

void AsmFonts::init(const QGLContext* context, QString dirPath) {
  initializeGLFunctions(context);

  QDir dir(dirPath);
  dir.setNameFilters(QStringList() << tr("*.png"));
  QStringList fileNames = dir.entryList();

  for (int i = 0; i < fileNames.size(); i++)
    readFile(dir.absoluteFilePath(fileNames[i]), fileNames[i].replace(".png", ""));
}

void AsmFonts::readFile(const QString& filePath, const QString& font) {
  GLuint tex_id;
  glGenTextures(1, &tex_id);

  glBindTexture(GL_TEXTURE_2D, tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  QImage img(filePath);
  QSize dim = img.size() / 16;

  qDebug() << font;

  img = QGLWidget::convertToGLFormat(img);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

  _font_sizes.insert(font, dim);
  _tex_ids.insert(font, tex_id);
}

QSize AsmFonts::getSize(const QString& tag) {
  if (_font_sizes.contains(tag))
    return _font_sizes[tag];

  return QSize(0, 0);
}

GLuint AsmFonts::getTextureId(const QString& tag) {
  if (_tex_ids.contains(tag))
    return _tex_ids[tag];

  return 0;
}
