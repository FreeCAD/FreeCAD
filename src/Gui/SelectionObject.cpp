/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <FreeCAD@juergen-riegel.net>         *
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

#include <sstream>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Interpreter.h>

#include "SelectionObject.h"
#include "Selection.h"
#include "Application.h"
#include "Command.h"
#include <Gui/SelectionObjectPy.h>

using namespace Gui;


TYPESYSTEM_SOURCE_ABSTRACT(Gui::SelectionObject, Base::BaseClass)

SelectionObject::SelectionObject()
{
}

SelectionObject::SelectionObject(const Gui::SelectionChanges& msg)
{
    FeatName = msg.pObjectName ? msg.pObjectName : "";
    DocName = msg.pDocName ? msg.pDocName : "";
    TypeName = msg.pTypeName ? msg.pTypeName : "";
    if (msg.pSubName) {
        SubNames.push_back(msg.pSubName);
        SelPoses.emplace_back(msg.x, msg.y, msg.z);
    }
}

SelectionObject::SelectionObject(App::DocumentObject* obj)
{
    FeatName = obj->getNameInDocument();
    DocName = obj->getDocument()->getName();
    TypeName = obj->getTypeId().getName();
}

SelectionObject::~SelectionObject()
{
}

const App::DocumentObject * SelectionObject::getObject(void) const
{
    if (!DocName.empty()) {
        App::Document *doc = App::GetApplication().getDocument(DocName.c_str());
        if (doc && !FeatName.empty())
            return doc->getObject(FeatName.c_str());
    }
    return 0;
}

App::DocumentObject * SelectionObject::getObject(void)
{
    if (!DocName.empty()) {
        App::Document *doc = App::GetApplication().getDocument(DocName.c_str());
        if (doc && !FeatName.empty())
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
    std::ostringstream str;
    str << "(" << Gui::Command::getObjectCmd(getObject()) << ",[";
    for(std::vector<std::string>::const_iterator it = SubNames.begin();it!=SubNames.end();++it)
        str << "'" << *it << "',";
    str << "])";
    return str.str();
}

PyObject* SelectionObject::getPyObject()
{
    return new SelectionObjectPy(new SelectionObject(*this));
}
