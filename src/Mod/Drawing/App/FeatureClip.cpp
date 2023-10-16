/***************************************************************************
 *   Copyright (c) Yorik van Havre <yorik@uncreated.net> 2012              *
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
#include <iostream>
#include <sstream>
#endif

#include "FeatureClip.h"
#include "FeatureView.h"


using namespace Drawing;
using namespace std;

//===========================================================================
// FeaturePage
//===========================================================================

PROPERTY_SOURCE(Drawing::FeatureClip, App::DocumentObjectGroup)

FeatureClip::FeatureClip(void)
{
    static const char* group = "Drawing view";
    App::PropertyType hidden = (App::PropertyType)(App::Prop_Hidden);
    ADD_PROPERTY_TYPE(ViewResult, (""), group, hidden, "Resulting SVG view of this clip");
    ADD_PROPERTY_TYPE(X,
                      (10),
                      group,
                      App::Prop_None,
                      "The left margin of the view area of this clip");
    ADD_PROPERTY_TYPE(Y,
                      (10),
                      group,
                      App::Prop_None,
                      "The top margin of the view area of this clip");
    ADD_PROPERTY_TYPE(Height,
                      (10),
                      group,
                      App::Prop_None,
                      "The height of the view area of this clip");
    ADD_PROPERTY_TYPE(Width,
                      (10),
                      group,
                      App::Prop_None,
                      "The width of the view area of this clip");
    ADD_PROPERTY_TYPE(ShowFrame,
                      (0),
                      group,
                      App::Prop_None,
                      "Specifies if the clip frame appears on the page or not");
    // The 'Visible' property is handled by the view provider exclusively. It has the 'Output' flag
    // set to avoid to call the execute() method. The view provider touches the page object,
    // instead.
    App::PropertyType propType =
        static_cast<App::PropertyType>(App::Prop_Hidden | App::Prop_Output);
    ADD_PROPERTY_TYPE(Visible,
                      (true),
                      group,
                      propType,
                      "Control whether frame is visible in page object");
}

FeatureClip::~FeatureClip()
{}

/// get called by the container when a Property was changed
void FeatureClip::onChanged(const App::Property* prop)
{
    App::DocumentObjectGroup::onChanged(prop);
}

App::DocumentObjectExecReturn* FeatureClip::execute(void)
{
    ostringstream svg;

    // creating clip path
    svg << "<clipPath id=\"" << Label.getValue() << "\">"
        << "<rect x=\"" << X.getValue() << "\""
        << " y=\"" << Y.getValue() << "\""
        << " width=\"" << Width.getValue() << "\""
        << " height=\"" << Height.getValue() << "\"/></clipPath>" << endl;

    // show clip frame on the page if needed

    if (ShowFrame.getValue()) {
        svg << "<rect fill=\"None\" stroke=\"#ff0000\" stroke-width=\"1px\""
            << " x=\"" << X.getValue() << "\""
            << " y=\"" << Y.getValue() << "\""
            << " width=\"" << Width.getValue() << "\""
            << " height=\"" << Height.getValue() << "\"/>" << endl;
    }

    // create clipped group
    svg << "<g clip-path=\"url(#" << Label.getValue() << ")\">" << endl;

    // get through the children and collect all the views
    const vector<App::DocumentObject*>& Grp = Group.getValues();
    for (vector<App::DocumentObject*>::const_iterator It = Grp.begin(); It != Grp.end(); ++It) {
        if ((*It)->getTypeId().isDerivedFrom(Drawing::FeatureView::getClassTypeId())) {
            Drawing::FeatureView* View = static_cast<Drawing::FeatureView*>(*It);
            svg << View->ViewResult.getValue() << endl;
        }
    }

    // closing clipped group
    svg << "</g>" << endl;

    ViewResult.setValue(svg.str().c_str());
    return App::DocumentObject::StdReturn;
}
