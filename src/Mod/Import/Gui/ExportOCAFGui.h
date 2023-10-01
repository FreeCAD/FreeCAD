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

#ifndef IMPORT_EXPORTOCAFGUI_H
#define IMPORT_EXPORTOCAFGUI_H

#include <Mod/Import/App/ExportOCAF.h>

namespace ImportGui
{

class ExportOCAFGui: public Import::ExportOCAF
{
public:
    ExportOCAFGui(Handle(TDocStd_Document) hDoc, bool explicitPlacement);
    void findColors(Part::Feature* part, std::vector<App::Color>& colors) const override;
};

}  // namespace ImportGui

#endif  // IMPORT_EXPORTOCAFGUI_H
