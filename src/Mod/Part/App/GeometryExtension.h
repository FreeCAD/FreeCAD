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


#ifndef PART_GEOMETRYEXTENSION_H
#define PART_GEOMETRYEXTENSION_H

#include <Base/Persistence.h>
#include <memory>

namespace std {
    template<typename T, typename... Args>
    std::unique_ptr<T> make_unique(Args&&... args)
    {
        return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
    }
}

namespace Part {

class PartExport GeometryExtension: public Base::Persistence
{
    TYPESYSTEM_HEADER();
public:
    virtual ~GeometryExtension();

    // Persistence implementer ---------------------
    virtual unsigned int getMemSize(void) const = 0;
    virtual void Save(Base::Writer &/*writer*/) const = 0;
    virtual void Restore(Base::XMLReader &/*reader*/) = 0;

    virtual std::unique_ptr<GeometryExtension> copy(void) const = 0;

    virtual PyObject *getPyObject(void) = 0;
protected:
    GeometryExtension();
};

}

#endif // PART_GEOMETRYEXTENSION_H
