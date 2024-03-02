// SPDX-License-Identifier: LGPL-2.0-or-later

/***************************************************************************
 *   Copyright (c) 2024 wandererfan <wandererfan at gmail dot com>         *
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

//! MeasureInfo.h
//! ancestor class and the various flavours of MeasureXXXXXInfo.

#ifndef PART_MEASUREINFO_H
#define PART_MEASUREINFO_H

#include <Mod/Part/PartGlobal.h>

// #include <TopoDS_Shape.hxx>

#include <Base/Vector3D.h>
#include <Base/Placement.h>

class TopoDS_Shape;

namespace Part {

class PartExport MeasureInfo {
public:
    // making the destructor virtual so MeasureInfo is polymorphic
    virtual ~MeasureInfo() = default;
    bool valid{false};
};

class PartExport MeasureAngleInfo : public MeasureInfo {
public:
    MeasureAngleInfo() = default;
    MeasureAngleInfo(bool val, Base::Vector3d orient, Base::Vector3d pos) { valid = val; orientation = orient; position = pos;};
    ~MeasureAngleInfo() = default;

    Base::Vector3d orientation{0.0, 0.0, 0.0};
    Base::Vector3d position{0.0, 0.0, 0.0};
};

class PartExport MeasureAreaInfo : public MeasureInfo {
public:
    MeasureAreaInfo() = default;
    MeasureAreaInfo(bool val, double a2, Base::Placement plm) { valid = val; area = a2; placement = plm;};
    ~MeasureAreaInfo() = default;

    double area{0};
    Base::Placement placement{};
};

// Translate geometry reference into an OCC type
class PartExport MeasureDistanceInfo : public MeasureInfo {
public:
    MeasureDistanceInfo() = default;
    MeasureDistanceInfo(bool val, const TopoDS_Shape* shp) { valid = val; shape = shp;};
    ~MeasureDistanceInfo() = default;

    // problematic as Gui can not see OCC
    // TopoDS_Shape shape{};
    const TopoDS_Shape* getShape() { return shape; }

private:
    const TopoDS_Shape* shape{nullptr};
};

class PartExport MeasureLengthInfo : public MeasureInfo {
public:
    MeasureLengthInfo() = default;
    MeasureLengthInfo(bool val, double len, Base::Placement plm) { valid = val; length = len; placement = plm;};
    ~MeasureLengthInfo() = default;

    double length{};
    Base::Placement placement{};
};

class PartExport MeasurePositionInfo : public MeasureInfo {
public:
    MeasurePositionInfo() = default;
    MeasurePositionInfo(bool val, Base::Vector3d pos) { valid = val; position = pos;};
    ~MeasurePositionInfo() = default;

    Base::Vector3d position{0.0, 09.0, 0.0};
};

class PartExport MeasureRadiusInfo : public MeasureInfo {
public:
    MeasureRadiusInfo() = default;
    MeasureRadiusInfo(bool val, double rad, Base::Vector3d point, Base::Placement plm) { valid = val; radius = rad; pointOnCurve = point;  placement = plm;};
    ~MeasureRadiusInfo() = default;

    double radius{};
    Base::Vector3d pointOnCurve;
    Base::Placement placement;  // curve center & circle orientation
};

//! callback registrations
    using GeometryHandler = std::function<Part::MeasureInfo* (std::string*, std::string*)>;
    
class PartExport CallbackRegistrationRecord
{
public:
    CallbackRegistrationRecord() = default;
    CallbackRegistrationRecord(const std::string& module, const std::string& measureType, GeometryHandler callback)
        { m_module = module; m_measureType = measureType;  m_callback = callback; }
    
    std::string m_module;
    std::string m_measureType;
    GeometryHandler m_callback;
};

using CallbackRegistrationList = std::vector<CallbackRegistrationRecord>;

}  //end namespace Part

#endif
