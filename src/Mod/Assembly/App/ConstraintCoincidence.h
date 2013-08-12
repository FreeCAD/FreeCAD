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


#ifndef Assembly_ConstraintCoincidence_H
#define Assembly_ConstraintCoincidence_H

#include <App/PropertyStandard.h>
#include "Constraint.h"


namespace Assembly
{

class AssemblyExport ConstraintCoincidence : public Assembly::Constraint
{
    PROPERTY_HEADER(Assembly::ConstraintCoincidence);

public:
    ConstraintCoincidence();
    
    App::PropertyEnumeration Orientation;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderConstraintCoincidence";
    }
    //@}
    
    virtual void init(Assembly::ItemAssembly* ass);
};

} //namespace Assembly


#endif // Assembly_ConstraintCoincidence_H
