// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2025 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef BASE_PARAMETEROBSERVER_H
#define BASE_PARAMETEROBSERVER_H

#include <Base/Parameter.h>
#include <FCGlobal.h>
#include <cstring>
#include <variant>
#include <memory>
#include <string>
#include <string_view>
#include <sstream>
#include <unordered_map>

namespace Base
{

class BaseExport ParameterObserver: public ParameterGrp::ObserverType
{
protected:
    using Type = std::variant<bool, long, unsigned long, double, std::string>;
    class Object
    {

    public:
        template<typename T>
        Object(T&& obj)  // NOLINT
            : object(std::make_shared<Model<T>>(std::forward<T>(obj)))
        {}

        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            object->fetch(handle, key);
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            object->setParameter(handle, key, value);
        }
        Type value() const
        {
            return object->value();
        }
        Type defaultValue() const
        {
            return object->defaultValue();
        }
        void setValue(const Type& value)
        {
            object->setValue(value);
        }

        struct Concept  // NOLINT
        {
            virtual ~Concept() = default;
            virtual void fetch(const ParameterGrp::handle&, const char* key) = 0;
            virtual void setParameter(
                const ParameterGrp::handle& handle,
                const char* key,
                const Type& value
            ) = 0;
            virtual Type value() const = 0;
            virtual Type defaultValue() const = 0;
            virtual void setValue(const Type&) = 0;
        };

        template<typename T>
        struct Model: Concept
        {
            explicit Model(const T& t)
                : object(t)
            {}
            void fetch(const ParameterGrp::handle& handle, const char* key) override
            {
                object.fetch(handle, key);
            }
            void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value) override
            {
                object.setParameter(handle, key, value);
            }
            Type value() const override
            {
                return object.value();
            }
            Type defaultValue() const override
            {
                return object.defaultValue();
            }
            void setValue(const Type& v) override
            {
                object.setValue(v);
            }

        private:
            T object;
        };

        std::shared_ptr<Concept> object;
    };

public:
    struct BaseType
    {
        Type _value, _default;
        explicit BaseType(const Type& d)
            : _value(d)
            , _default(d)
        {}
        Type value() const
        {
            return _value;
        }
        Type defaultValue() const
        {
            return _default;
        }
        void setValue(const Type& v)
        {
            _value = v;
        }
    };

    struct Bool: BaseType
    {
        using value_type = bool;
        explicit Bool(value_type d)
            : BaseType {d}
        {}
        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            _value = handle->GetBool(key, std::get<value_type>(_default));
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            handle->SetBool(key, std::get<value_type>(value));
        }
    };

    struct Int: BaseType
    {
        using value_type = long;
        explicit Int(value_type d)
            : BaseType {d}
        {}
        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            _value = handle->GetInt(key, std::get<value_type>(_default));
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            handle->SetInt(key, std::get<value_type>(value));
        }
    };

    struct Unsigned: BaseType
    {
        using value_type = unsigned long;
        explicit Unsigned(value_type d)
            : BaseType {d}
        {}
        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            _value = handle->GetUnsigned(key, std::get<value_type>(_default));
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            handle->SetUnsigned(key, std::get<value_type>(value));
        }
    };

    struct Double: BaseType
    {
        using value_type = double;
        explicit Double(value_type d)
            : BaseType {d}
        {}
        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            _value = handle->GetFloat(key, std::get<value_type>(_default));
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            handle->SetFloat(key, std::get<value_type>(value));
        }
    };

    struct String: BaseType
    {
        using value_type = std::string;
        explicit String(const value_type& d)
            : BaseType {d}
        {}
        void fetch(const ParameterGrp::handle& handle, const char* key)
        {
            _value = handle->GetASCII(key, std::get<value_type>(_default).c_str());
        }
        void setParameter(const ParameterGrp::handle& handle, const char* key, const Type& value)
        {
            handle->SetASCII(key, std::get<value_type>(value).c_str());
        }
    };

private:
    struct Hasher
    {
        std::size_t operator()(const char* s) const
        {
            return s ? std::hash<std::string_view> {}(s) : 0;
        }
        bool operator()(const char* a, const char* b) const
        {
            if (!a) {
                return !b;
            }
            if (!b) {
                return false;
            }
            return std::strcmp(a, b) == 0;
        }
    };
    std::unordered_map<const char*, Object, Hasher, Hasher> parameters;
    ParameterGrp::handle handle;

protected:
    ParameterObserver();

    void attachToParameter(ParameterGrp::handle parameter);
    ParameterGrp::handle getHandle() const
    {
        return handle;
    }
    void initParameters();
    void OnChange(Base::Subject<const char*>& subject, const char* sReason) override;
    void addParameter(const char* key, const Object& value);

    void setBoolean(const char* key, bool value);
    bool getBoolean(const char* key) const;
    bool getDefaultBoolean(const char* key) const;

    void setInt(const char* key, long value);
    long getInt(const char* key) const;
    long getDefaultInt(const char* key) const;

    void setUnsigned(const char* key, unsigned long value);
    unsigned long getUnsigned(const char* key) const;
    unsigned long getDefaultUnsigned(const char* key) const;

    void setFloat(const char* key, double value);
    double getFloat(const char* key) const;
    double getDefaultFloat(const char* key) const;

    void setString(const char* key, const char* value);
    std::string getString(const char* key) const;
    std::string getDefaultString(const char* key) const;

    template<typename T>
    void setValue(const char* key, const T& value)
    {
        try {
            parameters.at(key).setParameter(handle, key, value);
            parameters.at(key).setValue(value);
        }
        catch (const std::bad_variant_access&) {
            std::stringstream str;
            str << "Wrong cast of parameter " << key << '\n';
            throw Base::TypeError(str.str());
        }
        catch (const std::exception&) {
            std::stringstream str;
            str << "Unknown parameter " << key << '\n';
            throw Base::IndexError(str.str());
        }
    }
    template<typename T>
    T getValue(const char* key) const
    {
        try {
            return std::get<T>(parameters.at(key).value());
        }
        catch (const std::bad_variant_access&) {
            std::stringstream str;
            str << "Wrong cast of parameter " << key << '\n';
            throw Base::TypeError(str.str());
        }
        catch (const std::exception&) {
            std::stringstream str;
            str << "Unknown parameter " << key << '\n';
            throw Base::IndexError(str.str());
        }
    }
    template<typename T>
    T getDefault(const char* key) const
    {
        try {
            return std::get<T>(parameters.at(key).defaultValue());
        }
        catch (const std::bad_variant_access&) {
            std::stringstream str;
            str << "Wrong cast of parameter " << key << '\n';
            throw Base::TypeError(str.str());
        }
        catch (const std::exception&) {
            std::stringstream str;
            str << "Unknown parameter " << key << '\n';
            throw Base::IndexError(str.str());
        }
    }
};

template<class T>
struct type_traits
{
};

template<>
struct type_traits<bool>
{
    using value_type = ParameterObserver::Bool;
};

template<>
struct type_traits<long>
{
    using value_type = ParameterObserver::Int;
};

template<>
struct type_traits<unsigned long>
{
    using value_type = ParameterObserver::Unsigned;
};

template<>
struct type_traits<double>
{
    using value_type = ParameterObserver::Double;
};

template<>
struct type_traits<std::string>
{
    using value_type = ParameterObserver::String;
};

}  // namespace Base

template<typename T>
struct type_from_member;

template<typename M, typename T>
struct type_from_member<M T::*>
{
    using type = T;
};

template<typename T, typename R>
constexpr bool is_getter = std::is_same<T, R (type_from_member<T>::type::*)() const>::value;

template<typename T, typename A>
constexpr bool is_setter = std::is_same<T, void (type_from_member<T>::type::*)(A)>::value;

#define FC_PARAM_GETSET_IMP(_class, _name, _ctype) \
    _ctype _class::get##_name() const \
    { \
        return getValue<_ctype>(#_name); \
    } \
\
    void _class::set##_name(_ctype v) \
    { \
        setValue(#_name, v); \
    }

#endif  // BASE_PARAMETEROBSERVER_H
