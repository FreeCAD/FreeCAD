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
    QDialog *dialog = new QDialog(0, Qt::CustomizeWindowHint | Qt::WindowTitleHint);

    dialog->setWindowOpacity(0.8);
    dialog->setWindowTitle(windowTitle);
    dialog->setLayout(new QVBoxLayout);

    return dialog;
}

GraphicsScene::GraphicsScene()
    : m_wireframeEnabled(false)
    , m_normalsEnabled(false)
    , m_modelColor(153, 255, 0)
    , m_backgroundColor(0, 170, 255)
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

    m_modelButton = new QPushButton(tr("Load model"));
    //connect(m_modelButton, SIGNAL(clicked()), this, SLOT(loadModel()));
    controls->layout()->addWidget(m_modelButton);

    QCheckBox *wireframe = new QCheckBox(tr("Render as wireframe"));
    //connect(wireframe, SIGNAL(toggled(bool)), this, SLOT(enableWireframe(bool)));
    controls->layout()->addWidget(wireframe);

    QCheckBox *normals = new QCheckBox(tr("Display normals vectors"));
    //connect(normals, SIGNAL(toggled(bool)), this, SLOT(enableNormals(bool)));
    controls->layout()->addWidget(normals);

    QPushButton *colorButton = new QPushButton(tr("Choose model color"));
    //connect(colorButton, SIGNAL(clicked()), this, SLOT(setModelColor()));
    controls->layout()->addWidget(colorButton);

    QPushButton *backgroundButton = new QPushButton(tr("Choose background color"));
    //connect(backgroundButton, SIGNAL(clicked()), this, SLOT(setBackgroundColor()));
    controls->layout()->addWidget(backgroundButton);

    QWidget *statistics = createDialog(tr("Model info"));
    statistics->layout()->setMargin(20);

    for (int i = 0; i < 4; ++i) {
        m_labels[i] = new QLabel;
        statistics->layout()->addWidget(m_labels[i]);
    }

    QWidget *instructions = createDialog(tr("Instructions"));
    instructions->layout()->addWidget(new QLabel(tr("Use mouse wheel to zoom model, and click and drag to rotate model")));
    instructions->layout()->addWidget(new QLabel(tr("Move the sun around to change the light position")));

    addWidget(instructions);
    addWidget(controls);
    addWidget(statistics);

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

    loadModel(QLatin1String("qt.obj"));
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
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL) {
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
    glColor4f(m_modelColor.redF(), m_modelColor.greenF(), m_modelColor.blueF(), 1.0f);

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
    painter->drawText(50,50, QString::fromAscii("Done with QPainter"));
    painter->restore();

    QTimer::singleShot(20, this, SLOT(update()));
}

void GraphicsScene::loadModel()
{
    loadModel(QFileDialog::getOpenFileName(0, tr("Choose model"), QString(), QLatin1String("*.obj")));
}

void GraphicsScene::loadModel(const QString &filePath)
{
    if (filePath.isEmpty())
        return;

    m_modelButton->setEnabled(false);
    QApplication::setOverrideCursor(Qt::BusyCursor);
    modelLoaded();
}

void GraphicsScene::modelLoaded()
{
    m_modelButton->setEnabled(true);
    QApplication::restoreOverrideCursor();
}

void GraphicsScene::enableWireframe(bool enabled)
{
    m_wireframeEnabled = enabled;
    update();
}

void GraphicsScene::enableNormals(bool enabled)
{
    m_normalsEnabled = enabled;
    update();
}

void GraphicsScene::setModelColor()
{
    const QColor color = QColorDialog::getColor(m_modelColor);
    if (color.isValid()) {
        m_modelColor = color;
        update();
    }
}

void GraphicsScene::setBackgroundColor()
{
    const QColor color = QColorDialog::getColor(m_backgroundColor);
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
  : Gui::MDIView(doc, parent), m_Scene(new GraphicsScene()), m_view(new GraphicsView)
{
    m_view->setViewport(new QGLWidget(QGLFormat(QGL::SampleBuffers)));
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setScene(m_Scene);

    std::vector<ViewProvider*> v = doc->getViewProvidersOfType(ViewProvider::getClassTypeId());
    for (std::vector<ViewProvider*>::iterator it = v.begin(); it != v.end(); ++it)
        m_Scene->getSceneGraph()->addChild((*it)->getRoot());
    setCentralWidget(m_view);
    m_Scene->viewAll();
}

GraphicsView3D::~GraphicsView3D()
{
}

#include "moc_GLGraphicsView.cpp"
