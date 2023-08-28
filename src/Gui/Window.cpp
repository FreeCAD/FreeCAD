/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <boost/algorithm/string.hpp>
#include <qglobal.h>

#include <App/Application.h>

#include "Window.h"


using namespace Gui;

//**************************************************************************
//**************************************************************************
// WindowParameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//**************************************************************************
// Construction/Destruction

WindowParameter::WindowParameter(const char *name)
{
    // not allowed to use a Window without a name, see the constructor
    // of a DockWindow or a other QT Widget
    assert(name);

    // if string is empty do not create group
    if ( strcmp(name, "") != 0 )
      _handle = getDefaultParameter()->GetGroup( name );
}

WindowParameter::~WindowParameter()
{
    connParamChanged.disconnect();
}

/** Sets the group of the window to \a name */
bool WindowParameter::setGroupName(const char* name)
{
    // cannot change parameter group
    if (_handle.isValid()) {
        return false;
    }

    assert(name);

    std::string prefGroup = name;
    const auto& list = App::GetApplication().GetParameterSetList();

    auto found = std::find_if(list.begin(), list.end(), [prefGroup](auto item) {
        return boost::starts_with(prefGroup, item.first);
    });

    if (found != list.end()) {
        _handle = App::GetApplication().GetParameterGroupByPath(name);
    }
    else {
        _handle = getDefaultParameter()->GetGroup(name);
    }

    return true;
}

void WindowParameter::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
    Q_UNUSED(rCaller);
    Q_UNUSED(sReason);
}

ParameterGrp::handle  WindowParameter::getWindowParameter()
{
    return _handle;
}

/**
 * Returns a handle to the parameter group to the user parameter
 * under BaseApp/Preferences.
 */
ParameterGrp::handle  WindowParameter::getDefaultParameter()
{
    return App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences");
}
