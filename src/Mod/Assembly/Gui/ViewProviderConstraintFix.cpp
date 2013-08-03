/***************************************************************************
 *   Copyright (c) 2013 Stefan Tröger <stefantroeger@gmx.net>              *
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
#include "ViewProviderConstraintFix.h"
#include "Mod/Assembly/App/ConstraintFix.h"
#include <Base/Console.h>

using namespace AssemblyGui;

PROPERTY_SOURCE(AssemblyGui::ViewProviderConstraintFix, Gui::ViewProviderDocumentObject)

ViewProviderConstraintFix::ViewProviderConstraintFix() {

    sPixmap = "Assembly_ConstraintLock";
}

TopoDS_Shape ViewProviderConstraintFix::getConstraintShape(int link)
{
    if(link == 1) {

        App::DocumentObject* obj = dynamic_cast<Assembly::Constraint*>(pcObject)->First.getValue();
        if(!obj)
            return TopoDS_Shape();

        Assembly::ItemPart* part = static_cast<Assembly::ItemPart*>(obj);
	if(!part)
	  return TopoDS_Shape();
	
	//return the whole shape
	return part->getShape();
    }
    
    //there is no second link, only one part is fixed per constraint
    return TopoDS_Shape();
}
