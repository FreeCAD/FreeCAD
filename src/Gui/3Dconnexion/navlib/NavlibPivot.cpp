/***************************************************************************
 *   Copyright (c) 2014-2023 3Dconnexion.                                  *
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

#include <PreCompiled.h>

#ifndef _PreComp_
#include <limits>
#endif

#include <QImage>
#include <QScreen>
#include <QString>
#include <QWindow>

#include "NavlibInterface.h"
#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SoPickedPoint.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodes/SoCamera.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoResetTransform.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>

long NavlibInterface::GetSelectionTransform(navlib::matrix_t&) const
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetIsSelectionEmpty(navlib::bool_t& empty) const
{
    empty = !Gui::Selection().hasSelection();
    return 0;
}

long NavlibInterface::SetSelectionTransform(const navlib::matrix_t&)
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetPivotPosition(navlib::point_t&) const
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::SetPivotPosition(const navlib::point_t& position)
{
    if (pivot.pTransform == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    pivot.pTransform->translation.setValue(position.x, position.y, position.z);
    return 0;
}

long NavlibInterface::IsUserPivot(navlib::bool_t& userPivot) const
{
    userPivot = false;
    return 0;
}

long NavlibInterface::GetPivotVisible(navlib::bool_t& visible) const
{
    if (pivot.pVisibility == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    visible = pivot.pVisibility->whichChild.getValue() == SO_SWITCH_ALL;

    return 0;
}

long NavlibInterface::SetPivotVisible(bool visible)
{
    if (pivot.pVisibility == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    if (visible)
        pivot.pVisibility->whichChild = SO_SWITCH_ALL;
    else
        pivot.pVisibility->whichChild = SO_SWITCH_NONE;

    return 0;
}

extern template SoCamera* NavlibInterface::getCamera<SoCamera*>() const;

long NavlibInterface::GetHitLookAt(navlib::point_t& position) const
{
    if (is2DView() || !is3DView())
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    const Gui::View3DInventorViewer* const inventorViewer = currentView.pView3d->getViewer();
    if (inventorViewer == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    SoNode* pSceneGraph = inventorViewer->getSceneGraph();
    if (pSceneGraph == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    // Prepare the ray-picking object
    SoRayPickAction rayPickAction(inventorViewer->getSoRenderManager()->getViewportRegion());
    SbMatrix cameraMatrix;
    SbVec3f closestHitPoint;
    float minLength = std::numeric_limits<float>::max();

    // Get the camera rotation
    SoCamera* pCamera = getCamera<SoCamera*>();

    if (pCamera == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    pCamera->orientation.getValue().getValue(cameraMatrix);

     // Initialize the samples array if it wasn't done before
    initializePattern();

    navlib::bool_t isPerspective;
    GetIsViewPerspective(isPerspective);

    for (uint32_t i = 0; i < hitTestingResolution; i++) {

        SbVec3f origin;

        if (wasPointerPick) {
            origin = ray.origin;
        }
        else {
            // Scale the sample like it was defined in camera space (placed on XY plane)
            SbVec3f transform(hitTestPattern[i][0] * ray.radius,
                              hitTestPattern[i][1] * ray.radius,
                              0.0f);

            // Apply the model-view transform to a sample (only the rotation)
            cameraMatrix.multVecMatrix(transform, transform);

            // Calculate origin of current hit-testing ray
            origin = ray.origin + transform;
        }

        // Perform the hit-test
        if (isPerspective) {
            rayPickAction.setRay(origin,
                                 ray.direction,
                                 pCamera->nearDistance.getValue(),
                                 pCamera->farDistance.getValue());
        }
        else {
            rayPickAction.setRay(origin, ray.direction);
        }

        rayPickAction.apply(pSceneGraph);
        SoPickedPoint* pickedPoint = rayPickAction.getPickedPoint();

        // Check if there was a hit
        if (pickedPoint != nullptr) {
            SbVec3f hitPoint = pickedPoint->getPoint();
            float distance = (origin - hitPoint).length();

            // Save hit of the lowest depth
            if (distance < minLength) {
                minLength = distance;
                closestHitPoint = hitPoint;
            }
        }

        if (wasPointerPick) {
            wasPointerPick = false;
            break;
        }
    }

    if (minLength < std::numeric_limits<float>::max()) {
        std::copy(closestHitPoint.getValue(), closestHitPoint.getValue() + 3, &position.x);
        return 0;
    }

    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetSelectionExtents(navlib::box_t& extents) const
{
    Base::BoundBox3d boundingBox;
    auto selectionVector = Gui::Selection().getSelection();

    std::for_each(selectionVector.begin(),
                  selectionVector.end(),
                  [&boundingBox](Gui::SelectionSingleton::SelObj& selection) {
                      Gui::ViewProvider* pViewProvider =
                          Gui::Application::Instance->getViewProvider(selection.pObject);

                      if (pViewProvider == nullptr)
                          return navlib::make_result_code(navlib::navlib_errc::no_data_available);

                      boundingBox.Add(pViewProvider->getBoundingBox(selection.SubName, true));

                      return 0l;
                  });

    extents = {{boundingBox.MinX,
                boundingBox.MinY,
                boundingBox.MinZ},
               {boundingBox.MaxX,
                boundingBox.MaxY,
                boundingBox.MaxZ}};

    return 0;
}

long NavlibInterface::SetHitAperture(double aperture)
{
    ray.radius = aperture;
    return 0;
}

long NavlibInterface::SetHitDirection(const navlib::vector_t& direction)
{
    ray.direction.setValue(direction.x, direction.y, direction.z);
    return 0;
}

long NavlibInterface::SetHitLookFrom(const navlib::point_t& eye)
{
    navlib::bool_t isPerspective;

    GetIsViewPerspective(isPerspective);

    if (isPerspective) {
        ray.origin.setValue(eye.x, eye.y, eye.z);
    }
    else {
        auto pCamera = getCamera<SoCamera*>();
        if (pCamera == nullptr) {
            return navlib::make_result_code(navlib::navlib_errc::no_data_available);
        }

        SbVec3f position = pCamera->position.getValue();
        ray.origin = position + orthoNearDistance * ray.direction;
    }
    return 0;
}

long NavlibInterface::SetHitSelectionOnly(bool hitSelection)
{
    ray.selectionOnly = hitSelection;
    return 0;
}

void NavlibInterface::initializePivot()
{
    pivot.pVisibility = new SoSwitch;
    pivot.pTransform = new SoTransform;
    pivot.pResetTransform = new SoResetTransform;
    pivot.pImage = new SoImage;
    pivot.pDepthTestAlways = new SoDepthBuffer;
    pivot.pDepthTestLess = new SoDepthBuffer;

    pivot.pDepthTestAlways->function.setValue(SoDepthBufferElement::ALWAYS);
    pivot.pDepthTestLess->function.setValue(SoDepthBufferElement::LESS);

    pivot.pivotImage = QImage(QStringLiteral(":/icons/3dx_pivot.png"));
    Gui::BitmapFactory().convert(pivot.pivotImage, pivot.pImage->image);

    pivot.pVisibility->ref();
    pivot.pVisibility->whichChild = SO_SWITCH_NONE;
    pivot.pVisibility->addChild(pivot.pDepthTestAlways);
    pivot.pVisibility->addChild(pivot.pTransform);
    pivot.pVisibility->addChild(pivot.pImage);
    pivot.pVisibility->addChild(pivot.pResetTransform);
    pivot.pVisibility->addChild(pivot.pDepthTestLess);
}
