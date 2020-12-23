/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2012     *
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

#ifndef ORIGINFEATURE_H_6ZWJPB5V
#define ORIGINFEATURE_H_6ZWJPB5V

#include "GeoFeature.h"

namespace App
{

class Origin;

/** Plane Object
 *  Used to define planar support for all kind of operations in the document space
 */
class AppExport OriginFeature: public App::GeoFeature
{
    PROPERTY_HEADER(App::OriginFeature);
public:
    /// additional information about the feature usage (e.g. "BasePlane-XY" or "Axis-X" in a Origin)
    PropertyString Role;

    /// Constructor
    OriginFeature(void);
    virtual ~OriginFeature();

    /// Finds the origin object this plane belongs to
    App::Origin *getOrigin ();
};

class AppExport Plane: public App::OriginFeature {
    PROPERTY_HEADER(App::OriginFeature);
public:
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderPlane";
    }
};

class AppExport Line: public App::OriginFeature {
    PROPERTY_HEADER(App::OriginFeature);
public:
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderLine";
    }
};

} //namespace App

#endif /* end of include guard: ORIGINFEATURE_H_6ZWJPB5V */
