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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QtConcurrentMap>
#include <boost/math/special_functions/fpclassify.hpp>
#include <cmath>
#include <iostream>
#endif

#include <Base/Matrix.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "Points.h"
#include "PointsAlgos.h"


#ifdef _MSC_VER
#include <ppl.h>
#endif

using namespace Points;
using namespace std;

TYPESYSTEM_SOURCE(Points::PointKernel, Data::ComplexGeoData)

PointKernel::PointKernel(const PointKernel& pts)
    : _Mtrx(pts._Mtrx)
    , _Points(pts._Points)
{}

PointKernel::PointKernel(PointKernel&& pts) noexcept
    : _Mtrx(pts._Mtrx)
    , _Points(std::move(pts._Points))
{}

std::vector<const char*> PointKernel::getElementTypes() const
{
    std::vector<const char*> temp;
    // temp.push_back("Segment");

    return temp;
}

unsigned long PointKernel::countSubElements(const char* /*Type*/) const
{
    return 0;
}

Data::Segment* PointKernel::getSubElement(const char* /*Type*/, unsigned long /*n*/) const
{
    // unsigned long i = 1;

    // if (strcmp(Type,"Segment") == 0) {
    //     // not implemented
    //     assert(0);
    //     return 0;
    // }

    return nullptr;
}

void PointKernel::transformGeometry(const Base::Matrix4D& rclMat)
{
    std::vector<value_type>& kernel = getBasicPoints();
#ifdef _MSC_VER
    // Win32-only at the moment since ppl.h is a Microsoft library. Points is not using Qt so we
    // cannot use QtConcurrent We could also rewrite Points to leverage SIMD instructions Other
    // option: openMP. But with VC2013 results in high CPU usage even after computation (busy-waits
    // for >100ms)
    Concurrency::parallel_for_each(kernel.begin(), kernel.end(), [rclMat](value_type& value) {
        value = rclMat * value;
    });
#else
    QtConcurrent::blockingMap(kernel, [rclMat](value_type& value) {
        rclMat.multVec(value, value);
    });
#endif
}

Base::BoundBox3d PointKernel::getBoundBox() const
{
    Base::BoundBox3d bnd;

#ifdef _MSC_VER
    // Thread-local bounding boxes
    Concurrency::combinable<Base::BoundBox3d> bbs;
    // Cannot use a const_point_iterator here as it is *not* a proper iterator (fails the for_each
    // template)
    Concurrency::parallel_for_each(_Points.begin(),
                                   _Points.end(),
                                   [this, &bbs](const value_type& value) {
                                       Base::Vector3d vertd(value.x, value.y, value.z);
                                       bbs.local().Add(this->_Mtrx * vertd);
                                   });
    // Combine each thread-local bounding box in the final bounding box
    bbs.combine_each([&bnd](const Base::BoundBox3d& lbb) {
        bnd.Add(lbb);
    });
#else
    for (const auto& it : *this) {
        bnd.Add(it);
    }
#endif
    return bnd;
}

PointKernel& PointKernel::operator=(const PointKernel& Kernel)
{
    if (this != &Kernel) {
        // copy the mesh structure
        setTransform(Kernel._Mtrx);
        this->_Points = Kernel._Points;
    }

    return *this;
}

PointKernel& PointKernel::operator=(PointKernel&& Kernel) noexcept
{
    if (this != &Kernel) {
        // copy the mesh structure
        setTransform(Kernel._Mtrx);
        this->_Points = std::move(Kernel._Points);
    }

    return *this;
}

unsigned int PointKernel::getMemSize() const
{
    return _Points.size() * sizeof(value_type);
}

PointKernel::size_type PointKernel::countValid() const
{
    size_type num = 0;
    for (const auto& it : *this) {
        if (!(boost::math::isnan(it.x) || boost::math::isnan(it.y) || boost::math::isnan(it.z))) {
            num++;
        }
    }
    return num;
}

std::vector<PointKernel::value_type> PointKernel::getValidPoints() const
{
    std::vector<PointKernel::value_type> valid;
    valid.reserve(countValid());
    for (const auto& it : *this) {
        if (!(boost::math::isnan(it.x) || boost::math::isnan(it.y) || boost::math::isnan(it.z))) {
            valid.emplace_back(static_cast<float_type>(it.x),
                               static_cast<float_type>(it.y),
                               static_cast<float_type>(it.z));
        }
    }
    return valid;
}

void PointKernel::Save(Base::Writer& writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind() << "<Points file=\""
                        << writer.addFile(writer.ObjectName.c_str(), this) << "\" " << "mtrx=\""
                        << _Mtrx.toString() << "\"/>" << std::endl;
    }
}

void PointKernel::SaveDocFile(Base::Writer& writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)size();
    str << uCt;
    // store the data without transforming it
    for (const auto& pnt : _Points) {
        str << pnt.x << pnt.y << pnt.z;
    }
}

void PointKernel::Restore(Base::XMLReader& reader)
{
    clear();

    reader.readElement("Points");
    std::string file(reader.getAttribute("file"));

    if (!file.empty()) {
        // initiate a file read
        reader.addFile(file.c_str(), this);
    }
    if (reader.DocumentSchema > 3) {
        std::string Matrix(reader.getAttribute("mtrx"));
        _Mtrx.fromString(Matrix);
    }
}

void PointKernel::RestoreDocFile(Base::Reader& reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    _Points.resize(uCt);
    for (unsigned long i = 0; i < uCt; i++) {
        float x {};
        float y {};
        float z {};
        str >> x >> y >> z;
        _Points[i].Set(x, y, z);
    }
}

void PointKernel::save(const char* file) const
{
    Base::ofstream out(Base::FileInfo(file), std::ios::out);
    save(out);
}

void PointKernel::load(const char* file)
{
    PointsAlgos::Load(*this, file);
}

void PointKernel::save(std::ostream& out) const
{
    out << "# ASCII" << std::endl;
    for (const auto& pnt : _Points) {
        out << pnt.x << " " << pnt.y << " " << pnt.z << std::endl;
    }
}

void PointKernel::getPoints(std::vector<Base::Vector3d>& Points,
                            std::vector<Base::Vector3d>& /*Normals*/,
                            double /*Accuracy*/,
                            uint16_t /*flags*/) const
{
    unsigned long ctpoints = _Points.size();
    Points.reserve(ctpoints);
    for (unsigned long i = 0; i < ctpoints; i++) {
        Points.push_back(this->getPoint(i));
    }
}

// ----------------------------------------------------------------------------

PointKernel::const_point_iterator::const_point_iterator(
    const PointKernel* kernel,
    std::vector<kernel_type>::const_iterator index)
    : _kernel(kernel)
    , _p_it(index)
{
    if (_p_it != kernel->_Points.end()) {
        value_type vertd(_p_it->x, _p_it->y, _p_it->z);
        this->_point = _kernel->_Mtrx * vertd;
    }
}

PointKernel::const_point_iterator::const_point_iterator(
    const PointKernel::const_point_iterator& fi) = default;

PointKernel::const_point_iterator::const_point_iterator(PointKernel::const_point_iterator&& fi) =
    default;

PointKernel::const_point_iterator::~const_point_iterator() = default;

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator=(const PointKernel::const_point_iterator& pi) = default;

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator=(PointKernel::const_point_iterator&& pi) = default;

void PointKernel::const_point_iterator::dereference()
{
    value_type vertd(_p_it->x, _p_it->y, _p_it->z);
    this->_point = _kernel->_Mtrx * vertd;
}

const PointKernel::const_point_iterator::value_type& PointKernel::const_point_iterator::operator*()
{
    dereference();
    return this->_point;
}

const PointKernel::const_point_iterator::value_type* PointKernel::const_point_iterator::operator->()
{
    dereference();
    return &(this->_point);
}

bool PointKernel::const_point_iterator::operator==(
    const PointKernel::const_point_iterator& pi) const
{
    return (this->_kernel == pi._kernel) && (this->_p_it == pi._p_it);
}

bool PointKernel::const_point_iterator::operator!=(
    const PointKernel::const_point_iterator& pi) const
{
    return !operator==(pi);
}

PointKernel::const_point_iterator& PointKernel::const_point_iterator::operator++()
{
    ++(this->_p_it);
    return *this;
}

PointKernel::const_point_iterator PointKernel::const_point_iterator::operator++(int)
{
    PointKernel::const_point_iterator tmp = *this;
    ++(this->_p_it);
    return tmp;
}

PointKernel::const_point_iterator& PointKernel::const_point_iterator::operator--()
{
    --(this->_p_it);
    return *this;
}

PointKernel::const_point_iterator PointKernel::const_point_iterator::operator--(int)
{
    PointKernel::const_point_iterator tmp = *this;
    --(this->_p_it);
    return tmp;
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator+(difference_type off) const
{
    PointKernel::const_point_iterator tmp = *this;
    return (tmp += off);
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator-(difference_type off) const
{
    PointKernel::const_point_iterator tmp = *this;
    return (tmp -= off);
}

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator+=(difference_type off)
{
    (this->_p_it) += off;
    return *this;
}

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator-=(difference_type off)
{
    (this->_p_it) -= off;
    return *this;
}

PointKernel::difference_type
PointKernel::const_point_iterator::operator-(const PointKernel::const_point_iterator& right) const
{
    return this->_p_it - right._p_it;
}
