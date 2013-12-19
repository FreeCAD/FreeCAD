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
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/actions/SoHandleEventAction.h> 
# include <Inventor/actions/SoToVRML2Action.h>
# include <Inventor/actions/SoWriteAction.h>
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
# include <Inventor/Qt/SoQtBasic.h>
# include <QEventLoop>
# include <QGLFramebufferObject>
# include <QKeyEvent>
# include <QWheelEvent>
# include <QMessageBox>
# include <QTimer>
# include <QStatusBar>
#endif

#include <sstream>
#include <Base/Console.h>
#include <Base/Stream.h>
#include <Base/FileInfo.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <zipios++/gzipoutputstream.h>

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

#include <Inventor/draggers/SoCenterballDragger.h>


//#define FC_LOGGING_CB

#define new DEBUG_CLIENTBLOCK

using namespace Gui;

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

SOQT_OBJECT_ABSTRACT_SOURCE(View3DInventorViewer);

// *************************************************************************

View3DInventorViewer::View3DInventorViewer (QWidget *parent, const char *name, 
                                            SbBool embed, Type type, SbBool build) 
  : inherited (parent, name, embed, type, build), editViewProvider(0), navigation(0),
    framebuffer(0), editing(FALSE), redirected(FALSE), allowredir(FALSE),axisCross(0),axisGroup(0)
{
    Gui::Selection().Attach(this);

    // Coin should not clear the pixel-buffer, so the background image
    // is not removed.
    this->setClearBeforeRender(FALSE);

    // setting up the defaults for the spin rotation
    initialize();

    SoOrthographicCamera * cam = new SoOrthographicCamera;
    cam->position = SbVec3f(0, 0, 1);
    cam->height = 1;
    cam->nearDistance = 0.5;
    cam->farDistance = 1.5;

    // setup light sources
    SoDirectionalLight *hl = this->getHeadlight();
    backlight = new SoDirectionalLight();
    backlight->ref();
    backlight->setName("soqt->backlight");
    backlight->direction.setValue(-hl->direction.getValue());
    backlight->on.setValue(FALSE); // by default off

    // Set up background scenegraph with image in it.
    backgroundroot = new SoSeparator;
    backgroundroot->ref();
    this->backgroundroot->addChild(cam);

    //SoShapeHints* pShapeHints = new SoShapeHints;
    //pShapeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    //pShapeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    //pShapeHints->ref();
    //this->backgroundroot->addChild(pShapeHints);

    //SoLightModel* pcLightModel = new SoLightModel();
    ////pcLightModel->model = SoLightModel::PHONG;
    //pcLightModel->model = SoLightModel::BASE_COLOR;
    //this->backgroundroot->addChild(pcLightModel);


    // Background stuff
    pcBackGround = new SoFCBackgroundGradient;
    pcBackGround->ref();

    // Set up foreground, overlayed scenegraph.
    this->foregroundroot = new SoSeparator;
    this->foregroundroot->ref();

    SoLightModel * lm = new SoLightModel;
    lm->model = SoLightModel::BASE_COLOR;

    SoBaseColor * bc = new SoBaseColor;
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
    SoCallback * cb = new SoCallback;	 	
    cb->setCallback(interactionLoggerCB, this);
    pcViewProviderRoot->addChild(cb);
#endif

    // Set our own render action which show a bounding box if
    // the SoFCSelection::BOX style is set
    this->setGLRenderAction(new SoBoxSelectionRenderAction);

    // set the transperency and antialiasing settings
//  getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_BLEND);
    getGLRenderAction()->setTransparencyType(SoGLRenderAction::SORTED_OBJECT_SORTED_TRIANGLE_BLEND);
//  getGLRenderAction()->setSmoothing(true);

    // Settings
    setSeekTime(0.4f);
    if (isSeekValuePercentage() == false)
        setSeekValueAsPercentage(true);
    setSeekDistance(100);
    setViewing(false);

    setBackgroundColor(SbColor(0.1f, 0.1f, 0.1f));
    setGradientBackground(true);

    // set some callback functions for user interaction
    addStartCallback(interactionStartCB);
    addFinishCallback(interactionFinishCB);
}

View3DInventorViewer::~View3DInventorViewer()
{
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
    this->pcViewProviderRoot->unref();
    this->pcViewProviderRoot = 0;
    this->backlight->unref();
    this->backlight = 0;

    delete this->navigation;

    // Note: When closing the application the main window doesn't exist any more.
    if (getMainWindow())
        getMainWindow()->setPaneText(2, QLatin1String(""));
    Gui::Selection().Detach(this);
}

void View3DInventorViewer::setDocument(Gui::Document *pcDocument)
{
    // write the document the viewer belongs to to the selection node
    selectionRoot->pcDocument = pcDocument;
}

void View3DInventorViewer::initialize()
{
    navigation = new CADNavigationStyle();
    navigation->setViewer(this);

    this->axiscrossEnabled = TRUE;
    this->axiscrossSize = 10;
}

/// @cond DOXERR
void View3DInventorViewer::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                                    Gui::SelectionSingleton::MessageType Reason)
{
    if (Reason.Type == SelectionChanges::AddSelection ||
        Reason.Type == SelectionChanges::RmvSelection ||
        Reason.Type == SelectionChanges::SetSelection ||
        Reason.Type == SelectionChanges::ClrSelection) {
        SoFCSelectionAction cAct(Reason);
        cAct.apply(pcViewProviderRoot);
    }
}
/// @endcond

SbBool View3DInventorViewer::hasViewProvider(ViewProvider* pcProvider) const
{
    return _ViewProviderSet.find(pcProvider) != _ViewProviderSet.end();
}

/// adds an ViewProvider to the view, e.g. from a feature
void View3DInventorViewer::addViewProvider(ViewProvider* pcProvider)
{
    SoSeparator* root = pcProvider->getRoot();
    if (root){
        pcViewProviderRoot->addChild(root);
        _ViewProviderMap[root] = pcProvider;
    }
    SoSeparator* fore = pcProvider->getFrontRoot();
    if (fore) foregroundroot->addChild(fore);
    SoSeparator* back = pcProvider->getBackRoot ();
    if (back) backgroundroot->addChild(back);

    _ViewProviderSet.insert(pcProvider);
}

void View3DInventorViewer::removeViewProvider(ViewProvider* pcProvider)
{
    if (this->editViewProvider == pcProvider)
        resetEditingViewProvider();

    SoSeparator* root = pcProvider->getRoot();
    if (root){
        pcViewProviderRoot->removeChild(root);
        _ViewProviderMap.erase(root);
    }
    SoSeparator* fore = pcProvider->getFrontRoot();
    if (fore) foregroundroot->removeChild(fore);
    SoSeparator* back = pcProvider->getBackRoot ();
    if (back) backgroundroot->removeChild(back);
  
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
        this->editViewProvider->unsetEditViewer(this);
        this->editViewProvider->finishEditing();
        removeEventCallback(SoEvent::getClassTypeId(), Gui::ViewProvider::eventCallback,this->editViewProvider);
        this->editViewProvider = 0;
    }
}

/// reset from edit mode
SbBool View3DInventorViewer::isEditingViewProvider() const
{
    return this->editViewProvider ? true : false;
}

void View3DInventorViewer::clearBuffer(void * userdata, SoAction * action)
{
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        // do stuff specific for GL rendering here.
        glClear(GL_DEPTH_BUFFER_BIT);
    }
}

void View3DInventorViewer::setGLWidget(void * userdata, SoAction * action)
{
    //FIXME: This causes the Coin error message:
    // Coin error in SoNode::GLRenderS(): GL error: 'GL_STACK_UNDERFLOW', nodetype:
    // Separator (set envvar COIN_GLERROR_DEBUGGING=1 and re-run to get more information)
    if (action->isOfType(SoGLRenderAction::getClassTypeId())) {
        QWidget* gl = reinterpret_cast<QWidget*>(userdata);
        SoGLWidgetElement::set(action->getState(), qobject_cast<QGLWidget*>(gl));
    }
}

void View3DInventorViewer::handleEventCB(void * ud, SoEventCallback * n)
{
    View3DInventorViewer* that = reinterpret_cast<View3DInventorViewer*>(ud);
    SoGLRenderAction * glra = that->getGLRenderAction();
    SoAction* action = n->getAction();
    SoGLRenderActionElement::set(action->getState(), glra);
    SoGLWidgetElement::set(action->getState(), qobject_cast<QGLWidget*>(that->getGLWidget()));
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
#if defined (FC_OS_LINUX) || defined(FC_OS_CYGWIN) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD)
    setenv("COIN_SHOW_FPS_COUNTER", (on?"1":"0"), 1);
#else
    on ? _putenv ("COIN_SHOW_FPS_COUNTER=1") : _putenv ("COIN_SHOW_FPS_COUNTER=0");
#endif
}

void View3DInventorViewer::setAxisCross(bool b)
{
    SoNode* scene = getSceneGraph();
    SoSeparator* sep = static_cast<SoSeparator*>(scene);

    if(b){
        if(!axisGroup){
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
    }else{
        if(axisGroup){
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
    if (this->navigation && this->navigation->getTypeId() == t)
        return; // nothing to do
    Base::BaseClass* base = static_cast<Base::BaseClass*>(t.createInstance());
    if (!base)
        return;
    if (!base->getTypeId().isDerivedFrom(NavigationStyle::getClassTypeId())) {
        delete base;
#if SOQT_DEBUG
        SoDebugError::postWarning("View3DInventorViewer::setNavigationType",
                                  "Navigation object must be of type NavigationStyle.");
#endif // SO@GUI_DEBUG 
        return;
    }

    NavigationStyle* ns = static_cast<NavigationStyle*>(base);
    ns->operator = (*this->navigation);
    delete this->navigation;
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

void View3DInventorViewer::setSceneGraph (SoNode *root)
{
    inherited::setSceneGraph(root);

    SoSearchAction sa;
    sa.setNode(this->backlight);
    SoNode* scene = this->getSceneManager()->getSceneGraph();
    if (scene && scene->getTypeId().isDerivedFrom(SoSeparator::getClassTypeId())) {
        sa.apply(scene);
        if (!sa.getPath())
            static_cast<SoSeparator*>(scene)->insertChild(this->backlight, 1);
    }
}

void View3DInventorViewer::savePicture(int w, int h, int eBackgroundType, QImage& img) const
{
    // if no valid color use the current background
    bool useBackground = false;
    SbViewportRegion vp(getViewportRegion());
    if (w>0 && h>0)
        vp.setWindowSize( (short)w, (short)h );

    //NOTE: To support pixels per inch we must use SbViewportRegion::setPixelsPerInch( ppi );
    //The default value is 72.0.
    //If we need to support grayscale images with must either use SoOffscreenRenderer::LUMINANCE or 
    //SoOffscreenRenderer::LUMINANCE_TRANSPARENCY. 
    SoFCOffscreenRenderer& renderer = SoFCOffscreenRenderer::instance();
    renderer.setViewportRegion(vp);
    SoCallback* cb = 0;

    // if we use transparency then we must not set a background color
    switch(eBackgroundType){
        case Current:
            if (backgroundroot->findChild(pcBackGround) == -1) {
                renderer.setBackgroundColor(this->getBackgroundColor());
            }
            else {
                useBackground = true;
                cb = new SoCallback;
                cb->setCallback(clearBuffer);
            }
            break;
        case White:
            renderer.setBackgroundColor( SbColor(1.0, 1.0, 1.0) );
            break;
        case Black:
            renderer.setBackgroundColor( SbColor(0.0, 0.0, 0.0) );
            break;
        case Transparent:
            renderer.setComponents(SoFCOffscreenRenderer::RGB_TRANSPARENCY );
            break;
        default:
            break;
    }

    SoSeparator* root = new SoSeparator;
    root->ref();

    SoCamera* camera = getCamera();
    if (useBackground) {
        root->addChild(backgroundroot);
        root->addChild(cb);
    }
    root->addChild(getHeadlight());
    root->addChild(camera);
    SoCallback* gl = new SoCallback;
    gl->setCallback(setGLWidget, this->getGLWidget());
    root->addChild(gl);
    root->addChild(pcViewProviderRoot);
    if (useBackground)
        root->addChild(cb);
    root->addChild(foregroundroot);

    try {
        // render the scene
        if (!renderer.render(root))
            throw Base::Exception("Offscreen rendering failed");
        renderer.writeToImage(img);
        root->unref();
    }
    catch(...) {
        root->unref();
        throw; // re-throw exception
    }
}

void View3DInventorViewer::saveGraphic(int pagesize, int eBackgroundType, SoVectorizeAction* va) const
{
    switch(eBackgroundType){
        case Current:
            va->setBackgroundColor(true, this->getBackgroundColor());
            break;
        case White:
            va->setBackgroundColor(true, SbColor(1.0, 1.0, 1.0));
            break;
        case Black:
            va->setBackgroundColor(true, SbColor(0.0, 0.0, 0.0));
            break;
        case Transparent:
            break; // not supported
        default:
            break;
    }

    float border = 10.0f;
    SbVec2s vpsize = this->getViewportRegion().getViewportSizePixels();
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
    va->calibrate(this->getViewportRegion());
    
    va->apply(this->getSceneManager()->getSceneGraph());

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

const std::vector<SbVec2s>& View3DInventorViewer::getPolygon(SbBool* clip_inner) const
{
    return navigation->getPolygon(clip_inner);
}

std::vector<SbVec2f> View3DInventorViewer::getGLPolygon(const std::vector<SbVec2s>& pnts) const
{
    const SbViewportRegion& vp = this->getViewportRegion();
    const SbVec2s& sz = vp.getWindowSize();
    short w,h; sz.getValue(w,h);
    const SbVec2s& sp = vp.getViewportSizePixels();
    const SbVec2s& op = vp.getViewportOriginPixels();
    const SbVec2f& siz = vp.getViewportSize();
    float dX, dY; siz.getValue(dX, dY);
    float fRatio = vp.getViewportAspectRatio();

    std::vector<SbVec2f> poly;
    for (std::vector<SbVec2s>::const_iterator it = pnts.begin(); it != pnts.end(); ++it) {
        SbVec2s loc = *it - op;
        SbVec2f pos((float)loc[0]/(float)sp[0], (float)loc[1]/(float)sp[1]);
        float pX,pY; pos.getValue(pX,pY);

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

std::vector<SbVec2f> View3DInventorViewer::getGLPolygon(SbBool* clip_inner) const
{
    const std::vector<SbVec2s>& pnts = navigation->getPolygon(clip_inner);
    return getGLPolygon(pnts);
}

bool View3DInventorViewer::dumpToFile(const char* filename, bool binary) const
{
    bool ret = false;
    Base::FileInfo fi(filename);

    // Write VRML V2.0
    if (fi.hasExtension("wrl") || fi.hasExtension("vrml") || fi.hasExtension("wrz")) {
        // If 'wrz' is set then force compression
        if (fi.hasExtension("wrz"))
            binary = true;

        SoToVRML2Action tovrml2;
        tovrml2.apply(pcViewProviderRoot);
        SoVRMLGroup *vrmlRoot = tovrml2.getVRML2SceneGraph();
        vrmlRoot->ref();
        std::string buffer = SoFCDB::writeNodesToString(vrmlRoot);
        vrmlRoot->unref(); // release the memory as soon as possible

        if (binary) {
            // We want to write compressed VRML but Coin 2.4.3 doesn't do it even though
            // SoOutput::getAvailableCompressionMethods() delivers a string list that
            // contains 'GZIP'. setCompression() was called directly after opening the file, 
            // returned TRUE and no error message appeared but anyway it didn't work.
            // Strange is that reading GZIPped VRML files works.
            // So, we do the compression on our own.
            Base::ofstream str(fi, std::ios::out | std::ios::binary);
            zipios::GZIPOutputStream gzip(str);
            if (gzip) {
                gzip << buffer;
                gzip.close();
                ret = true;
            }
        }
        else {
            Base::ofstream str(fi, std::ios::out);
            if (str) {
                str << buffer;
                str.close();
                ret = true;
            }
        }
    }
    else if (fi.hasExtension("idtf") || fi.hasExtension("svg") ) {
        int ps=4, t=2;
        std::auto_ptr<SoVectorizeAction> vo;

        if (fi.hasExtension("svg")) {
            vo = std::auto_ptr<SoVectorizeAction>(new SoFCVectorizeSVGAction());
        }
        else if (fi.hasExtension("idtf")) {
            vo = std::auto_ptr<SoVectorizeAction>(new SoFCVectorizeU3DAction());
        }
        else {
            throw Base::Exception("Not supported vector graphic");
        }

        SoVectorOutput * out = vo->getOutput();
        if (!out || !out->openFile(filename)) {
            std::ostringstream a_out;
            a_out << "Cannot open file '" << filename << "'";
            throw Base::Exception(a_out.str());
        }

        saveGraphic(ps,t,vo.get());
        out->closeFile();
    }
    else {
        // Write Inventor in ASCII
        std::string buffer = SoFCDB::writeNodesToString(pcViewProviderRoot);
        Base::ofstream str(Base::FileInfo(filename), std::ios::out);
        if (str) {
            str << buffer;
            str.close();
            ret = true;
        }
    }

    return ret;
}

/**
 * Sets the SoFCInteractiveElement to \a true.
 */
void View3DInventorViewer::interactionStartCB(void * data, SoQtViewer * viewer)
{
    SoGLRenderAction * glra = viewer->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), true);
}

/**
 * Sets the SoFCInteractiveElement to \a false and forces a redraw.
 */
void View3DInventorViewer::interactionFinishCB(void * data, SoQtViewer * viewer)
{
    SoGLRenderAction * glra = viewer->getGLRenderAction();
    SoFCInteractiveElement::set(glra->getState(), viewer->getSceneGraph(), false);
    viewer->render();
}

/**
 * Logs the type of the action that traverses the Inventor tree.
 */
void View3DInventorViewer::interactionLoggerCB(void * ud, SoAction* action)
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

void View3DInventorViewer::setRenderFramebuffer(const SbBool enable)
{
    if (!enable) {
        delete framebuffer;
        framebuffer = 0;
    }
    else if (!this->framebuffer) {
        const SbViewportRegion vp = this->getViewportRegion();
        SbVec2s origin = vp.getViewportOriginPixels();
        SbVec2s size = vp.getViewportSizePixels();

        this->glLockNormal();
        this->framebuffer = new QGLFramebufferObject(size[0],size[1],QGLFramebufferObject::Depth);
        renderToFramebuffer(this->framebuffer);
    }
}

SbBool View3DInventorViewer::isRenderFramebuffer() const
{
    return this->framebuffer != 0;
}

void View3DInventorViewer::renderToFramebuffer(QGLFramebufferObject* fbo)
{
    this->glLockNormal();
    fbo->bind();
    int width = fbo->size().width();
    int height = fbo->size().height();

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LINE_SMOOTH);

    const SbColor col = this->getBackgroundColor();
    glViewport(0, 0, width, height);
    glClearColor(col[0], col[1], col[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glDepthRange(0.1,1.0);

    SoGLRenderAction gl(SbViewportRegion(width, height));
    gl.apply(this->backgroundroot);
    gl.apply(this->getSceneManager()->getSceneGraph());
    gl.apply(this->foregroundroot);
    if (this->axiscrossEnabled) { this->drawAxisCross(); }

    fbo->release();
    this->glUnlockNormal();
}

void View3DInventorViewer::actualRedraw()
{
    if (this->framebuffer)
        renderFramebuffer();
    else
        renderScene();
}

void View3DInventorViewer::renderFramebuffer()
{
    const SbViewportRegion vp = this->getViewportRegion();
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

// Documented in superclass. Overrides this method to be able to draw
// the axis cross, if selected, and to keep a continuous animation
// upon spin.
void View3DInventorViewer::renderScene(void)
{
    // https://bitbucket.org/Coin3D/sogui/src/239bd7ae533d/viewers/SoGuiViewer.cpp.in
    // The commit introduced a regression for empty view volumes.
#if SOQT_MAJOR_VERSION > 1 || (SOQT_MAJOR_VERSION == 1 && SOQT_MINOR_VERSION >= 6)
    // With SoQt 1.6 we have problems when the scene is empty and auto-clipping is turned on.
    // There is always a warning that the frustum is invalid because the far and near distance
    // values were set to garbage values when trying to determine the clipping planes.
    // It will be turned on/off depending on the bounding box since the Coin3d doc says it may
    // have a bad impact on performance if it's always off.
    SoGetBoundingBoxAction action(getViewportRegion());
    action.apply(this->getSceneGraph());
    SbXfBox3f xbox = action.getXfBoundingBox();
    if (xbox.isEmpty()) {
        if (this->isAutoClipping())
            this->setAutoClipping(FALSE);
    }
    else {
        if (!this->isAutoClipping())
            this->setAutoClipping(TRUE);
    }
#endif

    // Must set up the OpenGL viewport manually, as upon resize
    // operations, Coin won't set it up until the SoGLRenderAction is
    // applied again. And since we need to do glClear() before applying
    // the action..
    const SbViewportRegion vp = this->getViewportRegion();
    SbVec2s origin = vp.getViewportOriginPixels();
    SbVec2s size = vp.getViewportSizePixels();
    glViewport(origin[0], origin[1], size[0], size[1]);

    const SbColor col = this->getBackgroundColor();
    glClearColor(col[0], col[1], col[2], 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // using 90% of the z-buffer for the background and the main node
    glDepthRange(0.1,1.0);

    // Render our scenegraph with the image.
    SoGLRenderAction * glra = this->getGLRenderAction();
    SoGLWidgetElement::set(glra->getState(), qobject_cast<QGLWidget*>(this->getGLWidget()));
    SoGLRenderActionElement::set(glra->getState(), glra);
    glra->apply(this->backgroundroot);

    navigation->updateAnimation();

    try {
        // Render normal scenegraph.
        inherited::actualRedraw();
    } catch (const Base::MemoryException&) {
        // FIXME: If this exception appears then the background and camera position get broken somehow. (Werner 2006-02-01) 
        for ( std::set<ViewProvider*>::iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); ++it )
            (*it)->hide();
        inherited::actualRedraw();
        QMessageBox::warning(getParentWidget(), QObject::tr("Out of memory"),
            QObject::tr("Not enough memory available to display the data."));
    }

    // using 10% of the z-buffer for the foreground node
    glDepthRange(0.0,0.1);

    // Render overlay front scenegraph.
    glra->apply(this->foregroundroot);

    if (this->axiscrossEnabled) { this->drawAxisCross(); }
   
    // using the main portion of z-buffer again (for frontbuffer highlighting)
    glDepthRange(0.1,1.0);

    // Immediately reschedule to get continous spin animation.
    if (this->isAnimating()) { this->scheduleRedraw(); }

#if 0 // this breaks highlighting of edges
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
#endif

    printDimension();
    navigation->redraw();
    for (std::list<GLGraphicsItem*>::iterator it = this->graphicsItems.begin(); it != this->graphicsItems.end(); ++it)
        (*it)->paintGL();

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

    if (this->isAnimating()) { this->stopAnimating(); }
    inherited::setSeekMode(on);
    navigation->setViewingMode(on ? NavigationStyle::SEEK_WAIT_MODE :
                         (this->isViewing() ?
                         NavigationStyle::IDLE : NavigationStyle::INTERACT));
}

void View3DInventorViewer::printDimension()
{
    SoCamera* cam = getCamera();
    if (!cam) return; // no camera there
    SoType t = getCamera()->getTypeId();
    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {
        const SbViewportRegion& vp = getViewportRegion();
        const SbVec2s& size = vp.getWindowSize();
        short dimX, dimY; size.getValue(dimX, dimY);

        float fHeight = static_cast<SoOrthographicCamera*>(getCamera())->height.getValue();
        float fWidth = fHeight;
        if (dimX > dimY)
            fWidth *= ((float)dimX)/((float)dimY);
        else if ( dimX < dimY )
            fHeight *= ((float)dimY)/((float)dimX);

        float fLog = float(log10(fWidth)), fFac;
        int   nExp = int(fLog);
        QString unit;
  
        if (nExp >= 6) {
            fFac = 1.0e+6f;
            unit = QLatin1String("km");
        }
        else if (nExp >= 3) {
            fFac = 1.0e+3f;
            unit = QLatin1String("m");
        }
        else if ((nExp >= 0) && (fLog > 0.0f)) {
            fFac = 1.0e+0f;
            unit = QLatin1String("mm");
        }
        else if (nExp >= -3) {
            fFac = 1.0e-3f;
            unit = QLatin1String("um");
        }
        else {
            fFac = 1.0e-6f;
            unit = QLatin1String("nm");
        }

        QString dim = QString::fromAscii("%1 x %2 %3")
                             .arg(fWidth / fFac,0,'f',2)
                             .arg(fHeight / fFac,0,'f',2)
                             .arg(unit);
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

/*!
  As ProgressBar has no chance to control the incoming Qt events of SoQt we need to override
  SoQtViewer::processEvent() to prevent the scenegraph from being selected or deselected
  while the progress bar is running.
 */
void View3DInventorViewer::processEvent(QEvent * event)
{
    // Bug #0000607: Some mices also support horizontal scrolling which however might
    // lead to some unwanted zooming when pressing the MMB for panning.
    // Thus, we filter out horizontal scrolling.
    if (event->type() == QEvent::Wheel) {
        QWheelEvent* we = static_cast<QWheelEvent*>(event);
        if (we->orientation() == Qt::Horizontal)
            return;
    }
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(event);
        if (ke->matches(QKeySequence::SelectAll)) {
            selectAll();
            return;
        }
    }
    if (!Base::Sequencer().isRunning() ||
        !Base::Sequencer().isBlocking())
        inherited::processEvent(event);

    if (event->type() == Spaceball::ButtonEvent::ButtonEventType){
        Spaceball::ButtonEvent *buttonEvent = static_cast<Spaceball::ButtonEvent *>(event);
        if (!buttonEvent){
            Base::Console().Log("invalid spaceball button event\n");
            return;
        }
    }

    if (event->type() == Spaceball::MotionEvent::MotionEventType) {
        Spaceball::MotionEvent *motionEvent = static_cast<Spaceball::MotionEvent *>(event);
        if (!motionEvent){
            Base::Console().Log("invalid spaceball motion event\n");
            return;
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

        SoMotion3Event motion3Event;
        motion3Event.setTranslation(translationVector);
        motion3Event.setRotation(xRot * yRot * zRot);

        this->processSoEvent(&motion3Event);
    }
}

SbBool View3DInventorViewer::processSoEvent(const SoEvent * const ev)
{
    if (isRedirectedToSceneGraph()) {
        SbBool processed = SoQtRenderArea::processSoEvent(ev);
        if (!processed)
            processed = navigation->processEvent(ev);
        return processed;
    }
    if (ev->getTypeId().isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        // filter out 'Q' and 'ESC' keys
        const SoKeyboardEvent * const ke = static_cast<const SoKeyboardEvent *>(ev);
        switch (ke->getKey()) {
        case SoKeyboardEvent::ESCAPE:
        case SoKeyboardEvent::Q: // ignore 'Q' keys (to prevent app from being closed)
            return SoQtRenderArea::processSoEvent(ev);
        default:
            break;
        }
    }

    return navigation->processEvent(ev);
}

SbBool View3DInventorViewer::processSoEventBase(const SoEvent * const ev)
{
    return inherited::processSoEvent(ev);
}

SbVec3f View3DInventorViewer::getViewDirection() const
{
    SoCamera* cam = this->getCamera();
    if (!cam) return SbVec3f(0,0,-1); // this is the default
    SbRotation camrot = cam->orientation.getValue();
    SbVec3f lookat(0, 0, -1); // init to default view direction vector
    camrot.multVec(lookat, lookat);
    return lookat;
}

SbVec3f View3DInventorViewer::getUpDirection() const
{
    SoCamera* cam = this->getCamera();
    if (!cam) return SbVec3f(0,1,0);
    SbRotation camrot = cam->orientation.getValue();
    SbVec3f upvec(0, 1, 0); // init to default up vector
    camrot.multVec(upvec, upvec);
    return upvec;
}

SbVec3f View3DInventorViewer::getPointOnScreen(const SbVec2s& pnt) const
{
    const SbViewportRegion& vp = this->getViewportRegion();

    short x,y; pnt.getValue(x,y);
    SbVec2f siz = vp.getViewportSize();
    float dX, dY; siz.getValue(dX, dY);

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

    SoCamera* pCam = this->getCamera();
    if (!pCam) return SbVec3f(); // return invalid point
    SbViewVolume  vol = pCam->getViewVolume();

    float nearDist = pCam->nearDistance.getValue();
    float farDist = pCam->farDistance.getValue();
    float focalDist = pCam->focalDistance.getValue();
    if (focalDist < nearDist || focalDist > farDist)
        focalDist = 0.5f*(nearDist + farDist);

    SbLine line; SbVec3f pt;
    SbPlane focalPlane = vol.getPlane(focalDist);
    vol.projectPointToLine(SbVec2f(pX,pY), line);
    focalPlane.intersect(line, pt);
    
    return pt;
}

void View3DInventorViewer::getNearPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getCamera();
    if (!pCam) return; // just do nothing
    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the front clipping plane
    SbPlane nearPlane = vol.getPlane(vol.nearDist);
    float d = nearPlane.getDistanceFromOrigin();
    rcNormal = nearPlane.getNormal();
    rcNormal.normalize();
    float nx, ny, nz; rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(d*rcNormal[0], d*rcNormal[1], d*rcNormal[2]);
}

void View3DInventorViewer::getFarPlane(SbVec3f& rcPt, SbVec3f& rcNormal) const
{
    SoCamera* pCam = getCamera();
    if (!pCam) return; // just do nothing
    SbViewVolume vol = pCam->getViewVolume();

    // get the normal of the back clipping plane
    SbPlane farPlane = vol.getPlane(vol.nearDist+vol.nearToFar);
    float d = farPlane.getDistanceFromOrigin();
    rcNormal = farPlane.getNormal();
    rcNormal.normalize();
    float nx, ny, nz; rcNormal.getValue(nx, ny, nz);
    rcPt.setValue(d*rcNormal[0], d*rcNormal[1], d*rcNormal[2]);
}

SbVec3f View3DInventorViewer::projectOnNearPlane(const SbVec2f& pt) const
{
    SbVec3f pt1, pt2;
    SoCamera* cam = this->getCamera();
    if (!cam) return SbVec3f(); // return invalid point
    SbViewVolume vol = cam->getViewVolume();
    vol.projectPointToLine(pt, pt1, pt2);
    return pt1;
}

SbVec3f View3DInventorViewer::projectOnFarPlane(const SbVec2f& pt) const
{
    SbVec3f pt1, pt2;
    SoCamera* cam = this->getCamera();
    if (!cam) return SbVec3f(); // return invalid point
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
        SoGetBoundingBoxAction action(this->getViewportRegion());
        action.apply(this->getSceneGraph());
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
bool View3DInventorViewer::pickPoint(const SbVec2s& pos,SbVec3f &point,SbVec3f &norm) const
{
    // attempting raypick in the event_cb() callback method
    SoRayPickAction rp(getViewportRegion());
    rp.setPoint(pos);
    rp.apply(getSceneManager()->getSceneGraph());
    SoPickedPoint *Point = rp.getPickedPoint();

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
    SoRayPickAction rp(getViewportRegion());
    rp.setPoint(pos);
    rp.apply(getSceneManager()->getSceneGraph());

    // returns a copy of the point
    SoPickedPoint* pick = rp.getPickedPoint();
    //return (pick ? pick->copy() : 0); // needs the same instance of CRT under MS Windows
    return (pick ? new SoPickedPoint(*pick) : 0);
}

const SoPickedPoint* View3DInventorViewer::getPickedPoint(SoEventCallback * n) const
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
        SoCamera* cam = this->getCamera();
        if (cam == 0) return;
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(M_PI / 4.0);
    }
}

void View3DInventorViewer::moveCameraTo(const SbRotation& rot, const SbVec3f& pos, int steps, int ms)
{
    SoCamera* cam = this->getCamera();
    if (cam == 0) return;

    SbVec3f campos = cam->position.getValue();
    SbRotation camrot = cam->orientation.getValue();
    //SbVec3f dir1, dir2;
    //camrot.multVec(SbVec3f(0, 0, -1), dir1);
    //rot.multVec(SbVec3f(0, 0, -1), dir2);

    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    for (int i=0; i<steps; i++) {
        float s = float(i)/float(steps);
        SbVec3f curpos = campos * (1.0f-s) + pos * s;
        SbRotation currot = SbRotation::slerp(camrot, rot, s);
        cam->orientation.setValue(currot);
        cam->position.setValue(curpos);
        timer.start(Base::clamp<int>(ms,0,5000));
        loop.exec(QEventLoop::ExcludeUserInputEvents);
    }

    cam->orientation.setValue(rot);
    cam->position.setValue(pos);
}

void View3DInventorViewer::boxZoom(const SbBox2s& box)
{
    navigation->boxZoom(box);
}

void View3DInventorViewer::viewAll()
{
    // in the scene graph we may have objects which we want to exlcude
    // when doing a fit all. Such objects must be part of the group
    // SoSkipBoundingGroup.
    SoSearchAction sa;
    sa.setType(SoSkipBoundingGroup::getClassTypeId());
    sa.setInterest(SoSearchAction::ALL);
    sa.apply(this->getSceneGraph());
    const SoPathList & pathlist = sa.getPaths();
    for (int i = 0; i < pathlist.getLength(); i++ ) {
        SoPath * path = pathlist[i];
        SoSkipBoundingGroup * group = static_cast<SoSkipBoundingGroup*>(path->getTail());
        group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
    }

    // Set the height angle to 45 deg
    SoCamera* cam = this->getCamera();
    if (cam && cam->getTypeId().isDerivedFrom(SoPerspectiveCamera::getClassTypeId()))
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = (float)(M_PI / 4.0);

    // call the default implementation first to make sure everything is visible
    SoQtViewer::viewAll();

    for (int i = 0; i < pathlist.getLength(); i++ ) {
        SoPath * path = pathlist[i];
        SoSkipBoundingGroup * group = static_cast<SoSkipBoundingGroup*>(path->getTail());
        group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
    }
    //navigation->viewAll();
}

void View3DInventorViewer::viewAll(float factor)
{
    SoCamera * cam = this->getCamera();
    if (!cam) return;
    if (factor <= 0.0f) return;

    if (factor != 1.0f) {
        SoSearchAction sa;
        sa.setType(SoSkipBoundingGroup::getClassTypeId());
        sa.setInterest(SoSearchAction::ALL);
        sa.apply(this->getSceneGraph());
        const SoPathList & pathlist = sa.getPaths();
        for (int i = 0; i < pathlist.getLength(); i++ ) {
            SoPath * path = pathlist[i];
            SoSkipBoundingGroup * group = static_cast<SoSkipBoundingGroup*>(path->getTail());
            group->mode = SoSkipBoundingGroup::EXCLUDE_BBOX;
        }

        SoGetBoundingBoxAction action(this->getViewportRegion());
        action.apply(this->getSceneGraph());
        SbBox3f box = action.getBoundingBox();
        float minx,miny,minz,maxx,maxy,maxz;
        box.getBounds(minx,miny,minz,maxx,maxy,maxz);

        for (int i = 0; i < pathlist.getLength(); i++ ) {
            SoPath * path = pathlist[i];
            SoSkipBoundingGroup * group = static_cast<SoSkipBoundingGroup*>(path->getTail());
            group->mode = SoSkipBoundingGroup::INCLUDE_BBOX;
        }

        SoCube * cube = new SoCube();
        cube->width  = factor*(maxx-minx);
        cube->height = factor*(maxy-miny);
        cube->depth  = factor*(maxz-minz);

        // fake a scenegraph with the desired bounding size
        SoSeparator* graph = new SoSeparator();
        graph->ref();
        SoTranslation * tr = new SoTranslation();
        tr->translation.setValue(box.getCenter());

        graph->addChild(tr);
        graph->addChild(cube);
        cam->viewAll(graph, this->getViewportRegion());
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

    for (int i=0; i<countPaths;i++) {
        SoPath* path = paths[i];
        SoNode* node = path->getTail();
        if (!node || node->getTypeId() != SoFCSelection::getClassTypeId())
            continue; // should not happen
        SoFCSelection* select = static_cast<SoFCSelection *>(node);
        // Check only document and object name but not sub-element name
        if (Selection().isSelected(select->documentName.getValue().getString(),
                                   select->objectName.getValue().getString())
                                   ) {
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

    SoCamera* cam = this->getCamera();
    if (cam) cam->viewAll(root, this->getViewportRegion());
    root->unref();
}

/*!
  Decide if it should be possible to start a spin animation of the
  model in the viewer by releasing the mouse button while dragging.

  If the \a enable flag is \c FALSE and we're currently animating, the
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

    if (this->isViewing()) { this->scheduleRedraw(); }
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
        this->scheduleRedraw();
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
void View3DInventorViewer::setCursorEnabled(SbBool enable)
{
    inherited::setCursorEnabled(enable);
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
  SbVec2s view = this->getGLSize();
  const int pixelarea =
    int(float(this->axiscrossSize)/100.0f * SoQtMin(view[0], view[1]));
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
  const float dim = NEARVAL * float(tan(M_PI / 8.0)); // FOV is 45° (45/360 = 1/8)
  glFrustum(-dim, dim, -dim, dim, NEARVAL, FARVAL);


  // Set up the model matrix.
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  SbMatrix mx;
  SoCamera * cam = this->getCamera();

  // If there is no camera (like for an empty scene, for instance),
  // just use an identity rotation.
  if (cam) { mx = cam->orientation.getValue(); }
  else { mx = SbMatrix::identity(); }

  mx = mx.inverse();
  mx[3][2] = -3.5; // Translate away from the projection point (along z axis).
  glLoadMatrixf((float *)mx);


  // Find unit vector end points.
  SbMatrix px;
  glGetFloatv(GL_PROJECTION_MATRIX, (float *)px);
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
    if (val[0] < val[1]) { SoQtSwap(val[0], val[1]); SoQtSwap(idx[0], idx[1]); }
    if (val[1] < val[2]) { SoQtSwap(val[1], val[2]); SoQtSwap(idx[1], idx[2]); }
    if (val[0] < val[1]) { SoQtSwap(val[0], val[1]); SoQtSwap(idx[0], idx[1]); }
    assert((val[0] >= val[1]) && (val[1] >= val[2])); // Just checking..

    for (int i=0; i < 3; i++) {
      glPushMatrix();
      if (idx[i] == XAXIS) {                       // X axis.
        if (isStereoViewing())
          glColor3f(0.500f, 0.5f, 0.5f);
        else
          glColor3f(0.500f, 0.125f, 0.125f);
      } else if (idx[i] == YAXIS) {                // Y axis.
        glRotatef(90, 0, 0, 1);
        if (isStereoViewing())
          glColor3f(0.400f, 0.4f, 0.4f);
        else
          glColor3f(0.125f, 0.500f, 0.125f);
      } else {                                     // Z axis.
        glRotatef(-90, 0, 1, 0);
        if (isStereoViewing())
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

  if(isStereoViewing())
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

#define HAND_WITH 24
#define HAND_HEIGHT 24
#define HAND_HOT_X 9
#define HAND_HOT_Y 0

static unsigned char hand_bitmap[] = {
    0x00,0x03,0x00,0x80,0x04,0x00,0x80,0x04,0x00,0x80,0x04,0x00,0x80,0x04,0x00,
    0x80,0x1c,0x00,0x80,0xe4,0x00,0x80,0x24,0x01,0x80,0x24,0x07,0x8e,0x24,0x09,
    0x92,0x24,0x09,0xa4,0x00,0x09,0xc4,0x00,0x08,0x08,0x00,0x08,0x08,0x00,0x08,
    0x10,0x00,0x08,0x10,0x00,0x04,0x20,0x00,0x04,0x20,0x00,0x04,0x40,0x00,0x02,
    0x80,0x00,0x02,0x00,0x01,0x01,0x00,0xff,0x01,0x00,0x00,0x00,0x00,0xab,0xab,
    0xab,0xab,0xab,0xab,0xab,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,
    0x00,0x1b,0x00,0xee,0x04,0xee };

static unsigned char hand_mask_bitmap[] = {
    0x00,0x03,0x00,0x80,0x07,0x00,0x80,0x07,0x00,0x80,0x07,0x00,0x80,0x07,0x00,
    0x80,0x1f,0x00,0x80,0xff,0x00,0x80,0xff,0x01,0x80,0xff,0x07,0x8e,0xff,0x0f,
    0x9e,0xff,0x0f,0xbc,0xff,0x0f,0xfc,0xff,0x0f,0xf8,0xff,0x0f,0xf8,0xff,0x0f,
    0xf0,0xff,0x0f,0xf0,0xff,0x07,0xe0,0xff,0x07,0xe0,0xff,0x07,0xc0,0xff,0x03,
    0x80,0xff,0x03,0x00,0xff,0x01,0x00,0xff,0x01,0x00,0x00,0x00,0x00,0xab,0xab,
    0xab,0xab,0xab,0xab,0xab,0xab,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x05,
    0x00,0x1b,0x00,0xd5,0x07,0x1c };

#define CROSS_WIDTH 16
#define CROSS_HEIGHT 16
#define CROSS_HOT_X 7
#define CROSS_HOT_Y 7

static unsigned char cross_bitmap[] = {
  0xc0, 0x03, 0x40, 0x02, 0x40, 0x02, 0x40, 0x02,
  0x40, 0x02, 0x40, 0x02, 0x7f, 0xfe, 0x01, 0x80,
  0x01, 0x80, 0x7f, 0xfe, 0x40, 0x02, 0x40, 0x02,
  0x40, 0x02, 0x40, 0x02, 0x40, 0x02, 0xc0, 0x03
};

static unsigned char cross_mask_bitmap[] = {
 0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xff,0xff,0xff,
 0xff,0xff,0xff,0xff,0xff,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,0xc0,0x03,
 0xc0,0x03
};

// Set cursor graphics according to mode.
void View3DInventorViewer::setCursorRepresentation(int modearg)
{
    // There is a synchronization problem between Qt and SoQt which
    // happens when popping up a context-menu. In this case the
    // Qt::WA_UnderMouse attribute is resetted and never set again
    // even if the mouse is still in the canvas. Thus, the cursor
    // won't be changed as long as the user doesn't leave and enter
    // the canvas. To fix this we explicitly set Qt::WA_UnderMouse
    // if the mouse is inside the canvas.
    QWidget* w = this->getGLWidget();
    if (w && w->rect().contains(QCursor::pos()))
        w->setAttribute(Qt::WA_UnderMouse);

    if (!this->isCursorEnabled()) {
        this->setComponentCursor(SoQtCursor::getBlankCursor());
        return;
    }

    switch (modearg) {
    case NavigationStyle::IDLE:
    case NavigationStyle::INTERACT:
        if (isEditing())
            this->getWidget()->setCursor(this->editCursor);
        else
            this->setComponentCursor(SoQtCursor(SoQtCursor::DEFAULT));
        break;

    case NavigationStyle::DRAGGING:
    case NavigationStyle::SPINNING:
        this->setComponentCursor(SoQtCursor::getRotateCursor());
        break;

    case NavigationStyle::ZOOMING:
        {
            this->setComponentCursor(SoQtCursor::getZoomCursor());
        }   break;

    case NavigationStyle::SEEK_MODE:
    case NavigationStyle::SEEK_WAIT_MODE:
    case NavigationStyle::BOXZOOM:
        {
            SoQtCursor::CustomCursor custom;
            custom.dim.setValue(CROSS_WIDTH, CROSS_HEIGHT);
            custom.hotspot.setValue(CROSS_HOT_X, CROSS_HOT_Y);
            custom.bitmap = cross_bitmap;
            custom.mask = cross_mask_bitmap;
            this->setComponentCursor(SoQtCursor(&custom));
        }
        break;

    case NavigationStyle::PANNING:
        this->setComponentCursor(SoQtCursor::getPanCursor());
        break;

    case NavigationStyle::SELECTION:
        {
            SoQtCursor::CustomCursor custom;
            custom.dim.setValue(HAND_WITH, HAND_HEIGHT);
            custom.hotspot.setValue(HAND_HOT_X, HAND_HOT_Y);
            custom.bitmap = hand_bitmap;
            custom.mask = hand_mask_bitmap;
            this->setComponentCursor(SoQtCursor(&custom));
        }
        break;

    default: assert(0); break;
    }
}

void View3DInventorViewer::setEditing(SbBool edit) 
{
    this->editing = edit; 
    this->setComponentCursor(SoQtCursor(SoQtCursor::DEFAULT));
    this->editCursor = QCursor();
}

void View3DInventorViewer::setEditingCursor (const SoQtCursor& cursor)
{
    //Note: Coin caches the pointer to the CustomCursor instance with
    //the QCursor instance in a dictionary. So we must not store the
    //SoQtCursor object here but the QCursor object, otherwise we might
    //restore the wrong QCursor from the dictionary. 
    this->setComponentCursor(cursor);
    this->editCursor = this->getWidget()->cursor();
}

void View3DInventorViewer::setEditingCursor (const QCursor& cursor)
{
    //Note: Coin caches the pointer to the CustomCursor instance with
    //the QCursor instance in a dictionary. So we must not store the
    //SoQtCursor object here but the QCursor object, otherwise we might
    //restore the wrong QCursor from the dictionary. 
    this->getWidget()->setCursor(cursor);
    this->editCursor = cursor;
}

void View3DInventorViewer::selectCB(void *viewer, SoPath *path)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(path);
    if (vp && vp->useNewSelectionModel()) {
    }
}

void View3DInventorViewer::deselectCB(void *viewer, SoPath *path)
{
    ViewProvider* vp = static_cast<View3DInventorViewer*>(viewer)->getViewProviderByPath(path);
    if (vp && vp->useNewSelectionModel()) {
    }
}

SoPath * View3DInventorViewer::pickFilterCB(void *viewer, const SoPickedPoint * pp)
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

        getMainWindow()->showMessage(QString::fromAscii(buf),3000);
    }
    return pp->getPath();
}

void View3DInventorViewer::addEventCallback(SoType eventtype, SoEventCallbackCB * cb, void* userdata)
{
    pEventCallback->addEventCallback(eventtype, cb, userdata);
}

void View3DInventorViewer::removeEventCallback(SoType eventtype, SoEventCallbackCB * cb, void* userdata)
{
    pEventCallback->removeEventCallback(eventtype, cb, userdata);
}

ViewProvider* View3DInventorViewer::getViewProviderByPath(SoPath * path) const
{
    // FIXME Use the viewprovider map introduced for the selection
    for (std::set<ViewProvider*>::const_iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); it++) {
        for (int i = 0; i<path->getLength();i++) {
            SoNode *node = path->getNode(i);
            if ((*it)->getRoot() == node) {
                return (*it);
            }
        }
    }

    return 0;
}

ViewProvider* View3DInventorViewer::getViewProviderByPathFromTail(SoPath * path) const
{
    // Make sure I'm the lowest LocHL in the pick path!
    for (int i = 0; i < path->getLength(); i++) {
        SoNode *node = path->getNodeFromTail(i);
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            std::map<SoSeparator*,ViewProvider*>::const_iterator it = _ViewProviderMap.find(static_cast<SoSeparator*>(node));
            if (it != _ViewProviderMap.end()){
                return it->second;
            }
         }
    }

    return 0;
}

std::vector<ViewProvider*> View3DInventorViewer::getViewProvidersOfType(const Base::Type& typeId) const
{
    std::vector<ViewProvider*> views;
    for (std::set<ViewProvider*>::const_iterator it = _ViewProviderSet.begin(); it != _ViewProviderSet.end(); it++) {
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
  static_cast<SoSwitch *>(dimensionRoot->getChild(0))->removeAllChildren();
  static_cast<SoSwitch *>(dimensionRoot->getChild(1))->removeAllChildren();
}

void View3DInventorViewer::turn3dDimensionsOn()
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_ALL;
}

void View3DInventorViewer::turn3dDimensionsOff()
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(0))->whichChild = SO_SWITCH_NONE;
}

void View3DInventorViewer::addDimension3d(SoNode *node)
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(0))->addChild(node);
}

void View3DInventorViewer::addDimensionDelta(SoNode *node)
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(1))->addChild(node);
}

void View3DInventorViewer::turnDeltaDimensionsOn()
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_ALL;
}

void View3DInventorViewer::turnDeltaDimensionsOff()
{
  static_cast<SoSwitch *>(dimensionRoot->getChild(1))->whichChild = SO_SWITCH_NONE;
}
