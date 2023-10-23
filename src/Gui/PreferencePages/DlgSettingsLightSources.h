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

class SoDragger;
class SoDirectionalLightDragger;

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
    ~DlgSettingsLightSources() override;

    void saveSettings() override;
    void loadSettings() override;

protected:
    void changeEvent(QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void setupConnection();
    void toggleLight(bool on);
    void lightIntensity(int value);
    void lightColor();
    void saveDirection();
    void loadDirection();
    QWidget* createViewer(QWidget* parent);
    SoDirectionalLightDragger* createDragger();
    static void dragMotionCallback(void *data, SoDragger *drag);

private:
    std::unique_ptr<Ui_DlgSettingsLightSources> ui;
    View3DInventorViewer* view = nullptr;
    SoDirectionalLightDragger* lightDragger = nullptr;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSLIGHTSOURCES_H
