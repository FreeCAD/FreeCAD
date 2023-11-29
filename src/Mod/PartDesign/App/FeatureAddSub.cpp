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
# include <Standard_Failure.hxx>
#endif

#include <App/Application.h>
#include <App/FeaturePythonPyImp.h>
#include <Base/Parameter.h>
# include <BRepAlgoAPI_Cut.hxx>
# include <BRepAlgoAPI_Common.hxx>
# include <BRepAlgoAPI_Fuse.hxx>

#include <Mod/Part/App/modelRefine.h>

#include "FeatureAddSub.h"
#include "FeaturePy.h"


using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::FeatureAddSub, PartDesign::Feature)

FeatureAddSub::FeatureAddSub()
{
    ADD_PROPERTY(AddSubShape,(TopoDS_Shape()));
    ADD_PROPERTY_TYPE(Refine,(0),"Part Design",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after adding/subtracting");
    ADD_PROPERTY_TYPE(Outside, (false),"Part Design", App::Prop_None,
        QT_TRANSLATE_NOOP("App::Property", "If set, the result will be the intersection of the profile and the preexisting body."));
    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/PartDesign");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

bool FeatureAddSub::isAdditive()
{
    return addSubType == Additive;
}

bool FeatureAddSub::isSubtractive()
{
    return addSubType == Subtractive;
}

short FeatureAddSub::mustExecute() const
{
    if (Refine.isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}

TopoDS_Shape FeatureAddSub::refineShapeIfActive(const TopoDS_Shape& oldShape) const
{
    if (this->Refine.getValue()) {
        try {
            Part::BRepBuilderAPI_RefineModel mkRefine(oldShape);
            TopoDS_Shape resShape = mkRefine.Shape();
            if (!TopoShape(resShape).isClosed()) {
                return oldShape;
            }
            return resShape;
        }
        catch (Standard_Failure&) {
            return oldShape;
        }
    }

    return oldShape;
}

void FeatureAddSub::getAddSubShape(Part::TopoShape &addShape, Part::TopoShape &subShape)
{
    if (addSubType == Additive)
        addShape = AddSubShape.getShape();
    else if (addSubType == Subtractive)
        subShape = AddSubShape.getShape();
}

TopoDS_Shape FeatureAddSub::subtractiveOp(const TopoDS_Shape &baseShape, const TopoDS_Shape &opShape)
{
    Outside.setStatus(App::Property::Hidden, ! isSubtractive());    // Set this after creation, like here.
    TopoDS_Shape result;
    if (  Outside.getValue() ) {
        BRepAlgoAPI_Common mkCom(baseShape, opShape);
        if (!mkCom.IsDone())
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Intersection of base feature failed"));
        result = mkCom.Shape();
    } else {
        BRepAlgoAPI_Cut mkCut(baseShape, opShape);
        if (!mkCut.IsDone())
            throw Base::CADKernelError(QT_TRANSLATE_NOOP("Exception", "Cut out of base feature failed"));
        result = mkCut.Shape();
    }
    return result;
}

App::DocumentObjectExecReturn* FeatureAddSub::addSubOp(const TopoDS_Shape &baseShape, const TopoDS_Shape &opShape)
{
    if (addSubType == Additive) {
        BRepAlgoAPI_Fuse mkFuse(baseShape, opShape);
        if (!mkFuse.IsDone())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Adding the object failed"));
        // we have to get the solids (fuse sometimes creates compounds)
        TopoDS_Shape boolOp = this->getSolid(mkFuse.Shape());
        if (boolOp.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));
        int solidCount = countSolids(boolOp);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result has multiple solids"));
        }
        boolOp = refineShapeIfActive(boolOp);
        Shape.setValue(getSolid(boolOp));
    }
    else if (addSubType == Subtractive) {
        TopoDS_Shape boolOp = subtractiveOp(baseShape, opShape);
        if (boolOp.IsNull())
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result is not a solid"));
        int solidCount = countSolids(boolOp);
        if (solidCount > 1) {
            return new App::DocumentObjectExecReturn(QT_TRANSLATE_NOOP("Exception", "Error: Result has multiple solids"));
        }
        boolOp = refineShapeIfActive(boolOp);
        Shape.setValue(getSolid(boolOp));
        AddSubShape.setValue(boolOp);
    }
    return App::DocumentObject::StdReturn;
}

}

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(PartDesign::FeatureAddSubPython, PartDesign::FeatureAddSub)
template<> const char* PartDesign::FeatureAddSubPython::getViewProviderName() const {
    return "PartDesignGui::ViewProviderPython";
}
template<> PyObject* PartDesign::FeatureAddSubPython::getPyObject() {
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

FeatureAdditivePython::~FeatureAdditivePython() = default;


PROPERTY_SOURCE(PartDesign::FeatureSubtractivePython, PartDesign::FeatureAddSubPython)

FeatureSubtractivePython::FeatureSubtractivePython()
{
    addSubType = Subtractive;
}

FeatureSubtractivePython::~FeatureSubtractivePython() = default;

}
