// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>               *
 *   Copyright (c) 2022 Zheng, Lei <realthunder.dev@gmail.com>              *
 *   Copyright (c) 2023 FreeCAD Project Association                         *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/


#ifndef APP_COMPLEX_GEO_DATA_H
#define APP_COMPLEX_GEO_DATA_H

#include <algorithm>
#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include "MappedName.h"
#include "MappedElement.h"
#include "ElementMap.h"
#include "StringHasher.h"

#ifdef __GNUC__
# include <cstdint>
#endif


namespace Base
{
class Placement;
class Rotation;
template <class _Precision> class BoundBox3;// NOLINT
using BoundBox3d = BoundBox3<double>;
}

namespace Data
{

struct MappedChildElements;

/** Segments
 *  Sub-element type of the ComplexGeoData type
 *  It is used to split an object in further sub-parts.
 */
class AppExport Segment: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();// NOLINT

public:
    ~Segment() override = default;
    virtual std::string getName() const=0;
};


/** ComplexGeoData Object
 */
class AppExport ComplexGeoData: public Base::Persistence, public Base::Handled
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();// NOLINT

public:
    struct Line  {uint32_t I1; uint32_t I2;};
    struct Facet {uint32_t I1; uint32_t I2; uint32_t I3;};
    struct Domain {
        std::vector<Base::Vector3d> points;
        std::vector<Facet> facets;
    };

    /// Constructor
    ComplexGeoData();
    /// Destructor
    ~ComplexGeoData() override = default;

    /** @name Sub-element management */
    //@{
    /** Sub type list
     *  List of different sub-element types
     *  its NOT a list of the sub-elements itself
     */
    virtual std::vector<const char*> getElementTypes() const=0;
    virtual unsigned long countSubElements(const char* Type) const=0;
    /// Returns a generic element type and index. The determined element type isn't
    /// necessarily supported by this geometry.
    static std::pair<std::string, unsigned long> getTypeAndIndex(const char* Name);
    /// get the sub-element by type and number
    virtual Segment* getSubElement(const char* Type, unsigned long) const=0;
    /// get sub-element by combined name
    virtual Segment* getSubElementByName(const char* Name) const;
    /** Get lines from segment */
    virtual void getLinesFromSubElement(
        const Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Line> &lines) const;
    /** Get faces from segment */
    virtual void getFacesFromSubElement(
        const Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &PointNormals,
        std::vector<Facet> &faces) const;
    //@}

    /** @name Placement control */
    //@{
    /** Applies an additional transformation to the current transformation. */
    void applyTransform(const Base::Matrix4D& rclTrf);
    /** Applies an additional translation to the current transformation. */
    void applyTranslation(const Base::Vector3d&);
    /** Applies an additional rotation to the current transformation. */
    void applyRotation(const Base::Rotation&);
    /** Override the current transformation with a placement
     * using the setTransform() method.
     */
    void setPlacement(const Base::Placement& rclPlacement);
    /** Return the current transformation as placement using
     * getTransform().
     */
    Base::Placement getPlacement() const;
    /** Override the current transformation with the new one.
     * This method has to be handled by the child classes.
     * the actual placement and matrix is not part of this class.
     */
    virtual void setTransform(const Base::Matrix4D& rclTrf)=0;
    /** Return the current matrix
     * This method has to be handled by the child classes.
     * the actual placement and matrix is not part of this class.
     */
    virtual Base::Matrix4D getTransform() const = 0;
    //@}

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D &rclMat) = 0;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /// Get the standard accuracy to be used with getPoints, getLines or getFaces
    virtual double getAccuracy() const;
    /// Get the bound box
    virtual Base::BoundBox3d getBoundBox() const=0;
    /** Get point from line object intersection  */
    virtual Base::Vector3d getPointFromLineIntersection(
        const Base::Vector3f& base,
        const Base::Vector3f& dir) const;
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        double Accuracy, uint16_t flags=0) const;
    /** Get lines from object with given accuracy */
    virtual void getLines(std::vector<Base::Vector3d> &Points,std::vector<Line> &lines,
        double Accuracy, uint16_t flags=0) const;
    /** Get faces from object with given accuracy */
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        double Accuracy, uint16_t flags=0) const;
    /** Get the center of gravity
     * If this method is implemented then true is returned and the center of gravity.
     * The default implementation only returns false.
     */
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;
    //@}


    /** @name Element name mapping */
    //@{

    /** Get element indexed name
     *
     * @param name: the input name
     * @param sid: optional output of and App::StringID involved forming this mapped name
     *
     * @return Returns an indexed name.
     */
    IndexedName getIndexedName(const MappedName & name,
                               ElementIDRefs *sid = nullptr) const;

    /** Get element mapped name
     *
     * @param name: the input name
     * @param allowUnmapped: If the queried element is not mapped, then return
     *                       an empty name if \c allowUnmapped is false, or
     *                       else, return the indexed name.
     * @param sid: optional output of and App::StringID involved forming this mapped name
     * @return Returns the mapped name.
     */
    MappedName getMappedName(const IndexedName & element,
                             bool allowUnmapped = false,
                             ElementIDRefs *sid = nullptr) const;

    /** Return a pair of indexed name and mapped name
     *
     * @param name: the input name.
     * @param sid: optional output of any App::StringID involved in forming
     *             this mapped name
     * @param copy: if true, copy the name string, or else use it as constant
     *              string, and caller must make sure the memory is not freed.
     *
     * @return Returns the MappedElement which contains both the indexed and
     * mapped name.
     *
     * This function guesses whether the input name is an indexed name or
     * mapped, and perform a lookup and return the names found. If the input
     * name contains only alphabets and underscore followed by optional digits,
     * it will be treated as indexed name. Or else, it will be treated as
     * mapped name.
     */
    MappedElement getElementName(const char * name,
                                 ElementIDRefs *sid = nullptr,
                                 bool copy = false) const;

    /** Get mapped element names
     *
     * @param element: original element name with \c Type + \c Index
     * @param needUnmapped: if true, return the original element name if no
     * mapping is found
     *
     * @return a list of mapped names of the give element along with their
     * associated string ID references
     */
    std::vector<std::pair<MappedName, ElementIDRefs> >
    getElementMappedNames(const IndexedName & element, bool needUnmapped=false) const;

    /// Append the Tag (if and only if it is non zero) into the element map
    virtual void reTagElementMap(long tag,
                                 App::StringHasherRef hasher,
                                 const char *postfix=nullptr) {
        (void)tag;
        (void)hasher;
        (void)postfix;
    }

    // NOTE: getElementHistory is now in ElementMap

    char elementType(const Data::MappedName &) const;
    char elementType(const Data::IndexedName &) const;
    char elementType(const char *name) const;

    /** Reset/swap the element map
     *
     * @param elementMap: optional new element map
     *
     * @return Returns the existing element map.
     */
    virtual ElementMapPtr resetElementMap(ElementMapPtr elementMap=ElementMapPtr()) {
        _elementMap.swap(elementMap);
        return elementMap;
    }

    /// Get the entire element map
    std::vector<MappedElement> getElementMap() const;

    /// Set the entire element map
    void setElementMap(const std::vector<MappedElement> &elements);

    /// Get the current element map size
    size_t getElementMapSize(bool flush=true) const;

    /// Check if the given sub-name only contains an element name
    static bool isElementName(const char *subName) {
        return (subName != nullptr) && (*subName != 0) && findElementName(subName)==subName;
    }

    /** Flush an internal buffering for element mapping */
    virtual void flushElementMap() const;
    //@}

protected:

    /// from local to outside
    inline Base::Vector3d transformPointToOutside(const Base::Vector3f& vec) const
    {
        return getTransform() * Base::Vector3d(static_cast<double>(vec.x),
                                               static_cast<double>(vec.y),
                                               static_cast<double>(vec.z));
    }
    /// from local to outside
    template<typename Vec>
    inline std::vector<Base::Vector3d> transformPointsToOutside(const std::vector<Vec>& input) const
    {
        std::vector<Base::Vector3d> output;
        output.reserve(input.size());
        Base::Matrix4D mat(getTransform());
        std::transform(input.cbegin(), input.cend(), std::back_inserter(output), [&mat](const Vec& vec) {
            return mat * Base::Vector3d(static_cast<double>(vec.x),
                                        static_cast<double>(vec.y),
                                        static_cast<double>(vec.z));
        });

        return output;
    }
    inline Base::Vector3d transformVectorToOutside(const Base::Vector3f& vec) const
    {
        Base::Matrix4D mat(getTransform());
        mat.setCol(3, Base::Vector3d());
        return mat * Base::Vector3d(static_cast<double>(vec.x),
                                    static_cast<double>(vec.y),
                                    static_cast<double>(vec.z));
    }
    template<typename Vec>
    std::vector<Base::Vector3d> transformVectorsToOutside(const std::vector<Vec>& input) const
    {
        std::vector<Base::Vector3d> output;
        output.reserve(input.size());
        Base::Matrix4D mat(getTransform());
        mat.setCol(3, Base::Vector3d());
        std::transform(input.cbegin(), input.cend(), std::back_inserter(output), [&mat](const Vec& vec) {
            return mat * Base::Vector3d(static_cast<double>(vec.x),
                                        static_cast<double>(vec.y),
                                        static_cast<double>(vec.z));
        });

        return output;
    }
    /// from local to inside
    inline Base::Vector3f transformPointToInside(const Base::Vector3d& vec) const
    {
        Base::Matrix4D tmpM(getTransform());
        tmpM.inverse();
        Base::Vector3d tmp = tmpM * vec;
        return Base::Vector3f(static_cast<float>(tmp.x),
                              static_cast<float>(tmp.y),
                              static_cast<float>(tmp.z));
    }
public:
    mutable long Tag{0};


public:
    /// String hasher for element name shortening
    mutable App::StringHasherRef Hasher;

protected:

    /// from local to outside
    inline Base::Vector3d transformToOutside(const Base::Vector3f& vec) const
    {
        return getTransform() * Base::Vector3d(static_cast<double>(vec.x),
                                               static_cast<double>(vec.y),
                                               static_cast<double>(vec.z));
    }
    /// from local to inside
    inline Base::Vector3f transformToInside(const Base::Vector3d& vec) const
    {
        Base::Matrix4D tmpM(getTransform());
        tmpM.inverse();
        Base::Vector3d tmp = tmpM * vec;
        return Base::Vector3f(static_cast<float>(tmp.x),
                              static_cast<float>(tmp.y),
                              static_cast<float>(tmp.z));
    }

protected:
    ElementMapPtr elementMap(bool flush=true) const;

private:
    ElementMapPtr _elementMap;
};

} //namespace App


#endif
