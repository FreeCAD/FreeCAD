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

#include <App/DocumentObject.h>
#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include <App/SuppressibleExtension.h>
#include <Mod/TechDraw/TechDrawGlobal.h>

namespace TechDraw
{

enum class BreakType : int {
    NONE,
    ZIGZAG,
    SIMPLE,
    SINUSOID
};

class TechDrawExport DrawViewBreak: public App::DocumentObject,
                                    public App::SuppressibleExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(TechDraw::DrawViewBreak);

public:
    DrawViewBreak();
    ~DrawViewBreak() override = default;

    App::PropertyVector StartPoint;
    App::PropertyVector EndPoint;
    App::PropertyVector Direction;
    App::PropertyDistance Gap;
    App::PropertyEnumeration BreakType;

    static const char* BreakTypeEnums[];

    void onChanged(const App::Property* property) override;

    const char* getViewProviderName() const override
    {
        return "TechDrawGui::ViewProviderViewBreak";
    }
};

} // namespace TechDraw
