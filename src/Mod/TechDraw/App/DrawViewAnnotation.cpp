/***************************************************************************
 *   Copyright (c) 2012 Yorik van Havre <yorik@uncreated.net>              *
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
# include <iomanip>
# include <sstream>
#endif

#include "DrawViewAnnotation.h"
#include "Preferences.h"


using namespace TechDraw;

//===========================================================================
// DrawViewAnnotation
//===========================================================================

PROPERTY_SOURCE(TechDraw::DrawViewAnnotation, TechDraw::DrawView)

const char* DrawViewAnnotation::TextStyleEnums[]= {"Normal",
                                      "Bold",
                                      "Italic",
                                      "Bold-Italic",
                                      nullptr};

DrawViewAnnotation::DrawViewAnnotation()
{
    static const char *vgroup = "Annotation";

    ADD_PROPERTY_TYPE(Text ,("Default Text"), vgroup, App::Prop_None, "Annotation text");
    ADD_PROPERTY_TYPE(Font ,(Preferences::labelFont().c_str()),
                             vgroup, App::Prop_None, "Font name");
    ADD_PROPERTY_TYPE(TextColor, (Preferences::normalColor()), vgroup, App::Prop_None, "Text color");
    ADD_PROPERTY_TYPE(TextSize, (Preferences::labelFontSizeMM()),
                                 vgroup, App::Prop_None, "Text size");
    ADD_PROPERTY_TYPE(MaxWidth, (-1.0), vgroup, App::Prop_None, "Maximum width of the annotation block (in mm).\n -1 means no maximum width.");
    ADD_PROPERTY_TYPE(LineSpace, (100), vgroup, App::Prop_None, "Line spacing in %. 100 means single spaced,\n 200 would be double spaced.");

    TextStyle.setEnums(TextStyleEnums);
    ADD_PROPERTY_TYPE(TextStyle, ((long)0), vgroup, App::Prop_None, "Text style");

    Scale.setStatus(App::Property::Hidden, true);
    ScaleType.setStatus(App::Property::Hidden, true);
}

void DrawViewAnnotation::onChanged(const App::Property* prop)
{
    if (!isRestoring()) {
        if (prop == &Text ||
            prop == &Font ||
            prop == &TextColor ||
            prop == &TextSize ||
            prop == &LineSpace ||
            prop == &TextStyle ||
            prop == &MaxWidth) {
            requestPaint();
        }
    }
    TechDraw::DrawView::onChanged(prop);
}

void DrawViewAnnotation::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
// transforms properties that had been changed
{
    // also check for changed properties of the base class
    DrawView::handleChangedPropertyType(reader, TypeName, prop);

    // property LineSpace had the App::PropertyInteger and was changed to App::PropertyPercent
    // the above change does not allow lines to be spaced wider than single space, so the change has been
    // reverted, but we have to handle existing files that have a PropertyPercent
    if (prop == &LineSpace && strcmp(TypeName, "App::PropertyPercent") == 0) {
        App::PropertyPercent LineSpaceProperty;
        // restore the PropertyPercent to be able to set its value
        LineSpaceProperty.Restore(reader);
        LineSpace.setValue(LineSpaceProperty.getValue());
    }
    // property MaxWidth had the App::PropertyFloat and was changed to App::PropertyLength
    // the above change did not consider that -1 is a magic value that indicates the line
    // should not be broken.
    else if (prop == &MaxWidth && strcmp(TypeName, "App::PropertyLength") == 0) {
        App::PropertyFloat MaxWidthProperty;
        MaxWidthProperty.Restore(reader);
        MaxWidth.setValue(MaxWidthProperty.getValue());
    }
}

QRectF DrawViewAnnotation::getRect() const
{
    double tSize = TextSize.getValue();
    int lines = Text.getValues().size();
    int chars = 1;
    for (auto& l:Text.getValues()) {
        if ((int)l.size() > chars) {
            chars = (int)l.size();
        }
    }
    int w = chars * std::max(1, (int)tSize);
    int h = lines * std::max(1, (int)tSize);
    return { 0, 0, getScale() * w, getScale() * h};
}

App::DocumentObjectExecReturn *DrawViewAnnotation::execute()
{
    requestPaint();
    return TechDraw::DrawView::execute();
}

// Python Drawing feature ---------------------------------------------------------

namespace App {
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(TechDraw::DrawViewAnnotationPython, TechDraw::DrawViewAnnotation)
template<> const char* TechDraw::DrawViewAnnotationPython::getViewProviderName() const {
    return "TechDrawGui::ViewProviderAnnotation";
}
/// @endcond

// explicit template instantiation
template class TechDrawExport FeaturePythonT<TechDraw::DrawViewAnnotation>;
}
