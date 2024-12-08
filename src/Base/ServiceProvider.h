// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 Kacper Donat <kacper@kadet.net>                     *
 *                                                                          *
 *   This file is part of FreeCAD.                                          *
 *                                                                          *
 *   FreeCAD is free software: you can redistribute it and/or modify it     *
 *   under the terms of the GNU Lesser General Public License as            *
 *   published by the Free Software Foundation, either version 2.1 of the   *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   FreeCAD is distributed in the hope that it will be useful, but         *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
 *   Lesser General Public License for more details.                        *
 *                                                                          *
 *   You should have received a copy of the GNU Lesser General Public       *
 *   License along with FreeCAD. If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                       *
 *                                                                          *
 ***************************************************************************/

#ifndef APP_SERVICE_PROVIDER_H
#define APP_SERVICE_PROVIDER_H

#include <FCGlobal.h>

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <any>
#include <list>

namespace Base
{

class BaseExport ServiceProvider
{
    struct ServiceDescriptor
    {
        std::string name;
        std::any instance;

        template<typename T>
        T* get() const
        {
            return std::any_cast<T*>(instance);
        }
    };

public:
    ServiceProvider() = default;

    /**
     * Returns most recent implementation of service specified as T param.
     *
     * @tparam T Service interface
     */
    template<typename T>
    T* provide() const
    {
        if (auto it = _implementations.find(typeid(T).name()); it != _implementations.end()) {
            auto descriptors = it->second;

            if (descriptors.empty()) {
                return nullptr;
            }

            return descriptors.front().get<T>();
        }

        return nullptr;
    }

    /**
     * Returns all implementations of service specified as T param.
     *
     * @tparam T Service interface
     */
    template<typename T>
    std::list<T*> all() const
    {
        if (auto it = _implementations.find(typeid(T).name()); it != _implementations.end()) {
            auto source = it->second;

            std::list<T*> result(source.size());

            std::transform(source.begin(),
                           source.end(),
                           result.begin(),
                           [](const ServiceDescriptor& descriptor) {
                               return descriptor.get<T>();
                           });

            return result;
        }

        return {};
    }

    /**
     * Adds new implementation of service T.
     *
     * @tparam T Service interface
     */
    template<typename T>
    void implement(T* contract)
    {
        ServiceDescriptor descriptor {typeid(T).name(), contract};

        _implementations[typeid(T).name()].push_front(descriptor);
    }

    static ServiceProvider& get();

private:
    std::map<const char*, std::deque<ServiceDescriptor>> _implementations;
};

template<typename T>
T* provideImplementation()
{
    return ServiceProvider::get().provide<T>();
}

template<typename T>
std::list<T*> provideAllImplementations()
{
    return ServiceProvider::get().all<T>();
}

template<typename T>
void implementContract(T* implementation)
{
    ServiceProvider::get().implement<T>(implementation);
}

}  // namespace Base


#endif  // APP_SERVICE_PROVIDER_H
