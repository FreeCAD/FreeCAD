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
# include <BRepAlgoAPI_Fuse.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <Standard_Failure.hxx>
#endif


#include "FeaturePartFuse.h"
#include "modelRefine.h"
#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/Exception.h>

using namespace Part;

PROPERTY_SOURCE(Part::Fuse, Part::Boolean)


Fuse::Fuse(void)
{
}

BRepAlgoAPI_BooleanOperation* Fuse::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a fuse operation:
    return new BRepAlgoAPI_Fuse(base, tool);
}

// ----------------------------------------------------

PROPERTY_SOURCE(Part::MultiFuse, Part::Feature)


MultiFuse::MultiFuse(void)
{
    ADD_PROPERTY(Shapes,(0));
    Shapes.setSize(0);
    ADD_PROPERTY_TYPE(History,(ShapeHistory()), "Boolean", (App::PropertyType)
        (App::Prop_Output|App::Prop_Transient|App::Prop_Hidden), "Shape history");
    History.setSize(0);
}

short MultiFuse::mustExecute() const
{
    if (Shapes.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *MultiFuse::execute(void)
{
    std::vector<TopoDS_Shape> s;
    std::vector<App::DocumentObject*> obj = Shapes.getValues();

    std::vector<App::DocumentObject*>::iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            s.push_back(static_cast<Part::Feature*>(*it)->Shape.getValue());
        }
    }

    if (s.size() >= 2) {
        try {
            std::vector<ShapeHistory> history;
#if OCC_VERSION_HEX <= 0x060800
            TopoDS_Shape resShape = s.front();
            if (resShape.IsNull())
                throw Base::Exception("Input shape is null");
            for (std::vector<TopoDS_Shape>::iterator it = s.begin()+1; it != s.end(); ++it) {
                if (it->IsNull())
                    throw Base::Exception("Input shape is null");

                // Let's call algorithm computing a fuse operation:
                BRepAlgoAPI_Fuse mkFuse(resShape, *it);
                // Let's check if the fusion has been successful
                if (!mkFuse.IsDone()) 
                    throw Base::Exception("Fusion failed");
                resShape = mkFuse.Shape();

                ShapeHistory hist1 = buildHistory(mkFuse, TopAbs_FACE, resShape, mkFuse.Shape1());
                ShapeHistory hist2 = buildHistory(mkFuse, TopAbs_FACE, resShape, mkFuse.Shape2());
                if (history.empty()) {
                    history.push_back(hist1);
                    history.push_back(hist2);
                }
                else {
                    for (std::vector<ShapeHistory>::iterator jt = history.begin(); jt != history.end(); ++jt)
                        *jt = joinHistory(*jt, hist1);
                    history.push_back(hist2);
                }
            }
#else
            BRepAlgoAPI_Fuse mkFuse;
            TopTools_ListOfShape shapeArguments,shapeTools;
            shapeArguments.Append(s.front());
            for (std::vector<TopoDS_Shape>::iterator it = s.begin()+1; it != s.end(); ++it) {
                if (it->IsNull())
                    throw Base::Exception("Input shape is null");
                shapeTools.Append(*it);
            }
            mkFuse.SetArguments(shapeArguments);
            mkFuse.SetTools(shapeTools);
            mkFuse.Build();
            if (!mkFuse.IsDone())
                throw Base::Exception("MultiFusion failed");
            TopoDS_Shape resShape = mkFuse.Shape();
            for (std::vector<TopoDS_Shape>::iterator it = s.begin(); it != s.end(); ++it) {
                history.push_back(buildHistory(mkFuse, TopAbs_FACE, resShape, *it));
            }
#endif
            if (resShape.IsNull())
                throw Base::Exception("Resulting shape is null");

            Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
                .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");
            if (hGrp->GetBool("CheckModel", false)) {
                BRepCheck_Analyzer aChecker(resShape);
                if (! aChecker.IsValid() ) {
                    return new App::DocumentObjectExecReturn("Resulting shape is invalid");
                }
            }
            if (hGrp->GetBool("RefineModel", false)) {
                TopoDS_Shape oldShape = resShape;
                BRepBuilderAPI_RefineModel mkRefine(oldShape);
                resShape = mkRefine.Shape();
                ShapeHistory hist = buildHistory(mkRefine, TopAbs_FACE, resShape, oldShape);
                for (std::vector<ShapeHistory>::iterator jt = history.begin(); jt != history.end(); ++jt)
                    *jt = joinHistory(*jt, hist);
            }

            this->Shape.setValue(resShape);
            this->History.setValues(history);
        }
        catch (Standard_Failure) {
            Handle_Standard_Failure e = Standard_Failure::Caught();
            return new App::DocumentObjectExecReturn(e->GetMessageString());
        }
    }
    else {
        throw Base::Exception("Not enough shape objects linked");
    }

    return App::DocumentObject::StdReturn;
}
