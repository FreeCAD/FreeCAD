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
#include <cstring>
#include <functional>
#include <iterator>
#include <limits>
#include <vector>

#include <QAction>
#include <QMenu>
#include <QTimer>

#include <Bnd_Box.hxx>
#include <gp_Pnt.hxx>
#include <Precision.hxx>
#include <BRepBndLib.hxx>

#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/misc/SoChildList.h>
#include <Inventor/nodes/SoIndexedFaceSet.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoMaterialBinding.h>
#include <Inventor/nodes/SoPickStyle.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoLightModel.h>
#include <Inventor/nodes/SoPolygonOffset.h>
#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoTransparencyType.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoOrthographicCamera.h>
#include <Inventor/nodes/SoPerspectiveCamera.h>
#include <Gui/Inventor/Draggers/SoLinearDragger.h>
#include <Gui/Inventor/Draggers/SoRotationDragger.h>
#include <Gui/Inventor/Draggers/SoLinearDraggerGeometry.h>
#include <Gui/Inventor/Draggers/SoRotationDraggerGeometry.h>

#include <numbers>

#include <App/Application.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <Inventor/actions/SoGetBoundingBoxAction.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Converter.h>
#include <Base/ServiceProvider.h>
#include <Gui/Inventor/Draggers/GizmoStyleParameters.h>
#include <Gui/Application.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Inventor/Draggers/SoTransformDragger.h>
#include <Gui/ViewParams.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Utilities.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/ViewProvider.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Mod/Part/App/FeatureSectionAnalysis.h>

#include "SectionCapHarvest.h"
#include "SoBrepFaceSet.h"
#include "ViewProviderExt.h"
#include "ViewProviderSectionAnalysis.h"
#include "TaskSectionAnalysis.h"


using namespace PartGui;

App::PropertyFloatConstraint::Constraints ViewProviderSectionAnalysis::hatchWidthRange
    = {1.0, 64.0, 1.0};

App::Material ViewProviderSectionAnalysis::paletteColor(std::size_t index)
{
    static const float palette[][3] = {
        {0.8f, 0.3f, 0.2f},
        {0.2f, 0.5f, 0.8f},
        {0.3f, 0.7f, 0.3f},
        {0.8f, 0.7f, 0.2f},
        {0.6f, 0.3f, 0.7f},
        {0.9f, 0.5f, 0.3f},
    };
    const auto& p = palette[index % (sizeof(palette) / sizeof(palette[0]))];
    App::Material mat;
    mat.diffuseColor.set(p[0], p[1], p[2], 0.0f);
    // Section faces are never transparent, whatever the source body does
    mat.transparency = 0.0f;
    return mat;
}

App::Material ViewProviderSectionAnalysis::distinctColor(std::size_t index)
{
    // The first few come from the palette, so a two or three body section still
    // gets the colours that were chosen for it by eye.
    constexpr std::size_t paletteSize = 6;
    if (index < paletteSize) {
        return paletteColor(index);
    }

    // Beyond that, step the hue by the golden ratio. Successive indices land far
    // apart on the wheel however many there are, which is what cycling a fixed
    // table cannot do.
    constexpr double goldenRatioConjugate = 0.618033988749895;
    const double hue = std::fmod(static_cast<double>(index) * goldenRatioConjugate, 1.0);
    // Held off full saturation and brightness: fully saturated colours next to
    // each other are hard to look at across a whole assembly.
    constexpr double saturation = 0.65;
    constexpr double value = 0.85;

    const double h = hue * 6.0;
    const int sector = static_cast<int>(h) % 6;
    const double f = h - std::floor(h);
    const double p = value * (1.0 - saturation);
    const double q = value * (1.0 - saturation * f);
    const double t = value * (1.0 - saturation * (1.0 - f));

    double r = 0.0;
    double g = 0.0;
    double b = 0.0;
    switch (sector) {
        case 0:
            r = value;
            g = t;
            b = p;
            break;
        case 1:
            r = q;
            g = value;
            b = p;
            break;
        case 2:
            r = p;
            g = value;
            b = t;
            break;
        case 3:
            r = p;
            g = q;
            b = value;
            break;
        case 4:
            r = t;
            g = p;
            b = value;
            break;
        default:
            r = value;
            g = p;
            b = q;
            break;
    }

    App::Material mat;
    mat.diffuseColor.set(static_cast<float>(r), static_cast<float>(g), static_cast<float>(b), 0.0F);
    mat.transparency = 0.0F;
    return mat;
}

App::Material ViewProviderSectionAnalysis::partColor(const App::DocumentObject* part, std::size_t index)
{
    // Read here, at draw time, rather than cached with the triangles - a part's
    // colour is not a reason to walk the scene graph again.
    //
    // It does mean recolouring a part does not repaint the cap on its own:
    // ShapeAppearance lives on the view provider, and neither document's
    // signalChangedObject carries view provider properties (the Gui one says so
    // outright - its property argument is the document object's). The next
    // rebuild picks the colour up.
    if (part) {
        auto* vp = dynamic_cast<Gui::ViewProviderGeometryObject*>(
            Gui::Application::Instance->getViewProvider(part)
        );
        if (vp) {
            const auto& appearance = vp->ShapeAppearance.getValues();
            if (!appearance.empty() && !isDefaultShapeColor(appearance.front().diffuseColor)) {
                App::Material mat = appearance.front();
                // The cut face is solid whatever the part does - a transparent
                // cross-section is a hole you can see through, which is the one
                // thing a section is meant to close up.
                mat.transparency = 0.0F;
                return mat;
            }
        }
    }
    // A part the user has never coloured is still at the default, and so is
    // every other one - colouring them all identically tells nobody which part
    // is which. Those get a generated colour instead, so per-part colouring
    // means something on an assembly that arrived as one flat grey.
    return distinctColor(index);
}

bool ViewProviderSectionAnalysis::isDefaultShapeColor(const Base::Color& colour)
{
    Base::Color fallback;
    fallback.setPackedValue(static_cast<uint32_t>(Gui::ViewParams::instance()->getDefaultShapeColor()));

    // Compared loosely: the default arrives as 8 bit channels and comes back as
    // floats, so exact equality would never hold.
    constexpr float tolerance = 1.0F / 255.0F;
    return std::abs(colour.r - fallback.r) <= tolerance
        && std::abs(colour.g - fallback.g) <= tolerance
        && std::abs(colour.b - fallback.b) <= tolerance;
}

// Fade-out steps for the hatching, from crisp to nearly gone: the transparency
// of each level-of-detail step, paired with the on-screen pixels per hatch line
// at which that step takes over. Below the last one the hatching is dropped.
constexpr float hatchFadeSteps[] = {0.0f, 0.45f, 0.75f};
constexpr double hatchFadePixelsPerLine[] = {3.0, 1.5, 0.75};


SO_NODE_SOURCE(SoHatchLevelOfDetail)

void SoHatchLevelOfDetail::initClass()
{
    SO_NODE_INIT_CLASS(SoHatchLevelOfDetail, SoLevelOfDetail, "LevelOfDetail");
}

SoHatchLevelOfDetail::SoHatchLevelOfDetail()
{
    SO_NODE_CONSTRUCTOR(SoHatchLevelOfDetail);
}

void SoHatchLevelOfDetail::doAction(SoAction* action)
{
    switch (action->getCurPathCode()) {
        case SoAction::IN_PATH:
            SoGroup::doAction(action);
            return;
        case SoAction::OFF_PATH:
            return;
        default:
            break;
    }

    // Choosing a level needs a camera and a viewport, so only a render traversal
    // can do it. Everything else gets the full-detail child.
    if (!action->isOfType(SoGLRenderAction::getClassTypeId())) {
        if (getNumChildren() > 0) {
            getChildren()->traverse(action, 0);
        }
        return;
    }

    SoLevelOfDetail::doAction(action);
}

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
        "Fade the hatching out, then hide it, once the section is too small on\n"
        "screen for the individual lines to be told apart"
    );
    ADD_PROPERTY_TYPE(
        PerBodyColors,
        (false),
        "Section Analysis",
        App::Prop_None,
        "Use source body colors for cross-section faces"
    );

    ADD_PROPERTY_TYPE(
        ShowRemovedMaterial,
        (false),
        "Section Analysis",
        App::Prop_None,
        "Show the cut-away material faintly, so the section has context"
    );
    ADD_PROPERTY_TYPE(
        RemovedMaterialTransparency,
        (97),
        "Section Analysis",
        App::Prop_None,
        "How see-through the removed material is drawn, as a percentage - the\n"
        "same scale as any other object's Transparency"
    );
    ADD_PROPERTY_TYPE(
        RemovedMaterialColor,
        (Base::Color(0.5843F, 0.98823F, 0.8823F)),  /// x-ray vision sci-fi colour
        "Section Analysis",
        App::Prop_None,
        "Colour of the cut-away material"
    );

    ShapeAppearance.setValues({paletteColor(0)});
}

ViewProviderSectionAnalysis::~ViewProviderSectionAnalysis()
{
    removeClipPlane();
}

// Attach and create the visual representation of the cutting plane and hatching.
// The plane is a translucent rectangle with a solid border.
// The hatching is a set of diagonal lines (per settings in the view provider).
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
    cuttingPlane = new SoSeparator();
    cuttingPlane->setName("SectionPlaneVisual");

    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    cuttingPlane->addChild(pickStyle);

    planeHints = new SoShapeHints();
    planeHints->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    planeHints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    cuttingPlane->addChild(planeHints);

    planeMaterial = new SoMaterial();
    planeMaterial->diffuseColor.setValue(0.3f, 0.6f, 0.9f);
    planeMaterial->transparency.setValue(0.7f);
    cuttingPlane->addChild(planeMaterial);

    planeCoords = new SoCoordinate3();
    cuttingPlane->addChild(planeCoords);

    planeFaceSet = new SoFaceSet();
    planeFaceSet->numVertices.set1Value(0, 4);
    cuttingPlane->addChild(planeFaceSet);

    planeBorderMaterial = new SoMaterial();
    planeBorderMaterial->diffuseColor.setValue(0.2f, 0.4f, 0.8f);
    planeBorderMaterial->transparency.setValue(0.0f);
    cuttingPlane->addChild(planeBorderMaterial);

    auto* borderStyle = new SoDrawStyle();
    borderStyle->lineWidth.setValue(2.0f);
    cuttingPlane->addChild(borderStyle);

    planeBorderLines = new SoIndexedLineSet();
    cuttingPlane->addChild(planeBorderLines);

    // Wrap plane in a switch so we can hide it
    planeSwitch = new SoSwitch();
    planeSwitch->addChild(cuttingPlane);
    planeSwitch->whichChild = SO_SWITCH_ALL;
    pcRoot->addChild(planeSwitch);

    // Hatching 45 deg diagonal lines per ISO 128-50, built as real line geometry
    // rather than a repeating texture: the lines stay one pixel-crisp at any
    // zoom level and their width is whatever HatchLineWidth says.
    hatchRoot = new SoSeparator();
    hatchRoot->setName("SectionHatching");
    hatchRoot->renderCaching = SoSeparator::OFF;

    auto* hatchPick = new SoPickStyle();
    hatchPick->style = SoPickStyle::UNPICKABLE;
    hatchRoot->addChild(hatchPick);

    auto* hatchBind = new SoMaterialBinding();
    hatchBind->value = SoMaterialBinding::OVERALL;
    hatchRoot->addChild(hatchBind);

    // The hatch lines lie exactly on the section faces, so they z-fight with them.
    auto* hatchOffset = new SoPolygonOffset();
    hatchOffset->factor = -1.0F;
    hatchOffset->units = -1.0F;
    hatchOffset->styles = SoPolygonOffset::LINES;
    hatchOffset->on = TRUE;
    hatchRoot->addChild(hatchOffset);

    hatchStyle = new SoDrawStyle();
    hatchStyle->style = SoDrawStyle::LINES;
    hatchStyle->lineWidth.setValue(static_cast<float>(HatchLineWidth.getValue()));
    hatchRoot->addChild(hatchStyle);

    hatchCoords = new SoCoordinate3();
    hatchRoot->addChild(hatchCoords);

    hatchLines = new SoIndexedLineSet();
    hatchRoot->addChild(hatchLines);

    // Increasingly transparent copies of the same geometry, then nothing. The
    // levels differ only by material, so they share one set of coordinates and
    // one line set below pcHatchRoot.
    hatchLod = new SoHatchLevelOfDetail();
    for (float alpha : hatchFadeSteps) {
        auto* level = new SoSeparator();
        level->renderCaching = SoSeparator::OFF;
        auto* levelMat = new SoMaterial();
        levelMat->diffuseColor.setValue(0.1f, 0.1f, 0.1f);
        levelMat->transparency.setValue(alpha);
        level->addChild(levelMat);
        level->addChild(hatchRoot);
        hatchLod->addChild(level);
    }
    hatchLod->addChild(new SoGroup());

    hatchSwitch = new SoSwitch();
    hatchSwitch->addChild(hatchLod);
    hatchSwitch->whichChild = (hatchingEnabled() && Visibility.getValue()) ? SO_SWITCH_ALL
                                                                           : SO_SWITCH_NONE;
    pcRoot->addChild(hatchSwitch);

    capRoot = new SoSeparator();
    capRoot->renderCaching = SoSeparator::OFF;
    removedMaterialRoot = new SoSeparator();
    removedMaterialRoot->renderCaching = SoSeparator::OFF;
    removedMaterialSwitch = new SoSwitch();
    removedMaterialSwitch->addChild(removedMaterialRoot);
    removedMaterialSwitch->whichChild = SO_SWITCH_NONE;
    pcRoot->addChild(removedMaterialSwitch);

    capSwitch = new SoSwitch();
    capSwitch->addChild(capRoot);
    capSwitch->whichChild = Visibility.getValue() ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    pcRoot->addChild(capSwitch);

    updateHatchGeometry();
    updateCapFromScene();
    updateRemovedMaterial();
    updatePlaneVisual();
}


void ViewProviderSectionAnalysis::refreshHarvestCache()
{
    if (harvestValid) {
        return;
    }
    harvestCache.clear();

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    const std::vector<App::DocumentObject*> sources = feat->Source.getValues();

    // One entry per part, using the same recursion - and the same dedup by
    // object identity - that execute() uses to build SourceParts. Sectioning an
    // assembly that arrives as a single link used to produce exactly one body,
    // which left per-part colouring with a single part to colour.
    const std::vector<App::DocumentObject*> parts
        = Part::SectionAnalysis::distinctSourceParts(sources, feat);

    SectionCapHarvest::PartOwners owners;
    for (std::size_t i = 0; i < parts.size(); ++i) {
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(parts[i]);
        if (vp && vp->getRoot()) {
            owners.emplace(vp->getRoot(), i);
        }
    }
    if (owners.empty()) {
        harvestValid = true;
        return;
    }

    // Harvested from each source root rather than from each part's own root, so
    // the containers' placements are still in the accumulated transform. The
    // split back out to parts happens inside the walk.
    std::vector<Part::SectionCap::TriangleSoup> soups(parts.size());
    for (App::DocumentObject* src : sources) {
        if (!src || src == feat) {
            continue;
        }
        Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(src);
        if (!vp) {
            continue;
        }
        SectionCapHarvest::fromSceneGraph(vp->getRoot(), owners, soups);
    }

    for (std::size_t i = 0; i < soups.size(); ++i) {
        auto& soup = soups[i];
        if (soup.indices.empty()) {
            continue;
        }
        HarvestedBody body;
        body.source = parts[i];
        body.bounds = Base::BoundBox3d(soup.points.data(), soup.points.size());
        body.soup = std::move(soup);
        harvestCache.push_back(std::move(body));
    }

    harvestValid = true;
}

void ViewProviderSectionAnalysis::updateRemovedMaterialPlane()
{
    if (!removedMaterialClip) {
        return;
    }

    auto* feat = getObject<Part::SectionAnalysis>();
    Base::Vector3d n;
    double d = 0.0;
    if (!feat || !feat->cutPlane(n, d)) {
        return;
    }

    // World coordinates, so the plane needs no per object transform. Keeping
    // the half the section throws away means not negating the normal.
    removedMaterialClip->plane.setValue(SbPlane(
        SbVec3f(static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z)),
        SbVec3f(static_cast<float>(n.x * d), static_cast<float>(n.y * d), static_cast<float>(n.z * d))
    ));
}


void ViewProviderSectionAnalysis::updateRemovedMaterialAppearance()
{
    if (!removedMaterialAppearance) {
        return;
    }

    removedMaterialAppearance->diffuseColor.setValue(
        Base::convertTo<SbColor>(RemovedMaterialColor.getValue())
    );
    removedMaterialAppearance->transparency.setValue(
        static_cast<float>(RemovedMaterialTransparency.getValue()) / 100.0F
    );
}


void ViewProviderSectionAnalysis::updateRemovedMaterial()
{
    if (!removedMaterialRoot || !removedMaterialSwitch) {
        return;
    }
    removedMaterialRoot->removeAllChildren();
    removedMaterialClip = nullptr;
    removedMaterialAppearance = nullptr;
    removedMaterialSwitch->whichChild = SO_SWITCH_NONE;

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || !ShowRemovedMaterial.getValue() || !Visibility.getValue()) {
        return;
    }

    Base::Vector3d n;
    double d = 0.0;
    if (!feat->cutPlane(n, d)) {
        return;
    }

    // Built from our own harvested triangles rather than by referencing each
    // object's scene graph node.
    //
    // Referencing was free, but it gave that node a second parent. Coin caches a
    // shape's primitives per node rather than per path, and an assembly is full
    // of App::Links that already share nodes, so the cache could be validated
    // under one traversal and used under another. That asserted inside
    // SoIndexedFaceSet::generatePrimitives during a selection render.
    //
    // The soup is already in world coordinates, so this also does away with
    // re-deriving each object's placement - which was guesswork for anything
    // nested.
    refreshHarvestCache();

    // A second copy of an assembly's triangles is hundreds of megabytes, so
    // there is a point past which the removed material is not worth its cost.
    constexpr std::size_t maxRemovedMaterialTriangles = 10000000;
    std::size_t totalTriangles = 0;
    for (const HarvestedBody& body : harvestCache) {
        totalTriangles += body.soup.indices.size() / 3;
    }
    if (totalTriangles > maxRemovedMaterialTriangles) {
        Base::Console().warning(
            "SectionAnalysis: not drawing the cut-away removed material, %zu triangles is too "
            "many to copy for a visual aid.\n",
            totalTriangles
        );
        return;
    }

    // Every body is drawn identically and against the same plane, so all of this
    // sits above them rather than being repeated per body. That is what leaves a
    // plane move with exactly one field to write - see updateRemovedMaterialPlane().
    // Rebuilding these nodes on every drag step meant re-copying the whole
    // assembly to say nothing more than "the plane moved".

    // Scenery: clicking through to the real object matters more than being
    // able to select a hint.
    auto* pickStyle = new SoPickStyle();
    pickStyle->style = SoPickStyle::UNPICKABLE;
    removedMaterialRoot->addChild(pickStyle);

    // The viewer sorts transparency per triangle, which for a second copy of the
    // assembly costs more than everything else here together. One flat unlit
    // colour has no draw order to get wrong, so the sort buys nothing.
    auto* transparencyType = new SoTransparencyType();
    transparencyType->value = SoTransparencyType::DELAYED_BLEND;
    removedMaterialRoot->addChild(transparencyType);

    removedMaterialClip = new SoClipPlane();
    removedMaterialClip->on.setValue(TRUE);
    removedMaterialRoot->addChild(removedMaterialClip);
    updateRemovedMaterialPlane();

    // Drawn the way the rest of FreeCAD draws a preview: one flat colour,
    // unlit and translucent, so it reads as an overlay rather than as a
    // second, dimmer model competing with the real one.
    auto* lightModel = new SoLightModel();
    lightModel->model = SoLightModel::BASE_COLOR;
    removedMaterialRoot->addChild(lightModel);

    auto* polygonOffset = new SoPolygonOffset();
    polygonOffset->factor = 1.0F;
    polygonOffset->units = 1.0F;
    polygonOffset->on = TRUE;
    polygonOffset->styles = SoPolygonOffset::FILLED;
    removedMaterialRoot->addChild(polygonOffset);

    removedMaterialAppearance = new SoMaterial();
    removedMaterialRoot->addChild(removedMaterialAppearance);
    updateRemovedMaterialAppearance();

    auto* hints = new SoShapeHints();
    hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
    hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
    removedMaterialRoot->addChild(hints);

    std::size_t bodiesDrawn = 0;
    for (const HarvestedBody& body : harvestCache) {
        if (body.soup.indices.empty()) {
            continue;
        }

        auto* group = new SoSeparator();
        group->renderCaching = SoSeparator::OFF;

        std::vector<SbVec3f> points;
        points.reserve(body.soup.points.size());
        for (const auto& p : body.soup.points) {
            points.emplace_back(
                static_cast<float>(p.x),
                static_cast<float>(p.y),
                static_cast<float>(p.z)
            );
        }
        auto* coordinates = new SoCoordinate3();
        coordinates->point.setNum(static_cast<int>(points.size()));
        coordinates->point.setValues(0, static_cast<int>(points.size()), points.data());
        group->addChild(coordinates);

        std::vector<int32_t> faceIndex;
        faceIndex.reserve(body.soup.indices.size() / 3 * 4);
        for (std::size_t i = 0; i + 2 < body.soup.indices.size(); i += 3) {
            faceIndex.push_back(body.soup.indices[i]);
            faceIndex.push_back(body.soup.indices[i + 1]);
            faceIndex.push_back(body.soup.indices[i + 2]);
            faceIndex.push_back(SO_END_FACE_INDEX);
        }
        auto* faces = new SoIndexedFaceSet();
        faces->coordIndex.setNum(static_cast<int>(faceIndex.size()));
        faces->coordIndex.setValues(0, static_cast<int>(faceIndex.size()), faceIndex.data());
        group->addChild(faces);

        removedMaterialRoot->addChild(group);
        ++bodiesDrawn;
    }

    // Counted rather than read off the child count, which is never zero now that
    // the shared state nodes are added before the bodies are.
    if (bodiesDrawn > 0) {
        removedMaterialSwitch->whichChild = SO_SWITCH_ALL;
    }
}


void ViewProviderSectionAnalysis::releaseHarvestCache()
{
    harvestCache.clear();
    harvestCache.shrink_to_fit();  // clear() alone keeps the capacity
    harvestValid = false;
}


void ViewProviderSectionAnalysis::updateCapFromScene()
{
    if (!capRoot) {
        return;
    }
    capRoot->removeAllChildren();

    auto* feat = getObject<Part::SectionAnalysis>();
    // In Geometry mode the cap is real B-rep faces drawn the usual way, and
    // drawing it twice would only z-fight with itself.
    if (!feat || feat->wantsSolidGeometry()) {
        // Geometry mode draws real B-rep faces, so the harvested triangles are
        // dead weight until the user comes back to Display.
        releaseHarvestCache();
        return;
    }

    if (capSwitch) {
        capSwitch->whichChild = Visibility.getValue() ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }

    Base::Vector3d n;
    double d = 0.0;
    if (!feat->cutPlane(n, d)) {
        return;
    }

    Base::Vector3d u;
    Base::Vector3d v;
    Part::SectionAnalysis::planeFrame(n, u, v);

    const double spacing = HatchSpacing.getValue();
    const bool wantHatch = hatchingEnabled() && std::isfinite(spacing) && spacing >= minHatchSpacing;

    // Deliberately far looser than Precision::Confusion(). Coin holds vertices
    // as float, so points on a half metre part agree only to about 1e-4 mm;
    // chaining at OCCT's 1e-7 would leave every tessellation seam unjoined.
    constexpr double chainTolerance = 1e-3;

    // Walking the scene graph is most of the cost and does not depend on the
    // plane, so it is done once and kept.
    //
    // The removedMaterial is deliberately not rebuilt from here. It is built from the same
    // harvest and the callers that invalidate one invalidate the other, so doing
    // it here only meant doing it twice per plane move - once here and once from
    // updateData(), which already asks for both.
    refreshHarvestCache();

    // Indexed rather than range-for: index picks the colour and the hatch angle,
    // and has to advance even for bodies the plane misses, so a part keeps its
    // colour whether or not it is currently being cut.
    for (std::size_t index = 0; index < harvestCache.size(); ++index) {
        const HarvestedBody& body = harvestCache[index];
        // Reject on the bounding box first. On an assembly this is the
        // difference between visiting every triangle and visiting almost none,
        // which only holds because the box was measured at harvest time.
        double lo = 0.0;
        double hi = 0.0;
        if (!Part::SectionCap::extentAlong(body.bounds, n, lo, hi) || d < lo || d > hi) {
            continue;
        }

        const auto segments = Part::SectionCap::sliceTriangles(body.soup, n, d);
        if (segments.empty()) {
            continue;
        }

        const auto loops = Part::SectionCap::chainLoops(segments, chainTolerance);
        if (loops.empty()) {
            continue;
        }

        // The cap surface itself. Without it the section is see-through and you
        // look into the inside of the body that was just cut.
        auto* node = new SoSeparator();
        node->renderCaching = SoSeparator::OFF;

        // Per-part colouring takes each part's own colour, so the section reads
        // like the model it was cut from; otherwise the whole cap follows the
        // section's own appearance, so the colour property still means something
        // in Display mode.
        const auto& appearance = ShapeAppearance.getValues();
        const App::Material colour = (perSolidColors() || appearance.empty())
            ? partColor(body.source, index)
            : appearance.front();

        const auto fill = Part::SectionCap::fillLoops(loops, u, v);
        if (!fill.indices.empty()) {
            std::vector<SbVec3f> fillPoints;
            fillPoints.reserve(fill.points.size());
            for (const auto& p : fill.points) {
                fillPoints.emplace_back(
                    static_cast<float>(p.x),
                    static_cast<float>(p.y),
                    static_cast<float>(p.z)
                );
            }
            std::vector<int32_t> fillIndex;
            fillIndex.reserve(fill.indices.size() / 3 * 4);
            for (std::size_t i = 0; i + 2 < fill.indices.size(); i += 3) {
                fillIndex.push_back(fill.indices[i]);
                fillIndex.push_back(fill.indices[i + 1]);
                fillIndex.push_back(fill.indices[i + 2]);
                fillIndex.push_back(SO_END_FACE_INDEX);
            }

            auto* fillGroup = new SoSeparator();
            fillGroup->renderCaching = SoSeparator::OFF;

            // The fill, the outline and the hatching all lie exactly on the
            // cutting plane, because that is where the section is. Coplanar
            // filled polygons and lines z-fight, and the fill wins about as
            // often as it loses - which showed up as a section drawn as a flat
            // black shape with its hatching and outline missing.
            //
            // The fill is pushed back in depth rather than the lines being
            // lifted along the normal. A lift has to be scaled to the model to
            // be neither visible nor swallowed by depth precision; the offset is
            // applied in depth buffer units and so needs no such guess.
            auto* fillOffset = new SoPolygonOffset();
            fillOffset->factor = 1.0F;
            fillOffset->units = 1.0F;
            fillOffset->on = TRUE;
            fillOffset->styles = SoPolygonOffset::FILLED;
            fillGroup->addChild(fillOffset);

            // Two sided: which way the cap faces depends on the cut direction,
            // and a back facing cap would simply vanish.
            auto* hints = new SoShapeHints();
            hints->vertexOrdering = SoShapeHints::UNKNOWN_ORDERING;
            hints->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;
            fillGroup->addChild(hints);
            // The part's own colour, darkened, rather than a flat black. Black
            // made every cap look identical however the parts were coloured, so
            // per-part colouring only ever showed in the hatch lines - which is
            // to say it barely showed at all. Darkened so those lines, drawn in
            // the full colour on top, still read against it.
            constexpr float fillDarkening = 0.35F;
            auto* fillMaterial = new SoMaterial();
            fillMaterial->diffuseColor.setValue(
                colour.diffuseColor.r * fillDarkening,
                colour.diffuseColor.g * fillDarkening,
                colour.diffuseColor.b * fillDarkening
            );
            fillGroup->addChild(fillMaterial);
            auto* fillCoords = new SoCoordinate3();
            fillCoords->point.setNum(static_cast<int>(fillPoints.size()));
            fillCoords->point.setValues(0, static_cast<int>(fillPoints.size()), fillPoints.data());
            fillGroup->addChild(fillCoords);
            auto* faces = new SoIndexedFaceSet();
            faces->coordIndex.setNum(static_cast<int>(fillIndex.size()));
            faces->coordIndex.setValues(0, static_cast<int>(fillIndex.size()), fillIndex.data());
            fillGroup->addChild(faces);
            node->addChild(fillGroup);
        }

        std::vector<SbVec3f> points;
        std::vector<int32_t> lineIndex;

        // Outline first, so the cross section reads as a shape even before the
        // hatching is taken in.
        for (const auto& loop : loops) {
            for (const auto& p : loop) {
                lineIndex.push_back(static_cast<int32_t>(points.size()));
                points.emplace_back(
                    static_cast<float>(p.x),
                    static_cast<float>(p.y),
                    static_cast<float>(p.z)
                );
            }
            if (Part::SectionCap::isClosed(loop, chainTolerance)) {
                lineIndex.push_back(lineIndex[lineIndex.size() - loop.size()]);
            }
            lineIndex.push_back(SO_END_LINE_INDEX);
        }

        if (wantHatch) {
            // A different angle per body, so neighbouring parts stay legible
            // where their sections meet.
            const double angle = std::numbers::pi / 4.0 + (std::numbers::pi / 6.0) * (index % 6);
            // Hatching the cap triangles rather than the loops, so both result
            // modes go through one implementation.
            const Base::Vector3d levelDir = u * -std::sin(angle) + v * std::cos(angle);
            const auto hatch = Part::SectionCap::hatchTriangles(fill, levelDir, spacing);
            for (const auto& seg : hatch) {
                lineIndex.push_back(static_cast<int32_t>(points.size()));
                points.emplace_back(
                    static_cast<float>(seg.start.x),
                    static_cast<float>(seg.start.y),
                    static_cast<float>(seg.start.z)
                );
                lineIndex.push_back(static_cast<int32_t>(points.size()));
                points.emplace_back(
                    static_cast<float>(seg.end.x),
                    static_cast<float>(seg.end.y),
                    static_cast<float>(seg.end.z)
                );
                lineIndex.push_back(SO_END_LINE_INDEX);
            }
        }

        if (points.empty()) {
            continue;
        }

        // Same colour the fill was darkened from, at full strength, so the
        // outline and hatching read as belonging to that cap.
        auto* material = new SoMaterial();
        material->diffuseColor
            .setValue(colour.diffuseColor.r, colour.diffuseColor.g, colour.diffuseColor.b);
        material->emissiveColor
            .setValue(colour.diffuseColor.r, colour.diffuseColor.g, colour.diffuseColor.b);
        node->addChild(material);

        auto* style = new SoDrawStyle();
        style->lineWidth = static_cast<float>(HatchLineWidth.getValue());
        node->addChild(style);

        auto* coords = new SoCoordinate3();
        coords->point.setNum(static_cast<int>(points.size()));
        coords->point.setValues(0, static_cast<int>(points.size()), points.data());
        node->addChild(coords);

        auto* lines = new SoIndexedLineSet();
        lines->coordIndex.setNum(static_cast<int>(lineIndex.size()));
        lines->coordIndex.setValues(0, static_cast<int>(lineIndex.size()), lineIndex.data());
        node->addChild(lines);

        capRoot->addChild(node);
    }
}

void ViewProviderSectionAnalysis::finishRestoring()
{
    ViewProviderPart::finishRestoring();

    // After document restore the scene graph is fully built it is safe to set up
    // clip planes, plane visual and hatching(s)
    if (Visibility.getValue()) {
        installClipPlane();

        // attach() already tried this, but it runs before the source view
        // providers have their scene graphs, so it harvested an empty walk.
        // This is the first moment there is anything to slice.
        updateCapFromScene();
        updateRemovedMaterial();
    }
    updatePlaneVisual();
    if (perSolidColors()) {
        applyPerSolidColors();
    }
    applyHatching();
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
    if (targets.empty()) {
        targets = feat->Source.getValues();
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
        clippedObjects.push_back(ClippedObject {obj, node});
    }
    clipInstalled = !clippedObjects.empty();
    updateClipPlaneEquation();
}

void ViewProviderSectionAnalysis::removeClipPlane()
{
    for (const ClippedObject& clipped : clippedObjects) {
        if (!clipped.node) {
            continue;
        }
        if (auto* vp = Gui::Application::Instance->getViewProvider(clipped.object)) {
            if (auto* root = dynamic_cast<SoSeparator*>(vp->getRoot())) {
                int idx = root->findChild(clipped.node);
                if (idx >= 0) {
                    root->removeChild(idx);
                }
            }
        }
        clipped.node->unref();
    }
    clippedObjects.clear();
    clipInstalled = false;
}

void ViewProviderSectionAnalysis::updateClipPlaneEquation()
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || clippedObjects.empty()) {
        return;
    }

    Base::Vector3d n;
    double d = 0.0;
    if (!feat->cutPlane(n, d)) {
        return;
    }

    // The cutting plane passes through point (n * d) with normal vect n.
    // OCCT keeps the negative normal side (where n.p < d).
    // SoClipPlane keeps the positive normal half space, so we negate the
    // normal to keep the same side as OCCT.
    //
    // Nudge past the cut plane face, so the body's surface does not z-fight the cap.
    // Which would cause visual flicker.
    // Not an OCCT tolerance: those are for doubles, and Coin holds vertices as float.
    // Precision::Confusion here would round away to nothing.
    constexpr double clipEps = 0.01;

    const double dClip = d - clipEps;

    const Base::Vector3d gNormal(-n.x, -n.y, -n.z);                      // global half-space normal
    const Base::Vector3d gPoint(n.x * dClip, n.y * dClip, n.z * dClip);  // global point on plane

    // Each node lives inside its object's view provider, so its plane must be
    // expressed in that object's local frame: transform the global plane by the
    // inverse of the object's global placement. Objects with identity placement
    // get the global plane unchanged.
    for (const ClippedObject& clipped : clippedObjects) {
        if (!clipped.node) {
            continue;
        }
        App::DocumentObject* obj = clipped.object;

        Base::Vector3d localNormal = gNormal;
        Base::Vector3d localPoint = gPoint;
        if (obj && obj->isDerivedFrom(App::GeoFeature::getClassTypeId())) {
            const Base::Placement inv = App::GeoFeature::getGlobalPlacement(obj).inverse();
            inv.getRotation().multVec(gNormal, localNormal);  // rotate the normal only
            inv.multVec(gPoint, localPoint);                  // full transform for the point
        }

        clipped.node->plane.setValue(SbPlane(
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
        clipped.node->on.setValue(TRUE);
    }
}

void ViewProviderSectionAnalysis::slotChangedObject(
    const App::DocumentObject& obj,
    const App::Property& prop
)
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat || &obj == feat) {
        return;
    }
    // Appearing and disappearing changes which triangles exist, being edited
    // changes what they are, being moved changes where they are. Everything
    // else leaves the harvest usable.
    const bool visibilityChanged = &prop == &obj.Visibility;
    if (!Part::SectionAnalysis::isHarvestStaleAfter(obj, prop)) {
        return;
    }
    if (feat->getDocument()->testStatus(App::Document::Restoring)
        || feat->getDocument()->testStatus(App::Document::Recomputing)) {
        return;
    }

    // Figure out if there's a dependency, so that we dont cut hidden objects
    auto isAncestorOrSelf = [](const App::DocumentObject* a, const App::DocumentObject* b) {
        for (const auto* o = b; o; o = App::GeoFeatureGroupExtension::getGroupOfObject(o)) {
            if (o == a) {
                return true;
            }
        }
        return false;
    };
    const auto& sources = feat->Source.getValues();
    const bool related = std::any_of(sources.begin(), sources.end(), [&](const auto* src) {
        return isAncestorOrSelf(src, &obj) || isAncestorOrSelf(&obj, src);
    });
    if (!related) {
        return;
    }

    // Only now is it known the change was to geometry the cap is built from.
    harvestValid = false;

    // Geometry edits do not move the plane, so there is nothing for the feature
    // to recompute - the cap just has to be rebuilt from the new triangles. The
    // removed material is built from the same harvest that was just invalidated, so it goes
    // stale on exactly the same edits.
    if (!visibilityChanged) {
        updateCapFromScene();
        updateRemovedMaterial();
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
    // Resolving the whole source shape is the expensive part on large
    // assemblies, which is exactly why the result is cached here rather than
    // recomputed on every plane move.
    Bnd_Box bbox;
    if (!feat->sourceBoundingBox(bbox)) {
        return;
    }

    // sourceBoundingBox returned true, so the box is not void and the corners
    // are safe to ask for.
    const gp_Pnt lo = bbox.CornerMin();
    const gp_Pnt hi = bbox.CornerMax();
    sourceBBox = Base::BoundBox3d(lo.X(), lo.Y(), lo.Z(), hi.X(), hi.Y(), hi.Z());
    sourceBBoxValid = true;
}

bool ViewProviderSectionAnalysis::sourceBounds(Base::Vector3d& centre, double& diagonal)
{
    if (!sourceBBoxValid) {
        refreshSourceBBoxCache();
    }
    if (!sourceBBoxValid) {
        return false;
    }

    centre = sourceBBox.GetCenter();
    diagonal = sourceBBox.CalcDiagonalLength();
    return true;
}

Base::BoundBox3d ViewProviderSectionAnalysis::_getBoundingBox(
    const char* subname,
    const Base::Matrix4D* mat,
    bool transform,
    const Gui::View3DInventorViewer* view,
    int depth
) const
{
    Base::BoundBox3d box = ViewProviderPart::_getBoundingBox(subname, mat, transform, view, depth);

    // In Display mode the Shape is empty and the cap is harvested triangles, so
    // the base class walk of the mode switch finds nothing to measure.
    if (!capRoot) {
        return box;
    }

    // capRoot holds plain coordinate geometry, so the viewport plays no part.
    SoGetBoundingBoxAction action((SbViewportRegion()));
    action.apply(capRoot);
    const SbBox3f capBox = action.getBoundingBox();
    if (capBox.isEmpty()) {
        return box;
    }

    const SbVec3f low = capBox.getMin();
    const SbVec3f high = capBox.getMax();
    Base::BoundBox3d capBounds(low[0], low[1], low[2], high[0], high[1], high[2]);

    // Measured under capRoot, which sits below pcTransform in the graph, so the
    // placement still has to be applied by hand.
    if (transform) {
        if (const auto* geo = dynamic_cast<const App::GeoFeature*>(getObject())) {
            capBounds = capBounds.Transformed(geo->Placement.getValue().toMatrix());
        }
    }

    box.Add(capBounds);
    return box;
}

void ViewProviderSectionAnalysis::updatePlaneVisual()
{
    if (!planeCoords || !planeFaceSet) {
        return;
    }

    // Default to hidden and will be enabled if we successfully compute coordinates
    planeFaceSet->numVertices.set1Value(0, 0);
    planeBorderLines->coordIndex.setNum(0);  // clear border indices too

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
    const double xmin = sourceBBox.MinX;
    const double ymin = sourceBBox.MinY;
    const double zmin = sourceBBox.MinZ;
    const double xmax = sourceBBox.MaxX;
    const double ymax = sourceBBox.MaxY;
    const double zmax = sourceBBox.MaxZ;

    Base::Vector3d n = feat->PlaneNormal.getValue();
    double d = feat->PlaneOffset.getValue();
    double len = n.Length();
    if (len < Precision::Confusion()) {
        return;
    }
    n = n / len;

    // Build orthonormal frame on the cutting plane
    Base::Vector3d u, v;
    Part::SectionAnalysis::planeFrame(n, u, v);

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

    planeCoords->point.set1Value(0, SbVec3f(p0.x, p0.y, p0.z));
    planeCoords->point.set1Value(1, SbVec3f(p1.x, p1.y, p1.z));
    planeCoords->point.set1Value(2, SbVec3f(p2.x, p2.y, p2.z));
    planeCoords->point.set1Value(3, SbVec3f(p3.x, p3.y, p3.z));

    // Now safe to render the quad and border
    planeFaceSet->numVertices.set1Value(0, 4);
    static const int32_t borderIndices[] = {0, 1, 2, 3, 0, -1};
    planeBorderLines->coordIndex.setValues(0, 6, borderIndices);
}

void ViewProviderSectionAnalysis::updateHatchGeometry()
{
    if (!hatchCoords || !hatchLines) {
        return;
    }

    hatchLines->coordIndex.setNum(0);
    hatchCoords->point.setNum(0);

    auto* feat = getObject<Part::SectionAnalysis>();
    if (!hatchingEnabled() || !feat) {
        return;
    }

    Base::Vector3d n = feat->PlaneNormal.getValue();
    const double len = n.Length();
    if (len < Precision::Confusion()) {
        return;
    }
    n = n / len;

    // Build orthonormal frame on the cutting plane (same convention as the
    // plane visual, so hatching and plane quad stay in step)
    Base::Vector3d u, v;
    Part::SectionAnalysis::planeFrame(n, u, v);

    // Hatch lines run at 45 deg in that frame; `levelDir` is their in-plane normal,
    // so a hatch line is the set of points with p * levelDir == k * spacing.
    Base::Vector3d levelDir = u - v;
    levelDir.Normalize();

    // PropertyLength only clamps to >= 0 through the editor, and setValue() from
    // C++ or a restored file bypasses even that, so anything can land here
    const double spacing = HatchSpacing.getValue();
    if (!std::isfinite(spacing) || spacing < minHatchSpacing) {
        return;
    }

    // The triangles the viewer already renders, so the hatching is trimmed to
    // exactly the same outline as the visible faces.
    const int numPts = coords->point.getNum();
    const int numIdx = faceset->coordIndex.getNum();
    if (numPts < 3 || numIdx < 4) {
        return;
    }
    const SbVec3f* pts = coords->point.getValues(0);
    const int32_t* cidx = faceset->coordIndex.getValues(0);

    // SoBrepFaceSet stores triangles as {i0, i1, i2, -1}, but parse generically
    // (fan-triangulating any polygon) so this survives a different tesselation.
    Part::SectionCap::TriangleSoup cap;
    cap.points.reserve(static_cast<std::size_t>(numPts));
    for (int i = 0; i < numPts; ++i) {
        cap.points.emplace_back(pts[i][0], pts[i][1], pts[i][2]);
    }
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
        for (std::size_t j = 2; j < poly.size(); ++j) {
            cap.indices.push_back(poly[0]);
            cap.indices.push_back(poly[j - 1]);
            cap.indices.push_back(poly[j]);
        }
        poly.clear();
    }

    // How far the region reaches across the lines, i.e. how many lines there
    // are - the fade thresholds below are derived from it.
    double spanMin = std::numeric_limits<double>::max();
    double spanMax = std::numeric_limits<double>::lowest();
    for (const auto& point : cap.points) {
        const double level = point * levelDir;
        spanMin = std::min(spanMin, level);
        spanMax = std::max(spanMax, level);
    }

    constexpr std::size_t maxSegments = 500000;
    const auto hatch = Part::SectionCap::hatchTriangles(cap, levelDir, spacing, maxSegments);

    std::vector<SbVec3f> segPts;
    std::vector<int32_t> segIdx;
    segPts.reserve(hatch.size() * 2);
    segIdx.reserve(hatch.size() * 3);
    for (const auto& seg : hatch) {
        const auto base = static_cast<int32_t>(segPts.size());
        segPts.emplace_back(
            static_cast<float>(seg.start.x),
            static_cast<float>(seg.start.y),
            static_cast<float>(seg.start.z)
        );
        segPts.emplace_back(
            static_cast<float>(seg.end.x),
            static_cast<float>(seg.end.y),
            static_cast<float>(seg.end.z)
        );
        segIdx.push_back(base);
        segIdx.push_back(base + 1);
        segIdx.push_back(SO_END_LINE_INDEX);
    }

    if (segPts.empty()) {
        return;
    }

    // Screen areas at which each fade step takes over: the region holds
    // `span / spacing` lines and each needs a few pixels to read as a line, so
    // the thresholds grow with the line count rather than being a fixed size.
    // An empty screenArea makes the node always pick the first child.
    if (hatchLod) {
        hatchLod->screenArea.setNum(0);
        if (AutoHideHatching.getValue() && spanMax > spanMin) {
            const double lines = (spanMax - spanMin) / spacing;
            float thresholds[std::size(hatchFadePixelsPerLine)];
            for (size_t i = 0; i < std::size(hatchFadePixelsPerLine); ++i) {
                const double px = hatchFadePixelsPerLine[i] * lines;
                thresholds[i] = static_cast<float>(px * px);
            }
            hatchLod->screenArea.setValues(0, static_cast<int>(std::size(thresholds)), thresholds);
        }
    }

    hatchCoords->point.setValues(0, static_cast<int>(segPts.size()), segPts.data());
    hatchLines->coordIndex.setValues(0, static_cast<int>(segIdx.size()), segIdx.data());
}


void ViewProviderSectionAnalysis::applyPerSolidColors()
{
    auto* feat = getObject<Part::SectionAnalysis>();
    if (!feat) {
        return;
    }

    // Authoritative face-to-source mapping computed in execute():
    // Each faceIdx[i] = index into parts of the object that produced face i.
    // An object contributing several solids appears once in parts, so all of
    // its faces get the same colour.
    const auto& faceIdx = feat->FaceSourceIndex.getValues();
    const auto& parts = feat->SourceParts.getValues();
    if (faceIdx.empty()) {
        return;
    }

    // For single part - nothing to do here. Bail early.
    if (parts.size() < 2) {
        return;
    }

    std::vector<App::Material> partMats;
    partMats.reserve(parts.size());
    for (size_t i = 0; i < parts.size(); i++) {
        App::Material mat = distinctColor(i);
        if (parts[i]) {
            auto* vp = Gui::Application::Instance->getViewProvider(parts[i]);
            if (auto* vpPart = dynamic_cast<ViewProviderPartExt*>(vp)) {
                mat = vpPart->ShapeAppearance[0];
            }
        }
        // Force fully opaque - section faces should never be transparent
        mat.transparency = 0.0f;
        mat.diffuseColor.a = 0.0f;
        partMats.push_back(mat);
    }

    // One material per face, in the same order as the faces of the Shape
    std::vector<App::Material> materials;
    materials.reserve(faceIdx.size());
    for (size_t i = 0; i < faceIdx.size(); i++) {
        const long pi = faceIdx[i];
        materials.push_back(
            (pi >= 0 && pi < static_cast<long>(partMats.size())) ? partMats[pi] : distinctColor(i)
        );
    }

    ShapeAppearance.setValues(materials);
}

void ViewProviderSectionAnalysis::setHatching(bool on)
{
    ShowHatching.setValue(on);
}

void ViewProviderSectionAnalysis::applyHatching()
{
    // The hatching hangs off pcRoot rather than the display-mode switch, so it
    // has to follow the object's visibility explicitly
    if (hatchSwitch) {
        hatchSwitch->whichChild = (hatchingEnabled() && Visibility.getValue()) ? SO_SWITCH_ALL
                                                                               : SO_SWITCH_NONE;
    }
    updateHatchGeometry();
}

void ViewProviderSectionAnalysis::applySectionColors()
{
    if (perSolidColors()) {
        applyPerSolidColors();
        return;
    }

    // Back to one colour for the whole section. updateHatchGeometry() is a no-op
    // when hatching is off, so it needs no guard of its own.
    ShapeAppearance.setValues({paletteColor(0)});
    updateHatchGeometry();
}

void ViewProviderSectionAnalysis::onChanged(const App::Property* prop)
{
    if (prop == &HatchLineWidth) {
        if (hatchStyle) {
            hatchStyle->lineWidth.setValue(static_cast<float>(HatchLineWidth.getValue()));
        }
    }
    else if (prop == &HatchSpacing || prop == &AutoHideHatching) {
        updateHatchGeometry();
    }
    else if (prop == &ShowHatching) {
        applyHatching();
    }
    else if (prop == &PerBodyColors) {
        // Skipped while restoring: the colours are reapplied by finishRestoring()
        // once ShapeAppearance is back, and acting here could overwrite it
        // depending on property order.
        if (!isRestoring()) {
            applySectionColors();
        }
    }

    // The scene graph cap carries its own hatching and colour, so every one of
    // these has to reach it too - none of them go through the section Shape.
    if (prop == &HatchLineWidth || prop == &HatchSpacing || prop == &ShowHatching
        || prop == &PerBodyColors || prop == &ShapeAppearance) {
        if (!isRestoring()) {
            updateCapFromScene();
        }
    }
    if (prop == &ShowRemovedMaterial) {
        if (!isRestoring()) {
            updateRemovedMaterial();
        }
    }
    // Appearance only. Rebuilding an assembly's triangles to change a float is
    // what made dragging the transparency slider feel like the section was slow.
    if (prop == &RemovedMaterialTransparency || prop == &RemovedMaterialColor) {
        if (!isRestoring()) {
            updateRemovedMaterialAppearance();
        }
    }

    ViewProviderPart::onChanged(prop);
}

void ViewProviderSectionAnalysis::setShowPlane(bool on)
{
    if (planeSwitch) {
        planeSwitch->whichChild = on ? SO_SWITCH_ALL : SO_SWITCH_NONE;
    }
}

void ViewProviderSectionAnalysis::setPerSolidColors(bool on)
{
    PerBodyColors.setValue(on);
}

void ViewProviderSectionAnalysis::setEditViewer(Gui::View3DInventorViewer* viewer, int ModNum)
{
    // The task panel owns the handles now, and the base class is what puts the
    // gizmo container into the viewer and keeps it scaled to the camera.
    ViewProviderDragger::setEditViewer(viewer, ModNum);
}

void ViewProviderSectionAnalysis::unsetEditViewer(Gui::View3DInventorViewer* viewer)
{
    ViewProviderDragger::unsetEditViewer(viewer);
}

void ViewProviderSectionAnalysis::show()
{
    installClipPlane();
    updatePlaneVisual();
    if (perSolidColors()) {
        applyPerSolidColors();
    }
    // Plane visual hidden by default - shown when editing via task panel
    if (planeSwitch) {
        planeSwitch->whichChild = SO_SWITCH_NONE;
    }
    if (capSwitch) {
        capSwitch->whichChild = SO_SWITCH_ALL;
    }
    updateCapFromScene();
    // The removed material draws nothing while hidden, so coming back has to build it
    updateRemovedMaterial();
    ViewProviderPart::show();

    // After the base class, which rebuilds the tessellation the hatching is
    // sliced from when it went stale while hidden
    applyHatching();
}

void ViewProviderSectionAnalysis::hide()
{
    removeClipPlane();
    if (planeSwitch) {
        planeSwitch->whichChild = SO_SWITCH_NONE;
    }
    if (hatchSwitch) {
        hatchSwitch->whichChild = SO_SWITCH_NONE;
    }
    if (capSwitch) {
        capSwitch->whichChild = SO_SWITCH_NONE;
    }
    // Hiding the section has to hide the hint that goes with it. updateRemovedMaterial()
    // already refuses to draw while invisible, but nothing was calling it here,
    // so the cut-away material stayed on screen after the section went away.
    if (removedMaterialSwitch) {
        removedMaterialSwitch->whichChild = SO_SWITCH_NONE;
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
            // The direction of the 45 deg pattern follows the plane orientation.
            // FlipCut no longer changes anything here - the lines used to be
            // lifted towards the cut-away side, and are now offset in the depth
            // buffer instead - so that half of the condition is redundant.
            updateHatchGeometry();
        }
        // The scene graph cap is re-sliced for any plane change, offset
        // included: unlike the OCCT path there is no shape to wait for.
        updateCapFromScene();
        // The removed material's triangles are the same ones whichever way the plane
        // points, so only the plane it is clipped against has moved. This runs
        // on every drag step; rebuilding the geometry here meant re-copying the
        // whole assembly to say nothing more than that.
        updateRemovedMaterialPlane();
    }

    // The clipped set follows SourceParts, which execute() rebuilds
    if (prop == &feat->Source || prop == &feat->SourceParts) {
        const bool harvestStale = feat->invalidatesHarvest(*prop);
        if (harvestStale) {
            harvestValid = false;
        }
        removeClipPlane();
        if (Visibility.getValue()) {
            installClipPlane();
        }
        refreshSourceBBoxCache();
        updatePlaneVisual();
        // Not left to the Shape branch below. In Display mode execute() leaves
        // Shape null, so sectioning a different set of objects need not change
        // it at all - and then nothing would rebuild what is drawn.
        if (harvestStale) {
            updateCapFromScene();
            updateRemovedMaterial();
        }
    }

    // Switching between Display and Geometry changes which of the two draws the
    // cap, and neither publishes anything else to notice. Relying on Shape to
    // signal it fails outright when the plane misses everything, because Shape
    // is null in both modes and never changes.
    if (prop == &feat->ResultMode) {
        updateCapFromScene();
        updateRemovedMaterial();
    }

    if (prop == &feat->Shape) {
        if (!clipInstalled && Visibility.getValue()) {
            installClipPlane();
        }
        refreshSourceBBoxCache();
        updatePlaneVisual();
        if (perSolidColors()) {
            applyPerSolidColors();
        }
        // New tessellation, so the hatching has to be sliced again
        updateHatchGeometry();
        updateCapFromScene();
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

// Edit/cancelEdit workflow. Essentially entry points.
bool ViewProviderSectionAnalysis::setEdit(int ModNum)
{
    // We only support default mode
    if (ModNum != ViewProvider::Default) {
        return ViewProviderPart::setEdit(ModNum);
    }

    auto* dlg = Gui::Control().activeDialog(getDocument()->getDocument());
    auto* section = qobject_cast<TaskSectionAnalysis*>(dlg);

    // Reusable only if it is already editing this same section
    auto* reusable = (section && section->getObject() == this->getObject()) ? section : nullptr;

    if (dlg && !reusable) {
        if (!dlg->canClose()) {
            return false;
        }
        Gui::Control().closeDialog(getDocument()->getDocument());
    }

    Gui::Selection().clearSelection();

    // Show the cutting plane visual when entering edit mode
    if (planeSwitch) {
        planeSwitch->whichChild = SO_SWITCH_ALL;
    }

    if (reusable) {
        Gui::Control().showDialog(reusable, getDocument()->getDocument());
    }
    else {
        Gui::Control().showDialog(
            new TaskSectionAnalysis(getObject<Part::SectionAnalysis>(), this),
            getDocument()->getDocument()
        );
    }

    return true;
}

void ViewProviderSectionAnalysis::unsetEdit(int ModNum)
{
    if (ModNum != ViewProvider::Default) {
        return ViewProviderPart::unsetEdit(ModNum);
    }

    // Hide the cutting plane visual when leaving edit mode
    if (planeSwitch) {
        planeSwitch->whichChild = SO_SWITCH_NONE;
    }
    Gui::Control().closeDialog(nullptr);
}
