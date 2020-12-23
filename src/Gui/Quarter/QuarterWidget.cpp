/**************************************************************************\
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

/*!
  \class SIM::Coin3D::Quarter::QuarterWidget QuarterWidget.h Quarter/QuarterWidget.h

  \brief The QuarterWidget class is the main class in Quarter. It
  provides a widget for Coin rendering. It provides scenegraph
  management and event handling.

  If you want to modify the GL format for an existing QuarterWidget, you can
  set up a new GL context for the widget, e.g.:

  \code
  QGLContext * context = new QGLContext(QGLFormat(QGL::SampleBuffers), viewer);
  if (context->create()) {
    viewer->setContext(context);
  }
  \endcode
*/

#ifdef _MSC_VER
#pragma warning(disable : 4267)
#endif

#include <assert.h>

#include <Quarter/QuarterWidget.h>
#include <Quarter/eventhandlers/EventFilter.h>
#include <Quarter/eventhandlers/DragDropHandler.h>

#include <QtCore/QEvent>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QAction>
#include <QApplication>
#include <QPaintEvent>
#include <QResizeEvent>

#if defined(HAVE_QT5_OPENGL)
#include <QOpenGLDebugMessage>
#include <QOpenGLDebugLogger>
#endif

#include <Inventor/SbViewportRegion.h>
#include <Inventor/system/gl.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoDirectionalLight.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/SbColor.h>
#include <Inventor/sensors/SoSensorManager.h>
#include <Inventor/SoDB.h>

#include <Inventor/SbBasic.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/SoEventManager.h>
#include <Inventor/scxml/ScXML.h>
#include <Inventor/scxml/SoScXMLStateMachine.h>

#include <ctime>

#include "InteractionMode.h"
#include "QuarterWidgetP.h"
#include "QuarterP.h"

#if QT_VERSION >= 0x050000
#include <QWindow>
#include <QGuiApplication>
#include <QMetaObject>
#endif


using namespace SIM::Coin3D::Quarter;

/*!
  \enum SIM::Coin3D::Quarter::QuarterWidget::TransparencyType

  Various settings for how to do rendering of transparent objects in
  the scene. Some of the settings will provide faster rendering, while
  others gives you better quality rendering.

  See \ref SoGLRenderAction::TransparencyType for a full description of the modes
*/

/*!
  \enum SIM::Coin3D::Quarter::QuarterWidget::RenderMode

  Sets how rendering of primitives is done.

  See \ref SoRenderManager::RenderMode for a full description of the modes
*/

/*!
  \enum SIM::Coin3D::Quarter::QuarterWidget::StereoMode

  Sets how stereo rendering is performed.

  See \ref SoRenderManager::StereoMode for a full description of the modes
*/

  enum StereoMode {
    MONO = SoRenderManager::MONO,
    ANAGLYPH = SoRenderManager::ANAGLYPH,
    QUAD_BUFFER = SoRenderManager::QUAD_BUFFER,
    INTERLEAVED_ROWS = SoRenderManager::INTERLEAVED_ROWS,
    INTERLEAVED_COLUMNS = SoRenderManager::INTERLEAVED_COLUMNS
  };

#define PRIVATE(obj) obj->pimpl

#ifndef GL_MULTISAMPLE_BIT_EXT
#define GL_MULTISAMPLE_BIT_EXT 0x20000000
#endif

//We need to avoid buffer swapping when initializing a QPainter on this widget
#if defined(HAVE_QT5_OPENGL)
class CustomGLWidget : public QOpenGLWidget {
public:
    QSurfaceFormat myFormat;

    CustomGLWidget(const QSurfaceFormat& format, QWidget* parent = 0, const QOpenGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
     : QOpenGLWidget(parent, f), myFormat(format)
    {
        Q_UNUSED(shareWidget);
        QSurfaceFormat surfaceFormat(format);
        surfaceFormat.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
        // With the settings below we could determine deprecated OpenGL API
        // but can't do this since otherwise it will complain about almost any
        // OpenGL call in Coin3d
        //surfaceFormat.setMajorVersion(3);
        //surfaceFormat.setMinorVersion(2);
        //surfaceFormat.setProfile(QSurfaceFormat::CoreProfile);
#if defined (_DEBUG) && 0
        surfaceFormat.setOption(QSurfaceFormat::DebugContext);
#endif
        setFormat(surfaceFormat);
    }
    ~CustomGLWidget()
    {
    }
    void initializeGL()
    {
        QOpenGLContext *context = QOpenGLContext::currentContext();
#if defined (_DEBUG) && 0
        if (context && context->hasExtension(QByteArrayLiteral("GL_KHR_debug"))) {
            QOpenGLDebugLogger *logger = new QOpenGLDebugLogger(this);
            connect(logger, &QOpenGLDebugLogger::messageLogged, this, &CustomGLWidget::handleLoggedMessage);

            if (logger->initialize())
                logger->startLogging(QOpenGLDebugLogger::SynchronousLogging);
        }
#endif
        if (context) {
            connect(context, &QOpenGLContext::aboutToBeDestroyed,
                this, &CustomGLWidget::aboutToDestroyGLContext, Qt::DirectConnection);
        }
        connect(this, &CustomGLWidget::resized, this, &CustomGLWidget::slotResized);
    }
    void aboutToDestroyGLContext()
    {
#if QT_VERSION >= 0x050900
        // With Qt 5.9 a signal is emitted while the QuarterWidget is being destroyed.
        // At this state its type is a QWidget, not a QuarterWidget any more.
        QuarterWidget* qw = qobject_cast<QuarterWidget*>(parent());
        if (!qw)
            return;
#endif
        QMetaObject::invokeMethod(parent(), "aboutToDestroyGLContext",
            Qt::DirectConnection,
            QGenericReturnArgument());
    }
    bool event(QEvent *e)
    {
        // If a debug logger is activated then Qt's default implementation
        // first releases the context before stopping the logger. However,
        // the logger needs the active context and thus crashes because it's
        // null.
        if (e->type() == QEvent::WindowChangeInternal) {
            if (!qApp->testAttribute(Qt::AA_ShareOpenGLContexts)) {
                QOpenGLDebugLogger* logger = this->findChild<QOpenGLDebugLogger*>();
                if (logger) {
                    logger->stopLogging();
                    delete logger;
                }
            }
        }

        return QOpenGLWidget::event(e);
    }
    void handleLoggedMessage(const QOpenGLDebugMessage &message)
    {
        qDebug() << message;
    }
    void showEvent(QShowEvent*)
    {
        update(); // force update when changing window mode
    }
    void slotResized()
    {
        update(); // fixes flickering on some systems
    }
};
#else
class CustomGLWidget : public QGLWidget {
public:
    CustomGLWidget(const QGLFormat& fo, QWidget* parent = 0, const QGLWidget* shareWidget = 0, Qt::WindowFlags f = 0)
     : QGLWidget(fo, parent, shareWidget, f)
    {
         setAutoBufferSwap(false);
    }
};
#endif

/*! constructor */
QuarterWidget::QuarterWidget(const QtGLFormat & format, QWidget * parent, const QtGLWidget * sharewidget, Qt::WindowFlags f)
  : inherited(parent)
{
  Q_UNUSED(f); 
  this->constructor(format, sharewidget);
}

/*! constructor */
QuarterWidget::QuarterWidget(QWidget * parent, const QtGLWidget * sharewidget, Qt::WindowFlags f)
  : inherited(parent)
{
  Q_UNUSED(f); 
  this->constructor(QtGLFormat(), sharewidget);
}

/*! constructor */
QuarterWidget::QuarterWidget(QtGLContext * context, QWidget * parent, const QtGLWidget * sharewidget, Qt::WindowFlags f)
  : inherited(parent)
{
  Q_UNUSED(f); 
  this->constructor(context->format(), sharewidget);
}

void
QuarterWidget::constructor(const QtGLFormat & format, const QtGLWidget * sharewidget)
{
  QGraphicsScene* scene = new QGraphicsScene;
  setScene(scene);
  setViewport(new CustomGLWidget(format, this, sharewidget)); 
  
  setFrameStyle(QFrame::NoFrame);
  setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
  PRIVATE(this) = new QuarterWidgetP(this, sharewidget);

  PRIVATE(this)->sorendermanager = new SoRenderManager;
  PRIVATE(this)->initialsorendermanager = true;
  PRIVATE(this)->soeventmanager = new SoEventManager;
  PRIVATE(this)->initialsoeventmanager = true;
  PRIVATE(this)->processdelayqueue = true;

  //Mind the order of initialization as the XML state machine uses
  //callbacks which depends on other state being initialized
  PRIVATE(this)->eventfilter = new EventFilter(this);
  PRIVATE(this)->interactionmode = new InteractionMode(this);

  PRIVATE(this)->currentStateMachine = NULL;

  PRIVATE(this)->headlight = new SoDirectionalLight;
  PRIVATE(this)->headlight->ref();

  PRIVATE(this)->sorendermanager->setAutoClipping(SoRenderManager::VARIABLE_NEAR_PLANE);
  PRIVATE(this)->sorendermanager->setRenderCallback(QuarterWidgetP::rendercb, this);
  PRIVATE(this)->sorendermanager->setBackgroundColor(SbColor4f(0.0f, 0.0f, 0.0f, 0.0f));
  PRIVATE(this)->sorendermanager->activate();
  PRIVATE(this)->sorendermanager->addPreRenderCallback(QuarterWidgetP::prerendercb, PRIVATE(this));
  PRIVATE(this)->sorendermanager->addPostRenderCallback(QuarterWidgetP::postrendercb, PRIVATE(this));

  PRIVATE(this)->soeventmanager->setNavigationState(SoEventManager::MIXED_NAVIGATION);

  // set up a cache context for the default SoGLRenderAction
  PRIVATE(this)->sorendermanager->getGLRenderAction()->setCacheContext(this->getCacheContextId());

  this->setMouseTracking(true);

  // Qt::StrongFocus means the widget will accept keyboard focus by
  // both tabbing and clicking
  this->setFocusPolicy(Qt::StrongFocus);

  this->installEventFilter(PRIVATE(this)->eventfilter);
  this->installEventFilter(PRIVATE(this)->interactionmode);

  initialized = false;
}

void
QuarterWidget::replaceViewport()
{
#if defined(HAVE_QT5_OPENGL)
  CustomGLWidget* oldvp = static_cast<CustomGLWidget*>(viewport());
  CustomGLWidget* newvp = new CustomGLWidget(oldvp->myFormat, this);
  PRIVATE(this)->replaceGLWidget(newvp);
  setViewport(newvp);

  setAutoFillBackground(false);
  viewport()->setAutoFillBackground(false);
#endif
}

void
QuarterWidget::aboutToDestroyGLContext()
{
}

/*! destructor */
QuarterWidget::~QuarterWidget()
{
  if (PRIVATE(this)->currentStateMachine) {
    this->removeStateMachine(PRIVATE(this)->currentStateMachine);
    delete PRIVATE(this)->currentStateMachine;
  }
  PRIVATE(this)->headlight->unref();
  PRIVATE(this)->headlight = NULL;
  this->setSceneGraph(NULL);
  this->setSoRenderManager(NULL);
  this->setSoEventManager(NULL);
  delete PRIVATE(this)->eventfilter;
  delete PRIVATE(this);
}

/*!
  You can set the cursor you want to use for a given navigation
  state. See the Coin documentation on navigation for information
  about available states
*/
void
QuarterWidget::setStateCursor(const SbName & state, const QCursor & cursor)
{
  assert(QuarterP::statecursormap);
  // will overwrite the value of an existing item
  QuarterP::statecursormap->insert(state, cursor);
}

/*!
  Maps a state to a cursor

  \param[in] state Named state in the statemachine
  \retval Cursor corresponding to the given state
*/
QCursor
QuarterWidget::stateCursor(const SbName & state)
{
  assert(QuarterP::statecursormap);
  return QuarterP::statecursormap->value(state);
}

/*!
  \property QuarterWidget::headlightEnabled

  \copydetails QuarterWidget::setHeadlightEnabled
*/

/*!
  Enable/disable the headlight. This will toggle the SoDirectionalLight::on
  field (returned from getHeadlight()).
*/
void
QuarterWidget::setHeadlightEnabled(bool onoff)
{
  PRIVATE(this)->headlight->on = onoff;
}

/*!
  Returns true if the headlight is on, false if it is off
*/
bool
QuarterWidget::headlightEnabled(void) const
{
  return PRIVATE(this)->headlight->on.getValue();
}

/*!
  Returns the light used for the headlight.
*/
SoDirectionalLight *
QuarterWidget::getHeadlight(void) const
{
  return PRIVATE(this)->headlight;
}

/*!
  \property QuarterWidget::clearZBuffer

  \copydetails QuarterWidget::setClearZBuffer
*/

/*!
  Specify if you want the z buffer to be cleared before
  redraw. This is on by default.
*/
void
QuarterWidget::setClearZBuffer(bool onoff)
{
  PRIVATE(this)->clearzbuffer = onoff;
}

/*!
  Returns true if the z buffer is cleared before rendering.
*/
bool
QuarterWidget::clearZBuffer(void) const
{
  return PRIVATE(this)->clearzbuffer;
}

/*!
  \property QuarterWidget::clearWindow

  \copydetails QuarterWidget::setClearWindow
*/

/*!
  Specify if you want the rendering buffer to be cleared before
  rendering. This is on by default.
 */
void
QuarterWidget::setClearWindow(bool onoff)
{
  PRIVATE(this)->clearwindow = onoff;
}

/*!
  Returns true if the rendering buffer is cleared before rendering.
*/
bool
QuarterWidget::clearWindow(void) const
{
  return PRIVATE(this)->clearwindow;
}

/*!
  \property QuarterWidget::interactionModeEnabled

  \copydetails QuarterWidget::setInteractionModeEnabled
*/

/*!
  Enable/disable interaction mode.

  Specifies whether you may use the alt-key to enter interaction mode.
*/
void
QuarterWidget::setInteractionModeEnabled(bool onoff)
{
  PRIVATE(this)->interactionmode->setEnabled(onoff);
}

/*!
  Returns true if interaction mode is enabled, false otherwise.
 */
bool
QuarterWidget::interactionModeEnabled(void) const
{
  return PRIVATE(this)->interactionmode->enabled();
}

/*!
  \property QuarterWidget::interactionModeOn

  \copydetails QuarterWidget::setInteractionModeOn
*/

/*!
  Turn interaction mode on or off.
*/
void
QuarterWidget::setInteractionModeOn(bool onoff)
{
  PRIVATE(this)->interactionmode->setOn(onoff);
}

/*!
  Returns true if interaction mode is on.
 */
bool
QuarterWidget::interactionModeOn(void) const
{
  return PRIVATE(this)->interactionmode->on();
}

/*!
  Returns the Coin cache context id for this widget.
*/
uint32_t
QuarterWidget::getCacheContextId(void) const
{
  return PRIVATE(this)->getCacheContextId();
}

/*!
  \property QuarterWidget::transparencyType

  \copydetails QuarterWidget::setTransparencyType
*/

/*!
  Sets the transparency type to be used for the scene.
*/
void
QuarterWidget::setTransparencyType(TransparencyType type)
{
  assert(PRIVATE(this)->sorendermanager);
  PRIVATE(this)->sorendermanager->getGLRenderAction()->setTransparencyType((SoGLRenderAction::TransparencyType)type);
  PRIVATE(this)->sorendermanager->scheduleRedraw();
}

/*!
  \retval The current \ref TransparencyType
*/
QuarterWidget::TransparencyType
QuarterWidget::transparencyType(void) const
{
  assert(PRIVATE(this)->sorendermanager);
  SoGLRenderAction * action = PRIVATE(this)->sorendermanager->getGLRenderAction();
  return static_cast<QuarterWidget::TransparencyType>(action->getTransparencyType());
}

/*!
  \property QuarterWidget::renderMode

  \copydetails QuarterWidget::setRenderMode
*/

/*!
  \copydoc RenderMode
*/
void
QuarterWidget::setRenderMode(RenderMode mode)
{
  assert(PRIVATE(this)->sorendermanager);
  PRIVATE(this)->sorendermanager->setRenderMode(static_cast<SoRenderManager::RenderMode>(mode));
  PRIVATE(this)->sorendermanager->scheduleRedraw();
}

/*!
  \retval The current \ref RenderMode
*/
QuarterWidget::RenderMode
QuarterWidget::renderMode(void) const
{
  assert(PRIVATE(this)->sorendermanager);
  return static_cast<RenderMode>(PRIVATE(this)->sorendermanager->getRenderMode());
}

/*!
  \property QuarterWidget::stereoMode

  \copydetails QuarterWidget::setStereoMode
*/

/*!
  \copydoc StereoMode
*/
void
QuarterWidget::setStereoMode(StereoMode mode)
{
  assert(PRIVATE(this)->sorendermanager);
  PRIVATE(this)->sorendermanager->setStereoMode(static_cast<SoRenderManager::StereoMode>(mode));
  PRIVATE(this)->sorendermanager->scheduleRedraw();
}


/*!
  \retval The current \ref StereoMode
*/
QuarterWidget::StereoMode
QuarterWidget::stereoMode(void) const
{
  assert(PRIVATE(this)->sorendermanager);
  return static_cast<StereoMode>(PRIVATE(this)->sorendermanager->getStereoMode());
}

/*!
  \property QuarterWidget::devicePixelRatio

  \copydetails QuarterWidget::devicePixelRatio
*/

/*!
  The ratio between logical and physical pixel sizes -- obtained from the window that
the widget is located within, and updated whenever any change occurs, emitting a devicePixelRatioChanged signal.  Only available for version Qt 5.6 and above (will be 1.0 for all previous versions)
 */

qreal
QuarterWidget::devicePixelRatio(void) const
{
  return PRIVATE(this)->device_pixel_ratio;
}

/*!
  Sets the Inventor scenegraph to be rendered
 */
void
QuarterWidget::setSceneGraph(SoNode * node)
{
  if (node == PRIVATE(this)->scene) {
    return;
  }

  if (PRIVATE(this)->scene) {
    PRIVATE(this)->scene->unref();
    PRIVATE(this)->scene = NULL;
  }

  SoCamera * camera = NULL;
  SoSeparator * superscene = NULL;
  bool viewall = false;

  if (node) {
    PRIVATE(this)->scene = node;
    PRIVATE(this)->scene->ref();

    superscene = new SoSeparator;
    superscene->addChild(PRIVATE(this)->headlight);

    // if the scene does not contain a camera, add one
    if (!(camera = PRIVATE(this)->searchForCamera(node))) {
      camera = new SoPerspectiveCamera;
      superscene->addChild(camera);
      viewall = true;
    }

    superscene->addChild(node);
  }

  PRIVATE(this)->soeventmanager->setCamera(camera);
  PRIVATE(this)->sorendermanager->setCamera(camera);
  PRIVATE(this)->soeventmanager->setSceneGraph(superscene);
  PRIVATE(this)->sorendermanager->setSceneGraph(superscene);

  if (viewall) { this->viewAll(); }
  if (superscene) { superscene->touch(); }
}

/*!
  Returns pointer to root of scene graph
*/
SoNode *
QuarterWidget::getSceneGraph(void) const
{
  return PRIVATE(this)->scene;
}

/*!
  Set the render manager for the widget.
*/
void
QuarterWidget::setSoRenderManager(SoRenderManager * manager)
{
  bool carrydata = false;
  SoNode * scene = NULL;
  SoCamera * camera = NULL;
  SbViewportRegion vp;
  if (PRIVATE(this)->sorendermanager && (manager != NULL)) {
    scene = PRIVATE(this)->sorendermanager->getSceneGraph();
    camera = PRIVATE(this)->sorendermanager->getCamera();
    vp = PRIVATE(this)->sorendermanager->getViewportRegion();
    carrydata = true;
  }

  // ref before deleting the old scene manager to avoid that the nodes are deleted
  if (scene) scene->ref();
  if (camera) camera->ref();
  
  if (PRIVATE(this)->initialsorendermanager) {
    delete PRIVATE(this)->sorendermanager;
    PRIVATE(this)->initialsorendermanager = false;
  }
  PRIVATE(this)->sorendermanager = manager;
  if (carrydata) {
    PRIVATE(this)->sorendermanager->setSceneGraph(scene);
    PRIVATE(this)->sorendermanager->setCamera(camera);
    PRIVATE(this)->sorendermanager->setViewportRegion(vp);
  }

  if (scene) scene->unref();
  if (camera) camera->unref();
}

/*!
  Returns a pointer to the render manager.
*/
SoRenderManager *
QuarterWidget::getSoRenderManager(void) const
{
  return PRIVATE(this)->sorendermanager;
}

/*!
  Set the Coin event manager for the widget.
*/
void
QuarterWidget::setSoEventManager(SoEventManager * manager)
{
  bool carrydata = false;
  SoNode * scene = NULL;
  SoCamera * camera = NULL;
  SbViewportRegion vp;
  if (PRIVATE(this)->soeventmanager && (manager != NULL)) {
    scene = PRIVATE(this)->soeventmanager->getSceneGraph();
    camera = PRIVATE(this)->soeventmanager->getCamera();
    vp = PRIVATE(this)->soeventmanager->getViewportRegion();
    carrydata = true;
  }

  // ref before deleting the old scene manager to avoid that the nodes are deleted
  if (scene) scene->ref();
  if (camera) camera->ref();

  if (PRIVATE(this)->initialsoeventmanager) {
    delete PRIVATE(this)->soeventmanager;
    PRIVATE(this)->initialsoeventmanager = false;
  }
  PRIVATE(this)->soeventmanager = manager;
  if (carrydata) {
    PRIVATE(this)->soeventmanager->setSceneGraph(scene);
    PRIVATE(this)->soeventmanager->setCamera(camera);
    PRIVATE(this)->soeventmanager->setViewportRegion(vp);
  }

  if (scene) scene->unref();
  if (camera) camera->unref();
}

/*!
  Returns a pointer to the event manager
*/
SoEventManager *
QuarterWidget::getSoEventManager(void) const
{
  return PRIVATE(this)->soeventmanager;
}

/*!
  Returns a pointer to the event filter
 */
EventFilter *
QuarterWidget::getEventFilter(void) const
{
  return PRIVATE(this)->eventfilter;
}

/*!
  Reposition the current camera to display the entire scene
 */
void
QuarterWidget::viewAll(void)
{
  const SbName viewallevent("sim.coin3d.coin.navigation.ViewAll");
  for (int c = 0; c < PRIVATE(this)->soeventmanager->getNumSoScXMLStateMachines(); ++c) {
    SoScXMLStateMachine * sostatemachine =
      PRIVATE(this)->soeventmanager->getSoScXMLStateMachine(c);
    if (sostatemachine->isActive()) {
      sostatemachine->queueEvent(viewallevent);
      sostatemachine->processEventQueue();
    }
  }
}

/*!
  Sets the current camera in seekmode, if supported by the underlying navigation system.
  Camera typically seeks towards what the mouse is pointing at.
*/
void
QuarterWidget::seek(void)
{
  const SbName seekevent("sim.coin3d.coin.navigation.Seek");
  for (int c = 0; c < PRIVATE(this)->soeventmanager->getNumSoScXMLStateMachines(); ++c) {
    SoScXMLStateMachine * sostatemachine =
      PRIVATE(this)->soeventmanager->getSoScXMLStateMachine(c);
    if (sostatemachine->isActive()) {
      sostatemachine->queueEvent(seekevent);
      sostatemachine->processEventQueue();
    }
  }
}

bool
QuarterWidget::updateDevicePixelRatio(void) {
#if QT_VERSION >= 0x050000
    qreal dev_pix_ratio = 1.0;
    QWidget* winwidg = window();
    QWindow* win = NULL;
    if(winwidg) {
        win = winwidg->windowHandle();
    }
    if(win) {
        dev_pix_ratio = win->devicePixelRatio();
    }
    else {
        dev_pix_ratio = ((QGuiApplication*)QGuiApplication::instance())->devicePixelRatio();
    }
    if(PRIVATE(this)->device_pixel_ratio != dev_pix_ratio) {
        PRIVATE(this)->device_pixel_ratio = dev_pix_ratio;
        emit devicePixelRatioChanged(dev_pix_ratio);
        return true;
    }
#endif
    return false;
}

/*!
  Overridden from QGLWidget to resize the Coin scenegraph
 */
void QuarterWidget::resizeEvent(QResizeEvent* event)
{
    updateDevicePixelRatio();
    qreal dev_pix_ratio = devicePixelRatio();
    int width = static_cast<int>(dev_pix_ratio * event->size().width());
    int height = static_cast<int>(dev_pix_ratio * event->size().height());

    SbViewportRegion vp(width, height);
    PRIVATE(this)->sorendermanager->setViewportRegion(vp);
    PRIVATE(this)->soeventmanager->setViewportRegion(vp);
    if (scene())
        scene()->setSceneRect(QRect(QPoint(0, 0), event->size()));
    QGraphicsView::resizeEvent(event);
}

/*!
  Overridden from QGLWidget to render the scenegraph
*/
void QuarterWidget::paintEvent(QPaintEvent* event)
{
    if (updateDevicePixelRatio()) {
        qreal dev_pix_ratio = devicePixelRatio();
        int width = static_cast<int>(dev_pix_ratio * this->width());
        int height = static_cast<int>(dev_pix_ratio * this->height());
        SbViewportRegion vp(width, height);
        PRIVATE(this)->sorendermanager->setViewportRegion(vp);
        PRIVATE(this)->soeventmanager->setViewportRegion(vp);
    }

    if(!initialized) {
#if !defined(HAVE_QT5_OPENGL)
        glEnable(GL_DEPTH_TEST);
#endif
        this->getSoRenderManager()->reinitialize();
        initialized = true;
    }

    getSoRenderManager()->activate();

#if !defined(HAVE_QT5_OPENGL)
    glEnable(GL_DEPTH_TEST);
#endif
    glMatrixMode(GL_PROJECTION);

    QtGLWidget* w = static_cast<QtGLWidget*>(this->viewport());
    assert(w->isValid() && "No valid GL context found!");
    // We might have to process the delay queue here since we don't know
    // if paintGL() is called from Qt, and we might have some sensors
    // waiting to trigger (the redraw sensor has a lower priority than a
    // normal field sensor to guarantee that your sensor is processed
    // before the next redraw). Disable autorendering while we do this
    // to avoid recursive redraws.

    // We set the PRIVATE(this)->processdelayqueue = false in redraw()
    // to avoid processing the delay queue when paintGL() is triggered
    // by us, and we don't want to process the delay queue in those
    // cases

    PRIVATE(this)->autoredrawenabled = false;

    if(PRIVATE(this)->processdelayqueue && SoDB::getSensorManager()->isDelaySensorPending()) {
        // processing the sensors might trigger a redraw in another
        // context. Release this context temporarily
        w->doneCurrent();
        SoDB::getSensorManager()->processDelayQueue(false);
        w->makeCurrent();
    }

    assert(w->isValid() && "No valid GL context found!");

#if defined(HAVE_QT5_OPENGL)
    // Causes an OpenGL error on resize
    //glDrawBuffer(w->format().swapBehavior() == QSurfaceFormat::DoubleBuffer ? GL_BACK : GL_FRONT);
#else
    glDrawBuffer(w->doubleBuffer() ? GL_BACK : GL_FRONT);
#endif

    w->makeCurrent();
    this->actualRedraw();

    //start the standard graphics view processing for all widgets and graphic items. As 
    //QGraphicsView initaliizes a QPainter which changes the Opengl context in an unpredictable 
    //manner we need to store the context and recreate it after Qt is done.
    glPushAttrib(GL_MULTISAMPLE_BIT_EXT);
    inherited::paintEvent(event);
    glPopAttrib();

#if defined(HAVE_QT5_OPENGL)
    // Causes an OpenGL error on resize
    //if (w->format().swapBehavior() == QSurfaceFormat::DoubleBuffer)
    //    w->context()->swapBuffers(w->context()->surface());
#else
    if (w->doubleBuffer()) { w->swapBuffers(); }
#endif

    PRIVATE(this)->autoredrawenabled = true;

    // process the delay queue the next time we enter this function,
    // unless we get here after a call to redraw().
    PRIVATE(this)->processdelayqueue = true;
}

bool QuarterWidget::viewportEvent(QEvent* event)
{
    // Disable the old implementation of this method as it show
    // problems with panning and rotations when a widget item is
    // added to the scene.
#if 0
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
            return false;
        }

        return QGraphicsView::viewportEvent(event);
      }

     //if we return false the events get processed normally, this means they get passed to the quarter
     //event filters for processing in the scene graph. If we return true event processing stops here.
     return false;
#else
    // If no item is selected still let the graphics scene handle it but
    // additionally handle it by this viewer. This is e.g. needed when
    // resizing a widget item because the cursor may already be outside
    // this widget.
    if (event->type() == QEvent::Wheel ||
        event->type() == QEvent::MouseButtonDblClick ||
        event->type() == QEvent::MouseButtonPress) {
        QMouseEvent* mouse = static_cast<QMouseEvent*>(event);
        QGraphicsItem *item = itemAt(mouse->pos());
        if (!item) {
            QGraphicsView::viewportEvent(event);
            return false;
        }
    }
    else if (event->type() == QEvent::MouseMove ||
             event->type() == QEvent::MouseButtonRelease) {
        QGraphicsScene* glScene = this->scene();
        if (!(glScene && glScene->mouseGrabberItem())) {
            QGraphicsView::viewportEvent(event);
            return false;
        }
    }

    return QGraphicsView::viewportEvent(event);
#endif
}

/*!
  Used for rendering the scene. Usually Coin/Quarter will automatically redraw
  the scene graph at regular intervals, after the scene is modified.

  However, if you want to disable this functionality and gain full control over
  when the scene is rendered yourself, you can turn off autoredraw in the
  render manager and render the scene by calling this method.
*/
void
QuarterWidget::redraw(void)
{
  // we're triggering the next paintGL(). Set a flag to remember this
  // to avoid that we process the delay queue in paintGL()
  PRIVATE(this)->processdelayqueue = false;
#if QT_VERSION >= 0x050500 && QT_VERSION < 0x050600
  // With Qt 5.5.x there is a major performance problem
  this->viewport()->update();
#else
  this->viewport()->repaint();
#endif
}

/*!
  Overridden from QGLWidget to render the scenegraph
 */
void
QuarterWidget::actualRedraw(void)
{
  PRIVATE(this)->sorendermanager->render(PRIVATE(this)->clearwindow,
                                         PRIVATE(this)->clearzbuffer);
}


/*!
  Passes an event to the eventmanager.

  \param[in] event to pass
  \retval Returns true if the event was successfully processed
*/
bool
QuarterWidget::processSoEvent(const SoEvent * event)
{
  return
    event &&
    PRIVATE(this)->soeventmanager &&
    PRIVATE(this)->soeventmanager->processEvent(event);
}

/*!
  \property QuarterWidget::backgroundColor
  \copydoc QuarterWidget::setBackgroundColor
*/

/*!
  Set backgroundcolor to a given QColor

  Remember that QColors are given in integers between 0 and 255, as
  opposed to SbColor4f which is in [0 ,1]. The default alpha value for
  a QColor is 255, but you'll probably want to set it to zero before
  using it as an OpenGL clear color.
 */
void
QuarterWidget::setBackgroundColor(const QColor & color)
{
  SbColor4f bgcolor(SbClamp(color.red()   / 255.0, 0.0, 1.0),
                    SbClamp(color.green() / 255.0, 0.0, 1.0),
                    SbClamp(color.blue()  / 255.0, 0.0, 1.0),
                    SbClamp(color.alpha() / 255.0, 0.0, 1.0));

  PRIVATE(this)->sorendermanager->setBackgroundColor(bgcolor);
  PRIVATE(this)->sorendermanager->scheduleRedraw();
}

/*!
  Returns color used for clearing the rendering area before
  rendering the scene.
 */
QColor
QuarterWidget::backgroundColor(void) const
{
  SbColor4f bg = PRIVATE(this)->sorendermanager->getBackgroundColor();

  return QColor(SbClamp(int(bg[0] * 255.0), 0, 255),
                SbClamp(int(bg[1] * 255.0), 0, 255),
                SbClamp(int(bg[2] * 255.0), 0, 255),
                SbClamp(int(bg[3] * 255.0), 0, 255));
}

/*!
  Returns the context menu used by the widget.
*/
QMenu *
QuarterWidget::getContextMenu(void) const
{
  return PRIVATE(this)->contextMenu();
}

/*!
  \retval Is context menu enabled?
*/
bool
QuarterWidget::contextMenuEnabled(void) const
{
  return PRIVATE(this)->contextmenuenabled;
}

/*!
  \property QuarterWidget::contextMenuEnabled

  \copydetails QuarterWidget::setContextMenuEnabled
*/

/*!
  Controls the display of the contextmenu

  \param[in] yes Context menu on?
*/
void
QuarterWidget::setContextMenuEnabled(bool yes)
{
  PRIVATE(this)->contextmenuenabled = yes;
}

/*!
  Convenience method that adds a state machine to the current
  SoEventManager.  It also initializes the scene graph
  root and active camera for the state machine, and finally it sets
  up the default Quarter cursor handling.

  \sa removeStateMachine
*/
void
QuarterWidget::addStateMachine(SoScXMLStateMachine * statemachine)
{
  SoEventManager * em = this->getSoEventManager();
  em->addSoScXMLStateMachine(statemachine);
  statemachine->setSceneGraphRoot(this->getSoRenderManager()->getSceneGraph());
  statemachine->setActiveCamera(this->getSoRenderManager()->getCamera());
  statemachine->addStateChangeCallback(QuarterWidgetP::statechangecb, PRIVATE(this));
}

/*!
  Convenience method that removes a state machine to the current
  SoEventManager.

  \sa addStateMachine
*/
void
QuarterWidget::removeStateMachine(SoScXMLStateMachine * statemachine)
{
  SoEventManager * em = this->getSoEventManager();
  statemachine->setSceneGraphRoot(NULL);
  statemachine->setActiveCamera(NULL);
  em->removeSoScXMLStateMachine(statemachine);
}

/*!
  See \ref QWidget::minimumSizeHint
 */
QSize
QuarterWidget::minimumSizeHint(void) const
{
  return QSize(50, 50);
}

/*!  Returns a list of grouped actions that corresponds to the
  TransparencyType enum. If you want to create a menu in your
  application that controls the transparency type used in
  QuarterWidget, add these actions to the menu.
 */
QList<QAction *>
QuarterWidget::transparencyTypeActions(void) const
{
  return PRIVATE(this)->transparencyTypeActions();
}

/*!  Returns a list of grouped actions that corresponds to the
  StereoMode enum. If you want to create a menu in your
  application that controls the stereo mode used in
  QuarterWidget, add these actions to the menu.
 */
QList<QAction *>
QuarterWidget::stereoModeActions(void) const
{
  return PRIVATE(this)->stereoModeActions();
}

/*!  Returns a list of grouped actions that corresponds to the
  RenderMode enum. If you want to create a menu in your
  application that controls the render mode type used in
  QuarterWidget, add these actions to the menu.
 */
QList<QAction *>
QuarterWidget::renderModeActions(void) const
{
  return PRIVATE(this)->renderModeActions();
}

/*!
  \property QuarterWidget::navigationModeFile

  A url pointing to a navigation mode file which is a scxml file
  that defines the possible states for the Coin navigation system

  Supports:
  \li \b coin for internal coinresources
  \li \b file for filesystem path to resources

  \sa scxml
*/

/*!
  Removes any navigationModeFile set.
*/
void
QuarterWidget::resetNavigationModeFile(void) {
  this->setNavigationModeFile(QUrl());
}

/*!
  Sets a navigation mode file. Supports the schemes "coin" and "file"

  \param[in] url Url to the resource
*/
void
QuarterWidget::setNavigationModeFile(const QUrl & url)
{
  QString filename;

  if (url.scheme()=="coin") {
    filename = url.path();
    //FIXME: This conditional needs to be implemented when the
    //CoinResources systems if working
#if 0
    //#if (COIN_MAJOR_VERSION==3) && (COIN_MINOR_VERSION==0)
#endif
    //Workaround for differences between url scheme, and Coin internal
    //scheme in Coin 3.0.
    if (filename[0]=='/') {
      filename.remove(0,1);
    }
#if 0
    //#endif
#endif
    filename = url.scheme()+':'+filename;
  }
  else if (url.scheme()=="file")
    filename = url.toLocalFile();
  else if (url.isEmpty()) {
    if (PRIVATE(this)->currentStateMachine) {
      this->removeStateMachine(PRIVATE(this)->currentStateMachine);
      delete PRIVATE(this)->currentStateMachine;
      PRIVATE(this)->currentStateMachine = NULL;
      PRIVATE(this)->navigationModeFile = url;
    }
    return;
  }
  else {
    qDebug()<<url.scheme()<<"is not recognized";
    return;
  }

  QByteArray filenametmp = filename.toLocal8Bit();
  ScXMLStateMachine * stateMachine = NULL;

  if (filenametmp.startsWith("coin:")){
    stateMachine = ScXML::readFile(filenametmp.data());
  }
  else {
    //Use Qt to read the file in case it is a Qt resource
    QFile file(filenametmp);
    if (file.open(QIODevice::ReadOnly)){
      QByteArray fileContents = file.readAll();
#if COIN_MAJOR_VERSION >= 4
      stateMachine = ScXML::readBuffer(SbByteBuffer(fileContents.size(), fileContents.constData()));
#else
      stateMachine = ScXML::readBuffer(fileContents.constData());
#endif
      file.close();
    }
  }

  if (stateMachine &&
      stateMachine->isOfType(SoScXMLStateMachine::getClassTypeId())) {
    SoScXMLStateMachine * newsm = 
      static_cast<SoScXMLStateMachine *>(stateMachine);
    if (PRIVATE(this)->currentStateMachine) {
      this->removeStateMachine(PRIVATE(this)->currentStateMachine);
      delete PRIVATE(this)->currentStateMachine;
    }
    this->addStateMachine(newsm);
    newsm->initialize();
    PRIVATE(this)->currentStateMachine = newsm;
  }
  else {
    if (stateMachine)
      delete stateMachine;
    qDebug()<<filename;
    qDebug()<<"Unable to load"<<url;
    return;
  }

  //If we have gotten this far, we have successfully loaded the
  //navigation file, so we set the property
  PRIVATE(this)->navigationModeFile = url;

  if (QUrl(DEFAULT_NAVIGATIONFILE) == PRIVATE(this)->navigationModeFile ) {

    // set up default cursors for the examiner navigation states
    //FIXME: It may be overly restrictive to not do this for arbitrary
    //navigation systems? - BFG 20090117
    this->setStateCursor("interact", Qt::ArrowCursor);
    this->setStateCursor("idle", Qt::OpenHandCursor);
#if QT_VERSION >= 0x040200
    this->setStateCursor("rotate", Qt::ClosedHandCursor);
#endif
    this->setStateCursor("pan", Qt::SizeAllCursor);
    this->setStateCursor("zoom", Qt::SizeVerCursor);
    this->setStateCursor("dolly", Qt::SizeVerCursor);
    this->setStateCursor("seek", Qt::CrossCursor);
    this->setStateCursor("spin", Qt::OpenHandCursor);
  }
}

/*!
  \retval The current navigationModeFile
*/
const QUrl &
QuarterWidget::navigationModeFile(void) const
{
  return PRIVATE(this)->navigationModeFile;
}

#undef PRIVATE
