/***************************************************************************
 *   Copyright (c) 2019 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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

#ifndef SKETCHER_SKETCHGEOMETRYEXTENSION_H
#define SKETCHER_SKETCHGEOMETRYEXTENSION_H

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/SketcherGlobal.h>
#include <atomic>
#include <bitset>
#include <array>

namespace Sketcher
{

    namespace InternalType {
        enum InternalType {
            None                    = 0,
            EllipseMajorDiameter    = 1,
            EllipseMinorDiameter    = 2,
            EllipseFocus1           = 3,
            EllipseFocus2           = 4,
            HyperbolaMajor          = 5,
            HyperbolaMinor          = 6,
            HyperbolaFocus          = 7,
            ParabolaFocus           = 8,
            BSplineControlPoint     = 9,
            BSplineKnotPoint        = 10,
            NumInternalGeometryType        // Must be the last
        };
    }

    namespace GeometryMode {
        enum GeometryMode {
            Blocked                 = 0,
            Construction            = 1,
            NumGeometryMode        // Must be the last
        };
    }

class ISketchGeometryExtension
{

public:
    // Identification information
    virtual long getId() const = 0;
    virtual void setId(long id) = 0;

    // Internal Alignment Geometry Type
    virtual InternalType::InternalType getInternalType() const = 0;
    virtual void setInternalType(InternalType::InternalType type) = 0;

    // Geometry functional mode
    virtual bool testGeometryMode(int flag) const = 0;
    virtual void setGeometryMode(int flag, bool v=true) = 0;

    virtual int getGeometryLayerId() const = 0;
    virtual void setGeometryLayerId(int geolayer) = 0;
};

class SketcherExport SketchGeometryExtension : public Part::GeometryPersistenceExtension, private ISketchGeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:

    SketchGeometryExtension();
    SketchGeometryExtension(long cid);
    virtual ~SketchGeometryExtension() override = default;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    virtual long getId() const override {return Id;}
    virtual void setId(long id) override {Id = id;}

    virtual InternalType::InternalType getInternalType() const override {return InternalGeometryType;}
    virtual void setInternalType(InternalType::InternalType type) override {InternalGeometryType = type;}

    virtual bool testGeometryMode(int flag) const override { return GeometryModeFlags.test((size_t)(flag)); };
    virtual void setGeometryMode(int flag, bool v=true) override { GeometryModeFlags.set((size_t)(flag), v); };

    virtual int getGeometryLayerId() const override { return GeometryLayer;}
    virtual void setGeometryLayerId(int geolayer) override { GeometryLayer = geolayer;}

    constexpr static std::array<const char *,InternalType::NumInternalGeometryType> internaltype2str {{ "None", "EllipseMajorDiameter", "EllipseMinorDiameter","EllipseFocus1", "EllipseFocus2", "HyperbolaMajor", "HyperbolaMinor", "HyperbolaFocus", "ParabolaFocus", "BSplineControlPoint", "BSplineKnotPoint" }};

    constexpr static std::array<const char *,GeometryMode::NumGeometryMode> geometrymode2str {{ "Blocked", "Construction" }};

    static bool getInternalTypeFromName(std::string str, InternalType::InternalType &type);

    static bool getGeometryModeFromName(std::string str, GeometryMode::GeometryMode &type);

protected:
    virtual void copyAttributes(Part::GeometryExtension * cpy) const override;
    virtual void restoreAttributes(Base::XMLReader &reader) override;
    virtual void saveAttributes(Base::Writer &writer) const override;

private:
    SketchGeometryExtension(const SketchGeometryExtension&) = default;

private:
    using GeometryModeFlagType = std::bitset<32>;
    long                          Id;
    InternalType::InternalType    InternalGeometryType;
    GeometryModeFlagType          GeometryModeFlags;
    int                           GeometryLayer;

private:
    static std::atomic<long> _GeometryID;
};

} //namespace Sketcher


#endif // SKETCHER_SKETCHGEOMETRYEXTENSION_H
