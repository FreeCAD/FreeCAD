// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *                                                                         *
 *   Copyright (c) 2026 The FreeCAD project association AISBL              *
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

#include "SignalException.h"
#include <FCConfig.h>

#if defined(__GNUC__) && (defined(FC_OS_LINUX) || defined(FC_OS_MACOSX))
# include <array>
# include <csetjmp>
# include <csignal>
# include <mutex>
# include <regex>
# include <string>
# include <boost/core/demangle.hpp>
# include <OSD.hxx>
# include <Standard_ErrorHandler.hxx>
# include <Standard_Failure.hxx>
#endif

using namespace Part;

// NOLINTBEGIN(concurrency-mt-unsafe)
#if defined(__GNUC__) && (defined(FC_OS_LINUX) || defined(FC_OS_MACOSX))
/// Replace mangled C++ symbols (_Z...) in a stack trace with demangled names.
static std::string demangleStackTrace(const char* trace)
{
    std::string result;
    // Match mangled symbols: _Z followed by alphanumeric/underscore characters
    std::regex mangledPattern("(_Z[A-Za-z0-9_]+)");
    const char* pos = trace;
    std::cmatch match;
    while (std::regex_search(pos, match, mangledPattern)) {
        result.append(pos, match[0].first);
        std::string demangled = boost::core::demangle(match[0].str().c_str());
        result.append(demangled);
        pos = match[0].second;
    }
    result.append(pos);
    return result;
}
#endif

void SignalException::guard(std::function<void()> fn)
{
#if defined(__GNUC__) && (defined(FC_OS_LINUX) || defined(FC_OS_MACOSX))
    // Process-wide nesting counter and mutex. Only the outermost guard()
    // installs/uninstalls signal handlers. The mutex ensures the depth check
    // and OSD::SetSignal call are atomic - without it, a second thread could
    // enter guard() and run fn() before the first thread finishes installing
    // handlers.
    // Signals that OSD::SetSignal() installs handlers for.
    static constexpr std::array guardedSignals = {
        SIGFPE,
        SIGHUP,
        SIGINT,
        SIGQUIT,
        SIGILL,
        SIGBUS,
# ifdef SIGSYS
        SIGSYS,
# endif
        SIGSEGV
    };

    static int guardDepth = 0;
    static std::mutex guardMutex;
    static constexpr int stackTraceDepth = 50;
    // Snapshot of signal handlers before the outermost guard() installed
    // OCC's handlers, so we can restore them when the last guard() exits.
    static std::array<struct sigaction, guardedSignals.size()> savedHandlers = {};

    {
        std::lock_guard lock(guardMutex);
        if (guardDepth++ == 0) {
            for (size_t i = 0; i < guardedSignals.size(); ++i) {
                sigaction(guardedSignals[i], nullptr, &savedHandlers[i]);
            }
            OSD::SetSignalStackTraceLength(stackTraceDepth);
            OSD::SetSignal(OSD_SignalMode_Set, Standard_False);
        }
    }

    // setjmp returns 0 on initial call (normal path). If a signal fires
    // during fn(), OCC's handler calls longjmp back here with a non-zero
    // value, entering the else branch to re-raise as a C++ exception.
    Standard_ErrorHandler handler;
    if (setjmp(handler.Label()) == 0) {
        fn();
    }
    else {
        {
            std::lock_guard lock(guardMutex);
            if (--guardDepth == 0) {
                for (size_t i = 0; i < guardedSignals.size(); ++i) {
                    sigaction(guardedSignals[i], &savedHandlers[i], nullptr);
                }
            }
        }
        handler.Catches(STANDARD_TYPE(Standard_Failure));
        auto error = handler.Error();
        if (const char* stack = error->GetStackString()) {
            error->SetStackString(demangleStackTrace(stack).c_str());
        }
        error->Reraise();
    }

    {
        std::lock_guard lock(guardMutex);
        if (--guardDepth == 0) {
            for (size_t i = 0; i < guardedSignals.size(); ++i) {
                sigaction(guardedSignals[i], &savedHandlers[i], nullptr);
            }
        }
    }
#else
    fn();
#endif
}
// NOLINTEND(concurrency-mt-unsafe)
