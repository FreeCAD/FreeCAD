/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel (FreeCAD@juergen-riegel.net)             *
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

#include <strstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>

#include "SelectionObject.h"
#include "SelectionObjectPy.h"
#include "Application.h"

using namespace Gui;


TYPESYSTEM_SOURCE_ABSTRACT(Gui::SelectionObject, Base::BaseClass)

SelectionObject::SelectionObject()
{
}

//SelectionObject::SelectionObject( const Gui::SelectionSingleton::SelObj &Obj )
//{
//	// moving the information over
//	// no pointer is copied, cause is to dangerous to keep pointers to 
//	// the document outside....
//	DocName  = Obj.DocName;
//	FeatName = Obj.FeatName;
//	SubName  = Obj.SubName;
//	TypeName = Obj.TypeName;
//	x = Obj.x;
//	y = Obj.y;
//	z = Obj.z;
//
//}

SelectionObject::~SelectionObject()
{
}

const App::DocumentObject * SelectionObject::getObject(void) const
{
    if (DocName != "") {
        App::Document *doc = App::GetApplication().getDocument(DocName.c_str());
        if (doc && FeatName != "")
            return doc->getObject(FeatName.c_str());
    }
    return 0;
}

App::DocumentObject * SelectionObject::getObject(void) 
{
    if (DocName != "") {
        App::Document *doc = App::GetApplication().getDocument(DocName.c_str());
        if (doc && FeatName != "")
            return doc->getObject(FeatName.c_str());
    }
    return 0;
}

bool SelectionObject::isObjectTypeOf(const Base::Type& typeId) const
{
    const App::DocumentObject* obj = getObject();
    return (obj && obj->getTypeId().isDerivedFrom(typeId));
}

std::string SelectionObject::getAsPropertyLinkSubString(void)const
{
    std::string buf;
    buf += "(App.";
    buf += "ActiveDocument";//getObject()->getDocument()->getName(); 
    buf += ".";
    buf += getObject()->getNameInDocument(); 
    buf += ",[";
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it){
        buf += "\""; 
        buf += *it; 
        buf += "\"";
        if(it != --SubNames.end())
            buf += ",";
    }
    buf += "])";
   
    return buf;
}


PyObject* SelectionObject::getPyObject()
{
    return new SelectionObjectPy(new SelectionObject(*this));
}

