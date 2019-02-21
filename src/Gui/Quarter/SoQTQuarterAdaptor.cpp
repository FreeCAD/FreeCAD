/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright (C) 2014  Stefan Tröger <stefantroeger@gmx.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "PreCompiled.h"
#include "SoQTQuarterAdaptor.h"
#include <Base/Console.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoLocateHighlight.h>
#include <Inventor/SoEventManager.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/actions/SoHandleEventAction.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>

#if !defined(FC_OS_MACOSX)
# include <GL/gl.h>
# include <GL/glu.h>
# include <GL/glext.h>
#endif

static unsigned char fps2dfont[][12] = {
    {  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, //
    {  0,  0, 12, 12,  0,  8, 12, 12, 12, 12, 12,  0 }, // !
    {  0,  0,  0,  0,  0,  0,  0,  0,  0, 20, 20, 20 }, // \"
    {  0,  0, 18, 18, 18, 63, 18, 18, 63, 18, 18,  0 }, // #
    {  0,  8, 28, 42, 10, 10, 12, 24, 40, 42, 28,  8 }, // $
    {  0,  0,  6, 73, 41, 22,  8, 52, 74, 73, 48,  0 }, // %
    {  0, 12, 18, 18, 12, 25, 37, 34, 34, 29,  0,  0 }, // &
    { 12, 12, 24,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, // '
    {  0,  6,  8,  8, 16, 16, 16, 16, 16,  8,  8,  6 }, // (
    {  0, 48,  8,  8,  4,  4,  4,  4,  4,  8,  8, 48 }, //)
    {  0,  0,  0,  0,  0,  0,  8, 42, 20, 42,  8,  0 }, // *
    {  0,  0,  0,  8,  8,  8,127,  8,  8,  8,  0,  0 }, // +
    {  0, 24, 12, 12,  0,  0,  0,  0,  0,  0,  0,  0 }, // ,
    {  0,  0,  0,  0,  0,  0,127,  0,  0,  0,  0,  0 }, // -
    {  0,  0, 24, 24,  0,  0,  0,  0,  0,  0,  0,  0 }, // .
    {  0, 32, 32, 16, 16,  8,  8,  8,  4,  4,  2,  2 }, // /
    {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 28,  0 }, // 0
    {  0,  0,  8,  8,  8,  8,  8,  8, 40, 24,  8,  0 }, // 1
    {  0,  0, 62, 32, 16,  8,  4,  2,  2, 34, 28,  0 }, // 2
    {  0,  0, 28, 34,  2,  2, 12,  2,  2, 34, 28,  0 }, // 3
    {  0,  0,  4,  4,  4,126, 68, 36, 20, 12,  4,  0 }, // 4
    {  0,  0, 28, 34,  2,  2,  2, 60, 32, 32, 62,  0 }, // 5
    {  0,  0, 28, 34, 34, 34, 60, 32, 32, 34, 28,  0 }, // 6
    {  0,  0, 16, 16, 16,  8,  8,  4,  2,  2, 62,  0 }, // 7
    {  0,  0, 28, 34, 34, 34, 28, 34, 34, 34, 28,  0 }, // 8
    {  0,  0, 28, 34,  2,  2, 30, 34, 34, 34, 28,  0 }, // 9
    {  0,  0, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, // :
    {  0, 48, 24, 24,  0,  0,  0, 24, 24,  0,  0,  0 }, // ;
    {  0,  0,  0,  2,  4,  8, 16,  8,  4,  2,  0,  0 }, // <
    {  0,  0,  0,  0,  0,127,  0,127,  0,  0,  0,  0 }, // =
    {  0,  0,  0, 16,  8,  4,  2,  4,  8, 16,  0,  0 }, // >
    {  0,  0, 16, 16,  0, 16, 28,  2,  2,  2, 60,  0 }, // ?
    {  0,  0, 28, 32, 73, 86, 82, 82, 78, 34, 28,  0 }, // @
    {  0,  0, 33, 33, 33, 63, 18, 18, 18, 12, 12,  0 }, // A
    {  0,  0, 60, 34, 34, 34, 60, 34, 34, 34, 60,  0 }, // B
    {  0,  0, 14, 16, 32, 32, 32, 32, 32, 18, 14,  0 }, // C
    {  0,  0, 56, 36, 34, 34, 34, 34, 34, 36, 56,  0 }, // D
    {  0,  0, 62, 32, 32, 32, 60, 32, 32, 32, 62,  0 }, // E
    {  0,  0, 16, 16, 16, 16, 30, 16, 16, 16, 30,  0 }, // F
    {  0,  0, 14, 18, 34, 34, 32, 32, 32, 18, 14,  0 }, // G
    {  0,  0, 34, 34, 34, 34, 62, 34, 34, 34, 34,  0 }, // H
    {  0,  0, 62,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, // I
    {  0,  0,112,  8,  8,  8,  8,  8,  8,  8, 62,  0 }, // J
    {  0,  0, 33, 33, 34, 36, 56, 40, 36, 34, 33,  0 }, // K
    {  0,  0, 30, 16, 16, 16, 16, 16, 16, 16, 16,  0 }, // L
    {  0,  0, 33, 33, 33, 45, 45, 45, 51, 51, 33,  0 }, // M
    {  0,  0, 34, 34, 38, 38, 42, 42, 50, 50, 34,  0 }, // N
    {  0,  0, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, // O
    {  0,  0, 32, 32, 32, 60, 34, 34, 34, 34, 60,  0 }, // P
    {  3,  6, 12, 18, 33, 33, 33, 33, 33, 18, 12,  0 }, // Q
    {  0,  0, 34, 34, 34, 36, 60, 34, 34, 34, 60,  0 }, // R
    {  0,  0, 60,  2,  2,  6, 28, 48, 32, 32, 30,  0 }, // S
    {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,127,  0 }, // T
    {  0,  0, 28, 34, 34, 34, 34, 34, 34, 34, 34,  0 }, // U
    {  0,  0, 12, 12, 18, 18, 18, 33, 33, 33, 33,  0 }, // V
    {  0,  0, 34, 34, 34, 54, 85, 73, 73, 73, 65,  0 }, // W
    {  0,  0, 34, 34, 20, 20,  8, 20, 20, 34, 34,  0 }, // X
    {  0,  0,  8,  8,  8,  8, 20, 20, 34, 34, 34,  0 }, // Y
    {  0,  0, 62, 32, 16, 16,  8,  4,  4,  2, 62,  0 }, // Z
    {  0, 14,  8,  8,  8,  8,  8,  8,  8,  8,  8, 14 }, // [
    {  0,  2,  2,  4,  4,  8,  8,  8, 16, 16, 32, 32 }, // [backslash]
    {  0, 56,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, // ]
    {  0,  0,  0,  0,  0, 34, 34, 20, 20,  8,  8,  0 }, // ^
    {  0,127,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 }, // _
    {  0,  0,  0,  0,  0,  0,  0,  0,  0, 24, 24, 12 }, // `
    {  0,  0, 29, 34, 34, 30,  2, 34, 28,  0,  0,  0 }, // a
    {  0,  0, 60, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, // b
    {  0,  0, 14, 16, 32, 32, 32, 16, 14,  0,  0,  0 }, // c
    {  0,  0, 26, 38, 34, 34, 34, 34, 30,  2,  2,  2 }, // d
    {  0,  0, 28, 34, 32, 62, 34, 34, 28,  0,  0,  0 }, // e
    {  0,  0, 16, 16, 16, 16, 16, 16, 62, 16, 16, 14 }, // f
    { 28,  2,  2, 26, 38, 34, 34, 34, 30,  0,  0,  0 }, // g
    {  0,  0, 34, 34, 34, 34, 34, 50, 44, 32, 32, 32 }, // h
    {  0,  0,  8,  8,  8,  8,  8,  8, 56,  0,  8,  8 }, // i
    { 56,  4,  4,  4,  4,  4,  4,  4, 60,  0,  4,  4 }, // j
    {  0,  0, 33, 34, 36, 56, 40, 36, 34, 32, 32, 32 }, // k
    {  0,  0,  8,  8,  8,  8,  8,  8,  8,  8,  8, 56 }, // l
    {  0,  0, 73, 73, 73, 73, 73,109, 82,  0,  0,  0 }, // m
    {  0,  0, 34, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, // n
    {  0,  0, 28, 34, 34, 34, 34, 34, 28,  0,  0,  0 }, // o
    { 32, 32, 60, 34, 34, 34, 34, 50, 44,  0,  0,  0 }, // p
    {  2,  2, 26, 38, 34, 34, 34, 34, 30,  0,  0,  0 }, // q
    {  0,  0, 16, 16, 16, 16, 16, 24, 22,  0,  0,  0 }, // r
    {  0,  0, 60,  2,  2, 28, 32, 32, 30,  0,  0,  0 }, // s
    {  0,  0, 14, 16, 16, 16, 16, 16, 62, 16, 16,  0 }, // t
    {  0,  0, 26, 38, 34, 34, 34, 34, 34,  0,  0,  0 }, // u
    {  0,  0,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, // v
    {  0,  0, 34, 34, 34, 85, 73, 73, 65,  0,  0,  0 }, // w
    {  0,  0, 34, 34, 20,  8, 20, 34, 34,  0,  0,  0 }, // x
    { 48, 16,  8,  8, 20, 20, 34, 34, 34,  0,  0,  0 }, // y
    {  0,  0, 62, 32, 16,  8,  4,  2, 62,  0,  0,  0 }, // z
    {  0,  6,  8,  8,  8,  4, 24,  4,  8,  8,  8,  6 }, // {
    {  0,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8,  8 }, // |
    {  0, 48,  8,  8,  8, 16, 12, 16,  8,  8,  8, 48 }, // }
    {  0,  0,  0,  0,  0,  0, 78, 57,  0,  0,  0,  0 }  // ~
};

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QWidget* parent, const QtGLWidget* sharewidget, Qt::WindowFlags f)
    : QuarterWidget(parent, sharewidget, f), matrixaction(SbViewportRegion(100,100))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(const QtGLFormat& format, QWidget* parent, const QtGLWidget* shareWidget, Qt::WindowFlags f)
    : QuarterWidget(format, parent, shareWidget, f), matrixaction(SbViewportRegion(100,100))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QtGLContext* context, QWidget* parent, const QtGLWidget* sharewidget, Qt::WindowFlags f)
    : QuarterWidget(context, parent, sharewidget, f), matrixaction(SbViewportRegion(100,100))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::~SoQTQuarterAdaptor()
{
    delete m_seeksensor;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::init()
{
    m_interactionnesting = 0;
    m_seekdistance = 50.0f;
    m_seekdistanceabs = false;
    m_seekperiod = 2.0f;
    m_inseekmode = false;
    m_storedcamera = 0;
    m_viewingflag = false;
    pickRadius = 5.0;

    m_seeksensor = new SoTimerSensor(SoQTQuarterAdaptor::seeksensorCB, (void*)this);
    getSoEventManager()->setNavigationState(SoEventManager::NO_NAVIGATION);

    resetFrameCounter();
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
    return const_cast<SoQTQuarterAdaptor*>(this);
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getGLWidget() const
{
    return const_cast<QWidget*>(viewport());
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setCameraType(SoType type)
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (cam && !cam->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
               !cam->isOfType(SoOrthographicCamera::getClassTypeId())) {
        Base::Console().Warning("Quarter::setCameraType",
                                "Only SoPerspectiveCamera and SoOrthographicCamera is supported.");
        return;
    }


    SoType perspectivetype = SoPerspectiveCamera::getClassTypeId();
    SbBool oldisperspective = cam ? cam->getTypeId().isDerivedFrom(perspectivetype) : false;
    SbBool newisperspective = type.isDerivedFrom(perspectivetype);

    if((oldisperspective && newisperspective) ||
       (!oldisperspective && !newisperspective)) // Same old, same old..
        return;


    SoCamera* currentcam = getSoRenderManager()->getCamera();
    SoCamera* newcamera = (SoCamera*)type.createInstance();

    // Transfer and convert values from one camera type to the other.
    if(newisperspective) {
        convertOrtho2Perspective((SoOrthographicCamera*)currentcam,
                                 (SoPerspectiveCamera*)newcamera);
    }
    else {
        convertPerspective2Ortho((SoPerspectiveCamera*)currentcam,
                                 (SoOrthographicCamera*)newcamera);
    }

    getSoRenderManager()->setCamera(newcamera);
    getSoEventManager()->setCamera(newcamera);

    //if the superscene has a camera we need to replace it too
    SoSeparator* superscene = (SoSeparator*) getSoRenderManager()->getSceneGraph();
    SoSearchAction sa;
    sa.setInterest(SoSearchAction::FIRST);
    sa.setType(SoCamera::getClassTypeId());
    sa.apply(superscene);

    if(sa.getPath()) {
        SoNode* node = sa.getPath()->getTail();
        SoGroup* parent = (SoGroup*) sa.getPath()->getNodeFromTail(1);

        if(node && node->isOfType(SoCamera::getClassTypeId())) {
            parent->replaceChild(node, newcamera);
        }
    }
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::convertOrtho2Perspective(const SoOrthographicCamera* in,
        SoPerspectiveCamera* out)
{
    out->aspectRatio.setValue(in->aspectRatio.getValue());
    out->focalDistance.setValue(in->focalDistance.getValue());
    out->orientation.setValue(in->orientation.getValue());
    out->position.setValue(in->position.getValue());
    out->viewportMapping.setValue(in->viewportMapping.getValue());

    SbRotation camrot = in->orientation.getValue();

    float focaldist = in->height.getValue() / (2.0*tan(M_PI / 8.0));

    SbVec3f offset(0,0,focaldist-in->focalDistance.getValue());

    camrot.multVec(offset,offset);
    out->position.setValue(offset+in->position.getValue());

    out->focalDistance.setValue(focaldist);

    // 45° is the default value of this field in SoPerspectiveCamera.
    out->heightAngle = (float)(M_PI / 4.0);
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

    out->height = 2.0f * focaldist * (float)tan(in->heightAngle.getValue() / 2.0);
}

SoCamera* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getCamera(void) const
{
    return getSoRenderManager()->getCamera();
}

const SbViewportRegion & SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getViewportRegion(void) const
{
    return getSoRenderManager()->getViewportRegion();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setViewing(SbBool enable)
{

    m_viewingflag = enable;

    // Turn off the selection indicators when we go back from picking
    // mode into viewing mode.
    if(m_viewingflag) {
        SoGLRenderAction* action = getSoRenderManager()->getGLRenderAction();

        if(action != NULL)
            SoLocateHighlight::turnOffCurrentHighlight(action);
    }
}

SbBool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isViewing(void) const
{
    return m_viewingflag;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::interactiveCountInc(void)
{
    // Catch problems with missing interactiveCountDec() calls.
    assert(m_interactionnesting < 100);

    if(++m_interactionnesting == 1) {
        m_interactionStartCallback.invokeCallbacks(this);
    }
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::interactiveCountDec(void)
{
    if(--m_interactionnesting <= 0) {
        m_interactionEndCallback.invokeCallbacks(this);
        m_interactionnesting = 0;
    }
}

int SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getInteractiveCount(void) const
{
    return m_interactionnesting;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addStartCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionStartCallback.addCallback((SoCallbackListCB*)func, data);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::removeStartCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionStartCallback.removeCallback((SoCallbackListCB*)func, data);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addFinishCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionEndCallback.addCallback((SoCallbackListCB*)func, data);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::removeFinishCallback(SIM::Coin3D::Quarter::SoQTQuarterAdaptorCB* func, void* data)
{
    m_interactionEndCallback.removeCallback((SoCallbackListCB*)func, data);
}


float SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getSeekDistance(void) const
{
    return m_seekdistance;
}

float SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getSeekTime(void) const
{
    return m_seekperiod;
}

SbBool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isSeekMode(void) const
{
    return m_inseekmode;
}

SbBool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::isSeekValuePercentage(void) const
{
    return m_seekdistanceabs ? false : true;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setPickRadius(float pickRadius)
{
    this->pickRadius = pickRadius;
    SoEventManager* evm = this->getSoEventManager();
    if (evm){
        SoHandleEventAction* a = evm->getHandleEventAction();
        if (a){
            a->setPickRadius(pickRadius);
        }
    }
}

SbBool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seekToPoint(const SbVec2s screenpos)
{

    SoRayPickAction rpaction(getSoRenderManager()->getViewportRegion());
    rpaction.setPoint(screenpos);
    rpaction.setRadius(pickRadius);
    rpaction.apply(getSoRenderManager()->getSceneGraph());

    SoPickedPoint* picked = rpaction.getPickedPoint();

    if(!picked) {
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
    SbMatrix cameramatrix, camerainverse;
    getCameraCoordinateSystem(getSoRenderManager()->getCamera(),
                              getSceneGraph(),
                              cameramatrix,
                              camerainverse);
    camerainverse.multVecMatrix(hitpoint, hitpoint);

    float fd = m_seekdistance;

    if(!m_seekdistanceabs)
        fd *= (hitpoint - getSoRenderManager()->getCamera()->position.getValue()).length()/100.0f;

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

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekMode(SbBool enable)
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

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setSeekValueAsPercentage(const SbBool on)
{
    m_seekdistanceabs = on ? false : true;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getCameraCoordinateSystem(SoCamera* camera, SoNode* root, SbMatrix& matrix, SbMatrix& inverse)
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

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seeksensorCB(void* data, SoSensor* s)
{
    SoQTQuarterAdaptor* thisp = (SoQTQuarterAdaptor*) data;
    SbTime currenttime = SbTime::getTimeOfDay();

    SoTimerSensor* sensor = (SoTimerSensor*)s;

    float t =
        float((currenttime - sensor->getBaseTime()).getValue()) / thisp->m_seekperiod;

    if((t > 1.0f) || (t + sensor->getInterval().getValue() > 1.0f)) t = 1.0f;

    SbBool end = (t == 1.0f);

    t = (float)((1.0 - cos(M_PI*t)) * 0.5);

    thisp->getSoRenderManager()->getCamera()->position = thisp->m_camerastartposition +
            (thisp->m_cameraendposition - thisp->m_camerastartposition) * t;
    thisp->getSoRenderManager()->getCamera()->orientation =
        SbRotation::slerp(thisp->m_camerastartorient,
                          thisp->m_cameraendorient,
                          t);

    if(end) thisp->setSeekMode(false);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::saveHomePosition(void)
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    SoType t = cam->getTypeId();
    assert(t.isDerivedFrom(SoNode::getClassTypeId()));
    assert(t.canCreateInstance());

    if(m_storedcamera) {
        m_storedcamera->unref();
    }

    m_storedcamera = (SoNode*)t.createInstance();
    m_storedcamera->ref();

    m_storedcamera->copyFieldValues(getSoRenderManager()->getCamera());
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::resetToHomePosition(void)
{
    SoCamera* cam = getSoRenderManager()->getCamera();
    if (!cam) {
        return;
    }

    if(!m_storedcamera) {
        return;
    }

    SoType t = getSoRenderManager()->getCamera()->getTypeId();
    SoType s = m_storedcamera->getTypeId();

    // most common case
    if(t == s) {
        // We copy the field data directly, instead of using
        // SoFieldContainer::copyContents(), for the reason described in
        // detail in So@Gui@Viewer::saveHomePosition().
        getSoRenderManager()->getCamera()->copyFieldValues(m_storedcamera);
    }
    // handle common case #1
    else if(t == SoOrthographicCamera::getClassTypeId() &&
            s == SoPerspectiveCamera::getClassTypeId()) {
        convertPerspective2Ortho((SoPerspectiveCamera*)m_storedcamera,
                                 (SoOrthographicCamera*)getSoRenderManager()->getCamera());
    }
    // handle common case #2
    else if(t == SoPerspectiveCamera::getClassTypeId() &&
            s == SoOrthographicCamera::getClassTypeId()) {
        convertOrtho2Perspective((SoOrthographicCamera*)m_storedcamera,
                                 (SoPerspectiveCamera*)getSoRenderManager()->getCamera());
    }

    // otherwise, cameras have changed in ways we don't understand since
    // the last saveHomePosition() invocation, and so we're just going
    // to ignore the reset request
}


void
SIM::Coin3D::Quarter::SoQTQuarterAdaptor::draw2DString(const char* str, SbVec2s glsize, SbVec2f position)
{
    // Store GL state.
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_TEXTURE_2D);
    glDisable(GL_BLEND);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0, glsize[0], 0.0, glsize[1], -1, 1);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

//   glColor3f(0.0, 0.0, 0.0);
//   glRasterPos2f(position[0] + 1, position[1]);
//   printString(str);
//   glRasterPos2f(position[0] - 1, position[1]);
//   printString(str);
//   glRasterPos2f(position[0], position[1] + 1);
//   printString(str);
//   glRasterPos2f(position[0], position[1] - 1);
//   printString(str);

    glColor3f(1.0, 1.0, 0.0);
    glRasterPos2f(position[0], position[1]);
    printString(str);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // restore default value

    glPopAttrib();
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::printString(const char* s)
{
    int i,n;
    n = strlen(s);

    for(i = 0; i < n; i++)
        glBitmap(8, 12, 0.0, 2.0, 10.0, 0.0, fps2dfont[s[i] - 32]);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::moveCameraScreen(const SbVec2f& screenpos) {

    SoCamera* cam = getSoRenderManager()->getCamera();
    assert(cam);


    SbViewVolume vv = cam->getViewVolume(getGLWidget()->width() / getGLWidget()->height());
    SbPlane panplane = vv.getPlane(cam->focalDistance.getValue());

    SbLine line;
    vv.projectPointToLine(screenpos + SbVec2f(0.5, 0.5f), line);
    SbVec3f current_planept;
    panplane.intersect(line, current_planept);
    vv.projectPointToLine(SbVec2f(0.5f, 0.5f), line);
    SbVec3f old_planept;
    panplane.intersect(line, old_planept);

    // Reposition camera according to the vector difference between the
    // projected points.
    cam->position = cam->position.getValue() - (current_planept - old_planept);
}

bool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::processSoEvent(const SoEvent* event) {

    const SoType type(event->getTypeId());

    if(type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent* keyevent = static_cast<const SoKeyboardEvent*>(event);

        if(keyevent->getState() == SoButtonEvent::DOWN) {
            switch(keyevent->getKey()) {

            case SoKeyboardEvent::LEFT_ARROW:
                moveCameraScreen(SbVec2f(-0.1f, 0.0f));
                return true;

            case SoKeyboardEvent::UP_ARROW:
                moveCameraScreen(SbVec2f(0.0f, 0.1f));
                return true;

            case SoKeyboardEvent::RIGHT_ARROW:
                moveCameraScreen(SbVec2f(0.1f, 0.0f));
                return true;

            case SoKeyboardEvent::DOWN_ARROW:
                moveCameraScreen(SbVec2f(0.0f, -0.1f));
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
    QuarterWidget::paintEvent(event);

    this->framesPerSecond = addFrametime(SbTime::getTimeOfDay().getValue());
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::resetFrameCounter(void)
{
    this->framecount = 0;
    this->frames.assign(100, 0.0f);
    this->totaldraw = 0.0f;
    this->starttime = SbTime::getTimeOfDay().getValue();
    this->framesPerSecond = SbVec2f(0, 0);
}

SbVec2f SIM::Coin3D::Quarter::SoQTQuarterAdaptor::addFrametime(double timeofday)
{
    int framearray_size = 100;
    this->framecount++;

    int arrayptr = (this->framecount - 1) % framearray_size;

    double renderTime = timeofday - this->starttime;
    this->totaldraw += (float(renderTime) - this->frames[arrayptr]);
    float drawfps = this->totaldraw / std::min<int>(this->framecount, framearray_size);

    this->frames[arrayptr] = static_cast<float>(renderTime);
    this->starttime = timeofday;
    return SbVec2f(1000 * drawfps, 1.0f / drawfps);
}
