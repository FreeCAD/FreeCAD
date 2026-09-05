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

#pragma once

#include <vector>

#include <fastsignals/signal.h>

#include <Mod/Part/App/SectionCap.h>
#include <Mod/Part/PartGlobal.h>

#include <Inventor/nodes/SoLevelOfDetail.h>
#include <Inventor/sensors/SoFieldSensor.h>

#include <Mod/Part/Gui/ViewProvider.h>

class SoClipPlane;
class SoCoordinate3;
class SoDrawStyle;
class SoFaceSet;
class SoIndexedLineSet;
class SoMaterial;
class SoSeparator;
class SoShapeHints;
class SoSwitch;
class SoCamera;

namespace Gui
{
class SoLinearDraggerContainer;
class SoRotationDraggerContainer;
}  // namespace Gui

namespace App
{
class DocumentObject;
class Property;
}  // namespace App

namespace Part
{
class SectionAnalysis;
}

namespace PartGui
{

/// SoLevelOfDetail picks a level from the camera and viewport in doAction(),
/// but FreeCAD's own actions (selection, highlight, ...) enable only the handful
/// of elements they need, so a stock node reads an element that is not there and
/// asserts. This subclass serves every non-render traversal the full-detail child
/// instead; rendering is untouched, because SoLevelOfDetail::GLRender calls its
/// own doAction explicitly.
class SoHatchLevelOfDetail: public SoLevelOfDetail
{
    SO_NODE_HEADER(SoHatchLevelOfDetail);

public:
    static void initClass();
    SoHatchLevelOfDetail();

protected:
    void doAction(SoAction* action) override;
    ~SoHatchLevelOfDetail() override = default;
};

class PartGuiExport ViewProviderSectionAnalysis: public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSectionAnalysis);

public:
    App::PropertyBool ShowHatching;
    App::PropertyFloatConstraint HatchLineWidth;
    App::PropertyLength HatchSpacing;
    App::PropertyBool AutoHideHatching;
    App::PropertyBool PerBodyColors;

    /// Show the material the section cuts away, faintly.
    ///
    /// A section with everything on the far side clipped leaves cross-sections
    /// floating in space with nothing to place them against - worse still with
    /// two sections crossing. Drawing the removed half as a ghost puts them back
    /// in context. Off by default: it is an aid, not the point of the tool.
    App::PropertyBool ShowRemovedMaterial;
    App::PropertyPercent RemovedMaterialTransparency;
    App::PropertyColor RemovedMaterialColor;

    ViewProviderSectionAnalysis();
    ~ViewProviderSectionAnalysis() override;

    void attach(App::DocumentObject* pcFeat) override;
    void finishRestoring() override;
    void updateData(const App::Property* prop) override;
    void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    void unsetEditViewer(Gui::View3DInventorViewer*) override;

    void show() override;
    void hide() override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool onDelete(const std::vector<std::string>&) override;

    /// Closest the hatch lines may be spaced.
    ///
    /// Not a tolerance: it is the point below which hatching stops being legible
    /// and starts being an enormous number of lines. PropertyLength only clamps
    /// through the editor, so a restored file or a script can put anything here.
    static constexpr double minHatchSpacing = 0.001;

    /// Visually distinct default colours, cycled by index, so successive
    /// sections and the bodies within one are easy to tell apart.
    static App::Material paletteColor(std::size_t index);

    /// A distinct colour for any index, for when there are more parts than the
    /// palette has entries.
    ///
    /// The palette holds six. An assembly has hundreds - the file this was built
    /// against has 1428 parts - so cycling it puts the same colour on neighbours
    /// over and over and tells the user nothing. Hues are stepped by the golden
    /// ratio instead, which keeps successive indices far apart on the wheel and
    /// never repeats exactly.
    static App::Material distinctColor(std::size_t index);

    /// The colour to draw one part's cross-section in: the part's own, if it has
    /// one, else a generated distinct colour.
    ///
    /// Taking the part's own colour makes the section read like the model it was
    /// cut from rather than like a chart - which is the whole point on an
    /// assembly, where the user already knows what colour a thing is.
    static App::Material partColor(const App::DocumentObject* part, std::size_t index);

    /// True when a colour is the one every uncoloured object already has, and so
    /// says nothing about which part it belongs to.
    static bool isDefaultShapeColor(const Base::Color& colour);

    void setHatching(bool on);
    void setPerSolidColors(bool on);
    void setShowPlane(bool on);

    /// The properties are the only state these two have; reading them back
    /// beats a cached copy that has to be kept in step.
    bool hatchingEnabled() const
    {
        return ShowHatching.getValue();
    }
    bool perSolidColors() const
    {
        return PerBodyColors.getValue();
    }

    /// Centre and diagonal of the cached source bounding box.
    /// False if there is nothing to measure.
    /// Shared so the plane quad and the handles are placed
    /// from one box - computing it twice let them drift apart.
    bool sourceBounds(Base::Vector3d& centre, double& diagonal);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void onChanged(const App::Property* prop) override;

    /// Include the scene-graph cap, which hangs off pcRoot rather than the
    /// display-mode switch and so is invisible to the base class. Without it a
    /// Display-mode section has no bounding box and View Fit cannot frame it.
    Base::BoundBox3d _getBoundingBox(
        const char* subname,
        const Base::Matrix4D* mat,
        bool transform,
        const Gui::View3DInventorViewer* view,
        int depth
    ) const override;

private:
    void installClipPlane();
    void removeClipPlane();
    void updateClipPlaneEquation();
    void updatePlaneVisual();

    /// Rebuild the hatch line segments from the tessellated section faces.
    void updateHatchGeometry();

    /// Rebuild the cap outline and hatching from the triangles the 3D view is
    /// already drawing, without asking OCCT for anything. This is what keeps a
    /// large assembly interactive; see Part::SectionAnalysis::ResultMode.
    void updateCapFromScene();

    /// Draw the cut-away half faintly, so the section has something to sit in.
    ///
    /// Rebuilds the geometry, so it is for when the triangles or the appearance
    /// change - not for when the plane moves. See updateRemovedMaterialPlane().
    void updateRemovedMaterial();

    /// Point the ghost's clip plane at the current cutting plane.
    ///
    /// All a plane move actually changes. The triangles are the same ones, and
    /// re-copying an assembly's worth of them per drag step is what made the
    /// ghost expensive enough to look like the section itself was slow.
    void updateRemovedMaterialPlane();

    /// Split out from the rebuild because changing a float should not re-copy
    /// an assembly's worth of triangles.
    void updateRemovedMaterialAppearance();

    void applyPerSolidColors();

    /// Push ShowHatching / PerBodyColors into the scene graph. Separate from the
    /// setters so showing or restoring can re-apply without writing a property,
    /// which would mark the document modified.
    void applyHatching();
    void applySectionColors();

    /// Cache the source bbox; expensive on large assemblies, so refreshed only
    /// when the geometry can have changed not on every plane move.
    void refreshSourceBBoxCache();

    /// Recompute the section when visibility of an object under Source changes
    void slotChangedObject(const App::DocumentObject& obj, const App::Property& prop);

    SoSwitch* planeSwitch = nullptr;
    SoSeparator* cuttingPlane = nullptr;
    SoShapeHints* planeHints = nullptr;
    SoMaterial* planeMaterial = nullptr;
    SoCoordinate3* planeCoords = nullptr;
    SoFaceSet* planeFaceSet = nullptr;
    SoMaterial* planeBorderMaterial = nullptr;
    SoIndexedLineSet* planeBorderLines = nullptr;

    // Hatching drawn as real line geometry (crisp at any zoom, arbitrary width)
    SoSwitch* hatchSwitch = nullptr;
    SoHatchLevelOfDetail* hatchLod = nullptr;
    SoSeparator* hatchRoot = nullptr;
    SoDrawStyle* hatchStyle = nullptr;
    SoCoordinate3* hatchCoords = nullptr;
    SoIndexedLineSet* hatchLines = nullptr;

    // Cap drawn from the scene graph. One child per source object, so each can
    // carry its own colour and hatch angle.
    SoSwitch* capSwitch = nullptr;
    SoSeparator* capRoot = nullptr;

    SoSeparator* removedMaterialRoot = nullptr;
    SoSwitch* removedMaterialSwitch = nullptr;

    /// Shared by every ghosted body, so moving the plane writes one field.
    /// Owned by pcGhostRoot; cleared whenever its children are.
    SoClipPlane* removedMaterialClip = nullptr;

    /// Owned by removedMaterialRoot; cleared whenever its children are.
    SoMaterial* removedMaterialAppearance = nullptr;

    /// Triangles harvested from the 3D view, one entry per source object.
    ///
    /// Walking the scene graph costs far more than slicing what comes out of it,
    /// and the triangles do not change when the plane moves - only when the
    /// geometry does. So this is kept until something invalidates it.
    struct HarvestedBody
    {
        /// The part these triangles came from, for its colour. Not owned; the
        /// cache is dropped whenever the source list changes.
        App::DocumentObject* source = nullptr;
        Part::SectionCap::TriangleSoup soup;
        // Measured once, here, where the points are already being walked. The
        // extent along any normal follows from the box in constant time, so
        // moving the plane never costs a pass over the triangles just to decide
        // whether to skip them.
        Base::BoundBox3d bounds;
    };
    std::vector<HarvestedBody> harvestCache;
    bool harvestValid = false;

    /// Re-harvest the triangles of every source object. Only geometry or
    /// visibility changes need this; moving the plane does not.
    void refreshHarvestCache();

    /// Give the harvested triangles back.
    ///
    /// The cache exists to make plane moves cheap, and is worth hundreds of
    /// megabytes on an assembly. Nothing about it is worth keeping once the
    /// section is hidden or the cap is not being drawn at all, and a view
    /// provider outlives the editing session by the whole life of the document.
    void releaseHarvestCache();

    bool clipInstalled = false;
    /// An object and the clip node living inside its own view provider, which
    /// holds the cutting plane in that object's local frame. Paired rather than
    /// two lists, so the node cannot be transformed by the wrong placement.
    struct ClippedObject
    {
        App::DocumentObject* object = nullptr;
        SoClipPlane* node = nullptr;
    };
    std::vector<ClippedObject> clippedObjects;

    // Cached source bbox (xmin,ymin,zmin,xmax,ymax,zmax) for sizing the plane
    // quad without recomputing the source shape on every move.
    bool sourceBBoxValid = false;
    Base::BoundBox3d sourceBBox;

    fastsignals::scoped_connection visibilityConn;

    static App::PropertyFloatConstraint::Constraints hatchWidthRange;
};

}  // namespace PartGui
