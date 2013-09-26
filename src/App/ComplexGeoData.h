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

#include <Base/Placement.h>
#include <Base/Persistence.h>
#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/BoundBox.h>
#include <Base/Rotation.h>

#ifdef __GNUC__
# include <stdint.h>
#endif


namespace Data
{

/** Segments
 *  Subelement type of the ComplexGeoData type
 *  It is used to split an object in further sub-parts.
 */
class AppExport Segment: public Base::BaseClass
{
    TYPESYSTEM_HEADER();

public:
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
        std::vector<Line> &lines) const {}
    /** Get faces from segment */
    virtual void getFacesFromSubelement(
        const Segment*,
        std::vector<Base::Vector3d> &Points,
        std::vector<Base::Vector3d> &PointNormals,
        std::vector<Facet> &faces) const {}
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
        const Base::Vector3f& Base,
        const Base::Vector3f& Dir) const
    { return Base::Vector3d(); }
    /** Get points from object with given accuracy */
    virtual void getPoints(std::vector<Base::Vector3d> &Points,
        float Accuracy, uint16_t flags=0) const {};
    /** Get lines from object with given accuracy */
    virtual void getLines(std::vector<Base::Vector3d> &Points,std::vector<Line> &lines,
        float Accuracy, uint16_t flags=0) const {};
    /** Get faces from object with given accuracy */
    virtual void getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &faces,
        float Accuracy, uint16_t flags=0) const {};
    //@}

protected:

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
};

} //namespace App


#endif
