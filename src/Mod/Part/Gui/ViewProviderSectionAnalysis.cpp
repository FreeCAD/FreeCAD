// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Gregg Jaskiewicz
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include <cstring>

#include <QAction>
#include <QMenu>

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <Inventor/actions/SoSearchAction.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoTextureCoordinatePlane.h>

#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/Material.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>

#include "SoFCStencilCap.h"
#include "ViewProviderExt.h"
#include "ViewProviderSectionAnalysis.h"
#include "TaskSectionAnalysis.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderSectionAnalysis, PartGui::ViewProviderPart)

ViewProviderSectionAnalysis::ViewProviderSectionAnalysis()
{
    sPixmap = "Part_SectionAnalysis";

    // Default section face color: reddish-orange (like Fusion 360 hatching)
    App::Material mat;
    mat.diffuseColor.set(0.8f, 0.3f, 0.2f, 0.0f);
    ShapeAppearance.setValues({mat});
}

ViewProviderSectionAnalysis::~ViewProviderSectionAnalysis()
{
    removeClipPlane();
    if (pcClipPlane) {
        pcClipPlane->unref();
        pcClipPlane = nullptr;
    }
    if (pcHatchTexture) {
        pcHatchTexture->unref();
        pcHatchTexture = nullptr;
    }
    if (pcHatchCoordGen) {
        pcHatchCoordGen->unref();
        pcHatchCoordGen = nullptr;
    }
}

void ViewProviderSectionAnalysis::attach(App::DocumentObject* pcFeat)
{
    ViewProviderPart::attach(pcFeat);

    // Create the translucent cutting plane visual
    pcPlaneRoot = new SoSeparator();
    pcPlaneRoot->setName("SectionPlaneVisual");

    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    pcPlaneRoot->addChild(pickStyle);

    pcPlaneHints = new SoShapeHints();
    pcPlaneHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    pcPlaneHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    pcPlaneRoot->addChild(pcPlaneHints);

    pcPlaneMaterial = new SoMaterial();
    pcPlaneMaterial->diffuseColor.setValue(0.3f, 0.6f, 0.9f);
    pcPlaneMaterial->transparency.setValue(0.7f);
    pcPlaneRoot->addChild(pcPlaneMaterial);

    pcPlaneCoords = new SoCoordinate3();
    pcPlaneRoot->addChild(pcPlaneCoords);

    pcPlaneFaceSet = new SoFaceSet();
    pcPlaneFaceSet->numVertices.set1Value(0, 4);
    pcPlaneRoot->addChild(pcPlaneFaceSet);

    pcPlaneBorderMaterial = new SoMaterial();
    pcPlaneBorderMaterial->diffuseColor.setValue(0.2f, 0.4f, 0.8f);
    pcPlaneBorderMaterial->transparency.setValue(0.0f);
    pcPlaneRoot->addChild(pcPlaneBorderMaterial);

    auto* borderStyle = new SoDrawStyle();
    borderStyle->lineWidth.setValue(2.0f);
    pcPlaneRoot->addChild(borderStyle);

    pcPlaneBorderLines = new SoIndexedLineSet();
    pcPlaneRoot->addChild(pcPlaneBorderLines);

    // Wrap plane in a switch so we can hide it
    pcPlaneSwitch = new SoSwitch();
    pcPlaneSwitch->addChild(pcPlaneRoot);
    pcPlaneSwitch->whichChild = SO_SWITCH_ALL;
    pcRoot->addChild(pcPlaneSwitch);

    // Create hatching texture — 45° diagonal lines per ISO 128-50.
    // Large texture + binary alpha for crisp lines.
    pcHatchTexture = new SoTexture2();
    pcHatchTexture->ref();
    {
        const int sz = 256;
        const int spacing = 64;
        const int lineWidth = 2;
        // 45° diagonal lines in MODULATE mode (RGB, no alpha).
        // White background preserves surface color, dark lines darken it.
        unsigned char* img = new unsigned char[sz * sz * 3];
        std::memset(img, 255, sz * sz * 3);
        for (int y = 0; y < sz; y++) {
            for (int x = 0; x < sz; x++) {
                int idx = (y * sz + x) * 3;
                if (((x + y) % spacing) < lineWidth) {
                    img[idx] = 25;
                    img[idx + 1] = 25;
                    img[idx + 2] = 25;
                }
            }
        }
        pcHatchTexture->image.setValue(SbVec2s(sz, sz), 3, img);
        pcHatchTexture->wrapS = SoTexture2::REPEAT;
        pcHatchTexture->wrapT = SoTexture2::REPEAT;
        pcHatchTexture->model = SoTexture2::MODULATE;
        delete[] img;
    }

    // Auto-generate texture coordinates by projecting onto the cutting plane.
    // directionS/T are updated in updateHatchProjection() to match the
    // current normal so the 45° pattern is always correct.
    pcHatchCoordGen = new SoTextureCoordinatePlane();
    pcHatchCoordGen->ref();
    updateHatchProjection();



    // Create the clip plane node (not yet inserted into source VP)
    pcClipPlane = new SoClipPlane();
    pcClipPlane->ref();

    // GPU stencil-buffer capping node — renders filled cross-section
    // without OCCT computation.
    pcStencilCap = new SoFCStencilCap();
    pcRoot->addChild(pcStencilCap);

    updatePlaneVisual();
}

void ViewProviderSectionAnalysis::finishRestoring()
{
    ViewProviderPart::finishRestoring();

    // After document restore, shapes are computed and the scene graph is
    // fully built.  Safe to set up clip planes, plane visual, and hatching.
    if (Visibility.getValue()) {
        installClipPlane();
    }
    updatePlaneVisual();
    updateHatchProjection();
    updateStencilCap();
    if (hatchEnabled) {
        setHatching(true);
    }
}

void ViewProviderSectionAnalysis::installClipPlane()
{
    removeClipPlane();

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    updateClipPlaneEquation();

    // Install the clip plane on ALL visible objects in the document,
    // except the SectionAnalysis itself.  Coin3D supports shared nodes
    // (multi-parent), so one pcClipPlane with one equation update
    // affects every VP it is inserted into.
    auto* doc = feat->getDocument();
    for (auto* obj : doc->getObjects()) {
        if (obj->isDerivedFrom(Part::SectionAnalysis::getClassTypeId())) {
            continue;
        }
        auto* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp || !vp->isVisible()) {
            continue;
        }
        auto* root = dynamic_cast<SoSeparator*>(vp->getRoot());
        if (!root) {
            continue;
        }
        root->insertChild(pcClipPlane, 0);
        clippedObjects.push_back(obj);
    }
    clipInstalled = !clippedObjects.empty();
}

void ViewProviderSectionAnalysis::removeClipPlane()
{
    if (!pcClipPlane) {
        return;
    }

    for (auto* obj : clippedObjects) {
        auto* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;
        }
        auto* root = dynamic_cast<SoSeparator*>(vp->getRoot());
        if (!root) {
            continue;
        }
        int idx = root->findChild(pcClipPlane);
        if (idx >= 0) {
            root->removeChild(idx);
        }
    }
    clippedObjects.clear();
    clipInstalled = false;
}

void ViewProviderSectionAnalysis::updateClipPlaneEquation()
{
    if (!pcClipPlane) {
        return;
    }

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    Base::Vector3d n = feat->PlaneNormal.getValue();
    double d = feat->PlaneOffset.getValue();
    bool flip = feat->FlipCut.getValue();

    double len = n.Length();
    if (len < 1e-10) {
        return;
    }
    n = n / len;

    if (flip) {
        n = -n;
        d = -d;
    }

    // The cutting plane passes through point (n * d) with normal n.
    // OCCT keeps the negative-normal side (where n.p < d).
    // SoClipPlane keeps the positive-normal half-space.
    // So we negate the normal to keep the same side as OCCT.
    //
    // Offset the clip plane by a tiny epsilon toward the remaining solid
    // so it clips slightly past the section face.  This prevents z-fighting
    // between the source body's surface at the clip boundary and the
    // section face (which lies exactly on the cutting plane).
    constexpr double clipEps = 0.01;  // 10 microns
    double dClip = d - clipEps;
    SbVec3f planePoint(n.x * dClip, n.y * dClip, n.z * dClip);
    SbVec3f clipNormal(-n.x, -n.y, -n.z);
    pcClipPlane->plane.setValue(SbPlane(clipNormal, planePoint));
    pcClipPlane->on.setValue(TRUE);
}

void ViewProviderSectionAnalysis::updatePlaneVisual()
{
    if (!pcPlaneCoords || !pcPlaneFaceSet) {
        return;
    }

    // Default to hidden — will be enabled if we successfully compute coordinates
    pcPlaneFaceSet->numVertices.set1Value(0, 0);
    pcPlaneBorderLines->coordIndex.setNum(0);  // clear border indices too

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    App::DocumentObject* source = feat->Source.getValue();
    if (!source) {
        return;
    }

    TopoDS_Shape sourceShape = Part::Feature::getShape(
        source,
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    if (sourceShape.IsNull()) {
        return;
    }

    Bnd_Box bbox;
    BRepBndLib::Add(sourceShape, bbox);
    if (bbox.IsVoid()) {
        return;
    }

    double xmin, ymin, zmin, xmax, ymax, zmax;
    bbox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    Base::Vector3d n = feat->PlaneNormal.getValue();
    double d = feat->PlaneOffset.getValue();
    double len = n.Length();
    if (len < 1e-10) {
        return;
    }
    n = n / len;

    // Build orthonormal frame on the cutting plane
    Base::Vector3d u, v;
    if (std::abs(n.x) < 0.9) {
        u = Base::Vector3d(1, 0, 0).Cross(n);
    }
    else {
        u = Base::Vector3d(0, 1, 0).Cross(n);
    }
    u.Normalize();
    v = n.Cross(u);
    v.Normalize();

    // Project bbox corners onto plane tangent axes
    double umin_p = 1e20, umax_p = -1e20, vmin_p = 1e20, vmax_p = -1e20;
    double corners[8][3] = {
        {xmin, ymin, zmin},
        {xmax, ymin, zmin},
        {xmin, ymax, zmin},
        {xmax, ymax, zmin},
        {xmin, ymin, zmax},
        {xmax, ymin, zmax},
        {xmin, ymax, zmax},
        {xmax, ymax, zmax}
    };
    for (auto& c : corners) {
        Base::Vector3d pt(c[0], c[1], c[2]);
        umin_p = std::min(umin_p, pt * u);
        umax_p = std::max(umax_p, pt * u);
        vmin_p = std::min(vmin_p, pt * v);
        vmax_p = std::max(vmax_p, pt * v);
    }

    // Cap the plane size to bbox diagonal to prevent blowup at steep angles
    double bboxDiag = std::sqrt(
        (xmax - xmin) * (xmax - xmin) + (ymax - ymin) * (ymax - ymin) + (zmax - zmin) * (zmax - zmin)
    );
    double maxExtent = bboxDiag * 0.7;
    double umid = (umin_p + umax_p) / 2.0;
    double vmid = (vmin_p + vmax_p) / 2.0;
    double uHalf = std::min((umax_p - umin_p) / 2.0, maxExtent);
    double vHalf = std::min((vmax_p - vmin_p) / 2.0, maxExtent);

    // Add 15% margin
    uHalf *= 1.15;
    vHalf *= 1.15;

    // Center point on the cutting plane
    Base::Vector3d bboxCenter((xmin + xmax) / 2, (ymin + ymax) / 2, (zmin + zmax) / 2);
    double distToPlane = n * bboxCenter - d;
    Base::Vector3d planeCenter = bboxCenter - n * distToPlane;

    Base::Vector3d p0 = planeCenter + u * (umid - uHalf - (bboxCenter * u))
        + v * (vmid - vHalf - (bboxCenter * v));
    Base::Vector3d p1 = planeCenter + u * (umid + uHalf - (bboxCenter * u))
        + v * (vmid - vHalf - (bboxCenter * v));
    Base::Vector3d p2 = planeCenter + u * (umid + uHalf - (bboxCenter * u))
        + v * (vmid + vHalf - (bboxCenter * v));
    Base::Vector3d p3 = planeCenter + u * (umid - uHalf - (bboxCenter * u))
        + v * (vmid + vHalf - (bboxCenter * v));

    pcPlaneCoords->point.set1Value(0, SbVec3f(p0.x, p0.y, p0.z));
    pcPlaneCoords->point.set1Value(1, SbVec3f(p1.x, p1.y, p1.z));
    pcPlaneCoords->point.set1Value(2, SbVec3f(p2.x, p2.y, p2.z));
    pcPlaneCoords->point.set1Value(3, SbVec3f(p3.x, p3.y, p3.z));

    // Now safe to render the quad and border
    pcPlaneFaceSet->numVertices.set1Value(0, 4);
    static const int32_t borderIndices[] = {0, 1, 2, 3, 0, -1};
    pcPlaneBorderLines->coordIndex.setValues(0, 6, borderIndices);
}

void ViewProviderSectionAnalysis::updateHatchProjection()
{
    if (!pcHatchCoordGen) {
        return;
    }

    auto* feat = getObject<Part::SectionAnalysis>();
    Base::Vector3d n(0, 0, 1);
    if (feat) {
        n = feat->PlaneNormal.getValue();
        double len = n.Length();
        if (len > 1e-10) {
            n = n / len;
        }
    }

    // Build orthonormal frame on the cutting plane
    Base::Vector3d u, v;
    if (std::abs(n.x) < 0.9) {
        u = Base::Vector3d(1, 0, 0).Cross(n);
    }
    else {
        u = Base::Vector3d(0, 1, 0).Cross(n);
    }
    u.Normalize();
    v = n.Cross(u);
    v.Normalize();

    // 1 texture repeat per 8mm — gives ~1 line every 2mm
    float scale = 1.0f / 8.0f;
    pcHatchCoordGen->directionS.setValue(SbVec3f(u.x * scale, u.y * scale, u.z * scale));
    pcHatchCoordGen->directionT.setValue(SbVec3f(v.x * scale, v.y * scale, v.z * scale));
}

void ViewProviderSectionAnalysis::applyPerSolidColors()
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    const auto& counts = feat->SolidFaceCounts.getValues();
    if (counts.empty()) {
        return;
    }

    // Collect source body materials from the source's child bodies
    std::vector<App::Material> solidMats;
    App::DocumentObject* source = feat->Source.getValue();
    if (source) {
        auto children = source->getOutList();
        for (auto* child : children) {
            if (!child->isDerivedFrom(Part::Feature::getClassTypeId())) {
                continue;
            }
            auto* vp = Gui::Application::Instance->getViewProvider(child);
            auto* vpPart = dynamic_cast<ViewProviderPartExt*>(vp);
            if (vpPart) {
                App::Material mat = vpPart->ShapeAppearance[0];
                // Force fully opaque — section faces should never be
                // transparent even if the source body has transparency.
                mat.transparency = 0.0f;
                mat.diffuseColor.a = 0.0f;  // a=0 means opaque in FreeCAD
                solidMats.push_back(mat);
            }
        }
    }

    // Fallback: use a predefined palette
    if (solidMats.empty()) {
        float palette[][3] = {
            {0.8f, 0.3f, 0.2f},
            {0.2f, 0.5f, 0.8f},
            {0.3f, 0.7f, 0.3f},
            {0.8f, 0.7f, 0.2f},
            {0.6f, 0.3f, 0.7f},
            {0.9f, 0.5f, 0.3f},
        };
        for (size_t i = 0; i < counts.size(); i++) {
            App::Material mat;
            auto& p = palette[i % 6];
            mat.diffuseColor.set(p[0], p[1], p[2], 0.0f);
            solidMats.push_back(mat);
        }
    }

    // Build per-face material array
    int totalFaces = 0;
    for (auto c : counts) {
        totalFaces += c;
    }
    if (totalFaces == 0) {
        return;
    }

    std::vector<App::Material> materials;
    materials.reserve(totalFaces);
    for (size_t i = 0; i < counts.size(); i++) {
        const auto& mat = (i < solidMats.size()) ? solidMats[i] : solidMats.back();
        for (long j = 0; j < counts[i]; j++) {
            materials.push_back(mat);
        }
    }

    if (!materials.empty()) {
        ShapeAppearance.setValues(materials);
    }
}

void ViewProviderSectionAnalysis::updateStencilCap()
{
    if (!pcStencilCap) {
        return;
    }

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    // Sync cap quad corners from the plane visual
    if (pcPlaneCoords && pcPlaneCoords->point.getNum() >= 4) {
        pcStencilCap->capCorner0.setValue(pcPlaneCoords->point[0]);
        pcStencilCap->capCorner1.setValue(pcPlaneCoords->point[1]);
        pcStencilCap->capCorner2.setValue(pcPlaneCoords->point[2]);
        pcStencilCap->capCorner3.setValue(pcPlaneCoords->point[3]);
    }

    // Sync hatching parameters from the hatch coord gen
    if (pcHatchCoordGen) {
        pcStencilCap->hatchDirS.setValue(pcHatchCoordGen->directionS.getValue());
        pcStencilCap->hatchDirT.setValue(pcHatchCoordGen->directionT.getValue());
    }

    pcStencilCap->hatchEnabled.setValue(hatchEnabled);

    // Collect source geometry for the stencil fill pass
    std::vector<StencilSource> sources;
    for (auto* obj : clippedObjects) {
        auto* vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) {
            continue;
        }
        // Find the coordinate and face set nodes via scene graph search
        // (avoids accessing protected members of ViewProviderPartExt)
        auto* root = dynamic_cast<SoSeparator*>(vp->getRoot());
        if (!root) {
            continue;
        }

        SoSearchAction saCoords;
        saCoords.setType(SoCoordinate3::getClassTypeId());
        saCoords.setInterest(SoSearchAction::FIRST);
        saCoords.apply(root);
        auto* foundCoords = saCoords.getPath()
            ? static_cast<SoCoordinate3*>(saCoords.getPath()->getTail())
            : nullptr;

        SoSearchAction saFaces;
        saFaces.setType(SoIndexedFaceSet::getClassTypeId());
        saFaces.setInterest(SoSearchAction::FIRST);
        saFaces.apply(root);
        auto* foundFaces = saFaces.getPath()
            ? static_cast<SoIndexedFaceSet*>(saFaces.getPath()->getTail())
            : nullptr;

        if (!foundCoords || !foundFaces) {
            continue;
        }

        StencilSource src;
        src.coords = foundCoords;
        src.faceSet = foundFaces;
        // Get the object's world transform from its placement
        auto* geoFeat = dynamic_cast<App::GeoFeature*>(obj);
        if (geoFeat) {
            Base::Matrix4D mat = geoFeat->globalPlacement().toMatrix();
            // Convert Base::Matrix4D to SbMatrix (column-major)
            for (int r = 0; r < 4; r++) {
                for (int c = 0; c < 4; c++) {
                    src.transform[c][r] = static_cast<float>(mat[r][c]);
                }
            }
        }
        else {
            src.transform.makeIdentity();
        }

        sources.push_back(src);
    }

    pcStencilCap->setSources(sources);
}

void ViewProviderSectionAnalysis::setHatching(bool on)
{
    hatchEnabled = on;

    if (!pcHatchTexture || !pcHatchCoordGen || !pcRoot) {
        return;
    }

    if (on) {
        SoSearchAction sa;
        sa.setType(SoIndexedFaceSet::getClassTypeId());
        sa.setInterest(SoSearchAction::FIRST);
        sa.apply(pcRoot);
        SoPath* path = sa.getPath();
        if (path && path->getLength() >= 2) {
            auto* parent = static_cast<SoSeparator*>(path->getNodeFromTail(1));
            int faceIdx = parent->findChild(path->getTail());
            if (faceIdx >= 0 && parent->findChild(pcHatchTexture) < 0) {
                parent->insertChild(pcHatchTexture, faceIdx);
                parent->insertChild(pcHatchCoordGen, faceIdx);
            }
        }
    }
    else {
        SoSearchAction sa;
        sa.setNode(pcHatchTexture);
        sa.setInterest(SoSearchAction::FIRST);
        sa.apply(pcRoot);
        SoPath* path = sa.getPath();
        if (path && path->getLength() >= 2) {
            auto* parent = static_cast<SoSeparator*>(path->getNodeFromTail(1));
            parent->removeChild(pcHatchTexture);
        }

        SoSearchAction sa2;
        sa2.setNode(pcHatchCoordGen);
        sa2.setInterest(SoSearchAction::FIRST);
        sa2.apply(pcRoot);
        SoPath* path2 = sa2.getPath();
        if (path2 && path2->getLength() >= 2) {
            auto* parent = static_cast<SoSeparator*>(path2->getNodeFromTail(1));
            parent->removeChild(pcHatchCoordGen);
        }
    }
}

void ViewProviderSectionAnalysis::setShowPlane(bool on)
{
    if (pcPlaneSwitch) {
        pcPlaneSwitch->whichChild = on ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }
}

void ViewProviderSectionAnalysis::setPerSolidColors(bool on)
{
    usePerSolidColors = on;
    if (on) {
        applyPerSolidColors();
    }
    else {
        // Restore single section color
        App::Material mat;
        mat.diffuseColor.set(0.8f, 0.3f, 0.2f, 0.0f);
        ShapeAppearance.setValues({mat});
        if (hatchEnabled) {
            setHatching(true);
        }
    }
}

void ViewProviderSectionAnalysis::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    Q_UNUSED(ModNum);
    if (!viewer) {
        return;
    }

    // Get the gizmo container from the active task panel
    Gui::TaskView::TaskDialog* activeDlg = Gui::Control().activeDialog();
    auto* saDlg = qobject_cast<TaskSectionAnalysis*>(activeDlg);
    if (saDlg && saDlg->getGizmoContainer()) {
        // Use identity placement — our gizmo positions are already in world space
        Base::Placement identity;
        saDlg->getGizmoContainer()->attachViewer(viewer, identity);
    }
}

void ViewProviderSectionAnalysis::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    ViewProviderDragger::unsetEditViewer(viewer);
}

void ViewProviderSectionAnalysis::show()
{
    installClipPlane();
    updatePlaneVisual();
    updateHatchProjection();
    updateStencilCap();
    if (usePerSolidColors) {
        applyPerSolidColors();
    }
    if (hatchEnabled) {
        setHatching(true);
    }
    // Plane visual hidden by default — shown when editing via task panel
    if (pcPlaneSwitch) {
        pcPlaneSwitch->whichChild = SO_SWITCH_NONE;
    }
    ViewProviderPart::show();
}

void ViewProviderSectionAnalysis::hide()
{
    removeClipPlane();
    if (pcPlaneSwitch) {
        pcPlaneSwitch->whichChild = SO_SWITCH_NONE;
    }
    ViewProviderPart::hide();
}

void ViewProviderSectionAnalysis::updateData(const App::Property* prop)
{
    ViewProviderPart::updateData(prop);

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || !prop) {
        return;
    }

    if (prop == &feat->PlaneNormal || prop == &feat->PlaneOffset || prop == &feat->FlipCut) {
        updateClipPlaneEquation();
        updatePlaneVisual();
        if (prop == &feat->PlaneNormal) {
            updateHatchProjection();
        }
        updateStencilCap();
    }

    if (prop == &feat->Source) {
        removeClipPlane();
        if (Visibility.getValue()) {
            installClipPlane();
        }
        updatePlaneVisual();
        updateStencilCap();
    }

    // After a shape recompute, the face set geometry is rebuilt and all
    // source shapes are now available (important on document load where
    // show() runs before the recompute).
    if (prop == &feat->Shape) {
        if (!clipInstalled && Visibility.getValue()) {
            installClipPlane();
        }
        updatePlaneVisual();
        updateStencilCap();
        if (usePerSolidColors) {
            applyPerSolidColors();
        }
        if (hatchEnabled) {
            setHatching(true);
        }
    }
}

void ViewProviderSectionAnalysis::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    addDefaultAction(menu, QObject::tr("Edit Section Analysis"));
    ViewProviderPart::setupContextMenu(menu, receiver, member);
}

bool ViewProviderSectionAnalysis::onDelete(const std::vector<std::string>&)
{
    removeClipPlane();
    return true;
}

bool ViewProviderSectionAnalysis::setEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog(getDocument()->getDocument());
        TaskSectionAnalysis* saDlg = qobject_cast<TaskSectionAnalysis*>(dlg);
        if (saDlg && saDlg->getObject() != this->getObject()) {
            saDlg = nullptr;
        }
        if (dlg && !saDlg) {
            if (dlg->canClose()) {
                Gui::Control().closeDialog(getDocument()->getDocument());
            }
            else {
                return false;
            }
        }

        Gui::Selection().clearSelection();

        // Show the cutting plane visual when entering edit mode
        if (pcPlaneSwitch) {
            pcPlaneSwitch->whichChild = SO_SWITCH_ALL;
        }

        if (saDlg) {
            Gui::Control().showDialog(saDlg, getDocument()->getDocument());
        }
        else {
            // The task panel sets up gizmos via GizmoContainer::create(vp)
            // setEditViewer() will then attach them to the 3D viewer
            Gui::Control().showDialog(
                new TaskSectionAnalysis(getObject<Part::SectionAnalysis>(), this),
                getDocument()->getDocument()
            );
        }

        return true;
    }
    else {
        return ViewProviderPart::setEdit(ModNum);
    }
}

void ViewProviderSectionAnalysis::unsetEdit(int ModNum)
{
    if (ModNum == ViewProvider::Default) {
        // Hide the cutting plane visual when leaving edit mode
        if (pcPlaneSwitch) {
            pcPlaneSwitch->whichChild = SO_SWITCH_NONE;
        }
        Gui::Control().closeDialog(nullptr);
    }
    else {
        ViewProviderPart::unsetEdit(ModNum);
    }
}
