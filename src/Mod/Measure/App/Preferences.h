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

#ifndef Preferences_h_
#define Preferences_h_

#include <Mod/Measure/MeasureGlobal.h>

#include <string>

#include <Base/Parameter.h>
#include <App/Material.h>

namespace App
{
class Color;
}

namespace Measure
{

//getters for parameters used in multiple places.
class MeasureExport Preferences
{

public:
    static Base::Reference<ParameterGrp> getPreferenceGroup(const char* Name);

    static App::Color defaultLineColor();
    static App::Color defaultTextColor();
    static double defaultDistFactor();
    static int defaultFontSize();
    static bool defaultMirror();
    static App::Color defaultTextBackgroundColor();
};


}//end namespace Measure
#endif

