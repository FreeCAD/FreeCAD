// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Kacper Donat <kacper@kadet.net>                     *
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

#include <ranges>
#include <type_traits>
#include <utility>
#include <vector>
#include <functional>

#include <FCGlobal.h>

namespace App
{

/**
 * @brief Default accessor for property-like classes.
 *
 * The `DefaultAccessor` provides a generic implementation of `get()` and `set()` methods
 * that call the `getValue()` and `setValue()` members of a property. This works for the
 * majority of `App::Property` types in FreeCAD.
 *
 * ### Usage
 * Normally you do not use `DefaultAccessor` directly. Instead, use `PropAccessor<P>`,
 * which inherits from `DefaultAccessor` unless specialized.
 *
 * @tparam P The property type, e.g. `App::PropertyBool`, `App::PropertyFloat`.
 */
template<class P>
struct DefaultAccessor
{
    /// Value type of the property, deduced from `getValue()`.
    using Value = std::decay_t<decltype(std::declval<const P&>().getValue())>;

    /// Retrieves the property's current value.
    static Value get(const P& p)
    {
        return p.getValue();
    }

    /// Assigns a new value to the property.
    static void set(P& p, const Value& v)
    {
        p.setValue(v);
    }
};

/**
 * @brief Customizable accessor hook for properties.
 *
 * `PropAccessor` defaults to `DefaultAccessor`. Specialize `PropAccessor<P>` if you
 * need custom behavior for a specific property type (e.g. suppressing recomputes,
 * triggering extra updates).
 *
 * @tparam P The property type to customize.
 */
template<class P>
struct PropAccessor: DefaultAccessor<P>
{
};

template<class P>
concept HasGetValue = requires(const P& cp) { cp.getValue(); };
template<class P>
concept HasSetValue = requires(P& p, typename DefaultAccessor<P>::Value v) { p.setValue(v); };
template<class P>
concept PropertyLike = HasGetValue<P> && HasSetValue<P>;

/**
 * @brief RAII guard that overrides a property for the duration of a scope.
 *
 * `ScopedPropertyOverride` temporarily changes the value of a property, and restores
 * the old value when the guard is destroyed. This ensures properties are always restored
 * even if exceptions occur.
 *
 * ### Example
 * @code{.cpp}
 * App::PropertyBool& vis = ...;
 * {
 *     App::ScopedPropertyOverride guard(vis, false);
 *     // object is hidden here
 * }
 * // old visibility automatically restored
 * @endcode
 *
 * @tparam P The property type, e.g. `App::PropertyBool`.
 */
template<PropertyLike P>
class ScopedPropertyOverride
{
public:
    using Value = PropAccessor<P>::Value;


    /**
     * @brief Constructs an override guard.
     *
     * Saves the old value and sets the property to a new one.
     *
     * @param prop Reference to the property being overridden.
     * @param newValue The temporary value to apply.
     */
    ScopedPropertyOverride(P& prop, const Value& newValue)
        : _prop(&prop)
        , _old(PropAccessor<P>::get(prop))
        , _active(true)
    {
        PropAccessor<P>::set(*_prop, newValue);
    }

    FC_DISABLE_COPY(ScopedPropertyOverride);

    ScopedPropertyOverride(ScopedPropertyOverride&& other) noexcept
        : _prop(other._prop)
        , _old(std::move(other._old))
        , _active(other._active)
    {
        other._prop = nullptr;
        other._active = false;
    }

    ScopedPropertyOverride& operator=(ScopedPropertyOverride&& other) noexcept
    {
        if (this != &other) {
            restore();
            _prop = other._prop;
            _old = std::move(other._old);
            _active = other._active;
            other._prop = nullptr;
            other._active = false;
        }
        return *this;
    }

    /**
     * @brief Destructor.
     *
     * Restores the original value of the property.
     */
    ~ScopedPropertyOverride()
    {
        restore();
    }

    /**
     * @brief Checks if the guard is currently active.
     *
     * @return True if the override is in effect.
     */
    bool active() const
    {
        return _active;
    }

private:
    void restore() noexcept
    {
        if (_prop && _active) {
            PropAccessor<P>::set(*_prop, _old);
            _active = false;
        }
    }

    P* _prop {nullptr};    ///< Pointer to the property being overridden.
    Value _old {};         ///< Saved old value.
    bool _active {false};  ///< Whether this guard is currently active.
};

/**
 * @brief Helper function to create a `ScopedPropertyOverride` with type deduction.
 *
 * @tparam P The property type.
 * @tparam V The value type (deduced).
 * @param prop Property reference.
 * @param v New value to assign temporarily.
 *
 * @return A `ScopedPropertyOverride` guard.
 */
template<PropertyLike P, class V>
auto makeOverride(P& prop, V&& v)
{
    using Guard = ScopedPropertyOverride<P>;
    return Guard(prop, static_cast<Guard::Value>(std::forward<V>(v)));
}

/**
 * @brief RAII context for managing multiple property overrides.
 *
 * `PropertyOverrideContext` stores multiple property overrides at once. When the
 * context is destroyed, all properties are restored in reverse (LIFO) order.
 *
 * ### Example
 * @code{.cpp}
 * App::PropertyBool& vis = ...;
 * App::PropertyString& label = ...;
 *
 * App::PropertyOverrideContext overrides;
 * overrides.override(vis, false);
 * overrides.override(label, "Temp");
 * // both props overridden
 * // ...
 * // restored automatically when ctx is destroyed
 * @endcode
 */
class PropertyOverrideContext
{
public:
    PropertyOverrideContext() = default;

    FC_DISABLE_COPY(PropertyOverrideContext);
    FC_DEFAULT_MOVE(PropertyOverrideContext);

    /**
     * @brief Destructor.
     *
     * Restores all overrides.
     */
    ~PropertyOverrideContext()
    {
        clear();
    }

    /**
     * @brief Override a property within this context.
     *
     * Saves the old value and assigns a new one. The value is restored automatically
     * when the context is destroyed or `clear()` is called.
     *
     * @tparam P Property type.
     * @param prop Reference to property.
     * @param newValue Temporary value to assign.
     */
    template<PropertyLike P>
    void override(P& prop, PropAccessor<P>::Value newValue)
    {
        using Access = PropAccessor<P>;

        _guards.emplace_back([&prop, old = PropAccessor<P>::get(prop)]() {
            Access::set(prop, std::move(old));
        });

        Access::set(prop, std::move(newValue));
    }

    /**
     * @brief Restore all properties immediately.
     *
     * Calls the restore handlers in reverse order of creation and clears the context.
     */
    void clear()
    {
        for (auto& _guard : std::views::reverse(_guards)) {
            _guard();
        }
        _guards.clear();
    }

    /**
     * @brief Returns the number of active overrides in the context.
     *
     * @return Count of overrides.
     */
    std::size_t size() const
    {
        return _guards.size();
    }

private:
    std::vector<std::function<void()>> _guards;
};
}  // namespace App
