#ifndef RADARENGINE_H
#define RADARENGINE_H

#include <QTime>
#include <QColor>
#include <QVector2D>

#include <QtOpenGL/QGLFunctions>
#include <QtOpenGL/QGLFramebufferObject>
#include <QtOpenGL/QGLShaderProgram>


class RadarPalette {
public:
  RadarPalette();

  void setRgbVar(int var);
  void setBrightness(int br);

  inline float* getPalette() { return &palette[0][0]; }

private:
  // Расчёт зависимости RGBкодов цвета от амплитуды входного сигнала
  void updatePalette();

  // Параметры:
  int rgbRLI_Var;         //Текущая палитры (день/ночь)
  int brightnessRLI;      //Яркость РЛИ 0..255

  float palette[16][3];

  // Описание палитры РЛИ
  typedef struct rgbRLI_struct {
    unsigned char Rbg,Gbg,Bbg; //RGB фона
    unsigned char R01,G01,B01; //RGB для 1й градации РЛИ
    unsigned char R08,G08,B08; //RGB для 8й градации РЛИ
    unsigned char R15,G15,B15; //RGB для 15й градации РЛИ
    unsigned char Rtk,Gtk,Btk; //RGB следов
    float gamma01_08; //линейность яркости от 1й до 8й градации РЛИ
    float gamma08_15; //линейность яркости от 8й до 15й градации РЛИ
  } rgbRLI_struct;
};


class RadarEngine : public QObject, protected QGLFunctions {
  Q_OBJECT
public:
  explicit RadarEngine  (uint pel_count, uint pel_len);
  virtual ~RadarEngine  ();

  bool init           (const QGLContext* context);
  void resizeData     (uint pel_count, uint pel_len);
  void resizeTexture  (uint radius);
  void shiftCenter    (QPoint center);

  inline uint  getSize()        { return 2*_radius + 1; }
  inline uint  getTextureId ()  { return _fbo->texture(); }

public slots:
  void clearTexture();
  void clearData();

  void onBrightnessChanged(int br);

  void updateTexture();
  void updateData(uint offset, uint count, GLubyte* amps);

private:
  void initShader();
  void drawPelengs(uint first, uint last);

  bool _initialized;

  // Radar parameters
  QPoint  _center;
  uint    _radius;
  uint    _peleng_count;
  uint    _peleng_len;

  bool  _draw_circle;
  uint  _last_drawn_peleng;
  uint  _last_added_peleng;

  // Framebuffer vars
  QGLFramebufferObjectFormat _fbo_format;
  QGLFramebufferObject* _fbo;
  QGLShaderProgram* _prog;

  enum { ATTR_POS = 0, ATTR_FST = 1, ATTR_AMP = 2, ATTR_CNT = 3 } ;
  enum { UNIF_CLR = 0, UNIF_PAL = 1, UNIF_THR = 2, UNIF_CNT = 3 } ;

  GLuint _vbo_ids[ATTR_CNT];
  GLuint _unif_locs[UNIF_CNT];
  GLuint _attr_locs[ATTR_CNT];

  RadarPalette* _pal;
};

#endif // RADARENGINE_H
