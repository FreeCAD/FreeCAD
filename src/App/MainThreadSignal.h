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
#include <FCGlobal.h>
#include <fastsignals/signal.h>
#include <exception>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <utility>

namespace App
{

// App owns this Qt-free dispatch abstraction because App emits synchronous
// observer notifications that GUI code may consume. Gui installs the concrete
// main-thread hooks when a GUI application is available.
//
// MainThreadSignal is intended for App-owned observer notifications whose
// subscribers require main-thread delivery while the mutation may originate
// on a worker. Without installed hooks, emission remains synchronous on the
// calling thread. Raw App::DocumentObject signals intentionally remain plain
// fastsignals with same-thread semantics.
class AppExport MainThreadSignalConfig
{
public:
    using IsMainThreadFn = bool (*)();  // true iff currently on GUI/main thread
    using TaskFn = void (*)(void* context);
    using InvokeSyncFn = bool (*)(TaskFn task, void* context);

    // task/context are non-owning and remain valid only until InvokeSyncFn
    // returns. A successful synchronous invocation executes task(context)
    // exactly once and does not return until all task side effects are visible
    // to the caller. False means that the task was not executed.
    //
    // Hooks are installed during GUI initialization and must remain unchanged
    // while worker threads can emit a MainThreadSignal. Replacing or clearing
    // them is only safe after all such emitters have stopped.
    static void installHooks(IsMainThreadFn isMainThread, InvokeSyncFn invokeSync);
    static void clearHooks();
    static bool isMainThread();
    static bool hasHooks();

    // Blocking delivery is intentional: signal payloads can contain references
    // whose lifetime is guaranteed only until the emitter returns.
    template<typename Fn>
    static std::invoke_result_t<Fn&> callOnMainThreadSync(Fn&& fn)
    {
        using Callable = std::remove_reference_t<Fn>;
        using Result = std::invoke_result_t<Fn&>;

        static_assert(
            !std::is_rvalue_reference_v<Result>,
            "callOnMainThreadSync cannot safely return an rvalue reference"
        );

        if (isMainThread()) {
            return std::invoke(fn);
        }

        // Invocation is synchronous, so both lvalue and temporary callables
        // remain alive for the entire dispatch. Keep the callable and all
        // result state on this stack frame.
        auto invokeSyncWithGILReleased = [](TaskFn task, void* context) {
            std::optional<Base::PyGILStateRelease> release;
            if (Py_IsInitialized() && PyGILState_Check()) {
                release.emplace();
            }

            return MainThreadSignalConfig::invokeSync(task, context);
        };

        auto validateInvocation = [](
                                      bool invoked,
                                      bool completed,
                                      const std::exception_ptr& exception
                                  ) {
            if (!invoked) {
                throw std::runtime_error("Failed to invoke callable on the main thread");
            }
            if (exception) {
                std::rethrow_exception(exception);
            }
            if (!completed) {
                throw std::logic_error(
                    "Main-thread hook returned before the callable completed"
                );
            }
        };

        if constexpr (std::is_void_v<Result>) {
            struct State
            {
                Callable* callable;
                std::exception_ptr exception;
                bool completed = false;
            } state {std::addressof(fn), {}, false};

            const bool invoked = invokeSyncWithGILReleased(
                [](void* context) {
                    auto& state = *static_cast<State*>(context);
                    try {
                        std::invoke(*state.callable);
                    }
                    catch (...) {
                        state.exception = std::current_exception();
                    }
                    state.completed = true;
                },
                &state
            );

            validateInvocation(invoked, state.completed, state.exception);
        }
        else if constexpr (std::is_lvalue_reference_v<Result>) {
            using Referent = std::remove_reference_t<Result>;

            struct State
            {
                Callable* callable;
                std::optional<std::reference_wrapper<Referent>> result;
                std::exception_ptr exception;
                bool completed = false;
            } state {std::addressof(fn), {}, {}, false};

            const bool invoked = invokeSyncWithGILReleased(
                [](void* context) {
                    auto& state = *static_cast<State*>(context);
                    try {
                        state.result = std::ref(std::invoke(*state.callable));
                    }
                    catch (...) {
                        state.exception = std::current_exception();
                    }
                    state.completed = true;
                },
                &state
            );

            validateInvocation(invoked, state.completed, state.exception);
            if (!state.result) {
                throw std::logic_error("Main-thread callable completed without a result");
            }

            return state.result->get();
        }
        else {
            using StoredResult = std::remove_cv_t<Result>;

            struct State
            {
                Callable* callable;
                std::optional<StoredResult> result;
                std::exception_ptr exception;
                bool completed = false;
            } state {std::addressof(fn), {}, {}, false};

            const bool invoked = invokeSyncWithGILReleased(
                [](void* context) {
                    auto& state = *static_cast<State*>(context);
                    try {
                        state.result.emplace(std::invoke(*state.callable));
                    }
                    catch (...) {
                        state.exception = std::current_exception();
                    }
                    state.completed = true;
                },
                &state
            );

            validateInvocation(invoked, state.completed, state.exception);
            if (!state.result) {
                throw std::logic_error("Main-thread callable completed without a result");
            }

            // Supports both move-only and copy-only result types.
            return std::move_if_noexcept(*state.result);
        }
    }

private:
    static bool invokeSync(TaskFn task, void* context);
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

}  // namespace detail

// Wrapper that mirrors fastsignals::signal and executes slots on the GUI thread
// when hooks are installed; otherwise slots run on the calling thread.
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

        auto caps = std::make_tuple(
            detail::captureSignalArg<typename ::fastsignals::signal_arg_t<Arguments>>(args)...
        );

        return MainThreadSignalConfig::callOnMainThreadSync(
            [self, caps = std::move(caps)]() mutable -> result_type {
                return std::apply(
                    [self](auto&... captured) -> result_type {
                        return self->sig_(captured.get()...);
                    },
                    caps
                );
            }
        );
    }

    mutable base_sig sig_;
};

}  // namespace App

#endif  // APP_MAINTHREADSIGNAL_H
