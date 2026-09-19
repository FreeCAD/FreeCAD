// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026 FreeCAD Project Association                         *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Lesser General Public             *
 *   License as published by the Free Software Foundation; either           *
 *   version 2.1 of the License, or (at your option) any later version.     *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA 02111-1307, USA                                  *
 *                                                                         *
 ***************************************************************************/

#pragma once

#include "LinkArray.h"
#include "PolarPatternExtension.h"

namespace Part
{

class PartExport LinkArrayPolar: public Part::LinkArray, public Part::PolarPatternExtension
{
    PROPERTY_HEADER_WITH_EXTENSIONS(Part::LinkArrayPolar);
    using inherited = Part::LinkArray;

public:
    LinkArrayPolar();

    gp_Ax2 getRotation() const override;
    void onDocumentRestored() override;

protected:
    std::vector<Base::Placement> getElementPlacements() override;

private:
    void setAxisLinkScope();
    void transformAxisToLocal(gp_Ax2& axis) const;
};

}  // namespace Part
