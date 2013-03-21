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
# include <TopoDS_Solid.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
#endif


#include <Base/Exception.h>
#include "Feature.h"



namespace PartDesign {


PROPERTY_SOURCE(PartDesign::Feature,Part::Feature)

Feature::Feature()
{
}

TopoDS_Shape Feature::getSolid(const TopoDS_Shape& shape)
{
    if (shape.IsNull())
        Standard_Failure::Raise("Shape is null");
    TopExp_Explorer xp;
    xp.Init(shape,TopAbs_SOLID);
    for (;xp.More(); xp.Next()) {
        return xp.Current();
    }

    return TopoDS_Shape();
}

const gp_Pnt Feature::getPointFromFace(const TopoDS_Face& f)
{
    if (!f.Infinite()) {
        TopExp_Explorer exp;
        exp.Init(f, TopAbs_VERTEX);
        if (exp.More())
            return BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()));
        // Else try the other method
    }

    // TODO: Other method, e.g. intersect X,Y,Z axis with the (unlimited?) face?
    // Or get a "corner" point if the face is limited?
    throw Base::Exception("getPointFromFace(): Not implemented yet for this case");
}

}
