// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <vector>

#include <App/PropertyStandard.h>
#include <Base/Matrix.h>
#include <Base/Reader.h>
#include <Base/Writer.h>

#include "Points.h"


namespace Points
{

/** Greyvalue property.
 */
class PointsExport PropertyGreyValue: public App::PropertyFloat
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyGreyValue() = default;
};

class PointsExport PropertyGreyValueList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyGreyValueList();

    void setSize(int newSize) override;
    int getSize() const override;

    /** Sets the property
     */
    void setValue(float);

    /// index operator
    float operator[](const int idx) const
    {
        return _lValueList[idx];
    }

    void set1Value(const int idx, float value)
    {
        _lValueList[idx] = value;
    }
    void setValues(const std::vector<float>& values);

    const std::vector<float>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;
    unsigned int getMemSize() const override;

    /** @name Modify */
    //@{
    void removeIndices(const std::vector<unsigned long>&);
    //@}

private:
    std::vector<float> _lValueList;
};

class PointsExport PropertyNormalList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    PropertyNormalList();

    void setSize(int newSize) override;
    int getSize() const override;

    void setValue(const Base::Vector3f&);
    void setValue(float x, float y, float z);

    const Base::Vector3f& operator[](const int idx) const
    {
        return _lValueList[idx];
    }

    void set1Value(const int idx, const Base::Vector3f& value)
    {
        _lValueList[idx] = value;
    }

    void setValues(const std::vector<Base::Vector3f>& values);

    const std::vector<Base::Vector3f>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;

    App::Property* Copy() const override;
    void Paste(const App::Property& from) override;

    unsigned int getMemSize() const override;

    /** @name Modify */
    //@{
    void transformGeometry(const Base::Matrix4D& rclMat);
    void removeIndices(const std::vector<unsigned long>&);
    //@}

private:
    std::vector<Base::Vector3f> _lValueList;
};

/** Curvature information. */
struct PointsExport CurvatureInfo
{
    float fMaxCurvature {}, fMinCurvature {};
    Base::Vector3f cMaxCurvDir, cMinCurvDir;
};

/** The Curvature property class.
 */
class PointsExport PropertyCurvatureList: public App::PropertyLists
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum
    {
        MeanCurvature = 0,  /**< Mean curvature */
        GaussCurvature = 1, /**< Gaussian curvature */
        MaxCurvature = 2,   /**< Maximum curvature */
        MinCurvature = 3,   /**< Minimum curvature */
        AbsCurvature = 4    /**< Absolute curvature */
    };

public:
    PropertyCurvatureList();

    void setSize(int newSize) override
    {
        _lValueList.resize(newSize);
    }
    int getSize() const override
    {
        return _lValueList.size();
    }
    void setValue(const CurvatureInfo&);
    void setValues(const std::vector<CurvatureInfo>&);
    std::vector<float> getCurvature(int tMode) const;

    /// index operator
    const CurvatureInfo& operator[](const int idx) const
    {
        return _lValueList[idx];
    }
    void set1Value(const int idx, const CurvatureInfo& value)
    {
        _lValueList[idx] = value;
    }
    const std::vector<CurvatureInfo>& getValues() const
    {
        return _lValueList;
    }

    PyObject* getPyObject() override;
    void setPyObject(PyObject*) override;

    /** @name Save/restore */
    //@{
    void Save(Base::Writer& writer) const override;
    void Restore(Base::XMLReader& reader) override;

    void SaveDocFile(Base::Writer& writer) const override;
    void RestoreDocFile(Base::Reader& reader) override;
    //@}

    /** @name Undo/Redo */
    //@{
    /// returns a new copy of the property (mainly for Undo/Redo and transactions)
    App::Property* Copy() const override;
    /// paste the value from the property (mainly for Undo/Redo and transactions)
    void Paste(const App::Property& from) override;
    unsigned int getMemSize() const override;
    //@}

    /** @name Modify */
    //@{
    void transformGeometry(const Base::Matrix4D& rclMat);
    void removeIndices(const std::vector<unsigned long>&);
    //@}

private:
    std::vector<CurvatureInfo> _lValueList;
};

}  // namespace Points
