/***************************************************************************
 *   Copyright (c) 2014 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include <Base/Exception.h>
#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/FileInfo.h>
#include <Base/Vector3D.h>
#include <Base/Tools2D.h>

#include <App/Application.h>

#include "Geometry.h"

#include <iostream>
#include <iterator>

#include "DrawParametricTemplate.h"
#include <Mod/TechDraw/App/DrawParametricTemplatePy.h>

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawParametricTemplate, TechDraw::DrawTemplate)

DrawParametricTemplate::DrawParametricTemplate(void)
{
    static const char *group = "Page";
    ADD_PROPERTY_TYPE(Template ,(""),group, (App::PropertyType) App::Prop_None,"Template script");
}

DrawParametricTemplate::~DrawParametricTemplate()
{
}


PyObject *DrawParametricTemplate::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawParametricTemplatePy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

unsigned int DrawParametricTemplate::getMemSize(void) const
{
    return 0;
}

double DrawParametricTemplate::getWidth() const {
  throw Base::NotImplementedError("Need to Implement");
}


double DrawParametricTemplate::getHeight() const {
  throw Base::NotImplementedError("Need to Implement");
}


short DrawParametricTemplate::mustExecute() const
{
    return App::DocumentObject::mustExecute();
}

/// get called by the container when a Property was changed
void DrawParametricTemplate::onChanged(const App::Property* prop)
{
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawParametricTemplate::execute(void)
{
    std::string temp = Template.getValue();
    if (!temp.empty()) {
        Base::FileInfo tfi(temp);
        if (!tfi.isReadable()) {
            // if there is a old absolute template file set use a redirect
            return App::DocumentObject::StdReturn;
        }
        try {
            Base::Interpreter().runFile(temp.c_str(), true);
        }
    catch(const Base::Exception& e) {
        PyErr_SetString(PyExc_ImportError, e.what());
        return App::DocumentObject::StdReturn;
    }
    }

    return App::DocumentObject::StdReturn;
}

int DrawParametricTemplate::drawLine(double x1, double y1, double x2, double y2)
{
    TechDrawGeometry::Generic *line = new TechDrawGeometry::Generic();

    line->points.push_back(Base::Vector2d(x1, y1));
    line->points.push_back(Base::Vector2d(x2, y2));

    geom.push_back(line); // Push onto geometry stack
    return geom.size() -1;
}

int DrawParametricTemplate::clearGeometry()
{
    for(std::vector<TechDrawGeometry::BaseGeom *>::iterator it = geom.begin(); it != geom.end(); ++it) {
        delete *it;
        *it = 0;
    }
    geom.clear();
    return 0;
}

// Python Template feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawParametricTemplatePython, TechDraw::DrawParametricTemplate)
template<> const char* TechDraw::DrawParametricTemplatePython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderPython";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawParametricTemplate>;
}
