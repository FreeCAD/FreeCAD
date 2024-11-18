/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "CustomFeature.h"


using namespace Part;


PROPERTY_SOURCE(Part::CustomFeature, Part::Feature)


CustomFeature::CustomFeature() = default;

CustomFeature::~CustomFeature() = default;

short CustomFeature::mustExecute() const
{
    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn *CustomFeature::execute()
{
    return App::DocumentObject::StdReturn;
}

// ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Part::CustomFeaturePython, Part::CustomFeature)
template<> const char* Part::CustomFeaturePython::getViewProviderName() const {
    return "PartGui::ViewProviderCustomPython";
}
/// @endcond

// explicit template instantiation
template class PartExport FeaturePythonT<Part::CustomFeature>;
}

