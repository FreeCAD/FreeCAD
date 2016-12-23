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
class AppExport GeoFeatureGroupExtension : public App::GroupExtension
{
    EXTENSION_PROPERTY_HEADER(App::GeoFeatureGroupExtension);

public:
    PropertyPlacement& placement();
    
    virtual void initExtension(ExtensionContainer* obj);

    /**
     * @brief transformPlacement applies transform to placement of this shape.
     * Override this function to propagate the change of placement to base
     * features.
     * @param transform (input).
     */
    virtual void transformPlacement(const Base::Placement &transform);
    /// Constructor
    GeoFeatureGroupExtension(void);
    virtual ~GeoFeatureGroupExtension();

    /// Returns all geometrically controlled objects: all objects of this group and it's non-geo subgroups
    std::vector<App::DocumentObject*> getGeoSubObjects () const;

    /// Returns true if either the group or one of it's non-geo subgroups has the object
    bool geoHasObject (const DocumentObject* obj) const;

    /** Returns the geo feature group which contains this object.
     * In case this object is not part of any geoFeatureGroup 0 is returned.
     * Unlike DocumentObjectGroup::getGroupOfObject serches only for GeoFeatureGroups
     * @param obj       the object to search for
     * @param indirect  if true return if the group that so-called geoHas the object, @see geoHasObject()
     *                  default is true
     */
    static DocumentObject* getGroupOfObject(const DocumentObject* obj, bool indirect=true);

    /// Returns true if the given DocumentObject is DocumentObjectGroup but not GeoFeatureGroup
    static bool isNonGeoGroup(const DocumentObject* obj) {
        return obj->hasExtension(GroupExtension::getExtensionClassTypeId()) & 
               !obj->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId());
    }
};

typedef ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>> GeoFeatureGroupExtensionPython;


} //namespace App


#endif // APP_GeoFeatureGroup_H
