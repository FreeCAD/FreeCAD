// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2015 Yorik van Havre
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

#include <gp_Pnt.hxx>

#include <App/Document.h>
#include <Gui/Document.h>
#include <Mod/Part/App/TopoShape.h>

#include <Mod/Import/App/dxf/ImpExpDxf.h>


namespace ImportGui
{
class ImpExpDxfReadGui: public Import::ImpExpDxfRead
{
public:
    ImpExpDxfReadGui(const std::string& filepath, App::Document* pcDoc);

protected:
    void ApplyGuiStyles(Part::Feature* object) const override;
    void ApplyGuiStyles(App::Link* object) const override;
    void ApplyGuiStyles(App::FeaturePython* object) const override;

private:
    Gui::Document* GuiDocument;
    int GetDrawStyle() const;
};
}  // namespace ImportGui
