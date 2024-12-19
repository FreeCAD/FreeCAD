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
    {
        this->setMessage("Uninitalized");
    }
    explicit Uninitialized(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit Uninitialized(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~Uninitialized() noexcept override = default;
};

class ModelNotFound: public Base::Exception
{
public:
    ModelNotFound()
    {
        this->setMessage("Model not found");
    }
    explicit ModelNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit ModelNotFound(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~ModelNotFound() noexcept override = default;
};

class InvalidMaterialType: public Base::Exception
{
public:
    InvalidMaterialType()
    {
        this->setMessage("Invalid material type");
    }
    explicit InvalidMaterialType(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidMaterialType(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidMaterialType() noexcept override = default;
};

class MaterialNotFound: public Base::Exception
{
public:
    MaterialNotFound()
    {
        this->setMessage("Material not found");
    }
    explicit MaterialNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit MaterialNotFound(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~MaterialNotFound() noexcept override = default;
};

class MaterialExists: public Base::Exception
{
public:
    MaterialExists()
    {
        this->setMessage("Material already exxists");
    }
    explicit MaterialExists(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit MaterialExists(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~MaterialExists() noexcept override = default;
};

class MaterialReadError: public Base::Exception
{
public:
    MaterialReadError()
    {
        this->setMessage("Unable to read material");
    }
    explicit MaterialReadError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit MaterialReadError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~MaterialReadError() noexcept override = default;
};

class PropertyNotFound: public Base::Exception
{
public:
    PropertyNotFound()
    {
        this->setMessage("Property not found");
    }
    explicit PropertyNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit PropertyNotFound(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~PropertyNotFound() noexcept override = default;
};

class LibraryNotFound: public Base::Exception
{
public:
    LibraryNotFound()
    {
        this->setMessage("Library not found");
    }
    explicit LibraryNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit LibraryNotFound(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~LibraryNotFound() noexcept override = default;
};

class CreationError: public Base::Exception
{
public:
    CreationError()
    {
        this->setMessage("Unable to create object");
    }
    explicit CreationError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit CreationError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~CreationError() noexcept override = default;
};

class InvalidModel: public Base::Exception
{
public:
    InvalidModel()
    {
        this->setMessage("Invalid model");
    }
    explicit InvalidModel(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidModel(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidModel() noexcept override = default;
};

class InvalidMaterial: public Base::Exception
{
public:
    InvalidMaterial()
    {
        this->setMessage("Invalid material");
    }
    explicit InvalidMaterial(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidMaterial(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidMaterial() noexcept override = default;
};

class InvalidProperty: public Base::Exception
{
public:
    InvalidProperty()
    {
        this->setMessage("Invalid property");
    }
    explicit InvalidProperty(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidProperty(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidProperty() noexcept override = default;
};

class InvalidLibrary: public Base::Exception
{
public:
    InvalidLibrary()
    {
        this->setMessage("Invalid library");
    }
    explicit InvalidLibrary(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidLibrary(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidLibrary() noexcept override = default;
};

class InvalidIndex: public Base::Exception
{
public:
    InvalidIndex()
    {
        this->setMessage("Invalid index");
    }
    explicit InvalidIndex(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit InvalidIndex(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~InvalidIndex() noexcept override = default;
};

class UnknownValueType: public Base::Exception
{
public:
    UnknownValueType()
    {
        this->setMessage("Unkown value type");
    }
    explicit UnknownValueType(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit UnknownValueType(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~UnknownValueType() noexcept override = default;
};

class DeleteError: public Base::Exception
{
public:
    DeleteError()
    {
        this->setMessage("Unable to delete object");
    }
    explicit DeleteError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit DeleteError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~DeleteError() noexcept override = default;
};

class RenameError: public Base::Exception
{
public:
    RenameError()
    {
        this->setMessage("Unable to rename object");
    }
    explicit RenameError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit RenameError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~RenameError() noexcept override = default;
};

class ReplacementError: public Base::Exception
{
public:
    ReplacementError()
    {
        this->setMessage("Unable to replace object");
    }
    explicit ReplacementError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit ReplacementError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~ReplacementError() noexcept override = default;
};

class ConnectionError: public Base::Exception
{
public:
    ConnectionError()
    {
        this->setMessage("Unable to connect");
    }
    explicit ConnectionError(const char* msg)
    {
        this->setMessage(msg);
    }
    explicit ConnectionError(const QString& msg)
    {
        this->setMessage(msg.toStdString().c_str());
    }
    ~ConnectionError() noexcept override = default;
};

}  // namespace Materials

#endif  // MATERIAL_EXCEPTIONS_H
