/***************************************************************************
 *   Copyright (c) 2011 Jürgen Riegel <juergen.riegel@web.de>              *
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
#include <cassert>
#endif

#include "Type.h"
#include "Interpreter.h"
#include "Console.h"


using namespace Base;


struct Base::TypeData
{
    TypeData(const char* theName,
             const Type type = Type::badType(),
             const Type theParent = Type::badType(),
             Type::instantiationMethod method = nullptr)
        : name(theName)
        , parent(theParent)
        , type(type)
        , instMethod(method)
    {}

    std::string name;
    Type parent;
    Type type;
    Type::instantiationMethod instMethod;
};

std::map<std::string, unsigned int> Type::typemap;
std::vector<TypeData*> Type::typedata;
std::set<std::string> Type::loadModuleSet;

void* Type::createInstance() const
{
    instantiationMethod method = typedata[index]->instMethod;
    return method ? (*method)() : nullptr;
}

bool Type::canInstantiate() const
{
    instantiationMethod method = typedata[index]->instMethod;
    return method != nullptr;
}

void* Type::createInstanceByName(const char* TypeName, bool bLoadModule)
{
    // if not already, load the module
    if (bLoadModule) {
        importModule(TypeName);
    }

    // now the type should be in the type map
    Type type = fromName(TypeName);
    if (type == badType()) {
        return nullptr;
    }

    return type.createInstance();
}

void Type::importModule(const char* TypeName)
{
    // cut out the module name
    const std::string mod = getModuleName(TypeName);

    // ignore base modules
    if (mod == "App" || mod == "Gui" || mod == "Base") {
        return;
    }

    // remember already loaded modules
    const auto pos = loadModuleSet.find(mod);
    if (pos != loadModuleSet.end()) {
        return;
    }

    // lets load the module
    Interpreter().loadModule(mod.c_str());
#ifdef FC_LOGLOADMODULE
    Console().Log("Act: Module %s loaded through class %s \n", Mod.c_str(), TypeName);
#endif
    loadModuleSet.insert(mod);
}

std::string Type::getModuleName(const char* ClassName)
{
    std::string_view classNameView(ClassName);
    auto pos = classNameView.find("::");

    return pos != std::string_view::npos ? std::string(classNameView.substr(0, pos))
                                         : std::string();
}

Type Type::badType()
{
    Type bad;
    bad.index = 0;
    return bad;
}


Type Type::createType(const Type& parent, const char* name, instantiationMethod method)
{
    Type newType;
    newType.index = static_cast<unsigned int>(Type::typedata.size());
    TypeData* typeData = new TypeData(name, newType, parent, method);
    Type::typedata.push_back(typeData);

    // add to dictionary for fast lookup
    Type::typemap[name] = newType.getKey();

    return newType;
}


void Type::init()
{
    assert(Type::typedata.empty());


    Type::typedata.push_back(new TypeData("BadType"));
    Type::typemap["BadType"] = 0;
}

void Type::destruct()
{
    for (auto it : typedata) {
        delete it;
    }
    typedata.clear();
    typemap.clear();
    loadModuleSet.clear();
}

Type Type::fromName(const char* name)
{
    std::map<std::string, unsigned int>::const_iterator pos;

    pos = typemap.find(name);
    if (pos != typemap.end()) {
        return typedata[pos->second]->type;
    }

    return Type::badType();
}

Type Type::fromKey(unsigned int key)
{
    if (key < typedata.size()) {
        return typedata[key]->type;
    }

    return Type::badType();
}

const char* Type::getName() const
{
    return typedata[index]->name.c_str();
}

Type Type::getParent() const
{
    return typedata[index]->parent;
}

bool Type::isDerivedFrom(const Type& type) const
{

    Type temp(*this);
    do {
        if (temp == type) {
            return true;
        }
        temp = temp.getParent();
    } while (temp != badType());

    return false;
}

int Type::getAllDerivedFrom(const Type& type, std::vector<Type>& List)
{
    int cnt = 0;

    for (auto it : typedata) {
        if (it->type.isDerivedFrom(type)) {
            List.push_back(it->type);
            cnt++;
        }
    }
    return cnt;
}

int Type::getNumTypes()
{
    return static_cast<int>(typedata.size());
}

Type Type::getTypeIfDerivedFrom(const char* name, const Type& parent, bool bLoadModule)
{
    if (bLoadModule) {
        importModule(name);
    }

    Type type = fromName(name);

    if (type.isDerivedFrom(parent)) {
        return type;
    }

    return Type::badType();
}
