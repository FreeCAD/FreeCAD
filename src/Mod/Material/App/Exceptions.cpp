// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#include "Exceptions.h"

namespace Materials
{

Uninitialized::Uninitialized() : Base::Exception("Uninitialized") {}
Uninitialized::Uninitialized(const char* msg) : Base::Exception(msg) {}
Uninitialized::Uninitialized(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
Uninitialized::~Uninitialized() noexcept = default;

ModelNotFound::ModelNotFound() : Base::Exception("Model not found") {}
ModelNotFound::ModelNotFound(const char* msg) : Base::Exception(msg) {}
ModelNotFound::ModelNotFound(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
ModelNotFound::~ModelNotFound() noexcept = default;

InvalidMaterialType::InvalidMaterialType() : Base::Exception("Invalid material type") {}
InvalidMaterialType::InvalidMaterialType(const char* msg) : Base::Exception(msg) {}
InvalidMaterialType::InvalidMaterialType(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidMaterialType::~InvalidMaterialType() noexcept = default;

MaterialNotFound::MaterialNotFound() : Base::Exception("Material not found") {}
MaterialNotFound::MaterialNotFound(const char* msg) : Base::Exception(msg) {}
MaterialNotFound::MaterialNotFound(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
MaterialNotFound::~MaterialNotFound() noexcept = default;

MaterialExists::MaterialExists() : Base::Exception("Material already exists") {}
MaterialExists::MaterialExists(const char* msg) : Base::Exception(msg) {}
MaterialExists::MaterialExists(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
MaterialExists::~MaterialExists() noexcept = default;

MaterialReadError::MaterialReadError() : Base::Exception("Unable to read material") {}
MaterialReadError::MaterialReadError(const char* msg) : Base::Exception(msg) {}
MaterialReadError::MaterialReadError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
MaterialReadError::~MaterialReadError() noexcept = default;

PropertyNotFound::PropertyNotFound() : Base::Exception("Property not found") {}
PropertyNotFound::PropertyNotFound(const char* msg) : Base::Exception(msg) {}
PropertyNotFound::PropertyNotFound(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
PropertyNotFound::~PropertyNotFound() noexcept = default;

LibraryNotFound::LibraryNotFound() : Base::Exception("Library not found") {}
LibraryNotFound::LibraryNotFound(const char* msg) : Base::Exception(msg) {}
LibraryNotFound::LibraryNotFound(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
LibraryNotFound::~LibraryNotFound() noexcept = default;

CreationError::CreationError() : Base::Exception("Unable to create object") {}
CreationError::CreationError(const char* msg) : Base::Exception(msg) {}
CreationError::CreationError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
CreationError::~CreationError() noexcept = default;

InvalidModel::InvalidModel() : Base::Exception("Invalid model") {}
InvalidModel::InvalidModel(const char* msg) : Base::Exception(msg) {}
InvalidModel::InvalidModel(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidModel::~InvalidModel() noexcept = default;

InvalidMaterial::InvalidMaterial() : Base::Exception("Invalid material") {}
InvalidMaterial::InvalidMaterial(const char* msg) : Base::Exception(msg) {}
InvalidMaterial::InvalidMaterial(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidMaterial::~InvalidMaterial() noexcept = default;

InvalidProperty::InvalidProperty() : Base::Exception("Invalid property") {}
InvalidProperty::InvalidProperty(const char* msg) : Base::Exception(msg) {}
InvalidProperty::InvalidProperty(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidProperty::~InvalidProperty() noexcept = default;

InvalidLibrary::InvalidLibrary() : Base::Exception("Invalid library") {}
InvalidLibrary::InvalidLibrary(const char* msg) : Base::Exception(msg) {}
InvalidLibrary::InvalidLibrary(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidLibrary::~InvalidLibrary() noexcept = default;

InvalidIndex::InvalidIndex() : Base::Exception("Invalid index") {}
InvalidIndex::InvalidIndex(const char* msg) : Base::Exception(msg) {}
InvalidIndex::InvalidIndex(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
InvalidIndex::~InvalidIndex() noexcept = default;

UnknownValueType::UnknownValueType() : Base::Exception("Unknown value type") {}
UnknownValueType::UnknownValueType(const char* msg) : Base::Exception(msg) {}
UnknownValueType::UnknownValueType(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
UnknownValueType::~UnknownValueType() noexcept = default;

DeleteError::DeleteError() : Base::Exception("Unable to delete object") {}
DeleteError::DeleteError(const char* msg) : Base::Exception(msg) {}
DeleteError::DeleteError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
DeleteError::~DeleteError() noexcept = default;

RenameError::RenameError() : Base::Exception("Unable to rename object") {}
RenameError::RenameError(const char* msg) : Base::Exception(msg) {}
RenameError::RenameError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
RenameError::~RenameError() noexcept = default;

ReplacementError::ReplacementError() : Base::Exception("Unable to replace object") {}
ReplacementError::ReplacementError(const char* msg) : Base::Exception(msg) {}
ReplacementError::ReplacementError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
ReplacementError::~ReplacementError() noexcept = default;

UpdateError::UpdateError() : Base::Exception("Unable to update object") {}
UpdateError::UpdateError(const char* msg) : Base::Exception(msg) {}
UpdateError::UpdateError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
UpdateError::~UpdateError() noexcept = default;

MoveError::MoveError() : Base::Exception("Unable to move object") {}
MoveError::MoveError(const char* msg) : Base::Exception(msg) {}
MoveError::MoveError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
MoveError::~MoveError() noexcept = default;

ConnectionError::ConnectionError() : Base::Exception("Unable to connect") {}
ConnectionError::ConnectionError(const char* msg) : Base::Exception(msg) {}
ConnectionError::ConnectionError(const QString& msg) : Base::Exception(msg.toStdString().c_str()) {}
ConnectionError::~ConnectionError() noexcept = default;

} // namespace Materials
