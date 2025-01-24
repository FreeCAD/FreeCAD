/***************************************************************************
 *   Copyright (c) 2010 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepBuilderAPI_Transform.hxx>
# include <BRep_Tool.hxx>
# include <gp_Ax2.hxx>
# include <gp_Circ.hxx>
# include <gp_Dir.hxx>
# include <gp_Pnt.hxx>
# include <gp_Trsf.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <gp_Pln.hxx>
# include <Geom_Plane.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <TopExp_Explorer.hxx>
#endif

#include <Mod/Part/App/PrimitiveFeature.h>
#include <App/Link.h>
#include <App/Datums.h>

#include "FeatureMirroring.h"
#include "DatumFeature.h"



using namespace Part;

PROPERTY_SOURCE(Part::Mirroring, Part::Feature)

Mirroring::Mirroring()
{
    ADD_PROPERTY(Source,(nullptr));
    ADD_PROPERTY_TYPE(Base,(Base::Vector3d()),"Plane",App::Prop_None,"The base point of the plane");
    ADD_PROPERTY_TYPE(Normal,(Base::Vector3d(0,0,1)),"Plane",App::Prop_None,"The normal of the plane");
    ADD_PROPERTY_TYPE(MirrorPlane,(nullptr),"Plane",App::Prop_None,"A reference for the mirroring plane, overrides Base and Normal if set, can be face or circle");
}

short Mirroring::mustExecute() const
{
    if (Source.isTouched())
        return 1;
    if (Base.isTouched())
        return 1;
    if (Normal.isTouched())
        return 1;
    if (MirrorPlane.isTouched())
        return 1;
    return 0;
}

void Mirroring::onChanged(const App::Property* prop)
{
    /**
      In the case the user has a reference plane object, then
      Base and Normal are computed based on that object.  We must
      handle this by setting the changes to Base and Normal to not
      trigger a recompute when they are computed and changed.  We
      should also set Base and Normal to readonly so not to confuse
      the user, who might try to change them despite having a reference
      object. We could also hide them, but they contain useful information.
    */
    if (!isRestoring()) {
        bool needsRecompute = false;
        App::DocumentObject* refObject = MirrorPlane.getValue();
        if (!refObject){
            Base.setStatus(App::Property::ReadOnly, false);
            Normal.setStatus(App::Property::ReadOnly, false);
            if (prop == &Base || prop == &Normal) {
                needsRecompute = true;
            }
        } else {
            if (prop == &MirrorPlane){
                Base.setStatus(App::Property::ReadOnly, true);
                Normal.setStatus(App::Property::ReadOnly, true);
                needsRecompute = true;
            }
        }
        if (needsRecompute){
            try {
                App::DocumentObjectExecReturn *ret = recompute();
                delete ret;
            }
            catch (...) {
            }
        }
    }
    Part::Feature::onChanged(prop);
}

void Mirroring::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
{
    if (prop == &Base && strcmp(TypeName, "App::PropertyVector") == 0) {
        App::PropertyVector v;

        v.Restore(reader);

        Base.setValue(v.getValue());
    }
    else if (prop == &Normal && strcmp(TypeName, "App::PropertyVector") == 0) {
        App::PropertyVector v;

        v.Restore(reader);

        Normal.setValue(v.getValue());
    }
    else {
        Part::Feature::handleChangedPropertyType(reader, TypeName, prop);
    }
}

App::DocumentObjectExecReturn *Mirroring::execute()
{
    App::DocumentObject* link = Source.getValue();
    if (!link)
        return new App::DocumentObjectExecReturn("No object linked");

    App::DocumentObject* refObject = MirrorPlane.getValue();

    std::vector<std::string> subStrings = MirrorPlane.getSubValues();

    gp_Pnt axbase;
    gp_Dir axdir;
    /**
      Support mirror plane reference objects:
      DatumPlanes, Part::Planes, Origin planes, Faces, Circles
      Can also be App::Links to such objects
    */
    if (refObject){
        if (refObject->isDerivedFrom(Part::Plane::getClassTypeId()) || refObject->isDerivedFrom<App::Plane>() || (strstr(refObject->getNameInDocument(), "Plane")
                                                                                                                  && refObject->isDerivedFrom(Part::Datum::getClassTypeId()))) {
            Part::Feature* plane = static_cast<Part::Feature*>(refObject);
            Base::Vector3d base = plane->Placement.getValue().getPosition();
            axbase = gp_Pnt(base.x, base.y, base.z);
            Base::Rotation rot = plane->Placement.getValue().getRotation();
            Base::Vector3d dir;
            rot.multVec(Base::Vector3d(0,0,1), dir);
            axdir = gp_Dir(dir.x, dir.y, dir.z);
            // reference is an app::link or a part::feature or some subobject
        } else if (refObject->isDerivedFrom<Part::Feature>() || refObject->isDerivedFrom<App::Link>()) {
            if (subStrings.size() > 1){
                throw Base::ValueError(std::string(this->getFullLabel()) + ": Only 1 subobject is supported for Mirror Plane reference, either a plane face or a circle edge.");

            }
            auto linked = MirrorPlane.getValue();
            bool isFace = false; //will be true if user selected face subobject or if object only has 1 face
            bool isEdge = false; //will be true if user selected edge subobject or if object only has 1 edge
            TopoDS_Shape shape;
            if (!subStrings.empty() && subStrings[0].length() > 0){
                shape = Feature::getTopoShape(linked, subStrings[0].c_str(), true).getShape();
                if (strstr(subStrings[0].c_str(), "Face")){
                    isFace = true; //was face subobject, e.g. Face3
                } else {
                    if (strstr(subStrings[0].c_str(), "Edge")){
                        isEdge = true; //was edge subobject, e.g. Edge7
                    }
                }
            } else {
                shape = Feature::getShape(linked); //no subobjects were selected, so this is entire shape of feature
            }

            // if there is only 1 face or 1 edge, then we don't need to force the user to select that face or edge
            // instead we can infer what was intended
            int faceCount = Part::TopoShape(shape).countSubShapes(TopAbs_FACE);
            int edgeCount = Part::TopoShape(shape).countSubShapes(TopAbs_EDGE);

            TopoDS_Face face;
            TopoDS_Edge edge;

            if (isFace) { //user selected a face, so use shape to get the TopoDS::Face
                face = TopoDS::Face(shape);
            } else {
                if (faceCount == 1) { //entire feature selected, but it only has 1 face, so get that face
                    TopoDS_Shape tdface = Part::TopoShape(shape).getSubShape(std::string("Face1").c_str());
                    face = TopoDS::Face(tdface);
                    isFace = true;
                }
            }
            if (!isFace && isEdge){ //don't bother with edge if we already have a face to work with
                edge = TopoDS::Edge(shape); //isEdge means an edge was selected
            } else {
                if (edgeCount == 1){ //we don't have a face yet and there were no edges in the subobject selection
                    //but since this object only has 1 edge, we use it
                    TopoDS_Shape tdedge = Part::TopoShape(shape).getSubShape(std::string("Edge1").c_str());
                    edge = TopoDS::Edge(tdedge);
                    isEdge = true;
                }
            }

            if (isFace && face.IsNull()) { //ensure we have a good face to work with
                throw Base::ValueError(std::string(this->getFullLabel()) + ": Failed to extract mirror plane because face is null");
            }
            if (isEdge && edge.IsNull()){ //ensure we have a good edge to work with
                throw Base::ValueError(std::string(this->getFullLabel()) + ": Failed to extract mirror plane because edge is null");
            }
            if (!isFace && !isEdge){
                throw Base::ValueError(std::string(this->getFullLabel()) + ": Failed to extract mirror plane, unable to determine which face or edge to use.");
            }

            if (isFace) {
                BRepAdaptor_Surface adapt(face);
                if (adapt.GetType() != GeomAbs_Plane)
                    throw Base::TypeError(std::string(this->getFullLabel()) + ": Mirror plane face must be planar");
                TopExp_Explorer exp;
                exp.Init(face, TopAbs_VERTEX);
                if (exp.More()) {
                    axbase = BRep_Tool::Pnt(TopoDS::Vertex(exp.Current()));
                }
                axdir = adapt.Plane().Axis().Direction();
            } else {
                if (isEdge){
                    BRepAdaptor_Curve curve(edge);
                    if (!(curve.GetType() == GeomAbs_Circle)) {
                        throw Base::TypeError(std::string(this->getFullLabel()) + ": Only circle edge types are supported");
                    }
                    gp_Circ circle = curve.Circle();
                    axdir = circle.Axis().Direction();
                    axbase = circle.Location();
                }
            }
        } else {
            throw Base::ValueError(std::string(this->getFullLabel()) + ": Mirror plane reference must be a face of a feature or a plane object or a circle");
        }
        Base.setValue(axbase.X(), axbase.Y(), axbase.Z());
        Normal.setValue(axdir.X(), axdir.Y(), axdir.Z());
    }

    Base::Vector3d base = Base.getValue();
    Base::Vector3d norm = Normal.getValue();

    try {
        gp_Ax2 ax2(gp_Pnt(base.x,base.y,base.z), gp_Dir(norm.x,norm.y,norm.z));
        auto shape = Feature::getTopoShape(link);
        if (shape.isNull())
            Standard_Failure::Raise("Cannot mirror empty shape");
        this->Shape.setValue(TopoShape(0).makeElementMirror(shape,ax2));
        copyMaterial(link);

        return Part::Feature::execute();
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}
