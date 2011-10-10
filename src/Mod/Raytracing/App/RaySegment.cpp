/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
# include <sstream>
#endif


#include <strstream>
#include <Base/Writer.h>
#include <Base/Reader.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>

#include "RaySegment.h"

using namespace Raytracing;


//===========================================================================
// RaySegment
//===========================================================================

PROPERTY_SOURCE(Raytracing::RaySegment, App::DocumentObject)



RaySegment::RaySegment(void) 
{
    App::PropertyType type = (App::PropertyType)(App::Prop_Output|App::Prop_Hidden);
    ADD_PROPERTY_TYPE(Result ,(0),0,type,"Resulting SVG fragment of that view");
}

RaySegment::~RaySegment()
{
}

App::DocumentObjectExecReturn *RaySegment::execute(void)
{
    return App::DocumentObject::StdReturn;
}
