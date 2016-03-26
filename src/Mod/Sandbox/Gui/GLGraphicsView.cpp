/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#endif

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QGLWidget>
#include <QGraphicsView>
#include <QPaintEngine>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>

#include "GLGraphicsView.h"
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

using namespace Gui;

// http://doc.qt.digia.com/qq/qq26-openglcanvas.html

class GraphicsView : public QGraphicsView
{
public:
    GraphicsView()
    {
    }

protected:
    void resizeEvent(QResizeEvent *event) {
        if (scene())
            scene()->setSceneRect(
                QRect(QPoint(0, 0), event->size()));
        QGraphicsView::resizeEvent(event);
    }
};


#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>

QDialog *GraphicsScene::createDialog(const QString &windowTitle) const
{
    QDialog *dialog = new QDialog(0, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    dialog->setWindowOpacity(0.8);
    dialog->setWindowTitle(windowTitle);
    dialog->setLayout(new QVBoxLayout);

    return dialog;
}

GraphicsScene::GraphicsScene()
    : m_backgroundColor(0, 170, 255)
    , m_lastTime(0)
    , m_distance(1.4f)
{
    rootNode = new SoSeparator();
    rootNode->ref();
    sceneCamera = new SoOrthographicCamera();
    rootNode->addChild(sceneCamera);
    rootNode->addChild(new SoDirectionalLight());
    sceneNode = new SoSeparator();
    sceneNode->ref();
    rootNode->addChild(sceneNode);

    this->addEllipse(20,20, 120, 60);
    QWidget *controls = createDialog(tr("Controls"));

    QCheckBox *wireframe = new QCheckBox(tr("Render as wireframe"));
    //connect(wireframe, SIGNAL(toggled(bool)), this, SLOT(enableWireframe(bool)));
    controls->layout()->addWidget(wireframe);

    QCheckBox *normals = new QCheckBox(tr("Display normals vectors"));
    //connect(normals, SIGNAL(toggled(bool)), this, SLOT(enableNormals(bool)));
    controls->layout()->addWidget(normals);

    QPushButton *colorButton = new QPushButton(tr("Choose model color"));
    controls->layout()->addWidget(colorButton);

    QWidget *instructions = createDialog(tr("Instructions"));
    instructions->layout()->addWidget(new QLabel(tr("Use mouse wheel to zoom model, and click and drag to rotate model")));
    instructions->layout()->addWidget(new QLabel(tr("Move the sun around to change the light position")));

    QGraphicsProxyWidget* g1 = addWidget(instructions);
    g1->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    QGraphicsProxyWidget* g2 = addWidget(controls);
    g2->setWindowFlags(Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    controls->setAttribute(Qt::WA_TranslucentBackground);

    QPointF pos(10, 10);
    Q_FOREACH (QGraphicsItem *item, items()) {
        item->setFlag(QGraphicsItem::ItemIsMovable);
        item->setCacheMode(QGraphicsItem::DeviceCoordinateCache);

        const QRectF rect = item->boundingRect();
        item->setPos(pos.x() - rect.x(), pos.y() - rect.y());
        pos += QPointF(0, 10 + rect.height());
    }

    //QRadialGradient gradient(40, 40, 40, 40, 40);
    //gradient.setColorAt(0.2, Qt::yellow);
    //gradient.setColorAt(1, Qt::transparent);

    //m_lightItem = new QGraphicsRectItem(0, 0, 80, 80);
    //m_lightItem->setPen(Qt::NoPen);
    //m_lightItem->setBrush(gradient);
    //m_lightItem->setFlag(QGraphicsItem::ItemIsMovable);
    //m_lightItem->setPos(800, 200);
    //addItem(m_lightItem);

    m_time.start();
}

GraphicsScene::~GraphicsScene()
{
    sceneNode->unref();
    rootNode->unref();
}

void GraphicsScene::viewAll()
{
    sceneCamera->viewAll(rootNode, SbViewportRegion(width(),height()));
}

SoSeparator* GraphicsScene::getSceneGraph() const
{
    return sceneNode;
}

#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGLRenderAction.h>

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &)
{
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL && painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("GraphicsScene: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
        return;
    }

    glViewport(0, 0, width(), height());
/**/
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //glDepthRange(0.1,1.0); //

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    //gluPerspective(70, width() / height(), 0.01, 1000);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    //const float pos[] = { m_lightItem->x() - width() / 2, height() / 2 - m_lightItem->y(), 512, 0 };
    //glLightfv(GL_LIGHT0, GL_POSITION, pos);
    //glColor4f(m_modelColor.redF(), m_modelColor.greenF(), m_modelColor.blueF(), 1.0f);

    const int delta = m_time.elapsed() - m_lastTime;
    m_lastTime += delta;

    //glTranslatef(0, 0, -m_distance);
    //glRotatef(m_rotation.x, 1, 0, 0);
    //glRotatef(m_rotation.y, 0, 1, 0);
    //glRotatef(m_rotation.z, 0, 0, 1);

    //glEnable(GL_MULTISAMPLE);
    //m_model->render(m_wireframeEnabled, m_normalsEnabled);
    //glDisable(GL_MULTISAMPLE);

    glPopMatrix();

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
/**/
/**/
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glViewport(0, 0, width(), height());
        glEnable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDepthRange(0.1,1.0);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();


    SoGLRenderAction gl(SbViewportRegion(width(),height()));
    gl.apply(rootNode);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();

    glPopAttrib();

    glViewport(0, 0, width(), height());
    glColor3f(1,1,1);
    glLineWidth(4);
    glBegin(GL_LINES);
        glVertex3i(0, 0, 0);
        glVertex3i(400, 400, 0);
    glEnd();
/**/

    painter->save();
    painter->fillRect(40,40,40,60,Qt::lightGray);
    painter->drawText(50,50, QString::fromLatin1("Done with QPainter"));
    painter->restore();

    QTimer::singleShot(20, this, SLOT(update()));
}

void GraphicsScene::setBackgroundColor(const QColor& color)
{
    if (color.isValid()) {
        m_backgroundColor = color;
        update();
    }
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    if (event->isAccepted())
        return;
    if (event->buttons() & Qt::LeftButton) {
        const QPointF delta = event->scenePos() - event->lastScenePos();
        //const Point3d angularImpulse = Point3d(delta.y(), delta.x(), 0) * 0.1;

        //m_rotation += angularImpulse;
        //m_accumulatedMomentum += angularImpulse;

        event->accept();
        update();
    }
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

    m_mouseEventTime = m_time.elapsed();
    //m_angularMomentum = m_accumulatedMomentum = Point3d();
    event->accept();
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    const int delta = m_time.elapsed() - m_mouseEventTime;
    //m_angularMomentum = m_accumulatedMomentum * (1000.0 / qMax(1, delta));
    event->accept();
    update();
}

void GraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    QGraphicsScene::wheelEvent(event);
    if (event->isAccepted())
        return;

    //m_distance *= qPow(1.2, -event->delta() / 120);
    event->accept();
    update();
}


GraphicsView3D::GraphicsView3D(Gui::Document* doc, QWidget* parent)
  : Gui::MDIView(doc, parent), m_scene(new GraphicsScene()), m_view(new GraphicsView)
{
    m_view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setScene(m_scene);

    std::vector<ViewProvider*> v = doc->getViewProvidersOfType(ViewProvider::getClassTypeId());
    for (std::vector<ViewProvider*>::iterator it = v.begin(); it != v.end(); ++it)
        m_scene->getSceneGraph()->addChild((*it)->getRoot());
    setCentralWidget(m_view);
    m_scene->viewAll();

    // attach parameter Observer
    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->Attach(this);

    OnChange(*hGrp,"BackgroundColor");
}

GraphicsView3D::~GraphicsView3D()
{
    hGrp->Detach(this);
}

void GraphicsView3D::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);
#if 0
    if (strcmp(Reason,"HeadlightColor") == 0) {
        unsigned long headlight = rGrp.GetUnsigned("HeadlightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor headlightColor;
        headlightColor.setPackedValue((uint32_t)headlight, transparency);
        _viewer->getHeadlight()->color.setValue(headlightColor);
    }
    else if (strcmp(Reason,"HeadlightDirection") == 0) {
        std::string pos = rGrp.GetASCII("HeadlightDirection");
        QString flt = QString::fromLatin1("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromLatin1("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            _viewer->getHeadlight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"HeadlightIntensity") == 0) {
        long value = rGrp.GetInt("HeadlightIntensity", 100);
        _viewer->getHeadlight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnableBacklight") == 0) {
        _viewer->setBacklight(rGrp.GetBool("EnableBacklight", false));
    }
    else if (strcmp(Reason,"BacklightColor") == 0) {
        unsigned long backlight = rGrp.GetUnsigned("BacklightColor",ULONG_MAX); // default color (white)
        float transparency;
        SbColor backlightColor;
        backlightColor.setPackedValue((uint32_t)backlight, transparency);
        _viewer->getBacklight()->color.setValue(backlightColor);
    }
    else if (strcmp(Reason,"BacklightDirection") == 0) {
        std::string pos = rGrp.GetASCII("BacklightDirection");
        QString flt = QString::fromLatin1("([-+]?[0-9]+\\.?[0-9]+)");
        QRegExp rx(QString::fromLatin1("^\\(%1,%1,%1\\)$").arg(flt));
        if (rx.indexIn(QLatin1String(pos.c_str())) > -1) {
            float x = rx.cap(1).toFloat();
            float y = rx.cap(2).toFloat();
            float z = rx.cap(3).toFloat();
            _viewer->getBacklight()->direction.setValue(x,y,z);
        }
    }
    else if (strcmp(Reason,"BacklightIntensity") == 0) {
        long value = rGrp.GetInt("BacklightIntensity", 100);
        _viewer->getBacklight()->intensity.setValue((float)value/100.0f);
    }
    else if (strcmp(Reason,"EnablePreselection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableHighlightAction cAct(rclGrp.GetBool("EnablePreselection", true));
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"EnableSelection") == 0) {
        const ParameterGrp& rclGrp = ((ParameterGrp&)rCaller);
        SoFCEnableSelectionAction cAct(rclGrp.GetBool("EnableSelection", true));
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"HighlightColor") == 0) {
        float transparency;
        SbColor highlightColor(0.8f, 0.1f, 0.1f);
        unsigned long highlight = (unsigned long)(highlightColor.getPackedValue());
        highlight = rGrp.GetUnsigned("HighlightColor", highlight);
        highlightColor.setPackedValue((uint32_t)highlight, transparency);
        SoSFColor col; col.setValue(highlightColor);
        SoFCHighlightColorAction cAct(col);
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"SelectionColor") == 0) {
        float transparency;
        SbColor selectionColor(0.1f, 0.8f, 0.1f);
        unsigned long selection = (unsigned long)(selectionColor.getPackedValue());
        selection = rGrp.GetUnsigned("SelectionColor", selection);
        selectionColor.setPackedValue((uint32_t)selection, transparency);
        SoSFColor col; col.setValue(selectionColor);
        SoFCSelectionColorAction cAct(col);
        cAct.apply(_viewer->getSceneGraph());
    }
    else if (strcmp(Reason,"NavigationStyle") == 0) {
        // check whether the simple or the full mouse model is used
        std::string model = rGrp.GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
        Base::Type type = Base::Type::fromName(model.c_str());
        _viewer->setNavigationType(type);
    }
    else if (strcmp(Reason,"OrbitStyle") == 0) {
        int style = rGrp.GetInt("OrbitStyle",1);
        _viewer->navigationStyle()->setOrbitStyle(NavigationStyle::OrbitStyle(style));
    }
    else if (strcmp(Reason,"Sensitivity") == 0) {
        float val = rGrp.GetFloat("Sensitivity",2.0f);
        _viewer->navigationStyle()->setSensitivity(val);
    }
    else if (strcmp(Reason,"ResetCursorPosition") == 0) {
        bool on = rGrp.GetBool("ResetCursorPosition",false);
        _viewer->navigationStyle()->setResetCursorPosition(on);
    }
    else if (strcmp(Reason,"InvertZoom") == 0) {
        bool on = rGrp.GetBool("InvertZoom", false);
        _viewer->navigationStyle()->setZoomInverted(on);
    }
    else if (strcmp(Reason,"ZoomAtCursor") == 0) {
        bool on = rGrp.GetBool("ZoomAtCursor", true);
        _viewer->navigationStyle()->setZoomAtCursor(on);
    }
    else if (strcmp(Reason,"ZoomStep") == 0) {
        float val = rGrp.GetFloat("ZoomStep", 0.0f);
        _viewer->navigationStyle()->setZoomStep(val);
    }
    else if (strcmp(Reason,"EyeDistance") == 0) {
        _viewer->getSoRenderManager()->setStereoOffset(rGrp.GetFloat("EyeDistance",5.0));
    }
    else if (strcmp(Reason,"CornerCoordSystem") == 0) {
        _viewer->setFeedbackVisibility(rGrp.GetBool("CornerCoordSystem",true));
    }
    else if (strcmp(Reason,"UseAutoRotation") == 0) {
        _viewer->setAnimationEnabled(rGrp.GetBool("UseAutoRotation",true));
    }
    else if (strcmp(Reason,"Gradient") == 0) {
        _viewer->setGradientBackground((rGrp.GetBool("Gradient",true)));
    }
    else if (strcmp(Reason,"ShowFPS") == 0) {
        _viewer->setEnabledFPSCounter(rGrp.GetBool("ShowFPS",false));
    }
    else if (strcmp(Reason,"Orthographic") == 0) {
        // check whether a perspective or orthogrphic camera should be set
        if (rGrp.GetBool("Orthographic", true))
            _viewer->setCameraType(SoOrthographicCamera::getClassTypeId());
        else
            _viewer->setCameraType(SoPerspectiveCamera::getClassTypeId());
    }
    else if (strcmp(Reason, "DimensionsVisible") == 0)
    {
      if (rGrp.GetBool("DimensionsVisible", true))
        _viewer->turnAllDimensionsOn();
      else
        _viewer->turnAllDimensionsOff();
    }
    else if (strcmp(Reason, "Dimensions3dVisible") == 0)
    {
      if (rGrp.GetBool("Dimensions3dVisible", true))
        _viewer->turn3dDimensionsOn();
      else
        _viewer->turn3dDimensionsOff();
    }
    else if (strcmp(Reason, "DimensionsDeltaVisible") == 0)
    {
      if (rGrp.GetBool("DimensionsDeltaVisible", true))
        _viewer->turnDeltaDimensionsOn();
      else
        _viewer->turnDeltaDimensionsOff();
    }
    else
#endif
    if (strcmp(Reason, "BackgroundColor") == 0)
    {
        unsigned long col1 = rGrp.GetUnsigned("BackgroundColor",3940932863UL);
        unsigned long col2 = rGrp.GetUnsigned("BackgroundColor2",859006463UL); // default color (dark blue)
        unsigned long col3 = rGrp.GetUnsigned("BackgroundColor3",2880160255UL); // default color (blue/grey)
        unsigned long col4 = rGrp.GetUnsigned("BackgroundColor4",1869583359UL); // default color (blue/grey)
        float r1,g1,b1,r2,g2,b2,r3,g3,b3,r4,g4,b4;
        r1 = ((col1 >> 24) & 0xff) / 255.0; g1 = ((col1 >> 16) & 0xff) / 255.0; b1 = ((col1 >> 8) & 0xff) / 255.0;
        r2 = ((col2 >> 24) & 0xff) / 255.0; g2 = ((col2 >> 16) & 0xff) / 255.0; b2 = ((col2 >> 8) & 0xff) / 255.0;
        r3 = ((col3 >> 24) & 0xff) / 255.0; g3 = ((col3 >> 16) & 0xff) / 255.0; b3 = ((col3 >> 8) & 0xff) / 255.0;
        r4 = ((col4 >> 24) & 0xff) / 255.0; g4 = ((col4 >> 16) & 0xff) / 255.0; b4 = ((col4 >> 8) & 0xff) / 255.0;
        m_scene->setBackgroundColor(QColor::fromRgbF(r1, g1, b1));
        //if (rGrp.GetBool("UseBackgroundColorMid",false) == false)
        //    _viewer->setGradientBackgroundColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3));
        //else
        //    _viewer->setGradientBackgroundColor(SbColor(r2, g2, b2), SbColor(r3, g3, b3), SbColor(r4, g4, b4));
    }
}

#include "moc_GLGraphicsView.cpp"
