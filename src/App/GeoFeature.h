/***************************************************************************
 *   Copyright (c) Juergen Riegel          (juergen.riegel@web.de) 2002    *
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
    PropertyString _ElementMapVersion;

    /// Constructor
    GeoFeature(void);
    virtual ~GeoFeature();

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
    virtual PyObject* getPyObject(void);

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
     * NOTE: Unlike ComplexGeoData::getElementName(), this function
     * relies on ComplexGeoData::elementMapPrefix() to decide whether
     * it is a forward query, i.e. mapped -> original, or reverse query.
     * The reason being that, unlike ComplexGeoData who deals with the
     * actual element map data, GeoFeature here sits at a higher level.
     * GeoFeature should be dealing with whatever various PropertyLinkSub(s)
     * is assigned.
     *
     * This function is made virtual, so that inherited class can do something
     * unusual, such as Sketcher::SketcherObject, which uses this to expose its
     * private geometries without a correpsonding TopoShape, and yet being
     * source code compatible.
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
            const DocumentObject *filter=0,const char **element=0, GeoFeature **geo=0);

    static bool hasMissingElement(const char *subname);

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

protected:
    virtual void onChanged(const Property* prop);
    virtual void onDocumentRestored();
    void updateElementReference();

private:
    std::map<std::string,std::string> _elementMapCache;
    std::string _elementMapVersion;
};

} //namespace App


#endif // APP_GEOFEATURE_H
