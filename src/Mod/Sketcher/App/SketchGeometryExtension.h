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
#include <atomic>

namespace Sketcher
{

class SketcherExport SketchGeometryExtension : public Part::GeometryExtension
{
    TYPESYSTEM_HEADER();
public:
    SketchGeometryExtension();
    SketchGeometryExtension(long cid);
    virtual ~SketchGeometryExtension() override = default;

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const override;
    virtual void Save(Base::Writer &/*writer*/) const override;
    virtual void Restore(Base::XMLReader &/*reader*/) override;

    virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

    virtual PyObject *getPyObject(void) override;

    long getId() const {return Id;}
    void setId(long id) {Id = id;}

private:
    SketchGeometryExtension(const SketchGeometryExtension&) = default;

private:
    long Id;

private:
    static std::atomic<long> _GeometryID;
};

} //namespace Sketcher


#endif // SKETCHER_SKETCHGEOMETRYEXTENSION_H
