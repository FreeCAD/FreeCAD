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
#include "Solver.h"
#include "ItemPart.h"

namespace Assembly
{

class AssemblyExport ItemAssembly : public Assembly::Item
{
    PROPERTY_HEADER(Assembly::ItemAssembly); 

public:
    ItemAssembly();

    App::PropertyLinkList   Items;
    App::PropertyLinkList   Annotations;
 
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
    ItemAssembly* getParentAssembly(ItemPart* part);
    
    ItemPart* getContainingPart(App::DocumentObject* obj);
    void init(boost::shared_ptr<Solver> parent);
    
    boost::shared_ptr<Solver> m_solver;
};

} //namespace Assembly


#endif // Assembly_ItemAssembly_H
