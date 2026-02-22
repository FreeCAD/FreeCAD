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


#pragma once

#include <algorithm>
#include <optional>

#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include "MappedName.h"
#include "MappedElement.h"
#include "ElementMap.h"
#include "StringHasher.h"

#ifdef __GNUC__
#include <cstdint>
#endif


namespace Base
{
class Placement;
class Rotation;
template<class _Precision>
class BoundBox3;  // NOLINT
using BoundBox3d = BoundBox3<double>;
}  // namespace Base

namespace Data
{

// struct MappedChildElements;

/// Option for App::GeoFeature::searchElementCache()
enum class SearchOption
{
    CheckGeometry = 1, ///< Whether to compare shape geometry
    SingleResult = 2, ///< Stop at first found result
};

typedef Base::Flags<SearchOption> SearchOptions;

/**
 * @brief A class for segments.
 *
 *  A segment is a sub-element type of the ComplexGeoData type.  It is used to
 *  split an object in further sub-parts.
 */
class AppExport Segment: public Base::BaseClass
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    ~Segment() override = default;

    /// Get the name of the segment.
    virtual std::string getName() const = 0;
};

/**
 * @brief A class for complex geometric data.
 * @ingroup ElementMapping
 */
class AppExport ComplexGeoData: public Base::Persistence, public Base::Handled
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();  // NOLINT

public:
    /**
     * @brief A line as a facet boundary in a 3D mesh.
     *
     * The line is represented by two point indices.
     */
    struct Line
    {
        uint32_t I1;
        uint32_t I2;
    };

    /**
     * @brief A triangular facet in a 3D mesh.
     *
     * The facet is represented by three point indices.
     */
    struct Facet
    {
        uint32_t I1;
        uint32_t I2;
        uint32_t I3;
    };

    /**
     * @brief A domain in a 3D mesh.
     *
     * A domain consists of a list of points and a list of facets where the
     * indices of a facet index into the points list.
     */
    struct Domain
    {
        std::vector<Base::Vector3d> points;
        std::vector<Facet> facets;
    };

    ComplexGeoData();
    ~ComplexGeoData() override = default;

    /**
     * @name Sub-element management
     * @{
     */

    /**
     * @brief Get a list of sub-element types.
     *
     *  This list does not contain the sub-elements themselves.
     */
    virtual std::vector<const char*> getElementTypes() const = 0;

    /**
     * @brief Get the number of sub-elements of a given type.
     *
     * @param[in] Type The type of sub-element.
     * @return The number of sub-elements of the given type.
     */
    virtual unsigned long countSubElements(const char* Type) const = 0;

    /**
     * @brief Get the type and index from a combined name.
     *
     * An example is "Edge12", which would return the type "Edge" and the
     * index 12.
     *
     * @param[in] Name The combined name of the sub-element.
     * @returns A pair of a generic element type and index.
     *
     * @note The determined element type isn't necessarily supported by this
     * geometry.
     */
    static std::pair<std::string, unsigned long> getTypeAndIndex(const char* Name);

    /// Get the sub-element by type and number.
    virtual Segment* getSubElement(const char* Type, unsigned long) const = 0;

    /// Get sub-element by combined name
    virtual Segment* getSubElementByName(const char* Name) const;

    /**
     * @brief Get the lines from a segment.
     *
     * @param[in] segment The segment to get the lines from.
     *
     * @param[in,out] Points The list of points used by the lines.
     * @param[in,out] lines The list of lines retrieved from the segment.
     */
    virtual void getLinesFromSubElement(const Segment* segment,
                                        std::vector<Base::Vector3d>& Points,
                                        std::vector<Line>& lines) const;

    /**
     * @brief Get the faces from a segment.
     *
     * @param[in] segment The segment to get the faces from.
     * @param[in,out] Points The list of points used by the faces.
     * @param[in,out] PointNormals The list of point normals used by the faces.
     * @param[in,out] faces The list of faces retrieved from the segment.
     */
    virtual void getFacesFromSubElement(const Segment* segment,
                                        std::vector<Base::Vector3d>& Points,
                                        std::vector<Base::Vector3d>& PointNormals,
                                        std::vector<Facet>& faces) const;
    /// @}

    /**
     * @name Placement control
     *
     * @{
     */

    /// Apply an additional transformation to the current transformation.
    void applyTransform(const Base::Matrix4D& rclTrf);

    /// Apply an additional translation to the current transformation.
    void applyTranslation(const Base::Vector3d&);

    /// Applies an additional rotation to the current transformation.
    void applyRotation(const Base::Rotation&);

    /**
     * @brief Override the current transformation with a placement.
     *
     * Override the current transformation with a placement using the
     * setTransform() method.
     *
     * @param[in] rclPlacement The new placement to set.
     */
    void setPlacement(const Base::Placement& rclPlacement);

    /**
     * @brief Get the current transformation as placement.
     *
     * @return The current transformation as placement using getTransform().
     */
    Base::Placement getPlacement() const;

    /**
     * @brief Override the current transformation.
     *
     * Override the current transformation with the new one.  This method has
     * to be handled by the child classes.  the actual placement and matrix is
     * not part of this class.
     *
     * @param[in] rclTrf The new transformation matrix to set.
     */

    virtual void setTransform(const Base::Matrix4D& rclTrf) = 0;

    /**
     * @brief Get the current transformation matrix.
     *
     * This method has to be handled by the child classes.  The actual
     * placement and matrix is not part of this class.
     */
    virtual Base::Matrix4D getTransform() const = 0;
    /// @}

    /**
     *@name Modification
     *
     * @{
     */

    /**
     * @brief Applies a transformation on the real geometric data type.
     *
     * @param[in] rclMat The transformation matrix to apply.
     */
    virtual void transformGeometry(const Base::Matrix4D& rclMat) = 0;
    /// @}

    /**
     * @name Getting basic geometric entities
     *
     * @{
     */

    /**
     * @brief Get the standard accuracy for meshing.
     *
     * Get the standard accuracy to be used with getPoints(), getLines() or
     * getFaces().
     *
     * @return The standard accuracy.
     */
    virtual double getAccuracy() const;

    /// Get the bound box
    virtual Base::BoundBox3d getBoundBox() const = 0;

    /**
     * @brief Get point from line object intersection.
     *
     * @param[in] base The base point of the line.
     * @param[in] dir The direction of the line.
     *
     * @return The intersection point.
     */
    virtual Base::Vector3d getPointFromLineIntersection(const Base::Vector3f& base,
                                                        const Base::Vector3f& dir) const;

    /**
     * @brief Get points from object with given accuracy.
     *
     * @param[in,out] Points The list of points retrieved from the object.
     * @param[in,out] Normals The list of normals associated with faces
     * retrieved from the object.  If there are no faces, then this list will
     * be empty.
     * @param[in] Accuracy The accuracy to use when retrieving the points.
     * @param[in] flags Additional flags for point retrieval.
     */
    virtual void getPoints(std::vector<Base::Vector3d>& Points,
                           std::vector<Base::Vector3d>& Normals,
                           double Accuracy,
                           uint16_t flags = 0) const;

    /**
     * @brief Get lines from object with given accuracy
     *
     * @param[in,out] Points The list of points retrieved from the object.
     * @param[in,out] lines The list of lines retrieved from the object.
     * @param[in] Accuracy The accuracy to use when retrieving the lines.
     * @param[in] flags Additional flags for line retrieval.
     */
    virtual void getLines(std::vector<Base::Vector3d>& Points,
                          std::vector<Line>& lines,
                          double Accuracy,
                          uint16_t flags = 0) const;

    /**
     * @brief Get faces from object with given accuracy.
     *
     * @param[in,out] Points The list of points retrieved from the object.
     * @param[in,out] faces The list of faces retrieved from the object.
     * @param[in] Accuracy The accuracy to use when retrieving the faces.
     * @param[in] flags Additional flags for face retrieval.
     */
    virtual void getFaces(std::vector<Base::Vector3d>& Points,
                          std::vector<Facet>& faces,
                          double Accuracy,
                          uint16_t flags = 0) const;

    /**
     * @brief Get the center of gravity.
     *
     * @param[out] center The center of gravity.
     *
     * @return True if this method is implemented.  The default implementation returns false.
     */
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;

    /**
     * @brief Get the center of gravity.
     *
     * @return The center of gravity if available.
     */
    virtual std::optional<Base::Vector3d> centerOfGravity() const;
    /// @}

    /// Get the element map prefix.
    static const std::string& elementMapPrefix();

    /**
     * @name Element name mapping
     *
     * @{
     */

    /**
     * @brief Get the element's indexed name.
     *
     * @param[in] name The mapped name.
     *
     * @param[out] sid Optional output of and App::StringID involved forming
     * this mapped name.
     *
     * @return Returns an indexed name.
     */
    IndexedName getIndexedName(const MappedName& name, ElementIDRefs* sid = nullptr) const;

    /**
     * @brief Get the element's mapped name.
     *
     * @param[in] element The indexed name name of the element.
     * @param[in] allowUnmapped If the queried element is not mapped, then
     * return an empty name if @p allowUnmapped is false, or else, return the
     * indexed name.
     * @param sid Optional output of and App::StringID involved forming this
     * mapped name.
     *
     * @return The mapped name.
     */
    MappedName getMappedName(const IndexedName& element,
                             bool allowUnmapped = false,
                             ElementIDRefs* sid = nullptr) const;

    /**
     * @brief Get a pair of an indexed and mapped name.
     *
     * This function guesses whether the input name is an indexed name or
     * mapped one, performs a lookup, and returns the names found. If the input
     * name contains only alphabets and underscore followed by optional digits,
     * it will be treated as indexed name. Otherwise, it will be treated as a
     * mapped name.
     *
     * @param[in] name The input name.
     * @param[out] sid Optional output of any App::StringID involved in forming
     * this mapped name.
     * @param[in] copy If true, copy the name string, or else use it as
     * constant string, and caller must make sure the memory is not freed.
     *
     * @return The MappedElement that contains both the indexed and mapped
     * name.
     */
    MappedElement
    getElementName(const char* name, ElementIDRefs* sid = nullptr, bool copy = false) const;

    /**
     * @brief Add a sub-element name mapping.
     *
     * An element can have multiple mapped names. However, a name can only be
     * mapped to one element
     *
     * @param[in] element The original @c Type + @c Index element name.
     * @param[in] name The mapped sub-element name. It may or may not start
     * with elementMapPrefix().
     * @param[in] masterTag The master tag of the element.
     * @param[in] sid In case you use a hasher to hash the element name, pass
     * in the string id reference using this parameter. You can have more than
     * one string id associated with the same name.
     * @param[in] overwrite If true, it will overwrite existing names.
     *
     * @return The stored mapped element name.
     *
     * @note The original function was in the context of ComplexGeoData, which
     * provided `Tag` access, now you must pass in `long masterTag` explicitly.
     */
    MappedName setElementName(const IndexedName& element,
                              const MappedName& name,
                              long masterTag,
                              const ElementIDRefs* sid = nullptr,
                              bool overwrite = false)
    {
        return _elementMap->setElementName(element, name, masterTag, sid, overwrite);
    }

    /// Check if there is an element map.
    bool hasElementMap() const
    {
        return _elementMap != nullptr;
    }

    /**
     * @brief Get mapped element names.
     *
     * @param[in] element The original element name with @c Type + @c Index.
     * @param[in] needUnmapped If true, return the original element name if no
     * mapping is found.
     *
     * @return A list of mapped names of the give element along with their
     * associated string ID references.
     */
    std::vector<std::pair<MappedName, ElementIDRefs>>
    getElementMappedNames(const IndexedName& element, bool needUnmapped = false) const;

    /**
     * @brief Hash the child element map postfixes.
     *
     * The hashing is done to shorten element names from hierarchical maps.
     */
    void hashChildMaps();

    /// Check if there is child element map.
    bool hasChildElementMap() const;

    /**
     * @brief Append the Tag (if and only if it is non zero) into the element map.
     *
     * @param[in] tag The master tag to append.
     * @param[in] hasher The string hasher to use.
     * @param[in] postfix An optional postfix to append after the tag.
     */
    virtual void
    reTagElementMap(long tag, App::StringHasherRef hasher, const char* postfix = nullptr)
    {
        (void)tag;
        (void)hasher;
        (void)postfix;
    }

    /**
     * @brief Get the history of an element name.
     *
     * @param[in] name The mapped element name to query.
     * @param[out] original Optional output parameter to store the original
     * element name.
     * @param[out] history Optional output parameter to store the history of
     * element names.
     *
     * @note This function is now in ElementMap.
     */
    long getElementHistory(const MappedName& name,
                           MappedName* original = nullptr,
                           std::vector<MappedName>* history = nullptr) const
    {
        if (_elementMap != nullptr) {
            return _elementMap->getElementHistory(name, Tag, original, history);
        }
        return 0;
    };

    /// Set the mapped child elements.
    void setMappedChildElements(const std::vector<Data::ElementMap::MappedChildElements>& children);

    /// Get the mapped child elements.
    std::vector<Data::ElementMap::MappedChildElements> getMappedChildElements() const;

    /// Get the element type from a mapped name.
    char elementType(const Data::MappedName&) const;

    /// Get the element type from an indexed name.
    char elementType(const Data::IndexedName&) const;

    /// Get the element type from a raw name.
    char elementType(const char* name) const;

    /**
     * @brief Reset/swap the element map.
     *
     * @param[in] elementMap: optional new element map.
     *
     * @return The existing element map.
     */
    virtual ElementMapPtr resetElementMap(ElementMapPtr elementMap = ElementMapPtr());

    /// Get the entire element map.
    std::vector<MappedElement> getElementMap() const;

    /// Set the entire element map.
    void setElementMap(const std::vector<MappedElement>& elements);

    /// Get the current element map size.
    size_t getElementMapSize(bool flush = true) const;

    /**
     * @brief Get the higher level element names of the given element.
     *
     * @param name: the input element name.
     * @param silent: if true, suppress throwing exceptions.
     */
    virtual std::vector<IndexedName> getHigherElements(const char* name, bool silent = false) const;

    /// Get the current element map version.
    virtual std::string getElementMapVersion() const;

    /// Check the element map version.
    virtual bool checkElementMapVersion(const char* ver) const;

    /// Check if the given sub-name only contains an element name.
    static bool isElementName(const char* subName)
    {
        return (subName != nullptr) && (*subName != 0) && findElementName(subName) == subName;
    }

    /**
     * @brief Iterate through the history given an element name.
     *
     * Iterate through the history of the given element name with a given
     * callback.
     *
     * @param[in] name: the input element name
     * @param[in] cb: trace callback function.
     *
     * @sa TraceCallback
     */
    void traceElement(const MappedName& name, TraceCallback cb) const
    {
        _elementMap->traceElement(name, Tag, std::move(cb));
    }

    /// Flush internal buffers for element mapping.
    virtual void flushElementMap() const;
    /// @}

    /**
     * @name Save/restore
     *
     * @{
     */

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;
    unsigned int getMemSize() const override;

    /// Set the filename for persistence.
    void setPersistenceFileName(const char* name) const;

    /// Called before saving.
    virtual void beforeSave() const;

    /// Check if restore has failed.
    bool isRestoreFailed() const
    {
        return _restoreFailed;
    }

    /// Reset the restore failure flag.
    void resetRestoreFailure() const
    {
        _restoreFailed = true;
    }
    /// @}

    /**
     * @brief Dump the entire element map.
     *
     * Debugging method to dump an entire element map in human readable form to a stream.
     *
     * @param[in,out] stream The output stream.
     */
    void dumpElementMap(std::ostream& stream) const;

    /**
     * @brief Dump the entire element map to a string.
     *
     * Debugging method to dump an entire element map in human readable form
     * into a string.
     *
     * @return The string with the element map.
     */
    const std::string dumpElementMap() const;

protected:
    /// Transform the point from local to outside.
    inline Base::Vector3d transformPointToOutside(const Base::Vector3f& vec) const
    {
        // clang-format off
        return getTransform() * Base::Vector3d(static_cast<double>(vec.x),
                                               static_cast<double>(vec.y),
                                               static_cast<double>(vec.z));
        // clang-format on
    }

    /// Transform the points from local to outside.
    template<typename Vec>
    inline std::vector<Base::Vector3d> transformPointsToOutside(const std::vector<Vec>& input) const
    {
        // clang-format off
        std::vector<Base::Vector3d> output;
        output.reserve(input.size());
        Base::Matrix4D mat(getTransform());
        std::transform(input.cbegin(), input.cend(), std::back_inserter(output),
                       [&mat](const Vec& vec) {
                           return mat * Base::Vector3d(static_cast<double>(vec.x),
                                                       static_cast<double>(vec.y),
                                                       static_cast<double>(vec.z));
                       });

        return output;
        // clang-format on
    }

    /// Transform the vector from local to outside.
    inline Base::Vector3d transformVectorToOutside(const Base::Vector3f& vec) const
    {
        // clang-format off
        Base::Matrix4D mat(getTransform());
        mat.setCol(3, Base::Vector3d());
        return mat * Base::Vector3d(static_cast<double>(vec.x),
                                   static_cast<double>(vec.y),
                                   static_cast<double>(vec.z));
        // clang-format on
    }

    /// Transform the vectors from local to outside.
    template<typename Vec>
    std::vector<Base::Vector3d> transformVectorsToOutside(const std::vector<Vec>& input) const
    {
        // clang-format off
        std::vector<Base::Vector3d> output;
        output.reserve(input.size());
        Base::Matrix4D mat(getTransform());
        mat.setCol(3, Base::Vector3d());
        std::transform(input.cbegin(), input.cend(), std::back_inserter(output),
                       [&mat](const Vec& vec) {
                           return mat * Base::Vector3d(static_cast<double>(vec.x),
                                                       static_cast<double>(vec.y),
                                                       static_cast<double>(vec.z));
                       });

        return output;
        // clang-format on
    }

    /// Transform the point from local to inside.
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
    /**
     * @brief The master tag.
     *
     * The master tag is used to identify the shape to its owner document
     * object.  It should be unique within the document.  A tag of zero meanss
     * that automatic element mapping is disabled.
     */
    mutable long Tag {0};

    /// String hasher for element name shortening
    mutable App::StringHasherRef Hasher;

protected:
    /**
     * @brief Restore the element map from a stream.
     *
     * @param[in,out] stream The input stream.
     * @param[in] count The number of items to restore.
     */
    void restoreStream(std::istream& stream, std::size_t count);

    /**
     * @brief Read the elements from an XML reader.
     *
     * @param[in,out] reader The XML reader.
     * @param[in] count The number of elements to read.
     */
    void readElements(Base::XMLReader& reader, size_t count);

protected:
    /// Get the element map.
    ElementMapPtr elementMap(bool flush = true) const;
    /// Ensure there is an element map.
    ElementMapPtr ensureElementMap(bool flush = true);

private:
    ElementMapPtr _elementMap;

protected:
    /// The persistence file name.
    mutable std::string _persistenceName;
    /// Flag to indicate restore failure.
    mutable bool _restoreFailed = false;
};

}  // namespace Data

ENABLE_BITMASK_OPERATORS(Data::SearchOption)
