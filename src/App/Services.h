// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#pragma once

#include "DocumentObject.h"

#include <optional>
#include <Base/Placement.h>

namespace App
{

/**
* This service should provide placement of given sub object (like for example face).
* This feature is not implemented in the core and so it must be provided by module.
*/
class SubObjectPlacementProvider
{
public:
    virtual ~SubObjectPlacementProvider() = default;

    /**
    * Returns placement of sub object relative to the base placement.
    */
    virtual Base::Placement calculate(SubObjectT object, Base::Placement basePlacement) const = 0;
};

/**
* This service should provide center of mass calculation;
*/
class CenterOfMassProvider
{
public:
    virtual ~CenterOfMassProvider() = default;

    virtual bool supports(DocumentObject* object) const = 0;
    virtual std::optional<Base::Vector3d> ofDocumentObject(DocumentObject* object) const = 0;
};

/**
* Default implementation for the center of mass contract
* It always returns empty optional
*/
class NullCenterOfMass final : public CenterOfMassProvider
{
public:
    std::optional<Base::Vector3d> ofDocumentObject(DocumentObject* object) const override;
    bool supports(DocumentObject* object) const override;
};

}