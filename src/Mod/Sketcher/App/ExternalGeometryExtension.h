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
#include <array>

namespace Sketcher
{

class SketcherExport ExternalGeometryExtension : public Part::GeometryExtension
{
    TYPESYSTEM_HEADER();
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

    ExternalGeometryExtension() = default;
    virtual ~ExternalGeometryExtension() override = default;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const override;
    virtual void Save(Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    // START_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
    bool testFlag(int flag) const { return Flags.test((size_t)(flag)); }
    void setFlag(int flag, bool v=true) { Flags.set((size_t)(flag),v); }
    // END_CREDIT_BLOCK: Credit under LGPL for this block to Zheng, Lei (realthunder) <realthunder.dev@gmail.com>

    bool isClear() const {return Flags.none();}
    size_t flagSize() const {return Flags.size();}

    const std::string& getRef() const {return Ref;}
    void setRef(const std::string & ref) {Ref = ref;}

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
