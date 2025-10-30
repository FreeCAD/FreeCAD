// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTGUI_DLGIMPORTGLTF_H
#define PARTGUI_DLGIMPORTGLTF_H

#include <Mod/Part/PartGlobal.h>
#include <Gui/PropertyPage.h>
#include <QDialog>

class QButtonGroup;
class QCheckBox;

namespace PartGui {

struct GltfImportSettings
{
    bool refinement = false;
    bool skipEmptyNodes = true;
    bool doublePrecision = false;
    bool loadAllScenes = false;
    bool multiThreaded = false;
    bool printDebugMessages = false;
};

class Ui_DlgImportGltf;
class DlgImportGltf : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgImportGltf(QWidget* parent = nullptr);
    ~DlgImportGltf() override;

    void saveSettings() override;
    void loadSettings() override;

    GltfImportSettings getSettings() const;

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui_DlgImportGltf> ui;
};

// ----------------------------------------------------------------------------

class PartGuiExport TaskImportGltf : public QDialog
{
    Q_OBJECT

public:
    explicit TaskImportGltf(QWidget* parent = nullptr);
    ~TaskImportGltf() override;

    bool showDialog() const;
    void accept() override;
    GltfImportSettings getSettings() const;

private:
    QCheckBox* showThis;
    std::unique_ptr<DlgImportGltf> ui;
};

} // namespace PartGui

#endif // PARTGUI_DLGIMPORTGLTF_H
