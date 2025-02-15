// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2024 Eric Price (CorvusCorax)                           *
 *                      <eric.price[at]tuebingen.mpg.de>                   *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

/**
  * FCBRepAlgoAPI provides a wrapper for various OCCT functions.
  */

#include <FCBRepAlgoAPI_BooleanOperation.h>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <BRepCheck_Analyzer.hxx>
#include <BRep_Builder.hxx>
#include <TopoDS_Shape.hxx>
#include <TopoDS_Iterator.hxx>
#include <Precision.hxx>
#include <FuzzyHelper.h>

FCBRepAlgoAPI_BooleanOperation::FCBRepAlgoAPI_BooleanOperation()
{
    SetRunParallel(Standard_True);
    SetNonDestructive(Standard_True);
}


FCBRepAlgoAPI_BooleanOperation::FCBRepAlgoAPI_BooleanOperation(const TopoDS_Shape& theS1,
                                               const TopoDS_Shape& theS2,
                                               const BOPAlgo_Operation theOperation)
: BRepAlgoAPI_BooleanOperation(theS1, theS2, theOperation)
{
    if (!BRepCheck_Analyzer(theS1).IsValid()){
        Standard_ConstructionError::Raise("Base shape is not valid for boolean operation");
    }
    if (! BRepCheck_Analyzer(theS2).IsValid()){
        Standard_ConstructionError::Raise("Tool shape is not valid for boolean operation");
    }
    setAutoFuzzy();
    SetRunParallel(Standard_True);
    SetNonDestructive(Standard_True);
}

void FCBRepAlgoAPI_BooleanOperation::setAutoFuzzy()
{
    FCBRepAlgoAPIHelper::setAutoFuzzy(this);
}


void FCBRepAlgoAPIHelper::setAutoFuzzy(BRepAlgoAPI_BooleanOperation* op) {
    Bnd_Box bounds;
    for (TopTools_ListOfShape::Iterator it(op->Arguments()); it.More(); it.Next())
        BRepBndLib::Add(it.Value(), bounds);
    for (TopTools_ListOfShape::Iterator it(op->Tools()); it.More(); it.Next())
        BRepBndLib::Add(it.Value(), bounds);
    op->SetFuzzyValue(Part::FuzzyHelper::getBooleanFuzzy() * sqrt(bounds.SquareExtent()) * Precision::Confusion());
}

void FCBRepAlgoAPIHelper::setAutoFuzzy(BRepAlgoAPI_BuilderAlgo* op) {
    Bnd_Box bounds;
    for (TopTools_ListOfShape::Iterator it(op->Arguments()); it.More(); it.Next())
        BRepBndLib::Add(it.Value(), bounds);
    op->SetFuzzyValue(Part::FuzzyHelper::getBooleanFuzzy() * sqrt(bounds.SquareExtent()) * Precision::Confusion());
}


void FCBRepAlgoAPI_BooleanOperation::RecursiveAddArguments(const TopoDS_Shape& theArgument) {
    TopoDS_Iterator it(theArgument);
    for (; it.More(); it.Next()) {
        if (it.Value().ShapeType() == TopAbs_COMPOUND) {
            RecursiveAddArguments(it.Value());
        } else {
            if (myArguments.IsEmpty()) {
                myArguments.Append(it.Value());
            } else {
                myTools.Append(it.Value());
            }
        }
    }
}

void FCBRepAlgoAPI_BooleanOperation::Build() {

    if (myOperation == BOPAlgo_CUT && myArguments.Size() == 1 && myTools.Size() == 1 && myTools.First().ShapeType() == TopAbs_COMPOUND) {
        TopTools_ListOfShape myOriginalArguments = myArguments;
        TopTools_ListOfShape myOriginalTools = myTools;
        TopTools_ListOfShape currentTools;
        TopTools_ListOfShape currentArguments;
        myArguments = currentArguments;
        myTools = currentTools;
        RecursiveAddArguments(myOriginalTools.First());
        if (!myTools.IsEmpty()) {
            myOperation = BOPAlgo_FUSE; // fuse tools together
            Build();
            myOperation = BOPAlgo_CUT; // restore
            myArguments = myOriginalArguments;
            if (IsDone()) {
                myTools.Append(myShape);
                Build(); // cut with fused tools
            }
            myTools = myOriginalTools; //restore
        } else { // there was less than 2 shapes in the compound
            myArguments = myOriginalArguments;
            myTools = myOriginalTools; //restore
            Build();
        }
    } else if (myOperation==BOPAlgo_CUT && myArguments.Size()==1 && myArguments.First().ShapeType() == TopAbs_COMPOUND) {
        TopTools_ListOfShape myOriginalArguments = myArguments;
        myShape = RecursiveCutCompound(myOriginalArguments.First());
        myArguments = myOriginalArguments;
    } else {
        BRepAlgoAPI_BooleanOperation::Build();
    }
}

TopoDS_Shape FCBRepAlgoAPI_BooleanOperation::RecursiveCutCompound(const TopoDS_Shape& theArgument) {
    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);
    TopoDS_Iterator it(theArgument);
    for (; it.More(); it.Next()) {
        TopTools_ListOfShape currentArguments;
        currentArguments.Append(it.Value());
        myArguments = currentArguments;
        Build();
        if (IsDone()) {
            builder.Add(comp, myShape);
        } else {
            return {};
        }
    }
    return comp;
}

