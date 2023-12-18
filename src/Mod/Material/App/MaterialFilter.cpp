/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include <App/Application.h>

#include "MaterialFilter.h"
#include "Materials.h"


using namespace Materials;

TYPESYSTEM_SOURCE(Materials::MaterialFilter, Base::BaseClass)

MaterialFilter::MaterialFilter()
    : _includeFolders(true)
    , _includeLegacy(true)
    , _required()
    , _requiredComplete()
{}

bool MaterialFilter::modelIncluded(const std::shared_ptr<Material>& material) const
{
    for (const auto& complete : _requiredComplete) {
        if (!material->isModelComplete(complete)) {
            return false;
        }
    }
    for (const auto& required : _required) {
        if (!material->hasModel(required)) {
            return false;
        }
    }

    return true;
}

void MaterialFilter::addRequired(const QString& uuid)
{
    // Ignore any uuids already present
    if (!_requiredComplete.contains(uuid)) {
        _required.insert(uuid);
    }
}

void MaterialFilter::addRequiredComplete(const QString& uuid)
{
    if (_required.contains(uuid)) {
        // Completeness takes priority
        _required.remove(uuid);
    }
    _requiredComplete.insert(uuid);
}
