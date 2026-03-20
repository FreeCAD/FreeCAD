// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD contributors                              *
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

#pragma once

#include <Mod/Part/PartGlobal.h>

#include <Mod/Part/Gui/ViewProvider.h>

class SoClipPlane;
class SoCoordinate3;
class SoFaceSet;
class SoIndexedLineSet;
class SoMaterial;
class SoSeparator;
class SoShapeHints;
class SoSwitch;
class SoTexture2;
class SoTextureCoordinatePlane;

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
    ViewProviderSectionAnalysis();
    ~ViewProviderSectionAnalysis() override;

    void attach(App::DocumentObject* pcFeat) override;
    void updateData(const App::Property* prop) override;
    void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    void unsetEditViewer(Gui::View3DInventorViewer*) override;

    void show() override;
    void hide() override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;
    bool onDelete(const std::vector<std::string>&) override;

    void setHatching(bool on);

protected:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;

private:
    void installClipPlane();
    void removeClipPlane();
    void updateClipPlaneEquation();
    void updatePlaneVisual();

    SoSwitch* pcPlaneSwitch = nullptr;
    SoSeparator* pcPlaneRoot = nullptr;
    SoShapeHints* pcPlaneHints = nullptr;
    SoMaterial* pcPlaneMaterial = nullptr;
    SoCoordinate3* pcPlaneCoords = nullptr;
    SoFaceSet* pcPlaneFaceSet = nullptr;
    SoMaterial* pcPlaneBorderMaterial = nullptr;
    SoIndexedLineSet* pcPlaneBorderLines = nullptr;
    SoClipPlane* pcClipPlane = nullptr;
    SoTexture2* pcHatchTexture = nullptr;
    SoTextureCoordinatePlane* pcHatchCoordGen = nullptr;
    bool clipInstalled = false;
    App::DocumentObject* clipInstalledOn = nullptr;
};

}  // namespace PartGui
