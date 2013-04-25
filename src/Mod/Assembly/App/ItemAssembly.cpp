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
# include <BRep_Builder.hxx>
# include <TopoDS_Compound.hxx>
#endif

#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "ItemAssembly.h"
#include <ItemAssemblyPy.h>


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ItemAssembly, Assembly::Item)

ItemAssembly::ItemAssembly() : m_solver(new Solver)
{
    ADD_PROPERTY(Items,(0));
    ADD_PROPERTY(Annotations,(0));
}

short ItemAssembly::mustExecute() const
{
    return 0;
}

App::DocumentObjectExecReturn *ItemAssembly::execute(void)
{
    Base::Console().Message("Execute ItemAssembly\n");
    this->touch();
    return App::DocumentObject::StdReturn;
}

TopoDS_Shape ItemAssembly::getShape(void) const 
{
    std::vector<TopoDS_Shape> s;
    std::vector<App::DocumentObject*> obj = Items.getValues();

    std::vector<App::DocumentObject*>::iterator it;
    for (it = obj.begin(); it != obj.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(Assembly::Item::getClassTypeId())) {
            TopoDS_Shape aShape = static_cast<Assembly::Item*>(*it)->getShape();
            if (!aShape.IsNull())
                s.push_back(aShape);
        }
    }

    if (s.size() > 0) {
        TopoDS_Compound aRes = TopoDS_Compound();
        BRep_Builder aBuilder = BRep_Builder();
        aBuilder.MakeCompound(aRes);

        for (std::vector<TopoDS_Shape>::iterator it = s.begin(); it != s.end(); ++it) {

            aBuilder.Add(aRes, *it);
        }
        //if (aRes.IsNull())
        //    throw Base::Exception("Resulting shape is invalid");
        return aRes;
    }
    // set empty shape
    return TopoDS_Compound();
    
}

PyObject *ItemAssembly::getPyObject(void)
{
    if (PythonObject.is(Py::_None())){
        // ref counter is set to 1
        PythonObject = Py::Object(new ItemAssemblyPy(this),true);
    }
    return Py::new_reference_to(PythonObject); 
}

void ItemAssembly::addPart(ItemPart* part) {

    if(part->m_part) {
      //TODO: destroy old part
    }
  
    //add the part to our list
    const std::vector< App::DocumentObject * > &vals = this->Items.getValues();
    std::vector< App::DocumentObject * > newVals(vals);
    newVals.push_back(part);
    this->Items.setValues(newVals); 
    
    part->m_part = m_solver->createPart(part->Placement.getValue(), part->Uid.getValueStr());
}

void ItemAssembly::addComponent(ItemAssembly* assembly) {

    if(assembly->m_solver) {
      //TODO: destroy old solver system
    }
    
    //add the component to our list
    const std::vector< App::DocumentObject * > &vals = this->Items.getValues();
    std::vector< App::DocumentObject * > newVals(vals);
    newVals.push_back(assembly);
    this->Items.setValues(newVals); 
    
    assembly->m_solver = boost::shared_ptr<Solver>(m_solver->createSubsystem());
    assembly->m_solver->setTransformation(assembly->Placement.getValue());
}

ItemPart* ItemAssembly::getContainingPart(App::DocumentObject* obj) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;
    
    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {
      
	if( (*it)->getTypeId() == Assembly::ItemPart::getClassTypeId() ) {
	  if(static_cast<Assembly::ItemPart*>(*it)->holdsObject(obj))
	    return static_cast<Assembly::ItemPart*>(*it);
	}
	else if ( (*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId() ) {
	  
	    Assembly::ItemPart* part = static_cast<Assembly::ItemAssembly*>(*it)->getContainingPart(obj);
	    if(part)
	      return part;
	}
    };
    
    return NULL;
}


}