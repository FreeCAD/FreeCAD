/***************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder)<realthunder.dev@gmail.com>*
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
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <gp_Pln.hxx>
# include <gp_Dir.hxx>
# include <gp_Ax1.hxx>
#endif


#include "FeatureGenericPattern.h"
#include <Base/Exception.h>
#include <Mod/Part/App/TopoShape.h>

using namespace PartDesign;

PROPERTY_SOURCE(PartDesign::GenericPattern, PartDesign::Transformed)

GenericPattern::GenericPattern()
{
    ADD_PROPERTY_TYPE(MatrixList,(),"GenericPattern",(App::PropertyType)(App::Prop_None), 0);
}

short GenericPattern::mustExecute() const
{
    if (MatrixList.isTouched())
        return 1;
    return Transformed::mustExecute();
}

std::list<gp_Trsf> GenericPattern::getTransformations(const std::vector<Part::TopoShape> &)
{
    std::list<gp_Trsf> res;
    for (auto & m : MatrixList.getValues())
        res.push_back(TopoShape::convert(m));

    return res;
}
