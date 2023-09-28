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

#include <Base/BaseClass.h>
#include <Base/Exception.h>

namespace Materials
{

class Uninitialized: public Base::Exception
{
public:
    Uninitialized()
    {}
    explicit Uninitialized(const char* msg)
    {
        this->setMessage(msg);
    }
    ~Uninitialized() noexcept override = default;
};

class ModelNotFound: public Base::Exception
{
public:
    ModelNotFound()
    {}
    explicit ModelNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    ~ModelNotFound() noexcept override = default;
};

class MaterialNotFound: public Base::Exception
{
public:
    MaterialNotFound()
    {}
    explicit MaterialNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    ~MaterialNotFound() noexcept override = default;
};

class PropertyNotFound: public Base::Exception
{
public:
    PropertyNotFound()
    {}
    explicit PropertyNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    ~PropertyNotFound() noexcept override = default;
};

class LibraryNotFound: public Base::Exception
{
public:
    LibraryNotFound()
    {}
    explicit LibraryNotFound(const char* msg)
    {
        this->setMessage(msg);
    }
    ~LibraryNotFound() noexcept override = default;
};

class InvalidModel: public Base::Exception
{
public:
    InvalidModel()
    {}
    explicit InvalidModel(const char* msg)
    {
        this->setMessage(msg);
    }
    ~InvalidModel() noexcept override = default;
};

class InvalidRow: public Base::Exception
{
public:
    InvalidRow()
    {}
    explicit InvalidRow(char* msg)
    {
        this->setMessage(msg);
    }
    ~InvalidRow() noexcept override = default;
};

class InvalidColumn: public Base::Exception
{
public:
    InvalidColumn()
    {}
    explicit InvalidColumn(char* msg)
    {
        this->setMessage(msg);
    }
    ~InvalidColumn() noexcept override = default;
};

class InvalidIndex: public Base::Exception
{
public:
    InvalidIndex()
    {}
    explicit InvalidIndex(char* msg)
    {
        this->setMessage(msg);
    }
    ~InvalidIndex() noexcept override = default;
};

class UnknownValueType: public Base::Exception
{
public:
    UnknownValueType()
    {}
    explicit UnknownValueType(char* msg)
    {
        this->setMessage(msg);
    }
    ~UnknownValueType() noexcept override = default;
};

}  // namespace Materials

#endif  // MATERIAL_EXCEPTIONS_H
