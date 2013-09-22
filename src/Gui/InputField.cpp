/***************************************************************************
 *   Copyright (c) 2013 Juergen Riegel                                     *
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

#include <Base/Console.h>

#include "InputField.h"
using namespace Gui;

// --------------------------------------------------------------------

InputField::InputField ( QWidget * parent )
  : QLineEdit(parent)
{
}

InputField::~InputField()
{
}



/** Sets the preference path to \a path. */
void InputField::setParamGrpPath( const QByteArray& path )
{
//#ifdef FC_DEBUG
//  if (getWindowParameter().isValid())
//  {
//    if ( paramGrpPath() != path )
//      Base::Console().Warning("Widget already attached\n");
//  }
//#endif
//
//  if ( paramGrpPath() != path )
//  {
//    if ( setGroupName( path ) )
//    {
//      m_sPrefGrp = path;
//      assert(getWindowParameter().isValid());
//      getWindowParameter()->Attach(this);
//    }
//  }
}

/** Returns the widget's preferences path. */
QByteArray InputField::paramGrpPath() const
{
  return m_sPrefGrp;
}


// --------------------------------------------------------------------


#include "moc_InputField.cpp"
