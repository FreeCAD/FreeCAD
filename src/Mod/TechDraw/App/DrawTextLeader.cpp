/***************************************************************************
 *   Copyright (c) 2019 Wanderer Fan <wandererfan@gmail.com>               *
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

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Parameter.h>

#include "DrawUtil.h"

#include <Mod/TechDraw/App/DrawTextLeaderPy.h>  // generated from DrawTextLeaderPy.xml
#include "DrawTextLeader.h"

using namespace TechDraw;

//===========================================================================
// DrawTextLeader - DrawLeaderLine + movable text block
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawTextLeader, TechDraw::DrawLeaderLine)

DrawTextLeader::DrawTextLeader(void)
{
    static const char *group = "Text Leader";

    ADD_PROPERTY_TYPE(LeaderText, (""), group, App::Prop_None, "Leader text");
    Base::Vector3d pos(0.0,0.0,0.0);
    ADD_PROPERTY_TYPE(TextPosition, (pos), group, App::Prop_None, "Text position relative to parent");
}

DrawTextLeader::~DrawTextLeader()
{
}

void DrawTextLeader::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        //nothing in particular
    }
    DrawView::onChanged(prop);

}

short DrawTextLeader::mustExecute() const
{
    bool result = 0;
    if (!isRestoring()) {
        result =  (LeaderText.isTouched());
    }
    if (result) {
        return result;
    }

    return DrawView::mustExecute();
}

App::DocumentObjectExecReturn *DrawTextLeader::execute(void)
{ 
//    Base::Console().Message("DTL::execute()\n");
    if (!keepUpdated()) {
        return App::DocumentObject::StdReturn;
    }
    return DrawView::execute();
}

PyObject *DrawTextLeader::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawTextLeaderPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawTextLeaderPython, TechDraw::DrawTextLeader)
template<> const char* TechDraw::DrawTextLeaderPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderTextLeader";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawTextLeader>;
}

