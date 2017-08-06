#ifndef ASMFONTS_H
#define ASMFONTS_H

#include <QObject>
#include <QVector2D>
#include <QPixmap>
#include <QMap>

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
  void readFile(const QString& filePath, const QString& font);

  QHash<QString, QSize> _font_sizes;
  QHash<QString, GLuint> _tex_ids;
};

#endif // ASMFONTS_H
