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


#ifndef _AppComplexGeoData_h_
#define _AppComplexGeoData_h_

#include <cstring>
#include <memory>
#include <cctype>
#include <functional>

#include <QVector>

#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include "StringHasher.h"

#ifdef __GNUC__
# include <cstdint>
#endif


namespace Base
{
class Placement;
class Rotation;
template <class _Precision> class BoundBox3;
typedef BoundBox3<double> BoundBox3d;
}

namespace Data
{

class ElementMap;
typedef std::shared_ptr<ElementMap> ElementMapPtr;

typedef QVector<App::StringIDRef> ElementIDRefs;

class IndexedName;
class MappedName;
struct MappedElement;
struct MappedChildElements;

/** Segments
 *  Subelement type of the ComplexGeoData type
 *  It is used to split an object in further sub-parts.
 */
class AppExport Segment: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
    virtual ~Segment(){}
    virtual std::string getName() const=0;
};


/** ComplexGeoData Object
 */
class AppExport ComplexGeoData: public Base::Persistence, public Base::Handled
{
    TYPESYSTEM_HEADER();

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
    virtual ~ComplexGeoData();

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  its NOT a list of the subelements itself
     */
    virtual const std::vector<const char*>& getElementTypes() const=0;
    virtual unsigned long countSubElements(const char* Type) const=0;
    /// get the subelement by type and number
    virtual Segment* getSubElement(const char* Type, unsigned long) const=0;
    /// get subelement by combined name
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
    /// Get the bound box
    virtual Base::BoundBox3d getBoundBox()const=0;
    /** Get point from line object intersection  */
    virtual Base::Vector3d getPointFromLineIntersection(
        const Base::Vector3f& base,
        const Base::Vector3f& dir) const;
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &Normals,
        float Accuracy, uint16_t flags=0) const;
    /** Get lines from object with given accuracy */
    virtual void getLines(std::vector<Base::Vector3d> &Points,std::vector<Line> &lines,
        float Accuracy, uint16_t flags=0) const;
    /** Get faces from object with given accuracy */
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        float Accuracy, uint16_t flags=0) const;
    /** Get the center of gravity
     * If this method is implemented then true is returned and the center of gravity.
     * The default implementation only returns false.
     */
    virtual bool getCenterOfGravity(Base::Vector3d& center) const;
    //@}

    /** @name Element name mapping */
    //@{
    /// Special prefix to mark the beginning of a mapped sub-element name
    static const std::string &elementMapPrefix();
    /// Special postfix to mark the following tag encoded as hex number
    static const std::string &tagPostfix();
    /// Special postfix to mark the following tag encoded as decimal number
    static const std::string &decTagPostfix();
    /// Special postfix to mark the name includes encoding from an external object
    static const std::string &externalTagPostfix();
    /// Special postfix to mark the index of an array element
    static const std::string &indexPostfix();
    /// Special prefix to mark a missing element
    static const std::string &missingPrefix();
    /// Check if a subname contains missing element
    static bool hasMissingElement(const char *subname);
    /** Check if the name starts with elementMapPrefix()
     *
     * @param name: input name
     * @return Returns the name stripped with elementMapPrefix(), or 0 if not
     * start with the prefix
     */
    static const char *isMappedElement(const char *name);

    /// Strip out the trailing element name if there is mapped element name precedes it.
    static std::string newElementName(const char *name);
    /// Strip out the mapped element name if there is one.
    static std::string oldElementName(const char *name);
    /// Strip out the old and new element name if there is one.
    static std::string noElementName(const char *name);

    /// Find the start of an element name in a subname
    static const char *findElementName(const char *subname);
    
    /// Check if the given subname contains element name
    static bool hasElementName(const char *subname) {
        subname = findElementName(subname);
        return subname && *subname;
    }

    /// Return the element name portion of the subname without mapping prefix
    static inline const char *hasMappedElementName(const char *subname) {
        return isMappedElement(findElementName(subname));
    }

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
     * @param sid: optional output of and App::StringID involved forming this
     *             mapped name
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

    /** Get mapped element with a given prefix */
    std::vector<MappedElement> getElementNamesWithPrefix(const char *prefix) const;

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

    /** Add a sub-element name mapping.
     *
     * @param element: the original \c Type + \c Index element name
     * @param name: the mapped sub-element name. May or may not start with
     * elementMapPrefix().
     * @param sid: in case you use a hasher to hash the element name, pass in
     * the string id reference using this parameter. You can have more than one
     * string id associated with the same name.
     * @param overwrite: if true, it will overwrite existing names
     *
     * @return Returns the stored mapped element name.
     *
     * An element can have multiple mapped names. However, a name can only be
     * mapped to one element
     */
	MappedName setElementName(const IndexedName & element,
                              const MappedName & name,
                              const ElementIDRefs * sid = nullptr,
                              bool overwrite = false);

    void setMappedChildElements(const std::vector<MappedChildElements> & children);
    std::vector<MappedChildElements> getMappedChildElements() const;

    /** Convenience method to hash the main element name
     *
     * @param name: main element name
     * @param sid: store any output string ID references
     * @return the hashed element name;
     */
    MappedName hashElementName(const MappedName & name, ElementIDRefs &sid) const;

    /// Hash the child element map postfixes to shorten element name from hierarchical maps
    void hashChildMaps();

    /// Check if there is child element map
    bool hasChildElementMap() const;

    /// Reverse hashElementName()
    MappedName dehashElementName(const MappedName & name) const;
     
    /// Append the Tag (if and only if it is non zero) into the element map
    virtual void reTagElementMap(long tag, App::StringHasherRef hasher, const char *postfix=0) {
        (void)tag;
        (void)hasher;
        (void)postfix;
    }

    long getElementHistory(const char *name, 
            MappedName *original=0, std::vector<MappedName> *history=0) const;

    long getElementHistory(const MappedName & name, 
            MappedName *original=0, std::vector<MappedName> *history=0) const;

    void encodeElementName(char element_type, MappedName & name, std::ostringstream &ss,
            ElementIDRefs *sids, const char* postfix=0, long tag=0, bool forceTag=false) const;

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
        _ElementMap.swap(elementMap);
        return elementMap;
    }

    /// Get the entire element map
    std::vector<MappedElement> getElementMap() const;

    /// Set the entire element map
    void setElementMap(const std::vector<MappedElement> &elements);
    
    /// Get the current element map size
    size_t getElementMapSize(bool flush=true) const;

    /// Return the higher level element names of the given element
    virtual std::vector<IndexedName> getHigherElements(const char *name, bool silent=false) const;

    /// Return the current element map version
    virtual std::string getElementMapVersion() const;

    /// Return true to signal element map version change
    virtual bool checkElementMapVersion(const char * ver) const;

    /// Check if the given subname only contains an element name
    static bool isElementName(const char *subname) {
        return subname && *subname && findElementName(subname)==subname;
    }

    /** Extract tag and other information from a encoded element name
     *
     * @param name: encoded element name
     * @param tag: optional pointer to receive the extracted tag
     * @param len: optional pointer to receive the length field after the tag field.
     *             This gives the length of the previous hashsed element name starting
     *             from the beginning of the give element name.
     * @param postfix: optional pointer to receive the postfix starting at the found tag field.
     * @param type: optional pointer to receive the element type character
     * @param negative: return negative tag as it is. If disabled, then always return positive tag.
     *                  Negative tag is sometimes used for element disambiguation.
     * @param recursive: recursively find the last non-zero tag
     *
     * @return Return the end position of the tag field, or return -1 if not found.
     */
    static int findTagInElementName(const MappedName & name,
                                    long *tag=0,
                                    int *len=0,
                                    std::string *postfix=0,
                                    char *type=0,
                                    bool negative=false,
                                    bool recursive=true);

    /** Element trace callback
     *
     * The callback has the following call signature
     *  (const std::string &name, size_t offset, long encodedTag, long tag) -> bool
     *
     * @param name: the current element name.
     * @param offset: the offset skipping the encoded element name for the next iteration.
     * @param encodedTag: the tag encoded inside the current element, which is usually the tag
     *                    of the previous step in the shape history.
     * @param tag: the tag of the current shape element.
     *
     * @sa traceElement()
     */
    typedef std::function<bool(const MappedName &, int, long, long)> TraceCallback;

    /** Iterate through the history of the give element name with a given callback
     *
     * @param name: the input element name
     * @param cb: trace callback with call signature.
     * @sa TraceCallback
     */
    void traceElement(const MappedName &name, TraceCallback cb) const;

    /** Flush an internal buffering for element mapping */
    virtual void flushElementMap() const;
    virtual unsigned long getElementMapReserve() const { return 0; }
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    void SaveDocFile(Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);
    unsigned int getMemSize (void) const;
    void setPersistenceFileName(const char *name) const;
    virtual void beforeSave() const;
    bool isRestoreFailed() const { return _restoreFailed; }
    void resetRestoreFailure() const { _restoreFailed = true; }
    //@}

public:
    /// String hasher for element name shortening
    mutable App::StringHasherRef Hasher;

protected:
    virtual MappedName renameDuplicateElement(int index,
									         const IndexedName & element, 
           								     const IndexedName & element2,
										     const MappedName & name,
										     ElementIDRefs &sids);

    void restoreStream(std::istream &s, std::size_t count);

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

public:
    mutable long Tag;

protected:
    ElementMapPtr elementMap(bool flush=true) const;

protected:
    mutable std::string _PersistenceName;
    mutable bool _restoreFailed = false;

private:
    ElementMapPtr _ElementMap;
};

} //namespace App


#endif
