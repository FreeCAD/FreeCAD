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

#ifndef APP_PROPERTYCOMPLEXGEODATA_H
#define APP_PROPERTYCOMPLEXGEODATA_H

#include <string>

#include <App/Property.h>


namespace Base {
class Matrix4D;
template<class _Precision>
class BoundBox3;
using BoundBox3d = BoundBox3<double>;
}

namespace Data {
class ComplexGeoData;
}

namespace App {

/** The base class for all basic geometry properties.
 * @author Werner Mayer
 */
class AppExport PropertyGeometry: public App::Property
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyGeometry();
    ~PropertyGeometry() override;

    /** @name Modification */
    //@{
    /// Set the placement of the geometry
    virtual void setTransform(const Base::Matrix4D& rclTrf) = 0;
    /// Get the placement of the geometry
    virtual Base::Matrix4D getTransform() const = 0;
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D& rclMat) = 0;
    /// Retrieve bounding box information
    virtual Base::BoundBox3d getBoundingBox() const = 0;
    //@}
}; 

/** The base class for all complex data properties.
 * @author Werner Mayer
 */
class AppExport PropertyComplexGeoData: public App::PropertyGeometry
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyComplexGeoData();
    ~PropertyComplexGeoData() override;

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    void transformGeometry(const Base::Matrix4D& rclMat) override = 0;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    virtual const Data::ComplexGeoData* getComplexData() const = 0;
    Base::BoundBox3d getBoundingBox() const override = 0;
    //@}

    /** Return the element map version
     *
     * @param persisted: if true, return the restored element map version. Or
     * else, return the current element map version
     */
    virtual std::string getElementMapVersion(bool restored = false) const;

    /// Return true to signal element map version change
    virtual bool checkElementMapVersion(const char* ver) const;

    void afterRestore() override;
};

}  // namespace App

#endif  // APP_PROPERTYGEO_H
