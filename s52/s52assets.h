#ifndef S52ASSETS_H
#define S52ASSETS_H

#include <QGLFunctions>
#include <QVector2D>
#include <QMap>

#include "s52references.h"

class S52Assets : protected QGLFunctions
{
public:
  explicit S52Assets                     ();
  virtual ~S52Assets                     ();

  void             init                  (const QGLContext* context, S52References* ref);

  inline GLuint    getGlyphTextureId     () { return glyph_tex_id; }

  inline GLuint    getPatternTextureId   (const QString& scheme)                      { return pattern_tex_ids[scheme]; }
  inline QVector2D getPatternTextureDim  (const QString& scheme)                      { return pattern_tex_dims[scheme]; }
  inline QVector2D getPatternIndex       (const QString& scheme, const QString& name) { return pattern_ind_maps[scheme][name]; }
  inline QVector2D getPatternDim         (const QString& scheme, const QString& name) { return pattern_dim_maps[scheme][name]; }

  inline GLuint    getLineTextureId      (const QString& scheme)                      { return line_tex_ids[scheme]; }
  inline QVector2D getLineTextureDim     (const QString& scheme)                      { return line_tex_dims[scheme]; }
  inline QVector2D getLineIndex          (const QString& scheme, const QString& name) { return line_ind_maps[scheme][name]; }
  inline QVector2D getLineDim            (const QString& scheme, const QString& name) { return line_dim_maps[scheme][name]; }

  inline GLuint     getSymbolTextureId   (const QString& scheme)                      { return symbol_tex_ids[graphic_files[scheme]]; }
  inline QVector2D  getSymbolTextureDim  (const QString& scheme)                      { return symbol_tex_dims[graphic_files[scheme]]; }

private:
  void initGlyphTexture                  ();
  void initPatternTextures               (S52References* ref);
  void initLineTextures                  (S52References* ref);
  void initSymbolTextures                (S52References* ref);

  bool                                      _initialized;

  GLuint                                    glyph_tex_id;

  QMap< QString, GLuint >                   pattern_tex_ids;
  QMap< QString, QVector2D >                pattern_tex_dims;
  QMap< QString, QMap<QString, QVector2D> > pattern_ind_maps;
  QMap< QString, QMap<QString, QVector2D> > pattern_dim_maps;

  QMap< QString, GLuint >                   line_tex_ids;
  QMap< QString, QVector2D >                line_tex_dims;
  QMap< QString, QMap<QString, QVector2D> > line_ind_maps;
  QMap< QString, QMap<QString, QVector2D> > line_dim_maps;

  QMap< QString, QString >                  graphic_files;
  QMap< QString, GLuint >                   symbol_tex_ids;
  QMap< QString, QVector2D >                symbol_tex_dims;
  QMap< QString, QMap<QString, QVector2D> > symbol_ind_maps;
  QMap< QString, QMap<QString, QVector2D> > symbol_dim_maps;
  QMap< QString, QMap<QString, QVector2D> > symbol_piv_maps;
};

#endif // S52ASSETS_H
