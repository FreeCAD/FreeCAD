/***************************************************************************
 *   Copyright (c) 2011 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#include <App/FeaturePythonPyImp.h>
#include "FeatureAddSub.h"
#include "FeaturePy.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::Feature)

FeatureAddSub::FeatureAddSub()
  :  addSubType(Additive)
{
    ADD_PROPERTY(AddSubShape,(TopoDS_Shape()));
}

FeatureAddSub::Type FeatureAddSub::getAddSubType()
{
    return addSubType;
}

}

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureAddSubPython, PartDesign::FeatureAddSub)
template<> const char* PartDesign::FeatureAddSubPython::getViewProviderName(void) const {
    return "PartDesignGui::ViewProviderPython";
}
template<> PyObject* PartDesign::FeatureAddSubPython::getPyObject(void) {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<PartDesign::FeaturePy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartDesignExport FeaturePythonT<PartDesign::FeatureAddSub>;
}


namespace PartDesign {

PROPERTY_SOURCE(PartDesign::FeatureAdditivePython, PartDesign::FeatureAddSubPython)

FeatureAdditivePython::FeatureAdditivePython()
{
    addSubType = Additive;
}

FeatureAdditivePython::~FeatureAdditivePython()
{
}


PROPERTY_SOURCE(PartDesign::FeatureSubtractivePython, PartDesign::FeatureAddSubPython)

FeatureSubtractivePython::FeatureSubtractivePython()
{
    addSubType = Subtractive;
}

FeatureSubtractivePython::~FeatureSubtractivePython()
{
}

}
