// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2026 AstoCAD     <hello@astocad.com>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/ViewProviderSuppressibleExtension.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

namespace TechDrawGui
{

class TechDrawGuiExport ViewProviderViewBreak:
    public Gui::ViewProviderDocumentObject,
    public Gui::ViewProviderSuppressibleExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(TechDrawGui::ViewProviderViewBreak);

public:
    ViewProviderViewBreak();
    ~ViewProviderViewBreak() override = default;
};

} // namespace TechDrawGui
