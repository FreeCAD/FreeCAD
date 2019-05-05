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
# include <BRepAlgoAPI_BooleanOperation.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <Standard_Failure.hxx>
# include <memory>
#endif

#include "FeaturePartBoolean.h"
#include "modelRefine.h"
#include <App/Application.h>
#include <App/Document.h>
#include <Base/Parameter.h>
#include "TopoShapeOpCode.h"


using namespace Part;

PROPERTY_SOURCE_ABSTRACT(Part::Boolean, Part::Feature)


Boolean::Boolean(void)
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Tool,(0));
    ADD_PROPERTY_TYPE(History,(ShapeHistory()), "Boolean", (App::PropertyType)
        (App::Prop_Output|App::Prop_Transient|App::Prop_Hidden), "Shape history");
    History.setSize(0);

    ADD_PROPERTY_TYPE(Refine,(0),"Boolean",(App::PropertyType)(App::Prop_None),"Refine shape (clean up redundant edges) after this boolean operation");

    //init Refine property
    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");
    this->Refine.setValue(hGrp->GetBool("RefineModel", false));
}

short Boolean::mustExecute() const
{
    if (Base.getValue() && Tool.getValue()) {
        if (Base.isTouched())
            return 1;
        if (Tool.isTouched())
            return 1;
    }
    return 0;
}

const char *Boolean::opCode() const {
    return TOPOP_BOOLEAN;
}

App::DocumentObjectExecReturn *Boolean::execute(void)
{
    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        std::vector<TopoShape> shapes;
        shapes.reserve(2);
        // Now, let's get the TopoDS_Shape
        shapes.push_back(Feature::getTopoShape(Base.getValue()));
        auto BaseShape = shapes[0].getShape();
        if (BaseShape.IsNull())
            throw NullShapeException("Base shape is null");
        shapes.push_back(Feature::getTopoShape(Tool.getValue()));
        auto ToolShape = shapes[1].getShape();
        if (ToolShape.IsNull())
            throw NullShapeException("Tool shape is null");

        std::unique_ptr<BRepAlgoAPI_BooleanOperation> mkBool(makeOperation(BaseShape, ToolShape));
        if (!mkBool->IsDone()) {
            std::stringstream error;
            error << "Boolean operation failed";
            if (BaseShape.ShapeType() != TopAbs_SOLID) {
                error << std::endl << Base.getValue()->Label.getValue() << " is not a solid";
            }
            if (ToolShape.ShapeType() != TopAbs_SOLID) {
                error << std::endl << Tool.getValue()->Label.getValue() << " is not a solid";
            }
            return new App::DocumentObjectExecReturn(error.str());
        }
        TopoDS_Shape resShape = mkBool->Shape();
        if (resShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }
        Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
            .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");

        if (hGrp->GetBool("CheckModel", false)) {
            BRepCheck_Analyzer aChecker(resShape);
            if (! aChecker.IsValid() ) {
                return new App::DocumentObjectExecReturn("Resulting shape is invalid");
            }
        }

#ifdef FC_NO_ELEMENT_MAP
        std::vector<ShapeHistory> history;
        history.emplace_back(*mkBool.get(), TopAbs_FACE, resShape, BaseShape);
        history.emplace_back(*mkBool.get(), TopAbs_FACE, resShape, ToolShape);

        if (this->Refine.getValue()) {
            try {
                TopoDS_Shape oldShape = resShape;
                BRepBuilderAPI_RefineModel mkRefine(oldShape);
                resShape = mkRefine.Shape();
                ShapeHistory hist(mkRefine, TopAbs_FACE, resShape, oldShape);
                history[0].join(hist);
                history[1].join(hist);
            }
            catch (Standard_Failure&) {
                // do nothing
            }
        }

        this->Shape.setValue(resShape);
        this->History.setValues(history);
#else
        TopoShape res(0,getDocument()->getStringHasher());
        res.makEShape(*mkBool,shapes,opCode());
        if (this->Refine.getValue()) 
            res = res.makERefine();
        this->Shape.setValue(res);
#endif
        return App::DocumentObject::StdReturn;
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when running boolean operation");
    }
}
