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

#include "PartRef.h"
#include "Product.h"
#include <Mod/Part/App/PartFeature.h>
#include <Mod/Part/App/BodyBase.h>
#include <PartRefPy.h>
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

struct AssemblyItemException : std::exception {
  const char* what() const throw() { return "Assembly items are in wrong structure";}
};


PROPERTY_SOURCE(Assembly::PartRef,  App::GeoFeature)

PartRef::PartRef() {
    ADD_PROPERTY(Model, (0));
    ADD_PROPERTY(Annotation,(0));
}

short PartRef::mustExecute() const {
    //if (Sketch.isTouched() ||
    //    Length.isTouched())
    //    return 1;
    return 0;
}

App::DocumentObjectExecReturn* PartRef::execute(void) {
  
    this->touch();
    return App::DocumentObject::StdReturn;
}

TopoDS_Shape PartRef::getShape(void) const {
    App::DocumentObject* obj = Model.getValue();

    if(obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
        return static_cast<Part::Feature*>(obj)->Shape.getValue();
    }

    return TopoDS_Shape();
}

PyObject* PartRef::getPyObject(void) {
    if(PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new PartRefPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

bool PartRef::holdsObject(App::DocumentObject* obj) const {

    //get the body object and the relevant model list
    Part::BodyBase* base = static_cast<Part::BodyBase*>(Model.getValue());
    const std::vector<App::DocumentObject*>& vector = base->Model.getValues();

    //check if it holds the relevant document object
    return std::find(vector.begin(), vector.end(), obj)!=vector.end();
}

void PartRef::setCalculatedPlacement(boost::shared_ptr< Part3D > part) {

    //part is the same as m_part, so it doasn't matter which one we use
    Base::Placement p = dcm::get<Base::Placement>(part);
    
    Product* ass = getParentAssembly();
    if(!ass)
      throw AssemblyItemException();
    
    if(ass->Rigid.getValue()) 
      Placement.setValue(p);
    else
      Placement.setValue(ass->m_downstream_placement.inverse()*p);
}

Product* PartRef::getParentAssembly() {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = getInList();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::Product::getClassTypeId()) 
            return static_cast<Assembly::Product*>(*it);
    };

    return (Product*)NULL;
}

void PartRef::ensureInitialisation() {

    Product* ass = getParentAssembly();
    if(!ass)
      throw AssemblyItemException();
    
    boost::shared_ptr<Solver> solver = ass->m_solver;
    if(!solver) 
      throw AssemblyItemException();  
    
    if(!solver->hasPart(Uid.getValueStr())) {
		
	//if the assembly is not rigid it was not added to the solver, so we need to incorporate its placement
	if(ass->Rigid.getValue()) {
	  m_part = solver->createPart(Placement.getValue(), Uid.getValueStr());
	}
	else {
	  m_part = solver->createPart(ass->m_downstream_placement*Placement.getValue(), Uid.getValueStr());
	}
	m_part->connectSignal<dcm::recalculated>(boost::bind(&PartRef::setCalculatedPlacement, this, _1));
    };        
}


boost::shared_ptr< Geometry3D > PartRef::getGeometry3D(const char* Type) {

    //check if the item is initialized
    if(!m_part)
        return boost::shared_ptr< Geometry3D >();

    boost::shared_ptr<Geometry3D> geometry;
    if(m_part->hasGeometry3D(Type)) {
        return m_part->getGeometry3D(Type);
    }
    else {
        Part::TopoShape ts;
        App::DocumentObject* obj = Model.getValue();

        if(obj->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId())) {
            ts = static_cast<Part::Feature*>(obj)->Shape.getShape();
        }
        else
            return boost::shared_ptr< Geometry3D >();

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
                Base::Console().Message("Unsupported Surface Geometry Type at selection\n");
                return boost::shared_ptr< Geometry3D >();
            }

        }
        else
            if(s.ShapeType() == TopAbs_EDGE) {
                TopoDS_Edge edge = TopoDS::Edge(s);
                BRepAdaptor_Curve curve(edge);
                switch(curve.GetType()) {
                case GeomAbs_Line: {
                    gp_Lin line = curve.Line();
                    geometry = m_part->addGeometry3D(line, Type, dcm::Local);
                    break;
                }
                default:
                    Base::Console().Message("Unsupported Curve Geometry Type at selection \n");
                    return boost::shared_ptr< Geometry3D >();
                }

            }
            else
                if(s.ShapeType() == TopAbs_VERTEX) {
                    TopoDS_Vertex v1 = TopoDS::Vertex(s);
                    gp_Pnt point = BRep_Tool::Pnt(v1);
                    geometry = m_part->addGeometry3D(point, Type, dcm::Local);

                }
                else {
                    Base::Console().Message("Unsupported Topology Type at selection\n");
                    return boost::shared_ptr< Geometry3D >();
                }
    };

    return geometry;
}

}
