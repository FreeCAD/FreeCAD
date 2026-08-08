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

#include <Mod/Part/PartGlobal.h>

#include <Mod/Part/Gui/ViewProvider.h>

class SoClipPlane;
class SoCoordinate3;
class SoDrawStyle;
class SoFaceSet;
class SoIndexedLineSet;
class SoLevelOfDetail;
class SoMaterial;
class SoSeparator;
class SoShapeHints;
class SoSwitch;

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

class PartGuiExport ViewProviderSectionAnalysis: public ViewProviderPart
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderSectionAnalysis);

public:
    App::PropertyBool ShowHatching;
    App::PropertyFloatConstraint HatchLineWidth;
    App::PropertyLength HatchSpacing;
    App::PropertyBool AutoHideHatching;
    App::PropertyBool PerBodyColors;

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

    void setHatching(bool on);
    void setPerSolidColors(bool on);
    void setShowPlane(bool on);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void onChanged(const App::Property* prop) override;

private:
    void installClipPlane();
    void removeClipPlane();
    void updateClipPlaneEquation();
    void updatePlaneVisual();

    /// Rebuild the hatch line segments from the tessellated section faces.
    void updateHatchGeometry();
    void applyPerSolidColors();

    /// Cache the source bbox; expensive on large assemblies, so refreshed only
    /// when the geometry can have changed not on every plane move.
    void refreshSourceBBoxCache();

    /// Recompute the section when visibility of an object under Source changes
    void slotChangedObject(const App::DocumentObject& obj, const App::Property& prop);

    static void sectionDragStartCallback(void* data, SoDragger* d);
    static void sectionDragMotionCallback(void* data, SoDragger* d);
    static void sectionDragFinishCallback(void* data, SoDragger* d);
    Base::Placement draggerStartPlacement;

    SoSwitch* pcPlaneSwitch = nullptr;
    SoSeparator* pcPlaneRoot = nullptr;
    SoShapeHints* pcPlaneHints = nullptr;
    SoMaterial* pcPlaneMaterial = nullptr;
    SoCoordinate3* pcPlaneCoords = nullptr;
    SoFaceSet* pcPlaneFaceSet = nullptr;
    SoMaterial* pcPlaneBorderMaterial = nullptr;
    SoIndexedLineSet* pcPlaneBorderLines = nullptr;

    // Hatching drawn as real line geometry (crisp at any zoom, arbitrary width)
    SoSwitch* pcHatchSwitch = nullptr;
    SoLevelOfDetail* pcHatchLod = nullptr;
    SoSeparator* pcHatchRoot = nullptr;
    SoDrawStyle* pcHatchStyle = nullptr;
    SoCoordinate3* pcHatchCoords = nullptr;
    SoIndexedLineSet* pcHatchLines = nullptr;

    bool clipInstalled = false;
    bool hatchEnabled = true;
    bool usePerSolidColors = false;
    std::vector<App::DocumentObject*> clippedObjects;
    // One clip plane per clipped object (parallel to clippedObjects), holding
    // the cutting plane in that object's local frame.
    std::vector<SoClipPlane*> clipNodes;

    // Cached source bbox (xmin,ymin,zmin,xmax,ymax,zmax) for sizing the plane
    // quad without recomputing the source shape on every move.
    bool sourceBBoxValid = false;
    double sourceBBox[6] = {0, 0, 0, 0, 0, 0};

    fastsignals::scoped_connection visibilityConn;

    static App::PropertyFloatConstraint::Constraints hatchWidthRange;
};

}  // namespace PartGui
