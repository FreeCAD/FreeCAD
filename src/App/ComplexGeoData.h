/***************************************************************************
 *   Copyright (c) Jürgen Riegel          (juergen.riegel@web.de) 2002     *
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

#include <memory>
#include <cctype>
#include <Base/Placement.h>
#include <Base/Persistence.h>
#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/BoundBox.h>
#include <Base/Rotation.h>
#include "StringHasher.h"

#ifdef __GNUC__
# include <stdint.h>
#endif


namespace Data
{

class ElementMap;
typedef std::shared_ptr<ElementMap> ElementMapPtr;

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
    ComplexGeoData(void);
    /// Destructor
    virtual ~ComplexGeoData();

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  its NOT a list of the subelements itself
     */
    virtual std::vector<const char*> getElementTypes(void) const=0;
    virtual unsigned long countSubElements(const char* Type) const=0;
    /// get the subelement by type and number
    virtual Segment* getSubElement(const char* Type, unsigned long) const=0;
    /// get subelement by combined name
    virtual Segment* getSubElementByName(const char* Name) const;
    /** Get lines from segment */
    virtual void getLinesFromSubelement(
        const Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Line> &lines) const;
    /** Get faces from segment */
    virtual void getFacesFromSubelement(
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
    virtual Base::Matrix4D getTransform(void) const = 0;
    //@}

    /** @name Modification */
    //@{
    /// Applies a transformation on the real geometric data type
    virtual void transformGeometry(const Base::Matrix4D &rclMat) = 0;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    /// Get the bound box
    virtual Base::BoundBox3d getBoundBox(void)const=0;
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
    /// Sepecial prefix to mark the begining of a mapped sub-element name
    static const std::string &elementMapPrefix();
    /// Sepecial postfix to mark the following tag
    static const std::string &tagPostfix();
    /// Speical postfix to mark the index of an array element
    static const std::string &indexPostfix();
    /// Speical prefix to mark a missing element
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

    /// Strip out the trailing element name if there is mapped element name preceeds it.
    static std::string newElementName(const char *name);
    /// Strip out the mapped element name if there is one.
    static std::string oldElementName(const char *name);
    /// Strip out the old and new element name if there is one.
    static std::string noElementName(const char *name);

    /// Find the start of an element name in a subname
    static const char *findElementName(const char *subname);

    static inline const char *hasMappedElementName(const char *subname) {
        return isMappedElement(findElementName(subname));
    }

    /** Get element name
     *
     * @param name: the input name
     * @param direction: if 0 (default), the function try to map the name
     * to the original \c Type + \c Index. If 1, then the function map the
     * other way round but demands the name starts with elementMapPrefix(). 
     * If 2, then same as 1 except the name can either start with
     * elementMapPrefix() or not.
     *
     * @return Returns the found mapping, or else return the original input. The
     * return pointer maybe invalidated when new element mapping is added.
     */
    const char *getElementName(const char *name, int direction=0, 
            std::vector<App::StringIDRef> *sid=0) const;

    /** Get mapped element names with a given prefix */
    std::vector<std::pair<std::string,std::string> > getElementNamesWithPrefix(const char *prefix) const;

    /** Get mapped element names
     *
     * @param element: original element name with \c Type + \c Index
     * @param needUnmapped: if true, return the original element name if no
     * mapping is found
     *
     * @return a list of mapped names of the give element along with their
     * associated string ID references
     */
    std::vector<std::pair<std::string, std::vector<App::StringIDRef> > >
       getElementMappedNames(const char *element, bool needUnmapped=false) const;

    /** Add a sub-element name mapping.
     *
     * @param element: the original \c Type + \c Index element name
     * @param name: the renamed sub-element name. May or may not start with
     * elementMapPrefix().
     * @param hasher: in case the raw 'name' is too long. The caller can opt to
     * use this hasher to hash the string and shorten it to an integer, which
     * is reference counted and persistent.
     * @param sid: in case you use a hasher to hash the element name, pass in
     * the string id reference using this parameter. You can have more than one
     * string id associated with the same name.
     * @param overwrite: if true, it will overwrite existing names
     *
     * @return Returns the stored mapped element name. Note that if hasher is
     * provided the stored name will be different from the input name.
     *
     * An element can have multiple mapped names. However, a name can only be
     * mapped to one element
     */
    const char *setElementName(const char *element, const char *name, 
            const std::vector<App::StringIDRef> *sid=0, bool overwrite=false, bool nohash=false);

    /** Add a sub element name mapping with unhashed postfix */
    const char *setElementName(const char *element, const char *name, 
            const char *postfix, const std::vector<App::StringIDRef> *sid=0, bool overwrite=false);

    /** Convenience method to hash the main element name
     *
     * @param name: main element name
     * @param sid: store any output string ID references
     * @return the hashed element name;
     */
    std::string hashElementName(const char *name, std::vector<App::StringIDRef> &sid) const;

    /// Reverse hashElementName()
    std::string dehashElementName(const char *name) const;
     
    /** Copy the element map from another geometry data with optional unhashed prefix and/or postfix */
    void copyElementMap(const ComplexGeoData &data, const char *postfix=0);

    /// Append the Tag (if and only if it is non zero) into the element map
    virtual void reTagElementMap(long tag, App::StringHasherRef hasher, const char *postfix=0) {
        (void)tag;
        (void)hasher;
        (void)postfix;
    }

    long getElementHistory(const char *name, 
            std::string *original=0, std::vector<std::string> *history=0) const;

    void encodeElementName(char element_type, std::string &name, std::ostringstream &ss, 
            std::vector<App::StringIDRef> &sids, const char* postfix=0, long tag=0) const;

    char elementType(const char *name) const;

    /** Reset/swap the element map
     *
     * @param elementMap: optional new element map
     *
     * @return Returns the existing element map.
     */
    ElementMapPtr resetElementMap(ElementMapPtr elementMap=ElementMapPtr()) {
        _ElementMap.swap(elementMap);
        return elementMap;
    }

    /// Get the entire element map
    std::map<std::string, std::string> getElementMap() const;

    /// Set the entire element map
    void setElementMap(const std::map<std::string, std::string> &map);
    
    /// Get the current element map size
    size_t getElementMapSize() const;

    virtual std::string getElementMapVersion() const;
    //@}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);
    unsigned int getMemSize (void) const;
    //@}

public:
    /// String hasher for element name shortening
    mutable App::StringHasherRef Hasher;

protected:
    virtual std::string renameDuplicateElement(int index, const char *element, 
           const char *element2, const char *name, std::vector<App::StringIDRef> &sids);

    static size_t findTagInElementName(const std::string &name, 
            long *tag=0, size_t *len=0, std::string *postfix=0, char *type=0);

    /// from local to outside
    inline Base::Vector3d transformToOutside(const Base::Vector3f& vec) const
    {
        return getTransform() * Base::Vector3d(vec.x,vec.y,vec.z);
    }
    /// from local to inside
    inline Base::Vector3f transformToInside(const Base::Vector3d& vec) const
    {
        Base::Matrix4D tmpM(getTransform()); 
        tmpM.inverse();
        Base::Vector3d tmp = tmpM * vec;
        return Base::Vector3f((float)tmp.x,(float)tmp.y,(float)tmp.z);
    }

public:
    mutable long Tag;

protected:
    ElementMapPtr _ElementMap;
};

struct AppExport ElementNameComp {
    /** Comparison function to make topo name more stable
     *
     * The sorting decompose the name into either of the following two forms
     *      '#' + hex_digits + tail
     *      non_digits + digits + tail
     *
     * The non-digits part is compared lexically, while the digits part is
     * compared by its integer value.
     *
     * The reason for this is to prevent name with bigger digits (usually means
     * comes late in history) comes early when sorting.
     */
    bool operator()(const std::string &a, const std::string &b) const;
};

typedef std::set<std::string,ElementNameComp> ElementNameSet;

} //namespace App


#endif
