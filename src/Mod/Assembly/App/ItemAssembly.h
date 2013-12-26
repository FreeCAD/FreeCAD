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


#ifndef ItemAssembly_ItemAssembly_H
#define ItemAssembly_ItemAssembly_H

#include <App/PropertyStandard.h>

#include "Item.h"
#include "Solver/Solver.h"

namespace Assembly
{

class ItemPart;

class AssemblyExport ItemAssembly : public Assembly::Item
{
    PROPERTY_HEADER(Assembly::ItemAssembly); 

public:
    ItemAssembly();

    App::PropertyLinkList   Items;
    App::PropertyLinkList   Annotations;
    App::PropertyBool	    Rigid;
 
    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderItemAssembly";
    }
    PyObject *getPyObject(void);
    //@}

    virtual TopoDS_Shape getShape(void) const;
    
    bool isParentAssembly(ItemPart* part);
    ItemAssembly* getToplevelAssembly();
    ItemAssembly* getParentAssembly(ItemPart* part);
    
    //returns the ItemPart which holds the given document object and the ItemAssembly, which holds
    //the this part and is a direct children of this ItemAssembly. The returned ItemAssembly is therefore
    //the "TopLevel" Assembly holding the part of all children of this assembly. If this assembly holds 
    //the children directly, without any subassembly, the returned ItemAssembly is this.
    std::pair< ItemPart*, ItemAssembly* > getContainingPart(App::DocumentObject* obj, bool isTop=true);
    
    //create a new solver for this assembly and initalise all downstream itemassemblys either with a 
    //subsystem (if they are rigid) or with this solver plus the downstream placement
    void initSolver(boost::shared_ptr<Solver> parent, Base::Placement& pl_downstream, bool stopped);
    
    //initialise the oen constraint group and go downstream as long as non-rigid itemassemblys exist, 
    //which need to be initialised too
    void initConstraints(boost::shared_ptr<Solver> parent);
    
    //read the downstream itemassemblys and set their placement to the propertyplacement
    void finish(boost::shared_ptr<Solver> subsystem);
    
    boost::shared_ptr<Solver> m_solver;
    Base::Placement m_downstream_placement;
    
    
#ifdef ASSEMBLY_DEBUG_FACILITIES
    App::PropertyBool  ApplyAtFailure;
    App::PropertyFloat Precision;
    App::PropertyBool  SaveState;
#endif
    
private:
    std::stringstream message;
};

} //namespace Assembly


#endif // Assembly_ItemAssembly_H
