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
# include <TopoDS_Compound.hxx>
# include <TopExp.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <Standard_Failure.hxx>
#endif


#include "FeatureCompound.h"


using namespace Part;


PROPERTY_SOURCE(Part::Compound, Part::Feature)

Compound::Compound()
{
    ADD_PROPERTY(Links,(nullptr));
    Links.setSize(0);
}

Compound::~Compound()
{
}

short Compound::mustExecute() const
{
    if (Links.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Compound::execute(void)
{
    try {
        // avoid duplicates without changing the order
        // See also ViewProviderCompound::updateData
        std::set<DocumentObject*> tempLinks;

#ifdef FC_NO_ELEMENT_MAP
        std::vector<ShapeHistory> history;
        int countFaces = 0;

        BRep_Builder builder;
        TopoDS_Compound comp;
        builder.MakeCompound(comp);

        const std::vector<DocumentObject*>& links = Links.getValues();
        std::vector<TopoShape> shapes;
        for (std::vector<DocumentObject*>::const_iterator it = links.begin(); it != links.end(); ++it) {
            if (*it) {
                auto pos = tempLinks.insert(*it);
                if (pos.second) {
                    const TopoDS_Shape& sh = Feature::getShape(*it);
                    if (!sh.IsNull()) {
                        builder.Add(comp, sh);
                        TopTools_IndexedMapOfShape faceMap;
                        TopExp::MapShapes(sh, TopAbs_FACE, faceMap);
                        ShapeHistory hist;
                        hist.type = TopAbs_FACE;
                        for (int i=1; i<=faceMap.Extent(); i++) {
                            hist.shapeMap[i-1].push_back(countFaces++);
                        }
                        history.push_back(hist);
                    }
                }
            }
        }

        this->Shape.setValue(comp);

        // make sure the 'PropertyShapeHistory' is not safed in undo/redo (#0001889)
        PropertyShapeHistory prop;
        prop.setValues(history);
        prop.setContainer(this);
        prop.touch();
#else
        std::vector<TopoShape> shapes;
        for(auto obj : Links.getValues()) {
            if(!tempLinks.insert(obj).second)
                continue;
            auto sh = Feature::getTopoShape(obj);
            if(!sh.isNull())
                shapes.push_back(sh);
        }
        this->Shape.setValue(TopoShape().makECompound(shapes));
#endif

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
