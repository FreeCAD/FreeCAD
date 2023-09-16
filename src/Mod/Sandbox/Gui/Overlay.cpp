/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer@users.sourceforge.net>        *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <QImage>
# include <QMouseEvent>
# include <QPainter>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoImage.h>
#endif
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>

#include <QtOpenGL.h>

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/GLPainter.h>
#include <Gui/NavigationStyle.h>

#include "Overlay.h"

using namespace SandboxGui;


class MyPaintable : public Gui::GLGraphicsItem
{
    QtGLFramebufferObject* fbo;
    Gui::View3DInventorViewer* view;
    QImage img;
public:
    ~MyPaintable()
    {
    }
    MyPaintable(Gui::View3DInventorViewer* v) :view(v), img(v->getGLWidget()->size(), QImage::Format_ARGB32)
    {
        img.fill(qRgba(255, 255, 255, 0));
        {
            QPainter p(&img);
            p.setPen(Qt::white);
            p.drawText(200,200,QString::fromLatin1("Render to QImage"));
        }

        fbo = new QtGLFramebufferObject(v->getGLWidget()->size());
        fbo->bind();
        //glClear(GL_COLOR_BUFFER_BIT);
        fbo->release();
        {
            //img = fbo->toImage();
            //img = QtGLWidget::convertToGLFormat(img);
        }
        //fbo->bind();
        //glEnable(GL_DEPTH_TEST);
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        //glDepthRange(0.1,1.0);
        //glEnable(GL_LINE_SMOOTH);
        //SoGLRenderAction a(SbViewportRegion(128,128));
        //a.apply(v->getSceneManager()->getSceneGraph());
        //fbo->release();
        //img = fbo->toImage();
        //img = QtGLWidget::convertToGLFormat(img);

        view->getSoRenderManager()->scheduleRedraw();
    }
 #ifndef GL_MULTISAMPLE
 #define GL_MULTISAMPLE  0x809D
 #endif
    void paintGL()
    {
    const SbViewportRegion vp = view->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, size[0], 0, size[1], -1, 1);

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4d(0.0,0.0,1.0,0.0f);
    glRasterPos2d(0,0);

    //http://wiki.delphigl.com/index.php/Multisampling
    //glDrawPixels(img.width(),img.height(),GL_RGBA,GL_UNSIGNED_BYTE,img.bits());
/*
    fbo->bind();
    GLuint* buf = new GLuint[size[0]*size[1]];
    glReadPixels(0, 0, size[0], size[1], GL_RGBA, GL_UNSIGNED_BYTE, buf);
    fbo->release();
    glDrawPixels(size[0],size[1],GL_RGBA,GL_UNSIGNED_BYTE,buf);
    delete [] buf;
*/
/*
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbo->texture());
    glBegin(GL_QUADS);
        glTexCoord2f(0.0,0.0);
        glVertex2f(-1.0,-1.0);
        glTexCoord2f(0.0,1.0);
        glVertex2f(-1.0,1.0);
        glTexCoord2f(1.0,1.0);
        glVertex2f(1.0,1.0);
        glTexCoord2f(1.0,0.0);
        glVertex2f(1.0,-1.0);
    glEnd();
    glDisable(GL_TEXTURE_2D);
*/


    glPopAttrib();
    glPopMatrix();
    }
};

class Teapots : public Gui::GLGraphicsItem
{
    QtGLFramebufferObject *fbObject;
    GLuint glTeapotObject;
    QPoint rubberBandCorner1;
    QPoint rubberBandCorner2;
    bool rubberBandIsShown;
    Gui::View3DInventorViewer* view;

public:
Teapots(Gui::View3DInventorViewer* v) :view(v)
{
    const SbViewportRegion vp = view->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    rubberBandIsShown = false;

//    makeCurrent();
    fbObject = new QtGLFramebufferObject(size[0],size[1],
                                         QtGLFramebufferObject::Depth);
    //initializeGL();
    resizeGL(size[0],size[1]);

    rubberBandIsShown = true;
    rubberBandCorner1.setX(200);
    rubberBandCorner1.setY(200);
    rubberBandCorner2.setX(800);
    rubberBandCorner2.setY(600);

    view->getSoRenderManager()->scheduleRedraw();
}

~Teapots()
{
    delete fbObject;
    glDeleteLists(glTeapotObject, 1);
}

void initializeGL()
{
    static const GLfloat ambient[] = { 0.0, 0.0, 0.0, 1.0 };
    static const GLfloat diffuse[] = { 1.0, 1.0, 1.0, 1.0 };
    static const GLfloat position[] = { 0.0, 3.0, 3.0, 0.0 };
    static const GLfloat lmodelAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    static const GLfloat localView[] = { 0.0 };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodelAmbient);
    glLightModelfv(GL_LIGHT_MODEL_LOCAL_VIEWER, localView);

    glFrontFace(GL_CW);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
}

void resizeGL(int width, int height)
{
#if 0
    fbObject->bind();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);

    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    if (width <= height) {
        glOrtho(0.0, 20.0, 0.0, 20.0 * GLfloat(height) / GLfloat(width),
                -10.0, 10.0);
    } else {
        glOrtho(0.0, 20.0 * GLfloat(width) / GLfloat(height), 0.0, 20.0,
                -10.0, 10.0);
    }
    glMatrixMode(GL_MODELVIEW);
    drawTeapots();

    fbObject->release();
#else
    (void)width;
    (void)height;
    fbObject->bind();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthRange(0.1,1.0);
    SoGLRenderAction gl(SbViewportRegion(fbObject->size().width(),fbObject->size().height()));
    gl.apply(view->getSoRenderManager()->getSceneGraph());
    fbObject->release();
#endif
}

void paintGL()
{
    const SbViewportRegion vp = view->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();


    glDisable(GL_LIGHTING);
    glViewport(0, 0, size[0], size[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fbObject->texture());
    glColor3f(1.0, 1.0, 1.0);
    GLfloat s = size[0] / GLfloat(fbObject->size().width());
    GLfloat t = size[1] / GLfloat(fbObject->size().height());

    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);
    glVertex2f(-1.0, -1.0);
    glTexCoord2f(s, 0.0);
    glVertex2f(1.0, -1.0);
    glTexCoord2f(s, t);
    glVertex2f(1.0, 1.0);
    glTexCoord2f(0.0, t);
    glVertex2f(-1.0, 1.0);
    glEnd();

    if (rubberBandIsShown) {
        glMatrixMode(GL_PROJECTION);
        glOrtho(0, size[0], size[1], 0, 0, 100);
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(4.0);
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glRecti(rubberBandCorner1.x(), rubberBandCorner1.y(),
                rubberBandCorner2.x(), rubberBandCorner2.y());
        glColor4f(1.0, 1.0, 0.0, 0.5);
        glLineStipple(3, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_LOOP);
        glVertex2i(rubberBandCorner1.x(), rubberBandCorner1.y());
        glVertex2i(rubberBandCorner2.x(), rubberBandCorner1.y());
        glVertex2i(rubberBandCorner2.x(), rubberBandCorner2.y());
        glVertex2i(rubberBandCorner1.x(), rubberBandCorner2.y());
        glEnd();

        glLineWidth(1.0);
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_BLEND);
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void mousePressEvent(QMouseEvent *event)
{
    rubberBandCorner1 = event->pos();
    rubberBandCorner2 = event->pos();
    rubberBandIsShown = true;
}

void mouseMoveEvent(QMouseEvent *event)
{
    if (rubberBandIsShown) {
        rubberBandCorner2 = event->pos();
//        updateGL();
    }
}

void mouseReleaseEvent(QMouseEvent * /* event */)
{
    if (rubberBandIsShown) {
        rubberBandIsShown = false;
//        updateGL();
    }
}

};

class Rubberband : public Gui::GLGraphicsItem
{
    QPoint rubberBandCorner1;
    QPoint rubberBandCorner2;
    Gui::View3DInventorViewer* view;

public:
Rubberband(Gui::View3DInventorViewer* v) :view(v)
{
    rubberBandCorner1.setX(200);
    rubberBandCorner1.setY(200);
    rubberBandCorner2.setX(800);
    rubberBandCorner2.setY(600);
    v->setRenderType(Gui::View3DInventorViewer::Image);
    v->getSoRenderManager()->scheduleRedraw();
}

~Rubberband()
{
}

void paintGL()
{
    const SbViewportRegion vp = view->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();


    //glDisable(GL_LIGHTING);
    //glViewport(0, 0, size[0], size[1]);
    //glMatrixMode(GL_PROJECTION);
    //glLoadIdentity();
    //glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    //glDisable(GL_DEPTH_TEST);

    //glClear(GL_COLOR_BUFFER_BIT);

        glMatrixMode(GL_PROJECTION);
        glOrtho(0, size[0], size[1], 0, 0, 100);
        glMatrixMode(GL_MODELVIEW);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glLineWidth(4.0);
        glColor4f(1.0f, 1.0f, 1.0f, 0.2f);
        glRecti(rubberBandCorner1.x(), rubberBandCorner1.y(),
                rubberBandCorner2.x(), rubberBandCorner2.y());
        glColor4f(1.0, 1.0, 0.0, 0.5);
        glLineStipple(3, 0xAAAA);
        glEnable(GL_LINE_STIPPLE);

        glBegin(GL_LINE_LOOP);
        glVertex2i(rubberBandCorner1.x(), rubberBandCorner1.y());
        glVertex2i(rubberBandCorner2.x(), rubberBandCorner1.y());
        glVertex2i(rubberBandCorner2.x(), rubberBandCorner2.y());
        glVertex2i(rubberBandCorner1.x(), rubberBandCorner2.y());
        glEnd();

        glLineWidth(1.0);
        glDisable(GL_LINE_STIPPLE);
        glDisable(GL_BLEND);

    //glEnable(GL_LIGHTING);
    //glEnable(GL_DEPTH_TEST);
}

};


void paintSelection()
{
#if 0
    SoAnnotation* hudRoot = new SoAnnotation;
    hudRoot->ref();

    SoOrthographicCamera* hudCam = new SoOrthographicCamera();
    hudCam->viewportMapping = SoCamera::LEAVE_ALONE;
    // Set the position in the window.
    // [0, 0] is in the center of the screen.
    //
    SoTranslation* hudTrans = new SoTranslation;
    hudTrans->translation.setValue(-1.0f, -1.0f, 0.0f);

    QImage image(100,100,QImage::Format_ARGB32_Premultiplied);
    image.fill(0x00000000);
    SoSFImage sfimage;
    Gui::BitmapFactory().convert(image, sfimage);
    SoImage* hudImage = new SoImage();
    hudImage->image = sfimage;

    // Assemble the parts...
    //
    hudRoot->addChild(hudCam);
    hudRoot->addChild(hudTrans);
    hudRoot->addChild(hudImage);

    Gui::View3DInventorViewer* viewer = this->getViewer();
    static_cast<SoGroup*>(viewer->getSceneGraph())->addChild(hudRoot);

    QWidget* gl = viewer->getGLWidget();
    DrawingPlane pln(hudImage->image, viewer, gl);
    gl->installEventFilter(&pln);
    QEventLoop loop;
    QObject::connect(&pln, SIGNAL(emitSelection()), &loop, SLOT(quit()));
    loop.exec();
    static_cast<SoGroup*>(viewer->getSceneGraph())->removeChild(hudRoot);
#endif
}

// ---------------------------------------
#if 0
void MeshSelection::prepareFreehandSelection(bool add)
{
    // a rubberband to select a rectangle area of the meshes
    Gui::View3DInventorViewer* viewer = this->getViewer();
    if (viewer) {
        stopInteractiveCallback(viewer);
        startInteractiveCallback(viewer, selectGLCallback);
        // set cross cursor
        DrawingPlane* brush = new DrawingPlane();
        //brush->setColor(1.0f,0.0f,0.0f);
        //brush->setLineWidth(3.0f);
        viewer->navigationStyle()->startSelection(brush);
        SoQtCursor::CustomCursor custom;
        custom.dim.setValue(16, 16);
        custom.hotspot.setValue(7, 7);
        custom.bitmap = cross_bitmap;
        custom.mask = cross_mask_bitmap;
        viewer->setComponentCursor(SoQtCursor(&custom));
        this->addToSelection = add;
    }
}
#endif
DrawingPlane::DrawingPlane()
{
    //image.fill(qRgba(255, 255, 255, 0));

    myPenWidth = 50;

    QRgb p = qRgba(255,255,0,0);
    int q = p;//((p << 16) & 0xff0000) | ((p >> 16) & 0xff) | (p & 0xff00ff00);
    int r = qRed(q);
    int g = qGreen(q);
    int b = qBlue(q);
    myPenColor = qRgb(r,g,b);//Qt::yellow;
    myRadius = 5.0f;
}

DrawingPlane::~DrawingPlane()
{
    terminate();
}

void DrawingPlane::initialize()
{
    fbo = new QtGLFramebufferObject(128, 128,QtGLFramebufferObject::Depth);
}

void DrawingPlane::terminate()
{
    fbo->bind();
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDepthRange(0.1,1.0);
    glEnable(GL_LINE_SMOOTH);
    SoGLRenderAction a(SbViewportRegion(128,128));
    a.apply(_pcView3D->getSoRenderManager()->getSceneGraph());
    fbo->release();
    fbo->toImage().save(QString::fromLatin1("C:/Temp/DrawingPlane.png"));
    delete fbo;
}

void DrawingPlane::draw ()
{return;
    if (1/*mustRedraw*/) {
        SbVec2s view = _pcView3D->getSoRenderManager()->getSize();
        static_cast<QtGLWidget*>(_pcView3D->getGLWidget())->makeCurrent();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, view[0], 0, view[1], -1, 1);

    // Store GL state
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    GLfloat depthrange[2];
    glGetFloatv(GL_DEPTH_RANGE, depthrange);
    GLdouble projectionmatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0,0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND);

    glLineWidth(1.0f);
    glColor4f(1.0, 1.0, 1.0, 0.0);
    glViewport(0, 0, view[0], view[1]);

    glEnable(GL_COLOR_LOGIC_OP);
    glLogicOp(GL_XOR);
    glDrawBuffer(GL_FRONT);


    //fbo->drawTexture(QPointF(), fbo->texture());

    glFlush();
    glLogicOp(GL_COPY);
    glDisable(GL_COLOR_LOGIC_OP);

    // Reset original state
    glDepthRange(depthrange[0], depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionmatrix);

    glPopAttrib();
    glPopMatrix();
    }
}

#include <Inventor/events/SoMouseButtonEvent.h>
int DrawingPlane::mouseButtonEvent(const SoMouseButtonEvent * const e, const QPoint& pos)
{
    const int button = e->getButton();
    const SbBool press = e->getState() == SoButtonEvent::DOWN ? true : false;

    if (press) {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON1:
            {
                scribbling = true;
                lastPoint = pos;
            }   break;
        default:
            {
            }   break;
        }
    }
    // release
    else {
        switch (button)
        {
        case SoMouseButtonEvent::BUTTON1:
            drawLineTo(pos);
            scribbling = false;
            return Finish;
        default:
            {
            }   break;
        }
    }

    return Continue;
}

int DrawingPlane::locationEvent(const SoLocation2Event * const, const QPoint& pos)
{
    if (scribbling) {
        drawLineTo(pos);

        // filter out some points
        if (selection.isEmpty()) {
            selection << pos;
        }
        else {
            const QPoint& top = selection.last();
            if (abs(top.x()-pos.x()) > 20 ||
                abs(top.y()-pos.y()) > 20)
                selection << pos;
        }

        draw();
    }

    return Continue;
}

int DrawingPlane::keyboardEvent(const SoKeyboardEvent * const)
{
    return Continue;
}

void DrawingPlane::drawLineTo(const QPoint &endPoint)
{
    Q_UNUSED(endPoint);
    return;
}
    //Gui::Document* doc = Gui::Application::Instance->activeDocument();
    //Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventor*>(doc->getActiveView())->getViewer();
    ////view->addGraphicsItem(new MyPaintable(view));
    ////view->addGraphicsItem(new Teapots(view));
    //view->addGraphicsItem(new Rubberband(view));
    //....
    //Gui::Document* doc = Gui::Application::Instance->activeDocument();
    //Gui::View3DInventorViewer* view = static_cast<Gui::View3DInventor*>(doc->getActiveView())->getViewer();
    //view->clearGraphicsItems();
