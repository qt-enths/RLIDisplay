#ifndef CHARTSHADERFACTORY_H
#define CHARTSHADERFACTORY_H

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLShaderProgram>

typedef enum CHART_SHADER_COMMON_UNIFORMS
{ COMMON_UNIFORMS_NORTH           = 0
, COMMON_UNIFORMS_CENTER          = 1
, COMMON_UNIFORMS_RADIUS          = 2
, COMMON_UNIFORMS_CANVAS          = 3
, COMMON_UNIFORMS_PATTERN_TEX_ID  = 4
, COMMON_UNIFORMS_PATTERN_TEX_DIM = 5
, COMMON_UNIFORMS_COUNT           = 6
} CHART_SHADER_COMMON_UNIFORMS;



typedef enum CHART_SHADER_AREA_UNIFORMS
{ AREA_UNIFORMS_COLOR_INDEX       = COMMON_UNIFORMS_COUNT+0
, AREA_UNIFORMS_PATTERN_INDEX     = COMMON_UNIFORMS_COUNT+1
, AREA_UNIFORMS_PATTERN_DIM       = COMMON_UNIFORMS_COUNT+2
, AREA_UNIFORMS_COLOR_TABLE       = COMMON_UNIFORMS_COUNT+3
, AREA_UNIFORMS_COUNT             = COMMON_UNIFORMS_COUNT+4
} CHART_SHADER_AREA_UNIFORMS;

typedef enum CHART_SHADER_AREA_ATTRIBUTES
{ AREA_ATTRIBUTES_COORDS          = 0
, AREA_ATTRIBUTES_COLOR_INDEX     = 1
, AREA_ATTRIBUTES_PATTERN_INDEX   = 2
, AREA_ATTRIBUTES_PATTERN_DIM     = 3
, AREA_ATTRIBUTES_COUNT           = 4
} CHART_SHADER_AREA_ATTRIBUTES;



typedef enum CHART_SHADER_LINE_UNIFORMS
{ LINE_UNIFORMS_COLOR_INDEX       = COMMON_UNIFORMS_COUNT+0
, LINE_UNIFORMS_PATTERN_INDEX     = COMMON_UNIFORMS_COUNT+1
, LINE_UNIFORMS_PATTERN_DIM       = COMMON_UNIFORMS_COUNT+2
, LINE_UNIFORMS_COLOR_TABLE       = COMMON_UNIFORMS_COUNT+3
, LINE_UNIFORMS_COUNT             = COMMON_UNIFORMS_COUNT+4
} CHART_SHADER_LINE_UNIFORMS;

typedef enum CHART_SHADER_LINE_ATTRIBUTES
{ LINE_ATTRIBUTES_COORDS1         = 0
, LINE_ATTRIBUTES_COORDS2         = 1
, LINE_ATTRIBUTES_DISTANCE        = 2
, LINE_ATTRIBUTES_ORDER           = 3
, LINE_ATTRIBUTES_COLOR_INDEX     = 4
, LINE_ATTRIBUTES_PATTERN_INDEX   = 5
, LINE_ATTRIBUTES_PATTERN_DIM     = 6
, LINE_ATTRIBUTES_COUNT           = 7
} CHART_SHADER_LINE_ATTRIBUTES;



typedef enum CHART_SHADER_TEXT_UNIFORMS
{ TEXT_UNIFORMS_COLOR             = COMMON_UNIFORMS_COUNT+0
, TEXT_UNIFORMS_COUNT             = COMMON_UNIFORMS_COUNT+1
} CHART_SHADER_TEXT_UNIFORMS;

typedef enum CHART_SHADER_TEXT_ATTRIBUTES
{ TEXT_ATTRIBUTES_COORDS          = 0
, TEXT_ATTRIBUTES_CHAR_ORDER      = 1
, TEXT_ATTRIBUTES_CHAR_VALUE      = 2
, TEXT_ATTRIBUTES_COUNT           = 3
} CHART_SHADER_TEXT_ATTRIBUTES;



typedef enum CHART_SHADER_MARK_UNIFORMS
{ MARK_UNIFORMS_SYMBOL_ORIGIN     = COMMON_UNIFORMS_COUNT+0
, MARK_UNIFORMS_SYMBOL_SIZE       = COMMON_UNIFORMS_COUNT+1
, MARK_UNIFORMS_SYMBOL_PIVOT      = COMMON_UNIFORMS_COUNT+2
, MARK_UNIFORMS_COUNT             = COMMON_UNIFORMS_COUNT+3
} CHART_SHADER_MARK_UNIFORMS;

typedef enum CHART_SHADER_MARK_ATTRIBUTES
{ MARK_ATTRIBUTES_WORLD_COORDS    = 0
, MARK_ATTRIBUTES_VERTEX_ORDER    = 1
, MARK_ATTRIBUTES_SYMBOL_ORIGIN   = 2
, MARK_ATTRIBUTES_SYMBOL_SIZE     = 3
, MARK_ATTRIBUTES_SYMBOL_PIVOT    = 4
, MARK_ATTRIBUTES_COUNT           = 5
} CHART_SHADER_MARK_ATTRIBUTES;



typedef enum CHART_SHADER_SNDG_UNIFORMS
{ SNDG_UNIFORMS_COUNT             = COMMON_UNIFORMS_COUNT
} CHART_SHADER_SNDG_UNIFORMS;

typedef enum CHART_SHADER_SNDG_ATTRIBUTES
{ SNDG_ATTRIBUTES_WORLD_COORDS    = 0
, SNDG_ATTRIBUTES_VERTEX_ORDER    = 1
, SNDG_ATTRIBUTES_SYMBOL_ORDER    = 2
, SNDG_ATTRIBUTES_SYMBOL_FRAC     = 3
, SNDG_ATTRIBUTES_SYMBOL_COUNT    = 4
, SNDG_ATTRIBUTES_SYMBOL_ORIGIN   = 5
, SNDG_ATTRIBUTES_SYMBOL_SIZE     = 6
, SNDG_ATTRIBUTES_SYMBOL_PIVOT    = 7
, SNDG_ATTRIBUTES_COUNT           = 8
} CHART_SHADER_SNDG_ATTRIBUTES;



class ChartShaders : protected QGLFunctions {
public:
  ChartShaders();
  ~ChartShaders();

  void init(const QGLContext* context);

  QGLShaderProgram* getChartAreaProgram();
  QGLShaderProgram* getChartLineProgram();
  QGLShaderProgram* getChartTextProgram();
  QGLShaderProgram* getChartMarkProgram();
  QGLShaderProgram* getChartSndgProgram();

  GLuint getAreaUniformLoc(unsigned int index);
  GLuint getAreaAttributeLoc(unsigned int index);

  GLuint getLineUniformLoc(unsigned int index);
  GLuint getLineAttributeLoc(unsigned int index);

  GLuint getTextUniformLoc(unsigned int index);
  GLuint getTextAttributeLoc(unsigned int index);

  GLuint getMarkUniformLoc(unsigned int index);
  GLuint getMarkAttributeLoc(unsigned int index);

  GLuint getSndgUniformLoc(unsigned int index);
  GLuint getSndgAttributeLoc(unsigned int index);

private:
  bool is_initialized;

  bool initChartAreaProgram();
  bool initChartLineProgram();
  bool initChartTextProgram();
  bool initChartMarkProgram();
  bool initChartSndgProgram();

  int area_uniform_locs[AREA_UNIFORMS_COUNT];
  int area_attribute_locs[AREA_ATTRIBUTES_COUNT];

  int line_uniform_locs[LINE_UNIFORMS_COUNT];
  int line_attribute_locs[LINE_ATTRIBUTES_COUNT];

  int text_uniform_locs[TEXT_UNIFORMS_COUNT];
  int text_attribute_locs[TEXT_ATTRIBUTES_COUNT];

  int mark_uniform_locs[MARK_UNIFORMS_COUNT];
  int mark_attribute_locs[MARK_ATTRIBUTES_COUNT];

  int sndg_uniform_locs[SNDG_UNIFORMS_COUNT];
  int sndg_attribute_locs[SNDG_ATTRIBUTES_COUNT];

  QGLShaderProgram* chart_area_program;
  QGLShaderProgram* chart_line_program;
  QGLShaderProgram* chart_text_program;
  QGLShaderProgram* chart_mark_program;
  QGLShaderProgram* chart_sndg_program;
};

#endif // CHARTSHADERFACTORY_H
