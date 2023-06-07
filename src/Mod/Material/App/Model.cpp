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

#include "PreCompiled.h"
#ifndef _PreComp_
#endif

#include "Model.h"


using namespace Material;

TYPESYSTEM_SOURCE(Material::ModelProperty, Base::BaseClass)

ModelProperty::ModelProperty()
{

}

ModelProperty::ModelProperty(const std::string& name, const std::string& type,
                        const std::string& units, const std::string& url,
                        const std::string& description):
    _name(name), _type(type), _units(units), _url(url), _description(description)
{

}

ModelProperty::~ModelProperty()
{

}

TYPESYSTEM_SOURCE(Material::Model, Base::BaseClass)

Model::Model()
{}

Model::Model(ModelType type, const std::string &name, const QDir &directory, 
        const std::string &uuid, const std::string& description, const std::string& url,
        const std::string& doi):
    _type(type), _name(name), _directory(directory), _uuid(uuid), _description(description),
    _url(url), _doi(doi)
{}

Model::~Model()
{}

TYPESYSTEM_SOURCE(Material::ModelLibrary, Base::BaseClass)

ModelLibrary::ModelLibrary(const std::string &libraryName, const QDir &dir, const std::string &icon):
    name(libraryName), directory(dir), iconPath(icon)
{}
ModelLibrary::ModelLibrary()
{}

ModelLibrary::~ModelLibrary()
{}

#include "moc_Model.cpp"
