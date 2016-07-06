#include "asmfonts.h"

#include <QPixmap>
#include <QPainter>
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
  dir.setNameFilters(QStringList() << tr("*.asm"));
  QStringList fileNames = dir.entryList();

  for (int i = 0; i < fileNames.size(); i++) {
    QString fName = fileNames[i];
    int w = fName.mid(fName.indexOf(tr("F")) + 1
                    , fName.indexOf(tr("X")) - fName.indexOf(tr("F")) - 1).toInt();
    int h = fName.mid(fName.indexOf(tr("X")) + 1
                    , fName.indexOf(tr("B")) - fName.indexOf(tr("X")) - 1).toInt();

    QSize dim(w, h);

    QFile inputFile(dir.absoluteFilePath(fName));
    if (inputFile.open(QIODevice::ReadOnly)) {
       QTextStream in(&inputFile);
       readFile(dim, &in);
       inputFile.close();
    }
  }
}

void AsmFonts::readFile(QSize dim, QTextStream* in) {
  GLuint tex_id;
  glGenTextures(1, &tex_id);

  glBindTexture(GL_TEXTURE_2D, tex_id);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  QPixmap font_pixmap(16 * dim);
  QPainter painter(&font_pixmap);
  painter.fillRect(font_pixmap.rect(), QBrush(Qt::white));
  painter.setPen(QPen(Qt::black));

  while (!in->atEnd()) {
    QString line = in->readLine();

    if (line.left(3) == tr("CHR")) {
      bool ok;
      unsigned char char_val = line.mid(line.indexOf(tr("_")) + 1, 4).toUInt(&ok, 16);

      QStringList bits;
      bits << getBits(line, static_cast<int>(dim.width()));

      for (int i = 1; i < dim.height(); i++)
        bits << getBits(in->readLine(), static_cast<int>(dim.height()));

      for (int j = 0; j < dim.height(); j++)
        for (int i = 0; i < dim.width(); i++)
          if (bits[j][i] == '1') {
            QPointF point(i, j);
            QPointF shift(i, j);

            shift.setX(dim.width() * (char_val % 16));
            shift.setY(dim.height() * (char_val / 16));

            painter.drawPoint(point + shift);
          }
    }
  }

  QString font_tag = QString::number(dim.width()) + "x" + QString::number(dim.height());

  //font_pixmap.save("/home/zappa/" + font_tag + ".png");

  QImage img = QGLWidget::convertToGLFormat(font_pixmap.toImage());
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());

  _font_sizes.insert(font_tag, dim);
  _tex_ids.insert(font_tag, tex_id);
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
