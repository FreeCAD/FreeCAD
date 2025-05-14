/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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
#ifndef _PreComp_
#include <string>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Preferences.h"


// getters for parameters used in multiple places.
// ensure this is in sync with parameter names and default values on preference pages

using namespace Measure;

//! Returns the Measure preference group
Base::Reference<ParameterGrp> Preferences::getPreferenceGroup(const char* Name)
{
    return App::GetApplication()
        .GetUserParameter()
        .GetGroup("BaseApp/Preferences/Mod/Measure")
        ->GetGroup(Name);
}

Base::Color Preferences::defaultLineColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(
        getPreferenceGroup("Appearance")->GetUnsigned("DefaultLineColor", 0x3CF00000));
    return fcColor;
}

Base::Color Preferences::defaultTextColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(
        getPreferenceGroup("Appearance")->GetUnsigned("DefaultTextColor", 0x00000000));
    return fcColor;
}

Base::Color Preferences::defaultTextBackgroundColor()
{
    Base::Color fcColor;
    fcColor.setPackedValue(
        getPreferenceGroup("Appearance")->GetUnsigned("DefaultTextBackgroundColor", 0x3CF00000));
    return fcColor;
}

int Preferences::defaultFontSize()
{
    return getPreferenceGroup("Appearance")->GetInt("DefaultFontSize", 18);
}
