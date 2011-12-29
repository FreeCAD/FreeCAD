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
# include <cmath>
# include <gp_Trsf.hxx>
# include <BRepOffsetAPI_MakeOffset.hxx>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_Transform.hxx>
# include <BRepOffsetAPI_ThruSections.hxx>
# include <BRepPrimAPI_MakePrism.hxx>
# include <Precision.hxx>
# include <ShapeFix_Wire.hxx>
# include <TopoDS.hxx>
# include <TopExp_Explorer.hxx>
#endif


#include "FeatureExtrusion.h"
#include <Base/Tools.h>
#include <Base/Exception.h>


using namespace Part;


PROPERTY_SOURCE(Part::Extrusion, Part::Feature)

Extrusion::Extrusion()
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Dir,(Base::Vector3f(0.0f,0.0f,1.0f)));
    ADD_PROPERTY(Solid,(false));
    ADD_PROPERTY(TaperAngle,(0.0f));
}

short Extrusion::mustExecute() const
{
    if (Base.isTouched() ||
        Dir.isTouched() ||
        Solid.isTouched() ||
        TaperAngle.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *Extrusion::execute(void)
{
    App::DocumentObject* link = Base.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");
    if (!link->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()))
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    Part::Feature *base = static_cast<Part::Feature*>(Base.getValue());

    Base::Vector3f v = Dir.getValue();
    gp_Vec vec(v.x,v.y,v.z);
    float taperAngle = TaperAngle.getValue();
    bool makeSolid = Solid.getValue();

    try {
        if (std::fabs(taperAngle) >= Precision::Confusion()) {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
            Base::SignalException se;
#endif
            double distance = std::tan(Base::toRadians(taperAngle)) * vec.Magnitude();
            const TopoDS_Shape& shape = base->Shape.getValue();
            bool isWire = (shape.ShapeType() == TopAbs_WIRE);
            if (!isWire)
                return new App::DocumentObjectExecReturn("Only wires supported");

            std::list<TopoDS_Wire> wire_list;
            BRepOffsetAPI_MakeOffset mkOffset;
#if 1 //OCC_HEX_VERSION < 0x060502
            // The input wire may have erorrs in its topology
            // and thus may cause a crash in the Perfrom() method
            // See also:
            // http://www.opencascade.org/org/forum/thread_17640/
            // http://www.opencascade.org/org/forum/thread_12012/
            ShapeFix_Wire aFix;
            aFix.Load(TopoDS::Wire(shape));
            aFix.FixReorder();
            aFix.FixConnected();
            aFix.FixClosed();
            mkOffset.AddWire(aFix.Wire());
            wire_list.push_back(aFix.Wire());
#else
            mkOffset.AddWire(TopoDS::Wire(shape));
#endif
            mkOffset.Perform(distance);

            gp_Trsf mat;
            mat.SetTranslation(vec);
            BRepBuilderAPI_Transform mkTransform(mkOffset.Shape(),mat);
            wire_list.push_back(TopoDS::Wire(mkTransform.Shape()));

            BRepOffsetAPI_ThruSections mkGenerator(makeSolid ? Standard_True : Standard_False, Standard_False);
            for (std::list<TopoDS_Wire>::const_iterator it = wire_list.begin(); it != wire_list.end(); ++it) {
                const TopoDS_Wire &wire = *it;
                mkGenerator.AddWire(wire);
            }

            mkGenerator.Build();
            this->Shape.setValue(mkGenerator.Shape());
        }
        else {
            // Now, let's get the TopoDS_Shape
            TopoDS_Shape myShape = base->Shape.getValue();
            if (myShape.IsNull())
                Standard_Failure::Raise("Cannot extrude empty shape");
            if (makeSolid && myShape.ShapeType() == TopAbs_WIRE) {
                BRepBuilderAPI_MakeFace mkFace(TopoDS::Wire(myShape));
                myShape = mkFace.Face();
            }
            BRepPrimAPI_MakePrism mkPrism(myShape, vec);
            TopoDS_Shape swept = mkPrism.Shape();
            if (swept.IsNull())
                return new App::DocumentObjectExecReturn("Resulting shape is null");
            this->Shape.setValue(swept);
        }
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure) {
        Handle_Standard_Failure e = Standard_Failure::Caught();
        return new App::DocumentObjectExecReturn(e->GetMessageString());
    }
}
