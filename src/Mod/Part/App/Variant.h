/***************************************************************************
 *   Copyright (c) 2025 Pieter Hijma <info@pieterhijma.net>                *
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

#ifndef PART_VARIANT_H
#define PART_VARIANT_H

#include <App/VariantExtension.h>

#include "PartFeature.h"


namespace Part
{

class PartExport Variant : public Part::Feature, public App::VariantExtension
{
    PROPERTY_HEADER_WITH_OVERRIDE(Part::Variant);

public:
    Variant();

    const char* getViewProviderName() const override;
};

} // namespace Part


#endif // PART_VARIANT_H
