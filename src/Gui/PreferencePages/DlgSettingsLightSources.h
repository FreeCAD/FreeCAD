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


#ifndef GUI_DIALOG_DLGSETTINGSLIGHTSOURCES_H
#define GUI_DIALOG_DLGSETTINGSLIGHTSOURCES_H

#include <Gui/PropertyPage.h>
#include <memory>
#include <QPointer>

class SoDragger;
class SoDirectionalLightDragger;
class SoOrthographicCamera;

namespace Gui {
class View3DInventorViewer;
namespace Dialog {
class Ui_DlgSettingsLightSources;

/**
 * The DlgSettingsLightSources class implements a preference page to change settings
 * for the light sources of a 3D view.
 * @author Werner Mayer
 */
class DlgSettingsLightSources : public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsLightSources(QWidget* parent = nullptr);
    ~DlgSettingsLightSources() override = default;

    void saveSettings() override;
    void loadSettings() override;
    void resetSettingsToDefaults() override;

public Q_SLOTS:
    void updateDraggerQS ();
    void updateDraggerXYZ();
    void toggleLight(bool on);
    void lightIntensity(int value);
    void lightColor();

    void pushIn (void);
    void pullOut(void);

protected:
    void changeEvent(QEvent* event) override;

private:
    void saveDirection();
    void loadDirection();
    void createViewer();
    SoDirectionalLightDragger* createDragger();
    static void dragMotionCallback(void *data, SoDragger *drag);

private:
    std::unique_ptr<Ui_DlgSettingsLightSources> ui;
    QPointer <View3DInventorViewer> view;
    SoDirectionalLightDragger* lightDragger = nullptr;
    SoOrthographicCamera *camera = nullptr;

    float cam_step = 3.0f;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSLIGHTSOURCES_H
