/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <cfloat>
# include "InventorAll.h"
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <QByteArray>
# include <QCursor>
# include <QList>
# include <QMenu>
# include <QMetaObject>
# include <QRegExp>
#endif

#include <Inventor/sensors/SoTimerSensor.h>

#include <App/Application.h>
#include "NavigationStyle.h"
#include "View3DInventorViewer.h"
#include "Application.h"
#include "MenuManager.h"
#include "MouseSelection.h"

using namespace Gui;

namespace Gui {
struct NavigationStyleP {
    int animationsteps;
    int animationdelta;
    SbVec3f focal1, focal2;
    SbVec3f startDragPoint;
    SbBool dragPointFound;
    SbBool dragAtCursor;
    SbRotation endRotation;
    SoTimerSensor * animsensor;
    float sensitivity;
    SbBool resetcursorpos;

    NavigationStyleP()
    {
        this->animationsteps = 0;
        this->animationdelta = 0;
        this->animsensor = 0;
        this->sensitivity = 2.0f;
        this->resetcursorpos = false;
        this->dragPointFound = false;
        this->dragAtCursor = false;
    }
    static void viewAnimationCB(void * data, SoSensor * sensor);
};
}

class FCSphereSheetProjector : public SbSphereSheetProjector {
    typedef SbSphereSheetProjector inherited;

public:
    enum OrbitStyle {
        Turntable,
        Trackball
    };

    FCSphereSheetProjector(const SbSphere & sph, const SbBool orienttoeye = true)
        : SbSphereSheetProjector(sph, orienttoeye), orbit(Trackball)
    {
    }

    void setViewVolume (const SbViewVolume &vol)
    {
        inherited::setViewVolume(vol);
    }

    void setWorkingSpace (const SbMatrix &space)
    {
        //inherited::setWorkingSpace(space);
        this->worldToScreen = space.inverse();
    }

    SbVec3f project(const SbVec2f &point)
    {
        return inherited::project(point);
    }

    SbRotation getRotation(const SbVec3f &point1, const SbVec3f &point2)
    {
        SbRotation rot = inherited::getRotation(point1, point2);
        if (orbit == Trackball)
            return rot;

        // 0000333: Turntable camera rotation
        SbVec3f axis;
        float angle;
        rot.getValue(axis, angle);
        SbVec3f dif = point1 - point2;
        if (fabs(dif[1]) > fabs(dif[0])) {
            SbVec3f xaxis(1,0,0);
            if (dif[1] < 0)
                angle = -angle;
            rot.setValue(xaxis, angle);
        }
        else {
            SbVec3f zaxis(0,0,1);
            this->worldToScreen.multDirMatrix(zaxis, zaxis);
            if (zaxis[1] < 0) {
                if (dif[0] < 0)
                    angle = -angle;
            }
            else {
                if (dif[0] > 0)
                    angle = -angle;
            }
            rot.setValue(zaxis, angle);
        }

        return rot;
    }

    void setOrbitStyle(OrbitStyle style)
    {
        this->orbit = style;
    }

    OrbitStyle getOrbitStyle() const
    {
        return this->orbit;
    }

private:
    SbMatrix worldToScreen;
    OrbitStyle orbit;
};

NavigationStyleEvent::NavigationStyleEvent(const Base::Type& s)
  : QEvent(QEvent::User), t(s)
{
}

NavigationStyleEvent::~NavigationStyleEvent()
{
}

const Base::Type& NavigationStyleEvent::style() const
{ 
    return t;
}

#define PRIVATE(ptr) (ptr->pimpl)
#define PUBLIC(ptr) (ptr->pub)

TYPESYSTEM_SOURCE_ABSTRACT(Gui::NavigationStyle,Base::BaseClass);

NavigationStyle::NavigationStyle() : viewer(0), mouseSelection(0)
{
    PRIVATE(this) = new NavigationStyleP();
    PRIVATE(this)->animsensor = new SoTimerSensor(NavigationStyleP::viewAnimationCB, this);
    initialize();
}

NavigationStyle::~NavigationStyle()
{
    finalize();
    if (PRIVATE(this)->animsensor->isScheduled())
        PRIVATE(this)->animsensor->unschedule();
    delete PRIVATE(this)->animsensor;
    delete PRIVATE(this);
}

NavigationStyle& NavigationStyle::operator = (const NavigationStyle& ns)
{
    this->panningplane = ns.panningplane;
    this->menuenabled = ns.menuenabled;
    this->spinanimatingallowed = ns.spinanimatingallowed;
    static_cast<FCSphereSheetProjector*>(this->spinprojector)->setOrbitStyle
        (static_cast<FCSphereSheetProjector*>(ns.spinprojector)->getOrbitStyle());
    return *this;
}

void NavigationStyle::setViewer(View3DInventorViewer* view)
{
    this->viewer = view;
}

void NavigationStyle::initialize()
{
    this->currentmode = NavigationStyle::IDLE;
    this->prevRedrawTime = SbTime::getTimeOfDay();
    this->spinanimatingallowed = true;
    this->spinsamplecounter = 0;
    this->spinincrement = SbRotation::identity();
    this->spinRotation.setValue(SbVec3f(0, 0, -1), 0);

    // FIXME: use a smaller sphere than the default one to have a larger
    // area close to the borders that gives us "z-axis rotation"?
    // 19990425 mortene.
    this->spinprojector = new FCSphereSheetProjector(SbSphere(SbVec3f(0, 0, 0), 0.8f));
    SbViewVolume volume;
    volume.ortho(-1, 1, -1, 1, -1, 1);
    this->spinprojector->setViewVolume(volume);

    this->log.size = 16;
    this->log.position = new SbVec2s [ 16 ];
    this->log.time = new SbTime [ 16 ];
    this->log.historysize = 0;

    this->menuenabled = true;
    this->button1down = false;
    this->button2down = false;
    this->button3down = false;
    this->ctrldown = false;
    this->shiftdown = false;
    this->altdown = false;
    this->invertZoom = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetBool("InvertZoom",true);
    this->zoomAtCursor = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetBool("ZoomAtCursor",true);
    this->zoomStep = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetFloat("ZoomStep",0.2f);
}

void NavigationStyle::finalize()
{
    delete this->spinprojector;
    delete[] this->log.position;
    delete[] this->log.time;
}

void NavigationStyle::interactiveCountInc(void)
{
    viewer->interactiveCountInc();
}

void NavigationStyle::interactiveCountDec(void)
{
    viewer->interactiveCountDec();
}

int NavigationStyle::getInteractiveCount(void) const
{
    return viewer->getInteractiveCount();
}

void NavigationStyle::setOrbitStyle(NavigationStyle::OrbitStyle style)
{
    FCSphereSheetProjector* projector = static_cast<FCSphereSheetProjector*>(this->spinprojector);
    projector->setOrbitStyle(FCSphereSheetProjector::OrbitStyle(style));
}

NavigationStyle::OrbitStyle NavigationStyle::getOrbitStyle() const
{
    FCSphereSheetProjector* projector = static_cast<FCSphereSheetProjector*>(this->spinprojector);
    return NavigationStyle::OrbitStyle(projector->getOrbitStyle());
}

SbBool NavigationStyle::isViewing(void) const
{
    return viewer->isViewing();
}

void NavigationStyle::setViewing(SbBool enable)
{
    viewer->setViewing(enable);
}

SbBool NavigationStyle::isSeekMode(void) const
{
    return viewer->isSeekMode();
}

void NavigationStyle::setSeekMode(SbBool enable)
{
    viewer->setSeekMode(enable);
}

SbBool NavigationStyle::seekToPoint(const SbVec2s screenpos)
{
    return viewer->seekToPoint(screenpos);
}

void NavigationStyle::seekToPoint(const SbVec3f& scenepos)
{
    viewer->seekToPoint(scenepos);
}

SbBool NavigationStyle::lookAtPoint(const SbVec2s screenpos)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (cam == 0) return false;

    SoRayPickAction rpaction(viewer->getSoRenderManager()->getViewportRegion());
    rpaction.setPoint(screenpos);
    rpaction.setRadius(viewer->getPickRadius());
    rpaction.apply(viewer->getSoRenderManager()->getSceneGraph());

    SoPickedPoint * picked = rpaction.getPickedPoint();
    if (!picked) {
        this->interactiveCountInc();
        return false;
    }

    SbVec3f hitpoint;
    hitpoint = picked->getPoint();
    lookAtPoint(hitpoint);
    return true;
}

void NavigationStyle::lookAtPoint(const SbVec3f& pos)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (cam == 0) return;
    PRIVATE(this)->dragPointFound = false;

    // Find global coordinates of focal point.
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    PRIVATE(this)->focal1 = cam->position.getValue() +
                            cam->focalDistance.getValue() * direction;
    PRIVATE(this)->focal2 = pos;

    // avoid to interfere with spinning (fixes #3101462)
    if (this->isAnimating())
        this->stopAnimating();

    if (PRIVATE(this)->animsensor->isScheduled()) {
        PRIVATE(this)->animsensor->unschedule();
        this->interactiveCountDec();
    }

    if (isAnimationEnabled()) {
        SbRotation cam_rot = cam->orientation.getValue();
        // get the amount of movement
        SbVec3f dir1 = direction, dir2;
        dir2 = pos - cam->position.getValue();
        dir2.normalize();
        SbRotation rot(dir1, dir2);
        float val = 0.5f*(1.0f + dir1.dot(dir2)); // value in range [0,1]
        int div = (int)(val * 20.0f);
        int steps = 20-div; // do it with max. 20 steps

        // check whether a movement is required
        if (steps > 0) {
            PRIVATE(this)->endRotation = cam_rot;
            this->spinRotation = cam_rot;
            PRIVATE(this)->animationsteps = 5;
            PRIVATE(this)->animationdelta = std::max<int>(100/steps, 5);
            PRIVATE(this)->animsensor->setBaseTime(SbTime::getTimeOfDay());
            PRIVATE(this)->animsensor->schedule();
            this->interactiveCountInc();
        }
        else {
            // set to the given position
            SbVec3f direction;
            cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
            cam->position = pos - cam->focalDistance.getValue() * direction;
        }
    }
    else {
        // set to the given position
        SbVec3f direction;
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        cam->position = pos - cam->focalDistance.getValue() * direction;
    }
}

void NavigationStyle::setCameraOrientation(const SbRotation& rot, SbBool moveToCenter)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (cam == 0) return;

    // Find global coordinates of focal point.
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    PRIVATE(this)->focal1 = cam->position.getValue() +
                            cam->focalDistance.getValue() * direction;
    PRIVATE(this)->focal2 = PRIVATE(this)->focal1;
    if (moveToCenter) {
        SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
        action.apply(viewer->getSceneGraph());
        SbBox3f box = action.getBoundingBox();
        if (!box.isEmpty()) {
            rot.multVec(SbVec3f(0, 0, -1), direction);
            //float s = (this->focal1 - box.getCenter()).dot(direction);
            //this->focal2 = box.getCenter() + s * direction;
            // setting the center of the overall bounding box as the future focal point
            // seems to be a satisfactory solution
            PRIVATE(this)->focal2 = box.getCenter();
        }
    }

    // avoid to interfere with spinning (fixes #3101462)
    if (this->isAnimating())
        this->stopAnimating();

    if (PRIVATE(this)->animsensor->isScheduled()) {
        PRIVATE(this)->animsensor->unschedule();
        this->interactiveCountDec();
    }

    if (isAnimationEnabled()) {
        // get the amount of movement
        SbVec3f dir1, dir2;
        SbRotation cam_rot = cam->orientation.getValue();
        cam_rot.multVec(SbVec3f(0, 0, -1), dir1);
        rot.multVec(SbVec3f(0, 0, -1), dir2);
        float val = 0.5f*(1.0f + dir1.dot(dir2)); // value in range [0,1]
        int div = (int)(val * 20.0f);
        int steps = 20-div; // do it with max. 20 steps

        // check whether a movement is required
        if (steps > 0) {
            PRIVATE(this)->endRotation = rot; // this is the final camera orientation
            this->spinRotation = cam_rot;
            PRIVATE(this)->animationsteps = 5;
            PRIVATE(this)->animationdelta = std::max<int>(100/steps, 5);
            PRIVATE(this)->animsensor->setBaseTime(SbTime::getTimeOfDay());
            PRIVATE(this)->animsensor->schedule();
            this->interactiveCountInc();
        }
        else {
            // due to possible round-off errors make sure that the
            // exact orientation is set
            cam->orientation.setValue(rot);
            cam->position = PRIVATE(this)->focal2 - cam->focalDistance.getValue() * direction;
        }
    }
    else {
        // set to the given rotation
        cam->orientation.setValue(rot);
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        cam->position = PRIVATE(this)->focal2 - cam->focalDistance.getValue() * direction;
    }
}

void NavigationStyleP::viewAnimationCB(void * data, SoSensor * sensor)
{
    Q_UNUSED(sensor); 
    NavigationStyle* that = reinterpret_cast<NavigationStyle*>(data);
    if (PRIVATE(that)->animationsteps > 0) {
        // here the camera rotates from the current rotation to a given
        // rotation (e.g. the standard views). To get this movement animated
        // we calculate an interpolated rotation and update the view after
        // each step
        float step = std::min<float>((float)PRIVATE(that)->animationsteps/100.0f, 1.0f);
        SbRotation slerp = SbRotation::slerp(that->spinRotation, PRIVATE(that)->endRotation, step);
        SbVec3f focalpoint = (1.0f-step)*PRIVATE(that)->focal1 + step*PRIVATE(that)->focal2;
        SoCamera* cam = that->viewer->getSoRenderManager()->getCamera();
        if (!cam) return; // no camera

        SbVec3f direction;
        cam->orientation.setValue(slerp);
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        cam->position = focalpoint - cam->focalDistance.getValue() * direction;

        PRIVATE(that)->animationsteps += PRIVATE(that)->animationdelta;
        if (PRIVATE(that)->animationsteps > 100) {
            // now we have reached the end of the movement
            PRIVATE(that)->animationsteps=0;
            PRIVATE(that)->animsensor->unschedule();
            that->interactiveCountDec();
            // set to the actual given rotation
            cam->orientation.setValue(PRIVATE(that)->endRotation);
            cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
            cam->position = PRIVATE(that)->focal2 - cam->focalDistance.getValue() * direction;
        }
    }
}

void NavigationStyle::boxZoom(const SbBox2s& box)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) return; // no camera 
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    SbViewVolume vv = cam->getViewVolume(vp.getViewportAspectRatio());

    short sizeX,sizeY;
    box.getSize(sizeX, sizeY);
    SbVec2s size = vp.getViewportSizePixels();

    // The bbox must not be empty i.e. width and length is zero, but it is possible that
    // either width or length is zero
    if (sizeX == 0 && sizeY == 0) 
        return;

    // Get the new center in normalized pixel coordinates
    short xmin,xmax,ymin,ymax;
    box.getBounds(xmin,ymin,xmax,ymax);
    const SbVec2f center((float) ((xmin+xmax)/2) / (float) std::max((int)(size[0] - 1), 1),
                         (float) (size[1]-(ymin+ymax)/2) / (float) std::max((int)(size[1] - 1), 1));

    SbPlane plane = vv.getPlane(cam->focalDistance.getValue());
    panCamera(cam,vp.getViewportAspectRatio(),plane, SbVec2f(0.5,0.5), center);

    // Set height or height angle of the camera
    float scaleX = (float)sizeX/(float)size[0];
    float scaleY = (float)sizeY/(float)size[1];
    float scale = std::max<float>(scaleX, scaleY);
    if (cam && cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        float height = static_cast<SoOrthographicCamera*>(cam)->height.getValue() * scale;
        static_cast<SoOrthographicCamera*>(cam)->height = height;
    }
    else if (cam && cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        float height = static_cast<SoPerspectiveCamera*>(cam)->heightAngle.getValue() / 2.0f;
        height = 2.0f * atan(tan(height) * scale);
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = height;
    }
}

void NavigationStyle::viewAll()
{
    // Get the bounding box of the scene
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    SbBox3f box = action.getBoundingBox();
    if (box.isEmpty()) return;

#if 0
    // check whether the box is very wide or tall, if not do nothing
    float box_width, box_height, box_depth;
    box.getSize( box_width, box_height, box_depth );
    if (box_width < 5.0f*box_height && box_width < 5.0f*box_depth && 
        box_height < 5.0f*box_width && box_height < 5.0f*box_depth && 
        box_depth < 5.0f*box_width && box_depth < 5.0f*box_height )
        return;
#endif

    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) return;

    SbViewVolume  vol = cam->getViewVolume();
    if (vol.ulf == vol.llf)
        return; // empty frustum (no view up vector defined)
    SbVec2f s = vol.projectBox(box);
    SbVec2s size = viewer->getSoRenderManager()->getSize();

    SbVec3f pt1, pt2, pt3, tmp;
    vol.projectPointToLine( SbVec2f(0.0f,0.0f), pt1, tmp );
    vol.projectPointToLine( SbVec2f(s[0],0.0f), pt2, tmp );
    vol.projectPointToLine( SbVec2f(0.0f,s[1]), pt3, tmp );

    float cam_width = (pt2-pt1).length();
    float cam_height = (pt3-pt1).length();

    // add a small border
    cam_height = 1.08f * std::max<float>((cam_width*(float)size[1])/(float)size[0],cam_height);

    float aspect = cam->aspectRatio.getValue();

    if (cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        // set the new camera position dependent on the occupied space of projected bounding box
        //SbVec3f direction = cam->position.getValue() - box.getCenter();
        //float movelength = direction.length();
        //direction.normalize();
        //float fRatio = getViewportRegion().getViewportAspectRatio();
        //if ( fRatio > 1.0f ) {
        //  float factor = std::max<float>(s[0]/fRatio,s[1]);
        //  movelength = factor * movelength;
        //}
        //else {
        //    float factor = std::max<float>(s[0],s[1]/fRatio);
        //    movelength = factor * movelength;
        //}
        //cam->position.setValue(box.getCenter() + direction * movelength);
    }
    else if (cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        SoOrthographicCamera* ocam = (SoOrthographicCamera *)cam;  // safe downward cast, knows the type
        if (aspect < 1.0f)
            ocam->height = cam_height / aspect;
        else
            ocam->height = cam_height;
    }
}

/** Rotate the camera by the given amount, then reposition it so we're
 * still pointing at the same focal point.
 */
void NavigationStyle::reorientCamera(SoCamera * cam, const SbRotation & rot)
{
    if (cam == NULL) return;

    // Find global coordinates of focal point.
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    SbVec3f focalpoint = cam->position.getValue() +
                         cam->focalDistance.getValue() * direction;

    // Set new orientation value by accumulating the new rotation.
    cam->orientation = rot * cam->orientation.getValue();

    // Reposition camera so we are still pointing at the same old focal point.
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    cam->position = focalpoint - cam->focalDistance.getValue() * direction;
}

void NavigationStyle::panCamera(SoCamera * cam, float aspectratio, const SbPlane & panplane,
                                const SbVec2f & currpos, const SbVec2f & prevpos)
{
    if (cam == NULL) return; // can happen for empty scenegraph
    if (currpos == prevpos) return; // useless invocation


    // Find projection points for the last and current mouse coordinates.
    SbViewVolume vv = cam->getViewVolume(aspectratio);

    // See note in Coin docs for SoCamera::getViewVolume re:viewport mapping
    if(aspectratio < 1.0)
        vv.scale(1.0 / aspectratio);

    SbLine line;
    vv.projectPointToLine(currpos, line);
    SbVec3f current_planept;
    panplane.intersect(line, current_planept);
    vv.projectPointToLine(prevpos, line);
    SbVec3f old_planept;
    panplane.intersect(line, old_planept);

    // Reposition camera according to the vector difference between the
    // projected points.
    cam->position = cam->position.getValue() - (current_planept - old_planept);
}

void NavigationStyle::pan(SoCamera* camera)
{
    // The plane we're projecting the mouse coordinates to get 3D
    // coordinates should stay the same during the whole pan
    // operation, so we should calculate this value here.
    if (camera == NULL) { // can happen for empty scenegraph
        this->panningplane = SbPlane(SbVec3f(0, 0, 1), 0);
    }
    else {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float aspectratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(aspectratio);

        // See note in Coin docs for SoCamera::getViewVolume re:viewport mapping
        if(aspectratio < 1.0)
            vv.scale(1.0 / aspectratio);

        this->panningplane = vv.getPlane(camera->focalDistance.getValue());
    }
}

void NavigationStyle::panToCenter(const SbPlane & pplane, const SbVec2f & currpos)
{
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    float ratio = vp.getViewportAspectRatio();
    panCamera(viewer->getSoRenderManager()->getCamera(), ratio, pplane, SbVec2f(0.5,0.5), currpos);
    PRIVATE(this)->dragPointFound = false;
}

/** Dependent on the camera type this will either shrink or expand the
 * height of the viewport (orthogonal camera) or move the camera
 * closer or further away from the focal point in the scene.
 */
void NavigationStyle::zoom(SoCamera * cam, float diffvalue)
{
    if (cam == NULL) return; // can happen for empty scenegraph
    SoType t = cam->getTypeId();
    SbName tname = t.getName();

    // This will be in the range of <0, ->>.
    float multiplicator = float(exp(diffvalue));

    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {

        // Since there's no perspective, "zooming" in the original sense
        // of the word won't have any visible effect. So we just increase
        // or decrease the field-of-view values of the camera instead, to
        // "shrink" the projection size of the model / scene.
        SoOrthographicCamera * oc = (SoOrthographicCamera *)cam;
        oc->height = oc->height.getValue() * multiplicator;

    }
    else {
        // FrustumCamera can be found in the SmallChange CVS module (it's
        // a camera that lets you specify (for instance) an off-center
        // frustum (similar to glFrustum())
        if (!t.isDerivedFrom(SoPerspectiveCamera::getClassTypeId()) &&
            tname != "FrustumCamera") {
 /*         static SbBool first = true;
            if (first) {
                SoDebugError::postWarning("SoGuiFullViewerP::zoom",
                                          "Unknown camera type, "
                                          "will zoom by moving position, but this might not be correct.");
                first = false;
            }*/
        }

        const float oldfocaldist = cam->focalDistance.getValue();
        const float newfocaldist = oldfocaldist * multiplicator;

        SbVec3f direction;
        cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);

        const SbVec3f oldpos = cam->position.getValue();
        const SbVec3f newpos = oldpos + (newfocaldist - oldfocaldist) * -direction;

        // This catches a rather common user interface "buglet": if the
        // user zooms the camera out to a distance from origo larger than
        // what we still can safely do floating point calculations on
        // (i.e. without getting NaN or Inf values), the faulty floating
        // point values will propagate until we start to get debug error
        // messages and eventually an assert failure from core Coin code.
        //
        // With the below bounds check, this problem is avoided.
        //
        // (But note that we depend on the input argument ''diffvalue'' to
        // be small enough that zooming happens gradually. Ideally, we
        // should also check distorigo with isinf() and isnan() (or
        // inversely; isinfite()), but those only became standardized with
        // C99.)
        const float distorigo = newpos.length();
        // sqrt(FLT_MAX) == ~ 1e+19, which should be both safe for further
        // calculations and ok for the end-user and app-programmer.
        if (distorigo > float(sqrt(FLT_MAX))) {
        }
        else {
            cam->position = newpos;
            cam->focalDistance = newfocaldist;
        }
    }
}

// Calculate a zoom/dolly factor from the difference of the current
// cursor position and the last.
void NavigationStyle::zoomByCursor(const SbVec2f & thispos, const SbVec2f & prevpos)
{
    // There is no "geometrically correct" value, 20 just seems to give
    // about the right "feel".
    float value = (thispos[1] - prevpos[1]) * 10.0f/*20.0f*/;
    if (this->invertZoom)
        value = -value;
    zoom(viewer->getSoRenderManager()->getCamera(), value);
}

void NavigationStyle::zoomIn()
{
    zoom(viewer->getSoRenderManager()->getCamera(), -this->zoomStep);
}

void NavigationStyle::zoomOut()
{
    zoom(viewer->getSoRenderManager()->getCamera(), this->zoomStep);
}

void NavigationStyle::doZoom(SoCamera* camera, SbBool forward, const SbVec2f& pos)
{
//    SbBool zoomAtCur = this->zoomAtCursor;
//    if (zoomAtCur) {
//        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
//        float ratio = vp.getViewportAspectRatio();
//        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
//        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
//        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, SbVec2f(0.5,0.5), pos);
//    }

    float value = this->zoomStep;
    if (!forward)
        value = -value;
    if (this->invertZoom)
        value = -value;
    doZoom(camera, value, pos);

//    if (zoomAtCur) {
//        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
//        float ratio = vp.getViewportAspectRatio();
//        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
//        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
//        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, pos, SbVec2f(0.5,0.5));
//    }
}


/*!
 *\brief NavigationStyle::doZoom Zooms in or out by specified factor, keeping the point on screen specified by parameter pos fixed
 *  or not according to user preference (NavigationStyle::zoomAtCursor). Ignores invertZoom user preference.
 */
void NavigationStyle::doZoom(SoCamera* camera, float logfactor, const SbVec2f& pos)
{
    // something is asking for big zoom factor. This func is made for interactive zooming,
    // where the changes are per mouse move and thus are small.
    if (fabs(logfactor)>4.0)
        return;
    SbBool zoomAtCur = this->zoomAtCursor;
    if (zoomAtCur) {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, SbVec2f(0.5,0.5), pos);
    }

    zoom(camera, logfactor);

    if (zoomAtCur) {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, pos, SbVec2f(0.5,0.5));
    }
}

void NavigationStyle::doRotate(SoCamera * camera, float angle, const SbVec2f& pos)
{
    SbBool zoomAtCur = this->zoomAtCursor;
    if (zoomAtCur) {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, SbVec2f(0.5,0.5), pos);
    }

    SbRotation rotcam = camera->orientation.getValue();
    //get view direction
    SbVec3f vdir;
    rotcam.multVec(SbVec3f(0,0,-1),vdir);
    //rotate
    SbRotation drot(vdir,angle);
    camera->orientation.setValue(rotcam * drot);

    if (zoomAtCur) {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(vp.getViewportAspectRatio());
        SbPlane panplane = vv.getPlane(camera->focalDistance.getValue());
        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, pos, SbVec2f(0.5,0.5));
    }

}

/** Uses the sphere sheet projector to map the mouseposition onto
 * a 3D point and find a rotation from this and the last calculated point.
 */
void NavigationStyle::spin(const SbVec2f & pointerpos)
{
    if (this->log.historysize < 2) return;
    assert(this->spinprojector != NULL);

    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s glsize(vp.getViewportSizePixels());
    SbVec2f lastpos;
    lastpos[0] = float(this->log.position[1][0]) / float(std::max((int)(glsize[0]-1), 1));
    lastpos[1] = float(this->log.position[1][1]) / float(std::max((int)(glsize[1]-1), 1));

    if (PRIVATE(this)->dragAtCursor && PRIVATE(this)->dragPointFound) {
        SbVec3f hitpoint = PRIVATE(this)->startDragPoint;

        // set to the given position
        SbVec3f direction;
        viewer->getSoRenderManager()->getCamera()->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        viewer->getSoRenderManager()->getCamera()->position = hitpoint - viewer->getSoRenderManager()->getCamera()->focalDistance.getValue() * direction;
    }

    // 0000333: Turntable camera rotation
    SbMatrix mat;
    viewer->getSoRenderManager()->getCamera()->orientation.getValue().getValue(mat);
    this->spinprojector->setWorkingSpace(mat);

    this->spinprojector->project(lastpos);
    SbRotation r;
    this->spinprojector->projectAndGetRotation(pointerpos, r);
    float sensitivity = getSensitivity();
    if (sensitivity > 1.0f) {
        SbVec3f axis;
        float radians;
        r.getValue(axis, radians);
        radians = sensitivity * radians;
        r.setValue(axis, radians);
    }
    r.invert();
    this->reorientCamera(viewer->getSoRenderManager()->getCamera(), r);

    if (PRIVATE(this)->dragAtCursor && PRIVATE(this)->dragPointFound) {
        float ratio = vp.getViewportAspectRatio();
        SbViewVolume vv = viewer->getSoRenderManager()->getCamera()->getViewVolume(vp.getViewportAspectRatio());
        SbPlane panplane = vv.getPlane(viewer->getSoRenderManager()->getCamera()->focalDistance.getValue());
        SbVec2f posn;
        posn[0] = float(this->localPos[0]) / float(std::max((int)(glsize[0]-1), 1));
        posn[1] = float(this->localPos[1]) / float(std::max((int)(glsize[1]-1), 1));
        panCamera(viewer->getSoRenderManager()->getCamera(), ratio, panplane, posn, SbVec2f(0.5,0.5));
    }

    // Calculate an average angle magnitude value to make the transition
    // to a possible spin animation mode appear smooth.

    SbVec3f dummy_axis, newaxis;
    float acc_angle, newangle;
    this->spinincrement.getValue(dummy_axis, acc_angle);
    acc_angle *= this->spinsamplecounter; // weight
    r.getValue(newaxis, newangle);
    acc_angle += newangle;

    this->spinsamplecounter++;
    acc_angle /= this->spinsamplecounter;
    // FIXME: accumulate and average axis vectors as well? 19990501 mortene.
    this->spinincrement.setValue(newaxis, acc_angle);

    // Don't carry too much baggage, as that'll give unwanted results
    // when the user quickly trigger (as in "click-drag-release") a spin
    // animation.
    if (this->spinsamplecounter > 3) this->spinsamplecounter = 3;
}

/*!
 * \brief NavigationStyle::spin_simplified is a simplified version of
 * NavigationStyle::spin(..), which uses less global variables. Doesn't support
 * starting an animated spinning.
 *
 * \param cam the camera to affect. The rotation amount is determined by delta
 * (curpos-prevpos), and rotation axis is also affected by average pos.
 * \param curpos  current normalized position or mouse pointer
 * \param prevpos  previous normalized position of mouse pointer
 */
void NavigationStyle::spin_simplified(SoCamera* cam, SbVec2f curpos, SbVec2f prevpos){
    assert(this->spinprojector != NULL);

    // 0000333: Turntable camera rotation
    SbMatrix mat;
    viewer->getSoRenderManager()->getCamera()->orientation.getValue().getValue(mat);
    this->spinprojector->setWorkingSpace(mat);

    this->spinprojector->project(prevpos);
    SbRotation r;
    this->spinprojector->projectAndGetRotation(curpos, r);
    float sensitivity = getSensitivity();
    if (sensitivity > 1.0f) {
        SbVec3f axis;
        float radians;
        r.getValue(axis, radians);
        radians = sensitivity * radians;
        r.setValue(axis, radians);
    }
    r.invert();
    this->reorientCamera(cam, r);

}

SbBool NavigationStyle::doSpin()
{
    if (this->log.historysize >= 3) {
        SbTime stoptime = (SbTime::getTimeOfDay() - this->log.time[0]);
        if (this->spinanimatingallowed && stoptime.getValue() < 0.100) {
            const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
            const SbVec2s glsize(vp.getViewportSizePixels());
            SbVec3f from = this->spinprojector->project(SbVec2f(float(this->log.position[2][0]) / float(std::max(glsize[0]-1, 1)),
                                                                float(this->log.position[2][1]) / float(std::max(glsize[1]-1, 1))));
            SbVec3f to = this->spinprojector->project(this->lastmouseposition);
            SbRotation rot = this->spinprojector->getRotation(from, to);

            SbTime delta = (this->log.time[0] - this->log.time[2]);
            double deltatime = delta.getValue();
            rot.invert();
            rot.scaleAngle(float(0.200 / deltatime));

            SbVec3f axis;
            float radians;
            rot.getValue(axis, radians);
            if ((radians > 0.01f) && (deltatime < 0.300)) {
                this->spinRotation = rot;
                return true;
            }
        }
    }

    return false;
}

void NavigationStyle::saveCursorPosition(const SoEvent * const ev)
{
    this->globalPos.setValue(QCursor::pos().x(), QCursor::pos().y());
    this->localPos = ev->getPosition();

    // get the 3d point to the screen position, if possible
    if (PRIVATE(this)->dragAtCursor) {
        SoRayPickAction rpaction(viewer->getSoRenderManager()->getViewportRegion());
        rpaction.setPoint(this->localPos);
        rpaction.setRadius(viewer->getPickRadius());
        rpaction.apply(viewer->getSoRenderManager()->getSceneGraph());

        SoPickedPoint * picked = rpaction.getPickedPoint();
        if (picked) {
            PRIVATE(this)->dragPointFound = true;
            PRIVATE(this)->startDragPoint = picked->getPoint();
        }
    }
}

SbVec2f NavigationStyle::normalizePixelPos(SbVec2s pixpos)
{
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    return SbVec2f ((float) pixpos[0] / (float) std::max((int)(size[0] - 1), 1),
                    (float) pixpos[1] / (float) std::max((int)(size[1] - 1), 1));
}

SbVec2f NavigationStyle::normalizePixelPos(SbVec2f pixpos)
{
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    return SbVec2f ( pixpos[0] / (float) std::max((int)(size[0] - 1), 1),
                     pixpos[1] / (float) std::max((int)(size[1] - 1), 1));
}

void NavigationStyle::moveCursorPosition()
{
    if (!isResetCursorPosition())
        return;

    QPoint cpos = QCursor::pos();
    if (abs(cpos.x()-globalPos[0]) > 10 ||
        abs(cpos.y()-globalPos[1]) > 10) {
        QCursor::setPos(globalPos[0], globalPos[1]-1);
        this->log.position[0] = localPos;
    }
}

void NavigationStyle::updateAnimation()
{
    SbTime now = SbTime::getTimeOfDay();
    double secs = now.getValue() -  prevRedrawTime.getValue();
    this->prevRedrawTime = now;

    if (this->isAnimating()) {
        // here the camera rotates around a fix axis
        SbRotation deltaRotation = this->spinRotation;
        deltaRotation.scaleAngle(secs * 5.0);
        this->reorientCamera(viewer->getSoRenderManager()->getCamera(), deltaRotation);
    }
}

void NavigationStyle::redraw()
{
    if (mouseSelection)
        mouseSelection->redraw();
}

SbBool NavigationStyle::handleEventInForeground(const SoEvent* const e)
{
    SoHandleEventAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.setEvent(e);
    action.setPickRadius(viewer->getPickRadius());
    action.apply(viewer->foregroundroot);
    return action.isHandled();
}

/*!
  Decide if it should be possible to start a spin animation of the
  model in the viewer by releasing the mouse button while dragging.

  If the \a enable flag is \c false and we're currently animating, the
  spin will be stopped.
*/
void
NavigationStyle::setAnimationEnabled(const SbBool enable)
{
    this->spinanimatingallowed = enable;
    if (!enable && this->isAnimating()) { this->stopAnimating(); }
}

/*!
  Query whether or not it is possible to start a spinning animation by
  releasing the left mouse button while dragging the mouse.
*/

SbBool
NavigationStyle::isAnimationEnabled(void) const
{
    return this->spinanimatingallowed;
}

/*!
  Query if the model in the viewer is currently in spinning mode after
  a user drag.
*/
SbBool NavigationStyle::isAnimating(void) const
{
    return this->currentmode == NavigationStyle::SPINNING;
}

/*!
 * Starts programmatically the viewer in animation mode. The given axis direction
 * is always in screen coordinates, not in world coordinates.
 */
void NavigationStyle::startAnimating(const SbVec3f& axis, float velocity)
{
    if (!isAnimationEnabled()) return;

    this->prevRedrawTime = SbTime::getTimeOfDay();
    this->spinincrement = SbRotation::identity();
    SbRotation rot;
    rot.setValue(axis, velocity);

    this->setViewing(true);
    this->setViewingMode(NavigationStyle::SPINNING);
    this->spinRotation = rot;
}

void NavigationStyle::stopAnimating(void)
{
    if (this->currentmode != NavigationStyle::SPINNING) {
        return;
    }
    this->setViewingMode(this->isViewing() ? 
        NavigationStyle::IDLE : NavigationStyle::INTERACT);
}

void NavigationStyle::setSensitivity(float val)
{
    PRIVATE(this)->sensitivity = val;
}

float NavigationStyle::getSensitivity() const
{
    return PRIVATE(this)->sensitivity;
}

void NavigationStyle::setResetCursorPosition(SbBool on)
{
    PRIVATE(this)->resetcursorpos = on;
}

SbBool NavigationStyle::isResetCursorPosition() const
{
    return PRIVATE(this)->resetcursorpos;
}

void NavigationStyle::setZoomInverted(SbBool on)
{
    this->invertZoom = on;
}

SbBool NavigationStyle::isZoomInverted() const
{
    return this->invertZoom;
}

void NavigationStyle::setZoomStep(float val)
{
    this->zoomStep = val;
}

void NavigationStyle::setZoomAtCursor(SbBool on)
{
    this->zoomAtCursor = on;
}

SbBool NavigationStyle::isZoomAtCursor() const
{
    return this->zoomAtCursor;
}

void NavigationStyle::startSelection(AbstractMouseSelection* mouse)
{
    if (!mouse)
        return;

    if (mouseSelection) {
        SoDebugError::postWarning("NavigationStyle::startSelection",
                                  "Set new mouse selection while an old is still active.");
    }

    mouseSelection = mouse;
    mouseSelection->grabMouseModel(viewer);
}

void NavigationStyle::startSelection(NavigationStyle::SelectionMode mode)
{
    if (mouseSelection)
        return;
    if (isSelecting())
        stopSelection();
  
    switch (mode)
    {
    case Lasso:
        mouseSelection = new PolyPickerSelection();
        break;
    case Rectangle:
        mouseSelection = new RectangleSelection();
        break;
    case Rubberband:
        mouseSelection = new RubberbandSelection();
        break;
    case BoxZoom:
        mouseSelection = new BoxZoomSelection();
        break;
    case Clip:
        mouseSelection = new PolyClipSelection();
        break;
    default:
        break;
    }

    if (mouseSelection)
        mouseSelection->grabMouseModel(viewer);
}

void NavigationStyle::stopSelection()
{
    pcPolygon.clear();
    if (mouseSelection) {
        mouseSelection->releaseMouseModel();
        delete mouseSelection;
        mouseSelection = 0;
    }
}

SbBool NavigationStyle::isSelecting() const
{
    return (mouseSelection ? true : false);
}

const std::vector<SbVec2s>& NavigationStyle::getPolygon(SbBool* clip_inner) const
{
    if (clip_inner)
        *clip_inner = this->clipInner;
    return pcPolygon;
}

// This method adds another point to the mouse location log, used for spin
// animation calculations.
void NavigationStyle::addToLog(const SbVec2s pos, const SbTime time)
{
    // In case someone changes the const size setting at the top of this
    // file too small.
    assert (this->log.size > 2 && "mouse log too small!");

    if (this->log.historysize > 0 && pos == this->log.position[0]) {
        return;
    }

    int lastidx = this->log.historysize;
    // If we've filled up the log, we should throw away the last item:
    if (lastidx == this->log.size) { lastidx--; }

    assert(lastidx < this->log.size);
    for (int i = lastidx; i > 0; i--) {
        this->log.position[i] = this->log.position[i-1];
        this->log.time[i] = this->log.time[i-1];
    }

    this->log.position[0] = pos;
    this->log.time[0] = time;
    if (this->log.historysize < this->log.size)
        this->log.historysize += 1;
}

// This method "clears" the mouse location log, used for spin
// animation calculations.
void NavigationStyle::clearLog(void)
{
    this->log.historysize = 0;
}

// The viewer is a state machine, and all changes to the current state
// are made through this call.
void NavigationStyle::setViewingMode(const ViewerMode newmode)
{
    const ViewerMode oldmode = this->currentmode;
    if (newmode == oldmode) { return; }

    switch (newmode) {
    case DRAGGING:
        // Set up initial projection point for the projector object when
        // first starting a drag operation.
        this->spinprojector->project(this->lastmouseposition);
        this->interactiveCountInc();
        this->clearLog();
        break;

    case SPINNING:
        this->interactiveCountInc();
        viewer->getSoRenderManager()->scheduleRedraw();
        break;

    case PANNING:
        pan(viewer->getSoRenderManager()->getCamera());
        this->interactiveCountInc();
        break;

    case ZOOMING:
        this->interactiveCountInc();
        break;

    case BOXZOOM:
        this->interactiveCountInc();
        break;

    default: // include default to avoid compiler warnings.
        break;
    }

    switch (oldmode) {
    case SPINNING:
    case DRAGGING:
    case PANNING:
    case ZOOMING:
    case BOXZOOM:
        this->interactiveCountDec();
        break;

    default:
        break;
    }

    viewer->setCursorRepresentation(newmode);
    this->currentmode = newmode;
}

int NavigationStyle::getViewingMode() const
{
    return (int)this->currentmode;
}

SbBool NavigationStyle::processEvent(const SoEvent * const ev)
{
    // If we're in picking mode then all events must be redirected to the
    // appropriate mouse model.
    if (mouseSelection) {
        int hd=mouseSelection->handleEvent(ev,viewer->getSoRenderManager()->getViewportRegion());
        if (hd==AbstractMouseSelection::Continue||
            hd==AbstractMouseSelection::Restart) {
            return true;
        }
        else if (hd==AbstractMouseSelection::Finish) {
            pcPolygon = mouseSelection->getPositions();
            clipInner = mouseSelection->isInner();
            delete mouseSelection; 
            mouseSelection = 0;
            syncWithEvent(ev);
            return NavigationStyle::processSoEvent(ev);
        }
        else if (hd==AbstractMouseSelection::Cancel) {
            pcPolygon.clear();
            delete mouseSelection; 
            mouseSelection = 0;
            syncWithEvent(ev);
            return NavigationStyle::processSoEvent(ev);
        }
    }

    const ViewerMode curmode = this->currentmode;

    SbBool processed = false;
    processed = this->processSoEvent(ev);

    // check for left click without selecting something
    if ((curmode == NavigationStyle::SELECTION || curmode == NavigationStyle::IDLE)
            && !processed) {
        if (ev->getTypeId().isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
            SoMouseButtonEvent * const e = (SoMouseButtonEvent *) ev;
            if (SoMouseButtonEvent::isButtonReleaseEvent(e,SoMouseButtonEvent::BUTTON1)) {
                Gui::Selection().clearSelection();
            }
        }
    }

    return processed;
}

SbBool NavigationStyle::processSoEvent(const SoEvent * const ev)
{
    return viewer->processSoEventBase(ev);
}

void NavigationStyle::syncWithEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) { return; }

    const SoType type(ev->getTypeId());

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    if (this->ctrldown != ev->wasCtrlDown()) {
        this->ctrldown = ev->wasCtrlDown();
    }
    if (this->shiftdown != ev->wasShiftDown()) {
        this->shiftdown = ev->wasShiftDown();
    }
    if (this->altdown != ev->wasAltDown()) {
        this->altdown = ev->wasAltDown();
    }

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        const SoKeyboardEvent * const event = (const SoKeyboardEvent *) ev;
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
        switch (event->getKey()) {
        case SoKeyboardEvent::LEFT_CONTROL:
        case SoKeyboardEvent::RIGHT_CONTROL:
            this->ctrldown = press;
            break;
        case SoKeyboardEvent::LEFT_SHIFT:
        case SoKeyboardEvent::RIGHT_SHIFT:
            this->shiftdown = press;
            break;
        case SoKeyboardEvent::LEFT_ALT:
        case SoKeyboardEvent::RIGHT_ALT:
            this->altdown = press;
            break;
        default:
            break;
        }
    }

    // Mouse Button / Spaceball Button handling
    if (type.isDerivedFrom(SoMouseButtonEvent::getClassTypeId())) {
        const SoMouseButtonEvent * const event = (const SoMouseButtonEvent *) ev;
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;

        // SoDebugError::postInfo("processSoEvent", "button = %d", button);
        switch (button) {
        case SoMouseButtonEvent::BUTTON1:
            this->button1down = press;
            break;
        case SoMouseButtonEvent::BUTTON2:
            this->button2down = press;
            break;
        case SoMouseButtonEvent::BUTTON3:
            this->button3down = press;
            break;
        default:
            break;
        }
    }
}

SbBool NavigationStyle::processMotionEvent(const SoMotion3Event * const ev)
{
    SoCamera * const camera = viewer->getSoRenderManager()->getCamera();
    if (!camera)
        return false;

    SbViewVolume volume(camera->getViewVolume());
    SbVec3f center(volume.getSightPoint(camera->focalDistance.getValue()));
    float scale(volume.getWorldToScreenScale(center, 1.0));
    float translationFactor = scale * .0001;

    SbVec3f dir = ev->getTranslation();

    if (camera->getTypeId().isDerivedFrom(SoOrthographicCamera::getClassTypeId())){
        SoOrthographicCamera *oCam = static_cast<SoOrthographicCamera *>(camera);
        oCam->scaleHeight(1.0 + (dir[2] * 0.0001));
        dir[2] = 0.0;//don't move the cam for z translation.
    }

    SbRotation newRotation(ev->getRotation() * camera->orientation.getValue());
    SbVec3f newPosition, newDirection;
    newRotation.multVec(SbVec3f(0.0, 0.0, -1.0), newDirection);
    newPosition = center - (newDirection * camera->focalDistance.getValue());

    camera->orientation.setValue(newRotation);
    camera->orientation.getValue().multVec(dir,dir);
    camera->position = newPosition + (dir * translationFactor);

    return true;
}

void NavigationStyle::setPopupMenuEnabled(const SbBool on)
{
    this->menuenabled = on;
}

SbBool NavigationStyle::isPopupMenuEnabled(void) const
{
    return this->menuenabled;
}

void NavigationStyle::openPopupMenu(const SbVec2s& position)
{
    Q_UNUSED(position); 
    // ask workbenches and view provider, ...
    MenuItem* view = new MenuItem;
    Gui::Application::Instance->setupContextMenu("View", view);

    QMenu contextMenu(viewer->getGLWidget());
    QMenu subMenu;
    QActionGroup subMenuGroup(&subMenu);
    subMenuGroup.setExclusive(true);
    subMenu.setTitle(QObject::tr("Navigation styles"));

    MenuManager::getInstance()->setupContextMenu(view, contextMenu);
    contextMenu.addMenu(&subMenu);

    // add submenu at the end to select navigation style
    std::map<Base::Type, std::string> styles = UserNavigationStyle::getUserFriendlyNames();
    for (std::map<Base::Type, std::string>::iterator it = styles.begin(); it != styles.end(); ++it) {
        QByteArray data(it->first.getName());
        QString name = QApplication::translate(it->first.getName(), it->second.c_str());

        QAction* item = subMenuGroup.addAction(name);
        item->setData(data);
        item->setCheckable(true);
        if (it->first == this->getTypeId())
            item->setChecked(true);
        subMenu.addAction(item);
    }

    delete view;
    QAction* used = contextMenu.exec(QCursor::pos());
    if (used && subMenuGroup.actions().indexOf(used) >= 0 && used->isChecked()) {
        QByteArray type = used->data().toByteArray();
        QWidget* widget = viewer->getWidget();
        while (widget && !widget->inherits("Gui::View3DInventor"))
            widget = widget->parentWidget();
        if (widget) {
            // this is the widget where the viewer is embedded
            Base::Type style = Base::Type::fromName((const char*)type);
            if (style != this->getTypeId()) {
                QEvent* event = new NavigationStyleEvent(style);
                QApplication::postEvent(widget, event);
            }
        }
    }
}

// ----------------------------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::UserNavigationStyle,Gui::NavigationStyle);

std::string UserNavigationStyle::userFriendlyName() const
{
    std::string name = this->getTypeId().getName();
    // remove namespaces
    std::size_t pos = name.rfind("::");
    if (pos != std::string::npos)
        name = name.substr(pos + 2);

    // remove 'NavigationStyle'
    pos = name.find("NavigationStyle");
    if (pos != std::string::npos)
        name = name.substr(0, pos);
    return name;
}

std::map<Base::Type, std::string> UserNavigationStyle::getUserFriendlyNames()
{
    std::map<Base::Type, std::string> names;
    std::vector<Base::Type> types;
    Base::Type::getAllDerivedFrom(UserNavigationStyle::getClassTypeId(), types);

    for (std::vector<Base::Type>::iterator it = types.begin(); it != types.end(); ++it) {
        if (*it != UserNavigationStyle::getClassTypeId()) {
            std::unique_ptr<UserNavigationStyle> inst(static_cast<UserNavigationStyle*>(it->createInstance()));
            if (inst.get()) {
                names[*it] = inst->userFriendlyName();
            }
        }
    }
    return names;
}
