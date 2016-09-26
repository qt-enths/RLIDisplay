#ifndef CONTROLSENGINE_H
#define CONTROLSENGINE_H

#define CIRCLE_POINTS 720

#include <QPoint>
#include <QGLFunctions>
#include <QGLShaderProgram>

class ControlsEngine : protected QGLFunctions {
public:
  explicit ControlsEngine();
  virtual ~ControlsEngine();

  bool init(const QGLContext* context);
  void draw();

  inline void setCursorPos(QPoint p) { _cursor = p; }
  inline QPoint getCursorPos() { return _cursor; }

  inline void setCenterPos(QPoint p) { _center = p; }
  inline QPoint getCenterPos() { return _center; }

  inline float getVnP() { return _vn_p; }
  inline float getVnCu() { return _vn_cu; }
  inline float getVd() { return _vd; }

  inline void shiftVnP(float d) { _vn_p += d; }
  inline void shiftVnCu(float d) {_vn_cu += d; if(_vn_cu < 0) _vn_cu = 360 + _vn_cu; if(_vn_cu >= 360) _vn_cu = _vn_cu - 360;}
  inline void shiftVd(float d) { _vd += d; if (_vd < 0) _vd = 0; }

  inline void setPlVisibility(bool val) { _is_pl_visible = val; }
  inline bool isPlVisible() { return _is_pl_visible; }

private:
  void initBuffers();
  void initShader();

  void drawCursor();
  void drawVN();
  void drawVD();
  void drawPL();
  void drawOZ();

  bool _initialized;

  QPoint _center;
  QPoint _cursor;

  float _vn_p;
  float _vn_cu;
  float _vd;

  float _oz_min_angle;
  float _oz_max_angle;
  float _oz_min_radius;
  float _oz_max_radius;
  float _oz_min_radius_step;
  float _oz_max_radius_step;

  bool _is_pl_visible;

  // Mask shader programs
  QGLShaderProgram* _prog;

  // -----------------------------------------------
  enum { CTRL_ATTR_ANGLE = 0
       , CTRL_ATTR_COUNT = 1 } ;

  GLuint vbo_ids [CTRL_ATTR_COUNT];

  // Ctrl shader uniform locations
  GLuint  loc_radius;
  GLuint  loc_color;
  // Ctrl line shader attributes locations
  GLuint  loc_angle;
};


#endif // CONTROLSENGINE_H
