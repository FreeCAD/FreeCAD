// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2004 Werner Mayer
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

class QTreeWidgetItem;

namespace Gui
{
class PythonSyntaxHighlighter;

namespace Dialog
{
class Ui_DlgSettingsEditor;

/** This class implements a preferences page for the editor settings.
 *  Here you can change different color settings and font for editors.
 *  @author Werner Mayer
 */
struct DlgSettingsEditorP;
class DlgSettingsEditor: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsEditor(QWidget* parent = nullptr);
    ~DlgSettingsEditor() override;

public:
    void saveSettings() override;
    void loadSettings() override;
    void resetSettingsToDefaults() override;

private:
    void setupConnections();
    void onDisplayItemsCurrentItemChanged(QTreeWidgetItem* i);
    void onColorButtonChanged();
    void onFontFamilyActivated(const QString&);
    void onFontSizeValueChanged(const QString&);

protected:
    void changeEvent(QEvent* e) override;
    void setEditorTabWidth(int);

private:
    std::unique_ptr<Ui_DlgSettingsEditor> ui;
    DlgSettingsEditorP* d;
    Gui::PythonSyntaxHighlighter* pythonSyntax;

    DlgSettingsEditor(const DlgSettingsEditor&);
    DlgSettingsEditor& operator=(const DlgSettingsEditor&);
};

}  // namespace Dialog
}  // namespace Gui
