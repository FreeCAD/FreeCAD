/***************************************************************************
 *   Copyright (c) 2014 Jürgen Riegel <juergen.riegel@web.de>              *
 *   Copyright (c) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include <unordered_set>
#include "DocumentObject.h"
#include "GroupExtension.h"
#include "PropertyGeo.h"


namespace App
{

/**
 * @brief The base class for placeable group of DocumentObjects. It represents a local coordnate system
 *
 * This class is the FreeCAD way of representing local coordinate systems. It groups its children beneath
 * it and transforms them all with the GeoFeatureGroup placement. A few important properties:
 * - Every child that belongs to the CS must be in the Group property. Even if a sketch is part of a pad,
 *   it must be in the Group property of the same GeoFeatureGroup as pad. This also holds for normal
 *   GroupExtensions. They can be added to a GeoFeatureGroup, but all objects that the group holds must
 *   also be added to the GeoFeatureGroup
 * - Objects can be only in a single GeoFeatureGroup. It is not allowed to have a document object in
 *   multiple GeoFeatureGroups
 * - PropertyLinks between different GeoFeatureGroups are forbidden. There are special link properties
 *   that allow such cross-CS links.
 * - Expressions can cross GeoFeatureGroup borders
 */
class AppExport GeoFeatureGroupExtension : public App::GroupExtension
{
    using inherited = App::GroupExtension;
    EXTENSION_PROPERTY_HEADER_WITH_OVERRIDE(App::GeoFeatureGroupExtension);

public:
    PropertyPlacement& placement();

    void initExtension(ExtensionContainer* obj) override;

    /**
     * @brief transformPlacement applies transform to placement of this shape.
     * Override this function to propagate the change of placement to base
     * features.
     * @param transform (input).
     */
    virtual void transformPlacement(const Base::Placement &transform);

    /// Constructor
    GeoFeatureGroupExtension();
    ~GeoFeatureGroupExtension() override;

    void extensionOnChanged(const Property* p) override;

    /** Returns the geo feature group which contains this object.
     * In case this object is not part of any geoFeatureGroup 0 is returned.
     * Unlike DocumentObjectGroup::getGroupOfObject searches only for GeoFeatureGroups
     * @param obj the object to search for
     */
    static DocumentObject* getGroupOfObject(const DocumentObject* obj);

    /**
     * @brief Calculates the global placement of this group
     *
     * The returned placement describes the transformation from the global reference coordinate
     * system to the local coordinate system of this geo feature group. If this group has a no parent
     * GeoFeatureGroup the returned placement is the one of this group. For multiple stacked
     * GeoFeatureGroups the returned Placement is the combination of all parent placements including
     * the one of this group.
     * @return Base::Placement The transformation from global reference system to the groups local system
     */
    Base::Placement globalGroupPlacement();

    /// Returns true if the given DocumentObject is DocumentObjectGroup but not GeoFeatureGroup
    static bool isNonGeoGroup(const DocumentObject* obj) {
        return obj->hasExtension(GroupExtension::getExtensionClassTypeId()) &&
               !obj->hasExtension(GeoFeatureGroupExtension::getExtensionClassTypeId());
    }

    bool extensionGetSubObject(DocumentObject *&ret, const char *subname, PyObject **pyObj,
            Base::Matrix4D *mat, bool transform, int depth) const override;

    bool extensionGetSubObjects(std::vector<std::string> &ret, int reason) const override;

    std::vector< DocumentObject* > addObjects(std::vector< DocumentObject* > obj) override;
    std::vector< DocumentObject* > removeObjects(std::vector< DocumentObject* > obj) override;

    /// Collects all links that are relevant for the coordinate system, meaning all recursive links to
    /// obj and from obj excluding expressions and stopping the recursion at other geofeaturegroups.
    /// The result is the combination of CSOutList and CSInList.
    static std::vector<App::DocumentObject*> getCSRelevantLinks(const App::DocumentObject* obj);
    /// Checks if the links of the given object comply with all GeoFeatureGroup requirements, that means
    /// if normal links are only within the parent GeoFeatureGroup.
    static bool areLinksValid(const App::DocumentObject* obj);
    /// Checks if the given link complies with all GeoFeatureGroup requirements, that means
    /// if normal links are only within the parent GeoFeatureGroup.
    static bool isLinkValid(App::Property* link);
    //Returns all objects that are wrongly linked from this object, meaning which are out of scope of the
    //links of obj
    static void getInvalidLinkObjects(const App::DocumentObject* obj, std::vector<App::DocumentObject*>& vec);

private:
    Base::Placement recursiveGroupPlacement(GeoFeatureGroupExtension* group, std::unordered_set<GeoFeatureGroupExtension*>& history);
    static std::vector<App::DocumentObject*> getScopedObjectsFromLinks(const App::DocumentObject*, LinkScope scope = LinkScope::Local);
    static std::vector<App::DocumentObject*> getScopedObjectsFromLink(App::Property*, LinkScope scope = LinkScope::Local);

    /// Collects GeoFeatureGroup relevant objects that are linked from the given one. That means all linked objects
    /// except GeoFeatureGroups. Expressions links are ignored. Only local scope links are considered. There is no
    /// recursion. An exception is thrown when there are dependency loops.
    static void getCSOutList(const App::DocumentObject* obj, std::vector<App::DocumentObject*>& vec);
    /// Collects GeoFeatureGroup relevant objects that link to the given one. That means all objects
    /// except GeoFeatureGroups. Expression links are ignored. Only local scope links are relevant, and
    /// there is no recursion. An exception is thrown when there are dependency loops.
    static void getCSInList(const App::DocumentObject* obj, std::vector<App::DocumentObject*>& vec);

    static void recursiveCSRelevantLinks(const App::DocumentObject* obj,
                                         std::vector<App::DocumentObject*>& vec);

};

using GeoFeatureGroupExtensionPython = ExtensionPythonT<GroupExtensionPythonT<GeoFeatureGroupExtension>>;


} //namespace App


#endif // APP_GeoFeatureGroup_H
