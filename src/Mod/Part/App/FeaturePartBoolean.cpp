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
# include <BRepAlgoAPI_BooleanOperation.hxx>
#include <memory>
#endif

#include "FeaturePartBoolean.h"


using namespace Part;

PROPERTY_SOURCE_ABSTRACT(Part::Boolean, Part::Feature)


Boolean::Boolean(void) : myBoolOp(0)
{
    ADD_PROPERTY(Base,(0));
    ADD_PROPERTY(Tool,(0));
}

short Boolean::mustExecute() const
{
    if (Base.getValue() && Tool.getValue()) {
        if (Base.isTouched())
            return 1;
        if (Tool.isTouched())
            return 1;
    }
    return 0;
}

App::DocumentObjectExecReturn *Boolean::execute(void)
{
    try {
#if defined(__GNUC__) && defined (FC_OS_LINUX)
        Base::SignalException se;
#endif
        Part::Feature *base = dynamic_cast<Part::Feature*>(Base.getValue());
        Part::Feature *tool = dynamic_cast<Part::Feature*>(Tool.getValue());

        if (!base || !tool)
            return new App::DocumentObjectExecReturn("Linked object is not a Part object");

        // Now, let's get the TopoDS_Shape
        TopoDS_Shape BaseShape = base->Shape.getValue();
        TopoDS_Shape ToolShape = tool->Shape.getValue();

        std::auto_ptr<BRepAlgoAPI_BooleanOperation> mkBool(makeOperation(BaseShape, ToolShape));
        if (!mkBool->IsDone()) {
            return new App::DocumentObjectExecReturn("Boolean operation failed");
        }
        const TopoDS_Shape& resShape = mkBool->Shape();
        if (resShape.IsNull()) {
            return new App::DocumentObjectExecReturn("Resulting shape is invalid");
        }

        // tmp. set boolean operation pointer
        this->myBoolOp = mkBool.get();
        this->Shape.setValue(resShape);
        this->myBoolOp = 0;
        return App::DocumentObject::StdReturn;
    }
    catch (...) {
        this->myBoolOp = 0;
        return new App::DocumentObjectExecReturn("A fatal error occurred when running boolean operation");
    }
}
