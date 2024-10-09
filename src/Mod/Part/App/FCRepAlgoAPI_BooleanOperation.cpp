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

#include <FCRepAlgoAPI_Section.h>
#include <BRepBndLib.hxx>
#include <Bnd_Box.hxx>
#include <TopoDS_Shape.hxx>
#include <Precision.hxx>
#include <App/Application.h>

FCRepAlgoAPI_BooleanOperation::FCRepAlgoAPI_BooleanOperation()
:
  BRepAlgoAPI_BooleanOperation()
{
    SetFuzzyValue(FCRepAlgoAPI_BooleanOperation::getDefaultFuzzyValue(0.0));
}


FCRepAlgoAPI_BooleanOperation::FCRepAlgoAPI_BooleanOperation(const TopoDS_Shape& theS1,
                                               const TopoDS_Shape& theS2,
                                               const BOPAlgo_Operation theOperation)
: BRepAlgoAPI_BooleanOperation(theS1, theS2, theOperation)
{
    Bnd_Box bounds;
    BRepBndLib::Add(theS1, bounds);
    BRepBndLib::Add(theS2, bounds);
    SetFuzzyValue(FCRepAlgoAPI_BooleanOperation::getDefaultFuzzyValue(bounds.SquareExtent()));
}
  
double FCRepAlgoAPI_BooleanOperation::getDefaultFuzzyValue(const double size) {
    const double DefaultFuzzyBooster=1.0;

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Part/Boolean");
    return hGrp->GetFloat("BooleanFuzzy",DefaultFuzzyBooster) * size * Precision::Confusion();
}
