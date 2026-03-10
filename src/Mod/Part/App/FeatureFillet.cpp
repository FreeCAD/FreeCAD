// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <FCConfig.h>

#include <BRepFilletAPI_MakeFillet.hxx>
#include <ChFiDS_ErrorStatus.hxx>
#include <Precision.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopTools_IndexedMapOfShape.hxx>


#include <Base/Exception.h>

#include "FeatureFillet.h"
#include "TopoShapeOpCode.h"


using namespace Part;

PROPERTY_SOURCE(Part::Fillet, Part::FilletBase)

Fillet::Fillet() = default;

App::DocumentObjectExecReturn* Fillet::execute()
{
    App::DocumentObject* link = Base.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }


    try {
#if defined(__GNUC__) && defined(FC_OS_LINUX)
        Base::SignalException se;
#endif
        TopoShape baseTopoShape
            = Feature::getTopoShape(link, ShapeOption::ResolveLink | ShapeOption::Transform);
        auto baseShape = baseTopoShape.getShape();
        BRepFilletAPI_MakeFillet mkFillet(baseShape);
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(baseShape, TopAbs_EDGE, mapOfShape);
        TopTools_IndexedMapOfShape mapOfEdges;
        TopExp::MapShapes(baseShape, TopAbs_EDGE, mapOfEdges);
        std::vector<Part::FilletElement> edges = Edges.getValues();
        std::string fullErrMsg;

        const auto& vals = EdgeLinks.getSubValues(true);
        const auto& subs = EdgeLinks.getShadowSubs();
        if (subs.size() != (size_t)Edges.getSize()) {
            return new App::DocumentObjectExecReturn("Edge link size mismatch");
        }
        size_t i = 0;
        for (const auto& info : edges) {
            auto& sub = subs[i];
            auto& ref = sub.newName.empty() ? vals[i] : sub.newName;
            auto& oldName = sub.oldName.empty() ? "" : sub.oldName;
            ++i;

            if (Data::hasMissingElement(ref.c_str()) || Data::hasMissingElement(oldName.c_str())) {
                fullErrMsg.append("Missing edge link: ");
                fullErrMsg.append(ref);
                fullErrMsg.append("\n");

                auto removeIt = std::remove(edges.begin(), edges.end(), info);
                edges.erase(removeIt, edges.end());

                continue;
            }

            // Toponaming project March 2024:  Replaced this code because it wouldn't work:
            //            TopoDS_Shape edge;
            //            try {
            //                edge = baseTopoShape.getSubShape(ref.c_str());
            //            }catch(...){}
            auto id = Data::MappedName(ref.c_str()).toIndexedName().getIndex();
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfEdges.FindKey(id));

            if (edge.IsNull()) {
                return new App::DocumentObjectExecReturn("Invalid edge link");
            }

            double radius1 = info.radius1;
            double radius2 = info.radius2;
            mkFillet.Add(radius1, radius2, TopoDS::Edge(edge));
        }

        if (!fullErrMsg.empty()) {
            return new App::DocumentObjectExecReturn(fullErrMsg);
        }
        Edges.setValues(edges);

        // Explicitly build so OCC diagnostics are available on failure.
        mkFillet.Build();
        if (!mkFillet.IsDone()) {
            int nFaulty = mkFillet.NbFaultyContours();
            if (nFaulty > 0) {
                int ic = mkFillet.FaultyContour(1);
                switch (mkFillet.StripeStatus(ic)) {
                    case ChFiDS_WalkingFailure:
                        return new App::DocumentObjectExecReturn(
                            "Fillet radius too large: cannot trace surface along edge. Reduce the "
                            "radius."
                        );
                    case ChFiDS_StartsolFailure:
                        return new App::DocumentObjectExecReturn(
                            "Fillet conflict at shared vertex: adjacent radii overlap. "
                            "Reduce the radius or fillet edges separately."
                        );
                    case ChFiDS_TwistedSurface:
                        return new App::DocumentObjectExecReturn(
                            "Fillet radius too large: surface would self-intersect. Reduce the "
                            "radius."
                        );
                    default:
                        break;
                }
                return new App::DocumentObjectExecReturn(
                    "Fillet failed on selected edge(s). Reduce the radius or fillet edges "
                    "individually."
                );
            }
            if (mkFillet.NbFaultyVertices() > 0) {
                return new App::DocumentObjectExecReturn(
                    "Fillet failed at shared corner: adjacent radii overlap. Reduce the radius."
                );
            }
            if (mkFillet.HasResult()) {
                return new App::DocumentObjectExecReturn(
                    "Fillet partially applied: some edges failed. Reduce the radius or fillet "
                    "individually."
                );
            }
            return new App::DocumentObjectExecReturn(
                "Fillet failed: radius exceeds adjacent face width. "
                "Reduce the radius or select fewer edges."
            );
        }

        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        }

        TopoShape res(0);
        this->Shape.setValue(res.makeElementShape(mkFillet, baseTopoShape, Part::OpCodes::Fillet));
        return Part::FilletBase::execute();
    }
    catch (Standard_Failure& e) {
        std::string msg = e.GetMessageString();
        if (msg.find("command not done") != std::string::npos) {
            return new App::DocumentObjectExecReturn(
                "Fillet failed: radius too large for selected edge(s). "
                "Reduce the radius or select fewer edges."
            );
        }
        return new App::DocumentObjectExecReturn(msg.c_str());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making fillets");
    }
}
