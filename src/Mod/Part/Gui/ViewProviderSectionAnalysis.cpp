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

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>

#include <QAction>
#include <QMenu>
#include <QTimer>

#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoLevelOfDetail.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoSwitch.h>

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

#include "SoBrepFaceSet.h"
#include "ViewProviderExt.h"
#include "ViewProviderSectionAnalysis.h"
#include "TaskSectionAnalysis.h"


using namespace PartGui;

App::PropertyFloatConstraint::Constraints ViewProviderSectionAnalysis::hatchWidthRange = {
    1.0,
    64.0,
    1.0
};

// Fade-out steps for the hatching, from crisp to nearly gone: the transparency
// of each level-of-detail step, paired with the on-screen pixels per hatch line
// at which that step takes over. Below the last one the hatching is dropped.
constexpr float hatchFadeSteps[] = {0.0f, 0.45f, 0.75f};
constexpr double hatchFadePixelsPerLine[] = {3.0, 1.5, 0.75};

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
        HatchLineWidth,
        (2.0),
        "Section Analysis",
        App::Prop_None,
        "Width of the hatching lines in pixels"
    );
    HatchLineWidth.setConstraints(&hatchWidthRange);
    ADD_PROPERTY_TYPE(
        HatchSpacing,
        (2.0),
        "Section Analysis",
        App::Prop_None,
        "Distance between the hatching lines"
    );
    ADD_PROPERTY_TYPE(
        AutoHideHatching,
        (true),
        "Section Analysis",
        App::Prop_None,
        "Hide the hatching once the section is too small on screen for the\n"
        "individual lines to be told apart"
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
}

void ViewProviderSectionAnalysis::attach(App::DocumentObject* pcFeat)
{
    ViewProviderPart::attach(pcFeat);

    visibilityConn = pcFeat->getDocument()->signalChangedObject.connect(
        std::bind(
            &ViewProviderSectionAnalysis::slotChangedObject,
            this,
            std::placeholders::_1,
            std::placeholders::_2
        )
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

    // Hatching 45° diagonal lines per ISO 128-50, built as real line geometry
    // rather than a repeating texture: the lines stay one pixel-crisp at any
    // zoom level and their width is whatever HatchLineWidth says.
    pcHatchRoot = new SoSeparator();
    pcHatchRoot->setName("SectionHatching");
    pcHatchRoot->renderCaching = SoSeparator::OFF;

    auto* hatchPick = new SoPickStyle();
    hatchPick->style = SoPickStyle::UNPICKABLE;
    pcHatchRoot->addChild(hatchPick);

    auto* hatchBind = new SoMaterialBinding();
    hatchBind->value = SoMaterialBinding::OVERALL;
    pcHatchRoot->addChild(hatchBind);

    pcHatchStyle = new SoDrawStyle();
    pcHatchStyle->style = SoDrawStyle::LINES;
    pcHatchStyle->lineWidth.setValue(static_cast<float>(HatchLineWidth.getValue()));
    pcHatchRoot->addChild(pcHatchStyle);

    pcHatchCoords = new SoCoordinate3();
    pcHatchRoot->addChild(pcHatchCoords);

    pcHatchLines = new SoIndexedLineSet();
    pcHatchRoot->addChild(pcHatchLines);

    // As the section shrinks on screen the lines crowd together and would merge
    // into a solid tone, so the LOD node steps through increasingly transparent
    // copies of the same geometry before dropping it altogether (last, empty
    // child). Each level only differs by its material, so they all share the one
    // set of coordinates and the one line set below pcHatchRoot.
    pcHatchLod = new SoLevelOfDetail();
    for (float alpha : hatchFadeSteps) {
        auto* level = new SoSeparator();
        level->renderCaching = SoSeparator::OFF;
        auto* levelMat = new SoMaterial();
        levelMat->diffuseColor.setValue(0.1f, 0.1f, 0.1f);
        levelMat->transparency.setValue(alpha);
        level->addChild(levelMat);
        level->addChild(pcHatchRoot);
        pcHatchLod->addChild(level);
    }
    pcHatchLod->addChild(new SoGroup());

    pcHatchSwitch = new SoSwitch();
    pcHatchSwitch->addChild(pcHatchLod);
    pcHatchSwitch->whichChild = (hatchEnabled && Visibility.getValue()) ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    pcRoot->addChild(pcHatchSwitch);

    updateHatchGeometry();
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
    if (usePerSolidColors) {
        applyPerSolidColors();
    }
    setHatching(hatchEnabled);
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
    const Base::Vector3d gNormal(-n.x, -n.y, -n.z);                      // global half-space normal
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
            SbVec3f(
                static_cast<float>(localNormal.x),
                static_cast<float>(localNormal.y),
                static_cast<float>(localNormal.z)
            ),
            SbVec3f(
                static_cast<float>(localPoint.x),
                static_cast<float>(localPoint.y),
                static_cast<float>(localPoint.z)
            )
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

void ViewProviderSectionAnalysis::updateHatchGeometry()
{
    if (!pcHatchCoords || !pcHatchLines) {
        return;
    }

    pcHatchLines->coordIndex.setNum(0);
    pcHatchCoords->point.setNum(0);

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!hatchEnabled || !feat) {
        return;
    }

    Base::Vector3d n = feat->PlaneNormal.getValue();
    const double len = n.Length();
    if (len < 1e-10) {
        return;
    }
    n = n / len;

    // Build orthonormal frame on the cutting plane (same convention as the
    // plane visual, so hatching and plane quad stay in step)
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

    // Hatch lines run at 45° in that frame; `levelDir` is their in-plane normal,
    // so a hatch line is the set of points with p * levelDir == k * spacing.
    Base::Vector3d levelDir = u - v;
    levelDir.Normalize();

    // PropertyLength only clamps to >= 0 through the editor, and setValue() from
    // C++ or a restored file bypasses even that, so anything can land here
    constexpr double minHatchSpacing = 0.001;
    const double spacing = HatchSpacing.getValue();
    if (!std::isfinite(spacing) || spacing < minHatchSpacing) {
        return;
    }

    // The section faces lie exactly on the cutting plane, so the lines would
    // z-fight with them. Lift them by a fraction of the model size towards the
    // cut-away side, i.e. the side the section face is looked at from.
    const Base::Vector3d cutNormal = feat->FlipCut.getValue() ? -n : n;
    double modelSize = 1.0;
    if (sourceBBoxValid) {
        const double dx = sourceBBox[3] - sourceBBox[0];
        const double dy = sourceBBox[4] - sourceBBox[1];
        const double dz = sourceBBox[5] - sourceBBox[2];
        modelSize = std::max(std::sqrt(dx * dx + dy * dy + dz * dz), 1.0);
    }
    const Base::Vector3d lift = cutNormal * (modelSize * 1e-4);

    // Walk the tessellation of the section faces and slice every triangle with
    // the family of hatch lines (marching triangles). Using the triangles the
    // viewer already renders means the hatching is trimmed to exactly the same
    // outline as the visible faces, with no extra shape/boolean work.
    const int numPts = coords->point.getNum();
    const int numIdx = faceset->coordIndex.getNum();
    if (numPts < 3 || numIdx < 4) {
        return;
    }
    const SbVec3f* pts = coords->point.getValues(0);
    const int32_t* cidx = faceset->coordIndex.getValues(0);

    // Guard against a pathologically small spacing eating all the memory
    constexpr int maxSegments = 500000;

    std::vector<SbVec3f> segPts;
    std::vector<int32_t> segIdx;

    // How far the hatched region reaches across the lines, i.e. how many lines
    // there are — the auto-hide threshold below is derived from it
    double spanMin = std::numeric_limits<double>::max();
    double spanMax = std::numeric_limits<double>::lowest();

    // If there is an easier way to do this already existing in FreeCAD codebase,
    // I could not find it...
    auto sliceTriangle = [&](int32_t ia, int32_t ib, int32_t ic) {
        const Base::Vector3d p[3] = {
            Base::Vector3d(pts[ia][0], pts[ia][1], pts[ia][2]),
            Base::Vector3d(pts[ib][0], pts[ib][1], pts[ib][2]),
            Base::Vector3d(pts[ic][0], pts[ic][1], pts[ic][2])
        };
        const double s[3] = {p[0] * levelDir, p[1] * levelDir, p[2] * levelDir};

        const double smin = std::min({s[0], s[1], s[2]});
        const double smax = std::max({s[0], s[1], s[2]});
        spanMin = std::min(spanMin, smin);
        spanMax = std::max(spanMax, smax);
        // Bound the range while it is still floating point, where overflow only
        // saturates: an out-of-range float to integer cast is undefined
        // behaviour, and `long` is 32 bit on Windows
        const double kminD = std::ceil(smin / spacing);
        const double kmaxD = std::floor(smax / spacing);
        if (!(kmaxD - kminD <= maxSegments)) {
            return;
        }
        const auto kmin = static_cast<std::int64_t>(kminD);
        const auto kmax = static_cast<std::int64_t>(kmaxD);

        for (std::int64_t k = kmin; k <= kmax; ++k) {
            const double level = static_cast<double>(k) * spacing;
            // Half-open sign test: every triangle yields exactly 0 or 2
            // crossings, so vertices sitting on a hatch line can't produce
            // duplicate or dangling segments.
            const bool above[3] = {s[0] > level, s[1] > level, s[2] > level};

            Base::Vector3d hit[2];
            int numHits = 0;
            for (int e = 0; e < 3 && numHits < 2; ++e) {
                const int a = e;
                const int b = (e + 1) % 3;
                if (above[a] == above[b]) {
                    continue;
                }
                const double t = (level - s[a]) / (s[b] - s[a]);
                hit[numHits++] = p[a] + (p[b] - p[a]) * t;
            }
            if (numHits < 2) {
                continue;
            }
            if (static_cast<int>(segIdx.size()) / 3 >= maxSegments) {
                return;
            }

            const auto base = static_cast<int32_t>(segPts.size());
            for (auto& h : hit) {
                const Base::Vector3d q = h + lift;
                segPts.emplace_back(
                    static_cast<float>(q.x),
                    static_cast<float>(q.y),
                    static_cast<float>(q.z)
                );
            }
            segIdx.push_back(base);
            segIdx.push_back(base + 1);
            segIdx.push_back(SO_END_LINE_INDEX);
        }
    };

    // SoBrepFaceSet stores triangles as {i0, i1, i2, -1}, but parse generically
    // (fan-triangulating any polygon) so this survives a different tesselation.
    std::vector<int32_t> poly;
    poly.reserve(4);
    for (int i = 0; i <= numIdx; ++i) {
        const int32_t idx = (i < numIdx) ? cidx[i] : SO_END_LINE_INDEX;
        if (idx >= 0) {
            if (idx < numPts) {
                poly.push_back(idx);
            }
            continue;
        }
        for (size_t j = 2; j < poly.size(); ++j) {
            sliceTriangle(poly[0], poly[j - 1], poly[j]);
        }
        poly.clear();
    }

    if (segPts.empty()) {
        return;
    }

    // Screen areas at which each fade step takes over: the region holds
    // `span / spacing` lines and each needs a few pixels to read as a line, so
    // the thresholds grow with the line count rather than being a fixed size.
    // An empty screenArea makes the LOD node always pick the first child.
    if (pcHatchLod) {
        pcHatchLod->screenArea.setNum(0);
        if (AutoHideHatching.getValue() && spanMax > spanMin) {
            const double lines = (spanMax - spanMin) / spacing;
            float thresholds[std::size(hatchFadePixelsPerLine)];
            for (size_t i = 0; i < std::size(hatchFadePixelsPerLine); ++i) {
                const double px = hatchFadePixelsPerLine[i] * lines;
                thresholds[i] = static_cast<float>(px * px);
            }
            pcHatchLod->screenArea.setValues(0, static_cast<int>(std::size(thresholds)), thresholds);
        }
    }

    pcHatchCoords->point.setValues(0, static_cast<int>(segPts.size()), segPts.data());
    pcHatchLines->coordIndex.setValues(0, static_cast<int>(segIdx.size()), segIdx.data());
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
    if (ShowHatching.getValue() != on) {
        ShowHatching.setValue(on);
    }

    // The hatching hangs off pcRoot rather than the display-mode switch, so it
    // has to follow the object's visibility explicitly
    if (pcHatchSwitch) {
        pcHatchSwitch->whichChild = (on && Visibility.getValue()) ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }
    updateHatchGeometry();
}

void ViewProviderSectionAnalysis::onChanged(const App::Property* prop)
{
    if (prop == &HatchLineWidth) {
        if (pcHatchStyle) {
            pcHatchStyle->lineWidth.setValue(static_cast<float>(HatchLineWidth.getValue()));
        }
    }
    else if (prop == &HatchSpacing || prop == &AutoHideHatching) {
        updateHatchGeometry();
    }
    else if (prop == &ShowHatching) {
        // Keep the property editor and the task panel switch in sync
        if (ShowHatching.getValue() != hatchEnabled) {
            setHatching(ShowHatching.getValue());
        }
    }

    ViewProviderPart::onChanged(prop);
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
    if (usePerSolidColors) {
        applyPerSolidColors();
    }
    // Plane visual hidden by default — shown when editing via task panel
    if (pcPlaneSwitch) {
        pcPlaneSwitch->whichChild = SO_SWITCH_NONE;
    }
    ViewProviderPart::show();

    // After the base class, which rebuilds the tessellation the hatching is
    // sliced from when it went stale while hidden
    setHatching(hatchEnabled);
}

void ViewProviderSectionAnalysis::hide()
{
    removeClipPlane();
    if (pcPlaneSwitch) {
        pcPlaneSwitch->whichChild = SO_SWITCH_NONE;
    }
    if (pcHatchSwitch) {
        pcHatchSwitch->whichChild = SO_SWITCH_NONE;
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
        if (prop == &feat->PlaneNormal || prop == &feat->FlipCut) {
            // Direction of the 45° pattern and the side the lines are lifted
            // towards both follow the plane orientation
            updateHatchGeometry();
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
        // New tessellation, so the hatching has to be sliced again
        updateHatchGeometry();
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
