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

class PointsExport PropertyGreyValueList: public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    PropertyGreyValueList();
    virtual ~PropertyGreyValueList();
    
    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    /** Sets the property 
     */
    void setValue(float);
    
    /// index operator
    float operator[] (const int idx) const {return _lValueList.operator[] (idx);} 
    
    void set1Value (const int idx, float value){_lValueList.operator[] (idx) = value;}
    void setValues (const std::vector<float>& values);
    
    const std::vector<float> &getValues(void) const{return _lValueList;}
    
    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);
    
    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);
    
    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);
    
    virtual App::Property *Copy(void) const;
    virtual void Paste(const App::Property &from);
    virtual unsigned int getMemSize (void) const;

    /** @name Modify */
    //@{
    void removeIndices( const std::vector<unsigned long>& );
    //@}

private:
    std::vector<float> _lValueList;
};

class PointsExport PropertyNormalList: public App::PropertyLists
{
    TYPESYSTEM_HEADER();

public:
    PropertyNormalList();
    ~PropertyNormalList();

    virtual void setSize(int newSize);
    virtual int getSize(void) const;

    void setValue(const Base::Vector3f&);
    void setValue(float x, float y, float z);

    const Base::Vector3f& operator[] (const int idx) const {
        return _lValueList.operator[] (idx);
    }

    void set1Value (const int idx, const Base::Vector3f& value) {
        _lValueList.operator[] (idx) = value;
    }

    void setValues (const std::vector<Base::Vector3f>& values);

    const std::vector<Base::Vector3f> &getValues(void) const {
        return _lValueList;
    }

    virtual PyObject *getPyObject(void);
    virtual void setPyObject(PyObject *);

    virtual void Save (Base::Writer &writer) const;
    virtual void Restore(Base::XMLReader &reader);

    virtual void SaveDocFile (Base::Writer &writer) const;
    virtual void RestoreDocFile(Base::Reader &reader);

    virtual App::Property *Copy(void) const;
    virtual void Paste(const App::Property &from);

    virtual unsigned int getMemSize (void) const;

    /** @name Modify */
    //@{
    void transform(const Base::Matrix4D &rclMat);
    void removeIndices( const std::vector<unsigned long>& );
    //@}

private:
    std::vector<Base::Vector3f> _lValueList;
};

/** Curvature information. */
struct PointsExport CurvatureInfo
{
    float fMaxCurvature, fMinCurvature;
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
    std::vector<float> getCurvature( int tMode) const;

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
