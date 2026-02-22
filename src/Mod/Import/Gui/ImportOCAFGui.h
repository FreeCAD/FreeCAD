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

#pragma once

#include <Mod/Import/App/ImportOCAF2.h>

namespace ImportGui
{

class ImportOCAFGui: public Import::ImportOCAF2
{
public:
    ImportOCAFGui(Handle(TDocStd_Document) hDoc, App::Document* pDoc, const std::string& name);

private:
    void applyFaceColors(Part::Feature* part, const std::vector<Base::Color>& colors) override;
    void applyEdgeColors(Part::Feature* part, const std::vector<Base::Color>& colors) override;
    void applyLinkColor(App::DocumentObject* obj, int index, Base::Color color) override;
    void applyElementColors(
        App::DocumentObject* obj,
        const std::map<std::string, Base::Color>& colors
    ) override;
};

}  // namespace ImportGui
