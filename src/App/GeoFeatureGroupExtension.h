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
 * @brief The base class for placeable group of DocumentObjects. It represents a local coordnate system
 * 
 * This class is the FreeCAD way of representing local coordinate systems. It groups its childs beneath 
 * it and transforms them all with the GeoFeatureGroup placement. A few important properties:
 * - Every child that belongs to the CS must be in the Group proeprty. Even if a sketch is part of a pad,
 *   it must be in the Group property of the same GeoFeatureGroup as pad. This also holds for normal 
 *   GroupExtensions. They can be added to a GeoFeatureGroup, but all objects that the group holds must 
 *   also be added to the GeoFeatureGroup
 * - Objects can be only in a single GeoFeatureGroup. It is not allowed to have a document object in 
 *   multiple GeoFeatureGroups
 * - PropertyLinks between different GeoFeatureGroups are forbidden. There are special link proeprties 
 *   that allow such cross-CS links.
 * - Expressions can cross GeoFeatureGroup borders
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

    /** Returns the geo feature group which contains this object.
     * In case this object is not part of any geoFeatureGroup 0 is returned.
     * Unlike DocumentObjectGroup::getGroupOfObject serches only for GeoFeatureGroups
     * @param obj       the object to search for
     * @param indirect  if true return if the group that so-called geoHas the object, @see geoHasObject()
     *                  default is true
     */
    static DocumentObject* getGroupOfObject(const DocumentObject* obj);
    
    /**
     * @brief Calculates the global placement of this group
     * 
     * The returned placement describes the transformation from the global reference coordinate 
     * system to the local coordinate system of this geo feature group. If this group has a no parent
     * GeoFeatureGroup the returned placement is the one of this group. For multiple stacked 
     * GeoFeatureGroups the returned Placement is the combination of all parent placements including 
     * ths one of this group.
     * @return Base::Placement The transformation from global reference system to the groups local system
     */
    Base::Placement globalGroupPlacement();

    /// Returns true if the given DocumentObject is DocumentObjectGroup but not GeoFeatureGroup
    static bool isNonGeoGroup(const DocumentObject* obj) {
        return obj->hasExtension(GroupExtension::getExtensionClassTypeId()) && 
               !obj->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId());
    }
    
    virtual std::vector< DocumentObject* > addObjects(std::vector< DocumentObject* > obj) override;
    virtual std::vector< DocumentObject* > removeObjects(std::vector< DocumentObject* > obj) override;
    
    /// returns GeoFeatureGroup relevant objects that are linked from the given one. That meas all linked objects
    /// including their linkes (recursively) except GeoFeatureGroups, where the recursion stops. Expressions 
    /// links are ignored.
    static std::vector<App::DocumentObject*> getCSOutList(App::DocumentObject* obj);
    ///returns GeoFeatureGroup relevant objects that link to the given one. That meas all objects 
    /// including their parents (recursively) except GeoFeatureGroups, where the recursion stops. Expression 
    /// links are ignored
    static std::vector<App::DocumentObject*> getCSInList(App::DocumentObject* obj);
    /// Returns all links that are relevant for the coordinate system, meaning all recursive links to 
    /// obj and from obj excluding expressions and stopping the recursion at other geofeaturegroups. 
    /// The result is the combination of CSOutList and CSInList.
    static std::vector<App::DocumentObject*> getCSRelevantLinks(App::DocumentObject* obj);
    
private:
    Base::Placement recursiveGroupPlacement(GeoFeatureGroupExtension* group);
    static std::vector<App::DocumentObject*> getObjectsFromLinks(App::DocumentObject*);
};

typedef ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>> GeoFeatureGroupExtensionPython;


} //namespace App


#endif // APP_GeoFeatureGroup_H
