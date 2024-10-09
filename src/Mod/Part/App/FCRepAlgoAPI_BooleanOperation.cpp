/***************************************************************************
 *   Copyright (c) 2024 Eric Price (CorvusCorax) <eric.price@tuebingen.mpg.de>
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
/**
  * FCRepAlgoAPI provides a wrapper for various OCCT functions.
  */

#include <FCRepAlgoAPI_BooleanOperation.h>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>
#include <FuzzyHelper.h>

FCRepAlgoAPI_BooleanOperation::FCRepAlgoAPI_BooleanOperation()
{
    SetFuzzyValue(Part::FuzzyHelper::getDefaultFuzzyValue(0.0));
    SetRunParallel(Standard_True);
}


FCRepAlgoAPI_BooleanOperation::FCRepAlgoAPI_BooleanOperation(const TopoDS_Shape& theS1,
                                               const TopoDS_Shape& theS2,
                                               const BOPAlgo_Operation theOperation)
: BRepAlgoAPI_BooleanOperation(theS1, theS2, theOperation)
{
    Bnd_Box bounds;
    BRepBndLib::Add(theS1, bounds);
    BRepBndLib::Add(theS2, bounds);
    SetFuzzyValue(Part::FuzzyHelper::getDefaultFuzzyValue(bounds.SquareExtent()));
    SetRunParallel(Standard_True);
}
  
