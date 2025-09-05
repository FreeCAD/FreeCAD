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

static_assert(sizeof(Base::Type) == sizeof(Type::TypeId),
              "Base::Type has been designed to be small to be passed around by value efficiently. "
              "The size of Base::Type has changed. Be careful when adding more data members.");

static_assert(
    sizeof(Base::Type) <= 2 * sizeof(void*),
    "Base::Type has been designed to be small to be passed around by value efficiently. "
    "When the size grows larger than ~2 words, consider passing by const reference instead. "
    "Exact limit depends on the architecture and ABI.");

struct Base::TypeData
{
    TypeData(const char* name,
             const Type type,
             const Type parent,
             const Type::instantiationMethod instMethod)
        : name(name)
        , parent(parent)
        , type(type)
        , instMethod(instMethod)
    {}

    const std::string name;
    const Type parent;
    const Type type;
    const Type::instantiationMethod instMethod;
};

namespace
{
constexpr const char* BadTypeName = "BadType";
}

std::map<std::string, unsigned int> Type::typemap;
std::vector<TypeData*> Type::typedata;
std::set<std::string> Type::loadModuleSet;

const Type Type::BadType;

Type::instantiationMethod Type::getInstantiationMethod() const
{
    assert(typedata.size() >= 1 && "Type::init() must be called before creating instances");
    assert(typedata.size() > index && "Type index out of bounds");
    if (isBad() || typedata.size() <= index) {
        return nullptr;
    }
    return typedata[index]->instMethod;
}

void* Type::createInstance() const
{
    const auto method = getInstantiationMethod();
    return method ? (*method)() : nullptr;
}

bool Type::canInstantiate() const
{
    const auto method = getInstantiationMethod();
    return method != nullptr;
}

void* Type::createInstanceByName(const char* typeName, bool loadModule)
{
    // if not already, load the module
    if (loadModule) {
        importModule(typeName);
    }

    // now the type should be in the type map
    const Type type = fromName(typeName);
    // let createInstance handle isBad check
    return type.createInstance();
}

void Type::importModule(const char* typeName)
{
    // cut out the module name
    const std::string mod = getModuleName(typeName);

    // ignore base modules
    if (mod == "App" || mod == "Gui" || mod == "Base") {
        return;
    }

    // remember already loaded modules
    if (loadModuleSet.contains(mod)) {
        return;
    }

    // lets load the module
    Interpreter().loadModule(mod.c_str());
#ifdef FC_LOGLOADMODULE
    Console().log("Act: Module %s loaded through class %s \n", mod.c_str(), typeName);
#endif
    loadModuleSet.insert(mod);
}

const std::string Type::getModuleName(const char* className)
{
    std::string_view classNameView(className);
    auto pos = classNameView.find("::");

    return pos != std::string_view::npos ? std::string(classNameView.substr(0, pos))
                                         : std::string();
}


const Type Type::createType(const Type parent, const char* name, instantiationMethod method)
{
    assert(name && name[0] != '\0' && "Type name must not be empty");

    Type newType;
    newType.index = static_cast<unsigned int>(Type::typedata.size());
    Type::typedata.emplace_back(new TypeData(name, newType, parent, method));

    // add to dictionary for fast lookup
    Type::typemap.emplace(name, newType.getKey());

    return newType;
}


void Type::init()
{
    assert(Type::typedata.size() == 0 && "Type::init() should only be called once");
    typedata.emplace_back(new TypeData(BadTypeName, BadType, BadType, nullptr));
    typemap[BadTypeName] = 0;
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

const Type Type::fromName(const char* name)
{
    const auto pos = typemap.find(name);
    if (pos == typemap.end()) {
        return Type::BadType;
    }

    return typedata[pos->second]->type;
}

const Type Type::fromKey(TypeId key)
{
    if (key < typedata.size()) {
        return typedata[key]->type;
    }

    return BadType;
}

const char* Type::getName() const
{
    assert(typedata.size() >= 1
           && "Type::init() must be called before fetching names, even for bad types");
    return typedata[index]->name.c_str();
}

const Type Type::getParent() const
{
    assert(typedata.size() >= 1
           && "Type::init() must be called before fetching parents, even for bad types");
    return typedata[index]->parent;
}

bool Type::isDerivedFrom(const Type type) const
{
    Type temp(*this);
    do {
        if (temp == type) {
            return true;
        }
        temp = temp.getParent();
    } while (!temp.isBad());

    return false;
}

int Type::getAllDerivedFrom(const Type type, std::vector<Type>& list)
{
    int cnt = 0;

    for (auto it : typedata) {
        if (it->type.isDerivedFrom(type)) {
            list.push_back(it->type);
            cnt++;
        }
    }
    return cnt;
}

int Type::getNumTypes()
{
    return static_cast<int>(typedata.size());
}

const Type Type::getTypeIfDerivedFrom(const char* name, const Type parent, bool loadModule)
{
    if (loadModule) {
        importModule(name);
    }

    if (const Type type(fromName(name)); type.isDerivedFrom(parent)) {
        return type;
    }

    return BadType;
}
