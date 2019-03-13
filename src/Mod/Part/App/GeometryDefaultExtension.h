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


#ifndef PART_GEOMETRYDEFAULTEXTENSION_H
#define PART_GEOMETRYDEFAULTEXTENSION_H

#include <string>
#include "GeometryExtension.h"

namespace Part {

    template <typename T>
    class PartExport GeometryDefaultExtension: public Part::GeometryExtension
    {
        TYPESYSTEM_HEADER();
    public:
        inline GeometryDefaultExtension();
        GeometryDefaultExtension(const T& val, std::string name = std::string());
        virtual ~GeometryDefaultExtension() override = default;

        inline void setValue(const T& val) {value = val;};
        inline const T &getValue() const {return value;};

        // Persistence implementer ---------------------
        virtual unsigned int getMemSize(void) const override;
        virtual void Save(Base::Writer &/*writer*/) const override;
        virtual void Restore(Base::XMLReader &/*reader*/) override;

        virtual std::unique_ptr<Part::GeometryExtension> copy(void) const override;

        virtual PyObject *getPyObject(void) override;

    private:
        GeometryDefaultExtension(const GeometryDefaultExtension<T>&) = default;

    private:
        T value;
    };

    // Description:
    //
    // This template allows to define a geometry extension for a given type (uniform interface for one value of type T).
    //
    // Warnings:
    // - The default constructor relies on the default constructor of T for initialisation. Built-in types
    //   so constructed will be uninitialised. Use the specific constructor from a T to initiliase it. Note
    //   that the default constructor is required by the type system (see TYPESYSTEM_SOURCE_TEMPLATE_T).
    //
    // Default assumptions:
    // - T can be constructed from T
    // - T can be assigned to T
    // - T is convertible to a std::string
    // - T is serialisable as a string
    //
    // template specialisation:
    //
    // If the assumptions do not meet for your type, template specialisation allows you to provide specific code,
    // look for examples (int/string) in GeometryDefaultExtensions.cpp
    //
    // Instructions:
    //
    // 1. Read the assumptions above and provide template initilisation if needed.
    // 2. Add an alias to your type under these comments
    // 3. Add a TYPESYSTEM_SOURCE_TEMPLATE_T in the cpp file to generate class type information
    // 4. Provide a specialisation of getPyObject to generate a py object of the corresponding type (cpp file)
    // 5. Provide specialisations if your type does not meet the assumptions above (e.g. for serialisation) (cpp file)
    // 6. Register your type and corresponding python type in AppPart.cpp

    template <typename T>
    inline GeometryDefaultExtension<T>::GeometryDefaultExtension(){ }

    // Specialised constructors go here so that specialisation is before the template instantiation
    // Specialised default constructors are inline, because a full specialisation otherwise shall go in the cpp file, but there it would be after the template instantiation.
    template <>
    inline GeometryDefaultExtension<long>::GeometryDefaultExtension():value(0){}

    template <>
    inline GeometryDefaultExtension<double>::GeometryDefaultExtension():value(0.0f){}

    // Prefer alias to typedef item 9
    using GeometryIntExtension = GeometryDefaultExtension<long>;
    using GeometryStringExtension = GeometryDefaultExtension<std::string>;
    using GeometryBoolExtension = GeometryDefaultExtension<bool>;
    using GeometryDoubleExtension = GeometryDefaultExtension<double>;
}

#endif // PART_GEOMETRYDEFAULTEXTENSION_H
