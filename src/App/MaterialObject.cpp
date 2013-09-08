/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel (FreeCAD@juergen-riegel.net)         *
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
#endif

#include "MaterialObject.h"
#include "DocumentObjectPy.h"

using namespace App;

PROPERTY_SOURCE(App::MaterialObject, App::DocumentObject)


MaterialObject::MaterialObject() 
{
    ADD_PROPERTY_TYPE(Material,(),"Material",Prop_None,"Material key/valu map");

}

MaterialObject::~MaterialObject()
{
}

// Python feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(App::MaterialObjectPython, App::MaterialObject)
template<> const char* App::MaterialObjectPython::getViewProviderName(void) const {
    return "Gui::ViewProviderMaterialObjectPython";
}
/// @endcond

// explicit template instantiation
template class AppExport FeaturePythonT<App::MaterialObject>;
}