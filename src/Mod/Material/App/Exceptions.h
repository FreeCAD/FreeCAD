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

#ifndef MATERIAL_EXCEPTIONS_H
#define MATERIAL_EXCEPTIONS_H

#include <QString>

#include <Base/BaseClass.h>
#include <Base/Exception.h>

namespace Materials
{

class Uninitialized: public Base::Exception
{
public:
    Uninitialized()
        : Base::Exception("Uninitialized")
    {}
    explicit Uninitialized(const char* msg)
        : Base::Exception(msg)
    {}
    explicit Uninitialized(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~Uninitialized() noexcept override = default;
};

class ModelNotFound: public Base::Exception
{
public:
    ModelNotFound()
        : Base::Exception("Model not found")
    {}
    explicit ModelNotFound(const char* msg)
        : Base::Exception(msg)
    {}
    explicit ModelNotFound(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~ModelNotFound() noexcept override = default;
};

class InvalidMaterialType: public Base::Exception
{
public:
    InvalidMaterialType()
        : Base::Exception("Invalid material type")
    {}
    explicit InvalidMaterialType(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidMaterialType(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidMaterialType() noexcept override = default;
};

class MaterialNotFound: public Base::Exception
{
public:
    MaterialNotFound()
        : Base::Exception("Material not found")
    {}
    explicit MaterialNotFound(const char* msg)
        : Base::Exception(msg)
    {}
    explicit MaterialNotFound(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~MaterialNotFound() noexcept override = default;
};

class MaterialExists: public Base::Exception
{
public:
    MaterialExists()
        : Base::Exception("Material already exists")
    {}
    explicit MaterialExists(const char* msg)
        : Base::Exception(msg)
    {}
    explicit MaterialExists(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~MaterialExists() noexcept override = default;
};

class MaterialReadError: public Base::Exception
{
public:
    MaterialReadError()
        : Base::Exception("Unable to read material")
    {}
    explicit MaterialReadError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit MaterialReadError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~MaterialReadError() noexcept override = default;
};

class PropertyNotFound: public Base::Exception
{
public:
    PropertyNotFound()
        : Base::Exception("Property not found")
    {}
    explicit PropertyNotFound(const char* msg)
        : Base::Exception(msg)
    {}
    explicit PropertyNotFound(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~PropertyNotFound() noexcept override = default;
};

class LibraryNotFound: public Base::Exception
{
public:
    LibraryNotFound()
        : Base::Exception("Library not found")
    {}
    explicit LibraryNotFound(const char* msg)
        : Base::Exception(msg)
    {}
    explicit LibraryNotFound(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~LibraryNotFound() noexcept override = default;
};

class CreationError: public Base::Exception
{
public:
    CreationError()
        : Base::Exception("Unable to create object")
    {}
    explicit CreationError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit CreationError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~CreationError() noexcept override = default;
};

class InvalidModel: public Base::Exception
{
public:
    InvalidModel()
        : Base::Exception("Invalid model")
    {}
    explicit InvalidModel(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidModel(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidModel() noexcept override = default;
};

class InvalidMaterial: public Base::Exception
{
public:
    InvalidMaterial()
        : Base::Exception("Invalid material")
    {}
    explicit InvalidMaterial(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidMaterial(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidMaterial() noexcept override = default;
};

class InvalidProperty: public Base::Exception
{
public:
    InvalidProperty()
        : Base::Exception("Invalid property")
    {}
    explicit InvalidProperty(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidProperty(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidProperty() noexcept override = default;
};

class InvalidLibrary: public Base::Exception
{
public:
    InvalidLibrary()
        : Base::Exception("Invalid library")
    {}
    explicit InvalidLibrary(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidLibrary(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidLibrary() noexcept override = default;
};

class InvalidIndex: public Base::Exception
{
public:
    InvalidIndex()
        : Base::Exception("Invalid index")
    {}
    explicit InvalidIndex(const char* msg)
        : Base::Exception(msg)
    {}
    explicit InvalidIndex(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~InvalidIndex() noexcept override = default;
};

class UnknownValueType: public Base::Exception
{
public:
    UnknownValueType()
        : Base::Exception("Unknown value type")
    {}
    explicit UnknownValueType(const char* msg)
        : Base::Exception(msg)
    {}
    explicit UnknownValueType(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~UnknownValueType() noexcept override = default;
};

class DeleteError: public Base::Exception
{
public:
    DeleteError()
        : Base::Exception("Unable to delete object")
    {}
    explicit DeleteError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit DeleteError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~DeleteError() noexcept override = default;
};

class RenameError: public Base::Exception
{
public:
    RenameError()
        : Base::Exception("Unable to rename object")
    {}
    explicit RenameError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit RenameError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~RenameError() noexcept override = default;
};

class ReplacementError: public Base::Exception
{
public:
    ReplacementError()
        : Base::Exception("Unable to replace object")
    {}
    explicit ReplacementError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit ReplacementError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~ReplacementError() noexcept override = default;
};

class UpdateError: public Base::Exception
{
public:
    UpdateError()
        : Base::Exception("Unable to update object")
    {}
    explicit UpdateError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit UpdateError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~UpdateError() noexcept override = default;
};

class MoveError: public Base::Exception
{
public:
    MoveError()
        : Base::Exception("Unable to move object")
    {}
    explicit MoveError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit MoveError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~MoveError() noexcept override = default;
};

class ConnectionError: public Base::Exception
{
public:
    ConnectionError()
        : Base::Exception("Unable to connect")
    {}
    explicit ConnectionError(const char* msg)
        : Base::Exception(msg)
    {}
    explicit ConnectionError(const QString& msg)
        : Base::Exception(msg.toStdString().c_str())
    {}
    ~ConnectionError() noexcept override = default;
};

}  // namespace Materials

#endif  // MATERIAL_EXCEPTIONS_H
