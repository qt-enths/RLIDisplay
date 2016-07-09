#include "radarengine.h"

#include <algorithm>
#include <math.h>

#include <QDateTime>


RadarPalette::RadarPalette() {
  rgbRLI_Var = 0;
  brightnessRLI = 255;

  updatePalette();
}

void RadarPalette::setRgbVar(int val) {
  if (val < 0 || val > 1)
    return;

  rgbRLI_Var = val;
  updatePalette();
}

void RadarPalette::setBrightness(int val) {
  if (val < 0 || val > 255)
    return;

  brightnessRLI = val;
  updatePalette();
}

void RadarPalette::updatePalette() {
  // Палитра РЛИ
  // Первый индекс — день (1) и ночь (0)
  // В текущей реализации ПИКО используются только 0е вторые индексы. Остальные наборы данных прозапас
  static const rgbRLI_struct rgbRLI[2][4] = {
    // Rbg, Gbg, Bbg, R01, G01, B01, R08, G08, B08, R15, G15, B15, Rtk, Gtk, Btk, g01_08, g08_15
    { {  0,   0,   0,  10,  25,  60,  30, 140,  35, 160, 255, 140,   0, 120, 150,      5,     15 },
      { 10,  50,  50,   0,  72,  76,   0, 128, 160,   0, 184, 244, 150,  40,  80,      0,      0 },
      { 60,  30,  10,  76,  24,   0, 160,  52,   0, 244,  80,   0,  20,  60, 170,      0,      0 },
      { 50,  10,  50,  76,  24,  76, 160,  52, 160, 244,  80, 244,  60, 150,  60,      0,      0 } },
    { {  0,   0,   0,  30,  30,  80,  70, 110,  80, 230, 255,  80,   0, 120, 150,     10,     25 },
      { 10,  50,  50,   0,  72,  76,   0, 128, 160,   0, 184, 244, 150,  40,  80,      0,      0 },
      { 60,  30,  10,  76,  24,   0, 160,  52,   0, 244,  80,   0,  20,  60, 170,      0,      0 },
      { 50,  10,  50,  76,  24,  76, 160,  52, 160, 244,  80, 244,  60, 150,  60,      0,      0 } }
    };

  float br = ((float)brightnessRLI) / 255.f; // Вычисление коэффициента яркости

  // Расчёт цветового расстояния между точками на кривой преобразования амплитуды в цвет
  float kR[2][4], kG[2][4], kB[2][4];

  for (int i = 0; i < 4; i++) {
    kR[0][i] = ((float)(rgbRLI[rgbRLI_Var][i].R08 - rgbRLI[rgbRLI_Var][i].R01)) / 7.f;
    kG[0][i] = ((float)(rgbRLI[rgbRLI_Var][i].G08 - rgbRLI[rgbRLI_Var][i].G01)) / 7.f;
    kB[0][i] = ((float)(rgbRLI[rgbRLI_Var][i].B08 - rgbRLI[rgbRLI_Var][i].B01)) / 7.f;

    kR[1][i] = ((float)(rgbRLI[rgbRLI_Var][i].R15 - rgbRLI[rgbRLI_Var][i].R08)) / 7.f;
    kG[1][i] = ((float)(rgbRLI[rgbRLI_Var][i].G15 - rgbRLI[rgbRLI_Var][i].G08)) / 7.f;
    kB[1][i] = ((float)(rgbRLI[rgbRLI_Var][i].B15 - rgbRLI[rgbRLI_Var][i].B08)) / 7.f;
  }

  int n = 0;
  for (int j = 0; j < 16; j++) {
    float R, G, B;

    if (j == 0) {
      // Вычисление цвета фона
      R = br * ((float)rgbRLI[rgbRLI_Var][n].Rbg);
      G = br * ((float)rgbRLI[rgbRLI_Var][n].Gbg);
      B = br * ((float)rgbRLI[rgbRLI_Var][n].Bbg);
    } else if (j < 8) {
      // Вычисление RGBкодов для амплитуд j = 1..7 и наборов цветов n = 0..3
      float fj = (j - 1) ? (pow(((float)(j - 1)) / 7.f, exp(rgbRLI[rgbRLI_Var][n].gamma01_08 / 32.f)) * 7.f) : 0.f;
      R = br * (((float)rgbRLI[rgbRLI_Var][n].R01)+fj*kR[0][n]);
      G = br * (((float)rgbRLI[rgbRLI_Var][n].G01)+fj*kG[0][n]);
      B = br * (((float)rgbRLI[rgbRLI_Var][n].B01)+fj*kB[0][n]);
    } else {
      // Вычисление RGBкодов для амплитуд j = 8..15 и наборов цветов n = 0..3
      float fj = (j - 8) ? (pow(((float)(j - 8)) / 7.f, exp(rgbRLI[rgbRLI_Var][n].gamma08_15 / 32.f)) * 7.f) : 0.f;
      R = br * (((float)rgbRLI[rgbRLI_Var][n].R08)+fj*kR[1][n]);
      G = br * (((float)rgbRLI[rgbRLI_Var][n].G08)+fj*kG[1][n]);
      B = br * (((float)rgbRLI[rgbRLI_Var][n].B08)+fj*kB[1][n]);
    }

    //Значения R, G, B сохраняются в таблице для каждой амплитуды (0..15).
    palette[j][0] = R;
    palette[j][1] = G;
    palette[j][2] = B;
  }
}




RadarEngine::RadarEngine(uint pel_count, uint pel_len) {
  _initialized = false;

  _fbo_format.setAttachment(QGLFramebufferObject::Depth);
  _fbo_format.setMipmap(false);
  _fbo_format.setSamples(0);
  _fbo_format.setTextureTarget(GL_TEXTURE_2D);
  _fbo_format.setInternalTextureFormat(GL_RGBA8);

  _prog  = new QGLShaderProgram();
  _pal = new RadarPalette();

  resize(pel_count, pel_len);
}

RadarEngine::~RadarEngine() {
  if (_initialized) {
    glDeleteBuffers(ATTR_CNT, _vbo_ids);
    delete _render_fbo;
    delete _texture_fbo;
  }

  delete _prog;
}

void RadarEngine::resize(uint pel_count, uint pel_len) {
  _peleng_count = pel_count;
  _peleng_len   = pel_len;

  if (_initialized) {
    delete _render_fbo;
    delete _texture_fbo;
    _render_fbo = new QGLFramebufferObject(getSize(), getSize(), _fbo_format);
    _texture_fbo = new QGLFramebufferObject(getSize(), getSize());
    clearData();
    clearTexture();
  }
}

bool RadarEngine::init(const QGLContext* context) {
  if (_initialized)
    return false;

  initializeGLFunctions(context);

  _render_fbo = new QGLFramebufferObject(getSize(), getSize(), _fbo_format);
  _texture_fbo = new QGLFramebufferObject(getSize(), getSize());

  glGenBuffers(ATTR_CNT, _vbo_ids);

  initShader();
  clearData();

  _initialized = true;
  clearTexture();

  return _initialized;
}

void RadarEngine::initShader() {
  setlocale(LC_NUMERIC, "C");
  _prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/radar.vert.glsl");
  _prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/radar.frag.glsl");
  setlocale(LC_ALL, "");

  _prog->link();
  _prog->bind();

  _unif_locs[UNIF_CLR]   = _prog->uniformLocation("clear");
  _unif_locs[UNIF_PAL]   = _prog->uniformLocation("palette");
  _unif_locs[UNIF_THR]   = _prog->uniformLocation("threshold");

  _attr_locs[ATTR_POS] = _prog->attributeLocation("pos");
  _attr_locs[ATTR_FST] = _prog->attributeLocation("first");
  _attr_locs[ATTR_AMP] = _prog->attributeLocation("amp");
  _attr_locs[ATTR_DIV] = _prog->attributeLocation("div");

  _prog->release();
}

void RadarEngine::clearTexture() {
  if (!_initialized)
    return;

  glDepthFunc(GL_ALWAYS);
  _render_fbo->bind();
  glClearColor(1.f, 1.f, 1.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);

  glClearDepth(-2.f);
  glClear(GL_DEPTH_BUFFER_BIT);
  _render_fbo->release();

  _texture_fbo->bind();
  glClearColor(1.f, 1.f, 1.f, 0.f);
  glClear(GL_COLOR_BUFFER_BIT);
  _texture_fbo->release();

}

static double const PI = acos(-1);

void RadarEngine::clearData() {
  std::vector<float> poss, fsts, amps, divs;
  QVector<QPoint> points;

  for (uint index = 0; index < _peleng_count; index++) {
    for (uint radius = 0; radius < _peleng_len; radius++) {
      float angle = (2 * PI * static_cast<float>(index)) / _peleng_count;
      float x = round( static_cast<float>(radius) * sin(angle));
      float y = round(-static_cast<float>(radius) * cos(angle));

      QPoint point(static_cast<int>(x), static_cast<int>(y));

      poss.push_back(point.x());
      poss.push_back(point.y());

      if (!points.contains(point)) {
        fsts.push_back(1.f);
      } else {
        fsts.push_back(0.f);
        points.push_back(point);
      }

      amps.push_back(0.f);
      divs.push_back(1.f);
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_POS]);
  glBufferData(GL_ARRAY_BUFFER, 2*_peleng_count*_peleng_len*sizeof(GLfloat), poss.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_FST]);
  glBufferData(GL_ARRAY_BUFFER, _peleng_count*_peleng_len*sizeof(GLfloat), fsts.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_AMP]);
  glBufferData(GL_ARRAY_BUFFER, _peleng_count*_peleng_len*sizeof(GLfloat), amps.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_DIV]);
  glBufferData(GL_ARRAY_BUFFER, _peleng_count*_peleng_len*sizeof(GLfloat), divs.data(), GL_DYNAMIC_DRAW);

  _draw_circle       = true;
  _last_drawn_peleng = _peleng_count - 1;
  _last_added_peleng = _peleng_count - 1;
}

void RadarEngine::updateData(uint offset, uint count, float* divs, float* amps) {
  if (!_initialized)
    return;

  // Replicate divs
  float* long_divs = new float[count*_peleng_len];
  for (uint i = 0; i < count; i++)
    std::fill(&long_divs[i*_peleng_len], &long_divs[(i+1)*_peleng_len], divs[i]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_AMP]);
  glBufferSubData(GL_ARRAY_BUFFER, offset*_peleng_len*sizeof(GLfloat), count*_peleng_len*sizeof(GLfloat), amps);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_DIV]);
  glBufferSubData(GL_ARRAY_BUFFER, offset*_peleng_len*sizeof(GLfloat), count*_peleng_len*sizeof(GLfloat), long_divs);

  delete[] long_divs;

  // New last added peleng
  uint nlap = (offset + count - 1) % _peleng_count;

  // If we recieved full circle after last draw
  _draw_circle = (_last_added_peleng < _last_drawn_peleng && nlap >= _last_drawn_peleng) || count == _peleng_len;
  _last_added_peleng = nlap;
}

void RadarEngine::updateTexture() {
  if (!_initialized)
    return;

  glViewport(0, 0, getSize(), getSize());

  if (_last_added_peleng == _last_drawn_peleng && !_draw_circle)
    return;

  uint first_peleng_to_draw = (_last_drawn_peleng + 1) % _peleng_count;
  uint last_peleng_to_draw = _last_added_peleng % _peleng_count;

  if (_draw_circle)
    first_peleng_to_draw = (_last_added_peleng + 1) % _peleng_count;

  _render_fbo->bind();

  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0, getSize(), getSize(), 0, -2, 2);

  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glTranslatef(_peleng_len-.5f, _peleng_len-.5f, 0);

  _prog->bind();
  if (first_peleng_to_draw <= last_peleng_to_draw) {
    drawPelengs(first_peleng_to_draw, last_peleng_to_draw);
  } else {
    drawPelengs(first_peleng_to_draw, _peleng_count - 1);
    drawPelengs(0, last_peleng_to_draw);
  }
  _prog->release();

  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glMatrixMode( GL_PROJECTION );
  glPopMatrix();

  glFlush();

  _render_fbo->release();

  QRect rect(0, 0, _render_fbo->width(), _render_fbo->height());
  QGLFramebufferObject::blitFramebuffer(_texture_fbo, rect, _render_fbo, rect);

  _last_drawn_peleng = last_peleng_to_draw;
  _draw_circle = false;
}

void RadarEngine::drawPelengs(uint first, uint last) {
  if (last < first || last >= _peleng_count)
    return;

  glPointSize(1);

  glUniform3fv(_unif_locs[UNIF_PAL], 16*3, _pal->getPalette());
  glUniform1f(_unif_locs[UNIF_THR], 4);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_POS]);
  glVertexAttribPointer(_attr_locs[ATTR_POS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (2 * first * _peleng_len * sizeof(GLfloat)));
  glEnableVertexAttribArray(_attr_locs[ATTR_POS]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_FST]);
  glVertexAttribPointer(_attr_locs[ATTR_FST], 1, GL_FLOAT, GL_FALSE, 0, (void*) (first * _peleng_len * sizeof(GLfloat)));
  glEnableVertexAttribArray(_attr_locs[ATTR_FST]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_AMP]);
  glVertexAttribPointer(_attr_locs[ATTR_AMP], 1, GL_FLOAT, GL_FALSE, 0, (void*) (first * _peleng_len * sizeof(GLfloat)));
  glEnableVertexAttribArray(_attr_locs[ATTR_AMP]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ATTR_DIV]);
  glVertexAttribPointer(_attr_locs[ATTR_DIV], 1, GL_FLOAT, GL_FALSE, 0, (void*) (first * _peleng_len * sizeof(GLfloat)));
  glEnableVertexAttribArray(_attr_locs[ATTR_DIV]);

  glDepthFunc(GL_ALWAYS);
  glUniform1f(_unif_locs[UNIF_CLR], 1.f);
  glDrawArrays(GL_POINTS, 0, (last - first + 1) * _peleng_len);

  glFlush();

  glDepthFunc(GL_GREATER);
  glUniform1f(_unif_locs[UNIF_CLR], 0.f);
  glDrawArrays(GL_POINTS, 0, (last - first + 1) * _peleng_len);

  glFlush();
}