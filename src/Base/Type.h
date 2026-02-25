// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

// Std. configurations

#include <string>
#include <map>
#include <set>
#include <vector>
#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

struct TypeData;


/** Type system class
  Many of the classes in the FreeCAD must have their type
  information registered before any instances are created (including,
  but not limited to: App::Feature, App::Property, Gui::ViewProvider
  ). The use of Type to store this information provides
  lots of various functionality for working with class hierarchies,
  comparing class types, instantiating objects from classnames, etc
  etc.

  It is for instance possible to do things like this:

  \code
  void getRightFeature(Base::Base * anode)
  {
    assert(anode->isDerivedFrom<App::Feature>());

    if (anode->is<Mesh::MeshFeature>()) {
      // do something..
    }
    else if (anode->is<Part::Feature>()) {
      // do something..
    }
    else {
      Base::Console().warning("getRightFeature", "Unknown feature type %s!\n",
                                anode->getTypeId().getName());
    }
  }
  \endcode

  A notable feature of the Type class is that it is only 16 bits
  long and therefore should be passed around by value for efficiency
  reasons.

  One important note about the use of Type to register class
  information: super classes must be registered before any of their
  derived classes are.
*/
class BaseExport Type final
{
public:
    using TypeId = unsigned int;
    /// Construction
    Type(const Type& type) = default;
    Type(Type&& type) = default;
    Type() = default;
    /// Destruction
    ~Type() = default;

    /// Creates an instance of this type
    [[nodiscard]] void* createInstance() const;
    /// Checks whether this type can instantiate
    [[nodiscard]] bool canInstantiate() const;
    /// Creates an instance of the named type
    [[nodiscard]] static void* createInstanceByName(const char* typeName, bool loadModule = false);

    using instantiationMethod = void* (*)();

    /// Returns a type object by name
    [[nodiscard]] static const Type fromName(const char* name);
    /// Returns a type object by key
    [[nodiscard]] static const Type fromKey(TypeId key);
    /// Returns the name of the type
    [[nodiscard]] const char* getName() const;
    /// Returns the parent type
    [[nodiscard]] const Type getParent() const;
    /// Checks whether this type is derived from "type"
    [[nodiscard]] bool isDerivedFrom(const Type type) const;
    /// Returns all descendants from the given type
    static int getAllDerivedFrom(const Type type, std::vector<Type>& list);
    /// Returns the given named type if is derived from parent type, otherwise return bad type
    [[nodiscard]] static const Type getTypeIfDerivedFrom(
        const char* name,
        const Type parent,
        bool loadModule = false
    );
    /// Returns the number of types created so far
    [[nodiscard]] static int getNumTypes();
    /// Creates a new type with the given name, parent and instantiation method
    [[nodiscard]] static const Type createType(
        const Type parent,
        const char* name,
        instantiationMethod method = nullptr
    );
    /// Returns the inner index of the type
    [[nodiscard]] TypeId getKey() const;
    /// Checks if the type is invalid
    [[nodiscard]] bool isBad() const;

    Type& operator=(const Type& type) = default;
    Type& operator=(Type&& type) = default;
    bool operator==(const Type& type) const;
    bool operator!=(const Type& type) const;

    bool operator<(const Type& type) const;
    bool operator<=(const Type& type) const;
    bool operator>=(const Type& type) const;
    bool operator>(const Type& type) const;

    static const Type BadType;
    static void init();
    static void destruct();

    /// Returns the name of the module the class is defined in
    static const std::string getModuleName(const char* className);

private:
    [[nodiscard]] instantiationMethod getInstantiationMethod() const;
    static void importModule(const char* TypeName);

    TypeId index {BadTypeIndex};

    static std::map<std::string, TypeId> typemap;
    static std::vector<TypeData*> typedata;  // use pointer to hide implementation details
    static std::set<std::string> loadModuleSet;

    static constexpr TypeId BadTypeIndex = 0;
};


inline Type::TypeId Type::getKey() const
{
    return this->index;
}

inline bool Type::operator!=(const Type& type) const
{
    return (this->getKey() != type.getKey());
}

inline bool Type::operator==(const Type& type) const
{
    return (this->getKey() == type.getKey());
}

inline bool Type::operator<(const Type& type) const
{
    return (this->getKey() < type.getKey());
}

inline bool Type::operator<=(const Type& type) const
{
    return (this->getKey() <= type.getKey());
}

inline bool Type::operator>=(const Type& type) const
{
    return (this->getKey() >= type.getKey());
}

inline bool Type::operator>(const Type& type) const
{
    return (this->getKey() > type.getKey());
}

inline bool Type::isBad() const
{
    return this->index == BadTypeIndex;
}

}  // namespace Base
