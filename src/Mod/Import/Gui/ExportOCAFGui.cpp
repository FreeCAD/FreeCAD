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


#include "PreCompiled.h"

#include "ExportOCAFGui.h"
#include <Gui/Application.h>
#include <Mod/Part/Gui/ViewProvider.h>

using namespace ImportGui;


ExportOCAFGui::ExportOCAFGui(Handle(TDocStd_Document) hDoc, bool explicitPlacement)
    : ExportOCAF(hDoc, explicitPlacement)
{}

void ExportOCAFGui::findColors(Part::Feature* part, std::vector<App::Color>& colors) const
{
    Gui::ViewProvider* vp = Gui::Application::Instance->getViewProvider(part);
    if (vp && vp->isDerivedFrom(PartGui::ViewProviderPartExt::getClassTypeId())) {
        colors = static_cast<PartGui::ViewProviderPartExt*>(vp)->DiffuseColor.getValues();
        if (colors.empty()) {
            colors.push_back(static_cast<PartGui::ViewProviderPart*>(vp)->ShapeColor.getValue());
        }
    }
}
