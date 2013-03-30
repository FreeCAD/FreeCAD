/***************************************************************************
 *   Copyright (c) Juergen Riegel         <juergen.riegel@web.de>          *
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
# include <math.h>
# include <iostream>
#endif

#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include <Base/Stream.h>
#include <Base/Writer.h>

#include "Points.h"
#include "PointsAlgos.h"
#include "PointsPy.h"

using namespace Points;
using namespace std;

TYPESYSTEM_SOURCE(Points::PointKernel, Data::ComplexGeoData);

std::vector<const char*> PointKernel::getElementTypes(void) const
{
    std::vector<const char*> temp;
    //temp.push_back("Segment");

    return temp;
}

unsigned long PointKernel::countSubElements(const char* Type) const
{
    return 0;
}

Data::Segment* PointKernel::getSubElement(const char* Type, unsigned long n) const
{
    //unsigned long i = 1;

    //if (strcmp(Type,"Segment") == 0) {
    //    // not implemented
    //    assert(0);
    //    return 0;
    //}

    return 0;
}

void PointKernel::transformGeometry(const Base::Matrix4D &rclMat)
{
    std::vector<value_type>& kernel = getBasicPoints();
    for (std::vector<value_type>::iterator it = kernel.begin(); it != kernel.end(); ++it)
        *it = rclMat * (*it);
}

Base::BoundBox3d PointKernel::getBoundBox(void)const
{
    Base::BoundBox3d bnd;
    for (const_point_iterator it = begin(); it != end(); ++it)
        bnd.Add(*it);
    return bnd;
}

void PointKernel::operator = (const PointKernel& Kernel)
{
    if (this != &Kernel) {
        // copy the mesh structure
        setTransform(Kernel._Mtrx);
        this->_Points = Kernel._Points;
    }
}

unsigned int PointKernel::getMemSize (void) const
{
    return _Points.size() * sizeof(value_type);
}

void PointKernel::Save (Base::Writer &writer) const
{
    if (!writer.isForceXML()) {
        writer.Stream() << writer.ind()
            << "<Points file=\"" << writer.addFile(writer.ObjectName.c_str(), this) << "\" " 
            << "mtrx=\"" << _Mtrx.toString() << "\"/>" << std::endl;
    }
}

void PointKernel::SaveDocFile (Base::Writer &writer) const
{
    Base::OutputStream str(writer.Stream());
    uint32_t uCt = (uint32_t)size();
    str << uCt;
    // store the data without transforming it
    for (std::vector<value_type>::const_iterator it = _Points.begin(); it != _Points.end(); ++it) {
        str << it->x << it->y << it->z;
    }
}

void PointKernel::Restore(Base::XMLReader &reader)
{
    clear();

    reader.readElement("Points");
    std::string file (reader.getAttribute("file") );

    if (!file.empty()) {
        // initate a file read
        reader.addFile(file.c_str(),this);
    }
    if (reader.DocumentSchema > 3) {
        std::string Matrix (reader.getAttribute("mtrx") );
        _Mtrx.fromString(Matrix);
    }
}

void PointKernel::RestoreDocFile(Base::Reader &reader)
{
    Base::InputStream str(reader);
    uint32_t uCt = 0;
    str >> uCt;
    _Points.resize(uCt);
    for (unsigned long i=0; i < uCt; i++) {
        float x, y, z;
        str >> x >> y >> z;
        _Points[i].Set(x,y,z);
    }
}

void PointKernel::save(const char* file) const
{
    //MeshCore::MeshOutput aWriter(_kernel);
    //aWriter.SaveAny(file);
}

void PointKernel::load(const char* file) 
{
    PointsAlgos::Load(*this,file);
}

void PointKernel::save(std::ostream& out) const
{
    //kernel.Write(out);
}

void PointKernel::getFaces(std::vector<Base::Vector3d> &Points,std::vector<Facet> &Topo,
                           float Accuracy, uint16_t flags) const
{
    unsigned long ctpoints = _Points.size();
    Points.reserve(ctpoints);
    for (unsigned long i=0; i<ctpoints; i++) {
        Points.push_back(this->getPoint(i));
    }
}

// ----------------------------------------------------------------------------

PointKernel::const_point_iterator::const_point_iterator
(const PointKernel* kernel, std::vector<kernel_type>::const_iterator index)
  : _kernel(kernel), _p_it(index)
{
    if(_p_it != kernel->_Points.end())
    {
        value_type vertd(_p_it->x, _p_it->y, _p_it->z);
        this->_point = _kernel->_Mtrx * vertd;
    }
}

PointKernel::const_point_iterator::const_point_iterator
(const PointKernel::const_point_iterator& fi)
  : _kernel(fi._kernel), _point(fi._point), _p_it(fi._p_it)
{
}

//PointKernel::const_point_iterator::~const_point_iterator()
//{
//}

PointKernel::const_point_iterator& 
PointKernel::const_point_iterator::operator=(const PointKernel::const_point_iterator& pi)
{
    this->_kernel  = pi._kernel;
    this->_point = pi._point;
    this->_p_it  = pi._p_it;
    return *this;
}

void PointKernel::const_point_iterator::dereference()
{
    value_type vertd(_p_it->x, _p_it->y, _p_it->z);
    this->_point = _kernel->_Mtrx * vertd;
}

const PointKernel::const_point_iterator::value_type&
PointKernel::const_point_iterator::operator*()
{
    dereference();
    return this->_point;
}

const PointKernel::const_point_iterator::value_type*
PointKernel::const_point_iterator::operator->()
{
    dereference();
    return &(this->_point);
}

bool PointKernel::const_point_iterator::operator==(const PointKernel::const_point_iterator& pi) const
{
    return (this->_kernel == pi._kernel) && (this->_p_it == pi._p_it);
}

bool PointKernel::const_point_iterator::operator!=(const PointKernel::const_point_iterator& pi) const
{
    return !operator==(pi);
}

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator++()
{
    ++(this->_p_it);
    return *this;
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator++(int)
{
    PointKernel::const_point_iterator tmp = *this;
    ++(this->_p_it);
    return tmp;
}

PointKernel::const_point_iterator&
PointKernel::const_point_iterator::operator--()
{
    --(this->_p_it);
    return *this;
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator--(int)
{
    PointKernel::const_point_iterator tmp = *this;
    --(this->_p_it);
    return tmp;
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator+ (difference_type off) const
{
    PointKernel::const_point_iterator tmp = *this;
    return (tmp+=off);
}

PointKernel::const_point_iterator
PointKernel::const_point_iterator::operator- (difference_type off) const
{
    PointKernel::const_point_iterator tmp = *this;
    return (tmp-=off);
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
PointKernel::const_point_iterator::operator- (const PointKernel::const_point_iterator& right) const
{
    return this->_p_it - right._p_it;
}
