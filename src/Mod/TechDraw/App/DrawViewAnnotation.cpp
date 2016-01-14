/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2012     *
 *   Copyright (c) 2013 Luke Parry <l.parry@warwick.ac.uk>                 *
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

#include "DrawViewAnnotation.h"

using namespace TechDraw;
using namespace std;


//===========================================================================
// DrawViewAnnotation
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewAnnotation, TechDraw::DrawView)


DrawViewAnnotation::DrawViewAnnotation(void) 
{
    static const char *vgroup = "Annotation";

    Base::Reference<ParameterGrp> hGrp = App::GetApplication().GetUserParameter()
        .GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod/Drawing");
    std::string fontName = hGrp->GetASCII("LabelFont", "osifont");

    ADD_PROPERTY_TYPE(Text ,("Default Text"),vgroup,App::Prop_None,"The text to be displayed");
    ADD_PROPERTY_TYPE(Font ,(fontName.c_str())         ,vgroup,App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(TextColor,(0.0f,0.0f,0.0f),vgroup,App::Prop_None,"The color of the text");

    ADD_PROPERTY_TYPE(TextSize,(8),vgroup,App::Prop_None,"The size of the text in mm");
    
    Scale.StatusBits.set(3);         //hide scale.  n/a for Annotation
    ScaleType.StatusBits.set(3);
}

DrawViewAnnotation::~DrawViewAnnotation()
{
}

App::DocumentObjectExecReturn *DrawViewAnnotation::execute(void)
{
    return App::DocumentObject::StdReturn;
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewAnnotationPython, TechDraw::DrawViewAnnotation)
template<> const char* TechDraw::DrawViewAnnotationPython::getViewProviderName(void) const {
    return "TechDrawGui::ViewProviderAnnotation";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<TechDraw::DrawViewAnnotation>;
}
