
// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2025 Joao Matos (tritao)                                 *
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

#ifndef APP_MAINTHREADSIGNAL_H
#define APP_MAINTHREADSIGNAL_H

#include <Base/Interpreter.h>
#include <fastsignals/signal.h>
#include <tuple>
#include <functional>
#include <type_traits>
#include <utility>
#include <variant>

namespace App
{

class MainThreadSignalConfig {
public:
    using IsMainThreadFn = bool (*)(); // true iff currently on GUI/main thread
    using InvokeFn       = void (*)(std::function<void()>&& fn, bool blocking);

    static void set_hooks(IsMainThreadFn is_main_thread, InvokeFn invoke) {
        is_main_thread_slot() = is_main_thread;
        invoke_slot()         = invoke;
    }

    static inline bool is_main_thread() {
        auto* f = is_main_thread_slot();
        return f ? f() : true; // no hooks, treat current thread as "main"
    }

    static inline void invoke(std::function<void()>&& fn, bool blocking) {
        auto* f = invoke_slot();
        if (f) f(std::move(fn), blocking);
        else   fn(); // no hooks, run inline
    }

private:
    static IsMainThreadFn& is_main_thread_slot() {
        static IsMainThreadFn fn = nullptr;
        return fn;
    }
    static InvokeFn& invoke_slot() {
        static InvokeFn fn = nullptr;
        return fn;
    }
};

namespace {
    // Holds a value (for by-value params)
    template <class T>
    struct _TA_Value {
        T v;
        constexpr decltype(auto) get()       noexcept { return (v); }
        constexpr decltype(auto) get() const noexcept { return (v); }
    };

    // Holds a pointer (for by-reference params, preserves cv-qualifiers)
    template <class T>
    struct _TA_Ref {
        using Raw = std::remove_reference_t<T>; // keeps const if present
        Raw* p{};                               // pointer preserves cv-ness
        constexpr decltype(auto) get() const noexcept { return *p; } // Raw& or const Raw&
    };

    // Explicitly choose storage kind from the declared parameter type PDecl
    template <class PDecl>
    static auto _ta_capture(PDecl&& x) {
        if constexpr (std::is_lvalue_reference_v<PDecl>) {
            using Raw = std::remove_reference_t<PDecl>;
            return _TA_Ref<Raw>{ std::addressof(x) };  // &param → pointer
        } else {
            using V = std::decay_t<PDecl>;
            return _TA_Value<V>{ std::forward<PDecl>(x) }; // value param → by value
        }
    }

    template <class T>
    using no_void_t = std::conditional_t<std::is_void_v<T>, std::monostate, T>;
}

// Wrapper that mirrors fastsignals::signal but executes slots on GUI thread.
template <class Signature, template <class T> class Combiner = ::fastsignals::optional_last_value>
class MainThreadSignal;

template <class Return, class... Arguments, template <class T> class Combiner>
class MainThreadSignal<Return(Arguments...), Combiner>
{
    using base_sig      = ::fastsignals::signal<Return(Arguments...), Combiner>;
public:
    using signature_type = typename base_sig::signature_type;
    using slot_type      = typename base_sig::slot_type;
    using combiner_type  = typename base_sig::combiner_type;
    using result_type    = typename base_sig::result_type;

    MainThreadSignal() = default;
    MainThreadSignal(const MainThreadSignal&) = delete;
    MainThreadSignal& operator=(const MainThreadSignal&) = delete;
    MainThreadSignal(MainThreadSignal&&) = default;
    MainThreadSignal& operator=(MainThreadSignal&&) = default;

    // connections
    ::fastsignals::connection connect(slot_type slot) {
        return sig_.connect(std::move(slot));
    }
    ::fastsignals::advanced_connection connect(slot_type slot, ::fastsignals::advanced_tag tag) {
        return sig_.connect(std::move(slot), tag);
    }

    void disconnect_all_slots() noexcept { sig_.disconnect_all_slots(); }
    std::size_t num_slots() const noexcept { return sig_.num_slots(); }
    bool empty() const noexcept { return sig_.empty(); }

    // ---- emission APIs ------------------------------------------------------

    // Blocking: executes slots on GUI thread, returns combiner's result_type.
    result_type emit(typename ::fastsignals::signal_arg_t<Arguments>... args) {
        if (MainThreadSignalConfig::is_main_thread()) {
            return sig_(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        }

        Base::PyGILStateRelease release;

        // Capture args without copying non-copyables:
        auto caps = std::make_tuple(
            _ta_capture<typename ::fastsignals::signal_arg_t<Arguments>>(args)...
        );

        if constexpr (std::is_void_v<result_type>) {
            MainThreadSignalConfig::invoke(
                [this, caps = std::move(caps)]() mutable {
                    std::apply([this](auto&... c){ sig_(c.get()...); }, caps);
                },
                /*blocking=*/true
            );
        } else {
            std::optional<no_void_t<result_type>> result;
            MainThreadSignalConfig::invoke(
                [this, caps = std::move(caps), &result]() mutable {
                    result.emplace(std::apply([this](auto&... c){
                        return sig_(c.get()...);
                    }, caps));
                },
                /*blocking=*/true
            );
            return std::move(*result);
        }
    }

    // Non-blocking post (queued). Returns immediately; slots still run on GUI.
    void post(typename ::fastsignals::signal_arg_t<Arguments>... args) {
        if (MainThreadSignalConfig::is_main_thread()) {
            sig_(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
            return;
        }

        auto caps = std::make_tuple(
            _ta_capture<typename ::fastsignals::signal_arg_t<Arguments>>(args)...
        );
        MainThreadSignalConfig::invoke(
            [this, caps = std::move(caps)]() mutable {
                std::apply([this](auto&... c){ sig_(c.get()...); }, caps);
            },
            /*blocking=*/false
        );
    }

    result_type operator()(typename ::fastsignals::signal_arg_t<Arguments>... args) {
        if constexpr (std::is_void_v<result_type>) {
            emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        } else {
            return emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        }
    }

    result_type operator()(typename ::fastsignals::signal_arg_t<Arguments>... args) const {
        if constexpr (std::is_void_v<result_type>) {
            const_cast<MainThreadSignal*>(this)->emit(
                std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        } else {
            return const_cast<MainThreadSignal*>(this)->emit(
                std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
        }
    }

    // Plug a MainThreadSignal into a plain fastsignal as a slot:
    operator slot_type() const noexcept {
        return [this](typename ::fastsignals::signal_arg_t<Arguments>... args) {
            const_cast<MainThreadSignal*>(this)->emit(std::forward<typename ::fastsignals::signal_arg_t<Arguments>>(args)...);
            if constexpr (!std::is_void_v<Return>) return Return();
        };
    }

    // escape hatch
    base_sig& underlying() { return sig_; }
    const base_sig& underlying() const { return sig_; }

private:
    base_sig sig_;
};

}  // namespace App

#endif // APP_MAINTHREADSIGNAL_H
