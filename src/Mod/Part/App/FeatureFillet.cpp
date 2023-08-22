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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <BRepFilletAPI_MakeFillet.hxx>
# include <Precision.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
#endif

#include <Base/Exception.h>

#include "FeatureFillet.h"


using namespace Part;

PROPERTY_SOURCE(Part::Fillet, Part::FilletBase)

Fillet::Fillet() = default;

App::DocumentObjectExecReturn *Fillet::execute()
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    auto baseShape = Feature::getShape(link);

    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        BRepFilletAPI_MakeFillet mkFillet(baseShape);
        TopTools_IndexedMapOfShape mapOfShape;
        TopExp::MapShapes(baseShape, TopAbs_EDGE, mapOfShape);

        std::vector<FilletElement> values = Edges.getValues();
        for (const auto & value : values) {
            int id = value.edgeid;
            double radius1 = value.radius1;
            double radius2 = value.radius2;
            const TopoDS_Edge& edge = TopoDS::Edge(mapOfShape.FindKey(id));
            mkFillet.Add(radius1, radius2, edge);
        }

        TopoDS_Shape shape = mkFillet.Shape();
        if (shape.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");

        //shapefix re #4285
        //https://www.forum.freecad.org/viewtopic.php?f=3&t=43890&sid=dae2fa6fda71670863a103b42739e47f
        TopoShape* ts = new TopoShape(shape);
        double minTol = 2.0 * Precision::Confusion();
        double maxTol = 4.0 * Precision::Confusion();
        bool rc = ts->fix(Precision::Confusion(), minTol, maxTol);
        if (rc) {
            shape = ts->getShape();
        }
        delete ts;

        ShapeHistory history = buildHistory(mkFillet, TopAbs_FACE, shape, baseShape);
        this->Shape.setValue(shape);

        // make sure the 'PropertyShapeHistory' is not safed in undo/redo (#0001889)
        PropertyShapeHistory prop;
        prop.setValue(history);
        prop.setContainer(this);
        prop.touch();

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
    catch (...) {
        return new App::DocumentObjectExecReturn("A fatal error occurred when making fillets");
    }
}
