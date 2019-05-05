/***************************************************************************
 *   Copyright (c) 2015 WandererFan <wandererfan@gmail.com>                *
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
#include <Precision.hxx>
#include <cmath>

#endif

#include <iomanip>

# include <QFile>
# include <QFileInfo>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

#include "DrawViewPart.h"
#include "DrawHatch.h"

#include <Mod/TechDraw/App/DrawHatchPy.h>  // generated from DrawHatchPy.xml

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawHatch, App::DocumentObject)


DrawHatch::DrawHatch(void)
{
    static const char *vgroup = "Hatch";

    ADD_PROPERTY_TYPE(DirProjection ,(0,0,1.0)    ,vgroup,App::Prop_None,"Projection direction when Hatch was defined");     //sb RO?
    ADD_PROPERTY_TYPE(Source,(0),vgroup,(App::PropertyType)(App::Prop_None),"The View + Face to be hatched");
    Source.setScope(App::LinkScope::Global);
    ADD_PROPERTY_TYPE(HatchPattern ,(""),vgroup,App::Prop_None,"The hatch pattern file for this area");

    DirProjection.setStatus(App::Property::ReadOnly,true);

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw/Files");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/TechDraw/Patterns/";
    std::string defaultFileName = defaultDir + "simple.svg";
    QString patternFileName = QString::fromStdString(hGrp->GetASCII("FileHatch",defaultFileName.c_str()));
    if (patternFileName.isEmpty()) {
        patternFileName = QString::fromStdString(defaultFileName);
    }
    QFileInfo tfi(patternFileName);
        if (tfi.isReadable()) {
            HatchPattern.setValue(patternFileName.toUtf8().constData());
        }
}

DrawHatch::~DrawHatch()
{
}

void DrawHatch::onChanged(const App::Property* prop)
{
    if ((prop == &Source)         ||
        (prop == &HatchPattern)) {
        if (!isRestoring()) {
              DrawHatch::execute();
          }
    }
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawHatch::execute(void)
{
    DrawViewPart* parent = getSourceView();
    if (parent) {
        parent->requestPaint();
    }
    return App::DocumentObject::StdReturn;
}

DrawViewPart* DrawHatch::getSourceView(void) const
{
    App::DocumentObject* obj = Source.getValue();
    DrawViewPart* result = dynamic_cast<DrawViewPart*>(obj);
    return result;
}

PyObject *DrawHatch::getPyObject(void)
{
    if (PythonObject.is(Py::_None())) {
        // ref counter is set to 1
        PythonObject = Py::Object(new DrawHatchPy(this),true);
    }
    return Py::new_reference_to(PythonObject);
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawHatchPython, TechDraw::DrawHatch)
template<> const char* TechDraw::DrawHatchPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderHatch";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawHatch>;
}
