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

#include <Inventor/SoEventManager.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/SbViewportRegion.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/events/SoLocation2Event.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QFileDialog>
#include <QLabel>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QGraphicsView>
#include <QPaintEngine>
#include <QGraphicsItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsProxyWidget>
#include <QUrl>

#include "GLGraphicsView.h"
#include <App/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>

#include <Quarter/devices/Mouse.h>
#include <Quarter/devices/Keyboard.h>
#include <Quarter/devices/SpaceNavigatorDevice.h>

using namespace Gui;

#ifndef GL_MULTISAMPLE
#define GL_MULTISAMPLE  0x809D
#endif

// http://doc.qt.digia.com/qq/qq26-openglcanvas.html

GraphicsView::GraphicsView()
{
}

GraphicsView::~GraphicsView()
{
}

bool GraphicsView::viewportEvent(QEvent* event)
{
    if (event->type() == QEvent::Paint || event->type() == QEvent::Resize) {
        return QGraphicsView::viewportEvent(event);
    }
    else if (event->type() == QEvent::MouseMove ||
             event->type() == QEvent::Wheel ||
             event->type() == QEvent::MouseButtonDblClick ||
             event->type() == QEvent::MouseButtonRelease ||
             event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        QGraphicsItem *item = itemAt(mouse->pos());
        if (!item) {
            QGraphicsView::viewportEvent(event);
            return false;
        }

        return QGraphicsView::viewportEvent(event);
    }

    return QGraphicsView::viewportEvent(event);
}

void GraphicsView::resizeEvent(QResizeEvent *event)
{
    if (scene())
        scene()->setSceneRect(
            QRect(QPoint(0, 0), event->size()));
    QGraphicsView::resizeEvent(event);
}


// ----------------------------------------------------------------------------

class SceneEventFilter::Private
{
public:
    QList<SIM::Coin3D::Quarter::InputDevice *> devices;
    GraphicsScene * scene;
    QPoint globalmousepos;
    SbVec2s windowsize;

    void trackWindowSize(QResizeEvent * event)
    {
        this->windowsize = SbVec2s(event->size().width(),
                                   event->size().height());

        Q_FOREACH(SIM::Coin3D::Quarter::InputDevice * device, this->devices) {
            device->setWindowSize(this->windowsize);
        }
    }

    void trackPointerPosition(QMouseEvent * event)
    {
        assert(this->windowsize[1] != -1);
        this->globalmousepos = event->globalPos();

        SbVec2s mousepos(event->pos().x(), this->windowsize[1] - event->pos().y() - 1);
        Q_FOREACH(SIM::Coin3D::Quarter::InputDevice * device, this->devices) {
            device->setMousePosition(mousepos);
        }
    }
};

#define PRIVATE(obj) obj->pimpl

SceneEventFilter::SceneEventFilter(QObject * parent)
  : QObject(parent)
{
    PRIVATE(this) = new Private;

    PRIVATE(this)->scene = dynamic_cast<GraphicsScene *>(parent);
    assert(PRIVATE(this)->scene);

    PRIVATE(this)->windowsize = SbVec2s(PRIVATE(this)->scene->width(),
                                        PRIVATE(this)->scene->height());

    PRIVATE(this)->devices += new SIM::Coin3D::Quarter::Mouse;
    PRIVATE(this)->devices += new SIM::Coin3D::Quarter::Keyboard;

#ifdef HAVE_SPACENAV_LIB
    PRIVATE(this)->devices += new SpaceNavigatorDevice;
#endif // HAVE_SPACENAV_LIB

}

SceneEventFilter::~SceneEventFilter()
{
    qDeleteAll(PRIVATE(this)->devices);
    delete PRIVATE(this);
}

/*!
  Adds a device for event translation
 */
void
SceneEventFilter::registerInputDevice(SIM::Coin3D::Quarter::InputDevice * device)
{
    PRIVATE(this)->devices += device;
}

/*!
  Removes a device from event translation
 */
void
SceneEventFilter::unregisterInputDevice(SIM::Coin3D::Quarter::InputDevice * device)
{
    int i = PRIVATE(this)->devices.indexOf(device);
    if (i != -1) {
        PRIVATE(this)->devices.removeAt(i);
    }
}

/*! Translates Qt Events into Coin events and passes them on to the
  event QuarterWidget for processing. If the event can not be
  translated or processed, it is forwarded to Qt and the method
  returns false.
 */
bool
SceneEventFilter::eventFilter(QObject *, QEvent * qevent)
{
    // Convert the scene event back to a standard event
    std::unique_ptr<QEvent> sceneev;
    switch (qevent->type()) {

    case QEvent::GraphicsSceneMouseMove:
        {
            QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(qevent);
            sceneev.reset(new QMouseEvent(QEvent::MouseMove, ev->pos().toPoint(),
                ev->button(), ev->buttons(), ev->modifiers()));
            qevent = sceneev.get();
            break;
        }
    case QEvent::GraphicsSceneMousePress:
        {
            QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(qevent);
            sceneev.reset(new QMouseEvent(QEvent::MouseButtonPress, ev->pos().toPoint(),
                ev->button(), ev->buttons(), ev->modifiers()));
            qevent = sceneev.get();
            break;
        }
    case QEvent::GraphicsSceneMouseRelease:
        {
            QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(qevent);
            sceneev.reset(new QMouseEvent(QEvent::MouseButtonRelease, ev->pos().toPoint(),
                ev->button(), ev->buttons(), ev->modifiers()));
            qevent = sceneev.get();
            break;
        }
    case QEvent::GraphicsSceneMouseDoubleClick:
        {
            QGraphicsSceneMouseEvent* ev = static_cast<QGraphicsSceneMouseEvent*>(qevent);
            sceneev.reset(new QMouseEvent(QEvent::MouseButtonDblClick, ev->pos().toPoint(),
                ev->button(), ev->buttons(), ev->modifiers()));
            qevent = sceneev.get();
            break;
        }
    case QEvent::GraphicsSceneWheel:
        {
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
            QGraphicsSceneWheelEvent* ev = static_cast<QGraphicsSceneWheelEvent*>(qevent);
            sceneev.reset(new QWheelEvent(ev->pos().toPoint(), ev->delta(), ev->buttons(),
                ev->modifiers(), ev->orientation()));
            qevent = sceneev.get();
#endif
            break;
        }
    case QEvent::GraphicsSceneResize:
        {
            QGraphicsSceneResizeEvent* ev = static_cast<QGraphicsSceneResizeEvent*>(qevent);
            sceneev.reset(new QResizeEvent(ev->newSize().toSize(), ev->oldSize().toSize()));
            qevent = sceneev.get();
            break;
        }
    default:
        break;
    }

    // make sure every device has updated screen size and mouse position
    // before translating events
    switch (qevent->type()) {
    case QEvent::MouseMove:
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
        PRIVATE(this)->trackPointerPosition(dynamic_cast<QMouseEvent *>(qevent));
        break;
    case QEvent::Resize:
        PRIVATE(this)->trackWindowSize(dynamic_cast<QResizeEvent *>(qevent));
        break;
    default:
        break;
    }

    // translate QEvent into SoEvent and see if it is handled by scene
    // graph
    Q_FOREACH(SIM::Coin3D::Quarter::InputDevice * device, PRIVATE(this)->devices) {
        const SoEvent * soevent = device->translateEvent(qevent);
        if (soevent && PRIVATE(this)->scene->processSoEvent(soevent)) {
            //return true;
        }
    }
    return false;
}

/*!
  Returns mouse position in global coordinates
 */
const QPoint &
SceneEventFilter::globalMousePosition(void) const
{
    return PRIVATE(this)->globalmousepos;
}

#undef PRIVATE

// ----------------------------------------------------------------------------

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
    headlight = new SoDirectionalLight();
    headlight->ref();
    sceneNode = new SoSeparator();
    sceneNode->ref();

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

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_time.start();
#endif
    sorendermanager = new SoRenderManager;

    sorendermanager->setAutoClipping(SoRenderManager::VARIABLE_NEAR_PLANE);

    sorendermanager->setBackgroundColor(SbColor4f(0.0f, 0.0f, 0.0f, 0.0f));
    sorendermanager->activate();

    soeventmanager = new SoEventManager;
    soeventmanager->setNavigationState(SoEventManager::MIXED_NAVIGATION);

    eventfilter = new SceneEventFilter(this);

    connect(this, SIGNAL(sceneRectChanged(const QRectF&)),
            this, SLOT(onSceneRectChanged(const QRectF&)));
}

GraphicsScene::~GraphicsScene()
{
    headlight->unref();
    sceneNode->unref();
    delete sorendermanager;
    delete soeventmanager;
    delete eventfilter;
}

void GraphicsScene::viewAll()
{
    SoCamera* cam = sorendermanager->getCamera();
    if (cam)
        cam->viewAll(sceneNode, sorendermanager->getViewportRegion());
}

SceneEventFilter *
GraphicsScene::getEventFilter(void) const
{
    return eventfilter;
}

bool
GraphicsScene::processSoEvent(const SoEvent * event)
{
    if (event) {
        return soeventmanager && soeventmanager->processEvent(event);
    }

    return false;
}

void
GraphicsScene::addStateMachine(SoScXMLStateMachine * statemachine)
{
    SoEventManager * em = this->getSoEventManager();
    em->addSoScXMLStateMachine(statemachine);
    statemachine->setSceneGraphRoot(this->getSoRenderManager()->getSceneGraph());
    statemachine->setActiveCamera(this->getSoRenderManager()->getCamera());
}

void
GraphicsScene::removeStateMachine(SoScXMLStateMachine * statemachine)
{
    SoEventManager * em = this->getSoEventManager();
    statemachine->setSceneGraphRoot(NULL);
    statemachine->setActiveCamera(NULL);
    em->removeSoScXMLStateMachine(statemachine);
}

void
GraphicsScene::setNavigationModeFile(const QUrl & url)
{
    QString filename;

    if (url.scheme()== QLatin1String("coin")) {
        filename = url.path();
        //FIXME: This conditional needs to be implemented when the
        //CoinResources systems if working
        //Workaround for differences between url scheme, and Coin internal
        //scheme in Coin 3.0.
        if (filename[0] == QLatin1Char('/')) {
            filename.remove(0,1);
        }
        filename = url.scheme() + QLatin1Char(':') + filename;
    }
    else if (url.scheme() == QLatin1String("file"))
        filename = url.toLocalFile();
    else if (url.isEmpty()) {

        return;
    }
    else {
        //qDebug() << url.scheme() << "is not recognized";
        return;
    }

    QByteArray filenametmp = filename.toLocal8Bit();
    ScXMLStateMachine * stateMachine = NULL;

    if (filenametmp.startsWith("coin:")){
        stateMachine = ScXML::readFile(filenametmp.data());
    }
    else {
        //Use Qt to read the file in case it is a Qt resource
        QFile file(QString::fromLatin1(filenametmp));
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray fileContents = file.readAll();
#if COIN_MAJOR_VERSION >= 4
            stateMachine = ScXML::readBuffer(SbByteBuffer(fileContents.size(), fileContents.constData()));
#else
            stateMachine = ScXML::readBuffer(fileContents.constData());
#endif
            file.close();
        }
    }

    if (stateMachine && stateMachine->isOfType(SoScXMLStateMachine::getClassTypeId())) {
        SoScXMLStateMachine * newsm = static_cast<SoScXMLStateMachine *>(stateMachine);

        this->addStateMachine(newsm);
        newsm->initialize();
    }
    else {
        if (stateMachine)
            delete stateMachine;

        return;
    }
}

SoNode* GraphicsScene::getSceneGraph() const
{
    return sceneNode;
}

SoCamera *
GraphicsScene::searchForCamera(SoNode * root)
{
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setType(SoCamera::getClassTypeId());
    sa.apply(root);

    if (sa.getPath()) {
        SoNode * node = sa.getPath()->getTail();
        if (node && node->isOfType(SoCamera::getClassTypeId())) {
            return (SoCamera *) node;
        }
    }
    return NULL;
}

void GraphicsScene::setSceneGraph(SoNode * node)
{
    if (node == sceneNode) {
        return;
    }

    if (sceneNode) {
        sceneNode->unref();
        sceneNode = NULL;
    }

    SoCamera * camera = NULL;
    SoSeparator * superscene = NULL;
    bool viewall = false;

    if (node) {
        sceneNode = node;
        sceneNode->ref();

        superscene = new SoSeparator;
        superscene->addChild(headlight);

        // if the scene does not contain a camera, add one
        if (!(camera = searchForCamera(node))) {
            camera = new SoOrthographicCamera;
            superscene->addChild(camera);
            viewall = true;
        }

        superscene->addChild(node);
    }

    soeventmanager->setCamera(camera);
    sorendermanager->setCamera(camera);
    soeventmanager->setSceneGraph(superscene);
    sorendermanager->setSceneGraph(superscene);

    if (viewall) {
        this->viewAll();
    }
    if (superscene) {
        superscene->touch();
    }
}

SoRenderManager *
GraphicsScene::getSoRenderManager(void) const
{
    return sorendermanager;
}

SoEventManager *
GraphicsScene::getSoEventManager(void) const
{
    return soeventmanager;
}

void GraphicsScene::onSceneRectChanged(const QRectF & rect)
{
    SbViewportRegion vp(rect.width(), rect.height());
    sorendermanager->setViewportRegion(vp);
    soeventmanager->setViewportRegion(vp);
}

void GraphicsScene::drawBackground(QPainter *painter, const QRectF &)
{
    if (painter->paintEngine()->type() != QPaintEngine::OpenGL && painter->paintEngine()->type() != QPaintEngine::OpenGL2) {
        qWarning("GraphicsScene: drawBackground needs a QGLWidget to be set as viewport on the graphics view");
        return;
    }

#if 0
    glViewport(0, 0, width(), height());
/**/
    glClearColor(m_backgroundColor.redF(), m_backgroundColor.greenF(), m_backgroundColor.blueF(), 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#endif


#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    const int delta = m_time.elapsed() - m_lastTime;
    m_lastTime += delta;
#endif


    sorendermanager->render(true/*PRIVATE(this)->clearwindow*/,
                            false/*PRIVATE(this)->clearzbuffer*/);

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

        SbColor4f bgcolor(SbClamp(color.red()   / 255.0, 0.0, 1.0),
                          SbClamp(color.green() / 255.0, 0.0, 1.0),
                          SbClamp(color.blue()  / 255.0, 0.0, 1.0),
                          SbClamp(color.alpha() / 255.0, 0.0, 1.0));
        sorendermanager->setBackgroundColor(bgcolor);
        sorendermanager->scheduleRedraw();
    }
}

void GraphicsScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseMoveEvent(event);
    if (event->isAccepted())
        return;
    if (event->buttons() & Qt::LeftButton) {
        event->accept();
        update();
    }
}

void GraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mousePressEvent(event);
    if (event->isAccepted())
        return;

#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
    m_mouseEventTime = m_time.elapsed();
#endif
    event->accept();
}

void GraphicsScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsScene::mouseReleaseEvent(event);
    if (event->isAccepted())
        return;

    event->accept();
    update();
}

void GraphicsScene::wheelEvent(QGraphicsSceneWheelEvent *event)
{
    QGraphicsScene::wheelEvent(event);
    if (event->isAccepted())
        return;
    event->accept();
    update();
}

// ----------------------------------------------------------------------------

GraphicsView3D::GraphicsView3D(Gui::Document* doc, QWidget* parent)
  : Gui::MDIView(doc, parent), m_scene(new GraphicsScene()), m_view(new GraphicsView)
{
    m_view->installEventFilter(m_scene->getEventFilter());
#if 0
    QtGLFormat f;
    f.setSampleBuffers(true);
    f.setSamples(8);
    m_view->setViewport(new QGLWidget(f));
#endif
    m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    m_view->setScene(m_scene);
    m_scene->setNavigationModeFile(QUrl(QString::fromLatin1("coin:///scxml/navigation/examiner.xml")));

    std::vector<ViewProvider*> v = doc->getViewProvidersOfType(ViewProvider::getClassTypeId());
    SoSeparator* root = new SoSeparator();
    for (std::vector<ViewProvider*>::iterator it = v.begin(); it != v.end(); ++it)
        root->addChild((*it)->getRoot());
    m_scene->setSceneGraph(root);
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
    delete m_view;
    delete m_scene;
}

void GraphicsView3D::OnChange(ParameterGrp::SubjectType &rCaller,ParameterGrp::MessageType Reason)
{
    const ParameterGrp& rGrp = static_cast<ParameterGrp&>(rCaller);

    if (strcmp(Reason, "BackgroundColor") == 0)
    {
        unsigned long col1 = rGrp.GetUnsigned("BackgroundColor",3940932863UL);

        float r1,g1,b1;

        r1 = ((col1 >> 24) & 0xff) / 255.0; g1 = ((col1 >> 16) & 0xff) / 255.0; b1 = ((col1 >> 8) & 0xff) / 255.0;
        m_scene->setBackgroundColor(QColor::fromRgbF(r1, g1, b1));
    }
}
#include "moc_GLGraphicsView.cpp"
