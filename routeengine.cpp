#include "routeengine.h"

RouteEngine::RouteEngine(QObject* parent) : QObject(parent), QGLFunctions() {
  _initialized = false;

  QList<QVector2D> route0;
  route0.push_back(QVector2D(12.7000f, -81.6000f));
  route0.push_back(QVector2D(12.6000f, -81.5000f));
  route0.push_back(QVector2D(12.4000f, -81.5500f));
  _routes.push_back(route0);

  _current = 0;
}

RouteEngine::~RouteEngine() {
  if (_initialized) {
    delete _prog;
    glDeleteBuffers(ROUTE_ATTR_COUNT, _vbo_ids);
  }
}

void RouteEngine::clearCurrentRoute() {
  _routes[_current].clear();
}

void RouteEngine::addPointToCurrent(const QVector2D& p) {
  _routes[_current].push_back(p);
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

  glPointSize(5);
  glDrawArrays(GL_POINTS, 0, pCount);

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

  _attr_locs[ROUTE_ATTR_COORDS] = _prog->attributeLocation("world_coords");

  _unif_locs[ROUTE_UNIF_CENTER] = _prog->uniformLocation("center");
  _unif_locs[ROUTE_UNIF_SCALE] = _prog->uniformLocation("scale");

  _prog->release();

  // Restore system locale
  setlocale(LC_ALL, "");
}

int RouteEngine::loadBuffers() {
  std::vector<GLfloat> points;

  QList<QVector2D>::const_iterator it;
  for (it = _routes.at(_current).begin(); it != _routes.at(_current).end(); it++) {
    points.push_back( (*it).x() );
    points.push_back( (*it).y() );
  }

  glBindBuffer(GL_ARRAY_BUFFER, _vbo_ids[ROUTE_ATTR_COORDS]);
  glBufferData(GL_ARRAY_BUFFER, points.size()*sizeof(GLfloat), points.data(), GL_DYNAMIC_DRAW);
  glVertexAttribPointer(_attr_locs[ROUTE_ATTR_COORDS], 2, GL_FLOAT, GL_FALSE, 0, (void*) (0));
  glEnableVertexAttribArray(_attr_locs[ROUTE_ATTR_COORDS]);

  return points.size() / 2;
}
