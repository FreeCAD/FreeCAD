/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
