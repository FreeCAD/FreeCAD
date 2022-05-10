/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
#endif

#include "Geometry.h"

#include "FeatureGeometrySet.h"


using namespace Part;


PROPERTY_SOURCE(Part::FeatureGeometrySet, Part::Feature)


FeatureGeometrySet::FeatureGeometrySet()
{
    ADD_PROPERTY(GeometrySet,(nullptr));
}


App::DocumentObjectExecReturn *FeatureGeometrySet::execute(void)
{
    TopoShape result;

    const std::vector<Geometry*> &Geoms = GeometrySet.getValues();

    bool first = true;
    for(std::vector<Geometry*>::const_iterator it=Geoms.begin();it!=Geoms.end();++it){
        TopoDS_Shape sh = (*it)->toShape();
        if (first) {
            first = false;
            result.setShape(sh);
        }
        else {
            result.setShape(result.fuse(sh));
        }
    }
    
    Shape.setValue(result);

    return App::DocumentObject::StdReturn;
}
