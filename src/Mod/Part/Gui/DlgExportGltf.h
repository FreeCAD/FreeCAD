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


#ifndef PARTGUI_DLGEXPORTGLTF_H
#define PARTGUI_DLGEXPORTGLTF_H

#include <Mod/Part/PartGlobal.h>
#include <Gui/PropertyPage.h>
#include <QDialog>

class QButtonGroup;
class QCheckBox;

namespace PartGui {

struct GltfExportSettings
{
    bool exportUVCoords = false;
    bool mergeFaces = false;
    bool multiThreaded = false;
};

// ----------------------------------------------------------------------------

class Ui_DlgExportGltf;
class DlgExportGltf : public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgExportGltf(QWidget* parent = nullptr);
    ~DlgExportGltf() override;

    void saveSettings() override;
    void loadSettings() override;

    GltfExportSettings getSettings() const;

protected:
    void changeEvent(QEvent *e) override;

private:
    std::unique_ptr<Ui_DlgExportGltf> ui;
};

// ----------------------------------------------------------------------------

class PartGuiExport TaskExportGltf : public QDialog
{
    Q_OBJECT

public:
    explicit TaskExportGltf(QWidget* parent = nullptr);
    ~TaskExportGltf() override;

    bool showDialog() const;
    void accept() override;
    GltfExportSettings getSettings() const;

private:
    QCheckBox* showThis;
    std::unique_ptr<DlgExportGltf> ui;
};

} // namespace PartGui

#endif // PARTGUI_DLGEXPORTGLTF_H
