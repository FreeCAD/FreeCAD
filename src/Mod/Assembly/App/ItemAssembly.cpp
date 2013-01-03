/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
# include <BRep_Builder.hxx>
# include <TopoDS_Compound.hxx>
#endif

#include <Base/Placement.h>
#include <Base/Exception.h>

#include "ItemAssembly.h"


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ItemAssembly, Assembly::Item)

ItemAssembly::ItemAssembly()
{
    ADD_PROPERTY(Items,(0));
    ADD_PROPERTY(Annotations,(0));
}

short ItemAssembly::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ItemAssembly::execute(void)
{

    return App::DocumentObject::StdReturn;
}

TopoDS_Shape ItemAssembly::getShape(void) const 
{
    std::vector<TopoDS_Shape> s;
    std::vector<App::DocumentObject*> obj = Items.getValues();

    std::vector<App::DocumentObject*>::iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(Assembly::Item::getClassTypeId())) {
            TopoDS_Shape aShape = static_cast<Assembly::Item*>(*it)->getShape();
            if (!aShape.IsNull())
                s.push_back(aShape);
        }
    }

    if (s.size() > 0) {
        TopoDS_Compound aRes = TopoDS_Compound();
        BRep_Builder aBuilder = BRep_Builder();
        aBuilder.MakeCompound(aRes);

        for (std::vector<TopoDS_Shape>::iterator it = s.begin(); it != s.end(); ++it) {

            aBuilder.Add(aRes, *it);
        }
        //if (aRes.IsNull())
        //    throw Base::Exception("Resulting shape is invalid");
        return aRes;
    }
    // set empty shape
    return TopoDS_Compound();
    
}

}