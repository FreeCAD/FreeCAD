/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAdaptor_Curve.hxx>
# include <gp_Ax1.hxx>
# include <gp_Circ.hxx>
# include <gp_Lin.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
#endif

#include "FeatureRevolution.h"
#include "FaceMaker.h"


using namespace Part;

App::PropertyFloatConstraint::Constraints Revolution::angleRangeU = {-360.0,360.0,1.0};

PROPERTY_SOURCE(Part::Revolution, Part::Feature)

Revolution::Revolution()
{
    ADD_PROPERTY_TYPE(Source,(nullptr), "Revolve", App::Prop_None, "Shape to revolve");
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d(0.0,0.0,0.0)), "Revolve", App::Prop_None, "Base point of revolution axis");
    ADD_PROPERTY_TYPE(Axis,(Base::Vector3d(0.0,0.0,1.0)), "Revolve", App::Prop_None, "Direction of revolution axis");
    ADD_PROPERTY_TYPE(AxisLink,(nullptr),"Revolve",App::Prop_None,"Link to edge to use as revolution axis.");
    ADD_PROPERTY_TYPE(Angle,(360.0), "Revolve", App::Prop_None, "Angle span of revolution. If angle is zero, and an arc is used for axis link, angle span of arc will be used.");
    Angle.setConstraints(&angleRangeU);
    ADD_PROPERTY_TYPE(Symmetric,(false),"Revolve",App::Prop_None,"Extend revolution symmetrically from the profile.");
    ADD_PROPERTY_TYPE(Solid,(false),"Revolve",App::Prop_None,"Make revolution a solid if possible");
    ADD_PROPERTY_TYPE(FaceMakerClass,(""),"Revolve",App::Prop_None,"Facemaker to use if Solid is true."); //default for old documents. For default for new objects, refer to setupObject().
}

short Revolution::mustExecute() const
{
    if (Base.isTouched() ||
        Axis.isTouched() ||
        Angle.isTouched() ||
        Source.isTouched() ||
        Solid.isTouched() ||
        AxisLink.isTouched() ||
        Symmetric.isTouched() ||
        FaceMakerClass.isTouched())
        return 1;
    return 0;
}

void Revolution::onChanged(const App::Property* prop)
{
    if(! this->isRestoring()){
        if(prop == &AxisLink){
            Base.setReadOnly(AxisLink.getValue() != nullptr);
            Axis.setReadOnly(AxisLink.getValue() != nullptr);
        }
    }
    Part::Feature::onChanged(prop);
}

bool Revolution::fetchAxisLink(const App::PropertyLinkSub &axisLink,
                               Base::Vector3d& center,
                               Base::Vector3d& dir,
                               double& angle)
{
    if (!axisLink.getValue())
        return false;

    auto linked = axisLink.getValue();

    TopoDS_Shape axEdge;
    if (!axisLink.getSubValues().empty()  &&  axisLink.getSubValues()[0].length() > 0){
        axEdge = Feature::getTopoShape(linked, axisLink.getSubValues()[0].c_str(), true /*need element*/).getShape();
    } else {
        axEdge = Feature::getShape(linked);
    }

    if (axEdge.IsNull())
        throw Base::ValueError("AxisLink shape is null");
    if (axEdge.ShapeType() != TopAbs_EDGE)
        throw Base::TypeError("AxisLink shape is not an edge");

    BRepAdaptor_Curve crv(TopoDS::Edge(axEdge));
    gp_Pnt base;
    gp_Dir occdir;
    bool reversed = axEdge.Orientation() == TopAbs_REVERSED;
    if (crv.GetType() == GeomAbs_Line){
        base = crv.Value(reversed ? crv.FirstParameter() : crv.LastParameter());
        occdir = crv.Line().Direction();
    } else if (crv.GetType() == GeomAbs_Circle) {
        base = crv.Circle().Axis().Location();
        occdir = crv.Circle().Axis().Direction();
        angle = crv.LastParameter() - crv.FirstParameter();
    } else {
        throw Base::TypeError("AxisLink edge is neither line nor arc of circle.");
    }
    if (reversed)
        occdir.Reverse();
    center.Set(base.X(), base.Y(),base.Z());
    dir.Set(occdir.X(), occdir.Y(), occdir.Z());
    return true;
}

App::DocumentObjectExecReturn *Revolution::execute()
{
    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    try {
        //read out axis link
        double angle_edge = 0;
        Base::Vector3d b = Base.getValue();
        Base::Vector3d v = Axis.getValue();
        bool linkFetched = this->fetchAxisLink(this->AxisLink, b, v, angle_edge);
        if (linkFetched){
            this->Base.setValue(b);
            this->Axis.setValue(v);
        }

        gp_Pnt pnt(b.x,b.y,b.z);
        gp_Dir dir(v.x,v.y,v.z);
        gp_Ax1 revAx(pnt, dir);

        //read out revolution angle
        double angle = Angle.getValue()/180.0f*M_PI;
        if (fabs(angle) < Precision::Angular())
            angle = angle_edge;

        //apply "midplane" symmetry
        TopoShape sourceShape = Feature::getShape(link);
        if (Symmetric.getValue()) {
            //rotate source shape backwards by half angle, to make resulting revolution symmetric to the profile
            gp_Trsf mov;
            mov.SetRotation(revAx, angle * (-0.5));
            TopLoc_Location loc(mov);
            sourceShape.setShape(sourceShape.getShape().Moved(loc));
        }

        //"make solid" processing: make faces from wires.
        Standard_Boolean makeSolid = Solid.getValue() ? Standard_True : Standard_False;
        if (makeSolid){
            //test if we need to make faces from wires. If there are faces - we don't.
            TopExp_Explorer xp(sourceShape.getShape(), TopAbs_FACE);
            if (xp.More())
                //source shape has faces. Just revolve as-is.
                makeSolid = Standard_False;
        }
        if (makeSolid && strlen(this->FaceMakerClass.getValue())>0){
            //new facemaking behavior: use facemaker class
            std::unique_ptr<FaceMaker> mkFace = FaceMaker::ConstructFromType(this->FaceMakerClass.getValue());

            TopoDS_Shape myShape = sourceShape.getShape();
            if(myShape.ShapeType() == TopAbs_COMPOUND)
                mkFace->useCompound(TopoDS::Compound(myShape));
            else
                mkFace->addShape(myShape);
            mkFace->Build();
            myShape = mkFace->Shape();
            sourceShape = TopoShape(myShape);

            makeSolid = Standard_False;//don't ask TopoShape::revolve to make solid, as we've made faces...
        }

        // actual revolution!
        TopoDS_Shape revolve = sourceShape.revolve(revAx, angle, makeSolid);

        if (revolve.IsNull())
            return new App::DocumentObjectExecReturn("Resulting shape is null");
        this->Shape.setValue(revolve);
        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}



void Part::Revolution::setupObject()
{
    Part::Feature::setupObject();
    this->FaceMakerClass.setValue("Part::FaceMakerBullseye"); //default for newly created features
}
