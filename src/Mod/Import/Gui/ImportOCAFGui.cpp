// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 Zheng Lei <realthunder.dev@gmail.com>              *
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

#include "ImportOCAFGui.h"
#include <Gui/Application.h>
#include <Gui/ViewProviderLink.h>
#include <Mod/Part/Gui/ViewProvider.h>

using namespace ImportGui;

ImportOCAFGui::ImportOCAFGui(Handle(TDocStd_Document) hDoc,
                             App::Document* pDoc,
                             const std::string& name)
    : ImportOCAF2(hDoc, pDoc, name)
{}

void ImportOCAFGui::applyFaceColors(Part::Feature* part, const std::vector<App::Color>& colors)
{
    auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
        Gui::Application::Instance->getViewProvider(part));
    if (!vp) {
        return;
    }
    if (colors.empty()) {
        return;
    }

    if (colors.size() == 1) {
        vp->ShapeColor.setValue(colors.front());
        vp->Transparency.setValue(100 * colors.front().a);
    }
    else {
        vp->DiffuseColor.setValues(colors);
    }
}

void ImportOCAFGui::applyEdgeColors(Part::Feature* part, const std::vector<App::Color>& colors)
{
    auto vp = dynamic_cast<PartGui::ViewProviderPartExt*>(
        Gui::Application::Instance->getViewProvider(part));
    if (!vp) {
        return;
    }
    if (colors.size() == 1) {
        vp->LineColor.setValue(colors.front());
    }
    else {
        vp->LineColorArray.setValues(colors);
    }
}

void ImportOCAFGui::applyLinkColor(App::DocumentObject* obj, int index, App::Color color)
{
    auto vp =
        dynamic_cast<Gui::ViewProviderLink*>(Gui::Application::Instance->getViewProvider(obj));
    if (!vp) {
        return;
    }
    if (index < 0) {
        vp->OverrideMaterial.setValue(true);
        vp->ShapeMaterial.setDiffuseColor(color);
        return;
    }
    if (vp->OverrideMaterialList.getSize() <= index) {
        vp->OverrideMaterialList.setSize(index + 1);
    }
    vp->OverrideMaterialList.set1Value(index, true);
    App::Material mat(App::Material::DEFAULT);
    if (vp->MaterialList.getSize() <= index) {
        vp->MaterialList.setSize(index + 1, mat);
    }
    mat.diffuseColor = color;
    vp->MaterialList.set1Value(index, mat);
}

void ImportOCAFGui::applyElementColors(App::DocumentObject* obj,
                                       const std::map<std::string, App::Color>& colors)
{
    auto vp = Gui::Application::Instance->getViewProvider(obj);
    if (!vp) {
        return;
    }
    (void)colors;
}
