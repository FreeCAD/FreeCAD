/***************************************************************************
 *   Copyright (c) 2007 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <BRepAlgoAPI_Section.hxx>
# include <Standard_Version.hxx>
#endif

#include "FeaturePartSection.h"


using namespace Part;

PROPERTY_SOURCE(Part::Section, Part::Boolean)


Section::Section()
{
    ADD_PROPERTY_TYPE(Approximation,(false),"Section",App::Prop_None,"Approximate the output edges");
}

short Section::mustExecute() const
{
    if (Approximation.isTouched())
        return 1;
    return 0;
}

BRepAlgoAPI_BooleanOperation* Section::makeOperation(const TopoDS_Shape& base, const TopoDS_Shape& tool) const
{
    // Let's call algorithm computing a section operation:

    bool approx = Approximation.getValue();
    std::unique_ptr<BRepAlgoAPI_Section> mkSection(new BRepAlgoAPI_Section());
    mkSection->Init1(base);
    mkSection->Init2(tool);
    mkSection->Approximation(approx);
    mkSection->Build();
    if (!mkSection->IsDone())
        throw Base::RuntimeError("Section failed");
    return mkSection.release();
}
