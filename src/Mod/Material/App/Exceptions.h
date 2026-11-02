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

#pragma once

#include <string>

#include <Base/BaseClass.h>
#include <Base/Exception.h>

namespace Materials
{

class Uninitialized: public Base::Exception
{
public:
    explicit Uninitialized(std::string msg = "Uninitialized");
    ~Uninitialized() noexcept override;
};

class ModelNotFound: public Base::Exception
{
public:
    explicit ModelNotFound(std::string msg = "Model not found");
    ~ModelNotFound() noexcept override;
};

class InvalidMaterialType: public Base::Exception
{
public:
    explicit InvalidMaterialType(std::string msg = "Invalid material type");
    ~InvalidMaterialType() noexcept override;
};

class MaterialNotFound: public Base::Exception
{
public:
    explicit MaterialNotFound(std::string msg = "Material not found");
    ~MaterialNotFound() noexcept override;
};

class MaterialExists: public Base::Exception
{
public:
    explicit MaterialExists(std::string msg = "Material already exists");
    ~MaterialExists() noexcept override;
};

class MaterialReadError: public Base::Exception
{
public:
    explicit MaterialReadError(std::string msg = "Unable to read material");
    ~MaterialReadError() noexcept override;
};

class PropertyNotFound: public Base::Exception
{
public:
    explicit PropertyNotFound(std::string msg = "Property not found");
    ~PropertyNotFound() noexcept override;
};

class LibraryNotFound: public Base::Exception
{
public:
    explicit LibraryNotFound(std::string msg = "Library not found");
    ~LibraryNotFound() noexcept override;
};

class CreationError: public Base::Exception
{
public:
    explicit CreationError(std::string msg = "Unable to create object");
    ~CreationError() noexcept override;
};

class InvalidModel: public Base::Exception
{
public:
    explicit InvalidModel(std::string msg = "Invalid model");
    ~InvalidModel() noexcept override;
};

class InvalidMaterial: public Base::Exception
{
public:
    explicit InvalidMaterial(std::string msg = "Invalid material");
    ~InvalidMaterial() noexcept override;
};

class InvalidProperty: public Base::Exception
{
public:
    explicit InvalidProperty(std::string msg = "Invalid property");
    ~InvalidProperty() noexcept override;
};

class InvalidLibrary: public Base::Exception
{
public:
    explicit InvalidLibrary(std::string msg = "Invalid library");
    ~InvalidLibrary() noexcept override;
};

class InvalidIndex: public Base::Exception
{
public:
    explicit InvalidIndex(std::string msg = "Invalid index");
    ~InvalidIndex() noexcept override;
};

class UnknownValueType: public Base::Exception
{
public:
    explicit UnknownValueType(std::string msg = "Unknown value type");
    ~UnknownValueType() noexcept override;
};

class DeleteError: public Base::Exception
{
public:
    explicit DeleteError(std::string msg = "Unable to delete object");
    ~DeleteError() noexcept override;
};

class RenameError: public Base::Exception
{
public:
    explicit RenameError(std::string msg = "Unable to rename object");
    ~RenameError() noexcept override;
};

class ReplacementError: public Base::Exception
{
public:
    explicit ReplacementError(std::string msg = "Unable to replace object");
    ~ReplacementError() noexcept override;
};

class UpdateError: public Base::Exception
{
public:
    explicit UpdateError(std::string msg = "Unable to update object");
    ~UpdateError() noexcept override;
};

class MoveError: public Base::Exception
{
public:
    explicit MoveError(std::string msg = "Unable to move object");
    ~MoveError() noexcept override;
};

class ConnectionError: public Base::Exception
{
public:
    explicit ConnectionError(std::string msg = "Unable to connect");
    ~ConnectionError() noexcept override;
};

}  // namespace Materials