/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2008     *
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



#include "SketchObjectSF.h"
#include <Base/Console.h>


using namespace Sketcher;


PROPERTY_SOURCE(Sketcher::SketchObjectSF, Part::Part2DObject)


SketchObjectSF::SketchObjectSF()
{
    ADD_PROPERTY_TYPE(SketchFlatFile,(0),"",(App::PropertyType)(App::Prop_None),
        "SketchFlat file (*.skf) which defines this sketch");
}

short SketchObjectSF::mustExecute() const
{
    if (SketchFlatFile.isTouched())
        return 1;
    return 0;
}

App::DocumentObjectExecReturn *SketchObjectSF::execute(void)
{
    Base::Console().Warning("%s: This feature is deprecated and won't be longer supported in future FreeCAD versions\n",this->getNameInDocument());
    // do nothing 
    return App::DocumentObject::StdReturn;
}
