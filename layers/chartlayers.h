#ifndef CHARTLAYERENGINES_H
#define CHARTLAYERENGINES_H

#include <QColor>
#include <QVector2D>

#include "chartshaders.h"

#include "../s52/s52chart.h"
#include "../s52/s52assets.h"
#include "../s52/s52references.h"

class ChartAreaEngine : protected QGLFunctions
{
public:

  explicit ChartAreaEngine();
  virtual ~ChartAreaEngine();

  void init(const QGLContext* context);
  void clearData();

  void setPatternTexture(GLuint tex_id, QVector2D dim);
  void setData(S52AreaLayer* layer, S52Assets* assets, S52References* ref);

  void draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle);

private:
  GLuint*   vbo_ids;

  bool initialized;
  int point_count;

  bool is_pattern_uniform;
  QVector2D patternIdx;
  QVector2D patternDim;

  bool is_color_uniform;
  int color_ind;

  std::vector<float> color_table;

  GLint  pattern_tex_id;
  QVector2D pattern_tex_dim;
};


class ChartLineEngine : protected QGLFunctions
{
public:
  explicit ChartLineEngine();
  virtual ~ChartLineEngine();

  void init(const QGLContext* context);
  void clearData();

  void setPatternTexture(GLuint tex_id, QVector2D dim);
  void setData(S52LineLayer* layer, S52Assets* assets, S52References* ref);

  void draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle);

private:
  bool initialized;
  int point_count;

  bool is_pattern_uniform;
  QVector2D patternIdx;
  QVector2D patternDim;

  bool is_color_uniform;
  int color_ind;

  GLuint*  vbo_ids;

  std::vector<float> color_table;

  GLint    pattern_tex_id;
  QVector2D pattern_tex_dim;
};


class ChartMarkEngine : protected QGLFunctions
{
public:
  explicit ChartMarkEngine();
  virtual ~ChartMarkEngine();

  void init(const QGLContext* context);
  void clearData();

  void setPatternTexture(GLuint tex_id, QVector2D size);
  void setData(S52MarkLayer* layer, S52Assets* assets, S52References* ref);

  void draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle);

private:
  bool      initialized;
  int       point_count;

  bool      is_pattern_uniform;
  QVector2D patternOrigin;
  QVector2D patternSize;
  QVector2D patternPivot;
  GLuint*   vbo_ids;

  GLint     pattern_tex_id;
  QVector2D pattern_tex_size;
};


class ChartSndgEngine : protected QGLFunctions
{
public:
  explicit ChartSndgEngine();
  virtual ~ChartSndgEngine();

  void init(const QGLContext* context);
  void clearData();

  void setPatternTexture(GLuint tex_id, QVector2D size);
  void setData(S52SndgLayer* layer, S52Assets* assets, S52References* ref);

  void draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle);

private:
  bool      initialized;
  int       point_count;

  GLuint*   vbo_ids;

  GLint     pattern_tex_id;
  QVector2D pattern_tex_size;
};


class ChartTextEngine : protected QGLFunctions
{
public:
  explicit ChartTextEngine();
  virtual ~ChartTextEngine();

  void init(const QGLContext* context);
  void clearData();

  void setGlyphTexture(GLuint tex_id);
  void setData(S52TextLayer* layer);

  void draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle);

private:
  bool initialized;
  int point_count;

  GLuint* vbo_ids;
  GLint   glyph_tex_id;
};


#endif // CHARTLAYERENGINES_H
