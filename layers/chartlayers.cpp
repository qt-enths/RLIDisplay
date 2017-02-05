#include "chartlayers.h"

ChartAreaEngine::ChartAreaEngine() {
  initialized = false;
  pattern_tex_id = -1;
  point_count = 0;

  is_color_uniform = false;
  is_pattern_uniform = false;
  vbo_ids = new GLuint[AREA_ATTRIBUTES_COUNT];
}

ChartAreaEngine::~ChartAreaEngine() {
  if (initialized)
    glDeleteBuffers(AREA_ATTRIBUTES_COUNT, vbo_ids);
  delete[] vbo_ids;
}

void ChartAreaEngine::init(const QGLContext* context) {
  if (initialized)
    return;

  initializeGLFunctions(context);
  glGenBuffers(AREA_ATTRIBUTES_COUNT, vbo_ids);
  initialized = true;
}

void ChartAreaEngine::clearData() {
  if (point_count == 0)
    return;

  point_count = 0;

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COLOR_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  }

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_PATTERN_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_PATTERN_DIM]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  }
}

void ChartAreaEngine::setPatternTexture(GLuint tex_id, QVector2D dim) {
  pattern_tex_id = tex_id;
  pattern_tex_dim = dim;
}

void ChartAreaEngine::setData(S52AreaLayer* layer, S52Assets* assets, S52References* ref) {
  std::vector<GLfloat> color_inds;
  std::vector<GLfloat> tex_inds;
  std::vector<GLfloat> tex_dims;

  if (layer->is_pattern_uniform) {
    is_pattern_uniform = true;
    patternIdx = assets->getPatternIndex(ref->getColorScheme(), layer->pattern_ref);
    patternDim = assets->getPatternDim(ref->getColorScheme(), layer->pattern_ref);
  }

  if (layer->is_color_uniform) {
    is_color_uniform = true;
    color_ind = layer->color_ind;
  }

  if ((!is_color_uniform) || (!is_pattern_uniform)) {
    for (unsigned int i = 0; i < layer->start_inds.size(); i++) {
      int fst_idx = layer->start_inds[i];
      int lst_idx = 0;

      if (i < layer->start_inds.size() - 1)
        lst_idx = layer->start_inds[i+1] - 1;
      else
        lst_idx = layer->triangles.size() - 1;

      if (lst_idx <= fst_idx)
        continue;

      QVector2D tex_ind;
      QVector2D tex_dim;

      if (!is_pattern_uniform) {
        tex_ind = assets->getPatternIndex(ref->getColorScheme(), layer->pattern_refs[i]);
        tex_dim = assets->getPatternDim(ref->getColorScheme(), layer->pattern_refs[i]);
      }

      for (int j = fst_idx; j < lst_idx; j += 2) {
        if (!is_color_uniform) {
          color_inds.push_back(layer->color_inds[i]);
        }

        if (!is_pattern_uniform) {
          tex_inds.push_back(tex_ind.x());
          tex_inds.push_back(tex_ind.y());
          tex_dims.push_back(tex_dim.x());
          tex_dims.push_back(tex_dim.y());
        }
      }
    }
  }

  point_count = layer->triangles.size() / 2;
  color_table = ref->getColorTable();

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, layer->triangles.size() * sizeof(GLfloat), &(layer->triangles[0]), GL_STATIC_DRAW);

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COLOR_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, color_inds.size() * sizeof(GLfloat), &color_inds[0], GL_STATIC_DRAW);
  }

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_PATTERN_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, tex_inds.size() * sizeof(GLfloat), &tex_inds[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_PATTERN_DIM]);
    glBufferData(GL_ARRAY_BUFFER, tex_dims.size() * sizeof(GLfloat), &tex_dims[0], GL_STATIC_DRAW);
  }
}

void ChartAreaEngine::draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle) {
  if (!initialized
   || pattern_tex_id == -1
   || point_count <= 0)
    return;

  glUniform2f(shaders->getAreaUniformLoc(COMMON_UNIFORMS_CENTER), cur_coords.x(), cur_coords.y());
  glUniform1f(shaders->getAreaUniformLoc(COMMON_UNIFORMS_SCALE), scale);
  glUniform1f(shaders->getAreaUniformLoc(COMMON_UNIFORMS_NORTH), angle);
  glUniform2f(shaders->getAreaUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_DIM), pattern_tex_dim.x(), pattern_tex_dim.y());

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COORDS]);
  glVertexAttribPointer(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_COORDS), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_COORDS));

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_COLOR_INDEX]);
    glVertexAttribPointer(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_COLOR_INDEX), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_COLOR_INDEX));

    glUniform1f(shaders->getAreaUniformLoc(AREA_UNIFORMS_COLOR_INDEX), -1);
  } else {
    glDisableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_COLOR_INDEX));

    glUniform1f(shaders->getAreaUniformLoc(AREA_UNIFORMS_COLOR_INDEX), color_ind);
  }

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[AREA_ATTRIBUTES_PATTERN_INDEX]);
    glVertexAttribPointer(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_INDEX), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_INDEX));

    glBindBuffer(GL_ARRAY_BUFFER, AREA_ATTRIBUTES_PATTERN_DIM);
    glVertexAttribPointer(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_DIM), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_DIM));

    glUniform2f(shaders->getAreaUniformLoc(AREA_UNIFORMS_PATTERN_INDEX), -1, -1);
    glUniform2f(shaders->getAreaUniformLoc(AREA_UNIFORMS_PATTERN_DIM), -1, -1);
  } else {
    glDisableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_INDEX));
    glDisableVertexAttribArray(shaders->getAreaAttributeLoc(AREA_ATTRIBUTES_PATTERN_DIM));

    glUniform2f(shaders->getAreaUniformLoc(AREA_UNIFORMS_PATTERN_INDEX), patternIdx.x(), patternIdx.y());
    glUniform2f(shaders->getAreaUniformLoc(AREA_UNIFORMS_PATTERN_DIM), patternDim.x(), patternDim.y());
  }

  glUniform1f(shaders->getAreaUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_ID), 0);
  glUniform1fv(shaders->getAreaUniformLoc(AREA_UNIFORMS_COLOR_TABLE), color_table.size(), &color_table[0]);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pattern_tex_id);

  glDrawArrays(GL_TRIANGLES, 0, point_count);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
}




ChartLineEngine::ChartLineEngine() {
  initialized = false;
  point_count = 0;
  pattern_tex_id = -1;
  is_pattern_uniform = false;
  is_color_uniform = false;
  vbo_ids = new GLuint[LINE_ATTRIBUTES_COUNT];
}

ChartLineEngine::~ChartLineEngine() {
  if (initialized)
    glDeleteBuffers(LINE_ATTRIBUTES_COUNT, vbo_ids);
  delete[] vbo_ids;
}

void ChartLineEngine::clearData() {
  if (point_count == 0)
    return;

  point_count = 0;

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS1]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS2]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_DISTANCE]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_DIM]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  }

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COLOR_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  }
}

void ChartLineEngine::init(const QGLContext* context) {
  if (initialized)
    return;

  initializeGLFunctions(context);
  glGenBuffers(LINE_ATTRIBUTES_COUNT, vbo_ids);
  initialized = true;
}

void ChartLineEngine::setPatternTexture(GLuint tex_id, QVector2D dim) {
  pattern_tex_id = tex_id;
  pattern_tex_dim = dim;
}

void ChartLineEngine::setData(S52LineLayer* layer, S52Assets* assets, S52References* ref) {
  if (!initialized)
    return;

  std::vector<GLfloat> coords1;
  std::vector<GLfloat> coords2;
  std::vector<GLfloat> point_ords;
  std::vector<GLfloat> distances;
  std::vector<GLfloat> color_inds;
  std::vector<GLfloat> tex_inds;
  std::vector<GLfloat> tex_dims;

  if (layer->is_pattern_uniform) {
    is_pattern_uniform = true;
    patternIdx = assets->getLineIndex(ref->getColorScheme(), layer->pattern_ref);
    patternDim = assets->getLineDim(ref->getColorScheme(), layer->pattern_ref);
  }

  if (layer->is_color_uniform) {
    is_color_uniform = true;
    color_ind = layer->color_ind;
  }

  for (unsigned int i = 0; i < layer->start_inds.size(); i++) {
    int fst_idx = layer->start_inds[i];
    int lst_idx = 0;

    if (i < layer->start_inds.size() - 1)
      lst_idx = layer->start_inds[i+1] - 1;
    else
      lst_idx = layer->points.size() - 1;

    if (lst_idx <= fst_idx)
      continue;

    QVector2D tex_ind, tex_dim;
    if (!is_pattern_uniform) {
      tex_ind = assets->getLineIndex(ref->getColorScheme(), layer->pattern_refs[i]);
      tex_dim = assets->getLineDim(ref->getColorScheme(), layer->pattern_refs[i]);
    }

    for (int j = fst_idx; j < lst_idx - 2; j += 2) {
      for (int k = 0; k < 4; k++) {
        coords1.push_back(layer->points[j+0]);
        coords1.push_back(layer->points[j+1]);
        coords2.push_back(layer->points[j+2]);
        coords2.push_back(layer->points[j+3]);
        point_ords.push_back(k);
        distances.push_back(layer->distances[j/2]);

        if (!is_color_uniform) {
          color_inds.push_back(layer->color_inds[i]);
        }

        if (!is_pattern_uniform) {
          tex_inds.push_back(tex_ind.x());
          tex_inds.push_back(tex_ind.y());
          tex_dims.push_back(tex_dim.x());
          tex_dims.push_back(tex_dim.y());
        }
      }
    }
  }

  point_count = point_ords.size();
  color_table = ref->getColorTable();

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS1]);
  glBufferData(GL_ARRAY_BUFFER, coords1.size() * sizeof(GLfloat), &coords1[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS2]);
  glBufferData(GL_ARRAY_BUFFER, coords2.size() * sizeof(GLfloat), &coords2[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_DISTANCE]);
  glBufferData(GL_ARRAY_BUFFER, distances.size() * sizeof(GLfloat), &distances[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, point_ords.size() * sizeof(GLfloat), &point_ords[0], GL_STATIC_DRAW);

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, tex_inds.size() * sizeof(GLfloat), &tex_inds[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_DIM]);
    glBufferData(GL_ARRAY_BUFFER, tex_dims.size() * sizeof(GLfloat), &tex_dims[0], GL_STATIC_DRAW);
  }

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COLOR_INDEX]);
    glBufferData(GL_ARRAY_BUFFER, color_inds.size() * sizeof(GLfloat), &color_inds[0], GL_STATIC_DRAW);
  }
}

void ChartLineEngine::draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle) {
  if (!initialized
   || pattern_tex_id == -1
   || point_count <= 0)
    return;

  glUniform2f(shaders->getLineUniformLoc(COMMON_UNIFORMS_CENTER), cur_coords.x(), cur_coords.y());
  glUniform1f(shaders->getLineUniformLoc(COMMON_UNIFORMS_SCALE), scale);
  glUniform1f(shaders->getLineUniformLoc(COMMON_UNIFORMS_NORTH), angle);
  glUniform2f(shaders->getLineUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_DIM), pattern_tex_dim.x(), pattern_tex_dim.y());

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS1]);
  glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COORDS1), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COORDS1));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COORDS2]);
  glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COORDS2), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COORDS2));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_DISTANCE]);
  glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_DISTANCE), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_DISTANCE));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_ORDER]);
  glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_ORDER), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_ORDER));

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_INDEX]);
    glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_INDEX), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_INDEX));

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_PATTERN_DIM]);
    glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_DIM), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_DIM));

    glUniform2f(shaders->getLineUniformLoc(LINE_UNIFORMS_PATTERN_INDEX), -1, -1);
    glUniform2f(shaders->getLineUniformLoc(LINE_UNIFORMS_PATTERN_DIM), -1, -1);
  } else {
    glDisableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_INDEX));
    glDisableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_PATTERN_DIM));

    glUniform2f(shaders->getLineUniformLoc(LINE_UNIFORMS_PATTERN_INDEX), patternIdx.x(), patternIdx.y());
    glUniform2f(shaders->getLineUniformLoc(LINE_UNIFORMS_PATTERN_DIM), patternDim.x(), patternDim.y());
  }

  if (!is_color_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[LINE_ATTRIBUTES_COLOR_INDEX]);
    glVertexAttribPointer(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COLOR_INDEX), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COLOR_INDEX));

    glUniform1f(shaders->getLineUniformLoc(LINE_UNIFORMS_COLOR_INDEX), -1);
  } else {
    glDisableVertexAttribArray(shaders->getLineAttributeLoc(LINE_ATTRIBUTES_COLOR_INDEX));

    glUniform1f(shaders->getLineUniformLoc(LINE_UNIFORMS_COLOR_INDEX), color_ind);
  }

  glUniform1f(shaders->getLineUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_ID), 0);
  glUniform1fv(shaders->getLineUniformLoc(LINE_UNIFORMS_COLOR_TABLE), color_table.size(), &color_table[0]);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pattern_tex_id);

  glDrawArrays(GL_QUADS, 0, point_count);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
}






ChartTextEngine::ChartTextEngine() {
  initialized = false;
  glyph_tex_id = -1;
  point_count = 0;
  vbo_ids = new GLuint[TEXT_ATTRIBUTES_COUNT];
}

ChartTextEngine::~ChartTextEngine() {
  if (initialized)
    glDeleteBuffers(TEXT_ATTRIBUTES_COUNT, vbo_ids);
  delete[] vbo_ids;
}

void ChartTextEngine::init(const QGLContext* context) {
  if (initialized)
    return;

  initializeGLFunctions(context);
  glGenBuffers(TEXT_ATTRIBUTES_COUNT, vbo_ids);
  initialized = true;
}

void ChartTextEngine::clearData() {
  if (point_count == 0)
    return;

  point_count = 0;

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_VALUE]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
}

void ChartTextEngine::setGlyphTexture(GLuint tex_id) {
  glyph_tex_id = tex_id;
}

void ChartTextEngine::setData(S52TextLayer* layer) {
  std::vector<GLfloat> world_coords;
  std::vector<GLfloat> char_orders;
  std::vector<GLfloat> char_values;

  for (unsigned int i = 0; i < (layer->points.size() / 2); i ++) {
    QString txt = layer->texts[i];

    for (int j = 0; j < txt.size(); j++) {
      world_coords.push_back(layer->points[2*i+0]);
      world_coords.push_back(layer->points[2*i+1]);

      char_orders.push_back(j);
      char_values.push_back(static_cast<int>(txt.at(j).toLatin1()));
    }
  }

  point_count = char_orders.size();

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, world_coords.size() * sizeof(GLfloat), &world_coords[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, char_orders.size() * sizeof(GLfloat), &char_orders[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_VALUE]);
  glBufferData(GL_ARRAY_BUFFER, char_values.size() * sizeof(GLfloat), &char_values[0], GL_STATIC_DRAW);
}

void ChartTextEngine::draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle) {
  if (!initialized
   || glyph_tex_id == -1
   || point_count <= 0)
    return;

  glUniform2f(shaders->getTextUniformLoc(COMMON_UNIFORMS_CENTER), cur_coords.x(), cur_coords.y());
  glUniform1f(shaders->getTextUniformLoc(COMMON_UNIFORMS_SCALE), scale);
  glUniform1f(shaders->getTextUniformLoc(COMMON_UNIFORMS_NORTH),  angle);
  glUniform2f(shaders->getTextUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_DIM), 32*16, 32*16);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_COORDS]);
  glVertexAttribPointer(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_COORDS), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_COORDS));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_ORDER]);
  glVertexAttribPointer(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_CHAR_ORDER), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_CHAR_ORDER));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[TEXT_ATTRIBUTES_CHAR_VALUE]);
  glVertexAttribPointer(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_CHAR_VALUE), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getTextAttributeLoc(TEXT_ATTRIBUTES_CHAR_VALUE));

  glUniform1f(shaders->getLineUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_ID), 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, glyph_tex_id);

  glPointSize(32);

  glDrawArrays(GL_POINTS, 0, point_count);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
}





ChartMarkEngine::ChartMarkEngine() {
  initialized = false;
  point_count = 0;
  is_pattern_uniform = false;
  pattern_tex_id = -1;
  vbo_ids = new GLuint[MARK_ATTRIBUTES_COUNT];
}

ChartMarkEngine::~ChartMarkEngine() {
  if (initialized)
    glDeleteBuffers(MARK_ATTRIBUTES_COUNT, vbo_ids);
  delete[] vbo_ids;
}

void ChartMarkEngine::init(const QGLContext* context) {
  if (initialized)
    return;

  initializeGLFunctions(context);
  glGenBuffers(MARK_ATTRIBUTES_COUNT, vbo_ids);
  initialized = true;
}

void ChartMarkEngine::clearData() {
  if (point_count == 0)
    return;

  point_count = 0;

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_WORLD_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_VERTEX_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_ORIGIN]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_SIZE]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_PIVOT]);
    glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  }
}

void ChartMarkEngine::setPatternTexture(GLuint tex_id, QVector2D size) {
  pattern_tex_id = tex_id;
  pattern_tex_size = size;
}

void ChartMarkEngine::setData(S52MarkLayer* layer, S52Assets* /*assets*/, S52References* ref) {
  std::vector<GLfloat> world_coords;
  std::vector<GLfloat> vertex_orders;
  std::vector<GLfloat> symbol_origins;
  std::vector<GLfloat> symbol_sizes;
  std::vector<GLfloat> symbol_pivots;

  if (layer->is_symbol_uniform) {
    is_pattern_uniform = true;
    patternOrigin = ref->getSymbolIndex(layer->symbol_ref);
    patternSize = ref->getSymbolDim(layer->symbol_ref);
    patternPivot = ref->getSymbolPivot(layer->symbol_ref);
  }

  for (unsigned int i = 0; i < (layer->points.size() / 2); i ++) {
    QVector2D tex_orig, tex_size, tex_pivt;
    if (!is_pattern_uniform) {
      tex_orig = ref->getSymbolIndex(layer->symbol_refs[i]);
      tex_size = ref->getSymbolDim(layer->symbol_refs[i]);
      tex_pivt = ref->getSymbolPivot(layer->symbol_refs[i]);
    }

    for (int k = 0; k < 4; k++) {
      world_coords.push_back(layer->points[2*i+0]);
      world_coords.push_back(layer->points[2*i+1]);

      vertex_orders.push_back(k);

      if (!is_pattern_uniform) {
        symbol_origins.push_back(tex_orig.x());
        symbol_origins.push_back(tex_orig.y());
        symbol_sizes.push_back(tex_size.x());
        symbol_sizes.push_back(tex_size.y());
        symbol_pivots.push_back(tex_pivt.x());
        symbol_pivots.push_back(tex_pivt.y());
      }
    }
  }

  point_count = vertex_orders.size();

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_WORLD_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, world_coords.size() * sizeof(GLfloat), &world_coords[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_VERTEX_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, vertex_orders.size() * sizeof(GLfloat), &vertex_orders[0], GL_STATIC_DRAW);

  if (!is_pattern_uniform) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_ORIGIN]);
    glBufferData(GL_ARRAY_BUFFER, symbol_origins.size() * sizeof(GLfloat), &symbol_origins[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_SIZE]);
    glBufferData(GL_ARRAY_BUFFER, symbol_sizes.size() * sizeof(GLfloat), &symbol_sizes[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_PIVOT]);
    glBufferData(GL_ARRAY_BUFFER, symbol_pivots.size() * sizeof(GLfloat), &symbol_pivots[0], GL_STATIC_DRAW);
  }
}

void ChartMarkEngine::draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle) {
  if (!initialized
   || pattern_tex_id == -1
   || point_count <= 0)
    return;

  glUniform2f(shaders->getMarkUniformLoc(COMMON_UNIFORMS_CENTER), cur_coords.x(), cur_coords.y());
  glUniform1f(shaders->getMarkUniformLoc(COMMON_UNIFORMS_SCALE), scale);
  glUniform1f(shaders->getMarkUniformLoc(COMMON_UNIFORMS_NORTH),  angle);
  glUniform2f(shaders->getMarkUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_DIM), pattern_tex_size.x(), pattern_tex_size.y());

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_WORLD_COORDS]);
  glVertexAttribPointer(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_WORLD_COORDS), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_WORLD_COORDS));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_VERTEX_ORDER]);
  glVertexAttribPointer(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_VERTEX_ORDER), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_VERTEX_ORDER));

  if (!is_pattern_uniform) {
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_ORIGIN), -1, -1);
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_SIZE), -1, -1);
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_PIVOT), -1, -1);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_ORIGIN]);
    glVertexAttribPointer(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_ORIGIN), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_ORIGIN));

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_SIZE]);
    glVertexAttribPointer(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_SIZE), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_SIZE));

    glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[MARK_ATTRIBUTES_SYMBOL_PIVOT]);
    glVertexAttribPointer(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_PIVOT), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
    glEnableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_PIVOT));
  } else {
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_ORIGIN), patternOrigin.x(), patternOrigin.y());
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_SIZE), patternSize.x(), patternSize.y());
    glUniform2f(shaders->getMarkUniformLoc(MARK_UNIFORMS_SYMBOL_PIVOT), patternPivot.x(), patternPivot.y());

    glDisableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_ORIGIN));
    glDisableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_SIZE));
    glDisableVertexAttribArray(shaders->getMarkAttributeLoc(MARK_ATTRIBUTES_SYMBOL_PIVOT));
  }

  glUniform1f(shaders->getLineUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_ID), 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pattern_tex_id);

  glDrawArrays(GL_QUADS, 0, point_count);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
}





ChartSndgEngine::ChartSndgEngine() {
  initialized = false;
  point_count = 0;
  pattern_tex_id = -1;
  vbo_ids = new GLuint[SNDG_ATTRIBUTES_COUNT];
}

ChartSndgEngine::~ChartSndgEngine() {
  if (initialized)
    glDeleteBuffers(SNDG_ATTRIBUTES_COUNT, vbo_ids);
  delete[] vbo_ids;
}

void ChartSndgEngine::init(const QGLContext* context) {
  if (initialized)
    return;

  initializeGLFunctions(context);
  glGenBuffers(SNDG_ATTRIBUTES_COUNT, vbo_ids);
  initialized = true;
}

void ChartSndgEngine::clearData() {
  if (!initialized)
    return;

  point_count = 0;

  /*glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_WORLD_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_VERTEX_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_FRAC]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_COUNT]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORIGIN]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_SIZE]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_PIVOT]);
  glBufferData(GL_ARRAY_BUFFER, 0, NULL, GL_STATIC_DRAW);*/
}

void ChartSndgEngine::setPatternTexture(GLuint tex_id, QVector2D size) {
  pattern_tex_id = tex_id;
  pattern_tex_size = size;
}

void ChartSndgEngine::setData(S52SndgLayer* layer, S52Assets* /*assets*/, S52References* ref) {
  std::vector<GLfloat> world_coords;
  std::vector<GLfloat> vertex_orders;
  std::vector<GLfloat> symbol_orders;
  std::vector<GLfloat> symbol_fracs;
  std::vector<GLfloat> symbol_counts;
  std::vector<GLfloat> symbol_origins;
  std::vector<GLfloat> symbol_sizes;
  std::vector<GLfloat> symbol_pivots;

  for (unsigned int i = 0; i < (layer->points.size() / 2); i++) {
    QVector2D tex_orig, tex_size, tex_pivt;
    QString depth = QString::number(layer->depths[i], 'f', 1);

    bool frac = false;
    for (int j = 0; j < depth.length(); j++) {
      if (depth[j] == '.') {
        frac = true;
        continue;
      }

      if (frac && depth[j] == '0')
        continue;

      QString symbol_ref = "SOUNDS0" + depth[j];
      tex_orig = ref->getSymbolIndex(symbol_ref);
      tex_size = ref->getSymbolDim(symbol_ref);
      tex_pivt = ref->getSymbolPivot(symbol_ref);

      for (int k = 0; k < 4; k++) {
        world_coords.push_back(layer->points[2*i+0]);
        world_coords.push_back(layer->points[2*i+1]);

        vertex_orders.push_back(k);

        if (!frac) {
          symbol_orders.push_back(j);
          symbol_fracs.push_back(0);
        } else {
          symbol_orders.push_back(j-1);
          symbol_fracs.push_back(1);
        }

        symbol_counts.push_back(depth.length() - 2);

        symbol_origins.push_back(tex_orig.x());
        symbol_origins.push_back(tex_orig.y());

        symbol_sizes.push_back(tex_size.x());
        symbol_sizes.push_back(tex_size.y());

        symbol_pivots.push_back(tex_pivt.x());
        symbol_pivots.push_back(tex_pivt.y());
      }
    }
  }

  point_count = vertex_orders.size();

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_WORLD_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, world_coords.size() * sizeof(GLfloat), &world_coords[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_VERTEX_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, vertex_orders.size() * sizeof(GLfloat), &vertex_orders[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORDER]);
  glBufferData(GL_ARRAY_BUFFER, symbol_orders.size() * sizeof(GLfloat), &symbol_orders[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_FRAC]);
  glBufferData(GL_ARRAY_BUFFER, symbol_fracs.size() * sizeof(GLfloat), &symbol_fracs[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_COUNT]);
  glBufferData(GL_ARRAY_BUFFER, symbol_counts.size() * sizeof(GLfloat), &symbol_counts[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORIGIN]);
  glBufferData(GL_ARRAY_BUFFER, symbol_origins.size() * sizeof(GLfloat), &symbol_origins[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_SIZE]);
  glBufferData(GL_ARRAY_BUFFER, symbol_sizes.size() * sizeof(GLfloat), &symbol_sizes[0], GL_STATIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_PIVOT]);
  glBufferData(GL_ARRAY_BUFFER, symbol_pivots.size() * sizeof(GLfloat), &symbol_pivots[0], GL_STATIC_DRAW);
}

void ChartSndgEngine::draw(ChartShaders* shaders, QVector2D cur_coords, float scale, float angle) {
  if (!initialized || pattern_tex_id == -1 || point_count <= 0)
    return;

  glUniform2f(shaders->getSndgUniformLoc(COMMON_UNIFORMS_CENTER), cur_coords.x(), cur_coords.y());
  glUniform1f(shaders->getSndgUniformLoc(COMMON_UNIFORMS_SCALE), scale);
  glUniform1f(shaders->getSndgUniformLoc(COMMON_UNIFORMS_NORTH),  angle);
  glUniform2f(shaders->getSndgUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_DIM), pattern_tex_size.x(), pattern_tex_size.y());

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_WORLD_COORDS]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_WORLD_COORDS), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_WORLD_COORDS));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_VERTEX_ORDER]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_VERTEX_ORDER), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_VERTEX_ORDER));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORDER]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_ORDER), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_ORDER));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_FRAC]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_FRAC), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_FRAC));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_COUNT]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_COUNT), 1, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_COUNT));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_ORIGIN]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_ORIGIN), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_ORIGIN));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_SIZE]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_SIZE), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_SIZE));

  glBindBuffer(GL_ARRAY_BUFFER, vbo_ids[SNDG_ATTRIBUTES_SYMBOL_PIVOT]);
  glVertexAttribPointer(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_PIVOT), 2, GL_FLOAT, GL_FALSE, 0, (void *) 0);
  glEnableVertexAttribArray(shaders->getSndgAttributeLoc(SNDG_ATTRIBUTES_SYMBOL_PIVOT));

  glUniform1f(shaders->getSndgUniformLoc(COMMON_UNIFORMS_PATTERN_TEX_ID), 0);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, pattern_tex_id);

  glDrawArrays(GL_QUADS, 0, point_count);
  glBindTexture(GL_TEXTURE_2D, 0);

  glFlush();
}
