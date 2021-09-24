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

#include <qglobal.h>
#include "Window.h"

#include <Base/Console.h>
#include <App/Application.h>

using namespace Gui;

//**************************************************************************
//**************************************************************************
// WindowParameter
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++


//**************************************************************************
// Construction/Destruction

WindowParameter::WindowParameter(const char *name)
{
  // not allowed to use a Window without a name, see the constructor of a DockWindow or a other QT Widget
  assert(name);
  //printf("Instanceate:%s\n",name);

  // if string is empty do not create group
  if ( strcmp(name, "") != 0 )
    _handle = getDefaultParameter()->GetGroup( name );
}

WindowParameter::~WindowParameter()
{
}

/** Sets the group of the window to \a name */
bool WindowParameter::setGroupName(const char* name)
{
  if (_handle.isValid())
    return false; // cannot change parameter group

  assert(name);
  std::string prefGroup = name;
  if (prefGroup.compare(0,15,"User parameter:") == 0 ||
      prefGroup.compare(0,17,"System parameter:") == 0)
    _handle = App::GetApplication().GetParameterGroupByPath( name );
  else
    _handle = getDefaultParameter()->GetGroup( name );
  return true;
}

void WindowParameter::OnChange(Base::Subject<const char*> &rCaller, const char * sReason)
{
  Q_UNUSED(rCaller);
  Q_UNUSED(sReason);
  Base::Console().Log("Parameter has changed and window (%s) has not overridden this function!",_handle->GetGroupName());
}

ParameterGrp::handle  WindowParameter::getWindowParameter(void)
{
  return _handle;
}

/**
 * Returns a handle to the parameter group to the user parameter
 * under BaseApp/Preferences.
 */
ParameterGrp::handle  WindowParameter::getDefaultParameter(void)
{
  return App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences");
}
