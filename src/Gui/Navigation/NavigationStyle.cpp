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
# include <Inventor/SbViewportRegion.h>
# include <Inventor/SoPickedPoint.h>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/errors/SoDebugError.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoCamera.h>
# include <Inventor/nodes/SoOrthographicCamera.h>
# include <Inventor/nodes/SoPerspectiveCamera.h>
# include <Inventor/projectors/SbSphereSheetProjector.h>
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <QByteArray>
# include <QCursor>
# include <QMenu>
#endif

#include <cmath>
#include <limits>

#include <Base/Interpreter.h>
#include <App/Application.h>

#include "Navigation/NavigationStyle.h"
#include "Navigation/NavigationStylePy.h"
#include "Application.h"
#include "Inventor/SoMouseWheelEvent.h"
#include "MenuManager.h"
#include "MouseSelection.h"
#include "Navigation/NavigationAnimator.h"
#include "Navigation/NavigationAnimation.h"
#include "View3DInventorViewer.h"

using namespace Gui;

class FCSphereSheetProjector : public SbSphereSheetProjector {
    using inherited = SbSphereSheetProjector;

public:
    enum OrbitStyle {
        Turntable,
        Trackball,
        FreeTurntable,
        TrackballClassic,
        RoundedArcball
    };

    static constexpr float defaultSphereRadius = 0.8F;

    FCSphereSheetProjector(const SbSphere & sph, const SbBool orienttoeye = true)
        : SbSphereSheetProjector(sph, orienttoeye)
    {
    }

    void setViewVolume (const SbViewVolume &vol) override
    {
        inherited::setViewVolume(vol);
    }

    void setWorkingSpace (const SbMatrix &space) override
    {
        //inherited::setWorkingSpace(space);
        this->worldToScreen = space.inverse();
    }

    SbVec3f project(const SbVec2f &point) override
    {
        if (orbit != RoundedArcball) {
            return inherited::project(point);
        }

        // Rounded Arcball implementation
        // based on SbSphereSheetProjector in Open Inventor
        SbVec3f result;
        SbLine workingLine = getWorkingLine(point);

        if (needSetup) {
            setupPlane();
        }

        SbVec3f planeIntersection;

        SbVec3f sphereIntersection, dontCare;
        SbBool hitSphere;
        if (intersectFront == TRUE) {
            hitSphere = sphere.intersect(workingLine, sphereIntersection, dontCare);
        }
        else {
            hitSphere = sphere.intersect(workingLine, dontCare, sphereIntersection);
        }

        if (hitSphere) {
            // drop the sphere intersection onto the tolerance plane

            SbLine projectLine(sphereIntersection, sphereIntersection + planeDir);
            if (!tolPlane.intersect(projectLine, planeIntersection)) {
#ifdef FC_DEBUG
                SoDebugError::post("SbSphereSheetProjector::project",
                                   "Could not intersect working line with plane");
#endif
            }
        }
        else if (!tolPlane.intersect(workingLine, planeIntersection)) {
#ifdef FC_DEBUG
            SoDebugError::post("SbSphereSheetProjector::project", "Could not intersect with plane");
#endif
        }

        // Three possibilities:
        // (1) Intersection is on the sphere inside where the fillet
        //	    hits it
        // (2) Intersection is on the fillet
        // (3) Intersection is on the plane
        float distance = (planeIntersection - planePoint).length();

        // Amount of filleting
        // 0 = no fillet (just sphere and plane)
        // infinity = only fillet
        float border = 0.5;

        // Radius where the fillet meets the plane in "squished space"
        float r_a = 1.0F + border;
        // Radius where the sphere meets the fillet in "squished space"
        float r_i = 2.0 / (r_a + 1.0 / r_a);

        // Distance squared in "squished space"
        float d_2 = (distance * distance) * r_a * r_a;
        // Distance in "squished space"
        float d = std::sqrt(d_2);

        // Compute how far off the plane we are
        float offsetDist = 0.0;

        if (d > r_a) {
            // On the plane
            offsetDist = 0.0;
        }
        else if (d < r_i) {
            // On the sphere inside the fillet
            offsetDist = std::sqrt(1.0 - d_2);
        }
        else {
            // On the fillet
            float d_r = r_a - d;
            float a = border * (1.0 + border / 2.0);
            offsetDist = a - std::sqrt((a + d_r) * (a - d_r));
        }

        SbVec3f offset;
        if (orientToEye) {
            if (viewVol.getProjectionType() == SbViewVolume::PERSPECTIVE) {
                offset = workingProjPoint - planeIntersection;
            }
            else {
                worldToWorking.multDirMatrix(viewVol.zVector(), offset);
            }

            offset.normalize();
        }
        else {
            offset.setValue(0, 0, 1);
        }
        if (intersectFront == FALSE) {
            offset *= -1.0;
        }

        offset *= offsetDist;
        result = planeIntersection + offset;

        lastPoint = result;
        return result;
    }

    SbRotation getRotation(const SbVec3f &point1, const SbVec3f &point2) override
    {
        SbRotation rot = inherited::getRotation(point1, point2);
        if (orbit == Turntable) {
            return getTurntable(rot, point1, point2);
        }
        if (orbit == FreeTurntable) {
            return getFreeTurntable(point1, point2);
        }
        if (orbit == TrackballClassic) {
            return getTrackballClassic(point1, point2);
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
    SbRotation getTurntable(SbRotation rot, const SbVec3f &point1, const SbVec3f &point2) const
    {
        // 0000333: Turntable camera rotation
        SbVec3f axis;
        float angle{};
        rot.getValue(axis, angle);
        SbVec3f dif = point1 - point2;
        if (fabs(dif[1]) > fabs(dif[0])) {
            SbVec3f xaxis(1,0,0);
            if (dif[1] < 0) {
                angle = -angle;
            }
            rot.setValue(xaxis, angle);
        }
        else {
            SbVec3f zaxis(0,0,1);
            this->worldToScreen.multDirMatrix(zaxis, zaxis);
            if (zaxis[1] < 0) {
                if (dif[0] < 0) {
                    angle = -angle;
                }
            }
            else {
                if (dif[0] > 0) {
                    angle = -angle;
                }
            }
            rot.setValue(zaxis, angle);
        }

        return rot;
    }

    SbRotation getFreeTurntable(const SbVec3f &point1, const SbVec3f &point2) const
    {
        // Turntable without constraints
        SbRotation zrot;
        SbRotation xrot;
        SbVec3f dif = point1 - point2;

        SbVec3f zaxis(1,0,0);
        zrot.setValue(zaxis, dif[1]);

        SbVec3f xaxis(0,0,1);
        this->worldToScreen.multDirMatrix(xaxis, xaxis);
        xrot.setValue(xaxis, -dif[0]);

        return zrot * xrot;
    }

    SbRotation getTrackballClassic(const SbVec3f &point1, const SbVec3f &point2) const
    {
        // Classic trackball
        SbRotation zrot;
        SbRotation yrot;
        SbVec3f dif = point1 - point2;

        SbVec3f zaxis(1,0,0);
        zrot.setValue(zaxis, dif[1]);

        SbVec3f yaxis(0,1,0);
        yrot.setValue(yaxis, -dif[0]);

        return zrot * yrot;
    }

private:
    SbMatrix worldToScreen;
    OrbitStyle orbit{RoundedArcball};
};

NavigationStyleEvent::NavigationStyleEvent(const Base::Type& s)
  : QEvent(QEvent::User), t(s)
{
}

NavigationStyleEvent::~NavigationStyleEvent() = default;

const Base::Type& NavigationStyleEvent::style() const
{
    return t;
}

TYPESYSTEM_SOURCE_ABSTRACT(Gui::NavigationStyle,Base::BaseClass)

NavigationStyle::NavigationStyle() : viewer(nullptr), mouseSelection(nullptr), pythonObject(nullptr)
{
    this->rotationCenterMode = NavigationStyle::RotationCenterMode::ScenePointAtCursor
        | NavigationStyle::RotationCenterMode::FocalPointAtCursor;
    initialize();
}

NavigationStyle::~NavigationStyle()
{
    finalize();
    delete this->animator;

    if (!pythonObject.is(nullptr)) {
        Base::PyGILStateLocker lock;
        Base::PyObjectBase* obj = static_cast<Base::PyObjectBase*>(pythonObject.ptr());
        obj->setInvalid();
    }
}

NavigationStyle& NavigationStyle::operator = (const NavigationStyle& ns)
{
    this->panningplane = ns.panningplane;
    this->menuenabled = ns.menuenabled;
    this->animationEnabled = ns.animationEnabled;
    this->spinningAnimationEnabled = ns.spinningAnimationEnabled;
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
    this->animator = new NavigationAnimator();

    this->sensitivity = 2.0f;
    this->resetcursorpos = false;
    this->currentmode = NavigationStyle::IDLE;
    this->animationEnabled = true;
    this->spinningAnimationEnabled = false;
    this->spinsamplecounter = 0;
    this->spinincrement = SbRotation::identity();
    this->rotationCenterFound = false;
    this->rotationCenterIsScenePointAtCursor = false;

    // FIXME: use a smaller sphere than the default one to have a larger
    // area close to the borders that gives us "z-axis rotation"?
    // 19990425 mortene.
    this->spinprojector = new FCSphereSheetProjector(SbSphere(SbVec3f(0, 0, 0), FCSphereSheetProjector::defaultSphereRadius));
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
    long mode = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View")->GetInt("RotationMode", 0);
    if (mode == 0) {
        setRotationCenterMode(NavigationStyle::RotationCenterMode::WindowCenter);
    }
    else if (mode == 1) {
        setRotationCenterMode(NavigationStyle::RotationCenterMode::ScenePointAtCursor |
                              NavigationStyle::RotationCenterMode::FocalPointAtCursor);
    }
    else if (mode == 2) {
        setRotationCenterMode(NavigationStyle::RotationCenterMode::ScenePointAtCursor |
                              NavigationStyle::RotationCenterMode::BoundingBoxCenter);
    }

    this->hasDragged = false;
    this->hasPanned = false;
    this->hasZoomed = false;
}

void NavigationStyle::finalize()
{
    delete this->spinprojector;
    delete[] this->log.position;
    delete[] this->log.time;
}

void NavigationStyle::interactiveCountInc()
{
    viewer->interactiveCountInc();
}

void NavigationStyle::interactiveCountDec()
{
    viewer->interactiveCountDec();
}

int NavigationStyle::getInteractiveCount() const
{
    return viewer->getInteractiveCount();
}

void NavigationStyle::setOrbitStyle(NavigationStyle::OrbitStyle style)
{
    auto projector = static_cast<FCSphereSheetProjector*>(this->spinprojector);
    projector->setOrbitStyle(FCSphereSheetProjector::OrbitStyle(style));
}

NavigationStyle::OrbitStyle NavigationStyle::getOrbitStyle() const
{
    auto projector = static_cast<FCSphereSheetProjector*>(this->spinprojector);
    return NavigationStyle::OrbitStyle(projector->getOrbitStyle());
}

SbBool NavigationStyle::isViewing() const
{
    return viewer->isViewing();
}

void NavigationStyle::setViewing(SbBool enable)
{
    viewer->setViewing(enable);
}

SbBool NavigationStyle::isSeekMode() const
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

void NavigationStyle::lookAtPoint(const SbVec2s screenpos)
{
    const SoCamera* camera = viewer->getCamera();
    if (!camera) {
        return;
    }

    SoRayPickAction rpaction(viewer->getViewportRegion());
    rpaction.setPoint(screenpos);
    rpaction.setRadius(viewer->getPickRadius());
    rpaction.apply(viewer->getSoRenderManager()->getSceneGraph());

    const SoPickedPoint* picked = rpaction.getPickedPoint();

    // Point is either the hitpoint or the projected point on the panning plane
    SbVec3f point;
    if (picked) {
        point = picked->getPoint();
    }
    else {
        const SbViewportRegion& vp = viewer->getViewportRegion();
        const float aspectratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(aspectratio);

        // See note in Coin docs for SoCamera::getViewVolume re:viewport mapping
        if (aspectratio < 1.0) {
            vv.scale(1.0 / aspectratio);
        }

        SbLine line;
        vv.projectPointToLine(normalizePixelPos(screenpos), line);
        panningplane.intersect(line, point);
    }

    lookAtPoint(point);
}

void NavigationStyle::lookAtPoint(const SbVec3f& position)
{
    this->rotationCenterFound = false;
    translateCamera(position - getFocalPoint());
}

SoCamera* NavigationStyle::getCamera() const
{
    return this->viewer->getCamera();
}

std::shared_ptr<NavigationAnimation> NavigationStyle::setCameraOrientation(const SbRotation& orientation, const SbBool moveToCenter) const
{
    SoCamera* camera = getCamera();
    if (!camera)
        return {};

    animator->stop();

    const SbVec3f focalPoint = getFocalPoint();
    SbVec3f translation(0, 0, 0);

    if (moveToCenter) {
        SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
        action.apply(viewer->getSceneGraph());
        SbBox3f box = action.getBoundingBox();
        if (!box.isEmpty()) {
            translation = box.getCenter() - focalPoint;
        }
    }

    // Start an animation and return it
    if (isAnimationEnabled()) {
        return viewer->startAnimation(orientation, focalPoint, translation);
    }

    // or set the pose directly

    // Distance from rotation center to camera position in camera coordinate system
    const SbVec3f rotationCenterDistanceCam = camera->focalDistance.getValue() * SbVec3f(0, 0, 1);

    // Set to the given orientation
    camera->orientation = orientation;

    // Distance from rotation center to new camera position in global coordinate system
    SbVec3f newRotationCenterDistance;
    camera->orientation.getValue().multVec(rotationCenterDistanceCam, newRotationCenterDistance);

    // Reposition camera so the rotation center stays in the same place
    // Optionally add translation to move to center
    camera->position = focalPoint + newRotationCenterDistance + translation;

    return {};
}

std::shared_ptr<NavigationAnimation> NavigationStyle::translateCamera(const SbVec3f& translation) const
{
    SoCamera* camera = getCamera();
    if (!camera)
        return {};

    animator->stop();

    // Start an animation and return it
    if (isAnimationEnabled()) {
        return viewer->startAnimation(camera->orientation.getValue(), SbVec3f(0, 0, 0), translation);
    }

    // or set the pose directly

    camera->position = camera->position.getValue() + translation;

    return {};
}

void NavigationStyle::boxZoom(const SbBox2s& box)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) // no camera
        return;
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    SbViewVolume vv = cam->getViewVolume(vp.getViewportAspectRatio());

    short sizeX{},sizeY{};
    box.getSize(sizeX, sizeY);
    SbVec2s size = vp.getViewportSizePixels();

    // The bbox must not be empty i.e. width and length is zero, but it is possible that
    // either width or length is zero
    if (sizeX == 0 && sizeY == 0)
        return;

    // Get the new center in normalized pixel coordinates
    short xmin{},xmax{},ymin{},ymax{};
    box.getBounds(xmin,ymin,xmax,ymax);
    const SbVec2f center((float) ((xmin+xmax)/2) / (float) std::max((int)(size[0] - 1), 1),
                         (float) (size[1]-(ymin+ymax)/2) / (float) std::max((int)(size[1] - 1), 1));

    SbPlane plane = vv.getPlane(cam->focalDistance.getValue());
    panCamera(cam,vp.getViewportAspectRatio(),plane, SbVec2f(0.5,0.5), center);

    // Set height or height angle of the camera
    float scaleX = (float)sizeX/(float)size[0];
    float scaleY = (float)sizeY/(float)size[1];
    float scaleFactor = std::max<float>(scaleX, scaleY);

    doScale(cam, scaleFactor);
}
void NavigationStyle::scale(float factor)
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam) { // no camera
        return;
    }

    // Find the current center of the screen
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    SbVec3f initCenter = cam->position.getValue() + cam->focalDistance.getValue() * direction;

    // Move the camera to the origin for scaling
    cam->position = cam->position.getValue() - initCenter;

    // Scale the view
    doScale(cam, factor);
    
    // Move the camera back to it's initial position scaled
    cam->position = cam->position.getValue() + initCenter * factor;
}

void NavigationStyle::viewAll()
{
    // Get the bounding box of the scene
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    SbBox3f box = action.getBoundingBox();
    if (box.isEmpty())
        return;


    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam)
        return;

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

    if (cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        auto ocam = static_cast<SoOrthographicCamera *>(cam);
        if (aspect < 1.0f)
            ocam->height = cam_height / aspect;
        else
            ocam->height = cam_height;
    }
}

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
void NavigationStyle::findBoundingSphere() {
    // Find a bounding sphere for the scene
    SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.apply(viewer->getSceneGraph());
    boundingSphere.circumscribe(action.getBoundingBox());
}
#endif

/** Rotate the camera by the given amount, then reposition it so we're still pointing at the same
 * focal point
 */
void NavigationStyle::reorientCamera(SoCamera* camera, const SbRotation& rotation)
{
    reorientCamera(camera, rotation, getFocalPoint());
}

/** Rotate the camera by the given amount, then reposition it so the rotation center stays in the
 * same place
 */
void NavigationStyle::reorientCamera(SoCamera* camera, const SbRotation& rotation, const SbVec3f& rotationCenter)
{
    if (!camera) {
        return;
    }

    // Distance from rotation center to camera position in camera coordinate system
    SbVec3f rotationCenterDistanceCam;
    camera->orientation.getValue().inverse().multVec(camera->position.getValue() - rotationCenter, rotationCenterDistanceCam);

    // Set new orientation value by accumulating the new rotation
    camera->orientation = rotation * camera->orientation.getValue();

    // Distance from rotation center to new camera position in global coordinate system
    SbVec3f newRotationCenterDistance;
    camera->orientation.getValue().multVec(rotationCenterDistanceCam, newRotationCenterDistance);

    // Reposition camera so the rotation center stays in the same place
    camera->position = rotationCenter + newRotationCenterDistance;

    if (camera->getTypeId().isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {
        // Adjust the camera position to keep the focal point in the same place while making sure
        // the focal distance stays small. This increases rotation stability for small and large
        // scenes

        SbVec3f direction;
        camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
        
#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 405)
        // Large focal distance puts the camera far away which causes Coin's auto clipping
        // calculations to add more slack (more space between near and far plane) and thus reduces
        // chances or hidden geometry.
        constexpr float orthographicFocalDistance = 250;
#else
        constexpr float orthographicFocalDistance = 1;
#endif
        camera->position = getFocalPoint() - orthographicFocalDistance * direction;
        camera->focalDistance = orthographicFocalDistance;
    }
    
#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
    // Fix issue with near clipping in orthogonal view
    if (camera->getTypeId().isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {

        // The center of the bounding sphere in camera coordinate system
        SbVec3f center;
         camera->orientation.getValue().inverse().multVec(boundingSphere.getCenter() - camera->position.getValue(), center);

        SbVec3f dir;
        camera->orientation.getValue().multVec(SbVec3f(0, 0, -1), dir);

        // Reposition the camera but keep the focal point the same
        // nearDistance is 0 and farDistance is the diameter of the bounding sphere
        float repositionDistance = -center.getValue()[2] - boundingSphere.getRadius();
        camera->position = camera->position.getValue() + repositionDistance * dir;
        camera->nearDistance = 0;
        camera->farDistance = 2 * boundingSphere.getRadius() + 1;
        camera->focalDistance = camera->focalDistance.getValue() - repositionDistance;
    }
#endif
}

void NavigationStyle::panCamera(SoCamera * cam, float aspectratio, const SbPlane & panplane,
                                const SbVec2f & currpos, const SbVec2f & prevpos)
{
    if (!cam) // can happen for empty scenegraph
        return;
    if (currpos == prevpos) // useless invocation
        return;


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

    if (this->currentmode != NavigationStyle::IDLE) {
        hasPanned = true;
    }
}

void NavigationStyle::setupPanningPlane(const SoCamera* camera)
{
    // The plane we're projecting the mouse coordinates to get 3D
    // coordinates should stay the same during the whole pan
    // operation, so we should calculate this value here.
    if (!camera) {  // can happen for empty scenegraph
        this->panningplane = SbPlane(SbVec3f(0, 0, 1), 0);
    }
    else {
        const SbViewportRegion& vp = viewer->getViewportRegion();
        const float aspectratio = vp.getViewportAspectRatio();
        SbViewVolume vv = camera->getViewVolume(aspectratio);

        // See note in Coin docs for SoCamera::getViewVolume re:viewport mapping
        if (aspectratio < 1.0) {
            vv.scale(1.0 / aspectratio);
        }

        this->panningplane = vv.getPlane(camera->focalDistance.getValue());
    }
}

/** Dependent on the camera type this will either shrink or expand the
 * height of the viewport (orthogonal camera) or move the camera
 * closer or further away from the focal point in the scene.
 */
void NavigationStyle::zoom(SoCamera * cam, float diffvalue)
{
    if (!cam) // can happen for empty scenegraph
        return;

    animator->stop();

    SoType t = cam->getTypeId();
    SbName tname = t.getName();

    // This will be in the range of <0, ->>.
    auto multiplicator = float(exp(diffvalue));

    if (t.isDerivedFrom(SoOrthographicCamera::getClassTypeId())) {

        // Since there's no perspective, "zooming" in the original sense
        // of the word won't have any visible effect. So we just increase
        // or decrease the field-of-view values of the camera instead, to
        // "shrink" the projection size of the model / scene.

        auto oc = static_cast<SoOrthographicCamera *>(cam);
        oc->height = oc->height.getValue() * multiplicator;

    }
    else {
        // FrustumCamera can be found in the SmallChange CVS module (it's
        // a camera that lets you specify (for instance) an off-center
        // frustum (similar to glFrustum())
        if (!t.isDerivedFrom(SoPerspectiveCamera::getClassTypeId()) &&
            tname != "FrustumCamera") {
#ifdef FC_DEBUG
                static SbBool first = true;
                if (first) {
                    SoDebugError::postWarning("NavigationStyle::zoom",
                                              "Unknown camera type, "
                                              "will zoom by moving position, "
                                              "but this might not be correct.");
                    first = false;
                }
#endif
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
        float maxDistance = std::sqrt(std::numeric_limits<float>::max());
        if (distorigo > maxDistance) {
            // do nothing here
        }
        else {
            cam->position = newpos;
            cam->focalDistance = newfocaldist;
        }
    }

    if (this->currentmode != NavigationStyle::IDLE) {
        hasZoomed = true;
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

/*!
 * Returns the steps if the mouse wheel is rotated
 */
int NavigationStyle::getDelta() const
{
    return 120;
}

void NavigationStyle::doZoom(SoCamera* camera, int wheeldelta, const SbVec2f& pos)
{
    float value = this->zoomStep * wheeldelta / float(getDelta());
    if (this->invertZoom)
        value = -value;
    doZoom(camera, value, pos);
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

        // Change the position of the rotation center indicator after zooming at cursor
        // Rotation mode is WindowCenter
        if (!rotationCenterMode) {
            viewer->changeRotationCenterPosition(getFocalPoint());

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
            findBoundingSphere();
#endif
        }
    }
}
void NavigationStyle::doScale(SoCamera * cam, float factor)
{
    if (cam->getTypeId() == SoOrthographicCamera::getClassTypeId()) {
        float height = static_cast<SoOrthographicCamera*>(cam)->height.getValue() * factor;
        static_cast<SoOrthographicCamera*>(cam)->height = height;
    }
    else if (cam->getTypeId() == SoPerspectiveCamera::getClassTypeId()) {
        float height = static_cast<SoPerspectiveCamera*>(cam)->heightAngle.getValue() / 2.0f;
        height = 2.0f * atan(tan(height) * factor);
        static_cast<SoPerspectiveCamera*>(cam)->heightAngle = height;
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

SbVec3f NavigationStyle::getRotationCenter(SbBool& found) const
{
    found = this->rotationCenterFound;
    return this->rotationCenter;
}

void NavigationStyle::setRotationCenter(const SbVec3f& cnt)
{
    this->rotationCenter = cnt;
    this->rotationCenterFound = true;
}

SbVec3f NavigationStyle::getFocalPoint() const
{
    SoCamera* cam = viewer->getSoRenderManager()->getCamera();
    if (!cam)
        return {0,0,0};

    // Find global coordinates of focal point.
    SbVec3f direction;
    cam->orientation.getValue().multVec(SbVec3f(0, 0, -1), direction);
    SbVec3f focal = cam->position.getValue() +
                    cam->focalDistance.getValue() * direction;
    return focal;
}

/** Uses the sphere sheet projector to map the mouse position onto
 * a 3D point and find a rotation from this and the last calculated point.
 */
void NavigationStyle::spin(const SbVec2f & pointerpos)
{
    if (this->log.historysize < 2)
        return;
    assert(this->spinprojector);

    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    SbVec2s glsize(vp.getViewportSizePixels());
    SbVec2f lastpos;
    lastpos[0] = float(this->log.position[1][0]) / float(std::max((int)(glsize[0]-1), 1));
    lastpos[1] = float(this->log.position[1][1]) / float(std::max((int)(glsize[1]-1), 1));

    float sensitivity = getSensitivity();

    // Adjust the spin projector sphere to the screen position of the rotation center when the mouse intersects an object
    if ((getOrbitStyle() == Trackball || getOrbitStyle() == TrackballClassic || getOrbitStyle() == RoundedArcball)
        && rotationCenterMode & RotationCenterMode::ScenePointAtCursor && rotationCenterFound
        && rotationCenterIsScenePointAtCursor) {
        const auto pointOnScreen = viewer->getPointOnViewport(rotationCenter);
        const auto sphereCenter = 2 * normalizePixelPos(pointOnScreen) - SbVec2f {1, 1};

        float x, y;
        sphereCenter.getValue(x, y);

        const float sphereScale = 1 + sphereCenter.length();
        const float radius = FCSphereSheetProjector::defaultSphereRadius * sphereScale;
        sensitivity *= sphereScale;

        spinprojector->setSphere(SbSphere {SbVec3f {x, y, 0}, radius});
    }
    else {
        spinprojector->setSphere(SbSphere {SbVec3f {0, 0, 0}, FCSphereSheetProjector::defaultSphereRadius});
    }

    // 0000333: Turntable camera rotation
    SbMatrix mat;
    viewer->getSoRenderManager()->getCamera()->orientation.getValue().getValue(mat);
    this->spinprojector->setWorkingSpace(mat);

    this->spinprojector->project(lastpos);
    SbRotation r;
    this->spinprojector->projectAndGetRotation(pointerpos, r);
    if (sensitivity > 1.0f) {
        SbVec3f axis;
        float radians{};
        r.getValue(axis, radians);
        radians = sensitivity * radians;
        r.setValue(axis, radians);
    }
    r.invert();

    if (this->rotationCenterMode && this->rotationCenterFound) {
        this->reorientCamera(viewer->getSoRenderManager()->getCamera(), r, rotationCenter);
    }
    else {
        this->reorientCamera(viewer->getSoRenderManager()->getCamera(), r);
    }

    // Calculate an average angle magnitude value to make the transition
    // to a possible spin animation mode appear smooth.

    SbVec3f dummy_axis, newaxis;
    float acc_angle{}, newangle{};
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

    if (this->currentmode != NavigationStyle::IDLE) {
        hasDragged = true;
    }
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
void NavigationStyle::spin_simplified(SbVec2f curpos, SbVec2f prevpos)
{
    assert(this->spinprojector);

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
        float radians{};
        r.getValue(axis, radians);
        radians = sensitivity * radians;
        r.setValue(axis, radians);
    }
    r.invert();

    if (this->rotationCenterMode && this->rotationCenterFound) {
        this->reorientCamera(viewer->getSoRenderManager()->getCamera(), r, rotationCenter);
    }
    else {
        this->reorientCamera(viewer->getSoRenderManager()->getCamera(), r);
    }

    hasDragged = true;
}

SbBool NavigationStyle::doSpin()
{
    if (this->log.historysize >= 3) {
        SbTime stoptime = (SbTime::getTimeOfDay() - this->log.time[0]);
        if (isSpinningAnimationEnabled() && stoptime.getValue() < 0.100) {
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
            float radians{};
            rot.getValue(axis, radians);
            if ((radians > 0.01f) && (deltatime < 0.300)) {
                viewer->startSpinningAnimation(axis, radians * 5);
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
    rotationCenterIsScenePointAtCursor = false;

    // mode is WindowCenter
    if (!this->rotationCenterMode) {
        setRotationCenter(getFocalPoint());
    }

    //Option to get point on model (slow) or always on focal plane (fast)
    //
    // mode is ScenePointAtCursor to get exact point if possible
    if (this->rotationCenterMode & NavigationStyle::RotationCenterMode::ScenePointAtCursor) {
        SoRayPickAction rpaction(viewer->getSoRenderManager()->getViewportRegion());
        rpaction.setPoint(this->localPos);
        rpaction.setRadius(viewer->getPickRadius());
        rpaction.apply(viewer->getSoRenderManager()->getSceneGraph());

        SoPickedPoint * picked = rpaction.getPickedPoint();
        if (picked) {
            setRotationCenter(picked->getPoint());
            rotationCenterIsScenePointAtCursor = true;
            return;
        }
    }

    // mode is FocalPointAtCursor or a ScenePointAtCursor failed
    if (this->rotationCenterMode & NavigationStyle::RotationCenterMode::FocalPointAtCursor) {
        // get the intersection point of the ray and the focal plane
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();

        SoCamera* cam = viewer->getSoRenderManager()->getCamera();
        if (!cam) // no camera
            return;
        SbViewVolume vv = cam->getViewVolume(ratio);

        SbLine line;
        SbVec2f currpos = ev->getNormalizedPosition(vp);
        vv.projectPointToLine(currpos, line);
        SbVec3f current_planept;
        SbPlane panplane = vv.getPlane(cam->focalDistance.getValue());
        panplane.intersect(line, current_planept);

        setRotationCenter(current_planept);
    }

    // mode is BoundingBoxCenter or a ScenePointAtCursor failed
    if (this->rotationCenterMode & NavigationStyle::RotationCenterMode::BoundingBoxCenter) {
        const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
        float ratio = vp.getViewportAspectRatio();

        SoCamera* cam = viewer->getSoRenderManager()->getCamera();
        if (!cam) // no camera
            return;

        // Get the bounding box center of the physical object group
        SoGetBoundingBoxAction action(viewer->getSoRenderManager()->getViewportRegion());
        action.apply(viewer->objectGroup);
        SbBox3f boundingBox = action.getBoundingBox();
        SbVec3f boundingBoxCenter = boundingBox.getCenter();
        setRotationCenter(boundingBoxCenter);

        // To drag around the center point of the bbox we have to determine
        // its projection on the screen because this information is used in
        // NavigationStyle::spin() for the panning
        SbViewVolume vv = cam->getViewVolume(ratio);
        vv.projectToScreen(boundingBoxCenter, boundingBoxCenter);
        SbVec2s size = vp.getViewportSizePixels();
        auto tox = static_cast<short>(boundingBoxCenter[0] * size[0]);
        auto toy = static_cast<short>(boundingBoxCenter[1] * size[1]);
        this->localPos.setValue(tox, toy);
    }
}

SbVec2f NavigationStyle::normalizePixelPos(SbVec2s pixpos)
{
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    return {(float) pixpos[0] / (float) std::max((int)(size[0] - 1), 1),
            (float) pixpos[1] / (float) std::max((int)(size[1] - 1), 1)};
}

SbVec2f NavigationStyle::normalizePixelPos(SbVec2f pixpos)
{
    const SbViewportRegion & vp = viewer->getSoRenderManager()->getViewportRegion();
    const SbVec2s size(vp.getViewportSizePixels());
    return {pixpos[0] / (float) std::max((int)(size[0] - 1), 1),
            pixpos[1] / (float) std::max((int)(size[1] - 1), 1)};
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


SbBool NavigationStyle::handleEventInForeground(const SoEvent* const e)
{
    SoHandleEventAction action(viewer->getSoRenderManager()->getViewportRegion());
    action.setEvent(e);
    action.setPickRadius(viewer->getPickRadius());
    action.apply(viewer->foregroundroot);
    return action.isHandled();
}

/**
 * @brief Decide if it should be possible to start any animation
 *
 * If the enable flag is false and we're currently animating, the animation will be stopped
 */
void NavigationStyle::setAnimationEnabled(const SbBool enable)
{
    animationEnabled = enable;
    if (!enable && isAnimating()) {
        animator->stop();
    }
}

/**
 * @brief Decide if it should be possible to start a spin animation of the model in the viewer by releasing the mouse button while dragging
 *
 * If the enable flag is false and we're currently animating, the spin animation will be stopped
 */
void NavigationStyle::setSpinningAnimationEnabled(const SbBool enable)
{
    spinningAnimationEnabled = enable;
    if (!enable && isSpinning()) {
        animator->stop();
    }
}

/**
 * @return Whether or not it is possible to start any animation
 */
SbBool NavigationStyle::isAnimationEnabled() const
{
    return animationEnabled;
}

/**
 * @return Whether or not it is possible to start a spinning animation e.g. after dragging
 */
SbBool NavigationStyle::isSpinningAnimationEnabled() const
{
    return animationEnabled && spinningAnimationEnabled;
}

/**
 * @return Whether or not any animation is currently active
 */
SbBool NavigationStyle::isAnimating() const
{
    return animator->isAnimating();
}

/**
 * @return Whether or not a spinning animation is currently active e.g. after a user drag
 */
SbBool NavigationStyle::isSpinning() const
{
    return currentmode == NavigationStyle::SPINNING;
}

void NavigationStyle::startAnimating(const std::shared_ptr<NavigationAnimation>& animation, bool wait) const
{
    if (wait) {
        animator->startAndWait(animation);
    }
    else {
        animator->start(animation);
    }
}

void NavigationStyle::stopAnimating() const
{
    animator->stop();
}

void NavigationStyle::setSensitivity(float val)
{
    this->sensitivity = val;
}

float NavigationStyle::getSensitivity() const
{
    return this->sensitivity;
}

void NavigationStyle::setResetCursorPosition(SbBool on)
{
    this->resetcursorpos = on;
}

SbBool NavigationStyle::isResetCursorPosition() const
{
    return this->resetcursorpos;
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

void NavigationStyle::setRotationCenterMode(NavigationStyle::RotationCenterModes mode)
{
    this->rotationCenterMode = mode;
}

NavigationStyle::RotationCenterModes NavigationStyle::getRotationCenterMode() const
{
    return this->rotationCenterMode;
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

void NavigationStyle::abortSelection()
{
    pcPolygon.clear();
    if (mouseSelection) {
        mouseSelection->releaseMouseModel(true);
        delete mouseSelection;
        mouseSelection = nullptr;
    }
}

void NavigationStyle::stopSelection()
{
    pcPolygon.clear();
    if (mouseSelection) {
        mouseSelection->releaseMouseModel();
        delete mouseSelection;
        mouseSelection = nullptr;
    }
}

SbBool NavigationStyle::isSelecting() const
{
    return (mouseSelection ? true : false);
}

const std::vector<SbVec2s>& NavigationStyle::getPolygon(SelectionRole* role) const
{
    if (role)
        *role = this->selectedRole;
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
void NavigationStyle::clearLog()
{
    this->log.historysize = 0;
}

void NavigationStyle::syncModifierKeys(const SoEvent * const ev)
{
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
}

// The viewer is a state machine, and all changes to the current state
// are made through this call.
void NavigationStyle::setViewingMode(const ViewerMode newmode)
{
    const ViewerMode oldmode = this->currentmode;
    if (newmode == oldmode) {

        // The rotation center could have been changed even if the mode has not changed
        if (newmode == NavigationStyle::DRAGGING && rotationCenterFound) {
            viewer->changeRotationCenterPosition(rotationCenter);
        }

        return;
    }

    if (newmode == NavigationStyle::IDLE) {
        hasPanned = false;
        hasDragged = false;
        hasZoomed = false;
    }

    switch (newmode) {
        case DRAGGING:
            // Set up initial projection point for the projector object when
            // first starting a drag operation.
            animator->stop();
            viewer->showRotationCenter(true);

#if (COIN_MAJOR_VERSION * 100 + COIN_MINOR_VERSION * 10 + COIN_MICRO_VERSION < 403)
            findBoundingSphere();
#endif

            this->spinprojector->project(this->lastmouseposition);
            this->interactiveCountInc();
            this->clearLog();
            break;

        case SPINNING:
            this->interactiveCountInc();
            viewer->getSoRenderManager()->scheduleRedraw();
            break;

        case PANNING:
            animator->stop();
            setupPanningPlane(viewer->getSoRenderManager()->getCamera());
            this->interactiveCountInc();
            break;

        case ZOOMING:
            animator->stop();
            this->interactiveCountInc();
            break;

        case BOXZOOM:
            animator->stop();
            this->interactiveCountInc();
            break;

    default: // include default to avoid compiler warnings.
            break;
    }

    switch (oldmode) {
        case SPINNING:
        case DRAGGING:
            viewer->showRotationCenter(false);
            [[fallthrough]];
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
            selectedRole = mouseSelection->selectedRole();
            delete mouseSelection;
            mouseSelection = nullptr;
            syncWithEvent(ev);
            return NavigationStyle::processSoEvent(ev);
        }
        else if (hd==AbstractMouseSelection::Cancel) {
            pcPolygon.clear();
            delete mouseSelection;
            mouseSelection = nullptr;
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
        if (SoMouseButtonEvent::isButtonReleaseEvent(ev, SoMouseButtonEvent::BUTTON1)) {
            if (!ev->wasCtrlDown()) {
                Gui::Selection().clearSelection();
            }
        }
    }

    return processed;
}

SbBool NavigationStyle::processSoEvent(const SoEvent * const ev)
{
    bool processed = false;
    bool offeredtoViewerEventBase = false;

    //handle mouse wheel zoom
    if (ev->isOfType(SoMouseWheelEvent::getClassTypeId())) {
        auto const event = static_cast<const SoMouseWheelEvent *>(ev);
        processed = processWheelEvent(event);
        viewer->processSoEventBase(ev);
        offeredtoViewerEventBase = true;
    }

    if (!processed && !offeredtoViewerEventBase) {
        processed = viewer->processSoEventBase(ev);
    }

    return processed;
}

void NavigationStyle::syncWithEvent(const SoEvent * const ev)
{
    // Events when in "ready-to-seek" mode are ignored, except those
    // which influence the seek mode itself -- these are handled further
    // up the inheritance hierarchy.
    if (this->isSeekMode()) {
        return;
    }

    const SoType type(ev->getTypeId());

    // Mismatches in state of the modifier keys happens if the user
    // presses or releases them outside the viewer window.
    syncModifierKeys(ev);

    // Keyboard handling
    if (type.isDerivedFrom(SoKeyboardEvent::getClassTypeId())) {
        auto const event = static_cast<const SoKeyboardEvent *>(ev);
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
        auto const event = static_cast<const SoMouseButtonEvent *>(ev);
        const int button = event->getButton();
        const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
#ifdef FC_DEBUG
        SoDebugError::postInfo("processSoEvent", "button = %d", button);
#endif
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
        auto oCam = static_cast<SoOrthographicCamera *>(camera);
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

SbBool NavigationStyle::processKeyboardEvent(const SoKeyboardEvent * const event)
{
    SbBool processed = false;
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
        case SoKeyboardEvent::S:
        case SoKeyboardEvent::HOME:
        case SoKeyboardEvent::LEFT_ARROW:
        case SoKeyboardEvent::UP_ARROW:
        case SoKeyboardEvent::RIGHT_ARROW:
        case SoKeyboardEvent::DOWN_ARROW:
        if (!this->isViewing())
                this->setViewing(true);
            break;
    case SoKeyboardEvent::PAGE_UP:
    {
            processed = true;
            const SbVec2f posn = normalizePixelPos(event->getPosition());
            doZoom(viewer->getSoRenderManager()->getCamera(), getDelta(), posn);
            break;
        }
    case SoKeyboardEvent::PAGE_DOWN:
    {
            processed = true;
            const SbVec2f posn = normalizePixelPos(event->getPosition());
            doZoom(viewer->getSoRenderManager()->getCamera(), -getDelta(), posn);
            break;
        }
        default:
            break;
    }

    return processed;
}

SbBool NavigationStyle::processClickEvent(const SoMouseButtonEvent * const event)
{
    // issue #0002433: avoid to swallow the UP event if down the
    // scene graph somewhere a dialog gets opened
    SbBool processed = false;
    const SbBool press = event->getState() == SoButtonEvent::DOWN ? true : false;
    if (press) {
        SbTime tmp = (event->getTime() - mouseDownConsumedEvent.getTime());
        float dci = (float)QApplication::doubleClickInterval()/1000.0f;
        // a double-click?
        if (tmp.getValue() < dci) {
            mouseDownConsumedEvent = *event;
            mouseDownConsumedEvent.setTime(event->getTime());
            processed = true;
        }
        else {
            mouseDownConsumedEvent.setTime(event->getTime());
            // 'ANY' is used to mark that we don't know yet if it will
            // be a double-click event.
            mouseDownConsumedEvent.setButton(SoMouseButtonEvent::ANY);
        }
    }
    else if (!press) {
        if (mouseDownConsumedEvent.getButton() == SoMouseButtonEvent::BUTTON1) {
            // now handle the postponed event
            NavigationStyle::processSoEvent(&mouseDownConsumedEvent);
            mouseDownConsumedEvent.setButton(SoMouseButtonEvent::ANY);
        }
    }

    return processed;
}

SbBool NavigationStyle::processWheelEvent(const SoMouseWheelEvent * const event)
{
    const SbVec2s pos(event->getPosition());
    const SbVec2f posn = normalizePixelPos(pos);

    //handle mouse wheel zoom
    doZoom(viewer->getSoRenderManager()->getCamera(),
           event->getDelta(), posn);
    return true;
}

void NavigationStyle::setPopupMenuEnabled(const SbBool on)
{
    this->menuenabled = on;
}

SbBool NavigationStyle::isPopupMenuEnabled() const
{
    return this->menuenabled;
}

void NavigationStyle::openPopupMenu(const SbVec2s& position)
{
    Q_UNUSED(position);
    // ask workbenches and view provider, ...
    MenuItem view;
    Gui::Application::Instance->setupContextMenu("View", &view);

    auto contextMenu = new QMenu(viewer->getGLWidget());
    MenuManager::getInstance()->setupContextMenu(&view, *contextMenu);
    contextMenu->setAttribute(Qt::WA_DeleteOnClose);

    auto navMenu = contextMenu->addMenu(QObject::tr("Navigation styles"));
    auto navMenuGroup = new QActionGroup(navMenu);

    // add submenu at the end to select navigation style
    const std::map<Base::Type, std::string> styles = UserNavigationStyle::getUserFriendlyNames();
    for (const auto &style : styles) {
        const QString name = QApplication::translate(style.first.getName(), style.second.c_str());
        QAction *item = navMenuGroup->addAction(name);
        navMenu->addAction(item);
        item->setCheckable(true);

        if (const Base::Type item_style = style.first; item_style != this->getTypeId()) {
            auto triggeredFun = [this, item_style](){
                QWidget *widget = viewer->getWidget();
                while (widget && !widget->inherits("Gui::View3DInventor"))
                    widget = widget->parentWidget();
                if (widget) {
                    // this is the widget where the viewer is embedded
                    QEvent *ns_event = new NavigationStyleEvent(item_style);
                    QApplication::postEvent(widget, ns_event);
                }
            };
            item->connect(item, &QAction::triggered, triggeredFun);
        } else
            item->setChecked(true);
    }

    contextMenu->popup(QCursor::pos());
}

PyObject* NavigationStyle::getPyObject()
{
    if (pythonObject.is(nullptr)) {
        // ref counter is set to 1
        pythonObject = Py::asObject(new NavigationStylePy(this));
    }
    return Py::new_reference_to(pythonObject);
}

// ----------------------------------------------------------------------------------

TYPESYSTEM_SOURCE_ABSTRACT(Gui::UserNavigationStyle,Gui::NavigationStyle)

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

    for (auto & type : types) {
        if (type != UserNavigationStyle::getClassTypeId()) {
            std::unique_ptr<UserNavigationStyle> inst(static_cast<UserNavigationStyle*>(type.createInstance()));
            if (inst) {
                names[type] = inst->userFriendlyName();
            }
        }
    }
    return names;
}
