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

#include <iostream>
#include <sstream>

#include "SystemHandler.h"
#include "Console.h"
#include "Exception.h"

#ifdef FC_OS_WIN32
# include <Windows.h>
#endif

#ifdef _MSC_VER  // New handler for Microsoft Visual C++ compiler
# pragma warning(disable : 4535)
# if !defined(_DEBUG) && defined(HAVE_SEH)
#  define FC_SE_TRANSLATOR
# endif

# include <new.h>
# include <eh.h>  // VC exception handling
#else             // Ansi C/C++ new handler
# include <new>
#endif

using namespace Base;

/** freecadNewHandler()
 * prints an error message and throws an exception
 */
#ifdef _MSC_VER  // New handler for Microsoft Visual C++ compiler
int __cdecl freecadNewHandler(size_t size)
{
    // throw an exception
    throw Base::MemoryException();
    return 0;
}
#else  // Ansi C/C++ new handler
static void freecadNewHandler()
{
    // throw an exception
    throw Base::MemoryException();
}
#endif

#if defined(FC_OS_LINUX)
# include <unistd.h>
# include <execinfo.h>
# include <dlfcn.h>
# include <cxxabi.h>

# include <cstdio>
# include <cstdlib>
# include <string>

# if HAVE_CONFIG_H
#  include <config.h>
# endif  // HAVE_CONFIG_H

// This function produces a stack backtrace with demangled function & method names.
static void printBacktrace([[maybe_unused]] size_t skip = 0)
{
# if defined HAVE_BACKTRACE_SYMBOLS
    void* callstack[128];
    size_t nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
    size_t nFrames = backtrace(callstack, nMaxFrames);
    char** symbols = backtrace_symbols(callstack, nFrames);

    for (size_t i = skip; i < nFrames; i++) {
        char* demangled = nullptr;
        int status = -1;
        Dl_info info;
        if (dladdr(callstack[i], &info) && info.dli_sname && info.dli_fname) {
            if (info.dli_sname[0] == '_') {
                demangled = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            }
        }

        std::stringstream str;
        if (status == 0) {
            void* offset = (void*)((char*)callstack[i] - (char*)info.dli_saddr);
            str << "#" << (i - skip) << "  " << callstack[i] << " in " << demangled << " from "
                << info.dli_fname << "+" << offset << '\n';
            free(demangled);
        }
        else {
            str << "#" << (i - skip) << "  " << symbols[i] << '\n';
        }

        // cannot directly print to cerr when using --write-log
        std::cerr << str.str();
    }

    free(symbols);
# else  // HAVE_BACKTRACE_SYMBOLS
    std::cerr << "Cannot print the stacktrace because the C runtime library doesn't provide "
                 "backtrace or backtrace_symbols\n";
# endif
}
#endif

void segmentation_fault_handler([[maybe_unused]] int sig)
{
#if defined(FC_OS_LINUX)
    std::cerr << "Program received signal SIGSEGV, Segmentation fault.\n";
    printBacktrace(2);
# if defined(FC_DEBUG)
    abort();
# else
    _exit(1);
# endif
#else
    switch (sig) {
        case SIGSEGV:
            std::cerr << "Illegal storage access..." << '\n';
# if !defined(_DEBUG)
            throw Base::AccessViolation(
                "Illegal storage access! Please save your work under a new "
                "file name and restart the application!"
            );
# endif
            break;
        case SIGABRT:
            std::cerr << "Abnormal program termination..." << '\n';
# if !defined(_DEBUG)
            throw Base::AbnormalProgramTermination("Break signal occurred");
# endif
            break;
        default:
            std::cerr << "Unknown error occurred..." << '\n';
            break;
    }
#endif  // FC_OS_LINUX
}

#if defined(_MSC_VER)
static void unhandled_exception_handler()
{
    std::cerr << "Terminating..." << '\n';
}

static void unexpected_error_handler()
{
    std::cerr << "Unexpected error occurred..." << '\n';
    // try to throw an exception and give the user chance to save their work
# if !defined(_DEBUG)
    throw Base::AbnormalProgramTermination(
        "Unexpected error occurred! Please save your work under "
        "a new file name and restart the application!"
    );
# else
    terminate();
# endif
}
#endif

#if defined(FC_SE_TRANSLATOR)  // Microsoft compiler
void my_se_translator_filter(unsigned int code, EXCEPTION_POINTERS* /*pExp*/)
{
    switch (code) {
        case EXCEPTION_ACCESS_VIOLATION:
            throw Base::AccessViolation();
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            Base::Console().error("SEH exception (%u): Division by zero\n", code);
            return;
    }

    std::stringstream str;
    str << "SEH exception of type: " << code;
    // general C++ SEH exception for things we don't need to handle separately....
    throw Base::RuntimeError(str.str());
}
#endif

void SystemHandler::installNewHandler()
{
#ifdef _MSC_VER                           // Microsoft compiler
    _set_new_handler(freecadNewHandler);  // Setup new handler
    _set_new_mode(1);                     // Re-route malloc failures to new handler !
#else                                     // Ansi compiler
    std::set_new_handler(freecadNewHandler);  // ANSI new handler
#endif
}

void SystemHandler::installSegfaultHandler()
{
    // if an unexpected crash occurs we can install a handler function to
    // write some additional information
#if defined(_MSC_VER)  // Microsoft compiler
    std::signal(SIGSEGV, segmentation_fault_handler);
    std::signal(SIGABRT, segmentation_fault_handler);
    std::set_terminate(unhandled_exception_handler);
    ::set_unexpected(unexpected_error_handler);
#elif defined(FC_OS_LINUX)
    std::signal(SIGSEGV, segmentation_fault_handler);
#endif
#if defined(FC_SE_TRANSLATOR)
    _set_se_translator(my_se_translator_filter);
#endif
}
