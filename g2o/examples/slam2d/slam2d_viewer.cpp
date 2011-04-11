#include "slam2d_viewer.h"

#include "draw_helpers.h"
#include "g2o/types/slam2d/vertex_se2.h"
#include "g2o/types/slam2d/vertex_point_xy.h"
#include "g2o/core/graph_optimizer_sparse.h"

#include <Eigen/Core>
#include <iostream>
using namespace std;

namespace g2o {

namespace {

  /**
   * \brief helper for setting up a camera for qglviewer
   */
  class StandardCamera : public qglviewer::Camera
  {
    public:
      StandardCamera() : _standard(true) {};

      float zNear() const {
        if (_standard) 
          return 0.001; 
        else 
          return Camera::zNear(); 
      }

      float zFar() const
      {  
        if (_standard) 
          return 1000.0; 
        else 
          return Camera::zFar();
      }

      const bool& standard() const {return _standard;}
      bool& standard() {return _standard;}

    private:
      bool _standard;
  };

  void drawSE2(const VertexSE2* v)
  {
    static const double len = 0.2;
    static Eigen::Vector2d p1( 0.75 * len,  0.);
    static Eigen::Vector2d p2(-0.25 * len,  0.5 * len);
    static Eigen::Vector2d p3(-0.25 * len, -0.5 * len);

    const SE2& pose = v->estimate();

    Eigen::Vector2d aux = pose * p1;
    glVertex3f(aux[0], aux[1], 0.f);
    aux = pose * p2;
    glVertex3f(aux[0], aux[1], 0.f);
    aux = pose * p3;
    glVertex3f(aux[0], aux[1], 0.f);
  }

  template <typename Derived>
  void drawCov(const Eigen::Vector2d& p, const Eigen::MatrixBase<Derived>& cov)
  {
    const double scalingFactor = 1.;

    glPushMatrix();
    glTranslatef(p.x(), p.y(), 0.f);

    const typename Derived::Scalar& a = cov(0, 0); 
    const typename Derived::Scalar& b = cov(0, 1); 
    const typename Derived::Scalar& d = cov(1, 1);

    /* get eigen-values */
    double D = a*d - b*b; // determinant of the matrix
    double T = a+d;       // Trace of the matrix
    double h = sqrt(0.25*(T*T) - D);
    double lambda1 = 0.5*T + h;  // solving characteristic polynom using p-q-formula
    double lambda2 = 0.5*T - h;

    double theta     = 0.5 * atan2(2.0 * b, a - d);
    double majorAxis = 3.0 * sqrt(lambda1);
    double minorAxis = 3.0 * sqrt(lambda2);


    glRotatef(RAD2DEG(theta), 0.f, 0.f, 1.f);
    glScalef(majorAxis * scalingFactor, minorAxis * scalingFactor, 1.f);
    glColor4f(1.0f, 1.f, 0.f, 0.4f);
    drawDisk(1.f);
    glColor4f(0.f, 0.f, 0.f, 1.0f);
    drawCircle(1.f);
    glPopMatrix();
  }

} // end anonymous namespace

Slam2DViewer::Slam2DViewer(QWidget* parent, const QGLWidget* shareWidget, Qt::WFlags flags) :
  QGLViewer(parent, shareWidget, flags),
  graph(0), drawCovariance(false)
{
}

Slam2DViewer::~Slam2DViewer()
{
}

void Slam2DViewer::draw()
{
  if (! graph)
    return;

  // drawing the graph
  glColor4f(0.00f, 0.67f, 1.00f, 1.f);
  glBegin(GL_TRIANGLES);
  for (SparseOptimizer::VertexIDMap::iterator it = graph->vertices().begin(); it != graph->vertices().end(); ++it) {
    VertexSE2* v = dynamic_cast<VertexSE2*>(it->second);
    if (v) {
      drawSE2(v);
    }
  }
  glEnd();

  glColor4f(1.00f, 0.67f, 0.00f, 1.f);
  glPointSize(2.f);
  glBegin(GL_POINTS);
  for (SparseOptimizer::VertexIDMap::iterator it = graph->vertices().begin(); it != graph->vertices().end(); ++it) {
    VertexPointXY* v = dynamic_cast<VertexPointXY*>(it->second);
    if (v) {
      glVertex3f(v->estimate()(0), v->estimate()(1), 0.f);
    }
  }
  glEnd();
  glPointSize(1.f);

  if (drawCovariance) {
    for (SparseOptimizer::VertexIDMap::iterator it = graph->vertices().begin(); it != graph->vertices().end(); ++it) {
      VertexSE2* v = dynamic_cast<VertexSE2*>(it->second);
      if (v) {
        drawCov(v->estimate().translation(), v->uncertainty());
      }
    }
  }
}

void Slam2DViewer::init()
{
  QGLViewer::init();

  // some default settings i like
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND); 
  glEnable(GL_DEPTH_TEST);
  glShadeModel(GL_SMOOTH);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  setAxisIsDrawn();

  // don't save state
  setStateFileName(QString::null);

  // mouse bindings
  setMouseBinding(Qt::RightButton, CAMERA, ZOOM);
  setMouseBinding(Qt::MidButton, CAMERA, TRANSLATE);

  // keyboard shortcuts
  setShortcut(CAMERA_MODE, 0);
  setShortcut(EXIT_VIEWER, 0);
  //setShortcut(SAVE_SCREENSHOT, 0);

  // replace camera
  qglviewer::Camera* oldcam = camera();
  qglviewer::Camera* cam = new StandardCamera();
  setCamera(cam);
  cam->setPosition(qglviewer::Vec(0., 0., 75.));
  cam->setUpVector(qglviewer::Vec(0., 1., 0.));
  cam->lookAt(qglviewer::Vec(0., 0., 0.));
  delete oldcam;
}

} // end namespace
