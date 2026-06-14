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

#include <QString>

#include <Base/BaseClass.h>
#include <Base/Exception.h>

namespace Materials
{

class Uninitialized: public Base::Exception
{
public:
    Uninitialized();
    explicit Uninitialized(const char* msg);
    explicit Uninitialized(const QString& msg);
    ~Uninitialized() noexcept override;
};

class ModelNotFound: public Base::Exception
{
public:
    ModelNotFound();
    explicit ModelNotFound(const char* msg);
    explicit ModelNotFound(const QString& msg);
    ~ModelNotFound() noexcept override;
};

class InvalidMaterialType: public Base::Exception
{
public:
    InvalidMaterialType();
    explicit InvalidMaterialType(const char* msg);
    explicit InvalidMaterialType(const QString& msg);
    ~InvalidMaterialType() noexcept override;
};

class MaterialNotFound: public Base::Exception
{
public:
    MaterialNotFound();
    explicit MaterialNotFound(const char* msg);
    explicit MaterialNotFound(const QString& msg);
    ~MaterialNotFound() noexcept override;
};

class MaterialExists: public Base::Exception
{
public:
    MaterialExists();
    explicit MaterialExists(const char* msg);
    explicit MaterialExists(const QString& msg);
    ~MaterialExists() noexcept override;
};

class MaterialReadError: public Base::Exception
{
public:
    MaterialReadError();
    explicit MaterialReadError(const char* msg);
    explicit MaterialReadError(const QString& msg);
    ~MaterialReadError() noexcept override;
};

class PropertyNotFound: public Base::Exception
{
public:
    PropertyNotFound();
    explicit PropertyNotFound(const char* msg);
    explicit PropertyNotFound(const QString& msg);
    ~PropertyNotFound() noexcept override;
};

class LibraryNotFound: public Base::Exception
{
public:
    LibraryNotFound();
    explicit LibraryNotFound(const char* msg);
    explicit LibraryNotFound(const QString& msg);
    ~LibraryNotFound() noexcept override;
};

class CreationError: public Base::Exception
{
public:
    CreationError();
    explicit CreationError(const char* msg);
    explicit CreationError(const QString& msg);
    ~CreationError() noexcept override;
};

class InvalidModel: public Base::Exception
{
public:
    InvalidModel();
    explicit InvalidModel(const char* msg);
    explicit InvalidModel(const QString& msg);
    ~InvalidModel() noexcept override;
};

class InvalidMaterial: public Base::Exception
{
public:
    InvalidMaterial();
    explicit InvalidMaterial(const char* msg);
    explicit InvalidMaterial(const QString& msg);
    ~InvalidMaterial() noexcept override;
};

class InvalidProperty: public Base::Exception
{
public:
    InvalidProperty();
    explicit InvalidProperty(const char* msg);
    explicit InvalidProperty(const QString& msg);
    ~InvalidProperty() noexcept override;
};

class InvalidLibrary: public Base::Exception
{
public:
    InvalidLibrary();
    explicit InvalidLibrary(const char* msg);
    explicit InvalidLibrary(const QString& msg);
    ~InvalidLibrary() noexcept override;
};

class InvalidIndex: public Base::Exception
{
public:
    InvalidIndex();
    explicit InvalidIndex(const char* msg);
    explicit InvalidIndex(const QString& msg);
    ~InvalidIndex() noexcept override;
};

class UnknownValueType: public Base::Exception
{
public:
    UnknownValueType();
    explicit UnknownValueType(const char* msg);
    explicit UnknownValueType(const QString& msg);
    ~UnknownValueType() noexcept override;
};

class DeleteError: public Base::Exception
{
public:
    DeleteError();
    explicit DeleteError(const char* msg);
    explicit DeleteError(const QString& msg);
    ~DeleteError() noexcept override;
};

class RenameError: public Base::Exception
{
public:
    RenameError();
    explicit RenameError(const char* msg);
    explicit RenameError(const QString& msg);
    ~RenameError() noexcept override;
};

class ReplacementError: public Base::Exception
{
public:
    ReplacementError();
    explicit ReplacementError(const char* msg);
    explicit ReplacementError(const QString& msg);
    ~ReplacementError() noexcept override;
};

class UpdateError: public Base::Exception
{
public:
    UpdateError();
    explicit UpdateError(const char* msg);
    explicit UpdateError(const QString& msg);
    ~UpdateError() noexcept override;
};

class MoveError: public Base::Exception
{
public:
    MoveError();
    explicit MoveError(const char* msg);
    explicit MoveError(const QString& msg);
    ~MoveError() noexcept override;
};

class ConnectionError: public Base::Exception
{
public:
    ConnectionError();
    explicit ConnectionError(const char* msg);
    explicit ConnectionError(const QString& msg);
    ~ConnectionError() noexcept override;
};

}  // namespace Materials