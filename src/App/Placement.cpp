/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <Base/Placement.h>

#include "Placement.h"

using namespace App;


PROPERTY_SOURCE(App::Placement, App::GeoFeature)


//===========================================================================
// Feature
//===========================================================================

Placement::Placement(void)
{

}

Placement::~Placement(void)
{
}



// Python feature ---------------------------------------------------------
namespace App {
PROPERTY_SOURCE_TEMPLATE(App::PlacementPython, App::Placement)
template<> const char* App::PlacementPython::getViewProviderName(void) const {
   return "Gui::ViewProviderPlacementPython";
}
template class AppExport FeaturePythonT<App::Placement>;
}




