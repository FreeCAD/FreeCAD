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
# include <Mod/Part/App/FCBRepAlgoAPI_Fuse.h>
# include <BRepCheck_Analyzer.hxx>
# include <Standard_Failure.hxx>
# include <TopoDS_Iterator.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>

#include "FeaturePartFuse.h"
#include "TopoShape.h"
#include "modelRefine.h"
#include "TopoShapeOpCode.h"

FC_LOG_LEVEL_INIT("Part",true,true);

using namespace Part;

PROPERTY_SOURCE(Part::Fuse, Part::Boolean)


Fuse::Fuse() = default;

BRepAlgoAPI_BooleanOperation* Fuse::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a fuse operation:
    return new FCBRepAlgoAPI_Fuse(base, tool);
}

const char *Fuse::opCode() const
{
    return Part::OpCodes::Fuse;
}

// ----------------------------------------------------

PROPERTY_SOURCE(Part::MultiFuse, Part::Feature)


MultiFuse::MultiFuse()
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

short MultiFuse::mustExecute() const
{
    if (Shapes.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *MultiFuse::execute()
{
    std::vector<TopoShape> shapes;
    std::vector<App::DocumentObject*> obj = Shapes.getValues();

    std::vector<App::DocumentObject*>::iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        shapes.push_back(Feature::getTopoShape(*it));
    }

    bool argumentsAreInCompound = false;
    TopoShape compoundOfArguments;

    // if only one source shape, and it is a compound - fuse children of the compound
    const int maxIterations = 1'000'000; // will trigger "not enough shape objects linked" error below if ever reached
    for (int i = 0; shapes.size() == 1 && i < maxIterations; ++i) {
        compoundOfArguments = shapes[0];
        if (compoundOfArguments.getShape().ShapeType() == TopAbs_COMPOUND) {
            shapes.clear();
            shapes = compoundOfArguments.getSubTopoShapes();
            argumentsAreInCompound = true;
        } else {
            break;
        }
    }

    if (shapes.size() >= 2) {
        try {
            std::vector<ShapeHistory> history;
            FCBRepAlgoAPI_Fuse mkFuse;
            TopTools_ListOfShape shapeArguments, shapeTools;
            const TopoShape& shape = shapes.front();
            if (shape.isNull()) {
                throw Base::RuntimeError("Input shape is null");
            }
            shapeArguments.Append(shape.getShape());

            for (auto it2 = shapes.begin() + 1; it2 != shapes.end(); ++it2) {
                if (it2->isNull()) {
                    throw Base::RuntimeError("Input shape is null");
                }
                shapeTools.Append(it2->getShape());
            }

            mkFuse.SetArguments(shapeArguments);
            mkFuse.SetTools(shapeTools);
            mkFuse.setAutoFuzzy();
            mkFuse.Build();

            if (!mkFuse.IsDone()) {
                throw Base::RuntimeError("MultiFusion failed");
            }

            TopoShape res(0);
            res = res.makeShapeWithElementMap(mkFuse.Shape(), MapperMaker(mkFuse), shapes, OpCodes::Fuse);
            for (const auto& it2 : shapes) {
                history.push_back(
                    buildHistory(mkFuse, TopAbs_FACE, res.getShape(), it2.getShape()));
            }
            if (res.isNull()) {
                throw Base::RuntimeError("Resulting shape is null");
            }

            Base::Reference<ParameterGrp> hGrp = App::GetApplication()
                                                     .GetUserParameter()
                                                     .GetGroup("BaseApp")
                                                     ->GetGroup("Preferences")
                                                     ->GetGroup("Mod/Part/Boolean");
            if (hGrp->GetBool("CheckModel", true)) {
                BRepCheck_Analyzer aChecker(res.getShape());
                if (!aChecker.IsValid()) {
                    return new App::DocumentObjectExecReturn("Resulting shape is invalid");
                }
            }
            if (this->Refine.getValue()) {
                try {
                    TopoDS_Shape oldShape = res.getShape();
                    BRepBuilderAPI_RefineModel mkRefine(oldShape);
                    // We just built an element map above for the fuse, don't erase it for a refine.
                    res.setShape(mkRefine.Shape(), false);
                    ShapeHistory hist =
                            buildHistory(mkRefine, TopAbs_FACE, res.getShape(), oldShape);
                    for (auto& jt : history) {
                        jt = joinHistory(jt, hist);
                    }
                }
                catch (Standard_Failure&) {
                    // do nothing
                }
            }

            if (argumentsAreInCompound) {
                // combine histories of every child of source compound into one
                ShapeHistory overallHist;
                TopTools_IndexedMapOfShape facesOfCompound;
                TopAbs_ShapeEnum type = TopAbs_FACE;
                TopExp::MapShapes(compoundOfArguments.getShape(), type, facesOfCompound);
                for (std::size_t iChild = 0; iChild < history.size();
                     iChild++) {  // loop over children of source compound
                    // for each face of a child, find the inex of the face in compound, and assign
                    // the corresponding right-hand-size of the history
                    TopTools_IndexedMapOfShape facesOfChild;
                    TopExp::MapShapes(shapes[iChild].getShape(), type, facesOfChild);
                    for (std::pair<const int, ShapeHistory::List>& histitem :
                         history[iChild].shapeMap) {  // loop over elements of history - that is -
                                                      // over faces of the child of source compound
                        int iFaceInChild = histitem.first;
                        ShapeHistory::List& iFacesInResult = histitem.second;
                        const TopoDS_Shape& srcFace = facesOfChild(
                            iFaceInChild
                            + 1);  //+1 to convert our 0-based to OCC 1-bsed conventions
                        int iFaceInCompound = facesOfCompound.FindIndex(srcFace) - 1;
                        overallHist.shapeMap[iFaceInCompound] =
                            iFacesInResult;  // this may overwrite existing info if the same face is
                                             // used in several children of compound. This shouldn't
                                             // be a problem, because the histories should match
                                             // anyway...
                    }
                }
                history.clear();
                history.push_back(overallHist);
            }
            this->Shape.setValue(res);
            this->History.setValues(history);

            App::DocumentObject* link = Shapes.getValues()[0];
            copyMaterial(link);
            return Part::Feature::execute();
        }
        catch (Standard_Failure& e) {
            return new App::DocumentObjectExecReturn(e.GetMessageString());
        }
    }
    else {
        throw Base::CADKernelError("Not enough shape objects linked");
    }
}
