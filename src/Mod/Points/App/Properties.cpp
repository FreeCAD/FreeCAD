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
# include <cmath>
# include <iostream>
# include <algorithm>
#endif

#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Persistence.h>
#include <Base/Stream.h>
#include <Base/Writer.h>
#include <Base/VectorPy.h>

#include "Points.h"
#include "Properties.h"
#include "PointsPy.h"

#include <QtConcurrentMap>
#ifdef _WIN32
# include <ppl.h>
#endif

using namespace Points;
using namespace std;

TYPESYSTEM_SOURCE(Points::PropertyGreyValue, App::PropertyFloat)
TYPESYSTEM_SOURCE(Points::PropertyGreyValueList, App::PropertyLists)
TYPESYSTEM_SOURCE(Points::PropertyNormalList, App::PropertyLists)
TYPESYSTEM_SOURCE(Points::PropertyCurvatureList , App::PropertyLists)

void PropertyGreyValueList::removeIndices( const std::vector<unsigned long>& uIndices )
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    const std::vector<float>& rValueList = getValues();

    assert( uSortedInds.size() <= rValueList.size() );
    if ( uSortedInds.size() > rValueList.size() )
        return;

    std::vector<float> remainValue;
    remainValue.reserve(rValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for ( std::vector<float>::const_iterator it = rValueList.begin(); it != rValueList.end(); ++it ) {
        unsigned long index = it - rValueList.begin();
        if (pos == uSortedInds.end())
            remainValue.push_back( *it );
        else if (index != *pos)
            remainValue.push_back( *it );
        else 
            ++pos;
    }

    setValues(std::move(remainValue));
}

void PropertyNormalList::transformGeometry(const Base::Matrix4D &mat)
{
    // A normal vector is only a direction with unit length, so we only need to rotate it
    // (no translations or scaling)

    // Extract scale factors (assumes an orthogonal rotation matrix)
    // Use the fact that the length of the row vectors of R are all equal to 1
    // And that scaling is applied after rotating
    double s[3];
    s[0] = sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);
    s[1] = sqrt(mat[1][0] * mat[1][0] + mat[1][1] * mat[1][1] + mat[1][2] * mat[1][2]);
    s[2] = sqrt(mat[2][0] * mat[2][0] + mat[2][1] * mat[2][1] + mat[2][2] * mat[2][2]);

    // Set up the rotation matrix: zero the translations and make the scale factors = 1
    Base::Matrix4D rot;
    rot.setToUnity();
    for (unsigned short i = 0; i < 3; i++) {
        for (unsigned short j = 0; j < 3; j++) {
            rot[i][j] = mat[i][j] / s[i];
        }
    }

    atomic_change guard(*this);

    // Rotate the normal vectors
#ifdef _WIN32
    Concurrency::parallel_for_each(_lValueList.begin(), _lValueList.end(), [rot](Base::Vector3f& value) {
        value = rot * value;
    });
#else
    QtConcurrent::blockingMap(_lValueList, [rot](Base::Vector3f& value) {
        rot.multVec(value, value);
    });
#endif

    _touchList.clear();
    guard.tryInvoke();
}

void PropertyNormalList::removeIndices( const std::vector<unsigned long>& uIndices )
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    const std::vector<Base::Vector3f>& rValueList = getValues();

    assert( uSortedInds.size() <= rValueList.size() );
    if ( uSortedInds.size() > rValueList.size() )
        return;

    std::vector<Base::Vector3f> remainValue;
    remainValue.reserve(rValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for ( std::vector<Base::Vector3f>::const_iterator it = rValueList.begin(); it != rValueList.end(); ++it ) {
        unsigned long index = it - rValueList.begin();
        if (pos == uSortedInds.end())
            remainValue.push_back( *it );
        else if (index != *pos)
            remainValue.push_back( *it );
        else 
            ++pos;
    }

    setValues(std::move(remainValue));
}

PropertyCurvatureList::PropertyCurvatureList()
{

}

PropertyCurvatureList::~PropertyCurvatureList()
{

}

std::vector<float> PropertyCurvatureList::getCurvature( int mode ) const
{
    const std::vector<Points::CurvatureInfo>& fCurvInfo = getValues();
    std::vector<float> fValues;
    fValues.reserve(fCurvInfo.size());

    // Mean curvature
    if (mode == MeanCurvature) {
        for (std::vector<Points::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it) {
            fValues.push_back( 0.5f*(it->fMaxCurvature+it->fMinCurvature) );
        }
    }
    // Gaussian curvature
    else if (mode == GaussCurvature) {
        for (std::vector<Points::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it) {
            fValues.push_back( it->fMaxCurvature * it->fMinCurvature );
        }
    }
    // Maximum curvature
    else if (mode == MaxCurvature) {
        for (std::vector<Points::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it) {
            fValues.push_back( it->fMaxCurvature );
        }
    }
    // Minimum curvature
    else if (mode == MinCurvature) {
        for (std::vector<Points::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it) {
            fValues.push_back( it->fMinCurvature );
        }
    }
    // Absolute curvature
    else if (mode == AbsCurvature) {
        for (std::vector<Points::CurvatureInfo>::const_iterator it=fCurvInfo.begin();it!=fCurvInfo.end(); ++it) {
            if (fabs(it->fMaxCurvature) > fabs(it->fMinCurvature))
                fValues.push_back( it->fMaxCurvature );
            else
                fValues.push_back( it->fMinCurvature );
        }
    }

    return fValues;
}

void PropertyCurvatureList::transformGeometry(const Base::Matrix4D &mat)
{
    // The principal direction is only a vector with unit length, so we only need to rotate it
    // (no translations or scaling)

    // Extract scale factors (assumes an orthogonal rotation matrix)
    // Use the fact that the length of the row vectors of R are all equal to 1
    // And that scaling is applied after rotating
    double s[3];
    s[0] = sqrt(mat[0][0] * mat[0][0] + mat[0][1] * mat[0][1] + mat[0][2] * mat[0][2]);
    s[1] = sqrt(mat[1][0] * mat[1][0] + mat[1][1] * mat[1][1] + mat[1][2] * mat[1][2]);
    s[2] = sqrt(mat[2][0] * mat[2][0] + mat[2][1] * mat[2][1] + mat[2][2] * mat[2][2]);

    // Set up the rotation matrix: zero the translations and make the scale factors = 1
    Base::Matrix4D rot;
    rot.setToUnity();
    for (unsigned short i = 0; i < 3; i++) {
        for (unsigned short j = 0; j < 3; j++) {
            rot[i][j] = mat[i][j] / s[i];
        }
    }

    atomic_change guard(*this);

    // Rotate the principal directions
    for(auto &v : _lValueList) {
        CurvatureInfo ci = v;
        ci.cMaxCurvDir = rot * ci.cMaxCurvDir;
        ci.cMinCurvDir = rot * ci.cMinCurvDir;
        v = ci;
    }

    _touchList.clear();
    guard.tryInvoke();
}

void PropertyCurvatureList::removeIndices( const std::vector<unsigned long>& uIndices )
{
    // We need a sorted array
    std::vector<unsigned long> uSortedInds = uIndices;
    std::sort(uSortedInds.begin(), uSortedInds.end());

    assert( uSortedInds.size() <= _lValueList.size() );
    if ( uSortedInds.size() > _lValueList.size() )
        return;

    std::vector<CurvatureInfo> remainValue;
    remainValue.reserve(_lValueList.size() - uSortedInds.size());

    std::vector<unsigned long>::iterator pos = uSortedInds.begin();
    for ( std::vector<CurvatureInfo>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it ) {
        unsigned long index = it - _lValueList.begin();
        if (pos == uSortedInds.end())
            remainValue.push_back( *it );
        else if (index != *pos)
            remainValue.push_back( *it );
        else 
            ++pos;
    }

    setValues(std::move(remainValue));
}

PyObject *PropertyCurvatureList::getPyObject(void)
{
    throw Base::NotImplementedError("Not yet implemented");
}

CurvatureInfo PropertyCurvatureList::getPyValue(PyObject *) const
{
    throw Base::NotImplementedError("Not yet implemented");
}

bool PropertyCurvatureList::saveXML(Base::Writer &writer) const
{
    writer.Stream() << ">" << std::endl;
    for(auto &v : _lValueList)
        writer.Stream() << v.fMaxCurvature << ' '
                        << v.fMinCurvature << ' '
                        << v.cMaxCurvDir.x << ' '
                        << v.cMaxCurvDir.y << ' '
                        << v.cMaxCurvDir.z << ' '
                        << v.cMinCurvDir.x << ' '
                        << v.cMinCurvDir.y << ' '
                        << v.cMinCurvDir.z << ' '
                        << std::endl;
    return false;
}

void PropertyCurvatureList::restoreXML(Base::XMLReader &reader)
{
    unsigned count = reader.getAttributeAsUnsigned("count");
    auto &s = reader.beginCharStream(false);
    std::vector<CurvatureInfo> values(count);
    for(auto &v : values) {
        s >> v.fMaxCurvature
          >> v.fMinCurvature
          >> v.cMinCurvDir.x
          >> v.cMinCurvDir.y
          >> v.cMinCurvDir.z
          >> v.cMaxCurvDir.x
          >> v.cMaxCurvDir.y
          >> v.cMaxCurvDir.z;
    }
    reader.endCharStream();
    setValues(std::move(values));
}

void PropertyCurvatureList::saveStream(Base::OutputStream &str) const
{
    for (std::vector<CurvatureInfo>::const_iterator it = _lValueList.begin(); it != _lValueList.end(); ++it) {
        str << it->fMaxCurvature << it->fMinCurvature;
        str << it->cMaxCurvDir.x << it->cMaxCurvDir.y << it->cMaxCurvDir.z;
        str << it->cMinCurvDir.x << it->cMinCurvDir.y << it->cMinCurvDir.z;
    }
}

void PropertyCurvatureList::restoreStream(Base::InputStream &str, unsigned uCt)
{
    std::vector<CurvatureInfo> values(uCt);
    for (std::vector<CurvatureInfo>::iterator it = values.begin(); it != values.end(); ++it) {
        str >> it->fMaxCurvature >> it->fMinCurvature;
        str >> it->cMaxCurvDir.x >> it->cMaxCurvDir.y >> it->cMaxCurvDir.z;
        str >> it->cMinCurvDir.x >> it->cMinCurvDir.y >> it->cMinCurvDir.z;
    }
    setValues(std::move(values));
}

App::Property *PropertyCurvatureList::Copy(void) const 
{
    PropertyCurvatureList* prop = new PropertyCurvatureList();
    prop->_lValueList = this->_lValueList;
    return prop;
}

void PropertyCurvatureList::Paste(const App::Property &from)
{
    const PropertyCurvatureList& prop = dynamic_cast<const PropertyCurvatureList&>(from);
    setValues(prop._lValueList);
}

