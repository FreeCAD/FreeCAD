// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Werner Mayer
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

#include <Gui/PropertyPage.h>
#include <memory>

namespace Gui
{

class PrefComboBox;

namespace Dialog
{
class Ui_DlgSettingsUI;

/**
 * The DlgSettingsUI class implements a preference page to change theme settings.
 * @author Pierre-Louis Boyer
 */
class DlgSettingsUI: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsUI(QWidget* parent = nullptr);
    ~DlgSettingsUI() override;

    void saveSettings() override;
    void loadSettings() override;

    void loadStyleSheet();

protected:
    void changeEvent(QEvent* e) override;

    void populateStylesheets(
        const char* key,
        const char* path,
        PrefComboBox* combo,
        const char* def,
        QStringList filter = QStringList()
    );

    void openThemeEditor();

private:
    std::unique_ptr<Ui_DlgSettingsUI> ui;
};

}  // namespace Dialog
}  // namespace Gui
