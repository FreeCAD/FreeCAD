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

#include "Type.h"

// Python stuff
using PyObject = struct _object;


// NOLINTBEGIN(cppcoreguidelines-macro-usage)
/// define for subclassing Base::BaseClass
#define TYPESYSTEM_HEADER() \
public: \
    static Base::Type getClassTypeId(void); \
    virtual Base::Type getTypeId(void) const; \
    static void init(void); \
    static void* create(void); \
\
private: \
    static Base::Type classTypeId


/// Like TYPESYSTEM_HEADER, but declare getTypeId as 'override'
#define TYPESYSTEM_HEADER_WITH_OVERRIDE() \
public: \
    static Base::Type getClassTypeId(void); \
    Base::Type getTypeId(void) const override; \
    static void init(void); \
    static void* create(void); \
\
private: \
    static Base::Type classTypeId


/// define to implement a  subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_P(_class_) \
    Base::Type _class_::getClassTypeId(void) \
    { \
        return _class_::classTypeId; \
    } \
    Base::Type _class_::getTypeId(void) const \
    { \
        return _class_::classTypeId; \
    } \
    Base::Type _class_::classTypeId = Base::Type::BadType; \
    void* _class_::create(void) \
    { \
        return new _class_(); \
    }

/// define to implement a  subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_TEMPLATE_P(_class_) \
    template<> \
    Base::Type _class_::getClassTypeId(void) \
    { \
        return _class_::classTypeId; \
    } \
    template<> \
    Base::Type _class_::getTypeId(void) const \
    { \
        return _class_::classTypeId; \
    } \
    template<> \
    void* _class_::create(void) \
    { \
        return new _class_(); \
    }

/// define to implement a  subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_ABSTRACT_P(_class_) \
    Base::Type _class_::getClassTypeId(void) \
    { \
        return _class_::classTypeId; \
    } \
    Base::Type _class_::getTypeId(void) const \
    { \
        return _class_::classTypeId; \
    } \
    Base::Type _class_::classTypeId = Base::Type::BadType; \
    void* _class_::create(void) \
    { \
        return nullptr; \
    }


/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE(_class_, _parentclass_) \
    TYPESYSTEM_SOURCE_P(_class_) \
    void _class_::init(void) \
    { \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, &(_class_::create)); \
    }

/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_TEMPLATE_T(_class_, _parentclass_) \
    TYPESYSTEM_SOURCE_TEMPLATE_P(_class_) \
    template<> \
    void _class_::init(void) \
    { \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, &(_class_::create)); \
    }

/// define to implement a subclass of Base::BaseClass
#define TYPESYSTEM_SOURCE_ABSTRACT(_class_, _parentclass_) \
    TYPESYSTEM_SOURCE_ABSTRACT_P(_class_) \
    void _class_::init(void) \
    { \
        initSubclass(_class_::classTypeId, #_class_, #_parentclass_, nullptr); \
    }
// NOLINTEND(cppcoreguidelines-macro-usage)

namespace Base
{
/// BaseClass class and root of the type system
class BaseExport BaseClass
{
public:
    static Type getClassTypeId();
    virtual Type getTypeId() const;
    bool isDerivedFrom(const Type type) const
    {
        return getTypeId().isDerivedFrom(type);
    }

    static void init();

    virtual PyObject* getPyObject();
    virtual void setPyObject(PyObject*);

    static void* create()
    {
        return nullptr;
    }

    template<typename T>
    bool is() const;

    template<typename T>
    bool isDerivedFrom() const;

private:
    static Type classTypeId;  // NOLINT

protected:
    static void initSubclass(
        Base::Type& toInit,
        const char* ClassName,
        const char* ParentName,
        Type::instantiationMethod method = nullptr
    );

public:
    /// Construction
    BaseClass();
    BaseClass(const BaseClass&) = default;
    BaseClass& operator=(const BaseClass&) = default;
    BaseClass(BaseClass&&) = default;
    BaseClass& operator=(BaseClass&&) = default;
    /// Destruction
    virtual ~BaseClass();
};

template<typename T>
bool BaseClass::is() const
{
    static_assert(std::is_base_of_v<BaseClass, T>, "T must be derived from Base::BaseClass");
    return getTypeId() == T::getClassTypeId();
}

template<typename T>
bool BaseClass::isDerivedFrom() const
{
    static_assert(std::is_base_of_v<BaseClass, T>, "T must be derived from Base::BaseClass");
    return getTypeId().isDerivedFrom(T::getClassTypeId());
}

/**
 * Template that works just like dynamic_cast, but expects the argument to
 * inherit from Base::BaseClass.
 */
template<typename T, typename U = std::remove_pointer_t<T>>
    requires(std::is_pointer_v<T>)
T freecad_cast(Base::BaseClass* type)
{
    static_assert(std::is_base_of_v<Base::BaseClass, U>, "T must be derived from Base::BaseClass");

    if (type && type->isDerivedFrom(U::getClassTypeId())) {
        return static_cast<T>(type);
    }

    return nullptr;
}

/**
 * Template that works just like dynamic_cast, but expects the argument to
 * inherit from a const Base::BaseClass.
 */
template<typename T, typename U = std::remove_pointer_t<T>>
    requires(std::is_pointer_v<T>)
const U* freecad_cast(const Base::BaseClass* type)
{
    static_assert(std::is_base_of_v<Base::BaseClass, U>, "T must be derived from Base::BaseClass");

    if (type && type->isDerivedFrom(U::getClassTypeId())) {
        return static_cast<const U*>(type);
    }

    return nullptr;
}

}  // namespace Base

// We define global alias for freecad_cast to be used by all FreeCAD files that include
// BaseClass.h. While doing using on header level is non-ideal it allows for much easier use
// of the important freecad_cast. In that case the name is prefixed with freecad so there is no
// chance of symbols collision.
using Base::freecad_cast;
