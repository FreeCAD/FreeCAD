/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
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


#ifndef Fem_FemPostObject_H
#define Fem_FemPostObject_H

#include "PropertyPostDataObject.h"
#include <App/GeoFeature.h>
#include <vtkBoundingBox.h>


namespace Fem
{

// poly data is the only data we can visualize, hence every post
// processing object needs to expose it
class FemExport FemPostObject: public App::GeoFeature
{
    PROPERTY_HEADER_WITH_OVERRIDE(Fem::FemPostObject);

public:
    /// Constructor
    FemPostObject();
    ~FemPostObject() override;

    Fem::PropertyPostDataObject Data;

    vtkBoundingBox getBoundingBox();
};

}  // namespace Fem


#endif  // Fem_FemPostObject_H
