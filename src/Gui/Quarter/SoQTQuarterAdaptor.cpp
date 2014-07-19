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

#include "SoQTQuarterAdaptor.h"
#include "PreCompiled.h"
#include <Base/Console.h>
#include <Inventor/Qt/SoQt.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoLocateHighlight.h>
#include <Inventor/SoEventManager.h>
#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/actions/SoRayPickAction.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/SoPickedPoint.h>


SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QWidget* parent, const QGLWidget* sharewidget, Qt::WindowFlags f)
    : QuarterWidget(parent, sharewidget, f), matrixaction(SbViewportRegion(100,100))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
    : QuarterWidget(format, parent, shareWidget, f), matrixaction(SbViewportRegion(100,100))
{
    init();
}

SIM::Coin3D::Quarter::SoQTQuarterAdaptor::SoQTQuarterAdaptor(QGLContext* context, QWidget* parent, const QGLWidget* sharewidget, Qt::WindowFlags f)
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
    m_seekdistanceabs = FALSE;
    m_seekperiod = 2.0f;
    m_inseekmode = FALSE;

    m_seeksensor = new SoTimerSensor(SoQTQuarterAdaptor::seeksensorCB, (void*)this);
    getSoEventManager()->setNavigationState(SoEventManager::NO_NAVIGATION);
}


QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getWidget()
{
    //we keep the function from SoQt as we want to introduce the QGraphicsView and then the GLWidget
    //is seperated from the Widget used in layouts again
    return this;
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getGLWidget()
{
    return this;
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getWidget() const
{
    //we keep the function from SoQt as we want to introduce the QGraphicsView and then the GLWidget
    //is seperated from the Widget used in layouts again
    return const_cast<SoQTQuarterAdaptor*>(this);
}

QWidget* SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getGLWidget() const
{
    return const_cast<SoQTQuarterAdaptor*>(this);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::setCameraType(SoType type)
{
    if(!getSoRenderManager()->getCamera()->isOfType(SoPerspectiveCamera::getClassTypeId()) &&
            !getSoRenderManager()->getCamera()->isOfType(SoOrthographicCamera::getClassTypeId())) {
        Base::Console().Warning("Quarter::setCameraType",
                                "Only SoPerspectiveCamera and SoOrthographicCamera is supported.");
        return;
    }


    SoType perspectivetype = SoPerspectiveCamera::getClassTypeId();
    SoType orthotype = SoOrthographicCamera::getClassTypeId();
    SbBool oldisperspective = getSoRenderManager()->getCamera()->getTypeId().isDerivedFrom(perspectivetype);
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
    SoCamera* camera = NULL;
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
};

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
};

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
};

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
    return m_seekdistanceabs ? FALSE : TRUE;
}

SbBool SIM::Coin3D::Quarter::SoQTQuarterAdaptor::seekToPoint(const SbVec2s screenpos)
{

    SoRayPickAction rpaction(getSoRenderManager()->getViewportRegion());
    rpaction.setPoint(screenpos);
    rpaction.setRadius(2);
    rpaction.apply(getSoRenderManager()->getSceneGraph());

    SoPickedPoint* picked = rpaction.getPickedPoint();

    if(!picked) {
        this->interactiveCountInc(); // decremented in setSeekMode(FALSE)
        this->setSeekMode(FALSE);
        return FALSE;
    }

    SbVec3f hitpoint;
    hitpoint = picked->getPoint();

    this->seekToPoint(hitpoint);
    return TRUE;
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
    m_seekdistanceabs = on ? FALSE : TRUE;
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::getCameraCoordinateSystem(SoCamera* camera, SoNode* root, SbMatrix& matrix, SbMatrix& inverse)
{
    searchaction.reset();
    searchaction.setSearchingAll(TRUE);
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

    if(end) thisp->setSeekMode(FALSE);
}

void SIM::Coin3D::Quarter::SoQTQuarterAdaptor::saveHomePosition(void)
{
    SoType t = getSoRenderManager()->getCamera()->getTypeId();
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
    // the last saveHomePosition() invokation, and so we're just going
    // to ignore the reset request
}