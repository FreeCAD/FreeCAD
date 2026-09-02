// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2025 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#ifndef APP_MAINTHREADSIGNAL_H
#define APP_MAINTHREADSIGNAL_H

#include <Base/Interpreter.h>
#include <fastsignals/signal.h>
#include <functional>
#include <memory>
#include <optional>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace App
{

// App owns these signal types because App::Document declares them. Gui installs
// the actual main-thread hooks when a GUI application is available.
//
// These signals are intended for document-scoped notifications that GUI code
// may observe while recompute can run on a worker thread. Raw
// App::DocumentObject signals intentionally remain plain fastsignals with
// same-thread semantics.
class MainThreadSignalConfig
{
public:
    using IsMainThreadFn = bool (*)();  // true iff currently on GUI/main thread
    using InvokeFn = void (*)(std::function<void()>&& fn, bool blocking);

    static void setHooks(IsMainThreadFn isMainThread, InvokeFn invoke)
    {
        isMainThreadSlot() = isMainThread;
        invokeSlot() = invoke;
    }

    static inline bool isMainThread()
    {
        auto* f = isMainThreadSlot();
        return f ? f() : true;  // no hooks, treat current thread as "main"
    }

    static inline bool hasHooks()
    {
        return isMainThreadSlot() && invokeSlot();
    }

    static inline void invoke(std::function<void()>&& fn, bool blocking)
    {
        auto* f = invokeSlot();
        if (f) {
            f(std::move(fn), blocking);
        }
        else {
            fn();  // no hooks, run inline
        }
    }

private:
    static IsMainThreadFn& isMainThreadSlot()
    {
        static IsMainThreadFn fn = nullptr;
        return fn;
    }
    static InvokeFn& invokeSlot()
    {
        static InvokeFn fn = nullptr;
        return fn;
    }
};

namespace detail
{
// Holds a value (for by-value params)
template<class T>
struct SignalArgValue
{
    T v;
    constexpr decltype(auto) get() noexcept
    {
        return (v);
    }
    constexpr decltype(auto) get() const noexcept
    {
        return (v);
    }
};

// Holds a pointer (for by-reference params, preserves cv-qualifiers)
template<class T>
struct SignalArgRef
{
    using Raw = std::remove_reference_t<T>;  // keeps const if present
    Raw* p {};                               // pointer preserves cv-ness
    constexpr decltype(auto) get() const noexcept
    {
        return *p;
    }  // Raw& or const Raw&
};

// Explicitly choose storage kind from the declared parameter type PDecl
template<class PDecl>
auto captureSignalArg(PDecl&& x)
{
    if constexpr (std::is_lvalue_reference_v<PDecl>) {
        using Raw = std::remove_reference_t<PDecl>;
        return SignalArgRef<Raw> {std::addressof(x)};  // &param → pointer
    }
    else {
        using V = std::decay_t<PDecl>;
        return SignalArgValue<V> {std::forward<PDecl>(x)};  // value param → by value
    }
}

template<class T>
using non_void_t = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
}  // namespace detail

// Wrapper that mirrors fastsignals::signal but executes slots on GUI thread.
template<class Signature, template<class T> class Combiner = ::fastsignals::optional_last_value>
class MainThreadSignal;

template<class Return, class... Arguments, template<class T> class Combiner>
class MainThreadSignal<Return(Arguments...), Combiner>
{
    using base_sig = ::fastsignals::signal<Return(Arguments...), Combiner>;

public:
    using signature_type = typename base_sig::signature_type;
    using slot_type = typename base_sig::slot_type;
    using combiner_type = typename base_sig::combiner_type;
    using result_type = typename base_sig::result_type;

    MainThreadSignal() = default;
    MainThreadSignal(const MainThreadSignal&) = delete;
    MainThreadSignal& operator=(const MainThreadSignal&) = delete;
    MainThreadSignal(MainThreadSignal&&) = default;
    MainThreadSignal& operator=(MainThreadSignal&&) = default;

    // connections
    ::fastsignals::connection connect(slot_type slot)
    {
        return sig_.connect(std::move(slot));
    }
    ::fastsignals::advanced_connection connect(slot_type slot, ::fastsignals::advanced_tag tag)
    {
        return sig_.connect(std::move(slot), tag);
    }

    void disconnect_all_slots() noexcept
    {
        sig_.disconnect_all_slots();
    }
    std::size_t num_slots() const noexcept
    {
        return sig_.num_slots();
    }
    bool empty() const noexcept
    {
        return sig_.empty();
    }

    // ---- emission APIs ------------------------------------------------------

    result_type emit(typename ::fastsignals::signal_arg_t<Arguments>... args)
    {
        return emitImpl(this, std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
    }

    result_type emit(typename ::fastsignals::signal_arg_t<Arguments>... args) const
    {
        return emitImpl(this, std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
    }

    result_type operator()(typename ::fastsignals::signal_arg_t<Arguments>... args)
    {
        return emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
    }

    result_type operator()(typename ::fastsignals::signal_arg_t<Arguments>... args) const
    {
        return emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
    }

    // Plug a MainThreadSignal into a plain fastsignal as a slot:
    operator slot_type() const noexcept
    {
        return [this](typename ::fastsignals::signal_arg_t<Arguments>... args) {
            emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
            if constexpr (!std::is_void_v<Return>) {
                return Return();
            }
        };
    }

    // escape hatch
    base_sig& underlying()
    {
        return sig_;
    }
    const base_sig& underlying() const
    {
        return sig_;
    }

private:
    template<class Self>
    static result_type emitImpl(
        Self* self,
        typename ::fastsignals::signal_arg_t<Arguments>... args
    )
    {
        if (MainThreadSignalConfig::isMainThread()) {
            return self->sig_(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        }

        Base::PyGILStateRelease release;

        auto caps = std::make_tuple(
            detail::captureSignalArg<typename ::fastsignals::signal_arg_t<Arguments>>(args)...
        );

        if constexpr (std::is_void_v<result_type>) {
            MainThreadSignalConfig::invoke(
                [self, caps = std::move(caps)]() mutable {
                    std::apply([self](auto&... c) { self->sig_(c.get()...); }, caps);
                },
                /*blocking=*/true
            );
        }
        else {
            std::optional<detail::non_void_t<result_type>> result;
            MainThreadSignalConfig::invoke(
                [self, caps = std::move(caps), &result]() mutable {
                    result.emplace(
                        std::apply([self](auto&... c) { return self->sig_(c.get()...); }, caps)
                    );
                },
                /*blocking=*/true
            );
            return std::move(*result);
        }
    }

    mutable base_sig sig_;
};

}  // namespace App

#endif  // APP_MAINTHREADSIGNAL_H
