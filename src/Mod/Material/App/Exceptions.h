/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
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

#ifndef MATERIAL_EXCEPTIONS_H
#define MATERIAL_EXCEPTIONS_H

#include <Base/Exception.h>
#include <Base/BaseClass.h>

namespace Materials {


class ModelNotFound : public Base::Exception
{
public:
    ModelNotFound(){}
    explicit ModelNotFound(char* msg){this->setMessage(msg);}
    ~ModelNotFound() throw() override {}
};

class MaterialNotFound : public Base::Exception
{
public:
    MaterialNotFound(){}
    explicit MaterialNotFound(char* msg){this->setMessage(msg);}
    ~MaterialNotFound() throw() override {}
};

class PropertyNotFound : public Base::Exception
{
public:
    PropertyNotFound(){}
    explicit PropertyNotFound(char* msg){this->setMessage(msg);}
    ~PropertyNotFound() throw() override {}
};

class InvalidModel : public Base::Exception
{
public:
    InvalidModel(){}
    explicit InvalidModel(char* msg){this->setMessage(msg);}
    ~InvalidModel() throw() override {}
};

class InvalidRow : public Base::Exception
{
public:
    InvalidRow(){}
    explicit InvalidRow(char* msg){this->setMessage(msg);}
    ~InvalidRow() throw() override {}
};

class InvalidColumn : public Base::Exception
{
public:
    InvalidColumn(){}
    explicit InvalidColumn(char* msg){this->setMessage(msg);}
    ~InvalidColumn() throw() override {}
};

} // namespace Materials

#endif // MATERIAL_EXCEPTIONS_H
