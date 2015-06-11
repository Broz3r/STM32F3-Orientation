#ifndef MYGLWIDGET_H
#define MYGLWIDGET_H

#include <QtOpenGL>
#include <QGLWidget>

class MyGLWidget : public QGLWidget{

  Q_OBJECT // must include this if you use Qt signals/slots


public:
    MyGLWidget(QWidget *parent = NULL);

public slots:
    void setXAngle(int angle) {m_XAngle = angle;}
    void setYAngle(int angle) {m_YAngle = angle;}
    void setZAngle(int angle) {m_ZAngle = angle;}

private :
    int m_XAngle;
    int m_YAngle;
    int m_ZAngle;

protected:
    // Set up the rendering context, define display lists etc.:
   void initializeGL();
   // draw the scene:
   void paintGL();
   // setup viewport, projection etc.:
   void resizeGL (int width, int height);
};

#endif // MYGLWIDGET_H
