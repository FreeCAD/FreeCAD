/***************************************************************************
 *   Copyright (c) 2020 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef PARTDESIGN_FEATURE_EXTRUDE_H
#define PARTDESIGN_FEATURE_EXTRUDE_H

#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>
#include "FeatureSketchBased.h"

namespace PartDesign
{

class PartDesignExport FeatureExtrude : public ProfileBased
{
    PROPERTY_HEADER(PartDesign::FeatureExtrude);

public:
    FeatureExtrude();

    App::PropertyEnumeration Type;
    App::PropertyLength      Length;
    App::PropertyLength      Length2;
    App::PropertyBool        UseCustomVector;
    App::PropertyVector      Direction;
    App::PropertyBool        AlongSketchNormal;
    App::PropertyLength      Offset;
    App::PropertyLinkSub     ReferenceAxis;

    static App::PropertyQuantityConstraint::Constraints signedLengthConstraint;

    /** @name methods override feature */
    //@{
    short mustExecute() const;
    //@}

protected:
    Base::Vector3d computeDirection(const Base::Vector3d& sketchVector);
};

} //namespace PartDesign


#endif // PARTDESIGN_FEATURE_EXTRUDE_H
