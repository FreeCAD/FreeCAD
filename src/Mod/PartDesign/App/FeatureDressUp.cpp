/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <BRep_Tool.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#endif


#include <boost/algorithm/string/predicate.hpp>

#include "FeatureDressUp.h"
#include <Base/Console.h>
#include <Base/Exception.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::DressUp, PartDesign::Feature)

DressUp::DressUp()
{
    ADD_PROPERTY(Base,(0));
    Placement.setStatus(App::Property::ReadOnly, true);
}

short DressUp::mustExecute() const
{
    if (Base.getValue() && Base.getValue()->isTouched())
        return 1;
    return PartDesign::Feature::mustExecute();
}


void DressUp::positionByBaseFeature(void)
{
    Part::Feature *base = static_cast<Part::Feature*>(BaseFeature.getValue());
    if (base && base->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        this->Placement.setValue(base->Placement.getValue());
}

Part::Feature *DressUp::getBaseObject(bool silent) const
{
    Part::Feature *rv = Feature::getBaseObject(/* silent = */ true);
    if (rv) {
        return rv;
    }

    const char* err = nullptr;
    App::DocumentObject* base = Base.getValue();
    if (base) {
        if(base->isDerivedFrom(Part::Feature::getClassTypeId())) {
            rv = static_cast<Part::Feature*>(base);
        } else {
            err = "Linked object is not a Part object";
        }
    } else {
        err = "No Base object linked";
    }

    if (!silent && err) {
        throw Base::RuntimeError(err);
    }

    return rv;
}

std::vector<TopoShape> DressUp::getContiniusEdges(const TopoShape &shape) {
    std::vector<TopoShape> ret;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(shape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);

    for(auto &ref : Base.getSubValues(true)) {
        TopoDS_Shape subshape;
        try {
            subshape = shape.getSubShape(ref.c_str());
        }catch(...){}
        if(subshape.IsNull()) {
            throw Part::NullShapeException("Invalid edge link");
        }

        if (subshape.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge edge = TopoDS::Edge(subshape);
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if(los.Extent() != 2) {
                FC_WARN(getFullName() << ": skip edge '" 
                        << ref << "' with less two attaching faces");
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                FC_WARN(getFullName() << ": skip edge '"
                       << ref << "' that is not C0 continuous");
                continue;
            }
            ret.push_back(subshape);

        } else if(subshape.ShapeType() == TopAbs_FACE) {
            for(TopExp_Explorer exp(subshape,TopAbs_EDGE);exp.More();exp.Next())
                ret.push_back(exp.Current());
        } else
            FC_WARN(getFullName() << ": skip invalid shape '"
                    << ref << "' with type " << TopoShape::shapeName(subshape.ShapeType()));
    }
    return ret;
}

std::vector<TopoShape> DressUp::getFaces(const TopoShape &shape) {
    std::vector<TopoShape> ret;
    const auto &vals = Base.getSubValues();
    const auto &subs = Base.getShadowSubs();
    size_t i=0;
    for(auto &val : vals) {
        if(!boost::starts_with(val,"Face"))
            continue;
        auto &sub = subs[i++];
        auto &ref = sub.first.size()?sub.first:val;
        TopoShape subshape;
        try {
            subshape = shape.getSubTopoShape(ref.c_str());
        }catch(...){}
        if(subshape.isNull()) {
            FC_ERR(getFullName() << ": invalid face reference '" << ref << "'");
            throw Part::NullShapeException("Invalid Invalid face link");
        }

        if(subshape.shapeType() != TopAbs_FACE) {
            FC_WARN(getFullName() << ": skip invalid shape '"
                    << ref << "' with type " << subshape.shapeName());
            continue;
        }
        ret.push_back(subshape);
    }
    return ret;
}

void DressUp::onChanged(const App::Property* prop)
{
    // the BaseFeature property should track the Base and vice-versa as long as
    // the feature is inside a body (aka BaseFeature is nonzero)
    if (prop == &BaseFeature) {
        if (BaseFeature.getValue() && Base.getValue() != BaseFeature.getValue()) {
            auto subs = Base.getSubValues();
            auto shadows = Base.getShadowSubs();
            for(auto &sub : subs) {
                if(sub.size() && sub[0]!='?')
                    sub = std::string("?") + sub;
            }
            Base.setValue (BaseFeature.getValue(),std::move(subs),std::move(shadows));
        }
    } else if (prop == &Base) {
        // track the vice-versa changes
        if (BaseFeature.getValue() && Base.getValue() != BaseFeature.getValue()) {
            BaseFeature.setValue (Base.getValue());
        }
    }

    Feature::onChanged(prop);
}

}
