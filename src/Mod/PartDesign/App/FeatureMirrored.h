// SPDX-License-Identifier: LGPL-2.1-or-later

/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#pragma once

#include "FeatureTransformed.h"


namespace PartDesign
{

class PartDesignExport Mirrored: public PartDesign::Transformed
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesign::Mirrored);

public:
    Mirrored();

    App::PropertyLinkSub MirrorPlane;

    /** @name methods override feature */
    //@{
    short mustExecute() const override;

    /// returns the type name of the view provider
    const char* getViewProviderName() const override
    {
        return "PartDesignGui::ViewProviderMirrored";
    }
    //@}

    /** Create transformations
     * Returns a list containing one transformation since the first, untransformed instance
     * is not counted. The transformation will mirror the shape it is applied to on a plane
     * If MirrorPlane contains a feature and a face name, then the mirror plane will be
     * the given face, which must be planar
     */
    const std::list<gp_Trsf> getTransformations(const std::vector<App::DocumentObject*>) override;

private:
    std::list<gp_Trsf> createTransformations(gp_Pnt& axbase, gp_Dir& axdir) const;
};

}  // namespace PartDesign
