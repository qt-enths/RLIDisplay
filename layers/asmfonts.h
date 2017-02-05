#ifndef ASMFONTS_H
#define ASMFONTS_H

#include <QTextStream>
#include <QObject>
#include <QVector2D>
#include <QPixmap>
#include <QMap>
#include <QVector>

#include <QtOpenGL/QGLContext>
#include <QtOpenGL/QGLFunctions>

class AsmFonts : public QObject, protected QGLFunctions
{
  Q_OBJECT
public:
  explicit AsmFonts(QObject *parent = 0);
  virtual ~AsmFonts();

  void init(const QGLContext* context, QString dirPath);

  QSize getSize(const QString& tag);
  GLuint getTextureId(const QString& tag);

private:
  void readFile(QSize dim, QTextStream* in);

  inline QString getBits(QString line, int count)
  { return line.right(line.size() - line.indexOf("DB")).remove(QRegExp("[^\\d]")).left(count); }

  QHash<QString, QSize> _font_sizes;
  QHash<QString, GLuint> _tex_ids;
};

#endif // ASMFONTS_H
