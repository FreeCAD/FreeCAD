/***************************************************************************
 *   Copyright (c) Riegel         <juergen.riegel@web.de>                  *
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

#ifndef _PreComp_
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include "Persistence.h"

using namespace Base;

TYPESYSTEM_SOURCE_ABSTRACT(Base::Persistence,Base::BaseClass);


//**************************************************************************
// Construction/Destruction



//**************************************************************************
// separator for other implemetation aspects

unsigned int Persistence::getMemSize (void) const
{
    // you have to implement this method in all descending classes!
    assert(0);
    return 0;
}

void Persistence::Save (Writer &/*writer*/) const
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::Restore(XMLReader &/*reader*/)
{
    // you have to implement this method in all descending classes!
    assert(0);
}

void Persistence::SaveDocFile (Writer &/*writer*/) const
{
}

void Persistence::RestoreDocFile(Reader &/*reader*/)
{
}
