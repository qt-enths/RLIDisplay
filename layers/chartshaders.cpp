#include "chartshaders.h"

ChartShaders::ChartShaders()
{
  is_initialized = false;

  chart_area_program = NULL;
  chart_line_program = NULL;
  chart_text_program = NULL;
  chart_mark_program = NULL;
  chart_sndg_program = NULL;
}

ChartShaders::~ChartShaders()
{
  if (chart_area_program != NULL)
    delete chart_area_program;
  if (chart_line_program != NULL)
    delete chart_line_program;
  if (chart_text_program != NULL)
    delete chart_text_program;
  if (chart_mark_program != NULL)
    delete chart_mark_program;
  if (chart_sndg_program != NULL)
    delete chart_sndg_program;
}

void ChartShaders::init(const QGLContext* context) {
  is_initialized = false;
  initializeGLFunctions(context);

  if (!initChartAreaProgram() ||
      !initChartLineProgram() ||
      !initChartMarkProgram() ||
      !initChartTextProgram() ||
      !initChartSndgProgram())
    return;

  is_initialized = true;
}


QGLShaderProgram* ChartShaders::getChartAreaProgram() {
  if (!is_initialized)
    return NULL;

  return chart_area_program;
}

QGLShaderProgram* ChartShaders::getChartLineProgram() {
  if (!is_initialized)
    return NULL;

  return chart_line_program;
}

QGLShaderProgram* ChartShaders::getChartTextProgram() {
  if (!is_initialized)
    return NULL;

  return chart_text_program;
}

QGLShaderProgram* ChartShaders::getChartMarkProgram() {
  if (!is_initialized)
    return NULL;

  return chart_mark_program;
}

QGLShaderProgram* ChartShaders::getChartSndgProgram() {
  if (!is_initialized)
    return NULL;

  return chart_sndg_program;
}



GLuint ChartShaders::getAreaUniformLoc(unsigned int index) {
  if (index < AREA_UNIFORMS_COUNT)
    return area_uniform_locs[index];

  return 0;
}

GLuint ChartShaders::getAreaAttributeLoc(unsigned int index) {
  if (index < AREA_ATTRIBUTES_COUNT)
    return area_attribute_locs[index];

  return 0;
}

GLuint ChartShaders::getLineUniformLoc(unsigned int index) {
  if (index < LINE_UNIFORMS_COUNT)
    return line_uniform_locs[index];

  return 0;
}

GLuint ChartShaders::getLineAttributeLoc(unsigned int index) {
  if (index < LINE_ATTRIBUTES_COUNT)
    return line_attribute_locs[index];

  return 0;
}

GLuint ChartShaders::getTextUniformLoc(unsigned int index) {
  if (index < TEXT_UNIFORMS_COUNT)
    return text_uniform_locs[index];

  return 0;
}

GLuint ChartShaders::getTextAttributeLoc(unsigned int index) {
  if (index < TEXT_ATTRIBUTES_COUNT)
    return text_attribute_locs[index];

  return 0;
}

GLuint ChartShaders::getMarkUniformLoc(unsigned int index) {
  if (index < MARK_UNIFORMS_COUNT)
    return mark_uniform_locs[index];

  return 0;
}

GLuint ChartShaders::getMarkAttributeLoc(unsigned int index) {
  if (index < MARK_ATTRIBUTES_COUNT)
    return mark_attribute_locs[index];

  return 0;
}

GLuint ChartShaders::getSndgUniformLoc(unsigned int index) {
  if (index < SNDG_UNIFORMS_COUNT)
    return sndg_uniform_locs[index];

  return 0;
}

GLuint ChartShaders::getSndgAttributeLoc(unsigned int index) {
  if (index < SNDG_ATTRIBUTES_COUNT)
    return sndg_attribute_locs[index];

  return 0;
}



bool ChartShaders::initChartAreaProgram() {
  if (chart_area_program != NULL) {
    chart_area_program->release();
    delete chart_area_program;
  }

  chart_area_program = new QGLShaderProgram();
  setlocale(LC_NUMERIC, "C");     // Overriding system locale until shaders are compiled

  if (!chart_area_program->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/chart_area.vert.glsl")
   || !chart_area_program->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/chart_area.frag.glsl")
   || !chart_area_program->link()
   || !chart_area_program->bind())
      return false;

  area_uniform_locs[COMMON_UNIFORMS_NORTH]            = chart_area_program->uniformLocation("north");
  area_uniform_locs[COMMON_UNIFORMS_CENTER]           = chart_area_program->uniformLocation("center");
  area_uniform_locs[COMMON_UNIFORMS_SCALE]            = chart_area_program->uniformLocation("scale");
  area_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_ID]   = chart_area_program->uniformLocation("pattern_tex");
  area_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_DIM]  = chart_area_program->uniformLocation("assetdim");

  area_uniform_locs[AREA_UNIFORMS_COLOR_INDEX]        = chart_area_program->uniformLocation("u_color_index");
  area_uniform_locs[AREA_UNIFORMS_PATTERN_INDEX]      = chart_area_program->uniformLocation("u_tex_origin");
  area_uniform_locs[AREA_UNIFORMS_PATTERN_DIM]        = chart_area_program->uniformLocation("u_tex_dim");
  area_uniform_locs[AREA_UNIFORMS_COLOR_TABLE]        = chart_area_program->uniformLocation("u_color_table");


  area_attribute_locs[AREA_ATTRIBUTES_COORDS]         = chart_area_program->attributeLocation("coords");
  area_attribute_locs[AREA_ATTRIBUTES_COLOR_INDEX]    = chart_area_program->attributeLocation("color_index");
  area_attribute_locs[AREA_ATTRIBUTES_PATTERN_INDEX]  = chart_area_program->attributeLocation("tex_origin");
  area_attribute_locs[AREA_ATTRIBUTES_PATTERN_DIM]    = chart_area_program->attributeLocation("tex_dim");

  chart_area_program->release();
  setlocale(LC_ALL, "");          // Restore system locale
  return true;
}

bool ChartShaders::initChartLineProgram() {
  if (chart_line_program != NULL) {
    chart_line_program->release();
    delete chart_line_program;
  }

  chart_line_program = new QGLShaderProgram();
  setlocale(LC_NUMERIC, "C");     // Overriding system locale until shaders are compiled

  if (!chart_line_program->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/chart_line.vert.glsl")
   || !chart_line_program->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/chart_line.frag.glsl")
   || !chart_line_program->link()
   || !chart_line_program->bind())
      return false;

  line_uniform_locs[COMMON_UNIFORMS_NORTH]            = chart_line_program->uniformLocation("north");
  line_uniform_locs[COMMON_UNIFORMS_CENTER]           = chart_line_program->uniformLocation("center");
  line_uniform_locs[COMMON_UNIFORMS_SCALE]            = chart_line_program->uniformLocation("scale");
  line_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_ID]   = chart_line_program->uniformLocation("pattern_tex");
  line_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_DIM]  = chart_line_program->uniformLocation("assetdim");

  line_uniform_locs[LINE_UNIFORMS_COLOR_INDEX]        = chart_line_program->uniformLocation("u_color_index");
  line_uniform_locs[LINE_UNIFORMS_PATTERN_INDEX]      = chart_line_program->uniformLocation("u_tex_origin");
  line_uniform_locs[LINE_UNIFORMS_PATTERN_DIM]        = chart_line_program->uniformLocation("u_tex_dim");
  line_uniform_locs[LINE_UNIFORMS_COLOR_TABLE]        = chart_line_program->uniformLocation("u_color_table");

  line_attribute_locs[LINE_ATTRIBUTES_COORDS1]        = chart_line_program->attributeLocation("coords1");
  line_attribute_locs[LINE_ATTRIBUTES_COORDS2]        = chart_line_program->attributeLocation("coords2");
  line_attribute_locs[LINE_ATTRIBUTES_DISTANCE]       = chart_line_program->attributeLocation("dist");
  line_attribute_locs[LINE_ATTRIBUTES_ORDER]          = chart_line_program->attributeLocation("order");
  line_attribute_locs[LINE_ATTRIBUTES_PATTERN_INDEX]  = chart_line_program->attributeLocation("tex_orig");
  line_attribute_locs[LINE_ATTRIBUTES_PATTERN_DIM]    = chart_line_program->attributeLocation("tex_dim");
  line_attribute_locs[LINE_ATTRIBUTES_COLOR_INDEX]    = chart_line_program->attributeLocation("color_index");

  chart_line_program->release();
  setlocale(LC_ALL, "");      // Restore system locale
  return true;
}

bool ChartShaders::initChartTextProgram() {
  if (chart_text_program != NULL) {
    chart_text_program->release();
    delete chart_text_program;
  }

  chart_text_program = new QGLShaderProgram();
  setlocale(LC_NUMERIC, "C");     // Overriding system locale until shaders are compiled

  //qDebug() << "Linking chart_mark shader";
  if (!chart_text_program->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/chart_text.vert.glsl")
   || !chart_text_program->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/chart_text.frag.glsl")
   || !chart_text_program->link()
   || !chart_text_program->bind())
      return false;

  text_uniform_locs[COMMON_UNIFORMS_NORTH]            = chart_text_program->uniformLocation("north");
  text_uniform_locs[COMMON_UNIFORMS_CENTER]           = chart_text_program->uniformLocation("center");
  text_uniform_locs[COMMON_UNIFORMS_SCALE]            = chart_text_program->uniformLocation("scale");
  text_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_ID]   = chart_text_program->uniformLocation("glyph_tex");
  text_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_DIM]  = chart_text_program->uniformLocation("pattern_tex_size");

  text_uniform_locs[TEXT_UNIFORMS_COLOR]              = chart_text_program->uniformLocation("color");

  text_attribute_locs[TEXT_ATTRIBUTES_COORDS]         = chart_text_program->attributeLocation("world_coords");
  text_attribute_locs[TEXT_ATTRIBUTES_CHAR_ORDER]     = chart_text_program->attributeLocation("char_order");
  text_attribute_locs[TEXT_ATTRIBUTES_CHAR_VALUE]     = chart_text_program->attributeLocation("char_val");

  chart_text_program->release();
  setlocale(LC_ALL, "");      // Restore system locale
  return true;
}

bool ChartShaders::initChartMarkProgram() {
  if (chart_mark_program != NULL) {
    chart_mark_program->release();
    delete chart_mark_program;
  }

  chart_mark_program = new QGLShaderProgram();
  setlocale(LC_NUMERIC, "C");     // Overriding system locale until shaders are compiled

  //qDebug() << "Linking chart_mark shader";
  if (!chart_mark_program->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/chart_mark.vert.glsl")
   || !chart_mark_program->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/chart_mark.frag.glsl")
   || !chart_mark_program->link()
   || !chart_mark_program->bind())
      return false;

  mark_uniform_locs[COMMON_UNIFORMS_NORTH]            = chart_mark_program->uniformLocation("north");
  mark_uniform_locs[COMMON_UNIFORMS_CENTER]           = chart_mark_program->uniformLocation("center");
  mark_uniform_locs[COMMON_UNIFORMS_SCALE]            = chart_mark_program->uniformLocation("scale");
  mark_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_ID]   = chart_mark_program->uniformLocation("pattern_tex_id");
  mark_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_DIM]  = chart_mark_program->uniformLocation("pattern_tex_size");

  mark_uniform_locs[MARK_UNIFORMS_SYMBOL_ORIGIN]      = chart_mark_program->uniformLocation("u_symbol_origin");
  mark_uniform_locs[MARK_UNIFORMS_SYMBOL_SIZE]        = chart_mark_program->uniformLocation("u_symbol_size");
  mark_uniform_locs[MARK_UNIFORMS_SYMBOL_PIVOT]       = chart_mark_program->uniformLocation("u_symbol_pivot");

  mark_attribute_locs[MARK_ATTRIBUTES_WORLD_COORDS]   = chart_mark_program->attributeLocation("world_coords");
  mark_attribute_locs[MARK_ATTRIBUTES_VERTEX_ORDER]   = chart_mark_program->attributeLocation("vertex_order");
  mark_attribute_locs[MARK_ATTRIBUTES_SYMBOL_ORIGIN]  = chart_mark_program->attributeLocation("symbol_origin");
  mark_attribute_locs[MARK_ATTRIBUTES_SYMBOL_SIZE]    = chart_mark_program->attributeLocation("symbol_size");
  mark_attribute_locs[MARK_ATTRIBUTES_SYMBOL_PIVOT]   = chart_mark_program->attributeLocation("symbol_pivot");

  chart_mark_program->release();
  setlocale(LC_ALL, "");      // Restore system locale
  return true;
}

bool ChartShaders::initChartSndgProgram() {
  if (chart_sndg_program != NULL) {
    chart_sndg_program->release();
    delete chart_sndg_program;
  }

  chart_sndg_program = new QGLShaderProgram();
  setlocale(LC_NUMERIC, "C");     // Overriding system locale until shaders are compiled

  //qDebug() << "Linking chart_mark shader";
  if (!chart_sndg_program->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/chart_sndg.vert.glsl")
   || !chart_sndg_program->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/chart_sndg.frag.glsl")
   || !chart_sndg_program->link()
   || !chart_sndg_program->bind())
      return false;

  sndg_uniform_locs[COMMON_UNIFORMS_NORTH]            = chart_sndg_program->uniformLocation("north");
  sndg_uniform_locs[COMMON_UNIFORMS_CENTER]           = chart_sndg_program->uniformLocation("center");
  sndg_uniform_locs[COMMON_UNIFORMS_SCALE]            = chart_sndg_program->uniformLocation("scale");
  sndg_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_ID]   = chart_sndg_program->uniformLocation("pattern_tex_id");
  sndg_uniform_locs[COMMON_UNIFORMS_PATTERN_TEX_DIM]  = chart_sndg_program->uniformLocation("pattern_tex_size");

  sndg_attribute_locs[SNDG_ATTRIBUTES_WORLD_COORDS]   = chart_sndg_program->attributeLocation("world_coords");
  sndg_attribute_locs[SNDG_ATTRIBUTES_VERTEX_ORDER]   = chart_sndg_program->attributeLocation("vertex_order");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_ORDER]   = chart_sndg_program->attributeLocation("symbol_order");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_FRAC]    = chart_sndg_program->attributeLocation("symbol_frac");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_COUNT]   = chart_sndg_program->attributeLocation("symbol_count");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_ORIGIN]  = chart_sndg_program->attributeLocation("symbol_origin");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_SIZE]    = chart_sndg_program->attributeLocation("symbol_size");
  sndg_attribute_locs[SNDG_ATTRIBUTES_SYMBOL_PIVOT]   = chart_sndg_program->attributeLocation("symbol_pivot");

  chart_sndg_program->release();
  setlocale(LC_ALL, "");      // Restore system locale
  return true;
}
