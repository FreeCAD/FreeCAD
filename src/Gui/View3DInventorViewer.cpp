/***************************************************************************
 *   Copyright (c) 2004 Juergen Riegel <juergen.riegel@web.de>             *
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
# include <float.h>
# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif
# include <Inventor/SbBox.h>
# include <Inventor/SoEventManager.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoGetMatrixAction.h>
# include <Inventor/actions/SoHandleEventAction.h>
# include <Inventor/actions/SoToVRML2Action.h>
# include <Inventor/actions/SoWriteAction.h>
# include <Inventor/elements/SoViewportRegionElement.h>
# include <Inventor/manips/SoClipPlaneManip.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoCallback.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDirectionalLight.h>
# include <Inventor/nodes/SoEventCallback.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/nodes/SoImage.h>
# include <Inventor/nodes/SoIndexedFaceSet.h>
# include <Inventor/nodes/SoLightModel.h>
# include <Inventor/nodes/SoLocateHighlight.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoMaterialBinding.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/nodes/SoRotationXYZ.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoTranslation.h>
# include <Inventor/nodes/SoSelection.h>
# include <Inventor/nodes/SoText2.h>
# include <Inventor/actions/SoBoxHighlightRenderAction.h>
# include <Inventor/events/SoEvent.h>
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/events/SoLocation2Event.h>
# include <Inventor/events/SoMotion3Event.h>
# include <Inventor/events/SoMouseButtonEvent.h>
# include <Inventor/actions/SoRayPickAction.h>
# include <Inventor/projectors/SbSphereSheetProjector.h>
# include <Inventor/SoOffscreenRenderer.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/VRMLnodes/SoVRMLGroup.h>
# include <QEventLoop>
# include <QKeyEvent>
# include <QWheelEvent>
# include <QMessageBox>
# include <QTimer>
# include <QStatusBar>
# include <QBitmap>
# include <QMimeData>
#endif

#if !defined(FC_OS_MACOSX)
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

#include <QVariantAnimation>

#include <sstream>
#include <Base/Console.h>
#include <Base/Stream.h>
#include <Base/FileInfo.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/UnitsApi.h>

#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObject.h"
#include "SoFCBackgroundGradient.h"
#include "SoFCColorBar.h"
#include "SoFCColorLegend.h"
#include "SoFCColorGradient.h"
#include "SoFCOffscreenRenderer.h"
#include "SoFCSelection.h"
#include "SoFCUnifiedSelection.h"
#include "SoFCInteractiveElement.h"
#include "SoFCBoundingBox.h"
#include "SoAxisCrossKit.h"
#include "View3DInventorRiftViewer.h"

#include "Selection.h"
#include "SoFCSelectionAction.h"
#include "SoFCVectorizeU3DAction.h"
#include "SoFCVectorizeSVGAction.h"
#include "SoFCDB.h"
#include "Application.h"
#include "MainWindow.h"
#include "NavigationStyle.h"
#include "ViewProvider.h"
#include "SpaceballEvent.h"
#include "GLPainter.h"
#include <Quarter/eventhandlers/EventFilter.h>
#include <Quarter/devices/InputDevice.h>
#include "View3DViewerPy.h"
#include <Gui/NaviCube.h>

#include <Inventor/draggers/SoCenterballDragger.h>
#include <Inventor/annex/Profiler/SoProfiler.h>
#include <Inventor/annex/HardCopy/SoVectorizePSAction.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <QGesture>

#include "SoTouchEvents.h"
#include "WinNativeGestureRecognizers.h"
#include "Document.h"

//#define FC_LOGGING_CB

using namespace Gui;

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
 0x00,0x0f,0x80,0x1f,0xc0,0x3f,0xe0,0x7f,0xf0,0xff,0xf0,0xff,0xf0,0xff,0x00,
 0x0f,0x00,0x0f,0xf0,0xff,0xf0,0xff,0xf0,0xff,0xe0,0x7f,0xc0,0x3f,0x80,0x1f,
 0x00,0x0f
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
 0xc0,0x03,0xe0,0x03,0xe0,0x07,0xf0,0x0f,0xe8,0x17,0xdc,0x3b,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xdc,0x3b,0xe8,0x17,0xf0,0x0f,0xe0,0x07,0xc0,0x03,
 0xc0,0x03
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
 0xf0,0xef,0xf8,0xff,0xfc,0xff,0xfc,0xff,0x3c,0xfe,0x1c,0xff,0x00,0xff,0x00,
 0xff,0xff,0x00,0xff,0x00,0xff,0x38,0x7f,0x3c,0xff,0x3f,0xff,0x3f,0xff,0x1f,
 0xf7,0x0f
};


/*!
As ProgressBar has no chance to control the incoming Qt events of Quarter so we need to stop
the event handling to prevent the scenegraph from being selected or deselected
while the progress bar is running.
*/
class Gui::ViewerEventFilter : public QObject
{
public:
    ViewerEventFilter() {}
    ~ViewerEventFilter() {}

    bool eventFilter(QObject* obj, QEvent* event) {

#ifdef GESTURE_MESS
        if (obj->isWidgetType()) {
            View3DInventorViewer* v = dynamic_cast<View3DInventorViewer*>(obj);
            if(v) {
                /* Internally, Qt seems to set up the gestures upon showing the
                 * widget (but after this event is processed), thus invalidating
                 * our settings. This piece takes care to retune gestures on the
                 * next event after the show event.
                 */
                if(v->winGestureTuneState == View3DInventorViewer::ewgtsNeedTuning) {
                    try{
                        WinNativeGestureRecognizerPinch::TuneWindowsGestures(v);
                        v->winGestureTuneState = View3DInventorViewer::ewgtsTuned;
                    } catch (Base::Exception &e) {
                        Base::Console().Warning("Failed to TuneWindowsGestures. Error: %s\n",e.what());
                        v->winGestureTuneState = View3DInventorViewer::ewgtsDisabled;
                    } catch (...) {
                        Base::Console().Warning("Failed to TuneWindowsGestures. Unknown error.\n");
                        v->winGestureTuneState = View3DInventorViewer::ewgtsDisabled;
                    }
                }
                if (event->type() == QEvent::Show && v->winGestureTuneState == View3DInventorViewer::ewgtsTuned)
                    v->winGestureTuneState = View3DInventorViewer::ewgtsNeedTuning;

            }
        }
#endif

        // Bug #0000607: Some mice also support horizontal scrolling which however might
        // lead to some unwanted zooming when pressing the MMB for panning.
        // Thus, we filter out horizontal scrolling.
        if (event->type() == QEvent::Wheel) {
            QWheelEvent* we = static_cast<QWheelEvent*>(event);
            if (we->orientation() == Qt::Horizontal)
                return true;
        }
        else if (event->type() == QEvent::KeyPress) {
            QKeyEvent* ke = static_cast<QKeyEvent*>(event);
            if (ke->matches(QKeySequence::SelectAll)) {
                static_cast<View3DInventorViewer*>(obj)->selectAll();
                return true;
            }
        }
        if (Base::Sequencer().isRunning() && Base::Sequencer().isBlocking())
            return false;

        if (event->type() == Spaceball::ButtonEvent::ButtonEventType) {
            Spaceball::ButtonEvent* buttonEvent = static_cast<Spaceball::ButtonEvent*>(event);
            if (!buttonEvent) {
                Base::Console().Log("invalid spaceball button event\n");
                return true;
            }
        }
        else if (event->type() == Spaceball::MotionEvent::MotionEventType) {
            Spaceball::MotionEvent* motionEvent = static_cast<Spaceball::MotionEvent*>(event);
            if (!motionEvent) {
                Base::Console().Log("invalid spaceball motion event\n");
                return true;
            }
        }

        return false;
    }
};

class SpaceNavigatorDevice : public Quarter::InputDevice {
public:
    SpaceNavigatorDevice(void) {}
    virtual ~SpaceNavigatorDevice() {}
    virtual const SoEvent* translateEvent(QEvent* event) {

        if (event->type() == Spaceball::MotionEvent::MotionEventType) {
            Spaceball::MotionEvent* motionEvent = static_cast<Spaceball::MotionEvent*>(event);
            if (!motionEvent) {
                Base::Console().Log("invalid spaceball motion event\n");
                return NULL;
            }

            motionEvent->setHandled(true);

            float xTrans, yTrans, zTrans;
            xTrans = static_cast<float>(motionEvent->translationX());
            yTrans = static_cast<float>(motionEvent->translationY());
            zTrans = static_cast<float>(motionEvent->translationZ());
            SbVec3f translationVector(xTrans, yTrans, zTrans);

            static float rotationConstant(.0001f);
            SbRotation xRot, yRot, zRot;
            xRot.setValue(SbVec3f(1.0, 0.0, 0.0), static_cast<float>(motionEvent->rotationX()) * rotationConstant);
            yRot.setValue(SbVec3f(0.0, 1.0, 0.0), static_cast<float>(motionEvent->rotationY()) * rotationConstant);
            zRot.setValue(SbVec3f(0.0, 0.0, 1.0), static_cast<float>(motionEvent->rotationZ()) * rotationConstant);

            SoMotion3Event* motion3Event = new SoMotion3Event;
            motion3Event->setTranslation(translationVector);
            motion3Event->setRotation(xRot * yRot * zRot);

            return motion3Event;
        }

        return NULL;
    };
};

/** \defgroup View3D 3D Viewer
 *  \ingroup GUI
 *
 * The 3D Viewer is one of the major components in a CAD/CAE systems.
 * Therefore an overview and some remarks to the FreeCAD 3D viewing system.
 *
 * \section overview Overview
 * \todo Overview and complements for the 3D Viewer
 */


// *************************************************************************

View3DInventorViewer::View3DInventorViewer(QWidget* parent, const QtGLWidget* sharewidget)
    : Quarter::SoQTQuarterAdaptor(parent, sharewidget), editViewProvider(0), navigation(0),
      renderType(Native), framebuffer(0), axisCross(0), axisGroup(0), editing(false), redirected(false),
      allowredir(false), overrideMode("As Is"), _viewerPy(0)
{
    init();
}

View3DInventorViewer::View3DInventorViewer(const QtGLFormat& format, QWidget* parent, const QtGLWidget* sharewidget)
    : Quarter::SoQTQuarterAdaptor(format, parent, sharewidget), editViewProvider(0), navigation(0),
      renderType(Native), framebuffer(0), axisCross(0), axisGroup(0), editing(false), redirected(false),
      allowredir(false), overrideMode("As Is"), _viewerPy(0)
{
    init();
}

void View3DInventorViewer::init()
{
    shading = true;
    fpsEnabled = false;
    vboEnabled = false;

    Gui::Selection().Attach(this);

    // Coin should not clear the pixel-buffer, so the background image
    // is not removed.
    this->setClearWindow(false);

    // setting up the defaults for the spin rotation
    initialize();

    SoOrthographicCamera* cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 1);
    cam->height = 1;
    cam->nearDistance = 0.5;
    cam->farDistance = 1.5;

    // setup light sources
    SoDirectionalLight* hl = this->getHeadlight();
    backlight = new SoDirectionalLight();
    backlight->ref();
    backlight->setName("backlight");
    backlight->direction.setValue(-hl->direction.getValue());
    backlight->on.setValue(false); // by default off

    // Set up background scenegraph with image in it.
    backgroundroot = new SoSeparator;
    backgroundroot->ref();
    this->backgroundroot->addChild(cam);

    // Background stuff
    pcBackGround = new SoFCBackgroundGradient;
    pcBackGround->ref();

    // Set up foreground, overlaid scenegraph.
    this->foregroundroot = new SoSeparator;
    this->foregroundroot->ref();

    SoLightModel* lm = new SoLightModel;
    lm->model = SoLightModel::BASE_COLOR;

    SoBaseColor* bc = new SoBaseColor;
    bc->rgb = SbColor(1, 1, 0);

    cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 5);
    cam->height = 10;
    cam->nearDistance = 0;
    cam->farDistance = 10;

    // dragger
    //SoSeparator * dragSep = new SoSeparator();
    //SoScale *scale = new SoScale();
    //scale->scaleFactor = SbVec3f  (0.2,0.2,0.2);
    //dragSep->addChild(scale);
    //SoCenterballDragger *dragger = new SoCenterballDragger();
    //dragger->center = SbVec3f  (0.8,0.8,0);
    ////dragger->rotation = SbRotation(rrot[0],rrot[1],rrot[2],rrot[3]);
    //dragSep->addChild(dragger);

    this->foregroundroot->addChild(cam);
    this->foregroundroot->addChild(lm);
    this->foregroundroot->addChild(bc);
    //this->foregroundroot->addChild(dragSep);

#if 0
    // NOTE: For every mouse click event the SoSelection searches for the picked
    // point which causes a certain slow-down because for all objects the primitives
    // must be created. Using an SoSeparator avoids this drawback.
    SoSelection* selectionRoot = new SoSelection();
    selectionRoot->addSelectionCallback(View3DInventorViewer::selectCB, this);
    selectionRoot->addDeselectionCallback(View3DInventorViewer::deselectCB, this);
    selectionRoot->setPickFilterCallback(View3DInventorViewer::pickFilterCB, this);
#else
    // NOTE: For every mouse click event the SoFCUnifiedSelection searches for the picked
    // point which causes a certain slow-down because for all objects the primitives
    // must be created. Using an SoSeparator avoids this drawback.
    selectionRoot = new Gui::SoFCUnifiedSelection();
    selectionRoot->applySettings();
#endif
    // set the ViewProvider root node
    pcViewProviderRoot = selectionRoot;

    // increase refcount before passing it to setScenegraph(), to avoid
    // premature destruction
    pcViewProviderRoot->ref();
    // is not really working with Coin3D.
    //redrawOverlayOnSelectionChange(pcSelection);
    setSceneGraph(pcViewProviderRoot);
    // Event callback node
    pEventCallback = new SoEventCallback();
    pEventCallback->setUserData(this);
    pEventCallback->ref();
    pcViewProviderRoot->addChild(pEventCallback);
    pEventCallback->addEventCallback(SoEvent::getClassTypeId(), handleEventCB, this);

    dimensionRoot = new SoSwitch(SO_SWITCH_NONE);
    pcViewProviderRoot->addChild(dimensionRoot);
    dimensionRoot->addChild(new SoSwitch()); //first one will be for the 3d dimensions.
    dimensionRoot->addChild(new SoSwitch()); //second one for the delta dimensions.

    // This is a callback node that logs all action that traverse the Inventor tree.
#if defined (FC_DEBUG) && defined(FC_LOGGING_CB)
    SoCallback* cb = new SoCallback;
    cb->setCallback(interactionLoggerCB, this);
    pcViewProviderRoot->addChild(cb);
#endif

    // Set our own render action which show a bounding box if
    // the SoFCSelection::BOX style is set
    //
    // Important note:
    // When creating a new GL render action we have to copy over the cache context id
    // because otherwise we may get strange rendering behaviour. For more details see
    // http://forum.freecadweb.org/viewtopic.php?f=10&t=7486&start=120#p74398 and for
    // the fix and some details what happens behind the scene have a look at this
    // http://forum.freecadweb.org/viewtopic.php?f=10&t=7486&p=74777#p74736
    uint32_t id = this->getSoRenderManager()->getGLRenderAction()->getCacheContext();
    this->getSoRenderManager()->setGLRenderAction(new SoBoxSelectionRenderAction);
    this->getSoRenderManager()->getGLRenderAction()->setCacheContext(id);

    // set the transparency and antialiasing settings
//  getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    getSoRenderManager()->getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
//  getGLRenderAction()->setSmoothing(true);

    // Settings
    setSeekTime(0.4f);

    if (isSeekValuePercentage() == false)
        setSeekValueAsPercentage(true);

    setSeekDistance(100);
    setViewing(false);

    setBackgroundColor(QColor(25, 25, 25));
    setGradientBackground(true);

    // set some callback functions for user interaction
    addStartCallback(interactionStartCB);
    addFinishCallback(interactionFinishCB);

    //filter a few qt events
    viewerEventFilter = new ViewerEventFilter;
    installEventFilter(viewerEventFilter);
    getEventFilter()->registerInputDevice(new SpaceNavigatorDevice);
    getEventFilter()->registerInputDevice(new GesturesDevice(this));

    this->winGestureTuneState = View3DInventorViewer::ewgtsDisabled;
    try{
        this->grabGesture(Qt::PanGesture);
        this->grabGesture(Qt::PinchGesture);
    #ifdef GESTURE_MESS
        {
            static WinNativeGestureRecognizerPinch* recognizer;//static to avoid creating more than one recognizer, thus causing memory leak and gradual slowdown
            if(recognizer == 0){
                recognizer = new WinNativeGestureRecognizerPinch;
                recognizer->registerRecognizer(recognizer); //From now on, Qt owns the pointer.
            }
        }
        this->winGestureTuneState = View3DInventorViewer::ewgtsNeedTuning;
    #endif
    } catch (Base::Exception &e) {
        Base::Console().Warning("Failed to set up gestures. Error: %s\n", e.what());
    } catch (...) {
        Base::Console().Warning("Failed to set up gestures. Unknown error.\n");
    }

    //create the cursors
    QBitmap cursor = QBitmap::fromData(QSize(ROTATE_WIDTH, ROTATE_HEIGHT), rotate_bitmap);
    QBitmap mask = QBitmap::fromData(QSize(ROTATE_WIDTH, ROTATE_HEIGHT), rotate_mask_bitmap);
    spinCursor = QCursor(cursor, mask, ROTATE_HOT_X, ROTATE_HOT_Y);

    cursor = QBitmap::fromData(QSize(ZOOM_WIDTH, ZOOM_HEIGHT), zoom_bitmap);
    mask = QBitmap::fromData(QSize(ZOOM_WIDTH, ZOOM_HEIGHT), zoom_mask_bitmap);
    zoomCursor = QCursor(cursor, mask, ZOOM_HOT_X, ZOOM_HOT_Y);

    cursor = QBitmap::fromData(QSize(PAN_WIDTH, PAN_HEIGHT), pan_bitmap);
    mask = QBitmap::fromData(QSize(PAN_WIDTH, PAN_HEIGHT), pan_mask_bitmap);
    panCursor = QCursor(cursor, mask, PAN_HOT_X, PAN_HOT_Y);
    naviCube = new NaviCube(this);
    naviCubeEnabled = true;
}

View3DInventorViewer::~View3DInventorViewer()
{
    // to prevent following OpenGL error message: "Texture is not valid in the current context. Texture has not been destroyed"
    aboutToDestroyGLContext();

    // cleanup
    this->backgroundroot->unref();
    this->backgroundroot = 0;
    this->foregroundroot->unref();
    this->foregroundroot = 0;
    this->pcBackGround->unref();
    this->pcBackGround = 0;

    setSceneGraph(0);
    this->pEventCallback->unref();
    this->pEventCallback = 0;
    // Note: It can happen that there is still someone who references
    // the root node but isn't destroyed when closing this viewer so
    // that it prevents all children from being deleted. To reduce this
    // likelihood we explicitly remove all child nodes now.
    this->pcViewProviderRoot->removeAllChildren();
    this->pcViewProviderRoot->unref();
    this->pcViewProviderRoot = 0;
    this->backlight->unref();
    this->backlight = 0;

    delete this->navigation;

    // Note: When closing the application the main window doesn't exist any more.
    if (getMainWindow())
        getMainWindow()->setPaneText(2, QLatin1String(""));

    Gui::Selection().Detach(this);

    removeEventFilter(viewerEventFilter);
    delete viewerEventFilter;

    if (_viewerPy) {
        static_cast<View3DInventorViewerPy*>(_viewerPy)->_viewer = 0;
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

void View3DInventorViewer::aboutToDestroyGLContext()
{
    if (naviCube) {
        QtGLWidget* gl = qobject_cast<QtGLWidget*>(this->viewport());
        if (gl)
            gl->makeCurrent();
        delete naviCube;
        naviCube = 0;
        naviCubeEnabled = false;
    }
}

void View3DInventorViewer::setDocument(Gui::Document* pcDocument)
{
    // write the document the viewer belongs to the selection node
    guiDocument = pcDocument;
    selectionRoot->pcDocument = pcDocument;
}

Document* View3DInventorViewer::getDocument() {
    return guiDocument;
}


void View3DInventorViewer::initialize()
{
    navigation = new CADNavigationStyle();
    navigation->setViewer(this);

    this->axiscrossEnabled = true;
    this->axiscrossSize = 10;
}

/// @cond DOXERR
void View3DInventorViewer::OnChange(Gui::SelectionSingleton::SubjectType& rCaller,
                                    Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
        SoFCSelectionAction cAct(Reason);
        cAct.apply(pcViewProviderRoot);
    }
    else if (Reason.Type == SelectionChanges::RmvPreselect ||
        Reason.Type == SelectionChanges::SetPreselect) {
        SoFCHighlightAction cAct(Reason);
        cAct.apply(pcViewProviderRoot);
    }
}
/// @endcond

SbBool View3DInventorViewer::searchNode(SoNode* node) const
{
    SoSearchAction searchAction;
    searchAction.setNode(node);
    searchAction.setInterest(SoSearchAction::FIRST);
    searchAction.apply(this->getSceneGraph());
    SoPath* selectionPath = searchAction.getPath();
    return selectionPath ? true : false;
}

SbBool View3DInventorViewer::hasViewProvider(ViewProvider* pcProvider) const
{
    return _ViewProviderSet.find(pcProvider) != _ViewProviderSet.end();
}

/// adds an ViewProvider to the view, e.g. from a feature
void View3DInventorViewer::addViewProvider(ViewProvider* pcProvider)
{
    SoSeparator* root = pcProvider->getRoot();

    if (root) {
        pcViewProviderRoot->addChild(root);
        _ViewProviderMap[root] = pcProvider;
    }

    SoSeparator* fore = pcProvider->getFrontRoot();
    if (fore)
        foregroundroot->addChild(fore);

    SoSeparator* back = pcProvider->getBackRoot();
    if (back)
        backgroundroot->addChild(back);

    pcProvider->setOverrideMode(this->getOverrideMode());
    _ViewProviderSet.insert(pcProvider);
}

void View3DInventorViewer::removeViewProvider(ViewProvider* pcProvider)
{
    if (this->editViewProvider == pcProvider)
        resetEditingViewProvider();

    SoSeparator* root = pcProvider->getRoot();

    if (root && (pcViewProviderRoot->findChild(root) != -1)) {
        pcViewProviderRoot->removeChild(root);
        _ViewProviderMap.erase(root);
    }

    SoSeparator* fore = pcProvider->getFrontRoot();
    if (fore)
        foregroundroot->removeChild(fore);

    SoSeparator* back = pcProvider->getBackRoot();
    if (back)
        backgroundroot->removeChild(back);

    _ViewProviderSet.erase(pcProvider);
}

SbBool View3DInventorViewer::setEditingViewProvider(Gui::ViewProvider* p, int ModNum)
{
    if (this->editViewProvider)
        return false; // only one view provider is editable at a time

    bool ok = p->startEditing(ModNum);

    if (ok) {
        this->editViewProvider = p;
        this->editViewProvider->setEditViewer(this, ModNum);
        addEventCallback(SoEvent::getClassTypeId(), Gui::ViewProvider::eventCallback,this->editViewProvider);
    }

    return ok;
}

/// reset from edit mode
void View3DInventorViewer::resetEditingViewProvider()
{
    if (this->editViewProvider) {
        // In case the event action still has grabbed a node when leaving edit mode
        // force to release it now
        SoEventManager* mgr = this->getSoEventManager();
        SoHandleEventAction* heaction = mgr->getHandleEventAction();
        if (heaction && heaction->getGrabber())
            heaction->releaseGrabber();

        this->editViewProvider->unsetEditViewer(this);
        removeEventCallback(SoEvent::getClassTypeId(), Gui::ViewProvider::eventCallback,this->editViewProvider);
        this->editViewProvider = 0;
    }
}

/// reset from edit mode
SbBool View3DInventorViewer::isEditingViewProvider() const
{
    return this->editViewProvider ? true : false;
}

/// display override mode
void View3DInventorViewer::setOverrideMode(const std::string& mode)
{
    if (mode == overrideMode)
        return;

    overrideMode = mode;

    auto views = getDocument()->getViewProvidersOfType(Gui::ViewProvider::getClassTypeId());
    if (mode == "No Shading") {
        this->shading = false;
        std::string flatLines = "Flat Lines";
        for (auto view : views)
            view->setOverrideMode(flatLines);
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
    else if (mode == "Hidden Line") {
        this->shading = true;
        std::string shaded = "Shaded";
        for (auto view : views)
            view->setOverrideMode(shaded);
        this->getSoRenderManager()->setRenderMode(SoRenderManager::HIDDEN_LINE);
    }
    else {
        this->shading = true;
        for (auto view : views)
            view->setOverrideMode(mode);
        this->getSoRenderManager()->setRenderMode(SoRenderManager::AS_IS);
    }
}

/// update override mode. doesn't affect providers
void View3DInventorViewer::updateOverrideMode(const std::string& mode)
{
    if (mode == overrideMode)
        return;

    overrideMode = mode;
}

void View3DInventorViewer::setViewportCB(void*, SoAction* action)
{
    // Make sure to override the value set inside SoOffscreenRenderer::render()
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
        const SbViewportRegion& vp = renderer.getViewportRegion();
        SoViewportRegionElement::set(action->getState(), vp);
        static_cast<SoGLRenderAction*>(action)->setViewportRegion(vp);
    }
}

void View3DInventorViewer::clearBufferCB(void*, SoAction* action)
{
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
        QWidget* gl = reinterpret_cast<QWidget*>(userdata);
        SoGLWidgetElement::set(action->getState(), qobject_cast<QtGLWidget*>(gl));
    }
}

void View3DInventorViewer::handleEventCB(void* ud, SoEventCallback* n)
{
    View3DInventorViewer* that = reinterpret_cast<View3DInventorViewer*>(ud);
    SoGLRenderAction* glra = that->getSoRenderManager()->getGLRenderAction();
    SoAction* action = n->getAction();
    SoGLRenderActionElement::set(action->getState(), glra);
    SoGLWidgetElement::set(action->getState(), qobject_cast<QtGLWidget*>(that->getGLWidget()));
}

void View3DInventorViewer::setGradientBackground(bool on)
{
    if (on && backgroundroot->findChild(pcBackGround) == -1)
        backgroundroot->addChild(pcBackGround);
    else if (!on && backgroundroot->findChild(pcBackGround) != -1)
        backgroundroot->removeChild(pcBackGround);
}

bool View3DInventorViewer::hasGradientBackground() const
{
    return (backgroundroot->findChild(pcBackGround) != -1);
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

void View3DInventorViewer::setEnabledNaviCube(bool on)
{
    naviCubeEnabled = on;
}

bool View3DInventorViewer::isEnabledNaviCube(void) const
{
    return naviCubeEnabled;
}

void View3DInventorViewer::setNaviCubeCorner(int c)
{
    if (naviCube)
        naviCube->setCorner(static_cast<NaviCube::Corner>(c));
}

NaviCube* View3DInventorViewer::getNavigationCube() const
{
    return naviCube;
}

void View3DInventorViewer::setAxisCross(bool on)
{
    SoNode* scene = getSceneGraph();
    SoSeparator* sep = static_cast<SoSeparator*>(scene);

    if (on) {
        if (!axisGroup) {
            axisCross = new Gui::SoShapeScale;
            Gui::SoAxisCrossKit* axisKit = new Gui::SoAxisCrossKit();
            axisKit->set("xAxis.appearance.drawStyle", "lineWidth 2");
            axisKit->set("yAxis.appearance.drawStyle", "lineWidth 2");
            axisKit->set("zAxis.appearance.drawStyle", "lineWidth 2");
            axisCross->setPart("shape", axisKit);
            axisCross->scaleFactor = 1.0f;
            axisGroup = new SoSkipBoundingGroup;
            axisGroup->addChild(axisCross);

            sep->addChild(axisGroup);
        }
    }
    else {
        if (axisGroup) {
            sep->removeChild(axisGroup);
            axisGroup = 0;
        }
    }
}

bool View3DInventorViewer::hasAxisCross(void)
{
    return axisGroup;
}

void View3DInventorViewer::setNavigationType(Base::Type t)
{
    if (t.isBad())
        return;

    this->winGestureTuneState = View3DInventorViewer::ewgtsNeedTuning; //triggers enable/disable rotation gesture when preferences change

    if (this->navigation && this->navigation->getTypeId() == t)
        return; // nothing to do

    Base::BaseClass* base = static_cast<Base::BaseClass*>(t.createInstance());
    if (!base)
        return;

    if (!base->getTypeId().isDerivedFrom(NavigationStyle::getClassTypeId())) {
        delete base;
#if FC_DEBUG
        SoDebugError::postWarning("View3DInventorViewer::setNavigationType",
                                  "Navigation object must be of type NavigationStyle.");
#endif // FC_DEBUG
        return;
    }

    NavigationStyle* ns = static_cast<NavigationStyle*>(base);
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

SoDirectionalLight* View3DInventorViewer::getBacklight(void) const
{
    return this->backlight;
}

void View3DInventorViewer::setBacklight(SbBool on)
{
    this->backlight->on = on;
}

SbBool View3DInventorViewer::isBacklight(void) const
{
    return this->backlight->on.getValue();
}

void View3DInventorViewer::setSceneGraph(SoNode* root)
{
    inherited::setSceneGraph(root);
    if (!root) {
        _ViewProviderSet.clear();
        _ViewProviderMap.clear();
        editViewProvider = 0;
    }

    SoSearchAction sa;
    sa.setNode(this->backlight);
    //we want the rendered scene with all lights and cameras, viewer->getSceneGraph would return
    //the geometry scene only
    SoNode* scene = this->getSoRenderManager()->getSceneGraph();
    if (scene && scene->getTypeId().isDerivedFrom(SoSeparator::getClassTypeId())) {
        sa.apply(scene);
        if (!sa.getPath())
            static_cast<SoSeparator*>(scene)->insertChild(this->backlight, 0);
    }
}

void View3DInventorViewer::savePicture(int w, int h, int s, const QColor& bg, QImage& img) const
{
    // Save picture methods:
    // FramebufferObject -- viewer renders into FBO (no offscreen)
    // CoinOffscreenRenderer -- Coin's offscreen rendering method
    // PixelBuffer -- Qt's pixel buffer used for offscreen rendering (only Qt4)
    // Otherwise (Default) -- Qt's FBO used for offscreen rendering
    std::string saveMethod = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetASCII("SavePicture");

    bool useFramebufferObject = false;
    bool usePixelBuffer = false;
    bool useCoinOffscreenRenderer = false;
    if (saveMethod == "FramebufferObject") {
        useFramebufferObject = true;
    }
    else if (saveMethod == "PixelBuffer") {
        usePixelBuffer = true;
    }
    else if (saveMethod == "CoinOffscreenRenderer") {
        useCoinOffscreenRenderer = true;
    }

    if (useFramebufferObject) {
        View3DInventorViewer* self = const_cast<View3DInventorViewer*>(this);
        self->imageFromFramebuffer(w, h, s, bg, img);
        return;
    }

    // if no valid color use the current background
    bool useBackground = false;
    SbViewportRegion vp(getSoRenderManager()->getViewportRegion());

    if (w>0 && h>0)
        vp.setWindowSize((short)w, (short)h);

    //NOTE: To support pixels per inch we must use SbViewportRegion::setPixelsPerInch( ppi );
    //The default value is 72.0.
    //If we need to support grayscale images with must either use SoOffscreenRenderer::LUMINANCE or
    //SoOffscreenRenderer::LUMINANCE_TRANSPARENCY.

    SoCallback* cb = 0;

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

    SoSeparator* root = new SoSeparator;
    root->ref();

#if (COIN_MAJOR_VERSION >= 4)
    // The behaviour in Coin4 has changed so that when using the same instance of 'SoFCOffscreenRenderer'
    // multiple times internally the biggest viewport size is stored and set to the SoGLRenderAction.
    // The trick is to add a callback node and override the viewport size with what we want.
    if (useCoinOffscreenRenderer) {
        SoCallback* cbvp = new SoCallback;
        cbvp->setCallback(setViewportCB);
        root->addChild(cbvp);
    }
#endif

    SoCamera* camera = getSoRenderManager()->getCamera();

    if (useBackground) {
        root->addChild(backgroundroot);
        root->addChild(cb);
    }

    if (!this->shading) {
        SoLightModel* lm = new SoLightModel;
        lm->model = SoLightModel::BASE_COLOR;
        root->addChild(lm);
    }

    root->addChild(getHeadlight());
    root->addChild(camera);
    SoCallback* gl = new SoCallback;
    gl->setCallback(setGLWidgetCB, this->getGLWidget());
    root->addChild(gl);
    root->addChild(pcViewProviderRoot);

#if !defined(HAVE_QT5_OPENGL)
    if (useBackground)
        root->addChild(cb);
#endif

    root->addChild(foregroundroot);

    try {
        // render the scene
        if (!useCoinOffscreenRenderer) {
            SoQtOffscreenRenderer renderer(vp);
            renderer.setNumPasses(s);
            renderer.setPbufferEnable(usePixelBuffer);
            if (bgColor.isValid())
                renderer.setBackgroundColor(SbColor4f(bgColor.redF(), bgColor.greenF(), bgColor.blueF(), bgColor.alphaF()));
            if (!renderer.render(root))
                throw Base::RuntimeError("Offscreen rendering failed");

            renderer.writeToImage(img);
            root->unref();
        }
        else {
            SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
            renderer.setViewportRegion(vp);
            renderer.getGLRenderAction()->setSmoothing(true);
            renderer.getGLRenderAction()->setNumPasses(s);
            if (bgColor.isValid())
                renderer.setBackgroundColor(SbColor(bgColor.redF(), bgColor.greenF(), bgColor.blueF()));
            if (!renderer.render(root))
                throw Base::RuntimeError("Offscreen rendering failed");

            renderer.writeToImage(img);
            root->unref();
        }
    }
    catch (...) {
        root->unref();
        throw; // re-throw exception
    }
}

void View3DInventorViewer::saveGraphic(int pagesize, const QColor& bgcolor, SoVectorizeAction* va) const
{
    if (bgcolor.isValid())
        va->setBackgroundColor(true, SbColor(bgcolor.redF(), bgcolor.greenF(), bgcolor.blueF()));

    float border = 10.0f;
    SbVec2s vpsize = this->getSoRenderManager()->getViewportRegion().getViewportSizePixels();
    float vpratio = ((float)vpsize[0]) / ((float)vpsize[1]);

    if (vpratio > 1.0f) {
        va->setOrientation(SoVectorizeAction::LANDSCAPE);
        vpratio = 1.0f / vpratio;
    }
    else {
        va->setOrientation(SoVectorizeAction::PORTRAIT);
    }

    va->beginStandardPage(SoVectorizeAction::PageSize(pagesize), border);

    // try to fill as much "paper" as possible
    SbVec2f size = va->getPageSize();

    float pageratio = size[0] / size[1];
    float xsize, ysize;

    if (pageratio < vpratio) {
        xsize = size[0];
        ysize = xsize / vpratio;
    }
    else {
        ysize = size[1];
        xsize = ysize * vpratio;
    }

    float offx = border + (size[0]-xsize) * 0.5f;
    float offy = border + (size[1]-ysize) * 0.5f;

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

void View3DInventorViewer::stopSelection()
{
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

SbVec2f View3DInventorViewer::screenCoordsOfPath(SoPath* path) const
{
    // Generate a matrix (well, a SoGetMatrixAction) that
    // moves us to the picked object's coordinate space.
    SoGetMatrixAction gma(getSoRenderManager()->getViewportRegion());
    gma.apply(path);

    // Use that matrix to translate the origin in the picked
    // object's coordinate space into object space
    SbVec3f imageCoords(0, 0, 0);
    SbMatrix m = gma.getMatrix().transpose();
    m.multMatrixVec(imageCoords, imageCoords);

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
    int width = getGLWidget()->width(),
        height = getGLWidget()->height();

    if (width >= height) {
        // "Landscape" orientation, to square
        imageCoords[0] *= height;
        imageCoords[0] += (width-height) / 2.0;
        imageCoords[1] *= height;

    }
    else {
        // "Portrait" orientation
        imageCoords[0] *= width;
        imageCoords[1] *= width;
        imageCoords[1] += (height-width) / 2.0;
    }

    return SbVec2f(imageCoords[0], imageCoords[1]);
}

std::vector<SbVec2f> View3DInventorViewer::getGLPolygon(const std::vector<SbVec2s>& pnts) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();
    const SbVec2s& sz = vp.getWindowSize();
    short w,h;
    sz.getValue(w,h);
    const SbVec2s& sp = vp.getViewportSizePixels();
    const SbVec2s& op = vp.getViewportOriginPixels();
    const SbVec2f& siz = vp.getViewportSize();
    float dX, dY;
    siz.getValue(dX, dY);
    float fRatio = vp.getViewportAspectRatio();

    std::vector<SbVec2f> poly;
    for (std::vector<SbVec2s>::const_iterator it = pnts.begin(); it != pnts.end(); ++it) {
        SbVec2s loc = *it - op;
        SbVec2f pos((float)loc[0]/(float)sp[0], (float)loc[1]/(float)sp[1]);
        float pX,pY;
        pos.getValue(pX,pY);

        // now calculate the real points respecting aspect ratio information
        //
        if (fRatio > 1.0f) {
            pX = (pX - 0.5f*dX) * fRatio + 0.5f*dX;
            pos.setValue(pX,pY);
        }
        else if (fRatio < 1.0f) {
            pY = (pY - 0.5f*dY) / fRatio + 0.5f*dY;
            pos.setValue(pX,pY);
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

    if (fi.hasExtension("idtf") || fi.hasExtension("svg")) {
        int ps=4;
        QColor c = Qt::white;
        std::unique_ptr<SoVectorizeAction> vo;

        if (fi.hasExtension("svg")) {
            vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeSVGAction());
        }
        else if (fi.hasExtension("idtf")) {
            vo = std::unique_ptr<SoVectorizeAction>(new SoFCVectorizeU3DAction());
        }
        else if (fi.hasExtension("ps") || fi.hasExtension("eps")) {
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

        saveGraphic(ps,c,vo.get());
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
void View3DInventorViewer::interactionStartCB(void*, SoQTQuarterAdaptor* viewer)
{
    SoGLRenderAction* glra = viewer->getSoRenderManager()->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), true);
}

/**
 * Sets the SoFCInteractiveElement to \a false and forces a redraw.
 */
void View3DInventorViewer::interactionFinishCB(void*, SoQTQuarterAdaptor* viewer)
{
    SoGLRenderAction* glra = viewer->getSoRenderManager()->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), false);
    viewer->redraw();
}

/**
 * Logs the type of the action that traverses the Inventor tree.
 */
void View3DInventorViewer::interactionLoggerCB(void*, SoAction* action)
{
    Base::Console().Log("%s\n", action->getTypeId().getName().getString());
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
    for (std::list<GLGraphicsItem*>::const_iterator it = this->graphicsItems.begin(); it != this->graphicsItems.end(); ++it) {
        if ((*it)->isDerivedFrom(type))
            items.push_back(*it);
    }

    return items;
}

void View3DInventorViewer::clearGraphicsItems()
{
    this->graphicsItems.clear();
}

int View3DInventorViewer::getNumSamples()
{
    int samples = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetInt("AntiAliasing", 0);

    switch (samples) {
    case View3DInventorViewer::MSAA2x:
        return 2;
    case View3DInventorViewer::MSAA4x:
        return 4;
    case View3DInventorViewer::MSAA8x:
        return 8;
    case View3DInventorViewer::Smoothing:
        return 1;
    default:
        return 0;
    }
}

void View3DInventorViewer::setRenderType(const RenderType type)
{
    renderType = type;

    glImage = QImage();
    if (type != Framebuffer) {
        delete framebuffer;
        framebuffer = 0;
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

            QtGLWidget* gl = static_cast<QtGLWidget*>(this->viewport());
            gl->makeCurrent();
#if !defined(HAVE_QT5_OPENGL)
            framebuffer = new QtGLFramebufferObject(width, height, QtGLFramebufferObject::Depth);
            renderToFramebuffer(framebuffer);
#else
            QOpenGLFramebufferObjectFormat fboFormat;
            fboFormat.setSamples(getNumSamples());
            fboFormat.setAttachment(QtGLFramebufferObject::Depth);
            QtGLFramebufferObject* fbo = new QtGLFramebufferObject(width, height, fboFormat);
            if (fbo->format().samples() > 0) {
                renderToFramebuffer(fbo);
                framebuffer = new QtGLFramebufferObject(fbo->size());
                // this is needed to be able to render the texture later
                QOpenGLFramebufferObject::blitFramebuffer(framebuffer, fbo);
                delete fbo;
            }
            else {
                renderToFramebuffer(fbo);
                framebuffer = fbo;
            }
#endif
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
    QtGLWidget* gl = static_cast<QtGLWidget*>(this->viewport());
    gl->makeCurrent();

    QImage res;
#if !defined(HAVE_QT5_OPENGL)
    int w = gl->width();
    int h = gl->height();
    QImage img(QSize(w,h), QImage::Format_RGB32);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    res = img;
#else
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
        fboFormat.setInternalTextureFormat(GL_RGB32F_ARB);

        QOpenGLFramebufferObject fbo(width, height, fboFormat);
        renderToFramebuffer(&fbo);

        res = fbo.toImage(false);
    }
#endif

    return res;
}

void View3DInventorViewer::imageFromFramebuffer(int width, int height, int samples,
                                                const QColor& bgcolor, QImage& img)
{
    QtGLWidget* gl = static_cast<QtGLWidget*>(this->viewport());
    gl->makeCurrent();

    const QtGLContext* context = QtGLContext::currentContext();
    if (!context) {
        Base::Console().Warning("imageFromFramebuffer failed because no context is active\n");
        return;
    }

    QtGLFramebufferObjectFormat fboFormat;
    fboFormat.setSamples(samples);
    fboFormat.setAttachment(QtGLFramebufferObject::Depth);
    // With enabled alpha a transparent background is supported but
    // at the same time breaks semi-transparent models. A workaround
    // is to use a certain background color using GL_RGB as texture
    // format and in the output image search for the above color and
    // replaces it with the color requested by the user.
#if defined(HAVE_QT5_OPENGL)
    if (App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetBool("SaveThumbnailFix",false)) {
        fboFormat.setInternalTextureFormat(GL_RGBA32F_ARB);
    }
    else {
        fboFormat.setInternalTextureFormat(GL_RGB32F_ARB);
    }
#else
    //fboFormat.setInternalTextureFormat(GL_RGBA);
    fboFormat.setInternalTextureFormat(GL_RGB);
#endif
    QtGLFramebufferObject fbo(width, height, fboFormat);

    const QColor col = backgroundColor();
    bool on = hasGradientBackground();

    int alpha = 255;
    QColor bgopaque = bgcolor;
    if (bgopaque.isValid()) {
        // force an opaque background color
        alpha = bgopaque.alpha();
        if (alpha < 255)
            bgopaque.setRgb(255,255,255);
        setBackgroundColor(bgopaque);
        setGradientBackground(false);
    }

    renderToFramebuffer(&fbo);
    setBackgroundColor(col);
    setGradientBackground(on);
    img = fbo.toImage();

    // if background color isn't opaque manipulate the image
    if (alpha < 255) {
        QImage image(img.constBits(), img.width(), img.height(), QImage::Format_ARGB32);
        img = image.copy();
        QRgb rgba = bgcolor.rgba();
        QRgb rgb = bgopaque.rgb();
        QRgb * bits = (QRgb*) img.bits();
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                if (*bits == rgb)
                    *bits = rgba;
                bits++;
            }
        }
    }
}

void View3DInventorViewer::renderToFramebuffer(QtGLFramebufferObject* fbo)
{
    static_cast<QtGLWidget*>(this->viewport())->makeCurrent();
    fbo->bind();
    int width = fbo->size().width();
    int height = fbo->size().height();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    const QColor col = this->backgroundColor();
    glViewport(0, 0, width, height);
    glClearColor(col.redF(), col.greenF(), col.blueF(), col.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // If on then transparent areas may shine through opaque areas
    //glDepthRange(0.1,1.0);

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
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, 1.0f);
    glEnd();

    printDimension();
    navigation->redraw();

    for (std::list<GLGraphicsItem*>::iterator it = this->graphicsItems.begin(); it != this->graphicsItems.end(); ++it)
        (*it)->paintGL();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

void View3DInventorViewer::renderGLImage()
{
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s size = vp.getViewportSizePixels();

    glDisable(GL_LIGHTING);
    glViewport(0, 0, size[0], size[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, size[0], 0, size[1], 0, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT);

    glRasterPos2f(0,0);
#if !defined(HAVE_QT5_OPENGL)
    glDrawPixels(glImage.width(),glImage.height(),GL_RGBA,GL_UNSIGNED_BYTE,glImage.bits());
#else
    glDrawPixels(glImage.width(),glImage.height(),GL_BGRA,GL_UNSIGNED_BYTE,glImage.bits());
#endif

    printDimension();
    navigation->redraw();

    for (std::list<GLGraphicsItem*>::iterator it = this->graphicsItems.begin(); it != this->graphicsItems.end(); ++it)
        (*it)->paintGL();

    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
}

// #define ENABLE_GL_DEPTH_RANGE
// The calls of glDepthRange inside renderScene() causes problems with transparent objects
// so that's why it is disabled now: http://forum.freecadweb.org/viewtopic.php?f=3&t=6037&hilit=transparency

// Documented in superclass. Overrides this method to be able to draw
// the axis cross, if selected, and to keep a continuous animation
// upon spin.
void View3DInventorViewer::renderScene(void)
{
    // Must set up the OpenGL viewport manually, as upon resize
    // operations, Coin won't set it up until the SoGLRenderAction is
    // applied again. And since we need to do glClear() before applying
    // the action..
    const SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SbVec2s origin = vp.getViewportOriginPixels();
    SbVec2s size = vp.getViewportSizePixels();
    glViewport(origin[0], origin[1], size[0], size[1]);

    const QColor col = this->backgroundColor();
    glClearColor(col.redF(), col.greenF(), col.blueF(), 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

#if defined(ENABLE_GL_DEPTH_RANGE)
    // using 90% of the z-buffer for the background and the main node
    glDepthRange(0.1,1.0);
#endif

    // Render our scenegraph with the image.
    SoGLRenderAction* glra = this->getSoRenderManager()->getGLRenderAction();
    SoState* state = glra->getState();
    SoGLWidgetElement::set(state, qobject_cast<QtGLWidget*>(this->getGLWidget()));
    SoGLRenderActionElement::set(state, glra);
    SoGLVBOActivatedElement::set(state, this->vboEnabled);
    glra->apply(this->backgroundroot);

    navigation->updateAnimation();

    if (!this->shading) {
        state->push();
        SoLightModelElement::set(state, selectionRoot, SoLightModelElement::BASE_COLOR);
        SoOverrideElement::setLightModelOverride(state, selectionRoot, true);
    }

    try {
        // Render normal scenegraph.
        inherited::actualRedraw();
    }
    catch (const Base::MemoryException&) {
        // FIXME: If this exception appears then the background and camera position get broken somehow. (Werner 2006-02-01)
        for (std::set<ViewProvider*>::iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); ++it)
            (*it)->hide();

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
    glra->apply(this->foregroundroot);

    if (this->axiscrossEnabled) {
        this->drawAxisCross();
    }

#if defined (ENABLE_GL_DEPTH_RANGE)
    // using the main portion of z-buffer again (for frontbuffer highlighting)
    glDepthRange(0.1,1.0);
#endif

    // Immediately reschedule to get continuous spin animation.
    if (this->isAnimating()) {
        this->getSoRenderManager()->scheduleRedraw();
    }

#if 0 // this breaks highlighting of edges
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
#endif

    printDimension();
    navigation->redraw();

    for (std::list<GLGraphicsItem*>::iterator it = this->graphicsItems.begin(); it != this->graphicsItems.end(); ++it)
        (*it)->paintGL();

    //fps rendering
    if (fpsEnabled) {
        std::stringstream stream;
        stream.precision(1);
        stream.setf(std::ios::fixed | std::ios::showpoint);
        stream << framesPerSecond[0] << " ms / " << framesPerSecond[1] << " fps";
        draw2DString(stream.str().c_str(), SbVec2s(10,10), SbVec2f(0.1f,0.1f));
    }

    if (naviCubeEnabled)
        naviCube->drawNaviCube();

#if 0 // this breaks highlighting of edges
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
#endif
}

void View3DInventorViewer::setSeekMode(SbBool on)
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

void View3DInventorViewer::printDimension()
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) return; // no camera there

    SoType t = getSoRenderManager()->getCamera()->getTypeId();
    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {
        const SbViewportRegion& vp = getSoRenderManager()->getViewportRegion();
        const SbVec2s& size = vp.getWindowSize();
        short dimX, dimY;
        size.getValue(dimX, dimY);

        float fHeight = static_cast<SoOrthographicCamera*>(getSoRenderManager()->getCamera())->height.getValue();
        float fWidth = fHeight;

        if (dimX > dimY)
            fWidth *= ((float)dimX)/((float)dimY);
        else if (dimX < dimY)
            fHeight *= ((float)dimY)/((float)dimX);

        // Translate screen units into user's unit schema
        Base::Quantity qWidth(Base::Quantity::MilliMetre);
        Base::Quantity qHeight(Base::Quantity::MilliMetre);
        qWidth.setValue(fWidth);
        qHeight.setValue(fHeight);
        QString wStr = Base::UnitsApi::schemaTranslate(qWidth);
        QString hStr = Base::UnitsApi::schemaTranslate(qHeight);

        // Create final string and update window
        QString dim = QString::fromLatin1("%1 x %2")
                      .arg(wStr, hStr);
        getMainWindow()->setPaneText(2, dim);
    }
    else
        getMainWindow()->setPaneText(2, QLatin1String(""));
}

void View3DInventorViewer::selectAll()
{
    std::vector<App::DocumentObject*> objs;

    for (std::set<ViewProvider*>::iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
            ViewProviderDocumentObject* vp = static_cast<ViewProviderDocumentObject*>(*it);
            App::DocumentObject* obj = vp->getObject();

            if (obj) objs.push_back(obj);
        }
    }

    if (!objs.empty())
        Gui::Selection().setSelection(objs.front()->getDocument()->getName(), objs);
}

bool View3DInventorViewer::processSoEvent(const SoEvent* ev)
{
    if (naviCubeEnabled && naviCube->processSoEvent(ev))
        return true;
    if (isRedirectedToSceneGraph()) {
        SbBool processed = inherited::processSoEvent(ev);

        if (!processed)
            processed = navigation->processEvent(ev);

        return processed;
    }

    if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        // filter out 'Q' and 'ESC' keys
        const SoKeyboardEvent* const ke = static_cast<const SoKeyboardEvent*>(ev);

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

SbBool View3DInventorViewer::processSoEventBase(const SoEvent* const ev)
{
    return inherited::processSoEvent(ev);
}

SbVec3f View3DInventorViewer::getViewDirection() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) return SbVec3f(0,0,-1);  // this is the default

    SbVec3f projDir = cam->getViewVolume().getProjectionDirection();
    return projDir;
}

void View3DInventorViewer::setViewDirection(SbVec3f dir)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (cam)
        cam->orientation.setValue(SbRotation(SbVec3f(0, 0, -1), dir));
}

SbVec3f View3DInventorViewer::getUpDirection() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) return SbVec3f(0,1,0);

    SbRotation camrot = cam->orientation.getValue();
    SbVec3f upvec(0, 1, 0); // init to default up vector
    camrot.multVec(upvec, upvec);
    return upvec;
}

SbRotation View3DInventorViewer::getCameraOrientation() const
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam)
        return SbRotation(0,0,0,1); // this is the default

    return cam->orientation.getValue();
}

SbVec3f View3DInventorViewer::getPointOnScreen(const SbVec2s& pnt) const
{
    const SbViewportRegion& vp = this->getSoRenderManager()->getViewportRegion();

    short x,y;
    pnt.getValue(x,y);
    SbVec2f siz = vp.getViewportSize();
    float dX, dY;
    siz.getValue(dX, dY);

    float fRatio = vp.getViewportAspectRatio();
    float pX = (float)x / float(vp.getViewportSizePixels()[0]);
    float pY = (float)y / float(vp.getViewportSizePixels()[1]);

    // now calculate the real points respecting aspect ratio information
    //
    if (fRatio > 1.0f) {
        pX = (pX - 0.5f*dX) * fRatio + 0.5f*dX;
    }
    else if (fRatio < 1.0f) {
        pY = (pY - 0.5f*dY) / fRatio + 0.5f*dY;
    }

    SoCamera* pCam = this->getSoRenderManager()->getCamera();

    if (!pCam) return SbVec3f();  // return invalid point

    SbViewVolume  vol = pCam->getViewVolume();

    float nearDist = pCam->nearDistance.getValue();
    float farDist = pCam->farDistance.getValue();
    float focalDist = pCam->focalDistance.getValue();

    if (focalDist < nearDist || focalDist > farDist)
        focalDist = 0.5f*(nearDist + farDist);

    SbLine line;
    SbVec3f pt;
    SbPlane focalPlane = vol.getPlane(focalDist);
    vol.projectPointToLine(SbVec2f(pX,pY), line);
    focalPlane.intersect(line, pt);

    return pt;
}

void View3DInventorViewer::getNearPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getSoRenderManager()->getCamera();

    if (!pCam) return;  // just do nothing

    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the front clipping plane
    SbPlane nearPlane = vol.getPlane(vol.nearDist);
    float d = nearPlane.getDistanceFromOrigin();
    rcNormal = nearPlane.getNormal();
    rcNormal.normalize();
    float nx, ny, nz;
    rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(d*rcNormal[0], d*rcNormal[1], d*rcNormal[2]);
}

void View3DInventorViewer::getFarPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getSoRenderManager()->getCamera();

    if (!pCam) return;  // just do nothing

    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the back clipping plane
    SbPlane farPlane = vol.getPlane(vol.nearDist+vol.nearToFar);
    float d = farPlane.getDistanceFromOrigin();
    rcNormal = farPlane.getNormal();
    rcNormal.normalize();
    float nx, ny, nz;
    rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(d*rcNormal[0], d*rcNormal[1], d*rcNormal[2]);
}

SbVec3f View3DInventorViewer::projectOnNearPlane(const SbVec2f& pt) const
{
    SbVec3f pt1, pt2;
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) return SbVec3f();  // return invalid point

    SbViewVolume vol = cam->getViewVolume();
    vol.projectPointToLine(pt, pt1, pt2);
    return pt1;
}

SbVec3f View3DInventorViewer::projectOnFarPlane(const SbVec2f& pt) const
{
    SbVec3f pt1, pt2;
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) return SbVec3f();  // return invalid point

    SbViewVolume vol = cam->getViewVolume();
    vol.projectPointToLine(pt, pt1, pt2);
    return pt2;
}

void View3DInventorViewer::toggleClippingPlane()
{
    if (pcViewProviderRoot->getNumChildren() > 0 &&
        pcViewProviderRoot->getChild(0)->getTypeId() ==
        SoClipPlaneManip::getClassTypeId()) {
        pcViewProviderRoot->removeChild(0);
    }
    else {
        SoClipPlaneManip* clip = new SoClipPlaneManip;
        SoGetBoundingBoxAction action(this->getSoRenderManager()->getViewportRegion());
        action.apply(this->getSoRenderManager()->getSceneGraph());
        SbBox3f box = action.getBoundingBox();

        if (!box.isEmpty()) {
            // adjust to overall bounding box of the scene
            clip->setValue(box, SbVec3f(0.0f,0.0f,1.0f), 1.0f);
        }

        pcViewProviderRoot->insertChild(clip,0);
    }
}

bool View3DInventorViewer::hasClippingPlane() const
{
    if (pcViewProviderRoot && pcViewProviderRoot->getNumChildren() > 0) {
        return (pcViewProviderRoot->getChild(0)->getTypeId()
                == SoClipPlaneManip::getClassTypeId());
    }

    return false;
}

/**
 * This method picks the closest point to the camera in the underlying scenegraph
 * and returns its location and normal.
 * If no point was picked false is returned.
 */
bool View3DInventorViewer::pickPoint(const SbVec2s& pos,SbVec3f& point,SbVec3f& norm) const
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
    return (pick ? new SoPickedPoint(*pick) : 0);
}

const SoPickedPoint* View3DInventorViewer::getPickedPoint(SoEventCallback* n) const
{
    if (selectionRoot)
        return selectionRoot->getPickedPoint(n->getAction());
    else
        return n->getPickedPoint();
}

SbBool View3DInventorViewer::pubSeekToPoint(const SbVec2s& pos)
{
    return this->seekToPoint(pos);
}

void View3DInventorViewer::pubSeekToPoint(const SbVec3f& pos)
{
    this->seekToPoint(pos);
}

void View3DInventorViewer::setCameraOrientation(const SbRotation& rot, SbBool moveTocenter)
{
    navigation->setCameraOrientation(rot, moveTocenter);
}

void View3DInventorViewer::setCameraType(SoType t)
{
    inherited::setCameraType(t);

    if (t.isDerivedFrom(SoPerspectiveCamera::getClassTypeId())) {
        // When doing a viewAll() for an orthographic camera and switching
        // to perspective the scene looks completely strange because of the
        // heightAngle. Setting it to 45 deg also causes an issue with a too
        // close camera but we don't have this other ugly effect.
        SoCamera* cam = this->getSoRenderManager()->getCamera();

        if(cam == 0) return;

        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(M_PI / 4.0);
    }
}

namespace Gui {
    class CameraAnimation : public QVariantAnimation
    {
        SoCamera* camera;
        SbRotation startRot, endRot;
        SbVec3f startPos, endPos;

    public:
        CameraAnimation(SoCamera* camera, const SbRotation& rot, const SbVec3f& pos)
            : camera(camera), endRot(rot), endPos(pos)
        {
            startPos = camera->position.getValue();
            startRot = camera->orientation.getValue();
        }
        virtual ~CameraAnimation()
        {
        }
    protected:
        void updateCurrentValue(const QVariant & value)
        {
            int steps = endValue().toInt();
            int curr = value.toInt();

            float s = static_cast<float>(curr)/static_cast<float>(steps);
            SbVec3f curpos = startPos * (1.0f-s) + endPos * s;
            SbRotation currot = SbRotation::slerp(startRot, endRot, s);
            camera->orientation.setValue(currot);
            camera->position.setValue(curpos);
        }
    };
}

void View3DInventorViewer::moveCameraTo(const SbRotation& rot, const SbVec3f& pos, int steps, int ms)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (cam == 0) return;

    CameraAnimation anim(cam, rot, pos);
    anim.setDuration(Base::clamp<int>(ms,0,5000));
    anim.setStartValue(static_cast<int>(0));
    anim.setEndValue(steps);

    QEventLoop loop;
    QObject::connect(&anim, SIGNAL(finished()), &loop, SLOT(quit()));
    anim.start();
    loop.exec(QEventLoop::ExcludeUserInputEvents);

    cam->orientation.setValue(rot);
    cam->position.setValue(pos);
}

void View3DInventorViewer::animatedViewAll(int steps, int ms)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (!cam)
        return;

    SbVec3f campos = cam->position.getValue();
    SbRotation camrot = cam->orientation.getValue();
    SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SoGetBoundingBoxAction action(vp);
    action.apply(this->getSoRenderManager()->getSceneGraph());
    SbBox3f box = action.getBoundingBox();

#if (COIN_MAJOR_VERSION >= 3)
    float aspectRatio = vp.getViewportAspectRatio();
#endif

    if (box.isEmpty())
        return;

    SbSphere sphere;
    sphere.circumscribe(box);
    if (sphere.getRadius() == 0)
        return;

    SbVec3f direction, pos;
    camrot.multVec(SbVec3f(0, 0, -1), direction);

    bool isOrthographic = false;
    float height = 0;
    float diff = 0;

    if (cam->isOfType(SoOrthographicCamera::getClassTypeId())) {
        isOrthographic = true;
        height = static_cast<SoOrthographicCamera*>(cam)->height.getValue();
#if (COIN_MAJOR_VERSION >= 3)
        if (aspectRatio < 1.0f)
            diff = sphere.getRadius() * 2 - height * aspectRatio;
        else
#endif
        diff = sphere.getRadius() * 2 - height;
        pos = (box.getCenter() - direction * sphere.getRadius());
    }
    else if (cam->isOfType(SoPerspectiveCamera::getClassTypeId())) {
        float movelength = sphere.getRadius()/float(tan(static_cast<SoPerspectiveCamera*>
            (cam)->heightAngle.getValue() / 2.0));
        pos = box.getCenter() - direction * movelength;
    }

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));

    for (int i=0; i<steps; i++) {
        float s = float(i)/float(steps);

        if (isOrthographic) {
            float camHeight = height + diff * s;
            static_cast<SoOrthographicCamera*>(cam)->height.setValue(camHeight);
        }

        SbVec3f curpos = campos * (1.0f-s) + pos * s;
        cam->position.setValue(curpos);
        timer.start(Base::clamp<int>(ms,0,5000));
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }
}

#if BUILD_VR
extern View3DInventorRiftViewer* oculusStart(void);
extern bool oculusUp   (void);
extern void oculusStop (void);
void oculusSetTestScene(View3DInventorRiftViewer *window);
#endif

void View3DInventorViewer::viewVR(void)
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

void View3DInventorViewer::viewAll()
{
    SbViewportRegion vp = this->getSoRenderManager()->getViewportRegion();
    SoGetBoundingBoxAction action(vp);
    action.apply(this->getSoRenderManager()->getSceneGraph());
    SbBox3f box = action.getBoundingBox();

    if (box.isEmpty())
        return;

    SbSphere sphere;
    sphere.circumscribe(box);
    if (sphere.getRadius() == 0)
        return;

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
        SoSkipBoundingGroup* group = static_cast<SoSkipBoundingGroup*>(path->getTail());
        group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
    }

    // Set the height angle to 45 deg
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (cam && cam->getTypeId().isDerivedFrom(SoPerspectiveCamera::getClassTypeId()))
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(M_PI / 4.0);

    if (isAnimationEnabled())
        animatedViewAll(10, 20);

    // make sure everything is visible
    if (cam)
        cam->viewAll(getSoRenderManager()->getSceneGraph(), this->getSoRenderManager()->getViewportRegion());

    for (int i = 0; i < pathlist.getLength(); i++) {
        SoPath* path = pathlist[i];
        SoSkipBoundingGroup* group = static_cast<SoSkipBoundingGroup*>(path->getTail());
        group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
    }
}

void View3DInventorViewer::viewAll(float factor)
{
    SoCamera* cam = this->getSoRenderManager()->getCamera();

    if (!cam) return;

    if (factor <= 0.0f) return;

    if (factor != 1.0f) {
        SoSearchAction sa;
        sa.setType(SoSkipBoundingGroup::getClassTypeId());
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(this->getSoRenderManager()->getSceneGraph());
        const SoPathList& pathlist = sa.getPaths();

        for (int i = 0; i < pathlist.getLength(); i++) {
            SoPath* path = pathlist[i];
            SoSkipBoundingGroup* group = static_cast<SoSkipBoundingGroup*>(path->getTail());
            group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
        }

        SoGetBoundingBoxAction action(this->getSoRenderManager()->getViewportRegion());
        action.apply(this->getSoRenderManager()->getSceneGraph());
        SbBox3f box = action.getBoundingBox();
        float minx,miny,minz,maxx,maxy,maxz;
        box.getBounds(minx,miny,minz,maxx,maxy,maxz);

        for (int i = 0; i < pathlist.getLength(); i++) {
            SoPath* path = pathlist[i];
            SoSkipBoundingGroup* group = static_cast<SoSkipBoundingGroup*>(path->getTail());
            group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
        }

        SoCube* cube = new SoCube();
        cube->width  = factor*(maxx-minx);
        cube->height = factor*(maxy-miny);
        cube->depth  = factor*(maxz-minz);

        // fake a scenegraph with the desired bounding size
        SoSeparator* graph = new SoSeparator();
        graph->ref();
        SoTranslation* tr = new SoTranslation();
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
#if 0
    // Search for all SoFCSelection nodes
    SoSearchAction searchAction;
    searchAction.setType(SoFCSelection::getClassTypeId());
    searchAction.setInterest(SoSearchAction::ALL);
    searchAction.apply(pcViewProviderRoot);

    SoPathList& paths = searchAction.getPaths();
    int countPaths = paths.getLength();

    SoGroup* root = new SoGroup();
    root->ref();

    for (int i=0; i<countPaths; i++) {
        SoPath* path = paths[i];
        SoNode* node = path->getTail();

        if (!node || node->getTypeId() != SoFCSelection::getClassTypeId())
            continue; // should not happen

        SoFCSelection* select = static_cast<SoFCSelection*>(node);

        // Check only document and object name but not sub-element name
        if (Selection().isSelected(select->documentName.getValue().getString(),
                                   select->objectName.getValue().getString())) {
            root->addChild(select);
        }
    }

#else
    SoGroup* root = new SoGroup();
    root->ref();

    std::vector<App::DocumentObject*> selection = Selection().getObjectsOfType(App::DocumentObject::getClassTypeId());
    for (std::vector<App::DocumentObject*>::iterator it = selection.begin(); it != selection.end(); ++it) {
        ViewProvider* vp = Application::Instance->getViewProvider(*it);
        if (vp) {
            root->addChild(vp->getRoot());
        }
    }

#endif

    SoCamera* cam = this->getSoRenderManager()->getCamera();
    if (cam)
        cam->viewAll(root, this->getSoRenderManager()->getViewportRegion());

    root->unref();
}

/*!
  Decide if it should be possible to start a spin animation of the
  model in the viewer by releasing the mouse button while dragging.

  If the \a enable flag is \c false and we're currently animating, the
  spin will be stopped.
*/
void
View3DInventorViewer::setAnimationEnabled(const SbBool enable)
{
    navigation->setAnimationEnabled(enable);
}

/*!
  Query whether or not it is possible to start a spinning animation by
  releasing the left mouse button while dragging the mouse.
*/

SbBool
View3DInventorViewer::isAnimationEnabled(void) const
{
    return navigation->isAnimationEnabled();
}

/*!
  Query if the model in the viewer is currently in spinning mode after
  a user drag.
*/
SbBool View3DInventorViewer::isAnimating(void) const
{
    return navigation->isAnimating();
}

/*!
 * Starts programmatically the viewer in animation mode. The given axis direction
 * is always in screen coordinates, not in world coordinates.
 */
void View3DInventorViewer::startAnimating(const SbVec3f& axis, float velocity)
{
    navigation->startAnimating(axis, velocity);
}

void View3DInventorViewer::stopAnimating(void)
{
    navigation->stopAnimating();
}

void View3DInventorViewer::setPopupMenuEnabled(const SbBool on)
{
    navigation->setPopupMenuEnabled(on);
}

SbBool View3DInventorViewer::isPopupMenuEnabled(void) const
{
    return navigation->isPopupMenuEnabled();
}

/*!
  Set the flag deciding whether or not to show the axis cross.
*/

void
View3DInventorViewer::setFeedbackVisibility(const SbBool enable)
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

SbBool
View3DInventorViewer::isFeedbackVisible(void) const
{
    return this->axiscrossEnabled;
}

/*!
  Set the size of the feedback axiscross.  The value is interpreted as
  an approximate percentage chunk of the dimensions of the total
  canvas.
*/
void
View3DInventorViewer::setFeedbackSize(const int size)
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
View3DInventorViewer::getFeedbackSize(void) const
{
    return this->axiscrossSize;
}

/*!
  Decide whether or not the mouse pointer cursor should be visible in
  the rendering canvas.
*/
void View3DInventorViewer::setCursorEnabled(SbBool /*enable*/)
{
    this->setCursorRepresentation(navigation->getViewingMode());
}

void View3DInventorViewer::afterRealizeHook(void)
{
    inherited::afterRealizeHook();
    this->setCursorRepresentation(navigation->getViewingMode());
}

// Documented in superclass. This method overridden from parent class
// to make sure the mouse pointer cursor is updated.
void View3DInventorViewer::setViewing(SbBool enable)
{
    if (this->isViewing() == enable) {
        return;
    }

    navigation->setViewingMode(enable ?
        NavigationStyle::IDLE : NavigationStyle::INTERACT);
    inherited::setViewing(enable);
}

//****************************************************************************

// Bitmap representations of an "X", a "Y" and a "Z" for the axis cross.
static GLubyte xbmp[] = { 0x11,0x11,0x0a,0x04,0x0a,0x11,0x11 };
static GLubyte ybmp[] = { 0x04,0x04,0x04,0x04,0x0a,0x11,0x11 };
static GLubyte zbmp[] = { 0x1f,0x10,0x08,0x04,0x02,0x01,0x1f };

void View3DInventorViewer::drawAxisCross(void)
{
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
    const int pixelarea =
        int(float(this->axiscrossSize)/100.0f * std::min(view[0], view[1]));
#if 0 // middle of canvas
    SbVec2s origin(view[0]/2 - pixelarea/2, view[1]/2 - pixelarea/2);
#endif // middle of canvas
#if 1 // lower right of canvas
    SbVec2s origin(view[0] - pixelarea, 0);
#endif // lower right of canvas
    glViewport(origin[0], origin[1], pixelarea, pixelarea);

    // Set up the projection matrix.
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    const float NEARVAL = 0.1f;
    const float FARVAL = 10.0f;
    const float dim = NEARVAL * float(tan(M_PI / 8.0)); // FOV is 45 deg (45/360 = 1/8)
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
    SbMatrix comb = mx.multRight(px);

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

        for (int i=0; i < 3; i++) {
            glPushMatrix();

            if (idx[i] == XAXIS) {                        // X axis.
                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
                    glColor3f(0.500f, 0.5f, 0.5f);
                else
                    glColor3f(0.500f, 0.125f, 0.125f);
            }
            else if (idx[i] == YAXIS) {                   // Y axis.
                glRotatef(90, 0, 0, 1);

                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
                    glColor3f(0.400f, 0.4f, 0.4f);
                else
                    glColor3f(0.125f, 0.500f, 0.125f);
            }
            else {                                        // Z axis.
                glRotatef(-90, 0, 1, 0);

                if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
                    glColor3f(0.300f, 0.3f, 0.3f);
                else
                    glColor3f(0.125f, 0.125f, 0.500f);
            }

            this->drawArrow();
            glPopMatrix();
        }
    }

    // Render axis notation letters ("X", "Y", "Z").
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, view[0], 0, view[1], -1, 1);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    GLint unpack;
    glGetIntegerv(GL_UNPACK_ALIGNMENT, &unpack);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    if (stereoMode() != Quarter::SoQTQuarterAdaptor::MONO)
        glColor3fv(SbVec3f(1.0f, 1.0f, 1.0f).getValue());
    else
        glColor3fv(SbVec3f(0.0f, 0.0f, 0.0f).getValue());

    glRasterPos2d(xpos[0], xpos[1]);
    glBitmap(8, 7, 0, 0, 0, 0, xbmp);
    glRasterPos2d(ypos[0], ypos[1]);
    glBitmap(8, 7, 0, 0, 0, 0, ybmp);
    glRasterPos2d(zpos[0], zpos[1]);
    glBitmap(8, 7, 0, 0, 0, 0, zbmp);

    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack);
    glPopMatrix();

    // Reset original state.

    // FIXME: are these 3 lines really necessary, as we push
    // GL_ALL_ATTRIB_BITS at the start? 20000604 mortene.
    glDepthRange(depthrange[0], depthrange[1]);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixd(projectionmatrix);

    glPopAttrib();
}

// Draw an arrow for the axis representation directly through OpenGL.
void View3DInventorViewer::drawArrow(void)
{
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glEnd();
    glDisable(GL_CULL_FACE);
    glBegin(GL_TRIANGLES);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, +0.5f / 4.0f, 0.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, -0.5f / 4.0f, 0.0f);
    glVertex3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, 0.0f, +0.5f / 4.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, 0.0f, -0.5f / 4.0f);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(1.0f - 1.0f / 3.0f, +0.5f / 4.0f, 0.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, 0.0f, +0.5f / 4.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, -0.5f / 4.0f, 0.0f);
    glVertex3f(1.0f - 1.0f / 3.0f, 0.0f, -0.5f / 4.0f);
    glEnd();
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
    if (glWindow)
        glWindow->setAttribute(Qt::WA_SetCursor, false);

    if (glWindow && glWindow->rect().contains(QCursor::pos()))
        glWindow->setAttribute(Qt::WA_UnderMouse);

    switch (modearg) {
    case NavigationStyle::IDLE:
    case NavigationStyle::INTERACT:
        if (isEditing())
            this->getWidget()->setCursor(this->editCursor);
        else
            this->getWidget()->setCursor(QCursor(Qt::ArrowCursor));
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

void View3DInventorViewer::setEditing(SbBool edit)
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
    }
}

void View3DInventorViewer::deselectCB(void* viewer, SoPath* path)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(path);
    if (vp && vp->useNewSelectionModel()) {
    }
}

SoPath* View3DInventorViewer::pickFilterCB(void* viewer, const SoPickedPoint* pp)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(pp->getPath());
    if (vp && vp->useNewSelectionModel()) {
        std::string e = vp->getElement(pp->getDetail());
        vp->getSelectionShape(e.c_str());
        static char buf[513];
        snprintf(buf,512,"Hovered: %s (%f,%f,%f)"
                 ,e.c_str()
                 ,pp->getPoint()[0]
                 ,pp->getPoint()[1]
                 ,pp->getPoint()[2]);

        getMainWindow()->showMessage(QString::fromLatin1(buf),3000);
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
    // FIXME Use the viewprovider map introduced for the selection
    for (std::set<ViewProvider*>::const_iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); ++it) {
        for (int i = 0; i<path->getLength(); i++) {
            SoNode* node = path->getNode(i);
            if ((*it)->getRoot() == node) {
                return (*it);
            }
        }
    }

    return 0;
}

ViewProvider* View3DInventorViewer::getViewProviderByPathFromTail(SoPath* path) const
{
    // Make sure I'm the lowest LocHL in the pick path!
    for (int i = 0; i < path->getLength(); i++) {
        SoNode* node = path->getNodeFromTail(i);

        if (node->isOfType(SoSeparator::getClassTypeId())) {
            std::map<SoSeparator*,ViewProvider*>::const_iterator it = _ViewProviderMap.find(static_cast<SoSeparator*>(node));
            if (it != _ViewProviderMap.end()) {
                return it->second;
            }
        }
    }

    return 0;
}

std::vector<ViewProvider*> View3DInventorViewer::getViewProvidersOfType(const Base::Type& typeId) const
{
    std::vector<ViewProvider*> views;
    for (std::set<ViewProvider*>::const_iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(typeId)) {
            views.push_back(*it);
        }
    }

    return views;
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
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->removeAllChildren();
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->removeAllChildren();
}

void View3DInventorViewer::turn3dDimensionsOn()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_ALL;
}

void View3DInventorViewer::turn3dDimensionsOff()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_NONE;
}

void View3DInventorViewer::addDimension3d(SoNode* node)
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(0))->addChild(node);
}

void View3DInventorViewer::addDimensionDelta(SoNode* node)
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->addChild(node);
}

void View3DInventorViewer::turnDeltaDimensionsOn()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_ALL;
}

void View3DInventorViewer::turnDeltaDimensionsOff()
{
    static_cast<SoSwitch*>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_NONE;
}

PyObject *View3DInventorViewer::getPyObject(void)
{
    if (!_viewerPy)
        _viewerPy = new View3DInventorViewerPy(this);

    Py_INCREF(_viewerPy);
    return _viewerPy;
}

/**
 * Drops the event \a e and loads the files into the given document.
 */
void View3DInventorViewer::dropEvent (QDropEvent * e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls() && selectionRoot && selectionRoot->pcDocument) {
        getMainWindow()->loadUrls(selectionRoot->pcDocument->getDocument(), data->urls());
    }
    else {
        inherited::dropEvent(e);
    }
}

void View3DInventorViewer::dragEnterEvent (QDragEnterEvent * e)
{
    // Here we must allow uri drags and check them in dropEvent
    const QMimeData* data = e->mimeData();
    if (data->hasUrls()) {
        e->accept();
    }
    else {
        inherited::dragEnterEvent(e);
    }
}

void View3DInventorViewer::dragMoveEvent(QDragMoveEvent *e)
{
    const QMimeData* data = e->mimeData();
    if (data->hasUrls() && selectionRoot && selectionRoot->pcDocument) {
        e->accept();
    }
    else {
        inherited::dragMoveEvent(e);
    }
}

void View3DInventorViewer::dragLeaveEvent(QDragLeaveEvent *e)
{
    inherited::dragLeaveEvent(e);
}
