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


#ifndef APP_GEOFEATURE_H
#define APP_GEOFEATURE_H

#include "DocumentObject.h"
#include "PropertyGeo.h"



namespace App
{


/** Base class of all geometric document objects.
 */
class AppExport GeoFeature : public App::DocumentObject
{
    PROPERTY_HEADER(App::GeoFeature);

public:
    PropertyPlacement Placement;

    /// Constructor
    GeoFeature(void);
    virtual ~GeoFeature();

    virtual void transformPlacement(const Base::Placement &transform);
};

} //namespace App


#endif // APP_GEOFEATURE_H
