// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <utility>

#include "Exceptions.h"

namespace Materials
{

Uninitialized::Uninitialized(std::string msg) : Base::Exception(std::move(msg)) {}
Uninitialized::~Uninitialized() noexcept = default;

ModelNotFound::ModelNotFound(std::string msg) : Base::Exception(std::move(msg)) {}
ModelNotFound::~ModelNotFound() noexcept = default;

InvalidMaterialType::InvalidMaterialType(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidMaterialType::~InvalidMaterialType() noexcept = default;

MaterialNotFound::MaterialNotFound(std::string msg) : Base::Exception(std::move(msg)) {}
MaterialNotFound::~MaterialNotFound() noexcept = default;

MaterialExists::MaterialExists(std::string msg) : Base::Exception(std::move(msg)) {}
MaterialExists::~MaterialExists() noexcept = default;

MaterialReadError::MaterialReadError(std::string msg) : Base::Exception(std::move(msg)) {}
MaterialReadError::~MaterialReadError() noexcept = default;

PropertyNotFound::PropertyNotFound(std::string msg) : Base::Exception(std::move(msg)) {}
PropertyNotFound::~PropertyNotFound() noexcept = default;

LibraryNotFound::LibraryNotFound(std::string msg) : Base::Exception(std::move(msg)) {}
LibraryNotFound::~LibraryNotFound() noexcept = default;

CreationError::CreationError(std::string msg) : Base::Exception(std::move(msg)) {}
CreationError::~CreationError() noexcept = default;

InvalidModel::InvalidModel(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidModel::~InvalidModel() noexcept = default;

InvalidMaterial::InvalidMaterial(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidMaterial::~InvalidMaterial() noexcept = default;

InvalidProperty::InvalidProperty(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidProperty::~InvalidProperty() noexcept = default;

InvalidLibrary::InvalidLibrary(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidLibrary::~InvalidLibrary() noexcept = default;

InvalidIndex::InvalidIndex(std::string msg) : Base::Exception(std::move(msg)) {}
InvalidIndex::~InvalidIndex() noexcept = default;

UnknownValueType::UnknownValueType(std::string msg) : Base::Exception(std::move(msg)) {}
UnknownValueType::~UnknownValueType() noexcept = default;

DeleteError::DeleteError(std::string msg) : Base::Exception(std::move(msg)) {}
DeleteError::~DeleteError() noexcept = default;

RenameError::RenameError(std::string msg) : Base::Exception(std::move(msg)) {}
RenameError::~RenameError() noexcept = default;

ReplacementError::ReplacementError(std::string msg) : Base::Exception(std::move(msg)) {}
ReplacementError::~ReplacementError() noexcept = default;

UpdateError::UpdateError(std::string msg) : Base::Exception(std::move(msg)) {}
UpdateError::~UpdateError() noexcept = default;

MoveError::MoveError(std::string msg) : Base::Exception(std::move(msg)) {}
MoveError::~MoveError() noexcept = default;

ConnectionError::ConnectionError(std::string msg) : Base::Exception(std::move(msg)) {}
ConnectionError::~ConnectionError() noexcept = default;

} // namespace Materials
