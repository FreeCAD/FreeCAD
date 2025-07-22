/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# include <GL/glext.h>
# include <GL/glu.h>
# endif

# include <fmt/format.h>

# include <Inventor/SbBox.h>
# include <Inventor/SoEventManager.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetMatrixAction.h>
# include <Inventor/actions/SoHandleEventAction.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/annex/HardCopy/SoVectorizePSAction.h>
# include <Inventor/annex/Profiler/SoProfiler.h>
# include <Inventor/annex/Profiler/elements/SoProfilerElement.h>
# include <Inventor/details/SoDetail.h>
# include <Inventor/elements/SoLightModelElement.h>
# include <Inventor/elements/SoOverrideElement.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/events/SoMotion3Event.h>
# include <Inventor/manips/SoClipPlaneManip.h>
# include <Inventor/nodes/SoAnnotation.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCallback.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoScale.h>
# include <Inventor/nodes/SoSelection.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSphere.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoTranslation.h>
# include <QBitmap>
# include <QEventLoop>
#if defined(Q_OS_LINUX) && QT_VERSION < QT_VERSION_CHECK(6,6,0)
# include <QGuiApplication>
# define HAS_QTBUG_95434
#endif
# include <QKeyEvent>
# include <QMessageBox>
# include <QMimeData>
# include <QOpenGLFramebufferObject>
# include <QOpenGLWidget>
# include <QSurfaceFormat>
# include <QTimer>
# include <QVariantAnimation>
# include <QWheelEvent>
#endif

#include <App/Document.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Base/Console.h>
#include <Base/FileInfo.h>
#include <Base/Sequencer.h>
#include <Base/Profiler.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>
#include <Base/Tools2D.h>
#include <Quarter/devices/InputDevice.h>
#include <Quarter/eventhandlers/EventFilter.h>

#include "View3DInventorViewer.h"
#include "Application.h"
#include "Document.h"
#include "GLPainter.h"
#include "Inventor/SoAxisCrossKit.h"
#include "Inventor/SoFCBackgroundGradient.h"
#include "Inventor/SoFCBoundingBox.h"
#include "MainWindow.h"
#include "Multisample.h"
#include "NaviCube.h"
#include "Navigation/NavigationStyle.h"
#include "Selection.h"
#include "SoDevicePixelRatioElement.h"
#include "SoFCDB.h"
#include "SoFCInteractiveElement.h"
#include "SoFCOffscreenRenderer.h"
#include "SoFCSelection.h"
#include "SoFCSelectionAction.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCVectorizeSVGAction.h"
#include "SoFCVectorizeU3DAction.h"
#include "SoTouchEvents.h"
#include "SpaceballEvent.h"
#include "View3DInventorRiftViewer.h"
#include "View3DViewerPy.h"
#include "ViewParams.h"
#include "ViewProvider.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderLink.h"
#include "Navigation/NavigationAnimator.h"
#include "Navigation/NavigationAnimation.h"
#include "Utilities.h"

#include <Inventor/nodes/SoRotation.h>
#include <Inventor/nodes/SoTransformSeparator.h>
#include <Inventor/So3DAnnotation.h>


FC_LOG_LEVEL_INIT("3DViewer", true, true)

// #define FC_LOGGING_CB

using namespace Gui;

// NOLINTBEGIN
// clang-format off
/*** zoom-style cursor ******/

#define ZOOM_WIDTH 16
#define ZOOM_HEIGHT 16
#define ZOOM_BYTES ((ZOOM_WIDTH + 7) / 8) * ZOOM_HEIGHT
#define ZOOM_HOT_X 5
#define ZOOM_HOT_Y 7

static unsigned char zoom_bitmap[ZOOM_BYTES] =
{
    0x00, 0x0f, 0x80, 0x1c, 0x40, 0x38, 0x20, 0x70,
    0x90, 0xe4, 0xc0, 0xcc, 0xf0, 0xfc, 0x00, 0x0c,
    0x00, 0x0c, 0xf0, 0xfc, 0xc0, 0xcc, 0x90, 0xe4,
    0x20, 0x70, 0x40, 0x38, 0x80, 0x1c, 0x00, 0x0f
};

static unsigned char zoom_mask_bitmap[ZOOM_BYTES] =
{
    0x00, 0x0f, 0x80, 0x1f, 0xc0, 0x3f, 0xe0, 0x7f,
    0xf0, 0xff, 0xf0, 0xff, 0xf0, 0xff, 0x00, 0x0f,
    0x00, 0x0f, 0xf0, 0xff, 0xf0, 0xff, 0xf0, 0xff,
    0xe0, 0x7f, 0xc0, 0x3f, 0x80, 0x1f, 0x00, 0x0f
};

/*** pan-style cursor *******/

#define PAN_WIDTH 16
#define PAN_HEIGHT 16
#define PAN_BYTES ((PAN_WIDTH + 7) / 8) * PAN_HEIGHT
#define PAN_HOT_X 7
#define PAN_HOT_Y 7

static unsigned char pan_bitmap[PAN_BYTES] =
{
    0xc0, 0x03, 0x60, 0x02, 0x20, 0x04, 0x10, 0x08,
    0x68, 0x16, 0x54, 0x2a, 0x73, 0xce, 0x01, 0x80,
    0x01, 0x80, 0x73, 0xce, 0x54, 0x2a, 0x68, 0x16,
    0x10, 0x08, 0x20, 0x04, 0x40, 0x02, 0xc0, 0x03
};

static unsigned char pan_mask_bitmap[PAN_BYTES] =
{
    0xc0, 0x03, 0xe0, 0x03, 0xe0, 0x07, 0xf0, 0x0f,
    0xe8, 0x17, 0xdc, 0x3b, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xdc, 0x3b, 0xe8, 0x17,
    0xf0, 0x0f, 0xe0, 0x07, 0xc0, 0x03, 0xc0, 0x03
};

/*** rotate-style cursor ****/

#define ROTATE_WIDTH 16
#define ROTATE_HEIGHT 16
#define ROTATE_BYTES ((ROTATE_WIDTH + 7) / 8) * ROTATE_HEIGHT
#define ROTATE_HOT_X 6
#define ROTATE_HOT_Y 8

static unsigned char rotate_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0x18, 0xb8, 0x0c, 0x90, 0xe4, 0x83,
    0x34, 0x86, 0x1c, 0x83, 0x00, 0x81, 0x00, 0xff,
    0xff, 0x00, 0x81, 0x00, 0xc1, 0x38, 0x61, 0x2c,
    0xc1, 0x27, 0x09, 0x30, 0x1d, 0x18, 0xf7, 0x0f
};

static unsigned char rotate_mask_bitmap[ROTATE_BYTES] = {
    0xf0, 0xef, 0xf8, 0xff, 0xfc, 0xff, 0xfc, 0xff,
    0x3c, 0xfe, 0x1c, 0xff, 0x00, 0xff, 0x00, 0xff,
    0xff, 0x00, 0xff, 0x00, 0xff, 0x38, 0x7f, 0x3c,
    0xff, 0x3f, 0xff, 0x3f, 0xff, 0x1f, 0xf7, 0x0f
};
// clang-format on
// NOLINTEND


/*!
As ProgressBar has no chance to control the incoming Qt events of Quarter so we need to stop
the event handling to prevent the scenegraph from being selected or deselected
while the progress bar is running.
*/
class Gui::ViewerEventFilter : public QObject
{
public:
    ViewerEventFilter() = default;
    ~ViewerEventFilter() override = default;

    bool eventFilter(QObject* obj, QEvent* event) override {
        // Bug #0000607: Some mice also support horizontal scrolling which however might
        // lead to some unwanted zooming when pressing the MMB for panning.
        // Thus, we filter out horizontal scrolling.
        if (event->type() == QEvent::Wheel) {
            auto we = static_cast<QWheelEvent*>(event);  // NOLINT
            if (qAbs(we->angleDelta().x()) > qAbs(we->angleDelta().y())) {
                return true;
            }
        }
        else if (event->type() == QEvent::KeyPress) {
            auto ke = static_cast<QKeyEvent*>(event);  // NOLINT
            if (ke->matches(QKeySequence::SelectAll)) {
                auto* viewer3d = static_cast<View3DInventorViewer*>(obj);
                auto* editingVP = viewer3d->getEditingViewProvider();
                if(!editingVP || !editingVP->selectAll())
                {
                    viewer3d->selectAll();
                }
                return true;
            }
        }

        if (Base::Sequencer().isRunning() && Base::Sequencer().isBlocking()) {
            return false;
        }

        if (event->type() == Spaceball::ButtonEvent::ButtonEventType) {
            auto buttonEvent = static_cast<Spaceball::ButtonEvent*>(event);  // NOLINT
            if (!buttonEvent) {
                Base::Console().log("invalid spaceball button event\n");
                return true;
            }
        }
        else if (event->type() == Spaceball::MotionEvent::MotionEventType) {
            auto motionEvent = static_cast<Spaceball::MotionEvent*>(event);  // NOLINT
            if (!motionEvent) {
                Base::Console().log("invalid spaceball motion event\n");
                return true;
            }
        }

        return false;
    }
};

class SpaceNavigatorDevice : public Quarter::InputDevice {
public:
    SpaceNavigatorDevice() : InputDevice(nullptr) {}
    ~SpaceNavigatorDevice() override = default;
    const SoEvent* translateEvent(QEvent* event) override {

        if (event->type() == Spaceball::MotionEvent::MotionEventType) {
            auto motionEvent = static_cast<Spaceball::MotionEvent*>(event);  // NOLINT
            if (!motionEvent) {
                Base::Console().log("invalid spaceball motion event\n");
                return nullptr;
            }

            motionEvent->setHandled(true);

            float xTrans{};
            float yTrans{};
            float zTrans{};
            xTrans = static_cast<float>(motionEvent->translationX());
            yTrans = static_cast<float>(motionEvent->translationY());
            zTrans = static_cast<float>(motionEvent->translationZ());
            SbVec3f translationVector(xTrans, yTrans, zTrans);

            constexpr const float rotationConstant(.0001F);
            SbRotation xRot;
            SbRotation yRot;
            SbRotation zRot;
            xRot.setValue(SbVec3f(1.0, 0.0, 0.0), static_cast<float>(motionEvent->rotationX()) * rotationConstant);
            yRot.setValue(SbVec3f(0.0, 1.0, 0.0), static_cast<float>(motionEvent->rotationY()) * rotationConstant);
            zRot.setValue(SbVec3f(0.0, 0.0, 1.0), static_cast<float>(motionEvent->rotationZ()) * rotationConstant);

            auto motion3Event = new SoMotion3Event;
            motion3Event->setTranslation(translationVector);
            motion3Event->setRotation(xRot * yRot * zRot);
            motion3Event->setPosition(this->mousepos);

            return motion3Event;
        }

        return nullptr;
    }
};

/** \defgroup View3D 3D Viewer
 *  \ingroup GUI
 *
 * The 3D Viewer is one of the major components in a CAD/CAE systems.
 * Therefore an overview and some remarks to the FreeCAD 3D viewing system.
 *
 * \section overview Overview
 * \todo Overview and complements for the 3D Viewer
 *
 * \section trouble Troubleshooting
 * When it's needed to capture OpenGL function calls then the utility apitrace
 * can be very useful: https://github.com/apitrace/apitrace/blob/master/docs/USAGE.markdown
 *
 * To better locate the problematic code it's possible to add custom log messages.
 * For the prerequisites check:
 * https://github.com/apitrace/apitrace/blob/master/docs/USAGE.markdown#
 * emitting-annotations-to-the-trace
 * \code
 * #include <GL/glext.h>
 * #include <Inventor/C/glue/gl.h>
 *
 * void GLRender(SoGLRenderAction* glra)
 * {
 *     int context = glra->getCacheContext();
 *     const cc_glglue * glue = cc_glglue_instance(context);
 *
 *     PFNGLPUSHDEBUGGROUPPROC glPushDebugGroup = (PFNGLPUSHDEBUGGROUPPROC)
 *     cc_glglue_getprocaddress(glue, "glPushDebugGroup");
 *     PFNGLDEBUGMESSAGEINSERTARBPROC glDebugMessageInsert = (PFNGLDEBUGMESSAGEINSERTARBPROC)
 *     cc_glglue_getprocaddress(glue, "glDebugMessageInsert");
 *     PFNGLPOPDEBUGGROUPPROC glPopDebugGroup = (PFNGLPOPDEBUGGROUPPROC)
 *     cc_glglue_getprocaddress(glue, "glPopDebugGroup");
 *
 *     glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, __FUNCTION__);
 * ...
 *     glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER,
 *                          0, GL_DEBUG_SEVERITY_MEDIUM, -1, "begin_blabla");
 * ...
 *     glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION, GL_DEBUG_TYPE_OTHER,
 *                          0, GL_DEBUG_SEVERITY_MEDIUM, -1, "end_blabla");
 * ...
 *     glPopDebugGroup();
 * }
 * \endcode
 */


// *************************************************************************

View3DInventorViewer::View3DInventorViewer(QWidget* parent, const QOpenGLWidget* sharewidget)
    : Quarter::SoQTQuarterAdaptor(parent, sharewidget)
    , SelectionObserver(false, ResolveMode::NoResolve)
    , editViewProvider(nullptr)
    , objectGroup(nullptr)
    , navigation(nullptr)
    , renderType(Native)
    , framebuffer(nullptr)
    , axisCross(nullptr)
    , axisGroup(nullptr)
    , rotationCenterGroup(nullptr)
    , editing(false)
    , redirected(false)
    , allowredir(false)
    , overrideMode("As Is")
    , _viewerPy(nullptr)
{
    init();
}

View3DInventorViewer::View3DInventorViewer(const QSurfaceFormat& format, QWidget* parent, const QOpenGLWidget* sharewidget)
    : Quarter::SoQTQuarterAdaptor(format, parent, sharewidget)
    , SelectionObserver(false, ResolveMode::NoResolve)
    , editViewProvider(nullptr)
    , objectGroup(nullptr)
    , navigation(nullptr)
    , renderType(Native)
    , framebuffer(nullptr)
    , axisCross(nullptr)
    , axisGroup(nullptr)
    , rotationCenterGroup(nullptr)
    , editing(false)
    , redirected(false)
    , allowredir(false)
    , overrideMode("As Is")
    , _viewerPy(nullptr)
{
    init();
}

void View3DInventorViewer::init()
{
    static bool _cacheModeInited;
    if (!_cacheModeInited) {
        _cacheModeInited = true;
        pcViewProviderRoot = nullptr;
        setRenderCache(-1);
    }

    shading = true;
    fpsEnabled = false;
    vboEnabled = false;

    attachSelection();

    // Coin should not clear the pixel-buffer, so the background image
    // is not removed.
    this->setClearWindow(false);

    // setting up the defaults for the spin rotation
    initialize();

#ifdef TRACY_ENABLE
    SoProfiler::init();
    SoProfiler::enable(TRUE);
#endif

    // NOLINTBEGIN
    auto cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 1);
    cam->height = 1;
    cam->nearDistance = 0.5;
    cam->farDistance = 1.5;
    // NOLINTEND

    // setup light sources
    SoDirectionalLight* hl = this->getHeadlight();

    environment = new SoEnvironment();
    environment->ref();
    environment->setName("environment");

    backlight = new SoDirectionalLight();
    backlight->ref();
    backlight->setName("backlight");
    backlight->direction.setValue(-hl->direction.getValue());
    backlight->on.setValue(false); // by default off

    fillLight = new SoDirectionalLight();
    fillLight->ref();
    fillLight->setName("filllight");
    fillLight->direction.setValue(-0.60F, -0.35F, -0.79F);
    fillLight->intensity.setValue(0.6F);
    fillLight->color.setValue(0.95F, 0.95F, 1.0F);
    fillLight->on.setValue(false); // by default off

    // Set up background scenegraph with image in it.
    backgroundroot = new SoSeparator;
    backgroundroot->ref();
    this->backgroundroot->addChild(cam);
    this->backgroundroot->setName("backgroundroot");

    // Background stuff
    pcBackGround = new SoFCBackgroundGradient;
    pcBackGround->ref();

    // Set up foreground, overlaid scenegraph.
    this->foregroundroot = new SoSeparator;
    this->foregroundroot->ref();
    this->foregroundroot->setName("foregroundroot");

    auto lm = new SoLightModel;
    lm->model = SoLightModel::BASE_COLOR;

    auto bc = new SoBaseColor;
    bc->rgb = SbColor(1, 1, 0);

    // NOLINTBEGIN
    cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 5);
    cam->height = 10;
    cam->nearDistance = 0;
    cam->farDistance = 10;
    // NOLINTEND

    lightRotation = new SoRotation;
    lightRotation->ref();
    lightRotation->rotation.connectFrom(&cam->orientation);

    this->foregroundroot->addChild(cam);
    this->foregroundroot->addChild(lm);
    this->foregroundroot->addChild(bc);

    auto threePointLightingSeparator = new SoTransformSeparator;
    threePointLightingSeparator->addChild(lightRotation);
    threePointLightingSeparator->addChild(this->fillLight);

    this->foregroundroot->addChild(cam);

    // NOTE: For every mouse click event the SoFCUnifiedSelection searches for the picked
    // point which causes a certain slow-down because for all objects the primitives
    // must be created. Using an SoSeparator avoids this drawback.
    selectionRoot = new Gui::SoFCUnifiedSelection();
    selectionRoot->applySettings();

    // set the ViewProvider root node
    pcViewProviderRoot = selectionRoot;
    pcViewProviderRoot->addChild(threePointLightingSeparator);
    pcViewProviderRoot->addChild(environment);

    // increase refcount before passing it to setScenegraph(), to avoid
    // premature destruction
    pcViewProviderRoot->ref();
    setSceneGraph(pcViewProviderRoot);
    // Event callback node
    pEventCallback = new SoEventCallback();
    pEventCallback->setUserData(this);
    pEventCallback->ref();
    pcViewProviderRoot->addChild(pEventCallback);
    pEventCallback->addEventCallback(SoEvent::getClassTypeId(), handleEventCB, this);

    dimensionRoot = new SoSwitch(SO_SWITCH_NONE);
    dimensionRoot->setName("RootDimensions");
    pcViewProviderRoot->addChild(dimensionRoot);
    auto dimensions3d = new SoSwitch();
    dimensions3d->setName("_3dDimensions");
    dimensionRoot->addChild(dimensions3d); //first one will be for the 3d dimensions.
    auto dimensionsDelta = new SoSwitch();
    dimensionsDelta->setName("DeltaDimensions");
    dimensionRoot->addChild(dimensionsDelta); //second one for the delta dimensions.

    // This is a callback node that logs all action that traverse the Inventor tree.
#if defined (FC_DEBUG) && defined(FC_LOGGING_CB)
    SoCallback* cb = new SoCallback;
    cb->setCallback(interactionLoggerCB, this);
    pcViewProviderRoot->addChild(cb);
#endif

    inventorSelection = std::make_unique<View3DInventorSelection>(selectionRoot);

    pcClipPlane = nullptr;

    pcEditingRoot = new SoSeparator;
    pcEditingRoot->ref();
    pcEditingRoot->setName("EditingRoot");
    pcEditingTransform = new SoTransform;
    pcEditingTransform->ref();
    pcEditingTransform->setName("EditingTransform");
    restoreEditingRoot = false;
    pcEditingRoot->addChild(pcEditingTransform);
    pcViewProviderRoot->addChild(pcEditingRoot);

    // Create group for the physical object
    objectGroup = new SoGroup();
    objectGroup->ref();
    objectGroup->setName("ObjectGroup");
    pcViewProviderRoot->addChild(objectGroup);

    // Set our own render action which show a bounding box if
    // the SoFCSelection::BOX style is set
    //
    // Important note:
    // When creating a new GL render action we have to copy over the cache context id
    // because otherwise we may get strange rendering behaviour. For more details see
    // https://forum.freecad.org/viewtopic.php?f=10&t=7486&start=120#p74398 and for
    // the fix and some details what happens behind the scene have a look at this
    // https://forum.freecad.org/viewtopic.php?f=10&t=7486&p=74777#p74736
    uint32_t id = this->getSoRenderManager()->getGLRenderAction()->getCacheContext();
    auto boxSelectionAction = new SoBoxSelectionRenderAction;
    this->getSoRenderManager()->setGLRenderAction(boxSelectionAction);
    this->getSoRenderManager()->getGLRenderAction()->setCacheContext(id);

#ifdef TRACY_ENABLE
    boxSelectionAction->enableElement(SoProfilerElement::getClassTypeId(), SoProfilerElement::getClassStackIndex());
#endif

    // set the transparency and antialiasing settings
    getSoRenderManager()->getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);

    // Settings
    setSeekTime(0.4F);  // NOLINT

    if (!isSeekValuePercentage()) {
        setSeekValueAsPercentage(true);
    }

    setSeekDistance(100);  // NOLINT
    setViewing(false);

    setBackgroundColor(QColor(25, 25, 25));  // NOLINT
    setGradientBackground(Background::LinearGradient);

    // set some callback functions for user interaction
    addStartCallback(interactionStartCB);
    addFinishCallback(interactionFinishCB);

    //filter a few qt events
    viewerEventFilter = new ViewerEventFilter;
    installEventFilter(viewerEventFilter);
#if defined(USE_3DCONNEXION_NAVLIB)
    ParameterGrp::handle hViewGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View");
    if (hViewGrp->GetBool("LegacySpaceMouseDevices", false)) {
        getEventFilter()->registerInputDevice(new SpaceNavigatorDevice);
    }
#else
    getEventFilter()->registerInputDevice(new SpaceNavigatorDevice);
#endif
    getEventFilter()->registerInputDevice(new GesturesDevice(this));

    try{
        this->grabGesture(Qt::PanGesture);
        this->grabGesture(Qt::PinchGesture);
    } catch (Base::Exception &e) {
        Base::Console().warning("Failed to set up gestures. Error: %s\n", e.what());
    } catch (...) {
        Base::Console().warning("Failed to set up gestures. Unknown error.\n");
    }

    //create the cursors
    createStandardCursors(devicePixelRatio());
    connect(this, &View3DInventorViewer::devicePixelRatioChanged,
            this, &View3DInventorViewer::createStandardCursors);

    naviCube = new NaviCube(this);
    naviCubeEnabled = true;

    updateColors();
}

View3DInventorViewer::~View3DInventorViewer()
{
    // to prevent following OpenGL error message: "Texture is not valid in the current context. Texture has not been destroyed"
    aboutToDestroyGLContext();

    // It can happen that a document has several MDI views and when the about to be
    // closed 3D view is in edit mode the corresponding view provider must be restored
    // because otherwise it might be left in a broken state
    // See https://forum.freecad.org/viewtopic.php?f=3&t=39720
    if (restoreEditingRoot) {
        resetEditingRoot(false);
    }

    // cleanup
    this->backgroundroot->unref();
    this->backgroundroot = nullptr;
    this->foregroundroot->unref();
    this->foregroundroot = nullptr;
    this->pcBackGround->unref();
    this->pcBackGround = nullptr;

    setSceneGraph(nullptr);
    this->pEventCallback->unref();
    this->pEventCallback = nullptr;
    // Note: It can happen that there is still someone who references
    // the root node but isn't destroyed when closing this viewer so
    // that it prevents all children from being deleted. To reduce this
    // likelihood we explicitly remove all child nodes now.
    coinRemoveAllChildren(this->pcViewProviderRoot);
    this->pcViewProviderRoot->unref();
    this->pcViewProviderRoot = nullptr;
    this->objectGroup->unref();
    this->objectGroup = nullptr;
    this->backlight->unref();
    this->backlight = nullptr;
    this->fillLight->unref();
    this->fillLight = nullptr;
    this->environment->unref();
    this->environment = nullptr;

    inventorSelection.reset(nullptr);

    this->pcEditingRoot->unref();
    this->pcEditingTransform->unref();

    if (this->pcClipPlane) {
        this->pcClipPlane->unref();
    }

    delete this->navigation;

    // Note: When closing the application the main window doesn't exist any more.
    if (getMainWindow()) {
        getMainWindow()->setPaneText(2, QLatin1String(""));
    }

    detachSelection();

    removeEventFilter(viewerEventFilter);
    delete viewerEventFilter;

    if (_viewerPy) {
        static_cast<View3DInventorViewerPy*>(_viewerPy)->_viewer = nullptr;
        Py_DECREF(_viewerPy);
    }

    // In the init() function we have overridden the default SoGLRenderAction with our
    // own instance of SoBoxSelectionRenderAction and SoRenderManager destroyed the default.
    // But it does this only once so that now we have to explicitly destroy our instance in
    // order to free the memory.
    SoGLRenderAction* glAction = this->getSoRenderManager()->getGLRenderAction();
    this->getSoRenderManager()->setGLRenderAction(nullptr);
    delete glAction;
}

static QCursor createCursor(QBitmap &bitmap, QBitmap &mask, int hotX, int hotY, double dpr)
{
#if defined(Q_OS_WIN32)
    bitmap.setDevicePixelRatio(dpr);
    mask.setDevicePixelRatio(dpr);
#else
    Q_UNUSED(dpr)
#endif
#ifdef HAS_QTBUG_95434
    if (qGuiApp->platformName() == QLatin1String("wayland")) {
        QImage img = bitmap.toImage();
        img.convertTo(QImage::Format_ARGB32);
        QPixmap pixmap = QPixmap::fromImage(img);
        pixmap.setMask(mask);
        return QCursor(pixmap, hotX, hotY);
    }
#endif

    return QCursor(bitmap, mask, hotX, hotY);
}

void View3DInventorViewer::createStandardCursors(double dpr)
{
    QBitmap cursor = QBitmap::fromData(QSize(ROTATE_WIDTH, ROTATE_HEIGHT), rotate_bitmap);
    QBitmap mask = QBitmap::fromData(QSize(ROTATE_WIDTH, ROTATE_HEIGHT), rotate_mask_bitmap);
    spinCursor = createCursor(cursor, mask, ROTATE_HOT_X, ROTATE_HOT_Y, dpr);

    cursor = QBitmap::fromData(QSize(ZOOM_WIDTH, ZOOM_HEIGHT), zoom_bitmap);
    mask = QBitmap::fromData(QSize(ZOOM_WIDTH, ZOOM_HEIGHT), zoom_mask_bitmap);
    zoomCursor = createCursor(cursor, mask, ZOOM_HOT_X, ZOOM_HOT_Y, dpr);

    cursor = QBitmap::fromData(QSize(PAN_WIDTH, PAN_HEIGHT), pan_bitmap);
    mask = QBitmap::fromData(QSize(PAN_WIDTH, PAN_HEIGHT), pan_mask_bitmap);
    panCursor = createCursor(cursor, mask, PAN_HOT_X, PAN_HOT_Y, dpr);
}

void View3DInventorViewer::aboutToDestroyGLContext()
{
    if (naviCube) {
        if (auto gl = qobject_cast<QOpenGLWidget*>(this->viewport())) {
            gl->makeCurrent();
        }
        delete naviCube;
        naviCube = nullptr;
        naviCubeEnabled = false;
    }
}

void View3DInventorViewer::setDocument(Gui::Document* pcDocument)
{
    // write the document the viewer belongs to the selection node
    guiDocument = pcDocument;
    selectionRoot->pcDocument = pcDocument;
    inventorSelection->setDocument(pcDocument);

    if(pcDocument) {
        const auto &sels = Selection().getSelection(pcDocument->getDocument()->getName(), ResolveMode::NoResolve);
        for(auto &sel : sels) {
            SelectionChanges Chng(SelectionChanges::ShowSelection,
                    sel.DocName,sel.FeatName,sel.SubName);
            onSelectionChanged(Chng);
        }
    }
}

Document* View3DInventorViewer::getDocument() {
    return guiDocument;
}


void View3DInventorViewer::initialize()
{
    navigation = new CADNavigationStyle();
    navigation->setViewer(this);

    this->axiscrossEnabled = true;
    this->axiscrossSize = 10;  // NOLINT
}

/// @cond DOXERR
void View3DInventorViewer::onSelectionChanged(const SelectionChanges & reason)
{
    if(!getDocument()) {
        return;
    }

    SelectionChanges Reason(reason);

    if(Reason.pDocName && *Reason.pDocName &&
       strcmp(getDocument()->getDocument()->getName(),Reason.pDocName)!=0) {
        return;
    }

    switch(Reason.Type) {
    case SelectionChanges::ShowSelection:
    case SelectionChanges::HideSelection:
        if (Reason.Type == SelectionChanges::ShowSelection) {
            Reason.Type = SelectionChanges::AddSelection;
        }
        else {
            Reason.Type = SelectionChanges::RmvSelection;
        }
        // fall through
    case SelectionChanges::SetPreselect:
        if(Reason.SubType != SelectionChanges::MsgSource::TreeView) {
            break;
        }
        // fall through
    case SelectionChanges::RmvPreselect:
    case SelectionChanges::RmvPreselectSignal:
    case SelectionChanges::SetSelection:
    case SelectionChanges::AddSelection:
    case SelectionChanges::RmvSelection:
    case SelectionChanges::ClrSelection:
        inventorSelection->checkGroupOnTop(Reason);
        break;
    case SelectionChanges::SetPreselectSignal:
        break;
    default:
        return;
    }

    if(Reason.Type == SelectionChanges::RmvPreselect ||
       Reason.Type == SelectionChanges::RmvPreselectSignal)
    {
        //Hint: do not create a tmp. instance of SelectionChanges
        SelectionChanges selChanges(SelectionChanges::RmvPreselect);
        SoFCPreselectionAction preselectionAction(selChanges);
        preselectionAction.apply(pcViewProviderRoot);
    } else {
        SoFCSelectionAction selectionAction(Reason);
        selectionAction.apply(pcViewProviderRoot);
    }
}
/// @endcond

bool View3DInventorViewer::searchNode(SoNode* node) const
{
    SoSearchAction searchAction;
    searchAction.setNode(node);
    searchAction.setInterest(SoSearchAction::FIRST);
    searchAction.apply(this->getSceneGraph());
    SoPath* selectionPath = searchAction.getPath();
    return selectionPath != nullptr;
}

bool View3DInventorViewer::hasViewProvider(ViewProvider* pcProvider) const
{
    return _ViewProviderSet.find(pcProvider) != _ViewProviderSet.end();
}

bool View3DInventorViewer::containsViewProvider(const ViewProvider* vp) const
{
    SoSearchAction sa;
    sa.setNode(vp->getRoot());
    sa.setSearchingAll(false);
    sa.apply(getSoRenderManager()->getSceneGraph());
    return sa.getPath() != nullptr;
}

/// adds an ViewProvider to the view, e.g. from a feature
void View3DInventorViewer::addViewProvider(ViewProvider* pcProvider)
{
    SoSeparator* root = pcProvider->getRoot();

    if (root) {
        if (pcProvider->canAddToSceneGraph()) {
            // Add to the physical object group if related to the physical object otherwise add to the scene graph
            if (pcProvider->isPartOfPhysicalObject()) {
                objectGroup->addChild(root);
            }
            else {
                pcViewProviderRoot->addChild(root);
            }
        }
        _ViewProviderMap[root] = pcProvider;
    }

    if (SoSeparator* fore = pcProvider->getFrontRoot()) {
        foregroundroot->addChild(fore);
    }

    if (SoSeparator* back = pcProvider->getBackRoot()) {
        backgroundroot->addChild(back);
    }

    pcProvider->setOverrideMode(this->getOverrideMode());
    _ViewProviderSet.insert(pcProvider);
}

void View3DInventorViewer::removeViewProvider(ViewProvider* pcProvider)
{
    if (this->editViewProvider == pcProvider) {
        resetEditingViewProvider();
    }

    SoSeparator* root = pcProvider->getRoot();

    if (root) {
        int index = objectGroup->findChild(root);
        if (index >= 0) {
            objectGroup->removeChild(index);
        }

        index = pcViewProviderRoot->findChild(root);
        if (index >= 0) {
            pcViewProviderRoot->removeChild(index);
        }
        _ViewProviderMap.erase(root);
    }

    if (SoSeparator* fore = pcProvider->getFrontRoot()) {
        foregroundroot->removeChild(fore);
    }

    if (SoSeparator* back = pcProvider->getBackRoot()) {
        backgroundroot->removeChild(back);
    }

    _ViewProviderSet.erase(pcProvider);
}

void View3DInventorViewer::setEditingTransform(const Base::Matrix4D &mat)
{
    // NOLINTBEGIN
    if (pcEditingTransform) {
        double dMtrx[16];
        mat.getGLMatrix(dMtrx);
        pcEditingTransform->setMatrix(SbMatrix(
                    dMtrx[0], dMtrx[1], dMtrx[2],  dMtrx[3],
                    dMtrx[4], dMtrx[5], dMtrx[6],  dMtrx[7],
                    dMtrx[8], dMtrx[9], dMtrx[10], dMtrx[11],
                    dMtrx[12],dMtrx[13],dMtrx[14], dMtrx[15]));
    }
    // NOLINTEND
}

void View3DInventorViewer::setupEditingRoot(SoNode *node, const Base::Matrix4D *mat) {
    if(!editViewProvider) {
        return;
    }

    resetEditingRoot(false);
    if(mat) {
        setEditingTransform(*mat);
    }
    else {
        setEditingTransform(getDocument()->getEditingTransform());
    }
    if(node) {
        restoreEditingRoot = false;
        pcEditingRoot->addChild(node);
        return;
    }
    restoreEditingRoot = true;
    auto root = editViewProvider->getRoot();
    for(int i=0,count=root->getNumChildren();i<count;++i) {
        SoNode *node = root->getChild(i);
        if(node != editViewProvider->getTransformNode()) {
            pcEditingRoot->addChild(node);
        }
    }
    coinRemoveAllChildren(root);
    ViewProviderLink::updateLinks(editViewProvider);
}

void View3DInventorViewer::resetEditingRoot(bool updateLinks)
{
    if(!editViewProvider || pcEditingRoot->getNumChildren()<=1) {
        return;
    }
    if(!restoreEditingRoot) {
        pcEditingRoot->getChildren()->truncate(1);
        return;
    }
    restoreEditingRoot = false;
    auto root = editViewProvider->getRoot();
    if (root->getNumChildren()) {
        FC_ERR("WARNING!!! Editing view provider root node is tampered");
    }
    root->addChild(editViewProvider->getTransformNode());
    for (int i=1,count=pcEditingRoot->getNumChildren();i<count;++i) {
        root->addChild(pcEditingRoot->getChild(i));
    }
    pcEditingRoot->getChildren()->truncate(1);

    // handle exceptions eventually raised by ViewProviderLink
    try {
        if (updateLinks) {
            ViewProviderLink::updateLinks(editViewProvider);
        }
    }
    catch (const Py::Exception& e) {
        /* coverity[UNCAUGHT_EXCEPT] Uncaught exception */
        // Coverity created several reports when removeViewProvider()
        // is used somewhere in a destructor which indirectly invokes
        // resetEditingRoot().
        // Now theoretically Py::type can throw an exception which nowhere
        // will be handled and thus terminates the application. So, add an
        // extra try/catch block here.
        try {
            Py::Object py = Py::type(e);
            if (py.isString()) {
                Py::String str(py);
                Base::Console().warning("%s\n", str.as_std_string("utf-8").c_str());
            }
            else {
                Py::String str(py.repr());
                Base::Console().warning("%s\n", str.as_std_string("utf-8").c_str());
            }
            // Prints message to console window if we are in interactive mode
            PyErr_Print();
        }
        catch (Py::Exception& e) {
            e.clear();
            Base::Console().error("Unexpected exception raised in View3DInventorViewer::resetEditingRoot\n");
        }
    }
}

SoPickedPoint* View3DInventorViewer::getPointOnRay(const SbVec2s& pos, const ViewProvider* vp) const
{
    SoPath *path{};
    if (vp == editViewProvider && pcEditingRoot->getNumChildren() > 1) {
        path = new SoPath(1);
        path->ref();
        path->append(pcEditingRoot);
    }
    else {
        //first get the path to this node and calculate the current transformation
        SoSearchAction sa;
        sa.setNode(vp->getRoot());
        sa.setSearchingAll(true);
        sa.apply(getSoRenderManager()->getSceneGraph());
        path = sa.getPath();
        if (!path) {
            return nullptr;
        }
        path->ref();
    }
    SoGetMatrixAction gm(getSoRenderManager()->getViewportRegion());
    gm.apply(path);

    auto trans = new SoTransform;
    trans->setMatrix(gm.getMatrix());
    trans->ref();

    // build a temporary scenegraph only keeping this viewproviders nodes and the accumulated
    // transformation
    auto root = new SoSeparator;
    root->ref();
    root->addChild(getSoRenderManager()->getCamera());
    root->addChild(trans);
    root->addChild(path->getTail());

    //get the picked point
    SoRayPickAction rp(getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.setRadius(getPickRadius());
    rp.apply(root);
    root->unref();
    trans->unref();
    path->unref();

    SoPickedPoint* pick = rp.getPickedPoint();
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

SoPickedPoint* View3DInventorViewer::getPointOnRay(const SbVec3f& pos, const SbVec3f& dir, const ViewProvider* vp) const
{
    // Note: There seems to be a  bug with setRay() which causes SoRayPickAction
    // to fail to get intersections between the ray and a line

    SoPath *path{};
    if (vp == editViewProvider && pcEditingRoot->getNumChildren() > 1) {
        path = new SoPath(1);
        path->ref();
        path->append(pcEditingRoot);
    }
    else {
        //first get the path to this node and calculate the current setTransformation
        SoSearchAction sa;
        sa.setNode(vp->getRoot());
        sa.setSearchingAll(true);
        sa.apply(getSoRenderManager()->getSceneGraph());
        path = sa.getPath();
        if (!path) {
            return nullptr;
        }
        path->ref();
    }
    SoGetMatrixAction gm(getSoRenderManager()->getViewportRegion());
    gm.apply(path);

    // build a temporary scenegraph only keeping this viewproviders nodes and the accumulated
    // transformation
    auto trans = new SoTransform;
    trans->ref();
    trans->setMatrix(gm.getMatrix());

    auto root = new SoSeparator;
    root->ref();
    root->addChild(getSoRenderManager()->getCamera());
    root->addChild(trans);
    root->addChild(path->getTail());

    //get the picked point
    SoRayPickAction rp(getSoRenderManager()->getViewportRegion());
    rp.setRay(pos,dir);
    rp.setRadius(getPickRadius());
    rp.apply(root);
    root->unref();
    trans->unref();
    path->unref();

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

void View3DInventorViewer::setEditingViewProvider(Gui::ViewProvider* vp, int ModNum)
{
    this->editViewProvider = vp;
    this->editViewProvider->setEditViewer(this, ModNum);

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    this->navigation->findBoundingSphere();
#endif

    addEventCallback(SoEvent::getClassTypeId(), Gui::ViewProvider::eventCallback,this->editViewProvider);
}

/// reset from edit mode
void View3DInventorViewer::resetEditingViewProvider()
{
    if (this->editViewProvider) {

        // In case the event action still has grabbed a node when leaving edit mode
        // force to release it now
        SoEventManager* mgr = getSoEventManager();
        SoHandleEventAction* heaction = mgr->getHandleEventAction();
        if (heaction && heaction->getGrabber()) {
            heaction->releaseGrabber();
        }

        resetEditingRoot();

        this->editViewProvider->unsetEditViewer(this);
        removeEventCallback(SoEvent::getClassTypeId(), Gui::ViewProvider::eventCallback,this->editViewProvider);
        this->editViewProvider = nullptr;
    }
}

/// reset from edit mode
bool View3DInventorViewer::isEditingViewProvider() const
{
    return this->editViewProvider != nullptr;
}

/// return currently editing view provider
ViewProvider* View3DInventorViewer::getEditingViewProvider() const
{
    return this->editViewProvider;
}

/// display override mode
void View3DInventorViewer::setOverrideMode(const std::string& mode)
{
    if (mode == overrideMode) {
        return;
    }

    overrideMode = mode;

    auto document = getDocument();
    if (!document) {
        return;
    }

    auto views = document->getViewProvidersOfType(Gui::ViewProvider::getClassTypeId());
    if (mode == "No Shading") {
        this->shading = false;
        std::string flatLines = "Flat Lines";
        for (auto view : views) {
            view->setOverrideMode(flatLines);
        }
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
    else if (mode == "Hidden Line") {
        this->shading = true;
        std::string shaded = "Shaded";
        for (auto view : views) {
            view->setOverrideMode(shaded);
        }
        this->getSoRenderManager()->setRenderMode(SoRenderManager::HIDDEN_LINE);
    }
    else {
        this->shading = true;
        for (auto view : views) {
            view->setOverrideMode(mode);
        }
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
}

/// update override mode. doesn't affect providers
void View3DInventorViewer::updateOverrideMode(const std::string& mode)
{
    if (mode == overrideMode) {
        return;
    }

    overrideMode = mode;

    if (mode == "No Shading") {
        this->shading = false;
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
    else if (mode == "Hidden Line") {
        this->shading = true;
        this->getSoRenderManager()->setRenderMode(SoRenderManager::HIDDEN_LINE);
    }
    else {
        this->shading = true;
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
}

void View3DInventorViewer::setViewportCB(void* ud, SoAction* action)
{
    Q_UNUSED(ud)
    // Make sure to override the value set inside SoOffscreenRenderer::render()
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
        const SbViewportRegion& vp = renderer.getViewportRegion();
        SoViewportRegionElement::set(action->getState(), vp);
        static_cast<SoGLRenderAction*>(action)->setViewportRegion(vp);  // NOLINT
    }
}

void View3DInventorViewer::clearBufferCB(void* ud, SoAction* action)
{
    Q_UNUSED(ud)
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        // do stuff specific for GL rendering here.
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void View3DInventorViewer::setGLWidgetCB(void* userdata, SoAction* action)
{
    //FIXME: This causes the Coin error message:
    // Coin error in SoNode::GLRenderS(): GL error: 'GL_STACK_UNDERFLOW', nodetype:
    // Separator (set envvar COIN_GLERROR_DEBUGGING=1 and re-run to get more information)
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        auto gl = static_cast<QWidget*>(userdata);
        SoGLWidgetElement::set(action->getState(), qobject_cast<QOpenGLWidget*>(gl));
    }
}

void View3DInventorViewer::handleEventCB(void* userdata, SoEventCallback* n)
{
    auto that = static_cast<View3DInventorViewer*>(userdata);
    SoGLRenderAction* glra = that->getSoRenderManager()->getGLRenderAction();
    SoAction* action = n->getAction();
    SoGLRenderActionElement::set(action->getState(), glra);
    SoGLWidgetElement::set(action->getState(), qobject_cast<QOpenGLWidget*>(that->getGLWidget()));
}

void View3DInventorViewer::setGradientBackground(View3DInventorViewer::Background grad)
{
    switch (grad) {
    case Background::NoGradient:
        if (backgroundroot->findChild(pcBackGround) != -1) {
            backgroundroot->removeChild(pcBackGround);
        }
        break;
    case Background::LinearGradient:
        pcBackGround->setGradient(SoFCBackgroundGradient::LINEAR);
        if (backgroundroot->findChild(pcBackGround) == -1) {
            backgroundroot->addChild(pcBackGround);
        }
        break;
    case Background::RadialGradient:
        pcBackGround->setGradient(SoFCBackgroundGradient::RADIAL);
        if (backgroundroot->findChild(pcBackGround) == -1) {
            backgroundroot->addChild(pcBackGround);
        }
        break;
    }
}

View3DInventorViewer::Background View3DInventorViewer::getGradientBackground() const
{
    if (backgroundroot->findChild(pcBackGround) == -1) {
        return Background::NoGradient;
    }

    if (pcBackGround->getGradient() == SoFCBackgroundGradient::LINEAR) {
        return Background::LinearGradient;
    }

    return Background::RadialGradient;
}

void View3DInventorViewer::setGradientBackgroundColor(const SbColor& fromColor,
                                                      const SbColor& toColor)
{
    pcBackGround->setColorGradient(fromColor, toColor);
}

void View3DInventorViewer::setGradientBackgroundColor(const SbColor& fromColor,
                                                      const SbColor& toColor,
                                                      const SbColor& midColor)
{
    pcBackGround->setColorGradient(fromColor, toColor, midColor);
}

void View3DInventorViewer::setEnabledFPSCounter(bool on)
{
    fpsEnabled = on;
}

void View3DInventorViewer::setEnabledVBO(bool on)
{
    vboEnabled = on;
}

bool View3DInventorViewer::isEnabledVBO() const
{
    return vboEnabled;
}

void View3DInventorViewer::setRenderCache(int mode)
{
    static int canAutoCache = -1;

    if (mode < 0) {
        // Work around coin bug of unmatched call of
        // SoGLLazyElement::begin/endCaching() when on top rendering
        // transparent object with SORTED_OBJECT_SORTED_TRIANGLE_BLEND
        // transparency type.
        //
        // For more details see:
        // https://forum.freecad.org/viewtopic.php?f=18&t=43305&start=10#p412537
        coin_setenv("COIN_AUTO_CACHING", "0", TRUE);

        int setting = ViewParams::instance()->getRenderCache();
        if (mode == -2) {
            if (pcViewProviderRoot && setting != 1) {
                pcViewProviderRoot->renderCaching = SoSeparator::ON;
            }
            mode = 2;
        }
        else {
            if (pcViewProviderRoot) {
                pcViewProviderRoot->renderCaching = SoSeparator::AUTO;
            }
            mode = setting;
        }
    }

    if (canAutoCache < 0) {
        const char *env = coin_getenv("COIN_AUTO_CACHING");
        canAutoCache = env ? atoi(env) : 1;
    }

    // If coin auto cache is disabled, do not use 'Auto' render cache mode, but
    // fallback to 'Distributed' mode.
    if (!canAutoCache && mode != 2) {
        mode = 1;
    }

    auto caching = mode == 0 ? SoSeparator::AUTO :
                  (mode == 1 ? SoSeparator::ON :
                               SoSeparator::OFF);

    SoFCSeparator::setCacheMode(caching);
}

void View3DInventorViewer::setEnabledNaviCube(bool on)
{
    naviCubeEnabled = on;
}

bool View3DInventorViewer::isEnabledNaviCube() const
{
    return naviCubeEnabled;
}

void View3DInventorViewer::setNaviCubeCorner(int cc)
{
    if (naviCube) {
        naviCube->setCorner(static_cast<NaviCube::Corner>(cc));
    }
}

NaviCube* View3DInventorViewer::getNaviCube() const
{
    return naviCube;
}

void View3DInventorViewer::setAxisCross(bool on)
{
    SoNode* scene = getSceneGraph();
    auto sep = static_cast<SoSeparator*>(scene);  // NOLINT

    if (on) {
        if (!axisGroup) {
            axisCross = new Gui::SoShapeScale;
            auto axisKit = new Gui::SoAxisCrossKit();
            axisKit->set("xAxis.appearance.drawStyle", "lineWidth 2");
            axisKit->set("yAxis.appearance.drawStyle", "lineWidth 2");
            axisKit->set("zAxis.appearance.drawStyle", "lineWidth 2");
            axisCross->setPart("shape", axisKit);
            axisCross->scaleFactor = 1.0F;
            axisGroup = new SoSkipBoundingGroup;
            axisGroup->addChild(axisCross);

            sep->addChild(axisGroup);
        }
    }
    else {
        if (axisGroup) {
            sep->removeChild(axisGroup);
            axisGroup = nullptr;
        }
    }
}

bool View3DInventorViewer::hasAxisCross()
{
    return axisGroup;
}

void View3DInventorViewer::showRotationCenter(bool show)
{
    SoNode* scene = getSceneGraph();
    if (!scene) {
        return;
    }

    auto sep = static_cast<SoSeparator*>(scene);  // NOLINT

    bool showEnabled = App::GetApplication()
                           .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                           ->GetBool("ShowRotationCenter", true);

    if (show && showEnabled) {
        SbBool found{};
        SbVec3f center = navigation->getRotationCenter(found);

        if (!found) {
            return;
        }

        if (!rotationCenterGroup) {
            float size = App::GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                             ->GetFloat("RotationCenterSize", 5.0);  // NOLINT

            unsigned long rotationCenterColor =
                App::GetApplication()
                    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                    ->GetUnsigned("RotationCenterColor", 4278190131);  // NOLINT

            QColor color = Base::Color::fromPackedRGBA<QColor>(rotationCenterColor);

            rotationCenterGroup = new SoSkipBoundingGroup();

            auto sphere = new SoSphere();

            // There needs to be a non-transparent object to ensure the transparent sphere works when opening an new empty document
            auto hidden = new SoSeparator();
            auto hiddenScale = new SoScale();
            hiddenScale->scaleFactor = SbVec3f(0, 0, 0);
            hidden->addChild(hiddenScale);
            hidden->addChild(sphere);

            auto complexity = new SoComplexity();
            complexity->value = 1;

            auto material = new SoMaterial();
            material->emissiveColor = SbColor(float(color.redF()),
                                              float(color.greenF()),
                                              float(color.blueF()));
            material->transparency = 1.0F - float(color.alphaF());

            auto translation = new SoTranslation();
            translation->setName("translation");
            translation->translation.setValue(center);

            auto annotation = new SoAnnotation();
            annotation->addChild(complexity);
            annotation->addChild(material);
            annotation->addChild(sphere);

            auto scaledSphere = new SoShapeScale();
            scaledSphere->setPart("shape", annotation);
            scaledSphere->scaleFactor = size;

            rotationCenterGroup->addChild(translation);
            rotationCenterGroup->addChild(hidden);
            rotationCenterGroup->addChild(scaledSphere);

            sep->addChild(rotationCenterGroup);
        }
    }
    else {
        if (rotationCenterGroup) {
            sep->removeChild(rotationCenterGroup);
            rotationCenterGroup = nullptr;
        }
    }
}

// Changes the position of the rotation center indicator
void View3DInventorViewer::changeRotationCenterPosition(const SbVec3f& newCenter) {
    if (!rotationCenterGroup) {
        return;
    }

    SoTranslation* translation = dynamic_cast<SoTranslation*>(rotationCenterGroup->getByName("translation"));
    if (!translation) {
        return;
    }

    translation->translation = newCenter;
}

void View3DInventorViewer::setNavigationType(Base::Type type)
{
    if (this->navigation && this->navigation->getTypeId() == type) {
        return; // nothing to do
    }

    Base::Type navtype = Base::Type::getTypeIfDerivedFrom(type.getName(), NavigationStyle::getClassTypeId());
    auto ns = static_cast<NavigationStyle*>(navtype.createInstance());
    // createInstance could return a null pointer
    if (!ns) {
#if FC_DEBUG
        SoDebugError::postWarning("View3DInventorViewer::setNavigationType",
                                  "Navigation object must be of type NavigationStyle.");
#endif // FC_DEBUG
        return;
    }

    if (this->navigation) {
        ns->operator = (*this->navigation);
        delete this->navigation;
    }
    this->navigation = ns;
    this->navigation->setViewer(this);
}

NavigationStyle* View3DInventorViewer::navigationStyle() const
{
    return this->navigation;
}

SoDirectionalLight* View3DInventorViewer::getBacklight() const
{
    return this->backlight;
}

void View3DInventorViewer::setBacklightEnabled(bool on)
{
    this->backlight->on = on;
}

bool View3DInventorViewer::isBacklightEnabled() const
{
    return this->backlight->on.getValue();
}

SoDirectionalLight* View3DInventorViewer::getFillLight() const
{
    return this->fillLight;
}

void View3DInventorViewer::setFillLightEnabled(bool on)
{
    this->fillLight->on = on;
}

bool View3DInventorViewer::isFillLightEnabled() const
{
    return this->fillLight->on.getValue();
}
SoEnvironment* View3DInventorViewer::getEnvironment() const
{
    return this->environment;
}

void View3DInventorViewer::setSceneGraph(SoNode* root)
{
    inherited::setSceneGraph(root);
    if (!root) {
        _ViewProviderSet.clear();
        _ViewProviderMap.clear();
        editViewProvider = nullptr;
    }

    SoSearchAction sa;
    sa.setNode(this->backlight);
    //we want the rendered scene with all lights and cameras, viewer->getSceneGraph would return
    //the geometry scene only
    SoNode* scene = this->getSoRenderManager()->getSceneGraph();
    if (scene && scene->getTypeId().isDerivedFrom(SoSeparator::getClassTypeId())) {
        sa.apply(scene);
        if (!sa.getPath()) {
            static_cast<SoSeparator*>(scene)->insertChild(this->backlight, 0);
        }
    }

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    navigation->findBoundingSphere();
#endif
}

void View3DInventorViewer::savePicture(int width, int height, int sample, const QColor& bg, QImage& img) const
{
    // Save picture methods:
    // FramebufferObject -- viewer renders into FBO (no offscreen)
    // CoinOffscreenRenderer -- Coin's offscreen rendering method
    // Otherwise (Default) -- Qt's FBO used for offscreen rendering
    std::string saveMethod = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetASCII("SavePicture");

    bool useFramebufferObject = false;
    bool useGrabFramebuffer = false;
    bool useCoinOffscreenRenderer = false;
    if (saveMethod == "FramebufferObject") {
        useFramebufferObject = true;
    }
    else if (saveMethod == "GrabFramebuffer") {
        useGrabFramebuffer = true;
    }
    else if (saveMethod == "CoinOffscreenRenderer") {
        useCoinOffscreenRenderer = true;
    }

    if (useFramebufferObject) {
        auto self = const_cast<View3DInventorViewer*>(this);  // NOLINT
        self->imageFromFramebuffer(width, height, sample, bg, img);
        return;
    }

    if (useGrabFramebuffer) {
        auto self = const_cast<View3DInventorViewer*>(this);  // NOLINT
        img = self->grabFramebuffer();
        img = img.mirrored();
        img = img.scaledToWidth(width);
        return;
    }

    // if no valid color use the current background
    bool useBackground = false;
    SbViewportRegion vp(getSoRenderManager()->getViewportRegion());

    if (width > 0 && height > 0) {
        vp.setWindowSize(short(width), short(height));
    }

    //NOTE: To support pixels per inch we must use SbViewportRegion::setPixelsPerInch( ppi );
    //The default value is 72.0.
    //If we need to support grayscale images with must either use SoOffscreenRenderer::LUMINANCE or
    //SoOffscreenRenderer::LUMINANCE_TRANSPARENCY.

    SoCallback* cb = nullptr;

    // for an invalid color use the viewer's current background color
    QColor bgColor;
    if (!bg.isValid()) {
        if (backgroundroot->findChild(pcBackGround) == -1) {
            bgColor = this->backgroundColor();
        }
        else {
            useBackground = true;
            cb = new SoCallback;
            cb->setCallback(clearBufferCB);
        }
    }
    else {
        bgColor = bg;
    }

    auto root = new SoSeparator;
    root->ref();

    if (useCoinOffscreenRenderer) {
        auto cbvp = new SoCallback;
        cbvp->setCallback(setViewportCB);
        root->addChild(cbvp);
    }

    SoCamera* camera = getSoRenderManager()->getCamera();

    if (useBackground) {
        root->addChild(backgroundroot);
        root->addChild(cb);
    }

    if (!this->shading) {
        auto lm = new SoLightModel;
        lm->model = SoLightModel::BASE_COLOR;
        root->addChild(lm);
    }

    root->addChild(getHeadlight());
    root->addChild(camera);
    auto gl = new SoCallback;
    gl->setCallback(setGLWidgetCB, this->getGLWidget());
    root->addChild(gl);
    root->addChild(pcViewProviderRoot);
    root->addChild(foregroundroot);

    try {
        // render the scene
        if (!useCoinOffscreenRenderer) {
            SoQtOffscreenRenderer renderer(vp);
            renderer.setNumPasses(sample);
            renderer.setInternalTextureFormat(getInternalTextureFormat());
            if (bgColor.isValid()) {
                renderer.setBackgroundColor(SbColor4f(float(bgColor.redF()),
                                                      float(bgColor.greenF()),
                                                      float(bgColor.blueF()),
                                                      float(bgColor.alphaF())));
            }
            if (!renderer.render(root)) {
                throw Base::RuntimeError("Offscreen rendering failed");
            }

            renderer.writeToImage(img);
            root->unref();
        }
        else {
            SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
            renderer.setViewportRegion(vp);
            renderer.getGLRenderAction()->setSmoothing(true);
            renderer.getGLRenderAction()->setNumPasses(sample);
            renderer.getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
            if (bgColor.isValid()) {
                renderer.setBackgroundColor(SbColor(float(bgColor.redF()),
                                                    float(bgColor.greenF()),
                                                    float(bgColor.blueF())));
            }
            if (!renderer.render(root)) {
                throw Base::RuntimeError("Offscreen rendering failed");
            }

            renderer.writeToImage(img);
            root->unref();
        }

        if (!bgColor.isValid() || bgColor.alphaF() == 1.0) {
            QImage image(img.width(), img.height(), QImage::Format_RGB32);
            QPainter painter(&image);
            painter.fillRect(image.rect(), Qt::black);
            painter.drawImage(0, 0, img);
            painter.end();
            img = image;
        }
    }
    catch (...) {
        root->unref();
        throw; // re-throw exception
    }
}

void View3DInventorViewer::saveGraphic(int pagesize, const QColor& bgcolor, SoVectorizeAction* va) const
{
    if (bgcolor.isValid()) {
        va->setBackgroundColor(true, SbColor(float(bgcolor.redF()),
                                             float(bgcolor.greenF()),
                                             float(bgcolor.blueF())));
    }

    const float border = 10.0F;
    SbVec2s vpsize = this->getSoRenderManager()->getViewportRegion().getViewportSizePixels();
    float vpratio = ((float)vpsize[0]) / ((float)vpsize[1]);

    if (vpratio > 1.0F) {
        va->setOrientation(SoVectorizeAction::LANDSCAPE);
        vpratio = 1.0F / vpratio;
    }
    else {
        va->setOrientation(SoVectorizeAction::PORTRAIT);
    }

    va->beginStandardPage(SoVectorizeAction::PageSize(pagesize), border);

    // try to fill as much "paper" as possible
    SbVec2f size = va->getPageSize();

    float pageratio = size[0] / size[1];
    float xsize{};
    float ysize{};

    if (pageratio < vpratio) {
        xsize = size[0];
        ysize = xsize / vpratio;
    }
    else {
        ysize = size[1];
        xsize = ysize * vpratio;
    }

    float offx = border + (size[0]-xsize) * 0.5F;  // NOLINT
    float offy = border + (size[1]-ysize) * 0.5F;  // NOLINT

    va->beginViewport(SbVec2f(offx, offy), SbVec2f(xsize, ysize));
    va->calibrate(this->getSoRenderManager()->getViewportRegion());

    va->apply(this->getSoRenderManager()->getSceneGraph());

    va->endViewport();
    va->endPage();
}

void View3DInventorViewer::startSelection(View3DInventorViewer::SelectionMode mode)
{
    navigation->startSelection(NavigationStyle::SelectionMode(mode));
}

void View3DInventorViewer::abortSelection()
{
    setCursorEnabled(true);
    navigation->abortSelection();
}

void View3DInventorViewer::stopSelection()
{
    setCursorEnabled(true);
    navigation->stopSelection();
}

bool View3DInventorViewer::isSelecting() const
{
    return navigation->isSelecting();
}

const std::vector<SbVec2s>& View3DInventorViewer::getPolygon(SelectionRole* role) const
{
    return navigation->getPolygon(role);
}

void View3DInventorViewer::setSelectionEnabled(bool enable)
{
    this->selectionRoot->selectionEnabled.setValue(enable);  // NOLINT
}

bool View3DInventorViewer::isSelectionEnabled() const
{
    return this->selectionRoot->selectionEnabled.getValue();  // NOLINT
}

SbVec2f View3DInventorViewer::screenCoordsOfPath(SoPath* path) const
{
    // Generate a matrix (well, a SoGetMatrixAction) that
    // moves us to the picked object's coordinate space.
    SoGetMatrixAction gma(getSoRenderManager()->getViewportRegion());
    gma.apply(path);

    // Use that matrix to translate the origin in the picked
    // object's coordinate space into object space
    SbVec3f imageCoords(0, 0, 0);
    SbMatrix mat = gma.getMatrix().transpose();
    mat.multMatrixVec(imageCoords, imageCoords);

    // Now, project the object space coordinates of the object
    // into "normalized" screen coordinates.
    SbViewVolume  vol = getSoRenderManager()->getCamera()->getViewVolume();
    vol.projectToScreen(imageCoords, imageCoords);

    // Translate "normalized" screen coordinates to pixel coords.
    //
    // Note: for some reason, projectToScreen() doesn't seem to
    // handle non-square viewports properly.  The X and Y are
    // scaled such that [0,1] fits within the smaller of the window
    // width or height.  For instance, in a window that's 400px
    // tall and 800px wide, the Y will be within [0,1], but X can
    // vary within [-0.5,1.5]...
    int width = getGLWidget()->width();
    int height = getGLWidget()->height();

    if (width >= height) {
        // "Landscape" orientation, to square
        imageCoords[0] *= height;
        imageCoords[0] += (width-height) / 2.0;  // NOLINT
        imageCoords[1] *= height;

    }
    else {
        // "Portrait" orientation
        imageCoords[0] *= width;
        imageCoords[1] *= width;
        imageCoords[1] += (height-width) / 2.0;  // NOLINT
    }

    return {imageCoords[0], imageCoords[1]};
}

std::vector<SbVec2f> View3DInventorViewer::getGLPolygon(const std::vector<SbVec2s>& pnts) const
{
    const SbViewportRegion &vp = this->getSoRenderManager()->getViewportRegion();
    const SbVec2s &sp = vp.getViewportSizePixels();
    const SbVec2s &op = vp.getViewportOriginPixels();
    const SbVec2f &vpSize = vp.getViewportSize();
    float dX{};
    float dY{};
    vpSize.getValue(dX, dY);
    float fRatio = vp.getViewportAspectRatio();

    std::vector<SbVec2f> poly;
    for (const auto & pnt : pnts) {
        SbVec2s loc = pnt - op;
        SbVec2f pos((float)loc[0] / (float)sp[0], (float)loc[1] / (float)sp[1]);
        float pX{};
        float pY{};
        pos.getValue(pX, pY);

        // now calculate the real points respecting aspect ratio information
        //
        if (fRatio > 1.0F) {
            pX = (pX - 0.5F * dX) * fRatio + 0.5F * dX;  // NOLINT
            pos.setValue(pX, pY);
        }
        else if (fRatio < 1.0F) {
            pY = (pY - 0.5F * dY) / fRatio + 0.5F * dY;  // NOLINT
            pos.setValue(pX, pY);
        }

        poly.push_back(pos);
    }

    return poly;
}

std::vector<SbVec2f> View3DInventorViewer::getGLPolygon(SelectionRole* role) const
{
    const std::vector<SbVec2s>& pnts = navigation->getPolygon(role);
    return getGLPolygon(pnts);
}

bool View3DInventorViewer::dumpToFile(SoNode* node, const char* filename, bool binary) const
{
    bool ret = false;
    Base::FileInfo fi(filename);

    if (fi.hasExtension({"idtf", "svg"})) {
        int ps = 4;
        QColor col = Qt::white;
        std::unique_ptr<SoVectorizeAction> vo;

        if (fi.hasExtension("svg")) {
            vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeSVGAction());
        }
        else if (fi.hasExtension("idtf")) {
            vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeU3DAction());
        }
        else if (fi.hasExtension({"ps", "eps"})) {
            vo = std::unique_ptr<SoVectorizeAction>(new SoVectorizePSAction());
        }
        else {
            throw Base::ValueError("Not supported vector graphic");
        }

        SoVectorOutput* out = vo->getOutput();
        if (!out || !out->openFile(filename)) {
            std::ostringstream a_out;
            a_out << "Cannot open file '" << filename << "'";
            throw Base::FileSystemError(a_out.str());
        }

        saveGraphic(ps, col, vo.get());
        out->closeFile();
    }
    else {
        // Try VRML and Inventor format
        ret = SoFCDB::writeToFile(node, filename, binary);
    }

    return ret;
}

/**
 * Sets the SoFCInteractiveElement to \a true.
 */
void View3DInventorViewer::interactionStartCB(void* ud, SoQTQuarterAdaptor* viewer)
{
    Q_UNUSED(ud)
    SoGLRenderAction* glra = viewer->getSoRenderManager()->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), true);
}

/**
 * Sets the SoFCInteractiveElement to \a false and forces a redraw.
 */
void View3DInventorViewer::interactionFinishCB(void* ud, SoQTQuarterAdaptor* viewer)
{
    Q_UNUSED(ud)
    SoGLRenderAction* glra = viewer->getSoRenderManager()->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), false);
    viewer->redraw();
}

/**
 * Logs the type of the action that traverses the Inventor tree.
 */
void View3DInventorViewer::interactionLoggerCB(void* ud, SoAction* action)
{
    Q_UNUSED(ud)
    Base::Console().log("%s\n", action->getTypeId().getName().getString());
}

void View3DInventorViewer::addGraphicsItem(GLGraphicsItem* item)
{
    this->graphicsItems.push_back(item);
}

void View3DInventorViewer::removeGraphicsItem(GLGraphicsItem* item)
{
    this->graphicsItems.remove(item);
}

std::list<GLGraphicsItem*> View3DInventorViewer::getGraphicsItems() const
{
    return graphicsItems;
}

std::list<GLGraphicsItem*> View3DInventorViewer::getGraphicsItemsOfType(const Base::Type& type) const
{
    std::list<GLGraphicsItem*> items;
    for (auto it : this->graphicsItems) {
        if (it->isDerivedFrom(type)) {
            items.push_back(it);
        }
    }

    return items;
}

void View3DInventorViewer::clearGraphicsItems()
{
    this->graphicsItems.clear();
}

int View3DInventorViewer::getNumSamples()
{
    Gui::AntiAliasing msaa = Multisample::readMSAAFromSettings();
    return Multisample::toSamples(msaa);
}

GLenum View3DInventorViewer::getInternalTextureFormat()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    std::string format = hGrp->GetASCII("InternalTextureFormat", "Default");

    // NOLINTBEGIN
    if (format == "GL_RGB") {
        return GL_RGB;
    }
    else if (format == "GL_RGBA") {
        return GL_RGBA;
    }
    else if (format == "GL_RGB8") {
        return GL_RGB8;
    }
    else if (format == "GL_RGBA8") {
        return GL_RGBA8;
    }
    else if (format == "GL_RGB10") {
        return GL_RGB10;
    }
    else if (format == "GL_RGB10_A2") {
        return GL_RGB10_A2;
    }
    else if (format == "GL_RGB16") {
        return GL_RGB16;
    }
    else if (format == "GL_RGBA16") {
        return GL_RGBA16;
    }
    else if (format == "GL_RGB32F") {
        return GL_RGB32F_ARB;
    }
    else if (format == "GL_RGBA32F") {
        return GL_RGBA32F_ARB;
    }
    else {
        QOpenGLFramebufferObjectFormat fboFormat;
        return fboFormat.internalTextureFormat();
    }
    // NOLINTEND
}

void View3DInventorViewer::setRenderType(RenderType type)
{
    renderType = type;

    glImage = QImage();
    if (type != Framebuffer) {
        delete framebuffer;
        framebuffer = nullptr;
    }

    switch (type) {
    case Native:
        break;
    case Framebuffer:
        if (!framebuffer) {
            const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
            SbVec2s size = vp.getViewportSizePixels();
            int width = size[0];
            int height = size[1];

            auto gl = static_cast<QOpenGLWidget*>(this->viewport());  // NOLINT
            gl->makeCurrent();
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples(getNumSamples());
            fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
            auto fbo = new QOpenGLFramebufferObject(width, height, fboFormat);
            if (fbo->format().samples() > 0) {
                renderToFramebuffer(fbo);
                framebuffer = new QOpenGLFramebufferObject(fbo->size());
                // this is needed to be able to render the texture later
                QOpenGLFramebufferObject::blitFramebuffer(framebuffer, fbo);
                delete fbo;
            }
            else {
                renderToFramebuffer(fbo);
                framebuffer = fbo;
            }
        }
        break;
    case Image:
        {
            glImage = grabFramebuffer();
        }
        break;
    }
}

View3DInventorViewer::RenderType View3DInventorViewer::getRenderType() const
{
    return this->renderType;
}

QImage View3DInventorViewer::grabFramebuffer()
{
    auto gl = static_cast<QOpenGLWidget*>(this->viewport());  // NOLINT
    gl->makeCurrent();

    QImage res;
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();
    int width = size[0];
    int height = size[1];

    int samples = getNumSamples();
    if (samples == 0) {
        // if anti-aliasing is off we can directly use glReadPixels
        QImage img(QSize(width, height), QImage::Format_RGB32);
        glReadPixels(0, 0, width, height, GL_BGRA, GL_UNSIGNED_BYTE, img.bits());
        res = img;
    }
    else {
        QOpenGLFramebufferObjectFormat fboFormat;
        fboFormat.setSamples(getNumSamples());
        fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
        fboFormat.setTextureTarget(GL_TEXTURE_2D);
        fboFormat.setInternalTextureFormat(getInternalTextureFormat());

        QOpenGLFramebufferObject fbo(width, height, fboFormat);
        renderToFramebuffer(&fbo);

        res = fbo.toImage(false);

        QImage image(res.width(), res.height(), QImage::Format_RGB32);
        QPainter painter(&image);
        painter.fillRect(image.rect(),Qt::black);
        painter.drawImage(0, 0, res);
        painter.end();
        res = image;
    }

    return res;
}

void View3DInventorViewer::imageFromFramebuffer(int width, int height, int samples,
                                                const QColor& bgcolor, QImage& img)
{
    auto gl = static_cast<QOpenGLWidget*>(this->viewport());  // NOLINT
    gl->makeCurrent();

    const QOpenGLContext* context = QOpenGLContext::currentContext();
    if (!context) {
        Base::Console().warning("imageFromFramebuffer failed because no context is active\n");
        return;
    }

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setSamples(samples);
    fboFormat.setAttachment(QOpenGLFramebufferObject::Depth);
    // With enabled alpha a transparent background is supported but
    // at the same time breaks semi-transparent models. A workaround
    // is to use a certain background color using GL_RGB as texture
    // format and in the output image search for the above color and
    // replaces it with the color requested by the user.
    fboFormat.setInternalTextureFormat(getInternalTextureFormat());

    QOpenGLFramebufferObject fbo(width, height, fboFormat);

    const QColor col = backgroundColor();
    auto grad = getGradientBackground();

    constexpr const int maxAlpha = 255;
    int alpha = maxAlpha;
    QColor bgopaque = bgcolor;
    if (bgopaque.isValid()) {
        // force an opaque background color
        alpha = bgopaque.alpha();
        if (alpha < maxAlpha) {
            bgopaque.setRgb(maxAlpha, maxAlpha, maxAlpha);
        }
        setBackgroundColor(bgopaque);
        setGradientBackground(Background::NoGradient);
    }

    renderToFramebuffer(&fbo);
    setBackgroundColor(col);
    setGradientBackground(grad);
    img = fbo.toImage();

    // if background color isn't opaque manipulate the image
    if (alpha < maxAlpha) {
        QImage image(img.constBits(), img.width(), img.height(), QImage::Format_ARGB32);
        img = image.copy();
        QRgb rgba = bgcolor.rgba();
        QRgb rgb = bgopaque.rgb();
        QRgb * bits = (QRgb*) img.bits();
        for (int yy = 0; yy < height; yy++) {
            for (int xx = 0; xx < width; xx++) {
                if (*bits == rgb) {
                    *bits = rgba;
                }
                bits++;  // NOLINT
            }
        }
    }
    else if (alpha == maxAlpha) {
        QImage image(img.width(), img.height(), QImage::Format_RGB32);
        QPainter painter(&image);
        painter.fillRect(image.rect(),Qt::black);
        painter.drawImage(0, 0, img);
        painter.end();
        img = image;
    }
}

void View3DInventorViewer::renderToFramebuffer(QOpenGLFramebufferObject* fbo)
{
    static_cast<QOpenGLWidget*>(this->viewport())->makeCurrent();  // NOLINT
    fbo->bind();
    int width = fbo->size().width();
    int height = fbo->size().height();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    const QColor col = this->backgroundColor();
    glViewport(0, 0, width, height);
    glClearColor(float(col.redF()), float(col.greenF()), float(col.blueF()), float(col.alphaF()));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    SoBoxSelectionRenderAction gl(SbViewportRegion(width, height));
    // When creating a new GL render action we have to copy over the cache context id
    // For further details see init().
    uint32_t id = this->getSoRenderManager()->getGLRenderAction()->getCacheContext();
    gl.setCacheContext(id);
    gl.setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);

    if (!this->shading) {
        SoLightModelElement::set(gl.getState(), selectionRoot, SoLightModelElement::BASE_COLOR);
        SoOverrideElement::setLightModelOverride(gl.getState(), selectionRoot, true);
    }

    gl.apply(this->backgroundroot);
    // The render action of the render manager has set the depth function to GL_LESS
    // while creating a new render action has it set to GL_LEQUAL. So, in order to get
    // the exact same result set it explicitly to GL_LESS.
    glDepthFunc(GL_LESS);
    gl.apply(this->getSoRenderManager()->getSceneGraph());
    gl.apply(this->foregroundroot);

    if (this->axiscrossEnabled) {
        this->drawAxisCross();
    }

    fbo->release();
}

void View3DInventorViewer::actualRedraw()
{
    switch (renderType) {
    case Native:
        renderScene();
        break;
    case Framebuffer:
        renderFramebuffer();
        break;
    case Image:
        renderGLImage();
        break;
    }
}

void View3DInventorViewer::renderFramebuffer()
{
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glViewport(0, 0, size[0], size[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glDisable(GL_DEPTH_TEST);

    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, this->framebuffer->texture());
    glColor3f(1.0, 1.0, 1.0);

    glBegin(GL_QUADS);
        glTexCoord2f(0.0F, 0.0F);
        glVertex2f(-1.0, -1.0F);
        glTexCoord2f(1.0F, 0.0F);
        glVertex2f(1.0F, -1.0F);
        glTexCoord2f(1.0F, 1.0F);
        glVertex2f(1.0F, 1.0F);
        glTexCoord2f(0.0F, 1.0F);
        glVertex2f(-1.0F, 1.0F);
    glEnd();

    printDimension();

    for (auto it : this->graphicsItems) {
        it->paintGL();
    }

    if (naviCubeEnabled) {
        naviCube->drawNaviCube();
    }

    glPopAttrib();
}

void View3DInventorViewer::renderGLImage()
{
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    glDisable(GL_LIGHTING);
    glViewport(0, 0, size[0], size[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size[0], 0, size[1], 0, 100);  // NOLINT
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    glRasterPos2f(0,0);
    glDrawPixels(glImage.width(), glImage.height(), GL_BGRA,GL_UNSIGNED_BYTE, glImage.bits());

    printDimension();

    for (auto it : this->graphicsItems) {
        it->paintGL();
    }

    if (naviCubeEnabled) {
        naviCube->drawNaviCube();
    }

    glPopAttrib();
}

// #define ENABLE_GL_DEPTH_RANGE
// The calls of glDepthRange inside renderScene() causes problems with transparent objects
// so that's why it is disabled now: https://forum.freecad.org/viewtopic.php?f=3&t=6037&hilit=transparency

// Documented in superclass. Overrides this method to be able to draw
// the axis cross, if selected, and to keep a continuous animation
// upon spin.
void View3DInventorViewer::renderScene()
{
    ZoneScoped;

    // Must set up the OpenGL viewport manually, as upon resize
    // operations, Coin won't set it up until the SoGLRenderAction is
    // applied again. And since we need to do glClear() before applying
    // the action..
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s origin = vp.getViewportOriginPixels();
    SbVec2s size = vp.getViewportSizePixels();
    glViewport(origin[0], origin[1], size[0], size[1]);

    const QColor col = this->backgroundColor();
    glClearColor(float(col.redF()), float(col.greenF()), float(col.blueF()), 0.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

#if defined(ENABLE_GL_DEPTH_RANGE)
    // using 90% of the z-buffer for the background and the main node
    glDepthRange(0.1,1.0);
#endif

    SoGLRenderAction* glra = this->getSoRenderManager()->getGLRenderAction();
    SoState* state = glra->getState();

    // Render our scenegraph with the image.
    {
        ZoneScopedN("Background");
        SoDevicePixelRatioElement::set(state, devicePixelRatio());
        SoGLWidgetElement::set(state, qobject_cast<QOpenGLWidget*>(this->getGLWidget()));
        SoGLRenderActionElement::set(state, glra);
        SoGLVBOActivatedElement::set(state, this->vboEnabled);
        drawSingleBackground(col);
        glra->apply(this->backgroundroot);
    }

    if (!this->shading) {
        state->push();
        SoLightModelElement::set(state, selectionRoot, SoLightModelElement::BASE_COLOR);
        SoOverrideElement::setLightModelOverride(state, selectionRoot, true);
    }

    try {
        // Render normal scenegraph.
        inherited::actualRedraw();

        So3DAnnotation::render = true;
        glClear(GL_DEPTH_BUFFER_BIT);
        glra->apply(SoDelayedAnnotationsElement::getDelayedPaths(state));
        So3DAnnotation::render = false;
    }
    catch (const Base::MemoryException&) {
        // FIXME: If this exception appears then the background and camera position get broken somehow. (Werner 2006-02-01)
        for (auto it : _ViewProviderSet) {
            it->hide();
        }

        inherited::actualRedraw();
        QMessageBox::warning(parentWidget(), QObject::tr("Out of memory"),
                             QObject::tr("Not enough memory available to display the data."));
    }

    if (!this->shading) {
        state->pop();
    }

#if defined (ENABLE_GL_DEPTH_RANGE)
    // using 10% of the z-buffer for the foreground node
    glDepthRange(0.0,0.1);
#endif

    // Render overlay front scenegraph.
    {
        ZoneScopedN("Foreground");
        glra->apply(this->foregroundroot);
    }

    if (this->axiscrossEnabled) {
        this->drawAxisCross();
    }

#if defined (ENABLE_GL_DEPTH_RANGE)
    // using the main portion of z-buffer again (for frontbuffer highlighting)
    glDepthRange(0.1,1.0);
#endif

    // Immediately reschedule to get continuous animation.
    if (this->isAnimating()) {
        this->getSoRenderManager()->scheduleRedraw();
    }

    printDimension();

    {
        ZoneScopedN("Graphics items");
        for (auto it : this->graphicsItems) {
            it->paintGL();
        }
    }

    //fps rendering
    if (fpsEnabled) {
        std::stringstream stream;
        stream.precision(1);
        stream.setf(std::ios::fixed | std::ios::showpoint);
        stream << framesPerSecond[0] << " ms / " << framesPerSecond[1] << " fps";
        ParameterGrp::handle hGrpOverlayL = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/MainWindow/DockWindows/OverlayLeft");
        std::string overlayLeftWidgets = hGrpOverlayL->GetASCII("Widgets", "");
        ParameterGrp::handle hGrpView = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/View");
        unsigned long axisLetterColor =
            hGrpView->GetUnsigned("AxisLetterColor", 4294902015);  // default FPS color (yellow)
        draw2DString(stream.str().c_str(),
                     SbVec2s(10, 10),
                     SbVec2f((overlayLeftWidgets.empty() ? 0.1f : 1.1f), 0.1f),
                     Base::Color(static_cast<uint32_t>(axisLetterColor)));  // NOLINT
    }

    if (naviCubeEnabled) {
        naviCube->drawNaviCube();
    }

    // Workaround for inconsistent QT behavior related to handling custom OpenGL widgets that
    // leave non opaque alpha values in final output.
    // On wayland that can cause window to become transparent or blurry trail effect in the
    // parts that contain partially transparent objects.
    //
    // At the end of rendering clear alpha value, so that it doesn't matter how rest of the
    // compositing stack at QT and desktop level would interpret transparent pixels.
    //
    // Related issues:
    // https://bugreports.qt.io/browse/QTBUG-110014
    // https://bugreports.qt.io/browse/QTBUG-132197
    // https://bugreports.qt.io/browse/QTBUG-119214
    // https://github.com/FreeCAD/FreeCAD/issues/8341
    // https://github.com/FreeCAD/FreeCAD/issues/6177
    glPushAttrib(GL_COLOR_BUFFER_BIT);
    glColorMask(false, false, false, true);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glPopAttrib();
}

void View3DInventorViewer::setSeekMode(bool on)
{
    // Overrides this method to make sure any animations are stopped
    // before we go into seek mode.

    // Note: this method is almost identical to the setSeekMode() in the
    // SoQtFlyViewer and SoQtPlaneViewer, so migrate any changes.

    if (this->isAnimating()) {
        this->stopAnimating();
    }

    inherited::setSeekMode(on);
    navigation->setViewingMode(on ? NavigationStyle::SEEK_WAIT_MODE :
                               (this->isViewing() ?
                                NavigationStyle::IDLE : NavigationStyle::INTERACT));
}

SbVec3f View3DInventorViewer::getCenterPointOnFocalPlane() const {
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) {
        return {0. ,0. ,0. };
    }

    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    return cam->position.getValue() + cam->focalDistance.getValue() * direction;
}

float View3DInventorViewer::getMaxDimension() const {
    float fHeight = -1.0;
    float fWidth = -1.0;
    getDimensions(fHeight, fWidth);
    return std::max(fHeight, fWidth);
}

void View3DInventorViewer::getDimensions(float& fHeight, float& fWidth) const
{
    SoCamera* camera = getSoRenderManager()->getCamera();
    if (!camera) {
        // no camera there
        return;
    }

    float aspectRatio = getViewportRegion().getViewportAspectRatio();

    SoType type = camera->getTypeId();
    if (type.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {
        // NOLINTBEGIN
        fHeight = static_cast<SoOrthographicCamera*>(camera)->height.getValue();
        fWidth = fHeight;
        // NOLINTEND
    }
    else if (type.isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) {
        // NOLINTBEGIN
        float fHeightAngle = static_cast<SoPerspectiveCamera*>(camera)->heightAngle.getValue();
        fHeight = std::tan(fHeightAngle / 2.0) * 2.0 * camera->focalDistance.getValue();
        fWidth = fHeight;
        // NOLINTEND
    }

    if (aspectRatio > 1.0) {
        fWidth *= aspectRatio;
    }
    else {
        fHeight *= aspectRatio;
    }
}

void View3DInventorViewer::printDimension() const
{
    float fHeight = -1.0;
    float fWidth = -1.0;
    getDimensions(fHeight, fWidth);

    std::string dim;

    if (fWidth >= 0.0 && fHeight >= 0.0) {
        // Translate screen units into user's unit schema
        Base::Quantity qWidth(Base::Quantity::MilliMetre);
        Base::Quantity qHeight(Base::Quantity::MilliMetre);
        qWidth.setValue(fWidth);
        qHeight.setValue(fHeight);
        auto wStr = Base::UnitsApi::schemaTranslate(qWidth);
        auto hStr = Base::UnitsApi::schemaTranslate(qHeight);

        // Create final string and update window
        dim = fmt::format("{} x {}", wStr, hStr);
    }

    getMainWindow()->setPaneText(2, QString::fromStdString(dim));
}

void View3DInventorViewer::selectAll()
{
    std::vector<App::DocumentObject*> objs;

    for (auto it : _ViewProviderSet) {
        if (it->isDerivedFrom<ViewProviderDocumentObject>()) {
            auto vp = static_cast<ViewProviderDocumentObject*>(it);  // NOLINT
            App::DocumentObject* obj = vp->getObject();

            if (obj) {
                objs.push_back(obj);
            }
        }
    }

    if (!objs.empty()) {
        Gui::Selection().setSelection(objs.front()->getDocument()->getName(), objs);
    }
}

bool View3DInventorViewer::processSoEvent(const SoEvent* ev)
{
    ZoneScoped;

    if (naviCubeEnabled && naviCube->processSoEvent(ev)) {
        return true;
    }
    if (isRedirectedToSceneGraph()) {
        bool processed = inherited::processSoEvent(ev);

        if (!processed) {
            processed = navigation->processEvent(ev);
        }

        return processed;
    }

    if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        // filter out 'Q' and 'ESC' keys
        const auto ke = static_cast<const SoKeyboardEvent*>(ev);  // NOLINT

        switch (ke->getKey()) {
        case SoKeyboardEvent::ESCAPE:
        case SoKeyboardEvent::Q: // ignore 'Q' keys (to prevent app from being closed)
            return inherited::processSoEvent(ev);
        default:
            break;
        }
    }

    return navigation->processEvent(ev);
}

bool View3DInventorViewer::processSoEventBase(const SoEvent* const ev)
{
    return inherited::processSoEvent(ev);
}

SbVec3f View3DInventorViewer::getViewDirection() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) {
        // this is the default
        return {0,0,-1};
    }

    SbVec3f projDir = cam->getViewVolume().getProjectionDirection();
    return projDir;
}

void View3DInventorViewer::setViewDirection(SbVec3f dir)
{
    if (SoCamera* cam = this->getSoRenderManager()->getCamera()) {
        cam->orientation.setValue(SbRotation(SbVec3f(0, 0, -1), dir));
    }
}

SbVec3f View3DInventorViewer::getUpDirection() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) {
        return {0,1,0};
    }

    SbRotation camrot = cam->orientation.getValue();
    SbVec3f upvec(0, 1, 0); // init to default up vector
    camrot.multVec(upvec, upvec);
    return upvec;
}

SbRotation View3DInventorViewer::getCameraOrientation() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) {
        // this is the default
        return {0,0,0,1};
    }

    return cam->orientation.getValue();
}

SbVec2f View3DInventorViewer::getNormalizedPosition(const SbVec2s& pnt) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();

    short xpos{};
    short ypos{};
    pnt.getValue(xpos, ypos);
    SbVec2f siz = vp.getViewportSize();
    float dX{};
    float dY{};
    siz.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = float(xpos) / float(vp.getViewportSizePixels()[0]);
    float pY = float(ypos) / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    // NOLINTBEGIN
    if (fRatio > 1.0F) {
        pX = (pX - 0.5F * dX) * fRatio + 0.5F * dX;
    }
    else if (fRatio < 1.0F) {
        pY = (pY - 0.5F * dY) / fRatio + 0.5F * dY;
    }
    // NOLINTEND

    return {pX, pY};
}

Base::BoundBox2d View3DInventorViewer::getViewportOnXYPlaneOfPlacement(Base::Placement plc) const
{
    auto projBBox = Base::BoundBox3d();
    projBBox.SetVoid();

    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) {
        // Return empty box.
        return Base::BoundBox2d(0, 0, 0, 0);
    }

    Base::Vector3d pos = plc.getPosition();
    Base::Rotation rot = plc.getRotation();

    // Transform the plane LCS into the global one.
    Base::Vector3d zAxis(0, 0, 1);
    rot.multVec(zAxis, zAxis);

    // Get the position and convert Base::Vector3d to SbVec3f
    SbVec3f planeNormal(zAxis.x, zAxis.y, zAxis.z);
    SbVec3f planePosition(pos.x, pos.y, pos.z);
    SbPlane xyPlane(planeNormal, planePosition);

    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();
    SbViewVolume vol = pCam->getViewVolume();

    float fRatio = vp.getViewportAspectRatio();
    float dX, dY;
    vp.getViewportSize().getValue(dX, dY);

    // Projects a pair of normalized coordinates on the XY plane.
    auto projectPoint =
            [&](float x, float y) {
                if (fRatio > 1.f) {
                    x = (x - 0.5f * dX) * fRatio + 0.5f * dX;
                }
                else if (fRatio < 1.f) {
                    y = (y - 0.5f * dY) / fRatio + 0.5f * dY;
                }

                SbLine line;
                vol.projectPointToLine(SbVec2f(x, y), line);

                SbVec3f pt;
                // Intersection point on the XY plane.
                if (!xyPlane.intersect(line, pt)) {
                    return;
                }

                projBBox.Add(Base::convertTo<Base::Vector3d>(pt));
            };

    // Project the four corners of the viewport plane.
    projectPoint(0.f, 0.f);
    projectPoint(1.f, 0.f);
    projectPoint(0.f, 1.f);
    projectPoint(1.f, 1.f);

    if (!projBBox.IsValid()) {
        // Return empty box.
        return Base::BoundBox2d(0, 0, 0, 0);
    }

    plc.invert();
    Base::ViewOrthoProjMatrix proj(plc.toMatrix());
    return projBBox.ProjectBox(&proj);
}

SbVec3f View3DInventorViewer::getPointOnXYPlaneOfPlacement(const SbVec2s& pnt, const Base::Placement& plc) const
{
    SbVec2f pnt2d = getNormalizedPosition(pnt);
    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) {
        throw Base::RuntimeError("No camera node found");
    }

    SbViewVolume  vol = pCam->getViewVolume();
    SbLine line;
    vol.projectPointToLine(pnt2d, line);

    // Calculate the plane using plc
    Base::Rotation rot = plc.getRotation();
    Base::Vector3d normalVector = rot.multVec(Base::Vector3d(0, 0, 1));
    SbVec3f planeNormal = Base::convertTo<SbVec3f>(normalVector);

    // Get the position and convert Base::Vector3d to SbVec3f
    Base::Vector3d pos = plc.getPosition();
    SbVec3f planePosition = Base::convertTo<SbVec3f>(pos);
    SbPlane xyPlane(planeNormal, planePosition);

    SbVec3f pt;
    if (xyPlane.intersect(line, pt)) {
        return pt; // Intersection point on the XY plane
    }

    throw Base::RuntimeError("No intersection found");
}

SbVec3f projectPointOntoPlane(const SbVec3f& point, const SbPlane& plane) {
    SbVec3f planeNormal = plane.getNormal();
    float d = plane.getDistanceFromOrigin();
    float distance = planeNormal.dot(point) + d;
    return point - planeNormal * distance;
}

// Project a line onto a plane
SbLine projectLineOntoPlane(const SbVec3f& p1, const SbVec3f& p2, const SbPlane& plane) {
    SbVec3f projectedPoint1 = projectPointOntoPlane(p1, plane);
    SbVec3f projectedPoint2 = projectPointOntoPlane(p2, plane);
    return SbLine(projectedPoint1, projectedPoint2);
}

SbVec3f intersection(const SbVec3f& p11, const SbVec3f& p12, const SbVec3f& p21, const SbVec3f& p22)
{
    SbVec3f da = p12 - p11;
    SbVec3f db = p22 - p21;
    SbVec3f dc = p21 - p11;

    double s = (dc.cross(db)).dot(da.cross(db)) / da.cross(db).sqrLength();
    return p11 + da * s;
}

SbVec3f View3DInventorViewer::getPointOnLine(const SbVec2s& pnt, const SbVec3f& axisCenter, const SbVec3f& axis) const
{
    SbVec2f pnt2d = getNormalizedPosition(pnt);
    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) {
        // return invalid point
        return {};
    }

    // First we get pnt projection on the focal plane
    SbViewVolume  vol = pCam->getViewVolume();

    float nearDist = pCam->nearDistance.getValue();
    float farDist = pCam->farDistance.getValue();
    float focalDist = pCam->focalDistance.getValue();

    if (focalDist < nearDist || focalDist > farDist) {
        focalDist = 0.5F * (nearDist + farDist);  // NOLINT
    }

    SbLine line;
    SbVec3f pt, ptOnFocalPlaneAndOnLine, ptOnFocalPlane;
    SbPlane focalPlane = vol.getPlane(focalDist);
    vol.projectPointToLine(pnt2d, line);
    focalPlane.intersect(line, ptOnFocalPlane);

    // Check if line is orthogonal to the focal plane
    SbVec3f focalPlaneNormal = focalPlane.getNormal();
    float dotProduct = fabs(axis.dot(focalPlaneNormal));
    if (dotProduct > (1.0 - 1e-6)) {
        return ptOnFocalPlane;
    }

    SbLine projectedLine = projectLineOntoPlane(axisCenter, axisCenter + axis, focalPlane);
    ptOnFocalPlaneAndOnLine = projectedLine.getClosestPoint(ptOnFocalPlane);

    // now we need the intersection point between
    // - the line passing by ptOnFocalPlaneAndOnLine normal to focalPlane
    // - The line (axisCenter, axisCenter + axis)

    // Line normal to focal plane through ptOnFocalPlaneAndOnLine
    SbLine normalLine(ptOnFocalPlaneAndOnLine, ptOnFocalPlaneAndOnLine + focalPlaneNormal);
    SbLine axisLine(axisCenter, axisCenter + axis);
    pt = intersection(ptOnFocalPlaneAndOnLine, ptOnFocalPlaneAndOnLine + focalPlaneNormal, axisCenter, axisCenter + axis);

    return pt;
}

SbVec3f View3DInventorViewer::getPointOnFocalPlane(const SbVec2s& pnt) const
{
    SbVec2f pnt2d = getNormalizedPosition(pnt);
    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) {
        // return invalid point
        return {};
    }

    SbViewVolume  vol = pCam->getViewVolume();

    float nearDist = pCam->nearDistance.getValue();
    float farDist = pCam->farDistance.getValue();
    float focalDist = pCam->focalDistance.getValue();

    if (focalDist < nearDist || focalDist > farDist) {
        focalDist = 0.5F * (nearDist + farDist);  // NOLINT
    }

    SbLine line;
    SbVec3f pt;
    SbPlane focalPlane = vol.getPlane(focalDist);
    vol.projectPointToLine(pnt2d, line);
    focalPlane.intersect(line, pt);

    return pt;
}

SbVec2s View3DInventorViewer::getPointOnViewport(const SbVec3f& pnt) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();
    float fRatio = vp.getViewportAspectRatio();
    const SbVec2s& sp = vp.getViewportSizePixels();
    SbViewVolume vv = this->getSoRenderManager()->getCamera()->getViewVolume(fRatio);

    SbVec3f pt(pnt);
    vv.projectToScreen(pt, pt);

    auto xpos = short(std::roundf(pt[0] * sp[0]));  // NOLINT
    auto ypos = short(std::roundf(pt[1] * sp[1]));  // NOLINT

    return {xpos, ypos};
}

QPoint View3DInventorViewer::toQPoint(const SbVec2s& pnt) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();
    const SbVec2s& vps = vp.getViewportSizePixels();
    int xpos = pnt[0];
    int ypos = vps[1] - pnt[1] - 1;

    qreal dev_pix_ratio = devicePixelRatio();
    xpos = int(std::roundf(xpos / dev_pix_ratio));
    ypos = int(std::roundf(ypos / dev_pix_ratio));

    return {xpos, ypos};
}

SbVec2s View3DInventorViewer::fromQPoint(const QPoint& pnt) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();
    const SbVec2s& vps = vp.getViewportSizePixels();
    int xpos = pnt.x();
    int ypos = pnt.y();

    qreal dev_pix_ratio = devicePixelRatio();
    xpos = int(std::roundf(xpos * dev_pix_ratio));
    ypos = int(std::roundf(ypos * dev_pix_ratio));

    return SbVec2s(short(xpos), vps[1] - short(ypos) - 1);
}

void View3DInventorViewer::getNearPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getSoRenderManager()->getCamera();
    if (!pCam) {
        // just do nothing
        return;
    }

    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the front clipping plane
    SbPlane nearPlane = vol.getPlane(vol.nearDist);
    float dist = nearPlane.getDistanceFromOrigin();
    rcNormal = nearPlane.getNormal();
    rcNormal.normalize();
    float nx{};
    float ny{};
    float nz{};
    rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(dist * rcNormal[0], dist * rcNormal[1], dist * rcNormal[2]);
}

void View3DInventorViewer::getFarPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getSoRenderManager()->getCamera();
    if (!pCam) {
        // just do nothing
        return;
    }

    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the back clipping plane
    SbPlane farPlane = vol.getPlane(vol.nearDist+vol.nearToFar);
    float dist = farPlane.getDistanceFromOrigin();
    rcNormal = farPlane.getNormal();
    rcNormal.normalize();
    float nx{};
    float ny{};
    float nz{};
    rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(dist * rcNormal[0], dist * rcNormal[1], dist * rcNormal[2]);
}

SbVec3f View3DInventorViewer::projectOnNearPlane(const SbVec2f& pt) const
{
    SbVec3f pt1;
    SbVec3f pt2;
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    // return invalid point
    if (!cam) {
        return {};
    }

    SbViewVolume vol = cam->getViewVolume();
    vol.projectPointToLine(pt, pt1, pt2);
    return pt1;
}

SbVec3f View3DInventorViewer::projectOnFarPlane(const SbVec2f& pt) const
{
    SbVec3f pt1;
    SbVec3f pt2;
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    // return invalid point
    if (!cam) {
        return {};
    }

    SbViewVolume vol = cam->getViewVolume();
    vol.projectPointToLine(pt, pt1, pt2);
    return pt2;
}

void View3DInventorViewer::projectPointToLine(const SbVec2s& pt, SbVec3f& pt1, SbVec3f& pt2) const
{
    SbVec2f pnt2d = getNormalizedPosition(pt);
    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) {
        return;
    }

    SbViewVolume vol = pCam->getViewVolume();
    vol.projectPointToLine(pnt2d, pt1, pt2);
}

void View3DInventorViewer::toggleClippingPlane(int toggle, bool beforeEditing,
        bool noManip, const Base::Placement &pla)
{
    if (pcClipPlane) {
        if (toggle <= 0) {
            pcViewProviderRoot->removeChild(pcClipPlane);
            pcClipPlane->unref();
            pcClipPlane = nullptr;
        }
        return;
    }

    if (toggle == 0) {
        return;
    }

    Base::Vector3d dir;
    pla.getRotation().multVec(Base::Vector3d(0, 0, -1), dir);
    Base::Vector3d base = pla.getPosition();

    if (!noManip) {
        auto clip = new SoClipPlaneManip;
        pcClipPlane = clip;
        SbBox3f box = getBoundingBox();

        if (!box.isEmpty()) {
            // adjust to overall bounding box of the scene
            clip->setValue(box, Base::convertTo<SbVec3f>(dir), 1.0F);
        }
    }
    else {
        pcClipPlane = new SoClipPlane;
    }

    pcClipPlane->plane.setValue(SbPlane(Base::convertTo<SbVec3f>(dir),
                                        Base::convertTo<SbVec3f>(base)));
    pcClipPlane->ref();
    if (beforeEditing) {
        pcViewProviderRoot->insertChild(pcClipPlane, 0);
    }
    else {
        pcViewProviderRoot->insertChild(pcClipPlane,
                                        pcViewProviderRoot->findChild(pcEditingRoot) + 1);
    }
}

bool View3DInventorViewer::hasClippingPlane() const
{
    return pcClipPlane != nullptr;
}

/**
 * This method picks the closest point to the camera in the underlying scenegraph
 * and returns its location and normal.
 * If no point was picked false is returned.
 */
bool View3DInventorViewer::pickPoint(const SbVec2s& pos, SbVec3f& point, SbVec3f& norm) const
{
    // attempting raypick in the event_cb() callback method
    SoRayPickAction rp(getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.apply(getSoRenderManager()->getSceneGraph());
    SoPickedPoint* Point = rp.getPickedPoint();

    if (Point) {
        point = Point->getObjectPoint();
        norm  = Point->getObjectNormal();
        return true;
    }

    return false;
}

/**
 * This method is provided for convenience and does basically the same as method
 * above unless that it returns an SoPickedPoint object with additional information.
 * \note It is in the response of the client programmer to delete the returned
 * SoPickedPoint object.
 */
SoPickedPoint* View3DInventorViewer::pickPoint(const SbVec2s& pos) const
{
    SoRayPickAction rp(getSoRenderManager()->getViewportRegion());
    rp.setPoint(pos);
    rp.apply(getSoRenderManager()->getSceneGraph());

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : nullptr);
}

const SoPickedPoint* View3DInventorViewer::getPickedPoint(SoEventCallback* n) const
{
    if (selectionRoot) {
        auto ret = selectionRoot->getPickedList(n->getAction(), true);
        if (!ret.empty()) {
            return ret[0].pp;
        }
        return nullptr;
    }
    return n->getPickedPoint();
}

bool View3DInventorViewer::pubSeekToPoint(const SbVec2s& pos)
{
    return this->seekToPoint(pos);
}

void View3DInventorViewer::pubSeekToPoint(const SbVec3f& pos)
{
    this->seekToPoint(pos);
}

std::shared_ptr<NavigationAnimation> View3DInventorViewer::setCameraOrientation(const SbRotation& orientation, const bool moveToCenter) const
{
    return navigation->setCameraOrientation(orientation, moveToCenter);
}

void View3DInventorViewer::setCameraType(SoType type)
{
    inherited::setCameraType(type);

    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) {
        return;
    }

    if (type.isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) {
        // When doing a viewAll() for an orthographic camera and switching
        // to perspective the scene looks completely strange because of the
        // heightAngle. Setting it to 45 deg also causes an issue with a too
        // close camera but we don't have this other ugly effect.

        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(std::numbers::pi / 4.0);  // NOLINT
    }

    lightRotation->rotation.connectFrom(&cam->orientation);
}

void View3DInventorViewer::moveCameraTo(const SbRotation& orientation, const SbVec3f& position, int duration)
{
    SoCamera* camera = getCamera();
    if (!camera) {
        return;
    }

    if (isAnimationEnabled()) {
        startAnimation(
            orientation, camera->position.getValue(), position - camera->position.getValue(), duration, true);
    }

    camera->orientation.setValue(orientation);
    camera->position.setValue(position);
}

void View3DInventorViewer::animatedViewAll(int steps, int ms)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    SbVec3f campos = cam->position.getValue();
    SbRotation camrot = cam->orientation.getValue();
    SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbBox3f box = getBoundingBox();

    float aspectRatio = vp.getViewportAspectRatio();

    if (box.isEmpty()) {
        return;
    }

    SbSphere sphere;
    sphere.circumscribe(box);
    if (sphere.getRadius() == 0) {
        return;
    }

    SbVec3f direction;
    SbVec3f pos(0.0F, 0.0F, 0.0F);
    camrot.multVec(SbVec3f(0, 0, -1), direction);

    bool isOrthographic = false;
    float height = 0;
    float diff = 0;

    if (cam->isOfType(SoOrthographicCamera::getClassTypeId())) {
        isOrthographic = true;
        height = static_cast<SoOrthographicCamera*>(cam)->height.getValue();  // NOLINT
        if (aspectRatio < 1.0F) {
            diff = sphere.getRadius() * 2 - height * aspectRatio;
        }
        else {
            diff = sphere.getRadius() * 2 - height;
        }
        pos = (box.getCenter() - direction * sphere.getRadius());
    }
    else if (cam->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        // NOLINTBEGIN
        float movelength = sphere.getRadius()/float(tan(static_cast<SoPerspectiveCamera*>
            (cam)->heightAngle.getValue() / 2.0));
        // NOLINTEND
        pos = box.getCenter() - direction * movelength;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);

    for (int i=0; i<steps; i++) {
        float par = float(i)/float(steps);

        if (isOrthographic) {
            float camHeight = height + diff * par;
            static_cast<SoOrthographicCamera*>(cam)->height.setValue(camHeight);  // NOLINT
        }

        SbVec3f curpos = campos * (1.0F - par) + pos * par;
        cam->position.setValue(curpos);
        timer.start(Base::clamp<int>(ms, 0, 5000));  // NOLINT
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

#if BUILD_VR
extern View3DInventorRiftViewer* oculusStart(void);
extern bool oculusUp   (void);
extern void oculusStop (void);
void oculusSetTestScene(View3DInventorRiftViewer *window);
#endif

void View3DInventorViewer::viewVR()
{
#if BUILD_VR
    if (oculusUp()) {
        oculusStop();
    }
    else {
        View3DInventorRiftViewer* riftWin = oculusStart();
        riftWin->setSceneGraph(pcViewProviderRoot);
    }
#endif
}

void View3DInventorViewer::boxZoom(const SbBox2s& box)
{
    navigation->boxZoom(box);
}
void View3DInventorViewer::scale(float factor)
{
    navigation->scale(factor);
}

SbBox3f View3DInventorViewer::getBoundingBox() const
{
    SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SoGetBoundingBoxAction action(vp);
    action.apply(this->getSoRenderManager()->getSceneGraph());
    return action.getBoundingBox();
}

void View3DInventorViewer::viewAll()
{
    SbBox3f box = getBoundingBox();

    if (box.isEmpty()) {
        return;
    }

    SbSphere sphere;
    sphere.circumscribe(box);
    if (sphere.getRadius() == 0) {
        return;
    }

    // in the scene graph we may have objects which we want to exclude
    // when doing a fit all. Such objects must be part of the group
    // SoSkipBoundingGroup.
    SoSearchAction sa;
    sa.setType(SoSkipBoundingGroup::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.apply(this->getSoRenderManager()->getSceneGraph());
    const SoPathList& pathlist = sa.getPaths();

    for (int i = 0; i < pathlist.getLength(); i++) {
        SoPath* path = pathlist[i];
        auto group = static_cast<SoSkipBoundingGroup*>(path->getTail());  // NOLINT
        group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
    }

    // Set the height angle to 45 deg
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (cam && cam->getTypeId().isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) {
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(std::numbers::pi / 4.0);  // NOLINT
    }

    if (isAnimationEnabled()) {
        animatedViewAll(10, 20);  // NOLINT
    }

    // make sure everything is visible
    if (cam) {
        cam->viewAll(getSoRenderManager()->getSceneGraph(), this->getSoRenderManager()->getViewportRegion());
    }

    for (int i = 0; i < pathlist.getLength(); i++) {
        SoPath* path = pathlist[i];
        auto group = static_cast<SoSkipBoundingGroup*>(path->getTail());  // NOLINT
        group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
    }
}

void View3DInventorViewer::viewAll(float factor)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    if (factor <= 0.0F) {
        return;
    }

    if (factor != 1.0F) {
        SoSearchAction sa;
        sa.setType(SoSkipBoundingGroup::getClassTypeId());
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(this->getSoRenderManager()->getSceneGraph());
        const SoPathList& pathlist = sa.getPaths();

        for (int i = 0; i < pathlist.getLength(); i++) {
            SoPath* path = pathlist[i];
            auto group = static_cast<SoSkipBoundingGroup*>(path->getTail());  // NOLINT
            group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
        }

        SbBox3f box = getBoundingBox();
        float minx{};
        float miny{};
        float minz{};
        float maxx{};
        float maxy{};
        float maxz{};
        box.getBounds(minx,miny,minz,maxx,maxy,maxz);

        for (int i = 0; i < pathlist.getLength(); i++) {
            SoPath* path = pathlist[i];
            auto group = static_cast<SoSkipBoundingGroup*>(path->getTail());  // NOLINT
            group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
        }

        auto cube = new SoCube();
        cube->width  = factor*(maxx-minx);
        cube->height = factor*(maxy-miny);
        cube->depth  = factor*(maxz-minz);

        // fake a scenegraph with the desired bounding size
        auto graph = new SoSeparator();
        graph->ref();
        auto tr = new SoTranslation();
        tr->translation.setValue(box.getCenter());

        graph->addChild(tr);
        graph->addChild(cube);
        cam->viewAll(graph, this->getSoRenderManager()->getViewportRegion());
        graph->unref();
    }
    else {
        viewAll();
    }
}

void View3DInventorViewer::viewSelection()
{
    Base::BoundBox3d bbox;
    for(auto &sel : Selection().getSelection(nullptr, ResolveMode::NoResolve)) {
        auto vp = Application::Instance->getViewProvider(sel.pObject);
        if(!vp) {
            continue;
        }
        bbox.Add(vp->getBoundingBox(sel.SubName,true));
    }

    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (cam && bbox.IsValid()) {
        SbBox3f box(float(bbox.MinX),
                    float(bbox.MinY),
                    float(bbox.MinZ),
                    float(bbox.MaxX),
                    float(bbox.MaxY),
                    float(bbox.MaxZ));
        float aspectratio = getSoRenderManager()->getViewportRegion().getViewportAspectRatio();
        switch (cam->viewportMapping.getValue()) {
            case SoCamera::CROP_VIEWPORT_FILL_FRAME:
            case SoCamera::CROP_VIEWPORT_LINE_FRAME:
            case SoCamera::CROP_VIEWPORT_NO_FRAME:
                aspectratio = 1.0F;
                break;
            default:
                break;
        }
        cam->viewBoundingBox(box,aspectratio,1.0);
    }
}

void View3DInventorViewer::alignToSelection()
{
    if (!getCamera()) {
        return;
    }

    const auto selection = Selection().getSelection(nullptr, ResolveMode::NoResolve);

    // Empty selection
    if (selection.empty()) {
        return;
    }

    // Too much selections
    if (selection.size() > 1) {
        return;
    }

    // Get the geo feature
    App::GeoFeature* geoFeature = nullptr;
    App::ElementNamePair elementName;
    App::GeoFeature::resolveElement(selection[0].pObject, selection[0].SubName, elementName, true, App::GeoFeature::ElementNameType::Normal, nullptr, nullptr, &geoFeature);
    if (!geoFeature) {
        return;
    }

    const auto globalPlacement = App::GeoFeature::getGlobalPlacement(selection[0].pResolvedObject, selection[0].pObject, elementName.oldName);
    const auto globalRotation = globalPlacement.getRotation() * geoFeature->Placement.getValue().getRotation().inverse();
    const auto splitSubName = Base::Tools::splitSubName(elementName.oldName);
    const auto geoFeatureSubName = !splitSubName.empty() ? splitSubName.back() : "";

    Base::Vector3d alignmentZ;
    Base::Vector3d alignmentX (0, 0, 0);
    if (geoFeature->getCameraAlignmentDirection(alignmentZ, alignmentX, geoFeatureSubName.c_str())) {

        // Find a x alignment if the geoFeature did not suggest any
        if (alignmentX == Base::Vector3d (0, 0, 0)) {
            Base::Rotation(-Base::Vector3d::UnitZ, alignmentZ).multVec(Base::Vector3d::UnitX, alignmentX);
        }

        // Convert to global coordinates
        globalRotation.multVec(alignmentZ, alignmentZ);
        globalRotation.multVec(alignmentX, alignmentX);

        const auto cameraOrientation = getCameraOrientation();

        auto directionZ = Base::convertTo<SbVec3f>(alignmentZ);
        auto directionX = Base::convertTo<SbVec3f>(alignmentX);

        SbVec3f cameraZ;
        cameraOrientation.multVec(SbVec3f(0, 0, 1), cameraZ);

        // Negate if the camera is closer to the opposite direction
        if (cameraZ.dot(directionZ) < 0) {
            directionZ.negate();
        }

        // Rotate the camera to align with directionZ by the smallest angle to align the z-axis
        const SbRotation intermediateOrientation = cameraOrientation * SbRotation(cameraZ, directionZ);

        SbVec3f intermediateX;
        intermediateOrientation.multVec(SbVec3f(1, 0, 0), intermediateX);

        // Find angle between directionX and intermediateX
        const SbRotation rotation(directionX, intermediateX);
        SbVec3f axis;
        float angle;
        rotation.getValue(axis, angle);

        // Flip the sign of the angle if axis and directionZ are in opposite direction
        if (axis.dot(directionZ) < 0 && angle != 0) {
            angle *= -1;
        }

        constexpr auto pi = std::numbers::pi_v<float>;

        // Make angle positive
        if (angle < 0) {
            angle += 2 * pi;
        }

        // Find the angle to rotate to the nearest horizontal or vertical alignment with directionX.
        // f is a small value used to get more deterministic behavior when the camera is at directionX +- 45 degrees.
        const float f = 0.00001F;

        if (angle <= pi/4 + f) {
            angle = 0;
        }
        else if (angle <= 3 * pi/4 + f) {
            angle = pi/2;
        }
        else if (angle < pi + pi/4 - f) {
            angle = pi;
        }
        else if (angle < pi + 3 * pi/4 - f) {
            angle = pi + pi/2;
        }
        else {
            angle = 0;
        }

        SbRotation(directionZ, angle).multVec(directionX, directionX);

        const SbVec3f directionY = directionZ.cross(directionX);

        const auto orientation = SbRotation(SbMatrix(
            directionX[0],  directionX[1],  directionX[2],  0,
            directionY[0],  directionY[1],  directionY[2],  0,
            directionZ[0],  directionZ[1],  directionZ[2],  0,
            0,              0,              0,              1));

        setCameraOrientation(orientation);
    }
}

/**
 * @brief Decide if it should be possible to start any animation
 *
 * If the enable flag is false and we're currently animating, the animation will be stopped
 */
void View3DInventorViewer::setAnimationEnabled(bool enable)
{
    navigation->setAnimationEnabled(enable);
}

/**
 * @brief Decide if it should be possible to start a spin animation of the model in the viewer by releasing the mouse button while dragging
 *
 * If the enable flag is false and we're currently animating, the spin animation will be stopped
 */
void View3DInventorViewer::setSpinningAnimationEnabled(bool enable)
{
    navigation->setSpinningAnimationEnabled(enable);
}

/**
 * @return Whether or not it is possible to start any animation
 */
bool View3DInventorViewer::isAnimationEnabled() const
{
    return navigation->isAnimationEnabled();
}

/**
 * @return Whether or not it is possible to start a spinning animation e.g. after dragging
 */
bool View3DInventorViewer::isSpinningAnimationEnabled() const
{
    return navigation->isSpinningAnimationEnabled();
}

/**
 * @return Whether or not any animation is currently active
 */
bool View3DInventorViewer::isAnimating() const
{
    return navigation->isAnimating();
}

/**
 * @return Whether or not a spinning animation is currently active e.g. after a user drag
 */
bool View3DInventorViewer::isSpinning() const
{
    return navigation->isSpinning();
}

/**
 * @brief Change the camera pose with an animation
 *
 * @param orientation The new orientation
 * @param rotationCenter The rotation center
 * @param translation An additional translation on top of the translation caused by the rotation around the rotation center
 * @param duration The duration in milliseconds
 * @param wait When false, start the animation and continue (asynchronous). When true, start the animation and wait for the animation to finish (synchronous)
 *
 * @return The started @class NavigationAnimation
 */
std::shared_ptr<NavigationAnimation> View3DInventorViewer::startAnimation(const SbRotation& orientation,
                                          const SbVec3f& rotationCenter,
                                          const SbVec3f& translation,
                                          int duration,
                                          const bool wait) const
{
    // Currently starts a FixedTimeAnimation. If there is going to be an additional animation like
    // FixedVelocityAnimation, check the animation type from a parameter and start the right animation

    // Duration was not set or is invalid so use the AnimationDuration parameter as default
    if (duration < 0) {
        duration = App::GetApplication()
                       .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                       ->GetInt("AnimationDuration", 500);
    }

    QEasingCurve::Type easingCurve = static_cast<QEasingCurve::Type>(App::GetApplication()
                         .GetParameterGroupByPath("User parameter:BaseApp/Preferences/View")
                         ->GetInt("NavigationAnimationEasingCurve", QEasingCurve::Type::InOutCubic));

    auto animation = std::make_shared<FixedTimeAnimation>(
        navigation, orientation, rotationCenter, translation, duration, easingCurve);

    navigation->startAnimating(animation, wait);

    return animation;
}

/**
 * @brief Start an infinite spin animation
 *
 * @param axis The rotation axis in screen coordinates
 * @param velocity The angular velocity in radians per second
 */
void View3DInventorViewer::startSpinningAnimation(const SbVec3f& axis, float velocity)
{
    auto animation = std::make_shared<SpinningAnimation>(navigation, axis, velocity);
    navigation->startAnimating(animation);
}

void View3DInventorViewer::stopAnimating()
{
    navigation->stopAnimating();
}

void View3DInventorViewer::setPopupMenuEnabled(bool on)
{
    navigation->setPopupMenuEnabled(on);
}

bool View3DInventorViewer::isPopupMenuEnabled() const
{
    return navigation->isPopupMenuEnabled();
}

/*!
  Set the flag deciding whether or not to show the axis cross.
*/

void
View3DInventorViewer::setFeedbackVisibility(bool enable)
{
    if (enable == this->axiscrossEnabled) {
        return;
    }

    this->axiscrossEnabled = enable;

    if (this->isViewing()) {
        this->getSoRenderManager()->scheduleRedraw();
    }
}

/*!
  Check if the feedback axis cross is visible.
*/

bool
View3DInventorViewer::isFeedbackVisible() const
{
    return this->axiscrossEnabled;
}

/*!
  Set the size of the feedback axiscross.  The value is interpreted as
  an approximate percentage chunk of the dimensions of the total
  canvas.
*/
void
View3DInventorViewer::setFeedbackSize(int size)
{
    if (size < 1) {
        return;
    }

    this->axiscrossSize = size;

    if (this->isFeedbackVisible() && this->isViewing()) {
        this->getSoRenderManager()->scheduleRedraw();
    }
}

/*!
  Return the size of the feedback axis cross. Default is 10.
*/

int
View3DInventorViewer::getFeedbackSize() const
{
    return this->axiscrossSize;
}

/*!
  Decide whether or not the mouse pointer cursor should be visible in
  the rendering canvas.
*/
void View3DInventorViewer::setCursorEnabled(bool /*enable*/)
{
    this->setCursorRepresentation(navigation->getViewingMode());
}

void View3DInventorViewer::afterRealizeHook()
{
    inherited::afterRealizeHook();
    this->setCursorRepresentation(navigation->getViewingMode());
}

// Documented in superclass. This method overridden from parent class
// to make sure the mouse pointer cursor is updated.
void View3DInventorViewer::setViewing(bool enable)
{
    if (this->isViewing() == enable) {
        return;
    }

    navigation->setViewingMode(enable ? NavigationStyle::IDLE : NavigationStyle::INTERACT);
    inherited::setViewing(enable);
}

unsigned char View3DInventorViewer::XPM_pixel_data[XPM_WIDTH * XPM_HEIGHT * XPM_BYTES_PER_PIXEL + 1] = {};
unsigned char View3DInventorViewer::YPM_pixel_data[YPM_WIDTH * YPM_HEIGHT * YPM_BYTES_PER_PIXEL + 1] = {};
unsigned char View3DInventorViewer::ZPM_pixel_data[ZPM_WIDTH * ZPM_HEIGHT * ZPM_BYTES_PER_PIXEL + 1] = {};

void View3DInventorViewer::setAxisLetterColor(const SbColor& color)
{
    unsigned packed = color.getPackedValue();

    auto recolor =
        [&](const unsigned char* mask,
            unsigned char* data,
            unsigned width,
            unsigned height,
            unsigned bitdepth) {
            for (unsigned y = 0; y < height; y++) {
                for (unsigned x = 0; x < width; x++) {
                    unsigned offset = (y * width + x) * bitdepth;

                    const unsigned char* src = &mask[offset];
                    unsigned char* dst = &data[offset];

                    dst[0] = (packed >> 24) & 0xFF;  // RR - from color
                    dst[1] = (packed >> 16) & 0xFF;  // GG - from color
                    dst[2] = (packed >> 8) & 0xFF;   // BB - from color
                    dst[3] = src[3];                 // AA - from mask
                }
            }
        };

    recolor(XPM_PIXEL_MASK, XPM_pixel_data, XPM_WIDTH, XPM_HEIGHT, XPM_BYTES_PER_PIXEL);
    recolor(YPM_PIXEL_MASK, YPM_pixel_data, YPM_WIDTH, YPM_HEIGHT, YPM_BYTES_PER_PIXEL);
    recolor(ZPM_PIXEL_MASK, ZPM_pixel_data, ZPM_WIDTH, ZPM_HEIGHT, ZPM_BYTES_PER_PIXEL);
}

void View3DInventorViewer::updateColors()
{
    unsigned long colorLong;

    colorLong = Gui::ViewParams::instance()->getAxisXColor();
    m_xColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisYColor();
    m_yColor = Base::Color(static_cast<uint32_t>(colorLong));
    colorLong = Gui::ViewParams::instance()->getAxisZColor();
    m_zColor = Base::Color(static_cast<uint32_t>(colorLong));

    naviCube->updateColors();

    if(hasAxisCross()) {
        setAxisCross(false);  // Force redraw
        setAxisCross(true);
    }
}

void View3DInventorViewer::drawAxisCross()
{
    // NOLINTBEGIN
    // FIXME: convert this to a superimposition scenegraph instead of
    // OpenGL calls. 20020603 mortene.

    // Store GL state.
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    GLfloat depthrange[2];
    glGetFloatv(GL_DEPTH_RANGE, depthrange);
    GLdouble projectionmatrix[16];
    glGetDoublev(GL_PROJECTION_MATRIX, projectionmatrix);

    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_TRUE);
    glDepthRange(0, 0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glDisable(GL_BLEND); // Kills transparency.

    // Set the viewport in the OpenGL canvas. Dimensions are calculated
    // as a percentage of the total canvas size.
    SbVec2s view = this->getSoRenderManager()->getSize();
    const int pixelarea = int(float(this->axiscrossSize)/100.0F * std::min(view[0], view[1]));
    SbVec2s origin(view[0] - pixelarea, 0);
    glViewport(origin[0], origin[1], pixelarea, pixelarea);

    // Set up the projection matrix.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const float NEARVAL = 0.1F;
    const float FARVAL = 10.0F;
    const float dim = NEARVAL * float(tan(std::numbers::pi / 8.0)); // FOV is 45 deg (45/360 = 1/8)
    glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);


    // Set up the model matrix.
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    SbMatrix mx;
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    // If there is no camera (like for an empty scene, for instance),
    // just use an identity rotation.
    if (cam) {
        mx = cam->orientation.getValue();
    }
    else {
        mx = SbMatrix::identity();
    }

    mx = mx.inverse();
    mx[3][2] = -3.5; // Translate away from the projection point (along z axis).
    glLoadMatrixf((float*)mx);


    // Find unit vector end points.
    SbMatrix px;
    glGetFloatv(GL_PROJECTION_MATRIX, (float*)px);
    SbMatrix comb = mx.multRight(px); // clazy:exclude=rule-of-two-soft

    SbVec3f xpos;
    comb.multVecMatrix(SbVec3f(1,0,0), xpos);
    xpos[0] = (1 + xpos[0]) * view[0]/2;
    xpos[1] = (1 + xpos[1]) * view[1]/2;
    SbVec3f ypos;
    comb.multVecMatrix(SbVec3f(0,1,0), ypos);
    ypos[0] = (1 + ypos[0]) * view[0]/2;
    ypos[1] = (1 + ypos[1]) * view[1]/2;
    SbVec3f zpos;
    comb.multVecMatrix(SbVec3f(0,0,1), zpos);
    zpos[0] = (1 + zpos[0]) * view[0]/2;
    zpos[1] = (1 + zpos[1]) * view[1]/2;


    // Render the cross.
    {
        glLineWidth(2.0);

        enum { XAXIS, YAXIS, ZAXIS };
        int idx[3] = { XAXIS, YAXIS, ZAXIS };
        float val[3] = { xpos[2], ypos[2], zpos[2] };

        // Bubble sort.. :-}
        if (val[0] < val[1]) {
            std::swap(val[0], val[1]);
            std::swap(idx[0], idx[1]);
        }

        if (val[1] < val[2]) {
            std::swap(val[1], val[2]);
            std::swap(idx[1], idx[2]);
        }

        if (val[0] < val[1]) {
            std::swap(val[0], val[1]);
            std::swap(idx[0], idx[1]);
        }

        assert((val[0] >= val[1]) && (val[1] >= val[2])); // Just checking..

        for (const int & i : idx) {
            glPushMatrix();

            if (i == XAXIS) {                        // X axis.
                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)  // What is this
                    glColor3f(0.500F, 0.5F, 0.5F);  // Why different colors??
                else
                    glColor3f(m_xColor.r, m_xColor.g, m_xColor.b);
            }
            else if (i == YAXIS) {                   // Y axis.
                glRotatef(90, 0, 0, 1);

                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
                    glColor3f(0.400F, 0.4F, 0.4F);
                else
                    glColor3f(m_yColor.r, m_yColor.g, m_yColor.b);
            }
            else {                                        // Z axis.
                glRotatef(-90, 0, 1, 0);

                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
                    glColor3f(0.300F, 0.3F, 0.3F);
                else
                    glColor3f(m_zColor.r, m_zColor.g, m_zColor.b);
            }

            drawArrow();
            glPopMatrix();
        }
    }

    // Render axis notation letters ("X", "Y", "Z").
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, view[0], 0, view[1], -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLint unpack{};
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO) {
        glColor3fv(SbVec3f(1.0F, 1.0F, 1.0F).getValue());
    }
    else {
        glColor3fv(SbVec3f(0.0F, 0.0F, 0.0F).getValue());
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelZoom((float)axiscrossSize / 30, (float)axiscrossSize / 30); // 30 = 3 (character pixmap ratio) * 10 (default axiscrossSize)
    glRasterPos2d(xpos[0], xpos[1]);
    glDrawPixels(XPM_WIDTH, XPM_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, XPM_pixel_data);
    glRasterPos2d(ypos[0], ypos[1]);
    glDrawPixels(YPM_WIDTH, YPM_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, YPM_pixel_data);
    glRasterPos2d(zpos[0], zpos[1]);
    glDrawPixels(ZPM_WIDTH, ZPM_HEIGHT, GL_RGBA, GL_UNSIGNED_BYTE, ZPM_pixel_data);

    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
    glPopMatrix();

    // Reset original state.

    // FIXME: are these 3 lines really necessary, as we push
    // GL_ALL_ATTRIB_BITS at the start? 20000604 mortene.
    glDepthRange(depthrange[0], depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionmatrix);

    glPopAttrib();
    // NOLINTEND
}

// Draw an arrow for the axis representation directly through OpenGL.
void View3DInventorViewer::drawArrow()
{
    // NOLINTBEGIN
    glDisable(GL_CULL_FACE);
    glBegin(GL_QUADS);
    glVertex3f(0.0F, -0.02F, 0.02F);
    glVertex3f(0.0F, 0.02F, 0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.02F, 0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.02F, 0.02F);

    glVertex3f(0.0F, -0.02F, -0.02F);
    glVertex3f(0.0F, 0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.02F, -0.02F);

    glVertex3f(0.0F, -0.02F, 0.02F);
    glVertex3f(0.0F, -0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.02F, 0.02F);

    glVertex3f(0.0F, 0.02F, 0.02F);
    glVertex3f(0.0F, 0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.02F, -0.02F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.02F, 0.02F);

    glVertex3f(0.0F, 0.02F, 0.02F);
    glVertex3f(0.0F, 0.02F, -0.02F);
    glVertex3f(0.0F, -0.02F, -0.02F);
    glVertex3f(0.0F, -0.02F, 0.02F);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3f(1.0F, 0.0F, 0.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, +0.5F / 4.0F, 0.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.5F / 4.0F, 0.0F);
    glVertex3f(1.0F, 0.0F, 0.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.0F, +0.5F / 4.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.0F, -0.5F / 4.0F);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(1.0F - 1.0F / 3.0F, +0.5F / 4.0F, 0.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.0F, +0.5F / 4.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, -0.5F / 4.0F, 0.0F);
    glVertex3f(1.0F - 1.0F / 3.0F, 0.0F, -0.5F / 4.0F);
    glEnd();
    // NOLINTEND
}

void View3DInventorViewer::drawSingleBackground(const QColor& col)
{
    // Note: After changing the NaviCube code the content of an image plane may appear black.
    // A workaround is this function.
    // See also: https://github.com/FreeCAD/FreeCAD/pull/9356#issuecomment-1529521654
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(-1, 1, -1, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glDisable(GL_TEXTURE_2D);
    glBegin(GL_TRIANGLE_STRIP);
    glColor3f(float(col.redF()), float(col.greenF()), float(col.blueF()));
    glVertex2f(-1, 1);
    glColor3f(float(col.redF()), float(col.greenF()), float(col.blueF()));
    glVertex2f(-1, -1);
    glColor3f(float(col.redF()), float(col.greenF()), float(col.blueF()));
    glVertex2f(1, 1);
    glColor3f(float(col.redF()), float(col.greenF()), float(col.blueF()));
    glVertex2f(1, -1);
    glEnd();
    glPopAttrib();
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
}

// ************************************************************************

// Set cursor graphics according to mode.
void View3DInventorViewer::setCursorRepresentation(int modearg)
{
    // There is a synchronization problem between Qt and SoQt which
    // happens when popping up a context-menu. In this case the
    // Qt::WA_UnderMouse attribute is reset and never set again
    // even if the mouse is still in the canvas. Thus, the cursor
    // won't be changed as long as the user doesn't leave and enter
    // the canvas. To fix this we explicitly set Qt::WA_UnderMouse
    // if the mouse is inside the canvas.
    QWidget* glWindow = this->getGLWidget();

    // When a widget is added to the QGraphicsScene and the user
    // hovered over it the 'WA_SetCursor' attribute is set to the
    // GL widget but never reset and thus would cause that the
    // cursor on this widget won't be set.
    if (glWindow) {
        glWindow->setAttribute(Qt::WA_SetCursor, false);
    }

    if (glWindow && glWindow->rect().contains(QCursor::pos())) {
        glWindow->setAttribute(Qt::WA_UnderMouse);
    }

    switch (modearg) {
    case NavigationStyle::IDLE:
    case NavigationStyle::INTERACT:
        if (isEditing()) {
            this->getWidget()->setCursor(this->editCursor);
        }
        else {
            this->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
        }
        break;

    case NavigationStyle::DRAGGING:
    case NavigationStyle::SPINNING:
        this->getWidget()->setCursor(spinCursor);
        break;

    case NavigationStyle::ZOOMING:
        this->getWidget()->setCursor(zoomCursor);
        break;

    case NavigationStyle::SEEK_MODE:
    case NavigationStyle::SEEK_WAIT_MODE:
    case NavigationStyle::BOXZOOM:
        this->getWidget()->setCursor(Qt::CrossCursor);
        break;

    case NavigationStyle::PANNING:
        this->getWidget()->setCursor(panCursor);
        break;

    case NavigationStyle::SELECTION:
        this->getWidget()->setCursor(Qt::PointingHandCursor);
        break;

    default:
        assert(0);
        break;
    }
}

void View3DInventorViewer::setEditing(bool edit)
{
    this->editing = edit;
    this->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
    this->editCursor = QCursor();
}

void View3DInventorViewer::setComponentCursor(const QCursor& cursor)
{
    this->getWidget()->setCursor(cursor);
}

void View3DInventorViewer::setEditingCursor(const QCursor& cursor)
{
    this->getWidget()->setCursor(cursor);
    this->editCursor = this->getWidget()->cursor();
}

void View3DInventorViewer::selectCB(void* viewer, SoPath* path)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(path);
    if (vp && vp->useNewSelectionModel()) {
        // do nothing here
    }
}

void View3DInventorViewer::deselectCB(void* viewer, SoPath* path)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(path);
    if (vp && vp->useNewSelectionModel()) {
        // do nothing here
    }
}

SoPath* View3DInventorViewer::pickFilterCB(void* viewer, const SoPickedPoint* pp)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(pp->getPath());
    if (vp && vp->useNewSelectionModel()) {
        std::string str = vp->getElement(pp->getDetail());
        vp->getSelectionShape(str.c_str());
        static char buf[513];
        snprintf(buf,
                 sizeof(buf),
                 "Hovered: %s (%f,%f,%f)"
                 , str.c_str()
                 , pp->getPoint()[0]
                 , pp->getPoint()[1]
                 , pp->getPoint()[2]);

        getMainWindow()->showMessage(QString::fromLatin1(buf), 3000);
    }

    return pp->getPath();
}

void View3DInventorViewer::addEventCallback(SoType eventtype, SoEventCallbackCB* cb, void* userdata)
{
    pEventCallback->addEventCallback(eventtype, cb, userdata);
}

void View3DInventorViewer::removeEventCallback(SoType eventtype, SoEventCallbackCB* cb, void* userdata)
{
    pEventCallback->removeEventCallback(eventtype, cb, userdata);
}

ViewProvider* View3DInventorViewer::getViewProviderByPath(SoPath* path) const
{
    if (!guiDocument) {
        Base::Console().warning("View3DInventorViewer::getViewProviderByPath: No document set\n");
        return nullptr;
    }
    return guiDocument->getViewProviderByPathFromHead(path);
}

ViewProvider* View3DInventorViewer::getViewProviderByPathFromTail(SoPath* path) const
{
    if (!guiDocument) {
        Base::Console().warning("View3DInventorViewer::getViewProviderByPathFromTail: No document set\n");
        return nullptr;
    }
    return guiDocument->getViewProviderByPathFromTail(path);
}

std::vector<ViewProvider*> View3DInventorViewer::getViewProvidersOfType(const Base::Type& typeId) const
{
    if (!guiDocument) {
        Base::Console().warning("View3DInventorViewer::getViewProvidersOfType: No document set\n");
        return {};
    }
    return guiDocument->getViewProvidersOfType(typeId);
}

void View3DInventorViewer::turnAllDimensionsOn()
{
    dimensionRoot->whichChild = SO_SWITCH_ALL;
}

void View3DInventorViewer::turnAllDimensionsOff()
{
    dimensionRoot->whichChild = SO_SWITCH_NONE;
}

void View3DInventorViewer::eraseAllDimensions()
{
    coinRemoveAllChildren(static_cast<SoSwitch*>(dimensionRoot->getChild(0)));  // NOLINT
    coinRemoveAllChildren(static_cast<SoSwitch*>(dimensionRoot->getChild(1)));  // NOLINT
}

void View3DInventorViewer::turn3dDimensionsOn()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_ALL;  // NOLINT
}

void View3DInventorViewer::turn3dDimensionsOff()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_NONE;  // NOLINT
}

void View3DInventorViewer::addDimension3d(SoNode* node)
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->addChild(node);  // NOLINT
}

void View3DInventorViewer::addDimensionDelta(SoNode* node)
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->addChild(node);  // NOLINT
}

void View3DInventorViewer::turnDeltaDimensionsOn()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_ALL;  // NOLINT
}

void View3DInventorViewer::turnDeltaDimensionsOff()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_NONE;  // NOLINT
}

PyObject *View3DInventorViewer::getPyObject()
{
    if (!_viewerPy) {
        _viewerPy = new View3DInventorViewerPy(this);
    }

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

/**
 * Drops the event \a e and loads the files into the given document.
 */
void View3DInventorViewer::dropEvent (QDropEvent* ev)
{
    const QMimeData* data = ev->mimeData();
    if (data->hasUrls() && selectionRoot && selectionRoot->pcDocument) {
        getMainWindow()->loadUrls(selectionRoot->pcDocument->getDocument(), data->urls());
    }
    else {
        inherited::dropEvent(ev);
    }
}

void View3DInventorViewer::dragEnterEvent (QDragEnterEvent* ev)
{
    // Here we must allow uri drags and check them in dropEvent
    const QMimeData* data = ev->mimeData();
    if (data->hasUrls()) {
        ev->accept();
    }
    else {
        inherited::dragEnterEvent(ev);
    }
}

void View3DInventorViewer::dragMoveEvent(QDragMoveEvent* ev)
{
    const QMimeData* data = ev->mimeData();
    if (data->hasUrls() && selectionRoot && selectionRoot->pcDocument) {
        ev->accept();
    }
    else {
        inherited::dragMoveEvent(ev);
    }
}

void View3DInventorViewer::dragLeaveEvent(QDragLeaveEvent* ev)
{
    inherited::dragLeaveEvent(ev);
}

#include "moc_View3DInventorViewer.cpp" // NOLINT
