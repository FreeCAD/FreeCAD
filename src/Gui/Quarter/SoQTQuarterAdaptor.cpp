// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2014 Stefan Tröger <stefantroeger@gmx.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#include <FCConfig.h>

#include <numbers>

#include <Base/Console.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SoEventManager.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/events/SoEvents.h>
#include <Inventor/nodes/SoLocateHighlight.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoBaseColor.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoFont.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/nodes/SoTranslation.h>

# ifdef FC_OS_WIN32
#  include <windows.h>
# endif
# ifdef FC_OS_MACOSX
# include <OpenGL/gl.h>
# else
# include <GL/gl.h>
# endif

#include "SoQTQuarterAdaptor.h"

#ifdef BUILD_TRACY_FRAME_PROFILER
#include <tracy/Tracy.hpp>
#endif

namespace
{
struct OverlayTextState {
    SoSeparator* root {nullptr};
    SoOrthographicCamera* camera {nullptr};
    SoDepthBuffer* depth {nullptr};
    SoLightModel* lightModel {nullptr};
    SoBaseColor* baseColor {nullptr};
    SoFont* font {nullptr};
    SoTranslation* translation {nullptr};
    SoText2* text {nullptr};

    void ensureCreated()
    {
        if (root) {
            return;
        }

        root = new SoSeparator;
        root->ref();

        camera = new SoOrthographicCamera;
        root->addChild(camera);

        depth = new SoDepthBuffer;
        depth->test.setValue(false);
        depth->write.setValue(false);
        depth->function.setValue(SoDepthBuffer::ALWAYS);
        root->addChild(depth);

        lightModel = new SoLightModel;
        lightModel->model.setValue(SoLightModel::BASE_COLOR);
        root->addChild(lightModel);

        baseColor = new SoBaseColor;
        root->addChild(baseColor);

        font = new SoFont;
        font->size.setValue(12.0f);
        root->addChild(font);

        translation = new SoTranslation;
        root->addChild(translation);

        text = new SoText2;
        root->addChild(text);
    }
};

OverlayTextState& overlayTextState()
{
    static OverlayTextState state;
    return state;
}

}  // namespace

constexpr const int defaultSize = 100;

// NOLINTBEGIN(readability-implicit-bool-conversion)
SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QWidget* parent,
                                                             const QOpenGLWidget* sharewidget,
                                                             Qt::WindowFlags flags)
    : QuarterWidget(parent, sharewidget, flags)
    , matrixaction(SbViewportRegion(defaultSize, defaultSize))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(const QSurfaceFormat& format,
                                                             QWidget* parent,
                                                             const QOpenGLWidget* shareWidget,
                                                             Qt::WindowFlags flags)
    : QuarterWidget(format, parent, shareWidget, flags)
    , matrixaction(SbViewportRegion(defaultSize, defaultSize))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QOpenGLContext* context,
                                                             QWidget* parent,
                                                             const QOpenGLWidget* sharewidget,
                                                             Qt::WindowFlags flags)
    : QuarterWidget(context, parent, sharewidget, flags)
    , matrixaction(SbViewportRegion(defaultSize, defaultSize))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::~SoQTQuarterAdaptor()
{
    delete m_seeksensor;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::init()
{
    // NOLINTBEGIN
    m_interactionnesting = 0;
    m_seekdistance = 50.0F;
    m_seekdistanceabs = false;
    m_seekperiod = 2.0F;
    m_inseekmode = false;
    m_storedcamera = nullptr;
    m_viewingflag = false;
    pickRadius = 5.0;

    m_seeksensor = new SoTimerSensor(SoQTQuarterAdaptor::seeksensorCB, (void*)this);
    getSoEventManager()->setNavigationState(SoEventManager::NO_NAVIGATION);

    resetFrameCounter();
    // NOLINTEND
}


QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getWidget()
{
    //we keep the function from SoQt as we want to introduce the QGraphicsView and then the GLWidget
    //is separated from the Widget used in layouts again
    return this;
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getGLWidget()
{
    return viewport();
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getWidget() const
{
    //we keep the function from SoQt as we want to introduce the QGraphicsView and then the GLWidget
    //is separated from the Widget used in layouts again
    return const_cast<SoQTQuarterAdaptor*>(this);  // NOLINT
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getGLWidget() const
{
    return viewport();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setCameraType(SoType type)
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (cam && !cam->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
               !cam->isOfType(SoOrthographicCamera::getClassTypeId())) {
        Base::Console().warning("Quarter::setCameraType",
                                "Only SoPerspectiveCamera and SoOrthographicCamera is supported.");
        return;
    }


    SoType perspectivetype = SoPerspectiveCamera::getClassTypeId();
    SbBool oldisperspective = cam ? cam->getTypeId().isDerivedFrom(perspectivetype) : false;
    SbBool newisperspective = type.isDerivedFrom(perspectivetype);

    // Same old, same old..
    if (oldisperspective == newisperspective) {
        return;
    }

    SoCamera* currentcam = getSoRenderManager()->getCamera();
    SoCamera* newcamera = static_cast<SoCamera*>(type.createInstance());  // NOLINT

    // Transfer and convert values from one camera type to the other.
    if(newisperspective) {
        convertOrtho2Perspective(dynamic_cast<SoOrthographicCamera*>(currentcam),
                                 dynamic_cast<SoPerspectiveCamera*>(newcamera));
    }
    else {
        convertPerspective2Ortho(dynamic_cast<SoPerspectiveCamera*>(currentcam),
                                 dynamic_cast<SoOrthographicCamera*>(newcamera));
    }

    getSoRenderManager()->setCamera(newcamera);
    getSoEventManager()->setCamera(newcamera);

    //if the superscene has a camera we need to replace it too
    auto superscene = dynamic_cast<SoSeparator*>(getSoRenderManager()->getSceneGraph());
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setType(SoCamera::getClassTypeId());
    sa.apply(superscene);

    if (sa.getPath()) {
        SoNode* node = sa.getPath()->getTail();
        SoGroup* parent = static_cast<SoGroup*>(sa.getPath()->getNodeFromTail(1)); //  NOLINT

        if (node && node->isOfType(SoCamera::getClassTypeId())) {
            parent->replaceChild(node, newcamera);
        }
    }
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::convertOrtho2Perspective(const SoOrthographicCamera* in,
        SoPerspectiveCamera* out)
{
    if (!in || !out) {
        Base::Console().log("Quarter::convertOrtho2Perspective",
                            "Cannot convert camera settings due to wrong input.");
        return;
    }
    out->aspectRatio.setValue(in->aspectRatio.getValue());
    out->focalDistance.setValue(in->focalDistance.getValue());
    out->orientation.setValue(in->orientation.getValue());
    out->position.setValue(in->position.getValue());
    out->viewportMapping.setValue(in->viewportMapping.getValue());

    SbRotation camrot = in->orientation.getValue();

    float focaldist = float(in->height.getValue() / (2.0*tan(std::numbers::pi / 8.0)));  // NOLINT

    SbVec3f offset(0,0,focaldist-in->focalDistance.getValue());

    camrot.multVec(offset,offset);
    out->position.setValue(offset+in->position.getValue());

    out->focalDistance.setValue(focaldist);

    // 45° is the default value of this field in SoPerspectiveCamera.
    out->heightAngle = (float)(std::numbers::pi / 4.0);  // NOLINT
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::convertPerspective2Ortho(const SoPerspectiveCamera* in,
        SoOrthographicCamera* out)
{
    out->aspectRatio.setValue(in->aspectRatio.getValue());
    out->focalDistance.setValue(in->focalDistance.getValue());
    out->orientation.setValue(in->orientation.getValue());
    out->position.setValue(in->position.getValue());
    out->viewportMapping.setValue(in->viewportMapping.getValue());

    float focaldist = in->focalDistance.getValue();

    out->height = 2.0F * focaldist * (float)tan(in->heightAngle.getValue() / 2.0);  // NOLINT
}

SoCamera* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getCamera() const
{
    return getSoRenderManager()->getCamera();
}

const SbViewportRegion & SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getViewportRegion() const
{
    return getSoRenderManager()->getViewportRegion();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setViewing(bool enable)
{
    m_viewingflag = enable;

    // Turn off the selection indicators when we go back from picking
    // mode into viewing mode.
    if (m_viewingflag) {
        SoGLRenderAction* action = getSoRenderManager()->getGLRenderAction();

        if (action) {
            SoLocateHighlight::turnOffCurrentHighlight(action);
        }
    }
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isViewing() const
{
    return m_viewingflag;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::interactiveCountInc()
{
    // Catch problems with missing interactiveCountDec() calls.
    assert(m_interactionnesting < 100);

    if (++m_interactionnesting == 1) {
        m_interactionStartCallback.invokeCallbacks(this);
    }
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::interactiveCountDec()
{
    if (--m_interactionnesting <= 0) {
        m_interactionEndCallback.invokeCallbacks(this);
        m_interactionnesting = 0;
    }
}

int SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getInteractiveCount() const
{
    return m_interactionnesting;
}

// clang-format off
void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addStartCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionStartCallback.addCallback((SoCallbackListCB*)func, data);  // NOLINT
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::removeStartCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionStartCallback.removeCallback((SoCallbackListCB*)func, data);  // NOLINT
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addFinishCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionEndCallback.addCallback((SoCallbackListCB*)func, data);  // NOLINT
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::removeFinishCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionEndCallback.removeCallback((SoCallbackListCB*)func, data);  // NOLINT
}
// clang-format on

float SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getSeekDistance() const
{
    return m_seekdistance;
}

float SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getSeekTime() const
{
    return m_seekperiod;
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isSeekMode() const
{
    return m_inseekmode;
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isSeekValuePercentage() const
{
    return !m_seekdistanceabs;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setPickRadius(float pickRadius)
{
    this->pickRadius = pickRadius;
    SoEventManager* evm = this->getSoEventManager();
    if (evm){
        SoHandleEventAction* hea = evm->getHandleEventAction();
        if (hea){
            hea->setPickRadius(pickRadius);
        }
    }
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seekToPoint(const SbVec2s& screenpos)
{

    SoRayPickAction rpaction(getSoRenderManager()->getViewportRegion());
    rpaction.setPoint(screenpos);
    rpaction.setRadius(pickRadius);
    rpaction.apply(getSoRenderManager()->getSceneGraph());

    SoPickedPoint* picked = rpaction.getPickedPoint();

    if (!picked) {
        this->interactiveCountInc(); // decremented in setSeekMode(false)
        this->setSeekMode(false);
        return false;
    }

    SbVec3f hitpoint;
    hitpoint = picked->getPoint();

    this->seekToPoint(hitpoint);
    return true;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seekToPoint(const SbVec3f& scenepos)
{
    SbVec3f hitpoint(scenepos);

    m_camerastartposition = getSoRenderManager()->getCamera()->position.getValue();
    m_camerastartorient = getSoRenderManager()->getCamera()->orientation.getValue();

    // move point to the camera coordinate system, consider
    // transformations before camera in the scene graph
    SbMatrix cameramatrix;
    SbMatrix camerainverse;
    getCameraCoordinateSystem(getSoRenderManager()->getCamera(),
                              getSceneGraph(),
                              cameramatrix,
                              camerainverse);
    camerainverse.multVecMatrix(hitpoint, hitpoint);

    float fd = m_seekdistance;

    if(!m_seekdistanceabs) {
        fd *= (hitpoint - getSoRenderManager()->getCamera()->position.getValue()).length()/100.0F;
    }

    getSoRenderManager()->getCamera()->focalDistance = fd;

    SbVec3f dir = hitpoint - m_camerastartposition;
    dir.normalize();

    // find a rotation that rotates current camera direction into new
    // camera direction.
    SbVec3f olddir;
    getSoRenderManager()->getCamera()->orientation.getValue().multVec(SbVec3f(0, 0, -1), olddir);
    SbRotation diffrot(olddir, dir);
    m_cameraendposition = hitpoint - fd * dir;
    m_cameraendorient = getSoRenderManager()->getCamera()->orientation.getValue() * diffrot;

    if(m_seeksensor->isScheduled()) {
        m_seeksensor->unschedule();
        interactiveCountDec();
    }

    m_seeksensor->setBaseTime(SbTime::getTimeOfDay());
    m_seeksensor->schedule();
    interactiveCountInc();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekDistance(const float distance)
{
    m_seekdistance = distance;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekMode(bool enable)
{
    if(!enable && m_seeksensor->isScheduled()) {
        m_seeksensor->unschedule();
        interactiveCountDec();
    }

    m_inseekmode = enable;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekTime(const float seconds)
{
    m_seekperiod = seconds;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekValueAsPercentage(bool on)
{
    m_seekdistanceabs = !on;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getCameraCoordinateSystem(SoCamera* camera,
                                                                         SoNode* root,
                                                                         SbMatrix& matrix,
                                                                         SbMatrix& inverse)
{
    searchaction.reset();
    searchaction.setSearchingAll(true);
    searchaction.setInterest(SoSearchAction::FIRST);
    searchaction.setNode(camera);
    searchaction.apply(root);

    matrix = inverse = SbMatrix::identity();

    if(searchaction.getPath()) {
        matrixaction.apply(searchaction.getPath());
        matrix = matrixaction.getMatrix();
        inverse = matrixaction.getInverse();
    }

    searchaction.reset();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seeksensorCB(void* data, SoSensor* sensor)
{
    SoQTQuarterAdaptor* thisp = static_cast<SoQTQuarterAdaptor*>(data);  // NOLINT
    SbTime currenttime = SbTime::getTimeOfDay();

    SoTimerSensor* timer = static_cast<SoTimerSensor*>(sensor);  // NOLINT

    float par = float((currenttime - timer->getBaseTime()).getValue()) / thisp->m_seekperiod;

    if ((par > 1.0F) || (par + timer->getInterval().getValue() > 1.0F)) {
        par = 1.0F;
    }

    bool end = (par == 1.0F);

    par = (float)((1.0 - cos(std::numbers::pi * par)) * 0.5);  // NOLINT

    thisp->getSoRenderManager()->getCamera()->position = thisp->m_camerastartposition +
            (thisp->m_cameraendposition - thisp->m_camerastartposition) * par;
    thisp->getSoRenderManager()->getCamera()->orientation =
        SbRotation::slerp(thisp->m_camerastartorient,
                          thisp->m_cameraendorient,
                          par);

    if (end) {
        thisp->setSeekMode(false);
    }
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::saveHomePosition()
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    SoType type = cam->getTypeId();
    assert(type.isDerivedFrom(SoNode::getClassTypeId()));
    assert(type.canCreateInstance());

    if(m_storedcamera) {
        m_storedcamera->unref();
    }

    m_storedcamera = static_cast<SoNode*>(type.createInstance());  // NOLINT
    m_storedcamera->ref();

    m_storedcamera->copyFieldValues(getSoRenderManager()->getCamera());
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::resetToHomePosition()
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    if(!m_storedcamera) {
        return;
    }

    SoType ttype = getSoRenderManager()->getCamera()->getTypeId();
    SoType stype = m_storedcamera->getTypeId();

    // most common case
    if (ttype == stype) {
        // We copy the field data directly, instead of using
        // SoFieldContainer::copyContents(), for the reason described in
        // detail in So@Gui@Viewer::saveHomePosition().
        getSoRenderManager()->getCamera()->copyFieldValues(m_storedcamera);
    }
    // handle common case #1
    else if(ttype == SoOrthographicCamera::getClassTypeId() &&
            stype == SoPerspectiveCamera::getClassTypeId()) {
        convertPerspective2Ortho(dynamic_cast<SoPerspectiveCamera*>(m_storedcamera),
                                 dynamic_cast<SoOrthographicCamera*>(getSoRenderManager()->getCamera()));
    }
    // handle common case #2
    else if(ttype == SoPerspectiveCamera::getClassTypeId() &&
            stype == SoOrthographicCamera::getClassTypeId()) {
        convertOrtho2Perspective(dynamic_cast<SoOrthographicCamera*>(m_storedcamera),
                                 dynamic_cast<SoPerspectiveCamera*>(getSoRenderManager()->getCamera()));
    }

    // otherwise, cameras have changed in ways we don't understand since
    // the last saveHomePosition() invocation, and so we're just going
    // to ignore the reset request
}


void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::draw2DString(
    const char* str,
    SbVec2s glsize,
    SbVec2f position,
    Base::Color color = Base::Color(1.0F, 1.0F, 0.0F))  // retains yellow as default color
{
    if (!str || !str[0] || glsize[0] <= 0 || glsize[1] <= 0) {
        return;
    }

    GLint viewport[4] = {0, 0, 0, 0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    const int viewportWidth = viewport[2];
    const int viewportHeight = viewport[3];
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return;
    }

    auto& overlay = overlayTextState();
    overlay.ensureCreated();
    if (!overlay.root || !overlay.camera || !overlay.translation || !overlay.text || !overlay.baseColor) {
        return;
    }

    overlay.camera->aspectRatio.setValue(static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight));
    overlay.camera->height.setValue(static_cast<float>(viewportHeight));

    const float x = (position[0] / static_cast<float>(glsize[0])) * static_cast<float>(viewportWidth);
    const float y = (position[1] / static_cast<float>(glsize[1])) * static_cast<float>(viewportHeight);
    overlay.translation->translation.setValue(
        x - 0.5f * static_cast<float>(viewportWidth),
        y - 0.5f * static_cast<float>(viewportHeight),
        0.0f
    );

    overlay.baseColor->rgb.setValue(color.r, color.g, color.b);
    overlay.text->string.setValue(str);

    SoGLRenderAction action(SbViewportRegion(viewportWidth, viewportHeight));
    action.setCacheContext(this->getCacheContextId());
    action.setTransparencyType(SoGLRenderAction::BLEND);
    action.apply(overlay.root);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::moveCameraScreen(const SbVec2f& screenpos)
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    assert(cam);


    SbViewVolume vv = cam->getViewVolume(getGLWidget()->width() / getGLWidget()->height());
    SbPlane panplane = vv.getPlane(cam->focalDistance.getValue());

    constexpr const float mid = 0.5F;
    SbLine line;
    vv.projectPointToLine(screenpos + SbVec2f(mid, mid), line);
    SbVec3f current_planept;
    panplane.intersect(line, current_planept);
    vv.projectPointToLine(SbVec2f(mid, mid), line);
    SbVec3f old_planept;
    panplane.intersect(line, old_planept);

    // Reposition camera according to the vector difference between the
    // projected points.
    cam->position = cam->position.getValue() - (current_planept - old_planept);
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::processSoEvent(const SoEvent* event)
{
    const SoType type(event->getTypeId());

    constexpr const float delta = 0.1F;
    if(type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent* keyevent = static_cast<const SoKeyboardEvent*>(event);  // NOLINT

        if(keyevent->getState() == SoButtonEvent::DOWN) {
            switch(keyevent->getKey()) {

            case SoKeyboardEvent::LEFT_ARROW:
                moveCameraScreen(SbVec2f(-delta, 0.0F));
                return true;

            case SoKeyboardEvent::UP_ARROW:
                moveCameraScreen(SbVec2f(0.0F, delta));
                return true;

            case SoKeyboardEvent::RIGHT_ARROW:
                moveCameraScreen(SbVec2f(delta, 0.0F));
                return true;

            case SoKeyboardEvent::DOWN_ARROW:
                moveCameraScreen(SbVec2f(0.0F, -delta));
                return true;

            default:
                break;
            }
        }
    }

    return SIM::Coin3D::Quarter::QuarterWidget::processSoEvent(event);
}

/*!
  Overridden from QuarterWidget to render the scenegraph
*/
void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::paintEvent(QPaintEvent* event)
{
    double start = SbTime::getTimeOfDay().getValue();
    QuarterWidget::paintEvent(event);
    this->framesPerSecond = addFrametime(start);

#ifdef BUILD_TRACY_FRAME_PROFILER
    FrameMark;
#endif
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::resetFrameCounter()
{
    this->framecount = 0;
    this->frametime = 0.0F;
    this->drawtime = 0.0F;
    this->starttime = SbTime::getTimeOfDay().getValue();
    this->framesPerSecond = SbVec2f(0, 0);
}

SbVec2f SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addFrametime(double starttime)
{
    constexpr const double FPS_FACTOR = 0.7;
    constexpr const double FIVE_SECS = 5000.0;
    constexpr const float ONE_SEC = 1000.0F;

    this->framecount++;

    double timeofday = SbTime::getTimeOfDay().getValue();

    // draw time is the actual time spent on rendering
    double drawtime = timeofday - starttime;
    this->drawtime = (drawtime*FPS_FACTOR) + this->drawtime*(1.0 - FPS_FACTOR);

    // frame time is the time spent since the last frame. There could an
    // indefinite pause between the last frame because the scene is not
    // changing. So we limit the skew to 5 second.
    double frametime = std::min(timeofday-this->starttime, std::max(drawtime, FIVE_SECS));
    this->frametime = (frametime*FPS_FACTOR) + this->frametime*(1.0 - FPS_FACTOR);

    this->starttime = timeofday;
    return {ONE_SEC * float(this->drawtime), 1.0F / float(this->frametime)};
}
// NOLINTEND(readability-implicit-bool-conversion)

#include "moc_SoQTQuarterAdaptor.cpp"
