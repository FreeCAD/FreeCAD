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
#include <boost/exception/get_error_info.hpp>
#endif

#include <Base/Placement.h>
#include <Base/Console.h>
#include <Base/Exception.h>

#include "ItemAssembly.h"
#include "ItemPart.h"
#include "ConstraintGroup.h"
#include <ItemAssemblyPy.h>


using namespace Assembly;

namespace Assembly {


PROPERTY_SOURCE(Assembly::ItemAssembly, Assembly::Item)

ItemAssembly::ItemAssembly() {
    ADD_PROPERTY(Items,(0));
    ADD_PROPERTY(Annotations,(0));
    ADD_PROPERTY(Rigid,(true));
}

short ItemAssembly::mustExecute() const {
    return 0;
}

App::DocumentObjectExecReturn* ItemAssembly::execute(void) {

    try {

        //create a solver and init all child assemblys with subsolvers
        m_solver = boost::shared_ptr<Solver>(new Solver);
        m_downstream_placement = Base::Placement(Base::Vector3<double>(0,0,0), Base::Rotation());
        Base::Placement dummy;
        initSolver(boost::shared_ptr<Solver>(), dummy, false);
        initConstraints(boost::shared_ptr<Solver>());

        //solve the system
        m_solver->solve();

        //Parts have updated automaticly, however, currently there are no signals
        //for subsystems. We have to retrieve the product placements therefore by hand
        finish(boost::shared_ptr<Solver>());

    }
    catch(dcm::solving_error& e) {
        Base::Console().Error("Solver failed with error %i: %s",
                              *boost::get_error_info<boost::errinfo_errno>(e),
                              boost::get_error_info<dcm::error_message>(e)->c_str());
    }
    catch(dcm::creation_error& e) {
        Base::Console().Error("Creation failed with error %i: %s",
                              *boost::get_error_info<boost::errinfo_errno>(e),
                              boost::get_error_info<dcm::error_message>(e)->c_str());
    }

    this->touch();
    return App::DocumentObject::StdReturn;
}

TopoDS_Shape ItemAssembly::getShape(void) const {
    std::vector<TopoDS_Shape> s;
    std::vector<App::DocumentObject*> obj = Items.getValues();

    std::vector<App::DocumentObject*>::iterator it;
    for(it = obj.begin(); it != obj.end(); ++it) {
        if((*it)->getTypeId().isDerivedFrom(Assembly::Item::getClassTypeId())) {
            TopoDS_Shape aShape = static_cast<Assembly::Item*>(*it)->getShape();
            if(!aShape.IsNull())
                s.push_back(aShape);
        }
    }

    if(s.size() > 0) {
        TopoDS_Compound aRes = TopoDS_Compound();
        BRep_Builder aBuilder = BRep_Builder();
        aBuilder.MakeCompound(aRes);

        for(std::vector<TopoDS_Shape>::iterator it = s.begin(); it != s.end(); ++it) {

            aBuilder.Add(aRes, *it);
        }
        //if (aRes.IsNull())
        //    throw Base::Exception("Resulting shape is invalid");
        return aRes;
    }
    // set empty shape
    return TopoDS_Compound();

}

PyObject* ItemAssembly::getPyObject(void) {
    if(PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new ItemAssemblyPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

bool ItemAssembly::isParentAssembly(ItemPart* part) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemPart::getClassTypeId())
            if(*it == part)
                return true;
    };

    return false;
}

ItemAssembly* ItemAssembly::getParentAssembly(ItemPart* part) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemPart::getClassTypeId()) {
            if(*it == part)
                return this;
        }
        else
            if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

                Assembly::ItemAssembly* assembly = static_cast<Assembly::ItemAssembly*>(*it)->getParentAssembly(part);
                if(assembly)
                    return assembly;
            }
    };

    return (ItemAssembly*)NULL;
}



std::pair<ItemPart*, ItemAssembly*> ItemAssembly::getContainingPart(App::DocumentObject* obj) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemPart::getClassTypeId()) {
            if(static_cast<Assembly::ItemPart*>(*it)->holdsObject(obj))
                return std::make_pair(static_cast<Assembly::ItemPart*>(*it), this);
        }
        else
            if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

                std::pair<ItemPart*, ItemAssembly*> part = static_cast<Assembly::ItemAssembly*>(*it)->getContainingPart(obj);
                if(part.first && part.second)
                    return part;
            }
    };

    return std::pair<ItemPart*, ItemAssembly*>(NULL, NULL);
}

void ItemAssembly::initSolver(boost::shared_ptr<Solver> parent, Base::Placement& PL_downstream, bool stopped) {

    if(parent) {
        if(Rigid.getValue() || stopped) {
            m_solver = boost::shared_ptr<Solver>(parent->createSubsystem());
            m_solver->setTransformation(this->Placement.getValue());
            stopped = true; //all below belongs to this rigid group
        }
        else {
            m_solver = parent;
            PL_downstream *= this->Placement.getValue();
            m_downstream_placement = PL_downstream;
        }
    }

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

            static_cast<Assembly::ItemAssembly*>(*it)->initSolver(m_solver, PL_downstream, stopped);
        }
    };
}

void ItemAssembly::initConstraints(boost::shared_ptr<Solver> parent) {


    if(!parent || !Rigid.getValue()) {
        //get the constraint group and init the constraints
        typedef std::vector<App::DocumentObject*>::const_iterator iter;

        const std::vector<App::DocumentObject*>& vector = Annotations.getValues();
        for(iter it=vector.begin(); it != vector.end(); it++) {

            if((*it)->getTypeId() == Assembly::ConstraintGroup::getClassTypeId())
                static_cast<ConstraintGroup*>(*it)->init(this);
        };

        // iterate down as long as a non-rigid subsystem exists
        const std::vector<App::DocumentObject*>& vector2 = Items.getValues();
        for(iter it=vector2.begin(); it != vector2.end(); it++) {

            if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId())
                static_cast<Assembly::ItemAssembly*>(*it)->initConstraints(m_solver);

        };
    }
}

//no signals for subsystems, we need to extract the placement by hand
void ItemAssembly::finish(boost::shared_ptr<Solver> parent) {

    if(parent && Rigid.getValue()) {
        Base::Placement p = m_solver->getTransformation<Base::Placement>();
        this->Placement.setValue(p);
    }


    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();
    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

            static_cast<Assembly::ItemAssembly*>(*it)->finish(m_solver);
        }
    };

}


}
