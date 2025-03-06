/***************************************************************************
 *   Copyright (c) 2013 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Standard_Failure.hxx>
# include <TopoDS_Compound.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include "FeatureCompound.h"


using namespace Part;

PROPERTY_SOURCE(Part::Compound, Part::Feature)

Compound::Compound()
{
    ADD_PROPERTY(Links,(nullptr));
    Links.setSize(0);
}

Compound::~Compound() = default;

short Compound::mustExecute() const
{
    if (Links.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Compound::execute()
{
    try {
        // avoid duplicates without changing the order
        // See also ViewProviderCompound::updateData
        std::set<DocumentObject*> tempLinks;

        std::vector<TopoShape> shapes;
        for (auto obj : Links.getValues()) {
            if (!tempLinks.insert(obj).second) {
                continue;
            }
            auto sh = Feature::getTopoShape(obj);
            if (!sh.isNull()) {
                shapes.push_back(sh);
            }
        }
        this->Shape.setValue(TopoShape().makeElementCompound(shapes));
        if (Links.getSize() > 0) {
            App::DocumentObject* link = Links.getValues()[0];
            copyMaterial(link);
        }
        return Part::Feature::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}

////////////////////////////////////////////////////////////////////////

PROPERTY_SOURCE(Part::Compound2, Part::Compound)

Compound2::Compound2() {
    Shape.setStatus(App::Property::Transient,true);
}

void Compound2::onDocumentRestored() {
    Base::Placement pla = Placement.getValue();
    auto res = execute();
    delete res;
    Placement.setValue(pla);
}
