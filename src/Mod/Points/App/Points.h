/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef POINTS_POINT_H
#define POINTS_POINT_H

#include <iterator>
#include <vector>

#include <App/ComplexGeoData.h>
#include <App/PropertyGeo.h>
#include <Base/Matrix.h>
#include <Base/Reader.h>
#include <Base/Vector3D.h>
#include <Base/Writer.h>


#include <Mod/Points/PointsGlobal.h>

namespace Points
{

/** Point kernel
 */
class PointsExport PointKernel: public Data::ComplexGeoData
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    using float_type = float;
    using value_type = Base::Vector3<float_type>;
    using difference_type = std::vector<value_type>::difference_type;
    using size_type = std::vector<value_type>::size_type;

    PointKernel() = default;
    explicit PointKernel(size_type size)
    {
        resize(size);
    }
    PointKernel(const PointKernel&);
    ~PointKernel() override = default;

    void operator=(const PointKernel&);

    /** @name Subelement management */
    //@{
    /** Sub type list
     *  List of different subelement types
     *  its NOT a list of the subelements itself
     */
    std::vector<const char*> getElementTypes() const override;
    unsigned long countSubElements(const char* Type) const override;
    /// get the subelement by type and number
    Data::Segment* getSubElement(const char* Type, unsigned long) const override;
    //@}

    inline void setTransform(const Base::Matrix4D& rclTrf) override
    {
        _Mtrx = rclTrf;
    }
    inline Base::Matrix4D getTransform() const override
    {
        return _Mtrx;
    }
    std::vector<value_type>& getBasicPoints()
    {
        return this->_Points;
    }
    const std::vector<value_type>& getBasicPoints() const
    {
        return this->_Points;
    }
    void setBasicPoints(const std::vector<value_type>& pts)
    {
        this->_Points = pts;
    }
    void swap(std::vector<value_type>& pts)
    {
        this->_Points.swap(pts);
    }

    void getPoints(std::vector<Base::Vector3d>& Points,
                   std::vector<Base::Vector3d>& Normals,
                   double Accuracy,
                   uint16_t flags = 0) const override;
    void transformGeometry(const Base::Matrix4D& rclMat) override;
    Base::BoundBox3d getBoundBox() const override;

    /** @name I/O */
    //@{
    // Implemented from Persistence
    unsigned int getMemSize() const override;
    void Save(Base::Writer& writer) const override;
    void SaveDocFile(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;
    void RestoreDocFile(Base::Reader& reader) override;
    void save(const char* file) const;
    void save(std::ostream&) const;
    void load(const char* file);
    void load(std::istream&);
    //@}

private:
    Base::Matrix4D _Mtrx;
    std::vector<value_type> _Points;

public:
    /// number of points stored
    size_type size() const
    {
        return this->_Points.size();
    }
    size_type countValid() const;
    std::vector<value_type> getValidPoints() const;
    void resize(size_type n)
    {
        _Points.resize(n);
    }
    void reserve(size_type n)
    {
        _Points.reserve(n);
    }
    inline void erase(size_type first, size_type last)
    {
        _Points.erase(_Points.begin() + first, _Points.begin() + last);
    }

    void clear()
    {
        _Points.clear();
    }


    /// get the points
    inline const Base::Vector3d getPoint(const int idx) const
    {
        return transformPointToOutside(_Points[idx]);
    }
    /// set the points
    inline void setPoint(const int idx, const Base::Vector3d& point)
    {
        _Points[idx] = transformPointToInside(point);
    }
    /// insert the points
    inline void push_back(const Base::Vector3d& point)
    {
        _Points.push_back(transformPointToInside(point));
    }

    class PointsExport const_point_iterator
    {
    public:
        using kernel_type = PointKernel::value_type;
        using value_type = Base::Vector3d;
        using iter_type = std::vector<kernel_type>::const_iterator;
        using difference_type = iter_type::difference_type;
        using iterator_category = iter_type::iterator_category;
        using pointer = const value_type*;
        using reference = const value_type&;

        const_point_iterator(const PointKernel*, std::vector<kernel_type>::const_iterator index);
        const_point_iterator(const const_point_iterator& pi);
        ~const_point_iterator();

        const_point_iterator& operator=(const const_point_iterator& fi);
        const value_type& operator*();
        const value_type* operator->();
        bool operator==(const const_point_iterator& fi) const;
        bool operator!=(const const_point_iterator& fi) const;
        const_point_iterator& operator++();
        const_point_iterator operator++(int);
        const_point_iterator& operator--();
        const_point_iterator operator--(int);
        const_point_iterator operator+(difference_type off) const;
        const_point_iterator operator-(difference_type off) const;
        const_point_iterator& operator+=(difference_type off);
        const_point_iterator& operator-=(difference_type off);
        difference_type operator-(const const_point_iterator& right) const;

    private:
        void dereference();
        const PointKernel* _kernel;
        value_type _point;
        std::vector<kernel_type>::const_iterator _p_it;
    };

    using const_iterator = const_point_iterator;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    /** @name Iterator */
    //@{
    const_point_iterator begin() const
    {
        return {this, _Points.begin()};
    }
    const_point_iterator end() const
    {
        return {this, _Points.end()};
    }
    const_reverse_iterator rbegin() const
    {
        return const_reverse_iterator(end());
    }
    const_reverse_iterator rend() const
    {
        return const_reverse_iterator(begin());
    }
    //@}
};

}  // namespace Points


#endif  // POINTS_POINTPROPERTIES_H
