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

#ifndef SKETCHER_EXTERNALGEOMETRYEXTENSION_H
#define SKETCHER_EXTERNALGEOMETRYEXTENSION_H

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/SketcherGlobal.h>
#include <array>
#include <bitset>

namespace Sketcher
{

class ISketchExternalGeometryExtension
{
public:
    // Identification information
    // START_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
    virtual bool testFlag(int flag) const = 0;
    virtual void setFlag(int flag, bool v=true) = 0;
    // END_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>

    virtual bool isClear() const = 0;
    virtual size_t flagSize() const = 0;

    virtual const std::string& getRef() const = 0;
    virtual void setRef(const std::string & ref) = 0;
};

class SketcherExport ExternalGeometryExtension : public Part::GeometryPersistenceExtension, private ISketchExternalGeometryExtension
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();
public:
    // START_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
    enum Flag {
        Defining = 0,   // allow an external geometry to build shape
        Frozen = 1,     // freeze an external geometry
        Detached = 2,   // signal the intentions of detaching the geometry from external reference
        Missing = 3,    // geometry with missing external reference
        Sync = 4,       // signal the intention to synchronize a frozen geometry
        NumFlags        // Must be the last type
    };
    // END_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>

    constexpr static std::array<const char *,NumFlags> flag2str {{ "Defining", "Frozen", "Detached","Missing", "Sync" }};
public:

    ExternalGeometryExtension() = default;
    virtual ~ExternalGeometryExtension() override = default;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    // START_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
    virtual bool testFlag(int flag) const override { return Flags.test((size_t)(flag)); }
    virtual void setFlag(int flag, bool v=true) override { Flags.set((size_t)(flag),v); }
    // END_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>

    virtual bool isClear() const override {return Flags.none();}
    virtual size_t flagSize() const override {return Flags.size();}

    virtual const std::string& getRef() const override {return Ref;}
    virtual void setRef(const std::string & ref) override {Ref = ref;}

    static bool getFlagsFromName(std::string str, ExternalGeometryExtension::Flag &flag);

protected:
    virtual void copyAttributes(Part::GeometryExtension * cpy) const override;
    virtual void restoreAttributes(Base::XMLReader &reader) override;
    virtual void saveAttributes(Base::Writer &writer) const override;

private:
    ExternalGeometryExtension(const ExternalGeometryExtension&) = default;

private:
    using FlagType = std::bitset<32>;
    // START_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
    std::string Ref;
    FlagType Flags;
    // END_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>


};

} //namespace Sketcher


#endif // SKETCHER_EXTERNALGEOMETRYEXTENSION_H
