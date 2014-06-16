/***************************************************************************
 *   Copyright (c) 2012 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
 *   Copyright (c) 2013 Stefan Tröger  <stefantroeger@gmx.net>             *
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


#ifndef Assembly_Constraint_H
#define Assembly_Constraint_H

#include <App/PropertyLinks.h>
#include <App/DocumentObject.h>

#include <TopoDS_Shape.hxx>

#include "Solver/Solver.h"
#include "Product.h"


namespace Assembly
{

class AssemblyExport Constraint : public App::DocumentObject
{
    PROPERTY_HEADER(Assembly::Constraint);
        
public:
    Constraint();

    App::PropertyLinkSub        First;
    App::PropertyLinkSub        Second;
    App::PropertyFloat   	    Value;
    App::PropertyEnumeration 	Orientation;
    App::PropertyEnumeration 	SolutionSpace;
    App::PropertyEnumeration 	Type;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute(void);
    short mustExecute() const;
    /// returns the type name of the view provider
    const char* getViewProviderName(void) const {
        return "AssemblyGui::ViewProviderConstraint";
    }
    PyObject *getPyObject(void);

private:
    static const char* OrientationEnums[];
    static const char* TypeEnums[];
    static const char* SolutionSpaceEnums[];
};

} //namespace Assembly


#endif // Assembly_Constraint_H
