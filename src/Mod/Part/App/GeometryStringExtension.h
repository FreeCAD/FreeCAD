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


#ifndef PART_GEOMETRYSTRINGEXTENSION_H
#define PART_GEOMETRYSTRINGEXTENSION_H

#include <string>
#include "GeometryExtension.h"

namespace Part {

    class PartExport GeometryStringExtension: public Part::GeometryExtension
    {
        TYPESYSTEM_HEADER();
    public:
        GeometryStringExtension();
        GeometryStringExtension(std::string strp);
        virtual ~GeometryStringExtension();

        // Persistence implementer ---------------------
        virtual unsigned int getMemSize(void) const;
        virtual void Save(Base::Writer &/*writer*/) const;
        virtual void Restore(Base::XMLReader &/*reader*/);

        virtual std::unique_ptr<Part::GeometryExtension> copy(void) const;

        virtual PyObject *getPyObject(void);

    public:
        std::string str;
    };

}

#endif // PART_GEOMETRYSTRINGEXTENSION_H
