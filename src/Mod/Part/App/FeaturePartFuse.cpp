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
#endif


#include "FeaturePartFuse.h"

#include <Base/Exception.h>

using namespace Part;

PROPERTY_SOURCE(Part::Fuse, Part::Boolean)


Fuse::Fuse(void)
{
}

TopoDS_Shape Fuse::runOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a fuse operation:
    BRepAlgoAPI_Fuse mkFuse(base, tool);
    // Let's check if the fusion has been successful
    if (!mkFuse.IsDone()) 
        throw Base::Exception("Fusion failed");
    return mkFuse.Shape();
}

// ----------------------------------------------------

PROPERTY_SOURCE(Part::MultiFuse, Part::Feature)


MultiFuse::MultiFuse(void)
{
    ADD_PROPERTY(Shapes,(0));
    Shapes.setSize(0);
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
        TopoDS_Shape res = s.front();
        for (std::vector<TopoDS_Shape>::iterator it = s.begin()+1; it != s.end(); ++it) {
            // Let's call algorithm computing a fuse operation:
            BRepAlgoAPI_Fuse mkFuse(res, *it);
            // Let's check if the fusion has been successful
            if (!mkFuse.IsDone()) 
                throw Base::Exception("Fusion failed");
            res = mkFuse.Shape();
        }
        if (res.IsNull())
            throw Base::Exception("Resulting shape is invalid");
        this->Shape.setValue(res);
    }
    else {
        throw Base::Exception("Not enough shape objects linked");
    }

    return App::DocumentObject::StdReturn;
}
