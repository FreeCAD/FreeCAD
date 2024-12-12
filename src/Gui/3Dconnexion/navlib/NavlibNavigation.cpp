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
#include "NavlibInterface.h"

#include <QMatrix4x4>
#include <QMdiArea>
#include <QTabBar>

#include <Inventor/SbDPMatrix.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbViewVolume.h>
#include <Inventor/SoRenderManager.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <Inventor/fields/SoSFVec3f.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>

#include <Gui/Application.h>
#include <Gui/MDIView.h>
#include <Gui/MainWindow.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/Workbench.h>
#include <Gui/WorkbenchManager.h>

#include <Base/BoundBox.h>

NavlibInterface::NavlibInterface()
    : CNavigation3D(false, navlib::nlOptions_t::no_ui),
      patternInitialized(false),
      activeTab({-1, ""})
{}

NavlibInterface::~NavlibInterface()
{
    disableNavigation();

    if (pivot.pVisibility != nullptr)
        pivot.pVisibility->unref();
}

void NavlibInterface::initializePattern() const
{
    if (patternInitialized)
        return;

    if (hitTestingResolution > 0) {
        hitTestPattern[0][0] = 0.0;
        hitTestPattern[0][1] = 0.0;
    }

    for (uint32_t i = 1; i < hitTestingResolution; i++) {
        float coefficient = sqrt(static_cast<float>(i) / static_cast<float>(hitTestingResolution));
        float angle = 2.4f * static_cast<float>(i);
        float x = coefficient * sin(angle);
        float y = coefficient * cos(angle);
        hitTestPattern[i][0] = x;
        hitTestPattern[i][1] = y;
    }

    patternInitialized = true;
}

long NavlibInterface::GetPointerPosition(navlib::point_t& position) const
{
    if (is2DView()) {
        QPoint point = currentView.pView2d->mapFromGlobal(QCursor::pos());
        point = currentView.pView2d->mapToScene(point).toPoint();
        position.x = point.x();
        position.y = -point.y();

        return 0;
    }

    if (is3DView()) {
        const Gui::View3DInventorViewer* const inventorViewer = currentView.pView3d->getViewer();
        if (inventorViewer == nullptr)
            return navlib::make_result_code(navlib::navlib_errc::no_data_available);

        QPoint viewPoint = currentView.pView3d->mapFromGlobal(QCursor::pos());
        viewPoint.setY(currentView.pView3d->height() - viewPoint.y());

        double scaling = inventorViewer->devicePixelRatio();
        viewPoint *= scaling;

        SbVec3f worldPosition =
            inventorViewer->getPointOnFocalPlane(SbVec2s(viewPoint.x(), viewPoint.y()));
        std::copy(worldPosition.getValue(), worldPosition.getValue() + 3, &position.x);

        wasPointerPick = true;

        return 0;
    }
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

template<class CameraType>
CameraType NavlibInterface::getCamera() const
{
    if (is3DView()) {
        const Gui::View3DInventorViewer* inventorViewer = currentView.pView3d->getViewer();
        if (inventorViewer != nullptr)
            return dynamic_cast<CameraType>(inventorViewer->getCamera());
    }
    return nullptr;
}

void NavlibInterface::onViewChanged(const Gui::MDIView* view)
{
    if (view == nullptr)
        return;

    currentView.pView3d = dynamic_cast<const Gui::View3DInventor*>(view);
    currentView.pView2d = nullptr;
    if (currentView.pView3d != nullptr) {
        const Gui::View3DInventorViewer* const inventorViewer = currentView.pView3d->getViewer();
        if (inventorViewer == nullptr)
            return;

        auto pGroup = dynamic_cast<SoGroup* const>(inventorViewer->getSceneGraph());
        if (pGroup == nullptr)
            return;

        if (pGroup->findChild(pivot.pVisibility) == -1)
            pGroup->addChild(pivot.pVisibility);

        navlib::box_t extents;
        navlib::matrix_t camera;

        GetModelExtents(extents);
        GetCameraMatrix(camera);

        Write(navlib::model_extents_k, extents);
        Write(navlib::view_affine_k, camera);

        return;
    }

    for (auto viewinternal : view->findChildren<QGraphicsView*>()) {
        QList<QGraphicsView*> views = viewinternal->scene()->views();
        for (QGraphicsView* view : views) {
            if (view->isActiveWindow()) {
                currentView.pView2d = view;
                return;
            }
        }
    }
}

void NavlibInterface::enableNavigation()
{
    PutProfileHint("FreeCAD");
    CNav3D::EnableNavigation(true, errorCode);
    if (errorCode)
        return;

    PutFrameTimingSource(TimingSource::SpaceMouse);

    Gui::Application::Instance->signalActivateView.connect(
        boost::bind(&NavlibInterface::onViewChanged, this, boost::placeholders::_1));

    Gui::Application::Instance->signalActivateWorkbench.connect([this](const char* wb) {
        exportCommands(std::string(wb));
    });

    exportCommands("StartWorkbench");

    initializePivot();
    connectActiveTab();
}

void NavlibInterface::connectActiveTab()
{
    auto pQMdiArea = Gui::MainWindow::getInstance()->findChild<QMdiArea*>();
    if (pQMdiArea == nullptr)
        return;

    auto pQTabBar = pQMdiArea->findChild<QTabBar*>();
    if (pQTabBar == nullptr)
        return;

    pQTabBar->connect(pQTabBar, &QTabBar::currentChanged, [this, pQTabBar](int idx) {
        activeTab = {idx, idx >= 0 ? pQTabBar->tabText(idx).toStdString() : ""};
    });
}

void NavlibInterface::disableNavigation()
{
    CNav3D::EnableNavigation(false, errorCode);
}

long NavlibInterface::GetCameraMatrix(navlib::matrix_t& matrix) const
{
    if (activeTab.first == -1 || activeTab.second == "Start page")
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    if (is3DView()) {
        auto pCamera = getCamera<SoCamera*>();
        if (pCamera == nullptr)
            return navlib::make_result_code(navlib::navlib_errc::function_not_supported);

        SbMatrix cameraMatrix;

        pCamera->orientation.getValue().getValue(cameraMatrix);

        for (int i = 0; i < 4; i++)
            std::copy(cameraMatrix[i], cameraMatrix[i] + 4, &matrix.m00 + 4 * i);

        const SbVec3f position = pCamera->position.getValue();
        std::copy(position.getValue(), position.getValue() + 3, &matrix.m30);

        return 0;
    }
    if (is2DView()) {
        QMatrix4x4 data;
        const QWidget* viewport = currentView.pView2d->viewport();
        const QPointF viewportCenter(viewport->width() / 2.0, viewport->height() / 2.0);
        const QPointF scenePoint = currentView.pView2d->mapToScene(viewportCenter.toPoint());

        // Only XY translations are considered for 2D view. The Z coordinate can be a constant value.
        data(0, 3) = scenePoint.x();
        data(1, 3) = -scenePoint.y();
        data(2, 3) = 0.0;

        std::copy(data.data(), data.data() + 16, &matrix.m00);

        return 0;
    }
    return navlib::make_result_code(navlib::navlib_errc::function_not_supported);
}

long NavlibInterface::SetCameraMatrix(const navlib::matrix_t& matrix)
{
    if (is2DView()) {
        QMatrix4x4 data;

        std::copy(&matrix.m00, &matrix.m33, data.data());
        currentView.pView2d->centerOn(data(0, 3), -data(1, 3));

        return 0;
    }

    if (is3DView()) {
        auto pCamera = getCamera<SoCamera*>();
        if (pCamera == nullptr)
            return navlib::make_result_code(navlib::navlib_errc::no_data_available);

        SbMatrix cameraMatrix(matrix(0, 0), matrix(0, 1), matrix(0, 2), matrix(0, 3),
                              matrix(1, 0), matrix(1, 1), matrix(1, 2), matrix(1, 3),
                              matrix(2, 0), matrix(2, 1), matrix(2, 2), matrix(2, 3),
                              matrix(3, 0), matrix(3, 1), matrix(3, 2), matrix(3, 3));

        pCamera->orientation = SbRotation(cameraMatrix);
        pCamera->position.setValue(matrix(3, 0), matrix(3, 1), matrix(3, 2));
        pCamera->touch();

        return 0;
    }
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetViewFrustum(navlib::frustum_t& frustum) const
{
    const auto pCamera = getCamera<SoPerspectiveCamera* const>();
    if (pCamera == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    const SbViewVolume viewVolume = pCamera->getViewVolume(pCamera->aspectRatio.getValue());
    float halfHeight = viewVolume.getHeight() / 2.0f;
    float halfWidth = viewVolume.getWidth() / 2.0f;

    frustum = {-halfWidth,
               halfWidth,
               -halfHeight,
               halfHeight,
               viewVolume.getNearDist(),
               10.0f * (viewVolume.getNearDist() + viewVolume.nearToFar)};

    return 0;
}

long NavlibInterface::SetViewFrustum(const navlib::frustum_t& frustum)
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetViewExtents(navlib::box_t& extents) const
{
    if (is2DView()) {
        const QRectF viewRectangle = currentView.pView2d->mapToScene(
				currentView.pView2d->viewport()->geometry()).boundingRect();

        extents.min.x = viewRectangle.topLeft().x();
        extents.min.y = viewRectangle.topLeft().y();
        extents.max.x = viewRectangle.bottomRight().x();
        extents.max.y = viewRectangle.bottomRight().y();
        extents.min.z = -1;
        extents.max.z = 0;

        return 0;
    }

    const auto pCamera = getCamera<SoOrthographicCamera* const>();
    if (pCamera == nullptr)
        return navlib::make_result_code(navlib::navlib_errc::no_data_available);

    const SbViewVolume viewVolume = pCamera->getViewVolume(pCamera->aspectRatio.getValue());
    const double halfHeight = static_cast<double>(viewVolume.getHeight() / 2.0f);
    const double halfWidth = static_cast<double>(viewVolume.getWidth() / 2.0f);
    const double halfDepth = 1.0e8;

    extents = {-halfWidth,
               -halfHeight,
               -halfDepth,
               halfWidth,
               halfHeight,
               halfDepth};

    return 0;
}

long NavlibInterface::SetViewExtents(const navlib::box_t& extents)
{
    if (is2DView()) {
        const QRectF viewRectangle = currentView.pView2d->mapToScene(
				currentView.pView2d->viewport()->geometry()).boundingRect();

        const float scaling = viewRectangle.height() / (extents.max.y - extents.min.y);
        QTransform transform = currentView.pView2d->transform();

        transform.setMatrix(transform.m11() * scaling,
                            transform.m12(),
                            transform.m13(),
                            transform.m21(),
                            transform.m22() * scaling,
                            transform.m23(),
                            transform.m31(),
                            transform.m32(),
                            transform.m33());

        currentView.pView2d->setTransform(transform);

        return 0;
    }

    if (is3DView()) {
        auto pCamera = getCamera<SoOrthographicCamera* const>();
        if (pCamera == nullptr)
            return navlib::make_result_code(navlib::navlib_errc::no_data_available);

        navlib::box_t oldExtents;
        GetViewExtents(oldExtents);

        pCamera->scaleHeight(extents.max.x / oldExtents.max.x);
        orthoNearDistance = pCamera->nearDistance.getValue();

        return 0;
    }

    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetViewFOV(double& fov) const
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::SetViewFOV(double fov)
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetIsViewPerspective(navlib::bool_t& perspective) const
{
    auto pPerspectiveCamera = getCamera<SoPerspectiveCamera* const>();
    if (pPerspectiveCamera != nullptr) {
        perspective = true;
        return 0;
    }

    auto pOrthographicCamera = getCamera<SoOrthographicCamera* const>();
    if (pOrthographicCamera != nullptr || is2DView()) {
        perspective = false;
        return 0;
    }

    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetModelExtents(navlib::box_t& extents) const
{
    if (is3DView()) {
        const Gui::View3DInventorViewer* const inventorViewer = currentView.pView3d->getViewer();
        if (inventorViewer == nullptr)
            return navlib::make_result_code(navlib::navlib_errc::no_data_available);

        SoGetBoundingBoxAction action(inventorViewer->getSoRenderManager()->getViewportRegion());

        action.apply(inventorViewer->getSceneGraph());
        const SbBox3f boundingBox = action.getBoundingBox();

        std::copy(
            boundingBox.getMin().getValue(), boundingBox.getMin().getValue() + 3, &extents.min.x);

        std::copy(
            boundingBox.getMax().getValue(), boundingBox.getMax().getValue() + 3, &extents.max.x);

        return 0;
    }

    if (is2DView()) {
        const QRectF sceneExtents = currentView.pView2d->scene()->itemsBoundingRect();

        extents.min.x = sceneExtents.topLeft().x();
        extents.min.y = -sceneExtents.bottomRight().y();
        extents.max.x = sceneExtents.bottomRight().x();
        extents.max.y = -sceneExtents.topLeft().y();
        extents.max.z = 0;
        extents.min.z = -1;

        return 0;
    }
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::SetTransaction(long value)
{
    return navlib::make_result_code(navlib::navlib_errc::no_data_available);
}

long NavlibInterface::GetFrontView(navlib::matrix_t& matrix) const
{
    matrix = {1., 0., 0., 0.,
              0., 0., 1., 0.,
              0., -1., 0., 0.,
              0., 0., 0., 1.};
    return 0;
}

long NavlibInterface::GetCoordinateSystem(navlib::matrix_t& matrix) const
{
    matrix = {1., 0., 0., 0.,
              0., 0., -1., 0.,
              0., 1., 0., 0.,
              0., 0., 0., 1.};
    return 0;
}

long NavlibInterface::GetIsViewRotatable(navlib::bool_t& isRotatable) const
{
    isRotatable = is3DView();
    return 0;
}

long NavlibInterface::GetUnitsToMeters(double &units) const
{
    units = 0.001;
    return 0;
}

bool NavlibInterface::is3DView() const
{
    return currentView.pView3d != nullptr;
}

bool NavlibInterface::is2DView() const
{
    return currentView.pView2d != nullptr;
}
