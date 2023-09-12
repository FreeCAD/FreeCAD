/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef POINTS_VIEW_FEATURE_H
#define POINTS_VIEW_FEATURE_H

#include "PointsFeature.h"


namespace Points
{

/*! For the Structured class it is expected that the Point property has Width*Height vertices
  and that with respect to their x,y coordinates they are ordered in a grid structure.
  If a point is marked invalid then one of its coordinates is set to NaN.
 */
class PointsExport Structured: public Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Points::Structured);

public:
    /// Constructor
    Structured();

    App::PropertyInteger Width;  /**< The width of the structured cloud. */
    App::PropertyInteger Height; /**< The height of the structured cloud. */

    /** @name methods override Feature */
    //@{
    /// recalculate the Feature
    App::DocumentObjectExecReturn* execute() override;
    /// returns the type name of the ViewProvider
    const char* getViewProviderName() const override
    {
        return "PointsGui::ViewProviderStructured";
    }
    //@}
};

using StructuredCustom = App::FeatureCustomT<Structured>;

}  // namespace Points


#endif
