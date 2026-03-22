// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "DocumentObject.h"
#include "PropertyGeo.h"
#include "MappedElement.h"
#include "Material.h"
#include "ComplexGeoData.h"

namespace App
{


/** Base class of all geometric document objects.
 */
class AppExport GeoFeature: public App::DocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(App::GeoFeature);

public:
    PropertyPlacement Placement;
    PropertyString _ElementMapVersion;

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
    virtual void transformPlacement(const Base::Placement& transform);
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
    enum ElementNameType
    {
        /// Normal usage
        Normal = 0,
        /// For importing
        Import = 1,
        /// For exporting
        Export = 2,
    };

    /** Return the new and old style sub-element name
     *
     * @param name: input name
     * @param type: desired element name type to return
     *
     * @return a struct with the newName and oldName. New element name may be empty.
     */
    virtual ElementNamePair getElementName(  // NOLINT(google-default-arguments)
        const char* name,
        ElementNameType type = Normal) const;

    /**
     * @brief Resolve both the new and old style element name.
     *
     * @param[in] obj The top parent object
     * @param[in] subname The subname reference.
     * @param[out] elementName Output of a pair(newElementName,oldElementName)
     * @param[in] append: Whether to include the subname prefix into the
     * returned element name.
     * @param[in] type The type of element name to request.
     * @param[in] filter If not `nullptr`, then only perform lookup when the
     * element owner object is the same as this filter.
     * @param[out] element If not `nullptr`, provide start of element name in
     * subname.
     * @param[out] geoFeature If not `nullptr`, provide the GeoFeature that
     * contains the element.
     *
     * @return Return the owner object of the element.
     */
    static DocumentObject* resolveElement(const App::DocumentObject* obj,
                                          const char* subname,
                                          ElementNamePair& elementName,
                                          bool append = false,
                                          ElementNameType type = Normal,
                                          const DocumentObject* filter = nullptr,
                                          const char** element = nullptr,
                                          GeoFeature** geoFeature = nullptr);

    /**
     * @brief Deprecated. Calculates the placement in the global reference coordinate system
     *
     * Deprecated: This does not handle App::Links correctly. Use getGlobalPlacement() instead.
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
    /**
     * @brief Virtual function to get an App::Material object describing the appearance
     *
     * The appearance properties are described by the underlying features material. This can not
     * be accessed directly from within the Gui module. This virtual function will return a
     * App::Material object describing the appearance properties of the material.
     *
     * @return App::Material the appearance properties of the object material
     */
    virtual App::Material getMaterialAppearance() const;

    /**
     * @brief Virtual function to set the appearance with an App::Material object
     *
     * The appearance properties are described by the underlying features material. This cannot
     * be accessed directly from within the Gui module. This virtual function will set the
     * appearance from an App::Material object.
     */
    virtual void setMaterialAppearance(const App::Material& material);

    /**
     * @brief Virtual function to get the camera alignment direction
     *
     * Finds a directionZ to align the camera with.
     * May also find an optional directionX which could be used for horizontal or vertical alignment.
     *
     * @return bool whether or not a directionZ is found.
     */
    virtual bool getCameraAlignmentDirection(Base::Vector3d& directionZ,
                                             Base::Vector3d& directionX,
                                             const char* subname = nullptr) const;
    /** Search sub element using internal cached geometry
     *
     * @param element: element name
     * @param options: search options
     * @param tol: coordinate tolerance
     * @param atol: angle tolerance
     *
     * @return Returns a list of found element reference to the new geometry.
     * The returned value will be invalidated when the geometry is changed.
     *
     * Before changing the property of geometry, GeoFeature will internally
     * make a snapshot of all referenced element geometry. After change, user
     * code may call this function to search for the new element name that
     * reference to the same geometry of the old element.
     */
    virtual const std::vector<std::string>&
    searchElementCache(const std::string& element,
                       Data::SearchOptions options = Data::SearchOption::CheckGeometry,
                       double tol = 1e-7,
                       double atol = 1e-10) const;

    static bool hasMissingElement(const char* subname);

    /// Return the object that owns the shape that contains the give element name
    virtual DocumentObject* getElementOwner(const Data::MappedName& /*name*/) const
    {
        return nullptr;
    }

    virtual std::vector<const char*> getElementTypes(bool all = true) const;

    /// Return the higher level element names of the given element
    virtual std::vector<Data::IndexedName> getHigherElements(const char* name,
                                                             bool silent = false) const;

    static Base::Placement getPlacementFromProp(DocumentObject* obj, const char* propName);
    static Base::Placement
    getGlobalPlacement(DocumentObject* targetObj, DocumentObject* rootObj, const std::string& sub);
    static Base::Placement getGlobalPlacement(DocumentObject* targetObj, PropertyXLinkSub* prop);
    static Base::Placement getGlobalPlacement(const DocumentObject* obj);

protected:
    void onChanged(const Property* prop) override;
    void onDocumentRestored() override;
    void updateElementReference();

protected:
    ElementNamePair _getElementName(const char* name, const Data::MappedElement& mapped) const;
};

}  // namespace App
