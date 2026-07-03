// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 Morten Vajhøj
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

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ParamHandler.h>
#include <Base/Vector3D.h>

class SoSeparator;
class SoTranslation;
class SoSwitch;
class SoCoordinate3;
class SoMaterial;
class SoBaseColor;

namespace Gui
{
class SoShapeScale;
}

namespace MassPropertiesGui
{

class ViewProviderMassPropertiesResult: public Gui::ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(MassPropertiesGui::ViewProviderMassPropertiesResult);

public:
    ViewProviderMassPropertiesResult();
    ~ViewProviderMassPropertiesResult() override;

    void attach(App::DocumentObject*) override;
    void setCenters(const Base::Vector3d& cog, const Base::Vector3d& cov);
    void setPrincipalAxes(
        const Base::Vector3d& origin,
        const Base::Vector3d& axis1,
        const Base::Vector3d& axis2,
        const Base::Vector3d& axis3,
        bool visible = true
    );

    bool allowOverride(const App::DocumentObject&) const override
    {
        return true;
    }

private:
    void updateCenterMarkers();
    void updatePrincipalAxesMarker();

    SoSeparator* displayRoot = nullptr;
    SoTranslation* cogTranslation = nullptr;
    SoTranslation* covTranslation = nullptr;
    SoSwitch* lcsSwitch = nullptr;
    SoTranslation* lcsOriginTranslation = nullptr;
    Gui::SoShapeScale* lcsScale = nullptr;
    SoMaterial* lcsMaterial = nullptr;
    SoCoordinate3* lcsCoords = nullptr;
    SoBaseColor* lcsLabel1Color = nullptr;
    SoBaseColor* lcsLabel2Color = nullptr;
    SoBaseColor* lcsLabel3Color = nullptr;
    SoTranslation* lcsLabel1Translation = nullptr;
    SoTranslation* lcsLabel2Translation = nullptr;
    SoTranslation* lcsLabel3Translation = nullptr;
    Base::Vector3d centerOfGravity {};
    Base::Vector3d centerOfVolume {};
    Base::Vector3d principalOrigin {};
    Base::Vector3d principalAxis1 {};
    Base::Vector3d principalAxis2 {};
    Base::Vector3d principalAxis3 {};
    bool showPrincipalAxes = false;
    Gui::ParamHandlers handlers;
};

}  // namespace MassPropertiesGui
