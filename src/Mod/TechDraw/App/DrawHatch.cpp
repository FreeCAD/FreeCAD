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
#endif

#include <iomanip>

#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/Parameter.h>

#include "DrawHatch.h"

using namespace TechDraw;
using namespace std;

PROPERTY_SOURCE(TechDraw::DrawHatch, TechDraw::DrawView)


DrawHatch::DrawHatch(void)
{
    static const char *vgroup = "Hatch";

    ADD_PROPERTY_TYPE(PartView, (0), vgroup, (App::PropertyType)(App::Prop_None), "Parent view feature");
    ADD_PROPERTY_TYPE(DirProjection ,(0,0,1.0)    ,vgroup,App::Prop_None,"Projection direction when Hatch was defined");     //sb RO?
    ADD_PROPERTY_TYPE(Edges,(0,0),vgroup,(App::PropertyType)(App::Prop_None),"The outline of the hatch area");
    ADD_PROPERTY_TYPE(HatchPattern ,(""),vgroup,App::Prop_None,"The hatch pattern file for this area");
    ADD_PROPERTY_TYPE(HatchColor,(0.0f,0.0f,0.0f),vgroup,App::Prop_None,"The color of the hatch area");

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/TechDraw");

    std::string defaultDir = App::Application::getResourceDir() + "Mod/Drawing/patterns";
    QString patternDir = QString::fromStdString(hGrp->GetASCII("PatternDir", defaultDir.c_str()));
    if (patternDir.isEmpty()) {                                        //PatternDir key probably has null value
        patternDir = QString::fromStdString(defaultDir);
    }
    std::string defaultFileName = "simple.svg";
    QString patternFileName = QString::fromStdString(hGrp->GetASCII("PatternFile",defaultFileName.c_str()));
    if (patternFileName.isEmpty()) {
        patternFileName = QString::fromStdString(defaultFileName);
    }
    patternFileName = patternDir + QString::fromUtf8("/")  + patternFileName;
    HatchPattern.setValue(patternFileName.toUtf8().constData());
}

DrawHatch::~DrawHatch()
{
}

void DrawHatch::onChanged(const App::Property* prop)
{
    if (prop == &PartView      ||
        prop == &Edges         ||
        prop == &HatchPattern  ||
        prop == &HatchColor) {
        if (!isRestoring()) {
              DrawHatch::execute();
              if (PartView.getValue()) {
                  PartView.getValue()->touch();
                  PartView.getValue()->recompute();
              }
          }
    }
    App::DocumentObject::onChanged(prop);
}

App::DocumentObjectExecReturn *DrawHatch::execute(void)
{
    //TODO: need to refresh DrawViewPart to reflect change in hatch
    return App::DocumentObject::StdReturn;
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
