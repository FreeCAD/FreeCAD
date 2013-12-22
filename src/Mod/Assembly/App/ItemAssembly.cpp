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
#ifdef ASSEMBLY_DEBUG_FACILITIES
    ADD_PROPERTY(ApplyAtFailure,(false));
    ADD_PROPERTY(Precision,(1e-6));
#endif
}

short ItemAssembly::mustExecute() const {
    return 0;
}

App::DocumentObjectExecReturn* ItemAssembly::execute(void) {

    Base::Console().Message("Execute\n");

    try {

        //create a solver and init all child assemblys with subsolvers
        m_solver = boost::shared_ptr<Solver>(new Solver);
        m_downstream_placement = Base::Placement(Base::Vector3<double>(0,0,0), Base::Rotation());
        Base::Placement dummy;
        initSolver(boost::shared_ptr<Solver>(), dummy, false);

#ifdef ASSEMBLY_DEBUG_FACILITIES

        if(ApplyAtFailure.getValue())
            m_solver->setOption<dcm::solverfailure>(dcm::ApplyResults);
        else
            m_solver->setOption<dcm::solverfailure>(dcm::IgnoreResults);

        m_solver->setOption<dcm::precision>(Precision.getValue());
#endif
        initConstraints(boost::shared_ptr<Solver>());

        //solve the system
        m_solver->solve();
    }
    catch
        (boost::exception& e) {
        message.clear();
        message << "Solver exception " << *boost::get_error_info<boost::errinfo_errno>(e)
                << "raised: " << boost::get_error_info<dcm::error_message>(e)->c_str() << std::endl;
        //throw Base::Exception(message.str().c_str());
        Base::Console().Error(message.str().c_str());
    }
    catch
        (std::exception& e) {
        message.clear();
        message << "Exception raised in assembly solver: " << e.what() << std::endl;
        //throw Base::Exception(message.str().c_str());
        Base::Console().Error(message.str().c_str());
    }
    catch
        (...) {
        message.clear();
        message << "Unknown Exception raised in assembly solver during execution" << std::endl;
        //throw Base::Exception(message.str().c_str());
        Base::Console().Error(message.str().c_str());
    };

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

ItemAssembly* ItemAssembly::getToplevelAssembly() {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = getInList();

    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId())
            return static_cast<Assembly::ItemAssembly*>(*it)->getToplevelAssembly();
    };

    return this;
};

ItemAssembly* ItemAssembly::getParentAssembly(ItemPart* part) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();

    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemPart::getClassTypeId()) {
            if(*it == part)
                return this;
        }
        else if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

            Assembly::ItemAssembly* assembly = static_cast<Assembly::ItemAssembly*>(*it)->getParentAssembly(part);

            if(assembly)
                return assembly;
        }
    };

    return (ItemAssembly*)NULL;
}



std::pair<ItemPart*, ItemAssembly*> ItemAssembly::getContainingPart(App::DocumentObject* obj, bool isTop) {

    typedef std::vector<App::DocumentObject*>::const_iterator iter;

    const std::vector<App::DocumentObject*>& vector = Items.getValues();

    for(iter it=vector.begin(); it != vector.end(); it++) {

        if((*it)->getTypeId() == Assembly::ItemPart::getClassTypeId()) {
            if(static_cast<Assembly::ItemPart*>(*it)->holdsObject(obj))
                return std::make_pair(static_cast<Assembly::ItemPart*>(*it), this);
        }
        else if((*it)->getTypeId() == Assembly::ItemAssembly::getClassTypeId()) {

            std::pair<ItemPart*, ItemAssembly*> part = static_cast<Assembly::ItemAssembly*>(*it)->getContainingPart(obj, false);

            if(part.first && part.second) {

                if(isTop)
                    return part;
                else
                    return std::make_pair(part.first, this);
            }
        }
    };

    return std::pair<ItemPart*, ItemAssembly*>(NULL, NULL);
}

void ItemAssembly::initSolver(boost::shared_ptr<Solver> parent, Base::Placement& PL_downstream, bool stopped) {

    if(parent) {
        if(Rigid.getValue() || stopped) {
            m_solver = parent->createSubsystem();
            m_solver->setTransformation(this->Placement.getValue());
            stopped = true; //all below belongs to this rigid group

            //connect the recalculated signal in case we need to update the placement
            m_solver->connectSignal<dcm::recalculated>(boost::bind(&ItemAssembly::finish, this, _1));
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

};

//the callback for the recalculated signal
void ItemAssembly::finish(boost::shared_ptr<Solver> subsystem) {

    //assert(subsystem == m_solver);
    Base::Placement p = m_solver->getTransformation<Base::Placement>();
    this->Placement.setValue(p);
};

} //assembly
