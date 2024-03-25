/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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
#include <TopoDS_Shape.hxx>
#include <sstream>
#endif

#include <Mod/Part/App/PartFeature.h>

#include "FeatureViewPart.h"
#include "ProjectionAlgos.h"


using namespace Drawing;
using namespace std;

//===========================================================================
// FeatureViewPart
//===========================================================================

App::PropertyFloatConstraint::Constraints FeatureViewPart::floatRange = {0.01, 5.0, 0.05};

PROPERTY_SOURCE(Drawing::FeatureViewPart, Drawing::FeatureView)


FeatureViewPart::FeatureViewPart(void)
{
    static const char* group = "Shape view";
    static const char* vgroup = "Drawing view";

    ADD_PROPERTY_TYPE(Direction, (0, 0, 1.0), group, App::Prop_None, "Projection direction");
    ADD_PROPERTY_TYPE(Source, (nullptr), group, App::Prop_None, "Shape to view");
    ADD_PROPERTY_TYPE(ShowHiddenLines,
                      (false),
                      group,
                      App::Prop_None,
                      "Control the appearance of the dashed hidden lines");
    ADD_PROPERTY_TYPE(ShowSmoothLines,
                      (false),
                      group,
                      App::Prop_None,
                      "Control the appearance of the smooth lines");
    ADD_PROPERTY_TYPE(LineWidth,
                      (0.35),
                      vgroup,
                      App::Prop_None,
                      "The thickness of the viewed lines");
    ADD_PROPERTY_TYPE(HiddenWidth,
                      (0.15),
                      vgroup,
                      App::Prop_None,
                      "The thickness of the hidden lines, if enabled");
    ADD_PROPERTY_TYPE(Tolerance, (0.05), vgroup, App::Prop_None, "The tessellation tolerance");
    Tolerance.setConstraints(&floatRange);
}

FeatureViewPart::~FeatureViewPart()
{}

App::DocumentObjectExecReturn* FeatureViewPart::execute(void)
{
    std::stringstream result;
    std::string ViewName = Label.getValue();

    App::DocumentObject* link = Source.getValue();
    if (!link) {
        return new App::DocumentObjectExecReturn("No object linked");
    }
    if (!link->isDerivedFrom<Part::Feature>()) {
        return new App::DocumentObjectExecReturn("Linked object is not a Part object");
    }
    TopoDS_Shape shape = static_cast<Part::Feature*>(link)->Shape.getShape().getShape();
    if (shape.IsNull()) {
        return new App::DocumentObjectExecReturn("Linked shape object is empty");
    }
    Base::Vector3d Dir = Direction.getValue();
    bool hidden = ShowHiddenLines.getValue();
    bool smooth = ShowSmoothLines.getValue();

    try {
        ProjectionAlgos Alg(shape, Dir);
        result << "<g" << " id=\"" << ViewName << "\"" << endl
               << "   transform=\"rotate(" << Rotation.getValue() << "," << X.getValue() << ","
               << Y.getValue() << ") translate(" << X.getValue() << "," << Y.getValue()
               << ") scale(" << Scale.getValue() << "," << Scale.getValue() << ")\"" << endl
               << "  >" << endl;

        ProjectionAlgos::ExtractionType type = ProjectionAlgos::Plain;
        if (hidden) {
            type = (ProjectionAlgos::ExtractionType)(type | ProjectionAlgos::WithHidden);
        }
        if (smooth) {
            type = (ProjectionAlgos::ExtractionType)(type | ProjectionAlgos::WithSmooth);
        }
        ProjectionAlgos::XmlAttributes visible_style = {
            {"stroke-width", to_string(this->LineWidth.getValue() / this->Scale.getValue())}};
        ProjectionAlgos::XmlAttributes hidden_style = {
            {"stroke-width", to_string(this->HiddenWidth.getValue() / this->Scale.getValue())}};
        result << Alg.getSVG(type,
                             this->Tolerance.getValue(),
                             visible_style,
                             visible_style,
                             visible_style,
                             hidden_style,
                             hidden_style,
                             hidden_style);

        result << "</g>" << endl;

        // Apply the resulting fragment
        ViewResult.setValue(result.str().c_str());

        return App::DocumentObject::StdReturn;
    }
    catch (Standard_Failure& e) {
        return new App::DocumentObjectExecReturn(e.GetMessageString());
    }
}


// Python Drawing feature ---------------------------------------------------------

namespace App
{
/// @cond DOXERR
PROPERTY_SOURCE_TEMPLATE(Drawing::FeatureViewPartPython, Drawing::FeatureViewPart)
template<>
const char* Drawing::FeatureViewPartPython::getViewProviderName(void) const
{
    return "DrawingGui::ViewProviderDrawingView";
}
/// @endcond

// explicit template instantiation
template class DrawingExport FeaturePythonT<Drawing::FeatureViewPart>;
}  // namespace App
