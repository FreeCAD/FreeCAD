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

#pragma once

#include <FCGlobal.h>

#include <algorithm>
#include <deque>
#include <map>
#include <string>
#include <any>
#include <list>

namespace Base
{

/**
 * Class that implements basic service container that can be used to obtain different implementation
 * of various services.
 *
 * Primary use of such container is to provide ability to define global services that can be
 * implemented within non-core modules. This for example allows to use code that is available only
 * in Part module from Base with the only requirement being that Part implements specific interface
 * and registers the service within service provider.
 *
 * For ease of use global service provider instance is provided with convenience functions:
 *  - Base::provideService
 *  - Base::provideServiceImplementations
 *  - Base::registerServiceImplementation
 *
 * As the example, we can define service that provides placement of sub objects in App:
 * @code
 * class SubObjectPlacementProvider
 * {
 * public:
 *     virtual Base::Placement calculate(SubObjectT object, Base::Placement basePlacement) const =
 * 0;
 * };
 * @endcode
 *
 * App does not know how to implement this service, but it can be implemented within Part module:
 * @code
 * class AttacherSubObjectPlacement final: public App::SubObjectPlacementProvider { ... }
 *
 * // later in module initialization method
 *
 * Base::registerServiceImplementation<App::SubObjectPlacementProvider>(new
 * AttacherSubObjectPlacement);
 * @endcode
 *
 * This service can then be obtained inside other modules, without them being aware of the
 * implementation - only the interface:
 *
 * @code
 * auto subObjectPlacementProvider = Base::provideService<App::SubObjectPlacementProvider>();
 * @endcode
 *
 * This function can (and should) be used as default for constructor injection of services.
 */
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

            std::transform(
                source.begin(),
                source.end(),
                result.begin(),
                [](const ServiceDescriptor& descriptor) { return descriptor.get<T>(); }
            );

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
    void registerImplementation(T* contract)
    {
        ServiceDescriptor descriptor {typeid(T).name(), contract};

        _implementations[typeid(T).name()].push_front(descriptor);
    }

private:
    std::map<std::string, std::deque<ServiceDescriptor>> _implementations;
};

BaseExport extern ServiceProvider globalServiceProvider;

/**
 * Obtains primary implementation of requested service from the global service provider.
 *
 * @tparam T Service kind to obtain.
 * @return Primary implementation of the service or nullptr if there is no implementation available.
 */
template<typename T>
T* provideService()
{
    return globalServiceProvider.provide<T>();
}

/**
 * Obtains all available implementations of requested service in the global service provider.
 *
 * @tparam T Service kind to obtain.
 * @return List of available service implementation.
 */
template<typename T>
std::list<T*> provideServiceImplementations()
{
    return globalServiceProvider.all<T>();
}

/**
 * Registers implementation of service in the global service provider.
 *
 * @tparam T Service kind to obtain.
 * @return List of available service implementation.
 */
template<typename T>
void registerServiceImplementation(T* implementation)
{
    globalServiceProvider.registerImplementation<T>(implementation);
}

}  // namespace Base
