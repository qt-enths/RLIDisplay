#include "routeengine.h"

RouteEngine::RouteEngine(QObject* parent) : QObject(parent), QGLFunctions(), _routes(200) {
  _initialized = false;

  _currentRoute.push_back(QVector2D(12.7000f, -81.6000f));
  _currentRoute.push_back(QVector2D(12.6000f, -81.5000f));
  _currentRoute.push_back(QVector2D(12.4000f, -81.5500f));
}

RouteEngine::~RouteEngine() {
  if (_initialized) {
    delete _prog;
    glDeleteBuffers(ROUTE_ATTR_COUNT, _vbo_ids);
  }
}

void RouteEngine::clearCurrentRoute() {
  _routesMutex.lock();
  _currentRoute.clear();
  _routesMutex.unlock();
}

void RouteEngine::addPointToCurrent(const QVector2D& p) {
  _routesMutex.lock();
  _currentRoute.push_back(p);
  _routesMutex.unlock();
}

void RouteEngine::removePointFromCurrent() {
  _routesMutex.lock();
  if (_currentRoute.size() > 1)
    _currentRoute.removeLast();
  _routesMutex.unlock();
}

void RouteEngine::loadFrom(int index) {
  if (index >= 0 && index < _routes.size())
    _currentRoute = _routes[index];
}

void RouteEngine::saveTo(int index) {
  if (index >= 0 && index < _routes.size())
    _routes[index] = _currentRoute;
}

bool RouteEngine::init(const QGLContext* context) {
  Q_UNUSED(context);

  if (_initialized) return false;

  initializeGLFunctions(context);

  _prog = new QGLShaderProgram();

  glGenBuffers(ROUTE_ATTR_COUNT, _vbo_ids);

  initShader();

  _initialized = true;
  return _initialized;
}

void RouteEngine::draw(QVector2D center_coords, float scale) {
  _routesMutex.lock();

  _prog->bind();

  glUniform1f(_unif_locs[ROUTE_UNIF_SCALE], scale);
  glUniform2f(_unif_locs[ROUTE_UNIF_CENTER], center_coords.x(), center_coords.y());

  int pCount = loadBuffers();

  glUniform1f(_unif_locs[ROUTE_UNIF_TYPE], 0);
  glPointSize(5);
  glDrawArrays(GL_POINTS, 0, pCount);

  glPushAttrib(GL_ENABLE_BIT);

  glLineStipple(1, 0xF0F0);
  glEnable(GL_LINE_STIPPLE);

  glLineWidth(1);
  glDrawArrays(GL_LINE_STRIP, 0, pCount);

  glPopAttrib();

  glUniform1f(_unif_locs[ROUTE_UNIF_TYPE], 1);
  glLineWidth(1);
  glDrawArrays(GL_LINE_STRIP, 0, pCount);

  glUniform1f(_unif_locs[ROUTE_UNIF_TYPE], 2);
  glLineWidth(1);
  glDrawArrays(GL_LINE_STRIP, 0, pCount);

  _prog->release();

  _routesMutex.unlock();
}

void RouteEngine::initShader() {
  // Overriding system locale until shaders are compiled
  setlocale(LC_NUMERIC, "C");

  _prog->addShaderFromSourceFile(QGLShader::Vertex, ":/res/shaders/route.vert.glsl");
  _prog->addShaderFromSourceFile(QGLShader::Fragment, ":/res/shaders/route.frag.glsl");

  _prog->link();
  _prog->bind();

  _attr_locs[ROUTE_ATTR_PREV_COORDS] = _prog->attributeLocation("prev_world_coords");
  _attr_locs[ROUTE_ATTR_CURR_COORDS] = _prog->attributeLocation("curr_world_coords");
  _attr_locs[ROUTE_ATTR_NEXT_COORDS] = _prog->attributeLocation("next_world_coords");

  _unif_locs[ROUTE_UNIF_CENTER] = _prog->uniformLocation("center");
  _unif_locs[ROUTE_UNIF_SCALE] = _prog->uniformLocation("scale");
  _unif_locs[ROUTE_UNIF_TYPE] = _prog->uniformLocation("type");

  _prog->release();

  // Restore system locale
  setlocale(LC_ALL, "");
}

int RouteEngine::loadBuffers() {
  std::vector<GLfloat> prev_points;
  std::vector<GLfloat> curr_points;
  std::vector<GLfloat> next_points;

  QList<QVector2D>::const_iterator it;
  for (it = _currentRoute.begin(); it != _currentRoute.end(); it++) {
    curr_points.push_back( (*it).x() );
    curr_points.push_back( (*it).y() );

    if (it == _currentRoute.begin()) {
      prev_points.push_back( (*it).x() );
      prev_points.push_back( (*it).y() );
    } else {
      prev_points.push_back( (*(it-1)).x() );
      prev_points.push_back( (*(it-1)).y() );
    }

    if ((it+1) == _currentRoute.end()) {
      next_points.push_back( (*it).x() );
      next_points.push_back( (*it).y() );
    } else {
      next_points.push_back( (*(it+1)).x() );
      next_points.push_back( (*(it+1)).y() );
    }
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ROUTE_ATTR_PREV_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, prev_points.size()*sizeof(GLfloat), prev_points.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(_attr_locs[ROUTE_ATTR_PREV_COORDS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[ROUTE_ATTR_PREV_COORDS]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ROUTE_ATTR_CURR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, curr_points.size()*sizeof(GLfloat), curr_points.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(_attr_locs[ROUTE_ATTR_CURR_COORDS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[ROUTE_ATTR_CURR_COORDS]);

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ROUTE_ATTR_NEXT_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, next_points.size()*sizeof(GLfloat), next_points.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(_attr_locs[ROUTE_ATTR_NEXT_COORDS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[ROUTE_ATTR_NEXT_COORDS]);

  return curr_points.size() / 2;
}
