/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2014    *
 *   Copyright (c) Alexander Golubev (Fat-Zer) <fatzer2@gmail.com> 2015    *
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


#ifndef APP_GeoFeatureGroup_H
#define APP_GeoFeatureGroup_H

#include <App/FeaturePython.h>

#include "DocumentObjectGroup.h"
#include "PropertyGeo.h"

namespace App
{

/**
 * The base class for placeable group of DocumentObjects
 */
class AppExport GeoFeatureGroup : public App::DocumentObjectGroup
{
    PROPERTY_HEADER(App::GeoFeatureGroup);

public:
    PropertyPlacement Placement;

    /**
     * @brief transformPlacement applies transform to placement of this shape.
     * Override this function to propagate the change of placement to base
     * features, for example. By the time of writing this comment, the function
     * was only called by alignment task (Edit->Alignment)
     * @param transform (input).
     */
    virtual void transformPlacement(const Base::Placement &transform);
    /// Constructor
    GeoFeatureGroup(void);
    virtual ~GeoFeatureGroup();

    /** Returns the geo feature group which contains this object.
     * In case this object is not part of any geoFeatureGroup 0 is returned.
     * Unlike DocumentObjectGroup::getGroupOfObject serches only for GeoFeatureGroups
     */
    static GeoFeatureGroup* getGroupOfObject(const DocumentObject* obj);

    /// returns the type name of the ViewProvider
    virtual const char* getViewProviderName(void) const {
        return "Gui::ViewProviderGeoFeatureGroup";
    }

    virtual PyObject *getPyObject(void);
};

typedef App::FeaturePythonT<GeoFeatureGroup> GeoFeatureGroupPython;

} //namespace App


#endif // APP_GeoFeatureGroup_H
