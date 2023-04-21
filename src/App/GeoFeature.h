/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
    PROPERTY_HEADER_WITH_OVERRIDE(App::GeoFeature);

public:
    PropertyPlacement Placement;

    /// Constructor
    GeoFeature();
    ~GeoFeature() override;

    /**
     * @brief transformPlacement applies transform to placement of this shape.
     * Override this function to propagate the change of placement to base
     * features, for example. By the time of writing this comment, the function
     * was only called by alignment task (Edit->Alignment)
     * @param transform (input).
     */
    virtual void transformPlacement(const Base::Placement &transform);
    /**
     * This method returns the main property of a geometric object that holds
     * the actual geometry. For a part object this is the Shape property, for
     * a mesh object the Mesh property and so on.
     * The default implementation returns null.
     */
    virtual const PropertyComplexGeoData* getPropertyOfGeometry() const;
    /**
     * @brief getPyObject returns the Python binding object
     * @return the Python binding object
     */
    PyObject* getPyObject() override;

    /// Specify the type of element name to return when calling getElementName() 
    enum ElementNameType {
        /// Normal usage
        Normal=0,
        /// For importing
        Import=1,
        /// For exporting
        Export=2,
    };
    /** Return the new and old style sub-element name
     *
     * @param name: input name
     * @param type: desired element name type to return
     *
     * @return a pair(newName,oldName). New element name may be empty.
     *
     * This function currently is does nothing. The new style element name
     * generation will be added in the next batch of patches.
     */
    virtual std::pair<std::string,std::string> getElementName(
            const char *name, ElementNameType type=Normal) const;

    /** Resolve both the new and old style element name
     *
     * @param obj: top parent object
     * @param subname: subname reference 
     * @param elementName: output of a pair(newElementName,oldElementName)
     * @param append: Whether to include subname prefix into the returned
     *                element name
     * @param type: the type of element name to request
     * @param filter: If none zero, then only perform lookup when the element
     *                owner object is the same as this filter
     * @param element: return the start of element name in subname
     *
     * @return Return the owner object of the element
     */
    static DocumentObject *resolveElement(App::DocumentObject *obj, 
            const char *subname, std::pair<std::string,std::string> &elementName, 
            bool append=false, ElementNameType type=Normal,
            const DocumentObject *filter=nullptr,const char **element=nullptr, GeoFeature **geo=nullptr);

    /**
     * @brief Calculates the placement in the global reference coordinate system
     * 
     * In FreeCAD the GeoFeature placement describes the local placement of the object in its parent
     * coordinate system. This is however not always the same as the global reference system. If the
     * object is in a GeoFeatureGroup, hence in another local coordinate system, the Placement
     * property does only give the local transformation. This function can be used to calculate the 
     * placement of the object in the global reference coordinate system taking all stacked local 
     * systems into account.
     * 
     * @return Base::Placement The transformation from the global reference coordinate system
     */
    Base::Placement globalPlacement() const;
};

} //namespace App


#endif // APP_GEOFEATURE_H
