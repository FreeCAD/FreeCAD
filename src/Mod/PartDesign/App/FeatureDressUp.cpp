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
#include <BRep_Builder.hxx>
#include <BRep_Tool.hxx>
#include <TopExp.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>
#include <TopExp_Explorer.hxx>
#endif


#include <boost/algorithm/string/predicate.hpp>

#include "FeatureDressUp.h"
#include <Base/Console.h>
#include <App/Document.h>
#include <Base/Exception.h>
#include "Mod/Part/App/TopoShapeMapper.h"

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesign;

namespace PartDesign {


PROPERTY_SOURCE(PartDesign::DressUp, PartDesign::FeatureAddSub)

DressUp::DressUp()
{
    ADD_PROPERTY(Base,(nullptr));
    Placement.setStatus(App::Property::ReadOnly, true);

    ADD_PROPERTY_TYPE(SupportTransform,(false),"Base", App::Prop_None,
            "Include the base additive/subtractive shape when used in pattern features.\n"
            "If disabled, only the dressed part of the shape is used for patterning.");

    AddSubShape.setStatus(App::Property::Output, true);
}

short DressUp::mustExecute() const
{
    if (Base.getValue() && Base.getValue()->isTouched())
        return 1;
    return PartDesign::FeatureAddSub::mustExecute();
}

void DressUp::positionByBaseFeature()
{
    Part::Feature *base = static_cast<Part::Feature*>(BaseFeature.getValue());
    if (base && base->isDerivedFrom<Part::Feature>())
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
        if(base->isDerivedFrom<Part::Feature>()) {
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

void DressUp::getContinuousEdges(Part::TopoShape TopShape, std::vector< std::string >& SubNames) {

    std::vector< std::string > FaceNames;

    getContinuousEdges(TopShape, SubNames, FaceNames);
}

void DressUp::getContinuousEdges(Part::TopoShape TopShape, std::vector< std::string >& SubNames, std::vector< std::string >& FaceNames) {

    TopTools_IndexedMapOfShape mapOfEdges;
    TopTools_IndexedDataMapOfShapeListOfShape mapEdgeFace;
    TopExp::MapShapesAndAncestors(TopShape.getShape(), TopAbs_EDGE, TopAbs_FACE, mapEdgeFace);
    TopExp::MapShapes(TopShape.getShape(), TopAbs_EDGE, mapOfEdges);

    unsigned int i = 0;
    while(i < SubNames.size())
    {
        std::string aSubName = static_cast<std::string>(SubNames.at(i));

        if (aSubName.compare(0, 4, "Edge") == 0) {
            TopoDS_Edge edge = TopoDS::Edge(TopShape.getSubShape(aSubName.c_str()));
            const TopTools_ListOfShape& los = mapEdgeFace.FindFromKey(edge);

            if(los.Extent() != 2)
            {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            const TopoDS_Shape& face1 = los.First();
            const TopoDS_Shape& face2 = los.Last();
            GeomAbs_Shape cont = BRep_Tool::Continuity(TopoDS::Edge(edge),
                                                       TopoDS::Face(face1),
                                                       TopoDS::Face(face2));
            if (cont != GeomAbs_C0) {
                SubNames.erase(SubNames.begin()+i);
                continue;
            }

            i++;
        }
        else if(aSubName.compare(0, 4, "Face") == 0) {
            TopoDS_Face face = TopoDS::Face(TopShape.getSubShape(aSubName.c_str()));

            TopTools_IndexedMapOfShape mapOfFaces;
            TopExp::MapShapes(face, TopAbs_EDGE, mapOfFaces);

            for(int j = 1; j <= mapOfFaces.Extent(); ++j) {
                TopoDS_Edge edge = TopoDS::Edge(mapOfFaces.FindKey(j));

                int id = mapOfEdges.FindIndex(edge);

                std::stringstream buf;
                buf << "Edge";
                buf << id;

                if (std::ranges::find(SubNames, buf.str()) == SubNames.end())
                {
                    SubNames.push_back(buf.str());
                }

            }

            FaceNames.emplace_back(aSubName.c_str());
            SubNames.erase(SubNames.begin()+i);
        }
        // empty name or any other sub-element
        else {
            SubNames.erase(SubNames.begin()+i);
        }
    }
}

std::vector<TopoShape> DressUp::getContinuousEdges(const TopoShape& shape)
{
    std::vector<TopoShape> ret;
    std::unordered_set<TopoDS_Shape, Part::ShapeHasher, Part::ShapeHasher> shapeSet;

    auto addEdge = [&](const TopoDS_Shape& subshape, const std::string& ref) {
        if (!shapeSet.insert(subshape).second) {
            return;
        }

        auto faces = shape.findAncestorsShapes(subshape, TopAbs_FACE);
        if (faces.size() != 2) {
            FC_WARN(getFullName() << ": skip edge " << ref << " with less two attaching faces");
            return;
        }
        const TopoDS_Shape& face1 = faces.front();
        const TopoDS_Shape& face2 = faces.back();
        GeomAbs_Shape cont =
            BRep_Tool::Continuity(TopoDS::Edge(subshape), TopoDS::Face(face1), TopoDS::Face(face2));
        if (cont != GeomAbs_C0) {
            FC_WARN(getFullName() << ": skip edge " << ref << " that is not C0 continuous");
            return;
        }
        ret.push_back(subshape);
    };

    for (const auto& v : Base.getShadowSubs()) {
        TopoDS_Shape subshape;
        const auto& ref = v.newName.size() ? v.newName : v.oldName;
        subshape = shape.getSubShape(ref.c_str(), true);
        if (subshape.IsNull()) {
            FC_THROWM(Base::CADKernelError, "Invalid edge link: " << ref);
        }

        if (subshape.ShapeType() == TopAbs_EDGE) {
            addEdge(subshape, ref);
        }
        else if (subshape.ShapeType() == TopAbs_FACE || subshape.ShapeType() == TopAbs_WIRE) {
            for (TopExp_Explorer exp(subshape, TopAbs_EDGE); exp.More(); exp.Next()) {
                addEdge(exp.Current(), std::string());
            }
        }
        else {
            FC_WARN(getFullName() << ": skip invalid shape '" << ref << "' with type "
                                  << TopoShape::shapeName(subshape.ShapeType()));
        }
    }
    return ret;
}

std::vector<TopoShape> DressUp::getFaces(const TopoShape& shape)
{
    std::vector<TopoShape> ret;
    const auto& vals = Base.getSubValues();
    const auto& subs = Base.getShadowSubs();
    size_t i = 0;
    for (auto& val : vals) {
        if (!boost::starts_with(val, "Face")) {
            continue;
        }
        auto& sub = subs[i++];
        auto& ref = sub.newName.size() ? sub.newName : val;
        TopoShape subshape;
        try {
            subshape = shape.getSubTopoShape(ref.c_str());
        }
        catch (...) {
        }

        if (subshape.isNull()) {
            FC_ERR(getFullName() << ": invalid face reference '" << ref << "'");
            throw Part::NullShapeException("Invalid Invalid face link");
        }

        if (subshape.shapeType() != TopAbs_FACE) {
            FC_WARN(getFullName() << ": skip invalid shape '" << ref << "' with type "
                                  << subshape.shapeName());
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
        if (BaseFeature.getValue()
                && Base.getValue()
                && Base.getValue() != BaseFeature.getValue()) {

            auto subs = Base.getSubValues(false);
            auto shadows = Base.getShadowSubs();
            Base.setValue (BaseFeature.getValue(),std::move(subs),std::move(shadows));
        }
    } else if (prop == &Base) {
        // track the vice-versa changes
        if (BaseFeature.getValue() && Base.getValue() != BaseFeature.getValue()) {
            BaseFeature.setValue (Base.getValue());
        }
    } else if (prop == &Shape || prop == &SupportTransform) {
        if (!getDocument()->testStatus(App::Document::Restoring) &&
            !getDocument()->isPerformingTransaction())
        {
            // AddSubShape in DressUp acts as a shape cache. And here we shall
            // invalidate the cache upon changes in Shape. Other features
            // (currently only feature Transformed) shall call getAddSubShape()
            // to rebuild the cache. This allow us to perform expensive
            // calculation of AddSubShape only when necessary.
            AddSubShape.setValue(Part::TopoShape());
        }
    }

    Feature::onChanged(prop);
}

void DressUp::getAddSubShape(Part::TopoShape &addShape, Part::TopoShape &subShape)
{
    Part::TopoShape res = AddSubShape.getShape();

    if(res.isNull()) {
        try {
            std::vector<Part::TopoShape> shapes;
            Part::TopoShape shape = Shape.getShape();
            shape.setPlacement(Base::Placement());

            FeatureAddSub *base = nullptr;
            if(SupportTransform.getValue()) {
                // SupportTransform means transform the support together with
                // the dressing. So we need to find the previous support
                // feature (which must be of type FeatureAddSub), and skipping
                // any consecutive DressUp in-between.
                for(Feature *current=this; ;current=static_cast<DressUp*>(base)) {
                    base = freecad_cast<FeatureAddSub*>(current->getBaseObject(true));
                    if(!base)
                        FC_THROWM(Base::CADKernelError,
                                "Cannot find additive or subtractive support for " << getFullName());
                    if(!base->isDerivedFrom<DressUp>())
                        break;
                }
            }

            Part::TopoShape baseShape;
            if(base) {
                baseShape = base->getBaseTopoShape(true);
                baseShape.move(base->getLocation().Inverted());
                if (base->getAddSubType() == Additive) {
                    if(!baseShape.isNull() && baseShape.hasSubShape(TopAbs_SOLID))
                        shapes.emplace_back(shape.makeElementCut(baseShape.getShape()));
                    else
                        shapes.push_back(shape);
                } else {
                    BRep_Builder builder;
                    TopoDS_Compound comp;
                    builder.MakeCompound(comp);
                    // push an empty compound to indicate null additive shape
                    shapes.emplace_back(comp);
                    if(!baseShape.isNull() && baseShape.hasSubShape(TopAbs_SOLID))
                        shapes.emplace_back(baseShape.makeElementCut(shape.getShape()));
                    else
                        shapes.push_back(shape);
                }
            } else {
                baseShape = getBaseTopoShape();
                baseShape.move(getLocation().Inverted());
                shapes.emplace_back(shape.makeElementCut(baseShape.getShape()));
                shapes.emplace_back(baseShape.makeElementCut(shape.getShape()));
            }

            // Make a compound to contain both additive and subtractive shape,
            // bceause a dressing (e.g. a fillet) can either be additive or
            // subtractive. And the dressup feature can contain mixture of both.
            AddSubShape.setValue(Part::TopoShape().makeElementCompound(shapes));

        } catch (Standard_Failure &e) {
            FC_THROWM(Base::CADKernelError, "Failed to calculate AddSub shape: "
                    << e.GetMessageString());
        }
        res = AddSubShape.getShape();
    }

    if(res.isNull())
        throw Part::NullShapeException("Null AddSub shape");

    if(res.getShape().ShapeType() != TopAbs_COMPOUND) {
        addShape = res;
    } else {
        int count = res.countSubShapes(TopAbs_SHAPE);
        if(!count)
            throw Part::NullShapeException("Null AddSub shape");
        if(count) {
            Part::TopoShape s = res.getSubTopoShape(TopAbs_SHAPE, 1);
            if(!s.isNull() && s.hasSubShape(TopAbs_SOLID))
                addShape = s;
        }
        if(count > 1) {
            Part::TopoShape s = res.getSubTopoShape(TopAbs_SHAPE, 2);
            if(!s.isNull() && s.hasSubShape(TopAbs_SOLID))
                subShape = s;
        }
    }
}

}
