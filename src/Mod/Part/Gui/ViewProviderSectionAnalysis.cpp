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
#include <functional>

#include <QAction>
#include <QMenu>
#include <QTimer>

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

#include <App/Application.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Inventor/Draggers/SoTransformDragger.h>
#include <Gui/ViewParams.h>
#include <Gui/Selection/Selection.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>

#include "ViewProviderExt.h"
#include "ViewProviderSectionAnalysis.h"
#include "TaskSectionAnalysis.h"


using namespace PartGui;

PROPERTY_SOURCE(PartGui::ViewProviderSectionAnalysis, PartGui::ViewProviderPart)

ViewProviderSectionAnalysis::ViewProviderSectionAnalysis()
{
    sPixmap = "Part_SectionAnalysis";

    ADD_PROPERTY_TYPE(
        ShowHatching,
        (true),
        "Section Analysis",
        App::Prop_None,
        "Show diagonal hatching lines on cross-section faces"
    );
    ADD_PROPERTY_TYPE(
        PerBodyColors,
        (false),
        "Section Analysis",
        App::Prop_None,
        "Use source body colors for cross-section faces"
    );

    // Default section face color: reddish-orange
    App::Material mat;
    mat.diffuseColor.set(0.8f, 0.3f, 0.2f, 0.0f);
    ShapeAppearance.setValues({mat});
}

ViewProviderSectionAnalysis::~ViewProviderSectionAnalysis()
{
    removeClipPlane();
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

    visibilityConn = pcFeat->getDocument()->signalChangedObject.connect(
        std::bind(&ViewProviderSectionAnalysis::slotChangedObject, this,
                  std::placeholders::_1, std::placeholders::_2)
    );

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
        // Small tileable texture — GPU repeats it via GL_REPEAT.
        // 16x16 with one diagonal line = minimal memory, same visual.
        const int sz = 16;
        const int lineWidth = 1;
        unsigned char* img = new unsigned char[sz * sz * 3];
        std::memset(img, 255, sz * sz * 3);  // white background
        for (int y = 0; y < sz; y++) {
            for (int x = 0; x < sz; x++) {
                int idx = (y * sz + x) * 3;
                if (((x + y) % sz) < lineWidth) {
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

    updatePlaneVisual();
}

void ViewProviderSectionAnalysis::finishRestoring()
{
    ViewProviderPart::finishRestoring();

    // Restore persisted settings
    hatchEnabled = ShowHatching.getValue();
    usePerSolidColors = PerBodyColors.getValue();

    // After document restore the scene graph is fully built it is safe to set up
    // clip planes, plane visual and hatching(s)
    if (Visibility.getValue()) {
        installClipPlane();
    }
    updatePlaneVisual();
    updateHatchProjection();
    if (usePerSolidColors) {
        applyPerSolidColors();
    }
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

    // Clip exactly the objects that contributed section geometry (SourceParts,
    // computed by execute()), so clipping and capping always agree.
    // Each object gets its own clip plane node, inserted just before the mode switch
    // so the full global placement is in effect when
    // the node is traversed: updateClipPlaneEquation() then only needs the
    // inverse global placement. A shared node can't work as each insertion point
    // sits under a different accumulated transform.
    std::vector<App::DocumentObject*> targets = feat->SourceParts.getValues();
    if (targets.empty() && feat->Source.getValue()) {
        targets.push_back(feat->Source.getValue());
    }
    for (auto* obj : targets) {
        if (!obj || obj == feat) {
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
        int switchIdx = root->findChild(vp->getModeSwitch());
        auto* node = new SoClipPlane;
        node->ref();
        root->insertChild(node, switchIdx >= 0 ? switchIdx : 1);
        clippedObjects.push_back(obj);
        clipNodes.push_back(node);
    }
    clipInstalled = !clipNodes.empty();
    updateClipPlaneEquation();
}

void ViewProviderSectionAnalysis::removeClipPlane()
{
    for (size_t i = 0; i < clippedObjects.size(); ++i) {
        SoClipPlane* node = (i < clipNodes.size()) ? clipNodes[i] : nullptr;
        if (!node) {
            continue;
        }
        if (auto* vp = Gui::Application::Instance->getViewProvider(clippedObjects[i])) {
            if (auto* root = dynamic_cast<SoSeparator*>(vp->getRoot())) {
                int idx = root->findChild(node);
                if (idx >= 0) {
                    root->removeChild(idx);
                }
            }
        }
        node->unref();
    }
    clippedObjects.clear();
    clipNodes.clear();
    clipInstalled = false;
}

void ViewProviderSectionAnalysis::updateClipPlaneEquation()
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || clipNodes.empty()) {
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

    // The cutting plane passes through point (n * d) with normal vect n.
    // OCCT keeps the negative normal side (where n.p < d).
    // SoClipPlane keeps the positive normal half space, so we negate the
    // normal to keep the same side as OCCT.
    //
    // Offset the clip plane by a tiny epsilon toward the remaining solid
    // so it clips slightly past the section face. This prevents Z fighting
    // between the source body's surface at the clip boundary and the
    // section face (which lies exactly on the cutting plane).
    constexpr double clipEps = 0.01;  // 10 microns
    const double dClip = d - clipEps;
    const Base::Vector3d gNormal(-n.x, -n.y, -n.z);          // global half-space normal
    const Base::Vector3d gPoint(n.x * dClip, n.y * dClip, n.z * dClip);  // global point on plane

    // Each node lives inside its object's view provider, so its plane must be
    // expressed in that object's local frame: transform the global plane by the
    // inverse of the object's global placement. Objects with identity placement
    // get the global plane unchanged.
    for (size_t i = 0; i < clipNodes.size(); ++i) {
        SoClipPlane* node = clipNodes[i];
        if (!node) {
            continue;
        }
        App::DocumentObject* obj = (i < clippedObjects.size()) ? clippedObjects[i] : nullptr;

        Base::Vector3d localNormal = gNormal;
        Base::Vector3d localPoint = gPoint;
        if (obj && obj->isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            const Base::Placement inv = App::GeoFeature::getGlobalPlacement(obj).inverse();
            inv.getRotation().multVec(gNormal, localNormal);  // rotate the normal only
            inv.multVec(gPoint, localPoint);                  // full transform for the point
        }

        node->plane.setValue(SbPlane(
            SbVec3f(static_cast<float>(localNormal.x),
                    static_cast<float>(localNormal.y),
                    static_cast<float>(localNormal.z)),
            SbVec3f(static_cast<float>(localPoint.x),
                    static_cast<float>(localPoint.y),
                    static_cast<float>(localPoint.z))
        ));
        node->on.setValue(TRUE);
    }
}

void ViewProviderSectionAnalysis::slotChangedObject(
    const App::DocumentObject& obj,
    const App::Property& prop
)
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || &obj == feat || &prop != &obj.Visibility) {
        return;
    }
    if (feat->getDocument()->testStatus(App::Document::Restoring)
        || feat->getDocument()->testStatus(App::Document::Recomputing)) {
        return;
    }

    // Figure out if there's a dependency, so that we dont cut hidden objects
    App::DocumentObject* src = feat->Source.getValue();
    auto isAncestorOrSelf = [](const App::DocumentObject* a, const App::DocumentObject* b) {
        for (const auto* o = b; o; o = App::GeoFeatureGroupExtension::getGroupOfObject(o)) {
            if (o == a) {
                return true;
            }
        }
        return false;
    };
    if (!isAncestorOrSelf(src, &obj) && !isAncestorOrSelf(&obj, src)) {
        return;
    }

    // Defer the recompute out of the property-change signal
    feat->enforceRecompute();
    std::string docName = feat->getDocument()->getName();
    QTimer::singleShot(0, [docName]() {
        if (auto* doc = App::GetApplication().getDocument(docName.c_str())) {
            doc->recompute();
        }
    });
}

void ViewProviderSectionAnalysis::refreshSourceBBoxCache()
{
    sourceBBoxValid = false;

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }
    App::DocumentObject* source = feat->Source.getValue();
    if (!source) {
        return;
    }

    // Resolving the whole source shape is the expensive part on large
    // assemblies, which is exactly why the result is cached here rather than
    // recomputed on every plane move.
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

    bbox.Get(sourceBBox[0], sourceBBox[1], sourceBBox[2], sourceBBox[3], sourceBBox[4], sourceBBox[5]);
    sourceBBoxValid = true;
}

void ViewProviderSectionAnalysis::updatePlaneVisual()
{
    if (!pcPlaneCoords || !pcPlaneFaceSet) {
        return;
    }

    // Default to hidden and will be enabled if we successfully compute coordinates
    pcPlaneFaceSet->numVertices.set1Value(0, 0);
    pcPlaneBorderLines->coordIndex.setNum(0);  // clear border indices too

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    // Use cached source bounding box (refreshed only when the geometry can
    // have changed) so dragging the plane stays cheap on large assemblies.
    if (!sourceBBoxValid) {
        refreshSourceBBoxCache();
    }
    if (!sourceBBoxValid) {
        return;
    }
    const double xmin = sourceBBox[0];
    const double ymin = sourceBBox[1];
    const double zmin = sourceBBox[2];
    const double xmax = sourceBBox[3];
    const double ymax = sourceBBox[4];
    const double zmax = sourceBBox[5];

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

    // Authoritative solid-to-source mapping computed in execute():
    //   srcIdx[s] = index into `parts` of the object that produced solid s.
    // If object contains several solids it will appear once in `parts`,
    // so all of its solids get the same colour.
    const auto& srcIdx = feat->SolidSourceIndex.getValues();
    const auto& parts = feat->SourceParts.getValues();

    // Predefined fallback palette
    static const float palette[][3] = {
        {0.8f, 0.3f, 0.2f},
        {0.2f, 0.5f, 0.8f},
        {0.3f, 0.7f, 0.3f},
        {0.8f, 0.7f, 0.2f},
        {0.6f, 0.3f, 0.7f},
        {0.9f, 0.5f, 0.3f},
    };
    auto paletteMat = [](size_t i) {
        App::Material mat;
        const auto& p = palette[i % 6];
        mat.diffuseColor.set(p[0], p[1], p[2], 0.0f);
        return mat;
    };

    std::vector<App::Material> partMats;
    partMats.reserve(parts.size());
    for (size_t i = 0; i < parts.size(); i++) {
        App::Material mat = paletteMat(i);
        if (parts[i]) {
            auto* vp = Gui::Application::Instance->getViewProvider(parts[i]);
            if (auto* vpPart = dynamic_cast<ViewProviderPartExt*>(vp)) {
                mat = vpPart->ShapeAppearance[0];
            }
        }
        // Force fully opaque — section faces should never be transparent
        mat.transparency = 0.0f;
        mat.diffuseColor.a = 0.0f;
        partMats.push_back(mat);
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
    for (size_t s = 0; s < counts.size(); s++) {
        long pi = (s < srcIdx.size()) ? srcIdx[s] : static_cast<long>(s);
        App::Material mat = (pi >= 0 && pi < static_cast<long>(partMats.size()))
            ? partMats[pi]
            : paletteMat(static_cast<size_t>(pi < 0 ? s : pi));
        for (long j = 0; j < counts[s]; j++) {
            materials.push_back(mat);
        }
    }

    if (!materials.empty()) {
        ShapeAppearance.setValues(materials);
    }
}

void ViewProviderSectionAnalysis::setHatching(bool on)
{
    hatchEnabled = on;
    ShowHatching.setValue(on);

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
        // Clear per-part texture rotations
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
    PerBodyColors.setValue(on);
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
    if (!viewer || !transformDragger) {
        return;
    }

    transformDragger->setUpAutoScale(viewer->getSoRenderManager()->getCamera());

    // Orient the dragger so Z aligns with the cutting plane normal.
    // Translation along Z = change PlaneOffset.
    // Rotation around X/Y = change PlaneNormal.
    auto* feat = getObject<Part::SectionAnalysis>();
    if (feat) {
        Base::Vector3d n = feat->PlaneNormal.getValue();
        double len = n.Length();
        if (len > 1e-10) {
            n = n / len;
        }

        // Build rotation that maps (0,0,1) → plane normal
        Base::Rotation rot(Base::Vector3d(0, 0, 1), n);
        Base::Vector3d planePoint = n * feat->PlaneOffset.getValue();
        Base::Matrix4D mat = Base::Placement(planePoint, rot).toMatrix();

        viewer->getDocument()->setEditingTransform(mat);
        viewer->setupEditingRoot(transformDragger, &mat);
    }
}

void ViewProviderSectionAnalysis::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    ViewProviderDragger::unsetEditViewer(viewer);
}

void ViewProviderSectionAnalysis::sectionDragStartCallback(void* data, SoDragger*)
{
    auto* vp = static_cast<ViewProviderSectionAnalysis*>(data);
    vp->transformDragger->clearIncrementCounts();

    // Save the initial plane state
    auto* feat = vp->getObject<Part::SectionAnalysis>();
    if (feat) {
        Base::Vector3d n = feat->PlaneNormal.getValue();
        double d = feat->PlaneOffset.getValue();
        double len = n.Length();
        if (len > 1e-10) {
            n = n / len;
        }
        Base::Rotation rot(Base::Vector3d(0, 0, 1), n);
        vp->draggerStartPlacement = Base::Placement(n * d, rot);
    }
}

void ViewProviderSectionAnalysis::sectionDragMotionCallback(void* data, SoDragger*)
{
    auto* vp = static_cast<ViewProviderSectionAnalysis*>(data);
    auto* feat = vp->getObject<Part::SectionAnalysis>();
    if (!feat || !vp->transformDragger) {
        return;
    }

    // Read incremental changes from the dragger
    double transStep = vp->transformDragger->translationIncrement.getValue();
    int zSteps = vp->transformDragger->translationIncrementCountZ.getValue();
    double rotStep = vp->transformDragger->rotationIncrement.getValue();
    int xRotSteps = vp->transformDragger->rotationIncrementCountX.getValue();
    int yRotSteps = vp->transformDragger->rotationIncrementCountY.getValue();

    // Apply Z translation → PlaneOffset change
    double offsetDelta = zSteps * transStep;
    Base::Vector3d startNormal = vp->draggerStartPlacement.getRotation().multVec(
        Base::Vector3d(0, 0, 1)
    );
    double startOffset = startNormal * vp->draggerStartPlacement.getPosition();
    double newOffset = startOffset + offsetDelta;

    // Apply X/Y rotation → PlaneNormal change
    Base::Rotation startRot = vp->draggerStartPlacement.getRotation();
    Base::Rotation deltaRot;
    if (xRotSteps != 0 || yRotSteps != 0) {
        // Rotation around the dragger's local X and Y axes
        Base::Vector3d localX = startRot.multVec(Base::Vector3d(1, 0, 0));
        Base::Vector3d localY = startRot.multVec(Base::Vector3d(0, 1, 0));
        Base::Rotation rotX(localX, xRotSteps * rotStep);
        Base::Rotation rotY(localY, yRotSteps * rotStep);
        deltaRot = rotY * rotX;
    }

    Base::Vector3d newNormal = deltaRot.multVec(startNormal);
    newNormal.Normalize();

    feat->PlaneNormal.setValue(newNormal);
    feat->PlaneOffset.setValue(newOffset);

    // Sync the task panel UI to reflect dragger changes
    auto* dlg = qobject_cast<TaskSectionAnalysis*>(Gui::Control().activeDialog());
    if (dlg) {
        dlg->updateFromFeature();
    }
}

void ViewProviderSectionAnalysis::sectionDragFinishCallback(void* data, SoDragger*)
{
    auto* vp = static_cast<ViewProviderSectionAnalysis*>(data);
    if (vp->transformDragger) {
        vp->transformDragger->clearIncrementCounts();
    }

    // Recompute the section faces once for the final plane pose
    auto* feat = vp->getObject<Part::SectionAnalysis>();
    if (feat) {
        feat->recomputeFeature();
    }
}

void ViewProviderSectionAnalysis::show()
{
    installClipPlane();
    updatePlaneVisual();
    updateHatchProjection();
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
        // Runs on every gizmo motion and so must stay cheap
        updateClipPlaneEquation();
        updatePlaneVisual();
        if (prop == &feat->PlaneNormal) {
            updateHatchProjection();
        }
    }

    // The clipped set follows SourceParts, which execute() rebuilds
    if (prop == &feat->Source || prop == &feat->SourceParts) {
        removeClipPlane();
        if (Visibility.getValue()) {
            installClipPlane();
        }
        refreshSourceBBoxCache();
        updatePlaneVisual();
    }

    if (prop == &feat->Shape) {
        if (!clipInstalled && Visibility.getValue()) {
            installClipPlane();
        }
        refreshSourceBBoxCache();
        updatePlaneVisual();
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

        // Create the Transform dragger — configured for section plane DOF:
        // Z translation (offset along normal) + X/Y rotation (tilt)
        if (!transformDragger) {
            transformDragger = new Gui::SoTransformDragger();
            transformDragger->setAxisColors(
                Gui::ViewParams::instance()->getAxisXColor(),
                Gui::ViewParams::instance()->getAxisYColor(),
                Gui::ViewParams::instance()->getAxisZColor()
            );
            transformDragger->draggerSize.setValue(Gui::ViewParams::instance()->getDraggerScale());

            // Hide axis labels — not meaningful for section plane
            transformDragger->xAxisLabel.setValue("");
            transformDragger->yAxisLabel.setValue("");
            transformDragger->zAxisLabel.setValue("");

            // Finer increments for section plane manipulation
            transformDragger->translationIncrement.setValue(0.1);        // 0.1mm steps
            transformDragger->rotationIncrement.setValue(M_PI / 180.0);  // 1° steps

            // Section plane: only Z translation + X/Y rotation
            transformDragger->hideTranslationX();
            transformDragger->hideTranslationY();
            transformDragger->showTranslationZ();
            transformDragger->showRotationX();
            transformDragger->showRotationY();
            transformDragger->hideRotationZ();
            transformDragger->hidePlanarTranslationXY();
            transformDragger->hidePlanarTranslationYZ();
            transformDragger->hidePlanarTranslationZX();

            transformDragger->addStartCallback(sectionDragStartCallback, this);
            transformDragger->addFinishCallback(sectionDragFinishCallback, this);
            transformDragger->addMotionCallback(sectionDragMotionCallback, this);
        }

        if (saDlg) {
            Gui::Control().showDialog(saDlg, getDocument()->getDocument());
        }
        else {
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
        transformDragger.reset();
        Gui::Control().closeDialog(nullptr);
    }
    else {
        ViewProviderPart::unsetEdit(ModNum);
    }
}
