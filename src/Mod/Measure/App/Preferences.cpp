/***************************************************************************
 *   Copyright (c) 2023 WandererFan <wandererfan@gmail.com>                *
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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <string>
#endif

#include <App/Application.h>
#include <App/Material.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "Preferences.h"


//getters for parameters used in multiple places.
//ensure this is in sync with parameter names and default values on preference pages

using namespace Measure;

//! Returns the Measure preference group
Base::Reference<ParameterGrp> Preferences::getPreferenceGroup(const char* Name)
{
    return App::GetApplication().GetUserParameter().GetGroup("BaseApp/Preferences/Mod/Measure")->GetGroup(Name);
}

App::Color Preferences::defaultLineColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Appearance")->GetUnsigned("DefaultLineColor", 0xFFFFFFFF));
    return fcColor;
}

App::Color Preferences::defaultTextColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Appearance")->GetUnsigned("DefaultTextColor", 0x00000000));
    return fcColor;
}

App::Color Preferences::defaultTextBackgroundColor()
{
    App::Color fcColor;
    fcColor.setPackedValue(getPreferenceGroup("Appearance")->GetUnsigned("DefaultTextBackgroundColor", 0xFFFFFFFF));
    return fcColor;
}

double Preferences::defaultDistFactor()
{
    return getPreferenceGroup("Appearance")->GetFloat("DefaultDistFactor", 1.0);
}

int Preferences::defaultFontSize()
{
    return getPreferenceGroup("Appearance")->GetInt("DefaultFontSize", 18);
}

bool Preferences::defaultMirror()
{
    return getPreferenceGroup("Appearance")->GetBool("DefaultMirror", false);
}
