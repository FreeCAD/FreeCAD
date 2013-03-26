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


#ifndef POINTS_POINTPROPERTIES_H
#define POINTS_POINTPROPERTIES_H

#include <vector>

#include <Base/Vector3D.h>
#include <Base/Matrix.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>

#include "Points.h"

namespace Points
{


/** Greyvalue property.
 */
class PointsExport PropertyGreyValue : public App::PropertyFloat
{
    TYPESYSTEM_HEADER();

public:
    PropertyGreyValue(void)
    {
    }
    virtual ~PropertyGreyValue()
    {
    }
};

/**
 * Own class to distinguish from real float list
 */
class PointsExport PropertyGreyValueList : public App::PropertyFloatList
{
    TYPESYSTEM_HEADER();

public:
    PropertyGreyValueList()
    {
    }
    virtual ~PropertyGreyValueList()
    {
    }

    /** @name Modify */
    //@{
    void removeIndices( const std::vector<unsigned long>& );
    //@}
};

/**
 * Own class to distinguish from real vector list
 */
class PointsExport PropertyNormalList : public App::PropertyVectorList
{
    TYPESYSTEM_HEADER();

public:
    PropertyNormalList()
    {
    }
    virtual ~PropertyNormalList()
    {
    }

    /** @name Modify */
    //@{
    void transform(const Base::Matrix4D &rclMat);
    void removeIndices( const std::vector<unsigned long>& );
    //@}
};

/** Curvature information. */
struct PointsExport CurvatureInfo
{
    double fMaxCurvature, fMinCurvature;
    Base::Vector3f cMaxCurvDir, cMinCurvDir;
};

/** The Curvature property class.
 */
class PointsExport PropertyCurvatureList: public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    enum { 
        MeanCurvature  = 0,  /**< Mean curvature */
        GaussCurvature = 1,  /**< Gaussian curvature */
        MaxCurvature   = 2,  /**< Maximum curvature */ 
        MinCurvature   = 3,  /**< Minimum curvature */
        AbsCurvature   = 4   /**< Absolute curvature */
    };

public:
    PropertyCurvatureList();
    ~PropertyCurvatureList();

    void setSize(int newSize){_lValueList.resize(newSize);}   
    int getSize(void) const {return _lValueList.size();}   
    void setValue(const CurvatureInfo&);
    void setValues(const std::vector<CurvatureInfo>&);
    std::vector<double> getCurvature( int tMode) const;

    /// index operator
    const CurvatureInfo& operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    void  set1Value (const int idx, const CurvatureInfo& value){_lValueList.operator[] (idx) = value;}
    const std::vector<CurvatureInfo> &getValues(void) const{return _lValueList;}

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);
    //@}

    /** @name Undo/Redo */
    //@{
    /// returns a new copy of the property (mainly for Undo/Redo and transactions)
    App::Property *Copy(void) const;
    /// paste the value from the property (mainly for Undo/Redo and transactions)
    void Paste(const App::Property &from);
    unsigned int getMemSize (void) const;
    //@}

    /** @name Modify */
    //@{
    void transform(const Base::Matrix4D &rclMat);
    void removeIndices( const std::vector<unsigned long>& );
    //@}

private:
    std::vector<CurvatureInfo> _lValueList;
};

} // namespace Points


#endif // POINTS_POINTPROPERTIES_H 
