// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/


#pragma once

#include <Gui/PropertyPage.h>
#include <memory>
#include <QPointer>
#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Vector3D.h>

class SoDragger;
class SbRotation;
class SoDirectionalLightDragger;
class SoOrthographicCamera;

namespace Gui
{
class View3DInventorViewer;
namespace Dialog
{
class Ui_DlgSettingsLightSources;

/**
 * The DlgSettingsLightSources class implements a preference page to change settings
 * for the light sources of a 3D view.
 * @author Werner Mayer
 */
class DlgSettingsLightSources: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsLightSources(QWidget* parent = nullptr);
    ~DlgSettingsLightSources() override = default;

    void saveSettings() override;
    void loadSettings() override;
    void resetSettingsToDefaults() override;

public Q_SLOTS:
    void zoomIn() const;
    void zoomOut() const;

protected:
    void changeEvent(QEvent* event) override;

private:
    void configureViewer();

    Base::Vector3d azimuthElevationToDirection(double azimuth, double elevation);
    std::pair<double, double> directionToAzimuthElevation(Base::Vector3d direction);

private:
    std::unique_ptr<Ui_DlgSettingsLightSources> ui;
    QPointer<View3DInventorViewer> view;
    SoOrthographicCamera* camera = nullptr;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View/LightSources"
    );
    ParameterGrp::handle hGrpView = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/View"
    );

    float zoomStep = 3.0f;
};

}  // namespace Dialog
}  // namespace Gui
