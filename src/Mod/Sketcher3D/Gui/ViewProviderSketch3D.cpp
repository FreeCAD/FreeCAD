// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 Yash Suthar <yashsuthar983@gmail.com>              *
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

#include <QMenu>

#include <Inventor/SoPickedPoint.h>
#include <Inventor/SbLine.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/events/SoKeyboardEvent.h>
#include <Inventor/events/SoMouseButtonEvent.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoNormal.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoScale.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSphere.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTranslation.h>

#include <Precision.hxx>

#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/Rotation.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Inventor/SoAutoZoomTranslation.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolBarManager.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/Selection/SoFCUnifiedSelection.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/Gui/SoBrepEdgeSet.h>
#include <Mod/Part/Gui/SoBrepFaceSet.h>
#include <Mod/Part/Gui/SoBrepPointSet.h>
#include <Mod/Part/Gui/ViewProviderExt.h>
#include <Mod/Sketcher3D/App/GeoEnum3D.h>
#include <Mod/Sketcher3D/App/GeomReferencePlane3D.h>
#include <Mod/Sketcher3D/App/Sketch3DObject.h>

#include "DrawSketchHandler3D.h"
#include "SnapManager3D.h"
#include "TaskDlgEditSketch3D.h"
#include "TaskSketcher3DTool.h"
#include "Utils.h"
#include "ViewProviderSketch3D.h"

using namespace Sketcher3DGui;

PROPERTY_SOURCE(Sketcher3DGui::ViewProviderSketch3D, PartGui::ViewProviderPart)

namespace
{
inline QStringList editModeToolbarNames()
{
    return {QStringLiteral("Sketcher3D Edit")};
}

inline QStringList nonEditModeToolbarNames()
{
    return {
        QStringLiteral("Structure"),
        QStringLiteral("Sketcher"),
    };
}

constexpr const char* kEditingWorkbench = "SketcherWorkbench";

void setDiffuseColor(SoMaterial* material, const Color3f& color)
{
    material->diffuseColor.setValue(color[0], color[1], color[2]);
}

void setEmissiveColor(SoMaterial* material, const Color3f& color, float scale = 1.0F)
{
    material->emissiveColor.setValue(color[0] * scale, color[1] * scale, color[2] * scale);
}

SoSeparator* addAxisArrow(
    const SbVec3f& dir,
    const SbVec3f& side,
    const char* name,
    const Color3f& color,
    float emScale
)
{
    auto* axisSep = new SoSeparator();
    axisSep->setName(name);

    auto* material = new SoMaterial();
    setDiffuseColor(material, color);
    setEmissiveColor(material, color, emScale);
    material->transparency.setValue(kAxisHandleTransparency);
    axisSep->addChild(material);

    auto* style = new SoDrawStyle();
    style->lineWidth.setValue(kAxisHandleLineWidth);
    axisSep->addChild(style);

    SbVec3f end = dir * kAxisHandleLength;
    SbVec3f arrowBase = end - dir * kAxisHandleArrowLength;
    SbVec3f sideVec = side * (kAxisHandleArrowLength * kArrowHeadHalfWidthFactor);

    auto* coords = new SoCoordinate3();
    coords->point.setNum(4);
    coords->point.set1Value(0, SbVec3f(0.0F, 0.0F, 0.0F));
    coords->point.set1Value(1, end);
    coords->point.set1Value(2, arrowBase + sideVec);
    coords->point.set1Value(3, arrowBase - sideVec);
    axisSep->addChild(coords);

    auto* lineSet = new SoIndexedLineSet();
    static const int32_t lineIndices[] = {0, 1, -1, 1, 2, -1, 1, 3, -1};
    lineSet->coordIndex.setValues(0, 9, lineIndices);
    axisSep->addChild(lineSet);

    return axisSep;
}

float planeSizeFromFrame(const ViewProviderSketch3D::ActivePlaneFrame& frame, const Base::Vector3d& cursor)
{
    Base::Vector3d d = cursor - frame.origin;
    double alongU = std::abs(d * frame.xAxis);
    double alongV = std::abs(d * frame.yAxis);
    return std::max(kAxisHandleLength, static_cast<float>(std::max(alongU, alongV)));
}

}  // namespace

ViewProviderSketch3D::ViewProviderSketch3D()
{
    PointSize.setValue(5.0F);
    LineWidth.setValue(2.0F);
    updateActivePlaneFrame();
}

ViewProviderSketch3D::~ViewProviderSketch3D()
{
    if (referenceGeometryRoot) {
        referenceGeometryRoot->unref();
    }
}

bool ViewProviderSketch3D::isEditingSketch3D() const
{
    return getActiveSketch3DVP() == this;
}

void ViewProviderSketch3D::ensureReferenceGeometry()
{
    if (referenceGeometryRoot) {
        return;
    }

    referenceGeometryRoot = new SoSeparator();
    referenceGeometryRoot->setName("Sketcher3DReferenceGeometry");
    referenceGeometryRoot->ref();
    referenceGeometryRoot->renderCaching.setValue(SoSeparator::OFF);

    referenceGeometrySwitch = new SoSwitch();
    referenceGeometrySwitch->whichChild = SO_SWITCH_NONE;
    referenceGeometryRoot->addChild(referenceGeometrySwitch);

    referenceGeometryContent = new SoSeparator();

    referenceFaceCoords = new SoCoordinate3();
    referenceGeometryContent->addChild(referenceFaceCoords);

    referenceFaceNormals = new SoNormal();
    referenceGeometryContent->addChild(referenceFaceNormals);

    referenceFaceMaterial = new SoMaterial();
    setDiffuseColor(referenceFaceMaterial, kPlaneOverlayColor);
    referenceFaceMaterial->transparency.setValue(kPlaneTransparency);
    referenceGeometryContent->addChild(referenceFaceMaterial);

    auto* faceDrawStyle = new SoDrawStyle();
    faceDrawStyle->style.setValue(SoDrawStyle::FILLED);
    referenceGeometryContent->addChild(faceDrawStyle);

    auto* facePick = new SoPickStyle();
    facePick->style.setValue(SoPickStyle::SHAPE);
    referenceGeometryContent->addChild(facePick);

    referenceFaceset = new PartGui::SoBrepFaceSet();
    referenceGeometryContent->addChild(referenceFaceset);

    referenceCoords = new SoCoordinate3();
    referenceGeometryContent->addChild(referenceCoords);

    referenceNormals = new SoNormal();
    referenceGeometryContent->addChild(referenceNormals);

    auto* edgeMaterial = new SoMaterial();
    setDiffuseColor(edgeMaterial, kReferenceColor);
    referenceGeometryContent->addChild(edgeMaterial);

    auto* edgeStyle = new SoDrawStyle();
    edgeStyle->style.setValue(SoDrawStyle::LINES);
    edgeStyle->lineWidth.setValue(LineWidth.getValue());
    edgeStyle->pointSize.setValue(PointSize.getValue());
    referenceGeometryContent->addChild(edgeStyle);

    referenceLineset = new PartGui::SoBrepEdgeSet();
    referenceGeometryContent->addChild(referenceLineset);

    referencePointset = new PartGui::SoBrepPointSet();
    referenceGeometryContent->addChild(referencePointset);

    referenceGeometrySwitch->addChild(referenceGeometryContent);
    pcRoot->addChild(referenceGeometryRoot);
}

void ViewProviderSketch3D::updateReferenceGeometry()
{
    ensureReferenceGeometry();
    auto* sketch = getSketch3DObject();

    if (!isEditingSketch3D() || !sketch) {
        referenceGeometrySwitch->whichChild = SO_SWITCH_NONE;
        return;
    }

    updateReferencePlanes();

    const auto& shape = sketch->ReferenceShape.getShape();
    if (!lastRenderedRefShape.getShape().IsPartner(shape.getShape())) {
        lastRenderedRefShape = shape;

        Gui::SoSelectionElementAction saction(Gui::SoSelectionElementAction::None);
        saction.apply(referenceLineset);
        saction.apply(referencePointset);

        auto* scratchFaceset = new PartGui::SoBrepFaceSet();
        scratchFaceset->ref();
        if (!shape.isNull()) {
            PartGui::ViewProviderPartExt::setupCoinGeometry(
                shape.getShape(),
                referenceCoords,
                scratchFaceset,
                referenceNormals,
                referenceLineset,
                referencePointset,
                Deviation.getValue(),
                AngularDeflection.getValue()
            );
        }
        else {
            referenceCoords->point.setNum(0);
            referenceLineset->coordIndex.setNum(0);
            referencePointset->startIndex.setValue(0);
        }
        scratchFaceset->unref();
    }

    referenceGeometrySwitch->whichChild = SO_SWITCH_ALL;
}

void ViewProviderSketch3D::updateReferencePlanes()
{
    planeGeoIds.clear();

    auto* sketch = getSketch3DObject();
    if (!sketch) {
        return;
    }

    const auto& geos = sketch->Geometry.getValues();
    std::vector<Part::TopoShape> faceShapes;
    for (int i = 0; i < static_cast<int>(geos.size()); ++i) {
        if (!geos[i] || !geos[i]->is<Sketcher3D::GeomReferencePlane3D>()) {
            continue;
        }
        TopoDS_Shape face = geos[i]->toShape();
        if (face.IsNull()) {
            continue;
        }
        faceShapes.emplace_back(face);
        planeGeoIds.push_back(i);
    }

    Part::TopoShape faceCompound;
    faceCompound.makeElementCompound(faceShapes);

    referenceFaceCoords->point.setNum(0);
    referenceFaceNormals->vector.setNum(0);
    referenceFaceset->coordIndex.setNum(0);
    referenceFaceset->partIndex.setNum(0);

    // Face borders are discarded.
    auto* tempLineset = new PartGui::SoBrepEdgeSet();
    tempLineset->ref();
    auto* tempPointset = new PartGui::SoBrepPointSet();
    tempPointset->ref();
    PartGui::ViewProviderPartExt::setupCoinGeometry(
        faceCompound.getShape(),
        referenceFaceCoords,
        referenceFaceset,
        referenceFaceNormals,
        tempLineset,
        tempPointset,
        Deviation.getValue(),
        AngularDeflection.getValue()
    );
    tempLineset->unref();
    tempPointset->unref();
}

bool ViewProviderSketch3D::getElementPicked(const SoPickedPoint* pp, std::string& subname) const
{
    const SoDetail* detail = pp->getDetail();
    if (!detail) {
        return false;
    }

    auto hit = [&](SoNode* node, SoType detailType) {
        return node && pp->getPath()->containsNode(node) && detail->getTypeId() == detailType;
    };

    const char* element = nullptr;
    // this is beause the referance plane is only view and follow part 1 based index
    // and other element is follow 0 based index
    int number = -1;

    if (hit(referenceFaceset, SoFaceDetail::getClassTypeId())) {
        element = "Face";
        int partIdx = static_cast<const SoFaceDetail*>(detail)->getPartIndex();
        if (partIdx >= 0 && partIdx < static_cast<int>(planeGeoIds.size())) {
            number = planeGeoIds[partIdx];
        }
    }
    else if (hit(referenceLineset, SoLineDetail::getClassTypeId())) {
        element = "Edge";
        number = static_cast<const SoLineDetail*>(detail)->getLineIndex();
    }
    else if (hit(referencePointset, SoPointDetail::getClassTypeId())) {
        element = "Vertex";
        number = static_cast<const SoPointDetail*>(detail)->getCoordinateIndex()
            - referencePointset->startIndex.getValue();
    }
    else {
        return PartGui::ViewProviderPart::getElementPicked(pp, subname);
    }

    if (number < 0) {
        return false;
    }
    subname = Sketcher3D::Sketch3DObject::referencePrefix() + element + std::to_string(number + 1);
    return true;
}

bool ViewProviderSketch3D::getDetailPath(
    const char* subname,
    SoFullPath* pPath,
    bool append,
    SoDetail*& det
) const
{
    const auto& prefix = Sketcher3D::Sketch3DObject::referencePrefix();
    if (!subname || !std::string_view(subname).starts_with(prefix)) {
        return PartGui::ViewProviderPart::getDetailPath(subname, pPath, append, det);
    }

    auto [element, occIndex] = Part::TopoShape::getElementTypeAndIndex(subname + prefix.size());
    int index = static_cast<int>(occIndex) - 1;

    SoNode* setNode = nullptr;
    std::unique_ptr<SoDetail> detail;

    if (index >= 0 && element == "Edge" && referenceLineset) {
        auto* lineDetail = new SoLineDetail();
        lineDetail->setLineIndex(index);
        detail.reset(lineDetail);
        setNode = referenceLineset;
    }
    else if (index >= 0 && element == "Vertex" && referencePointset) {
        auto* pointDetail = new SoPointDetail();
        pointDetail->setCoordinateIndex(index + referencePointset->startIndex.getValue());
        detail.reset(pointDetail);
        setNode = referencePointset;
    }
    else if (index >= 0 && element == "Face" && referenceFaceset) {
        auto it = std::find(planeGeoIds.begin(), planeGeoIds.end(), index);
        if (it != planeGeoIds.end()) {
            auto* faceDetail = new SoFaceDetail();
            faceDetail->setPartIndex(static_cast<int>(std::distance(planeGeoIds.begin(), it)));
            detail.reset(faceDetail);
            setNode = referenceFaceset;
        }
    }

    if (!detail) {
        return false;
    }

    if (append && pPath) {
        pPath->append(pcRoot);
        pPath->append(referenceGeometryRoot);
        pPath->append(referenceGeometrySwitch);
        pPath->append(referenceGeometryContent);
        pPath->append(setNode);
    }
    det = detail.release();
    return true;
}

Sketcher3D::Sketch3DObject* ViewProviderSketch3D::getSketch3DObject() const
{
    return static_cast<Sketcher3D::Sketch3DObject*>(getObject());
}

void ViewProviderSketch3D::updateData(const App::Property* prop)
{
    PartGui::ViewProviderPart::updateData(prop);
    auto* sketch = getSketch3DObject();
    if (!taskPanel || !sketch) {
        return;
    }
    if (prop == &sketch->ReferenceShape) {
        updateReferenceGeometry();
    }
    if (prop == &sketch->Geometry || prop == &sketch->Constraints) {
        if (activeUserPlaneGeoId >= 0 && !getActiveReferencePlane()) {
            activeUserPlaneGeoId = -1;
        }
        if (prop == &sketch->Geometry) {
            updateReferenceGeometry();
        }
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    menu->addAction(tr("Edit 3D Sketch"), receiver, member);
    Gui::ViewProvider::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSketch3D::setEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        return PartGui::ViewProviderPart::setEdit(ModNum);
    }

    if (Gui::Control().activeDialog()) {
        Base::Console().warning(
            "Sketcher3D: cannot enter edit mode while another task dialog is open.\n"
        );
        return false;
    }

    Gui::Selection().clearSelection();
    Gui::Selection().rmvPreselect();

    oldWb = Gui::Command::assureWorkbench(kEditingWorkbench);

    // save non edit toolbar state and switch to edit mode toolbars
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::SaveState
    );
    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::ForceAvailable
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::ForceHidden
    );

    ensurePlaneOverlay();
    ensureSnapMarker();
    ensureReferenceGeometry();
    updateReferenceGeometry();
    snapManager = std::make_unique<SnapManager3D>(*this);

    auto* dlg = new TaskDlgEditSketch3D(this);
    Gui::Control().showDialog(dlg);

    return true;
}

void ViewProviderSketch3D::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    PartGui::ViewProviderPart::setEditViewer(viewer, ModNum);
    updateReferenceGeometry();
}

void ViewProviderSketch3D::unsetEdit(int ModNum)
{
    if (ModNum != ViewProviderSketch3D::Default) {
        PartGui::ViewProviderPart::unsetEdit(ModNum);
        return;
    }

    purgeHandler();
    snapManager.reset();
    lastRenderedRefShape = Part::TopoShape();
    updateReferenceGeometry();

    if (snapMarker) {
        pcRoot->removeChild(snapMarker);
        snapMarker->unref();
        snapMarker = nullptr;
        snapMarkerSwitch = nullptr;
        snapMarkerXf = nullptr;
    }

    if (planeOverlay) {
        pcRoot->removeChild(planeOverlay);
        planeOverlay->unref();
        planeOverlay = nullptr;
        planeOverlayTransform = nullptr;
        planeOverlayScale = nullptr;
        planeOverlaySize = kPlaneOverlaySize;
    }

    Gui::ToolBarManager::getInstance()->setState(
        editModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );
    Gui::ToolBarManager::getInstance()->setState(
        nonEditModeToolbarNames(),
        Gui::ToolBarManager::State::RestoreDefault
    );

    Gui::Control().closeDialog();

    // restore the workbench that was active before setEdit.
    if (!oldWb.empty()) {
        Gui::Command::assureWorkbench(oldWb.c_str());
        oldWb.clear();
    }
}

void ViewProviderSketch3D::activateHandler(std::unique_ptr<DrawSketchHandler3D> h)
{
    purgeHandler();
    handler = std::move(h);
    if (handler) {
        handler->activate(this);
    }
}

void ViewProviderSketch3D::purgeHandler()
{
    if (handler) {
        handler->quit();
        handler.reset();
    }
}

bool ViewProviderSketch3D::tryActivatePickedPlane(const std::string& subName)
{
    auto* sketch = getSketch3DObject();
    if (!sketch || subName.empty()) {
        return false;
    }

    Sketcher3D::GeoElementId3D id = sketch->resolveSubName(subName);
    if (!id.isValid()) {
        return false;
    }

    if (!sketch->getGeometry<Sketcher3D::GeomReferencePlane3D>(id.GeoId)) {
        return false;
    }

    setActiveUserPlane(id.GeoId);
    return true;
}

bool ViewProviderSketch3D::mouseButtonPressed(
    int button,
    bool pressed,
    const SbVec2s& cursorPos,
    const Gui::View3DInventorViewer* viewer
)
{
    if (button != SoMouseButtonEvent::BUTTON1 || !pressed) {
        return false;
    }

    // try reference plane picking
    if (!handler) {
        std::unique_ptr<SoPickedPoint> pp(getPointOnRay(cursorPos, viewer));
        if (pp) {
            std::string subName;
            if (getElementPicked(pp.get(), subName)) {
                return tryActivatePickedPlane(subName);
            }
        }
        return false;
    }

    Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
    Base::Vector3d p = applySnap(raw, cursorPos, viewer);
    hideSnapMarker();
    return handler->pressButton(p);
}

bool ViewProviderSketch3D::mouseMove(const SbVec2s& cursorPos, Gui::View3DInventorViewer* viewer)
{
    if (!handler) {
        return false;
    }
    Base::Vector3d raw = projectToSketchPlane(cursorPos, viewer);
    updatePlaneOverlaySize(raw);
    Base::Vector3d p = applySnap(raw, cursorPos, viewer);
    if (snapTarget.isValid()) {
        showSnapMarker(p);
    }
    else {
        hideSnapMarker();
    }
    return handler->mouseMove(p);
}

bool ViewProviderSketch3D::keyPressed(bool pressed, int key)
{
    if (key == SoKeyboardEvent::ESCAPE) {
        if (handler) {
            if (!pressed) {
                handler->keyPressed(key);
            }
            return true;
        }
        return false;
    }

    if (!pressed) {
        return false;
    }
    if (key == SoKeyboardEvent::TAB) {
        cyclePlane();
        return true;
    }
    if (!handler) {
        return false;
    }
    return handler->keyPressed(key);
}

const Sketcher3D::GeomReferencePlane3D* ViewProviderSketch3D::getActiveReferencePlane() const
{
    if (activeUserPlaneGeoId < 0) {
        return nullptr;
    }
    auto* sketch = getSketch3DObject();
    if (!sketch) {
        return nullptr;
    }
    return sketch->getGeometry<Sketcher3D::GeomReferencePlane3D>(activeUserPlaneGeoId);
}

void ViewProviderSketch3D::cyclePlane()
{
    setActiveBasePlane(static_cast<ActivePlane>((static_cast<int>(activeBasePlane) + 1) % 3));
}

void ViewProviderSketch3D::setActiveBasePlane(ActivePlane p)
{
    activeBasePlane = p;
    activeUserPlaneGeoId = -1;
    applyActivePlaneChanges();
}

void ViewProviderSketch3D::setActiveUserPlane(int geoId)
{
    activeUserPlaneGeoId = geoId;
    applyActivePlaneChanges();
}

void ViewProviderSketch3D::applyActivePlaneChanges()
{
    updateActivePlaneFrame();
    updatePlaneScale();
    updatePlaneOverlayTransform();
    // TODO: need to add plane in task panel
    if (taskPanel) {
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::updateActivePlaneFrame()
{
    activeFrame.isUserPlane = false;
    activeFrame.geoId = -1;

    if (auto* plane = getActiveReferencePlane()) {
        activeFrame.isUserPlane = true;
        activeFrame.geoId = activeUserPlaneGeoId;
        activeFrame.origin = plane->getLocation();
        activeFrame.xAxis = plane->getXDir();
        activeFrame.normal = plane->getDir();
        activeFrame.yAxis = activeFrame.normal.Cross(activeFrame.xAxis);
        return;
    }

    activeFrame.origin = planeBase;
    switch (activeBasePlane) {
        case ActivePlane::XY:
            activeFrame.xAxis = Base::Vector3d(1, 0, 0);
            activeFrame.yAxis = Base::Vector3d(0, 1, 0);
            activeFrame.normal = Base::Vector3d(0, 0, 1);
            break;
        case ActivePlane::YZ:
            activeFrame.xAxis = Base::Vector3d(0, 1, 0);
            activeFrame.yAxis = Base::Vector3d(0, 0, 1);
            activeFrame.normal = Base::Vector3d(1, 0, 0);
            break;
        case ActivePlane::ZX:
            activeFrame.xAxis = Base::Vector3d(0, 0, 1);
            activeFrame.yAxis = Base::Vector3d(1, 0, 0);
            activeFrame.normal = Base::Vector3d(0, 1, 0);
            break;
    }
}

void ViewProviderSketch3D::updatePlaneOverlayTransform()
{
    if (!planeOverlayTransform) {
        return;
    }
    auto& frame = getActivePlaneFrame();

    planeOverlayTransform->translation.setValue(frame.origin.x, frame.origin.y, frame.origin.z);

    auto rot
        = Base::Rotation::makeRotationByAxes(frame.xAxis, Base::Vector3d(0, 0, 0), frame.normal, "ZXY");
    planeOverlayTransform->rotation.setValue(Base::convertTo<SbRotation>(rot));
}

void ViewProviderSketch3D::setPlaneBase(const Base::Vector3d& base)
{
    if (activeFrame.isUserPlane) {
        return;
    }
    planeBase = base;
    updateActivePlaneFrame();
    updatePlaneOverlayTransform();
    if (taskPanel) {
        taskPanel->refresh();
    }
}

void ViewProviderSketch3D::ensurePlaneOverlay()
{
    if (planeOverlay) {
        updatePlaneOverlay();
        return;
    }

    planeOverlay = new SoSeparator();
    planeOverlay->setName("Sketcher3DPlaneOverlay");
    planeOverlay->ref();
    planeOverlay->renderCaching = SoSeparator::OFF;

    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    planeOverlay->addChild(pick);

    planeOverlayTransform = new SoTransform();
    planeOverlay->addChild(planeOverlayTransform);

    auto* quadSep = new SoSeparator();
    quadSep->setName("Sketcher3DPlaneQuad");

    auto* material = new SoMaterial();
    setDiffuseColor(material, kPlaneOverlayColor);
    material->transparency.setValue(kPlaneTransparency);
    quadSep->addChild(material);

    auto* style = new SoDrawStyle();
    style->style.setValue(SoDrawStyle::FILLED);
    quadSep->addChild(style);

    planeOverlayScale = new SoScale();
    quadSep->addChild(planeOverlayScale);

    auto* coords = new SoCoordinate3();
    coords->point.setNum(kPlaneQuadVertexCount);
    SbVec3f* pts = coords->point.startEditing();
    pts[0] = {-1.0F, -1.0F, 0.0F};
    pts[1] = {1.0F, -1.0F, 0.0F};
    pts[2] = {1.0F, 1.0F, 0.0F};
    pts[3] = {-1.0F, 1.0F, 0.0F};
    coords->point.finishEditing();
    quadSep->addChild(coords);

    auto* face = new SoFaceSet();
    face->numVertices.setNum(1);
    face->numVertices.set1Value(0, kPlaneQuadVertexCount);
    quadSep->addChild(face);

    planeOverlay->addChild(quadSep);

    auto* handleSep = new SoSeparator();
    handleSep->setName("Sketcher3DAxisHandleAutoZoom");
    handleSep->addChild(new Gui::SoAutoZoomTranslation());

    auto* handle = new SoSeparator();
    handle->setName("Sketcher3DAxisHandle");
    handle->addChild(addAxisArrow(
        {1.0F, 0.0F, 0.0F},
        {0.0F, 1.0F, 0.0F},
        "Sketcher3DAxisHandleX",
        kActiveAxisHandleColor,
        kActiveAxisHandleEmissiveScale
    ));
    handle->addChild(addAxisArrow(
        {0.0F, 1.0F, 0.0F},
        {1.0F, 0.0F, 0.0F},
        "Sketcher3DAxisHandleY",
        kActiveAxisHandleColor,
        kActiveAxisHandleEmissiveScale
    ));
    handle->addChild(addAxisArrow(
        {0.0F, 0.0F, 1.0F},
        {1.0F, 0.0F, 0.0F},
        "Sketcher3DAxisHandleZ",
        kInactiveAxisHandleColor,
        kInactiveAxisHandleEmissiveScale
    ));
    handleSep->addChild(handle);

    planeOverlay->addChild(handleSep);

    pcRoot->addChild(planeOverlay);
    updatePlaneOverlay();
}

void ViewProviderSketch3D::updatePlaneOverlay()
{
    if (!planeOverlay) {
        return;
    }
    updateActivePlaneFrame();
    updatePlaneScale();
    updatePlaneOverlayTransform();
}

void ViewProviderSketch3D::updatePlaneScale()
{
    if (!planeOverlayScale) {
        return;
    }
    planeOverlayScale->scaleFactor.setValue(planeOverlaySize, planeOverlaySize, 1.0F);
}

void ViewProviderSketch3D::updatePlaneOverlaySize(const Base::Vector3d& cursorPos)
{
    float updatedSize = planeSizeFromFrame(getActivePlaneFrame(), cursorPos);
    if (std::abs(planeOverlaySize - updatedSize) <= Precision::Confusion()) {
        return;
    }

    planeOverlaySize = updatedSize;
    updatePlaneScale();
}

Base::Vector3d ViewProviderSketch3D::projectToSketchPlane(
    const SbVec2s& cursorPx,
    const Gui::View3DInventorViewer* viewer
) const
{
    SbVec3f rayStart;
    SbVec3f rayEnd;
    viewer->projectPointToLine(cursorPx, rayStart, rayEnd);

    SbPlane plane(
        Base::convertTo<SbVec3f>(getActivePlaneFrame().normal),
        Base::convertTo<SbVec3f>(getActivePlaneFrame().origin)
    );

    SbVec3f hit;
    if (!plane.intersect(SbLine(rayStart, rayEnd), hit)) {
        SbVec3f fp = viewer->getPointOnFocalPlane(cursorPx);
        return Base::Vector3d(fp[0], fp[1], fp[2]);
    }
    return Base::Vector3d(hit[0], hit[1], hit[2]);
}

Base::Vector3d ViewProviderSketch3D::applySnap(
    const Base::Vector3d& raw,
    const SbVec2s& cursorPos,
    const Gui::View3DInventorViewer* viewer
)
{
    if (!snapManager) {
        snapTarget = {};
        return raw;
    }
    std::string pickedSubName = getPickedSubName(cursorPos, viewer);
    return snapManager->snap(raw, pickedSubName, snapTarget);
}

std::string ViewProviderSketch3D::getPickedSubName(
    const SbVec2s& cursorPx,
    const Gui::View3DInventorViewer* viewer
) const
{
    if (!viewer) {
        return {};
    }

    std::unique_ptr<SoPickedPoint> point(getPointOnRay(cursorPx, viewer));
    if (!point) {
        return {};
    }

    std::string subName;
    if (!getElementPicked(point.get(), subName)) {
        return {};
    }
    return subName;
}

void ViewProviderSketch3D::ensureSnapMarker()
{
    if (snapMarker) {
        return;
    }
    snapMarker = new SoSeparator();
    snapMarker->setName("Sketcher3DSnapMarker");
    snapMarker->ref();
    snapMarker->renderCaching = SoSeparator::OFF;

    snapMarkerSwitch = new SoSwitch();
    snapMarkerSwitch->whichChild = SO_SWITCH_NONE;
    snapMarker->addChild(snapMarkerSwitch);

    auto* content = new SoSeparator();

    auto* pick = new SoPickStyle();
    pick->style.setValue(SoPickStyle::UNPICKABLE);
    content->addChild(pick);

    auto* material = new SoMaterial();
    setDiffuseColor(material, kSnapMarkerDiffuseColor);
    setEmissiveColor(material, kSnapMarkerEmissiveColor);
    content->addChild(material);

    snapMarkerXf = new SoTranslation();
    content->addChild(snapMarkerXf);

    auto* sphere = new SoSphere();
    sphere->radius.setValue(kSnapMarkerRadius);
    content->addChild(sphere);

    snapMarkerSwitch->addChild(content);

    pcRoot->addChild(snapMarker);
}

void ViewProviderSketch3D::hideSnapMarker()
{
    if (snapMarkerSwitch) {
        snapMarkerSwitch->whichChild = SO_SWITCH_NONE;
    }
}

void ViewProviderSketch3D::showSnapMarker(const Base::Vector3d& pos)
{
    if (!snapMarkerSwitch || !snapMarkerXf) {
        return;
    }
    snapMarkerXf->translation.setValue(pos.x, pos.y, pos.z);
    snapMarkerSwitch->whichChild = 0;
}
