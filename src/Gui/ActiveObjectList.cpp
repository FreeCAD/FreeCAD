/***************************************************************************
*   (c) Jürgen Riegel (juergen.riegel@web.de) 2014                        *
*                                                                         *
*   This file is part of the FreeCAD CAx development system.              *
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU Library General Public License (LGPL)   *
*   as published by the Free Software Foundation; either version 2 of     *
*   the License, or (at your option) any later version.                   *
*   for detail see the LICENCE text file.                                 *
*                                                                         *
*   FreeCAD is distributed in the hope that it will be useful,            *
*   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
*   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
*   GNU Library General Public License for more details.                  *
*                                                                         *
*   You should have received a copy of the GNU Library General Public     *
*   License along with FreeCAD; if not, write to the Free Software        *
*   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  *
*   USA                                                                   *
*                                                                         *
*   Juergen Riegel 2014                                                   *
***************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_

#endif

#include "ActiveObjectList.h"
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>



using namespace Gui;


void Gui::ActiveObjectList::setObject(App::DocumentObject* obj, const char* name, const Gui::HighlightMode& mode)
{
	if (obj){
		if (hasObject(name)){
			Gui::Application::Instance->activeDocument()->signalHighlightObject(*dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->activeDocument()->getViewProvider(getObject<App::DocumentObject*>(name))), mode, false);
		}
		_ObjectMap[name] = obj;

		Gui::Application::Instance->activeDocument()->signalHighlightObject(*dynamic_cast<Gui::ViewProviderDocumentObject*>(Gui::Application::Instance->activeDocument()->getViewProvider(obj)), mode, true);
	}
}

bool Gui::ActiveObjectList::hasObject(const char*name)const 
{
	return _ObjectMap.find(name) != _ObjectMap.end();
}
