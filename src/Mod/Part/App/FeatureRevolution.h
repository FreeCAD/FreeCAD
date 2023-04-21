/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef PART_FEATUREREVOLUTION_H
#define PART_FEATUREREVOLUTION_H

#include <App/PropertyStandard.h>
#include <Base/Vector3D.h>

#include "PartFeature.h"


namespace Part
{

class PartExport Revolution : public Part::Feature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Revolution);

public:
    Revolution();

    App::PropertyLink Source;
    App::PropertyVector Base;
    App::PropertyVector Axis;
    App::PropertyLinkSub AxisLink;
    App::PropertyFloatConstraint Angle;
    App::PropertyBool Symmetric; //like "Midplane" in PartDesign
    App::PropertyBool Solid;
    App::PropertyString FaceMakerClass;

    /** @name methods override feature */
    //@{
    /// recalculate the feature
    App::DocumentObjectExecReturn *execute() override;
    short mustExecute() const override;

    void onChanged(const App::Property* prop) override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override{
        return "PartGui::ViewProviderRevolution";
    }
    //@}

    /**
     * @brief fetchAxisLink: read AxisLink to obtain the axis parameters and
     * angle span. Note: this routine is re-used in Revolve dialog, hence it
     * is static.
     * @param axisLink (input): the link
     * @param center (output): base point of axis
     * @param dir (output): direction of axis
     * @param angle (output): if edge is an arc of circle, this argument is
     * used to return the angle span of the arc.
     * @return true if link was fetched. false if link was empty. Throws if the
     * link is wrong.
     */
    static bool fetchAxisLink(const App::PropertyLinkSub& axisLink,
                              Base::Vector3d &center,
                              Base::Vector3d &dir,
                              double &angle);

private:
    static App::PropertyFloatConstraint::Constraints angleRangeU;

protected:
    void setupObject() override;
};

} //namespace Part


#endif // PART_FEATUREREVOLUTION_H
