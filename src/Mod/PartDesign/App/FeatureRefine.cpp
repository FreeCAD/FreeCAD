/***************************************************************************
 *   Copyright (c) 2024<bgbsww@gmail.com>                                  *
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
# include <Standard_Failure.hxx>
#endif

#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Parameter.h>
#include <Mod/Part/App/modelRefine.h>

#include "FeatureRefine.h"
#include "FeaturePy.h"


using namespace PartDesign;

namespace PartDesign
{
PROPERTY_SOURCE(PartDesign::FeatureRefine, PartDesign::Feature)

FeatureRefine::FeatureRefine()
{
    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after operations");
    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", true));
}

bool FeatureRefine::onlyHasToRefine() const
{
    if( ! Refine.isTouched()){
        return false;
    }
    if (rawShape.isNull()){
        return false;
    }
    std::vector<App::Property*> propList;
    getPropertyList(propList);
    for (auto prop : propList){
        if (prop != &Refine
            /*&& prop != &SuppressedShape*/
            && prop->isTouched()){
            return false;
        }
    }
    return true;
}

TopoShape FeatureRefine::refineShapeIfActive(const TopoShape& oldShape, const RefineErrorPolicy onError) const
{
    if (this->Refine.getValue()) {
        TopoShape shape(oldShape);
        try {
            return shape.makeElementRefine();
        }
        catch (Standard_Failure& err) {
            if(onError == RefineErrorPolicy::Warn){
                Base::Console().Warning((std::string("Refine failed: ") + err.GetMessageString()).c_str());
            } else {
                throw;
            }
        }
    }
    return oldShape;
}

}


namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureRefinePython, PartDesign::FeatureRefine)
template<> const char* PartDesign::FeatureRefinePython::getViewProviderName() const {
    return "PartDesignGui::ViewProviderPython";
}
template<> PyObject* PartDesign::FeatureRefinePython::getPyObject() {
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new FeaturePythonPyT<PartDesign::FeaturePy>(this),true);
    }
    return Py::new_reference_to(PythonObject);
}
/// @endcond

// explicit template instantiation
template class PartDesignExport FeaturePythonT<PartDesign::FeatureRefine>;
}


