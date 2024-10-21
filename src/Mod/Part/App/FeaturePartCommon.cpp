/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Mod/Part/App/FCBRepAlgoAPI_Common.h>
# include <BRepCheck_Analyzer.hxx>
# include <Standard_Failure.hxx>
# include <TopExp.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Application.h>
#include <Base/Parameter.h>

#include "FeaturePartCommon.h"
#include "TopoShapeOpCode.h"
#include "modelRefine.h"


using namespace Part;

PROPERTY_SOURCE(Part::Common, Part::Boolean)


Common::Common() = default;

const char *Common::opCode() const
{
    return Part::OpCodes::Common;
}

BRepAlgoAPI_BooleanOperation* Common::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a section operation:
    return new FCBRepAlgoAPI_Common(base, tool);
}

// ----------------------------------------------------

PROPERTY_SOURCE(Part::MultiCommon, Part::Feature)


MultiCommon::MultiCommon()
{
    ADD_PROPERTY(Shapes,(nullptr));
    Shapes.setSize(0);
    ADD_PROPERTY_TYPE(History,(ShapeHistory()), "Boolean", (App::PropertyType)
        (App::Prop_Output|App::Prop_Transient|App::Prop_Hidden), "Shape history");
    History.setSize(0);

    ADD_PROPERTY_TYPE(Refine,(0),"Boolean",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after this boolean operation");

    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

short MultiCommon::mustExecute() const
{
    if (Shapes.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *MultiCommon::execute()
{
    std::vector<TopoShape> shapes;
    for (auto obj : Shapes.getValues()) {
        TopoShape sh = Feature::getTopoShape(obj);
        if (sh.isNull()) {
            return new App::DocumentObjectExecReturn("Input shape is null");
        }
        shapes.push_back(sh);
    }

    TopoShape res {0};
    res.makeElementBoolean(Part::OpCodes::Common, shapes);
    if (res.isNull()) {
        throw Base::RuntimeError("Resulting shape is null");
    }

    Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                             .GetUserParameter()
                                             .GetGroup("BaseApp")
                                             ->GetGroup("Preferences")
                                             ->GetGroup("Mod/Part/Boolean");
    if (hGrp->GetBool("CheckModel", false)) {
        BRepCheck_Analyzer aChecker(res.getShape());
        if (!aChecker.IsValid()) {
            return new App::DocumentObjectExecReturn("Resulting shape is invalid");
        }
    }

    if (this->Refine.getValue()) {
        res = res.makeElementRefine();
    }
    this->Shape.setValue(res);
    if (Shapes.getSize() > 0) {
        App::DocumentObject* link = Shapes.getValues()[0];
        copyMaterial(link);
    }

    return Part::Feature::execute();
}
