/***************************************************************************
 *   Copyright (c) Yorik van Havre          (yorik@uncreated.net) 2012     *
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
#include <iomanip>
#include <sstream>
#endif

#include "FeatureViewAnnotation.h"


using namespace Drawing;
using namespace std;

//===========================================================================
// FeatureViewAnnotation
//===========================================================================

PROPERTY_SOURCE(Drawing::FeatureViewAnnotation, Drawing::FeatureView)


FeatureViewAnnotation::FeatureViewAnnotation(void)
{
    static const char* vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Text, (""), vgroup, App::Prop_None, "The text to be displayed");
    ADD_PROPERTY_TYPE(Font, ("Sans"), vgroup, App::Prop_None, "The name of the font to use");
    ADD_PROPERTY_TYPE(TextColor,
                      (0.0f, 0.0f, 0.0f),
                      vgroup,
                      App::Prop_None,
                      "The color of the text");
}

FeatureViewAnnotation::~FeatureViewAnnotation()
{}

App::DocumentObjectExecReturn* FeatureViewAnnotation::execute(void)
{
    stringstream result, hr, hg, hb;
    const App::Color& c = TextColor.getValue();
    hr << hex << setfill('0') << setw(2) << (int)(255.0 * c.r);
    hg << hex << setfill('0') << setw(2) << (int)(255.0 * c.g);
    hb << hex << setfill('0') << setw(2) << (int)(255.0 * c.b);

    result << "<g transform=\"translate(" << X.getValue() << "," << Y.getValue() << ")"
           << " rotate(" << Rotation.getValue() << ")\">" << endl
           << "<text id=\"" << Label.getValue() << "\"" << endl
           << " font-family=\"" << Font.getValue() << "\"" << endl
           << " font-size=\"" << Scale.getValue() << "\"" << endl
           << " fill=\"#" << hr.str() << hg.str() << hb.str() << "\">" << endl;

    for (vector<string>::const_iterator it = Text.getValues().begin(); it != Text.getValues().end();
         ++it) {
        result << "<tspan x=\"0\" dy=\"1em\">" << it->c_str() << "</tspan>" << endl;
    }

    result << "</text>" << endl << "</g>" << endl;

    // Apply the resulting fragment
    ViewResult.setValue(result.str().c_str());

    return App::DocumentObject::StdReturn;
}

// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Drawing::FeatureViewAnnotationPython, Drawing::FeatureViewAnnotation)
template<>
const char* Drawing::FeatureViewAnnotationPython::getViewProviderName(void) const
{
    return "DrawingGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<Drawing::FeatureViewAnnotation>;
}  // namespace App
