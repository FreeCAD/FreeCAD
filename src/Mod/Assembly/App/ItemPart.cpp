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
#endif

#include <Base/Placement.h>
#include <Base/Console.h>

#include "ItemPart.h"
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/BodyBase.h>
#include <ItemPartPy.h>
#include <TopoDS.hxx>
#include <GeomAbs_SurfaceType.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <TopoDS_Edge.hxx>
#include <TopoDS_Vertex.hxx>
#include <BRep_Tool.hxx>
#include <GeomAbs_CurveType.hxx>


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ItemPart, Assembly::Item)

ItemPart::ItemPart()
{
    ADD_PROPERTY(Model,     (0));
    ADD_PROPERTY(Annotation,(0));
}

short ItemPart::mustExecute() const
{
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn *ItemPart::execute(void)
{
     this->touch();
     return App::DocumentObject::StdReturn;
}

TopoDS_Shape ItemPart::getShape(void) const 
{
    App::DocumentObject* obj = Model.getValue();

    if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return static_cast<Part::Feature*>(obj)->Shape.getValue();
    }

    return TopoDS_Shape();
}

PyObject *ItemPart::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ItemPartPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

bool ItemPart::holdsObject(App::DocumentObject* obj) const {

    //get the body object and the relevant model list
    Part::BodyBase* base = static_cast<Part::BodyBase*>(Model.getValue());
    const std::vector<App::DocumentObject*>& vector = base->Model.getValues();
    
    //check if it holds the relevant document object
    return std::find(vector.begin(), vector.end(), obj)!=vector.end();
}

void ItemPart::setCalculatedPlacement(boost::shared_ptr< Part3D > part) {

    //part is the same as m_part, so it doasn't matter which one we use  
    Base::Placement p = dcm::get<Base::Placement>(part);
    Placement.setValue(p);
}


boost::shared_ptr< Geometry3D > ItemPart::getGeometry3D(const char* Type) 
{
  
    //check if the item is initialized
    if(!m_part) 
      return boost::shared_ptr< Geometry3D >();
  
    boost::shared_ptr<Geometry3D> geometry;
    if(m_part->hasGeometry3D(Type)) {
        return m_part->getGeometry3D(Type);
    } else {
	Part::TopoShape ts;	
	App::DocumentObject* obj = Model.getValue();

	if (obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
	  ts = static_cast<Part::Feature*>(obj)->Shape.getShape();
	}
	else return boost::shared_ptr< Geometry3D >();
	
	TopoDS_Shape s = ts.getSubShape(Type);
        if(s.ShapeType() == TopAbs_FACE) {
            TopoDS_Face face = TopoDS::Face(s);
            BRepAdaptor_Surface surface(face);
            switch(surface.GetType()) {
                case GeomAbs_Plane: {
                    gp_Pln plane = surface.Plane();
                    if(face.Orientation()==TopAbs_REVERSED) {
                        gp_Dir dir = plane.Axis().Direction();
                        plane = gp_Pln(plane.Location(), dir.Reversed());
                    }
                    geometry = m_part->addGeometry3D(plane, Type, dcm::Local);
                    break;
                }
                case GeomAbs_Cylinder: {
                    gp_Cylinder cyl = surface.Cylinder();
                    geometry = m_part->addGeometry3D(cyl, Type, dcm::Local);
                    break;
                }
                default:
                    Base::Console().Message("Unsuported Surface Geometrie Type at selection\n");
                    return boost::shared_ptr< Geometry3D >();
            }

        } else if(s.ShapeType() == TopAbs_EDGE) {
            TopoDS_Edge edge = TopoDS::Edge(s);
            BRepAdaptor_Curve curve(edge);
            switch(curve.GetType()) {
                case GeomAbs_Line: {
                    gp_Lin line = curve.Line();
                    geometry = m_part->addGeometry3D(line, Type, dcm::Local);
                    break;
                }
                default:
                    Base::Console().Message("Unsuported Curve Geometrie Type at selection \n");
                    return boost::shared_ptr< Geometry3D >();
            }

        } else if(s.ShapeType() == TopAbs_VERTEX) {
            TopoDS_Vertex v1 = TopoDS::Vertex(s);
            gp_Pnt point = BRep_Tool::Pnt(v1);
            geometry = m_part->addGeometry3D(point, Type, dcm::Local);

        } else {
            Base::Console().Message("Unsuported Topologie Type at selection\n");
            return boost::shared_ptr< Geometry3D >();
        }
    };
/*    
    std::stringstream s;
    s<<geometry->m_global;
    Base::Console().Message("Added geom: %s, %s\n", Type, s.str().c_str());
*/    
    return geometry;
}

}