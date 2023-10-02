/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef MESHGUI_DLGSETTINGSMESHVIEW_H
#define MESHGUI_DLGSETTINGSMESHVIEW_H

#ifndef MESH_GLOBAL_H
#include <Mod/Mesh/MeshGlobal.h>
#endif
#include <memory>

#include <Gui/PropertyPage.h>


namespace MeshGui
{
class Ui_DlgSettingsMeshView;
/**
 * The DlgSettingsMeshView class implements a preference page to change settings
 * for display of meshes.
 * @author Werner Mayer
 */
class DlgSettingsMeshView: public Gui::Dialog::PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsMeshView(QWidget* parent = nullptr);
    ~DlgSettingsMeshView() override;

protected:
    void saveSettings() override;
    void loadSettings() override;
    void changeEvent(QEvent* e) override;

private:
    std::unique_ptr<Ui_DlgSettingsMeshView> ui;
};

}  // namespace MeshGui

#endif  // MESHGUI_DLGSETTINGSMESHVIEW_H
