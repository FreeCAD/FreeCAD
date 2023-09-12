/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2007     *
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

#ifndef _FeatureViewPart_h_
#define _FeatureViewPart_h_

#include <App/DocumentObject.h>
#include <App/FeaturePython.h>
#include <App/PropertyLinks.h>

#include "FeatureView.h"


namespace Drawing
{

/** Base class of all View Features in the drawing module
 */
class DrawingExport FeatureViewPart: public FeatureView
{
    PROPERTY_HEADER(Part::FeatureViewPart);

public:
    /// Constructor
    FeatureViewPart(void);
    virtual ~FeatureViewPart();

    App::PropertyLink Source;
    App::PropertyVector Direction;
    App::PropertyBool ShowHiddenLines;
    App::PropertyBool ShowSmoothLines;
    App::PropertyFloat LineWidth;
    App::PropertyFloat HiddenWidth;
    App::PropertyFloatConstraint Tolerance;


    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    virtual App::DocumentObjectExecReturn* execute(void);
    //@}

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const
    {
        return "DrawingGui::ViewProviderDrawingView";
    }

private:
    static App::PropertyFloatConstraint::Constraints floatRange;
};

using FeatureViewPartPython = App::FeaturePythonT<FeatureViewPart>;


}  // namespace Drawing


#endif
