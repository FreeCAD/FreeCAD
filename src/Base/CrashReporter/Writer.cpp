// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 The FreeCAD project association AISBL
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


/*******************************************************************************
 *******************************************************************************
 **                                                                           **
 **  WARNING: THIS FILE INCLUDES CODE THAT IS EXECUTED INSIDE SIGNAL          **
 **           HANDLERS AND MUST BE ASYNC-SIGNAL-SAFE. IT MUST NOT CONTAIN     **
 **           ANYTHING THAT WILL ALLOCATE ON THE HEAP DURING CONSTRUCTION     **
 **           (E.G. STL CONTAINERS).                                          **
 **                                                                           **
 *******************************************************************************
 *******************************************************************************/

#include "Writer.h"

#include "Exception.h"
#include "Format.h"
#include "Console.h"
#include "FileInfo.h"

#include <atomic>
#include <charconv>
#include <climits>  // IWYU pragma: keep
#include <csignal>
#include <cstdint>
#include <cstring>  // IWYU pragma: keep
#include <fstream>
#include <string>
#include <string_view>


#include <Build/Version.h>
#include <FCConfig.h>

#if HAVE_CONFIG_H
# include <config.h>
#endif

#ifdef HAVE_BACKTRACE_SYMBOLS
# include <dlfcn.h>     // For dladdr
# include <execinfo.h>  // For backtrace
#endif

// The Writer is intentionally C-style (fixed static buffers, raw write) for async-signal-safety,
// so array decay and pointer arithmetic are pervasive and deliberate, not bounds hazards.
// NOLINTBEGIN(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
// NOLINTBEGIN(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)

#ifdef FC_HAVE_CPPTRACE
# include <cpptrace/cpptrace.hpp>
#endif

static std::atomic_flag writing;
static std::string resolvedCrashFilePath;  // Stored in UTF-8 (so on Windows, convert first)
#ifdef FC_HAVE_CPPTRACE
static bool canCaptureSignalSafely = false;  // Determined during the prewarm phase
#endif

// Cygwin is a POSIX layer, so it uses the signal handlers rather than the Windows path.
#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX) || defined(FC_OS_BSD) || defined(FC_OS_CYGWIN)
# define FC_CRASHREPORTER_POSIX
#elif defined(FC_OS_WIN32)
# define FC_CRASHREPORTER_WINDOWS
#else
# error "CrashReporter: no crash capture strategy is implemented for this operating system"
#endif

#ifdef FC_CRASHREPORTER_POSIX
# include <fcntl.h>
# include <pthread.h>
# include <unistd.h>
constexpr size_t MaxPathLength = PATH_MAX;
static char crashReportFilePOSIX[MaxPathLength];
static char alternateStack[64 * 1024];  // Used in the event of a SIGSEGV stack overrun
#else
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# ifndef NOMINMAX
#  define NOMINMAX
# endif
# include <windows.h>
constexpr size_t MaxPathLength = MAX_PATH;
static std::wstring crashReportFileWindows;
#endif


using namespace Base::CrashReporter;

static Header header;
static char fileBuffer[MaxFileSize];
static char stringTable[MaxFileSize];
static uint32_t stringTablePosition = 0;

namespace
{
std::uint8_t extractVersionComponent(std::string_view version)
{
    std::uint8_t versionComponent = 0;
    std::from_chars(version.data(), version.data() + version.size(), versionComponent);
    return versionComponent;
}
std::uint32_t addToStringTable(std::string_view string)
{
    if (stringTablePosition + string.length() + sizeof(std::uint16_t) > MaxFileSize) {
        return NoString;
    }
    if (string.length() > MaxStringLength) {
        return NoString;
    }
    const std::uint32_t stringPosition = stringTablePosition;
    const std::uint16_t stringLength = string.length();
    std::memcpy(&stringTable[stringTablePosition], &stringLength, sizeof(std::uint16_t));
    stringTablePosition += sizeof(std::uint16_t);
    std::memcpy(&stringTable[stringTablePosition], string.data(), string.length());
    stringTablePosition += stringLength;
    return stringPosition;
}

std::uint32_t finishHeader(std::uint32_t frameCount)
{
    header.frameCount = frameCount;

    header.frameTableOffset = sizeof(Header);
    header.stringTableOffset = sizeof(Header) + (frameCount * sizeof(Frame));
    const std::uint32_t fileSize = header.stringTableOffset + stringTablePosition + sizeof(Footer);
    if (fileSize > MaxFileSize) {
        return 0;
    }
    header.fileSize = fileSize;
    std::memcpy(&fileBuffer[0], &header, sizeof(Header));
    std::memcpy(&fileBuffer[header.stringTableOffset], stringTable, stringTablePosition);
    std::uint32_t crc = crc32({fileBuffer, fileSize - sizeof(Footer)});
    std::memcpy(&fileBuffer[fileSize - sizeof(Footer)], &crc, sizeof(crc));
    return fileSize;
}
}  // namespace

#ifdef FC_CRASHREPORTER_POSIX
extern "C" {

static void writeRawBufferPOSIX(std::uint32_t fileSize)
{
    // Raw syscalls only!!
    int fd = open(crashReportFilePOSIX, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd != -1) {
# if defined(FC_OS_LINUX)
        // Advisory, and a bare syscall wrapper so it is signal-safe.
        fallocate(fd, 0, 0, fileSize);
# endif
        ssize_t written = 0;
        while (written < fileSize) {
            ssize_t r = write(fd, fileBuffer + written, fileSize - written);
            if (r <= 0) {
                break;  // Bail out, we're dying anyway
            }
            written += static_cast<std::size_t>(r);
        }
        close(fd);
    }
}

// NOTE: *ALL* Calls in crashHandler must be async-signal-safe. Verify before changing anything in
// this function.
static void crashHandler(int sig, siginfo_t* info, [[maybe_unused]] void* ucontext)
{
    if (writing.test_and_set()) {
        return;
    }

    // A fault inside this handler has to kill the process, not hang it. The earlier call to
    // install() blocks all five signals while the handler runs. But a synchronous fault on a
    // blocked signal *cannot* be delivered. However, it tries (forever). To resolve, both steps
    // below are required:
    // 1) Restore SIG_DFL for each signal process-wide so we terminate instead of hang
    // 2) Unblock our five signals on this (the currently crashing) thread.
    // Note that restoring SIG_DFL alone still hangs, and using SA_RESETHAND/SA_NODEFER doesn't
    // work since we have an explicit sa_mask entry. The downside is that a crash in another thread
    // truncates our write, but the file format was designed to let us handle that case reasonably
    // gracefully.
    sigset_t unblockAll {};
    sigemptyset(&unblockAll);
    for (int s : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
        std::signal(s, SIG_DFL);
        sigaddset(&unblockAll, s);
    }
    pthread_sigmask(SIG_UNBLOCK, &unblockAll, nullptr);

    header.faultAddress = reinterpret_cast<std::uint64_t>(info->si_addr);
    header.code = static_cast<uint32_t>(sig);
    header.timestamp = std::time(nullptr);
    // pthread_t is an integer on Linux but an opaque pointer on macOS, the BSDs, and Cygwin, so it
    // is not portably castable. macOS has a real thread ID; elsewhere record the handle value,
    // which is at least unique per thread within the process.
# if defined(FC_OS_MACOSX)
    std::uint64_t threadID {0};
    pthread_threadid_np(nullptr, &threadID);  // Reads the thread's own record, no syscall
    header.threadID = threadID;
# elif defined(FC_OS_BSD) || defined(FC_OS_CYGWIN)
    header.threadID = reinterpret_cast<std::uintptr_t>(pthread_self());
# else
    header.threadID = static_cast<std::uint64_t>(pthread_self());
# endif

    // Now the call stack, if we have cpptrace:
    std::uint32_t frameCount = 0;
# ifdef FC_HAVE_CPPTRACE
    if (canCaptureSignalSafely) {
        cpptrace::frame_ptr rawFrames[MaxFrames];
        constexpr std::size_t skip {0};  // Don't skip any frames at this stage
        std::size_t nFrames = cpptrace::safe_generate_raw_trace(rawFrames, MaxFrames, skip);
        cpptrace::safe_object_frame objectFrame;
        for (std::uint32_t frame = 0; frame < nFrames && frameCount < MaxFrames; ++frame) {
            cpptrace::get_safe_object_frame(rawFrames[frame], &objectFrame);
            Frame extractedFrame;
            extractedFrame.rawAddress = objectFrame.raw_address;
            extractedFrame.moduleOffset = objectFrame.address_relative_to_object_start;
            extractedFrame.moduleStringOffset = addToStringTable(objectFrame.object_path);
            std::memcpy(
                &fileBuffer[sizeof(Header) + frameCount * sizeof(Frame)],
                &extractedFrame,
                sizeof(Frame)
            );
            ++frameCount;
        }
    }
    else
# endif
    {
        // The capture is NOT async-signal-safe: this is mostly macOS, where we currently cannot
        // get a version of cpptrace compiled against libunwind via pixi.
# ifdef HAVE_BACKTRACE_SYMBOLS
        void* callstack[MaxFrames];
        int nFrames = backtrace(callstack, MaxFrames);
        for (int frame = 0; frame < nFrames && frameCount < MaxFrames; ++frame) {
            Dl_info frameInfoStruct {};

            // A Dl_info contains:
            // const char* dli_fname -- pathname of the shared object containing the address
            // void* dli_fbase -- base address (mach_header) at which the image is mapped
            // const char* dli_sname -- nearest run-time symbol at or below the address
            // void* dli_saddr -- value of the symbol returned in dli_sname
            const int found = dladdr(callstack[frame], &frameInfoStruct);

            Frame extractedFrame;
            extractedFrame.rawAddress = reinterpret_cast<std::uint64_t>(callstack[frame]);
            if (found == 0 /*yes, zero... not very POSIX-ish*/
                || frameInfoStruct.dli_fname == nullptr) {
                // We don't really know anything about this module (except the address)
                extractedFrame.moduleOffset = 0;
                extractedFrame.moduleStringOffset = NoString;
            }
            else {
                const auto address = reinterpret_cast<std::uintptr_t>(callstack[frame]);
                const auto base = reinterpret_cast<std::uintptr_t>(frameInfoStruct.dli_fbase);
                extractedFrame.moduleOffset = address - base;
                extractedFrame.moduleStringOffset = addToStringTable(frameInfoStruct.dli_fname);
            }
            std::memcpy(
                &fileBuffer[sizeof(Header) + frameCount * sizeof(Frame)],
                &extractedFrame,
                sizeof(Frame)
            );
            ++frameCount;
        }
# endif
    }
    std::uint32_t fileSize = finishHeader(frameCount);
    if (fileSize > 0) {
        writeRawBufferPOSIX(fileSize);
    }

    std::signal(sig, SIG_DFL);  // Make sure to reset, or we infinite loop
    std::raise(sig);
}
}  // extern "C"
#elif defined(_MSC_VER)  // Must match the guard on Writer::handleException in Writer.h
namespace
{
void writeRawBufferWindows(std::uint32_t fileSize)
{
    // This does NOT have to be async-signal-safe, it only runs on Windows
    std::ofstream fcrashFile(crashReportFileWindows, std::ios::binary);
    if (!fcrashFile) {
        return;
    }
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
    fcrashFile.write(fileBuffer, fileSize);
    fcrashFile.close();
}
}  // namespace

void Writer::handleException(_EXCEPTION_POINTERS* exceptionInfo)
{
    if (writing.test_and_set()) {
        return;
    }
    const auto* record = exceptionInfo->ExceptionRecord;
    header.code = record->ExceptionCode;
    // The faulting data address is only defined for these two codes; the faulting instruction is
    // frame 0 of the captured stack.
    if (record->ExceptionCode == EXCEPTION_ACCESS_VIOLATION
        || record->ExceptionCode == EXCEPTION_IN_PAGE_ERROR) {
        header.faultAddress = record->ExceptionInformation[1];
    }
    header.threadID = GetCurrentThreadId();
    header.timestamp = std::time(nullptr);

    std::uint32_t frameCount = 0;
# ifdef FC_HAVE_CPPTRACE
    // Windows SEH is NOT async-signal-safe anyway, so no need to take the long path we do on POSIX:
    constexpr std::size_t skip {0};  // Don't skip any frames at this stage
    auto trace = cpptrace::generate_object_trace(skip);
    for (const auto& objectFrame : trace) {
        Frame extractedFrame;
        extractedFrame.rawAddress = objectFrame.raw_address;
        extractedFrame.moduleOffset = objectFrame.object_address;
        extractedFrame.moduleStringOffset = addToStringTable(objectFrame.object_path);
        std::memcpy(
            &fileBuffer[sizeof(Header) + (frameCount * sizeof(Frame))],
            &extractedFrame,
            sizeof(Frame)
        );
        ++frameCount;
        if (frameCount >= MaxFrames) {
            break;
        }
    }
# endif
    std::uint32_t fileSize = finishHeader(frameCount);
    if (fileSize == 0) {
        return;
    }
    writeRawBufferWindows(fileSize);
}

void Writer::setMinidumpPath(const std::string& path)
{
    if (path.length() > MaxPathLength) {
        Console().warning("CrashReporter: Path too long: %s\n", path);
        return;
    }
    header.minidumpPathStringOffset = addToStringTable(path);
}

#else
// MinGW: no writer, the capture path needs MSVC structured exception handling.
#endif


void Writer::prewarm()
{
#ifdef FC_HAVE_CPPTRACE

    // Pre-emptively call the two functions we will need in the event of a crash to ensure that
    // any needed dynamic loading mechanism is run (we can't let those loads run during a signal
    // handler).

# ifdef FC_CRASHREPORTER_WINDOWS
    [[maybe_unused]] auto trace = cpptrace::generate_object_trace();
# else
    cpptrace::frame_ptr buffer[MaxFrames];
    auto frameCount = cpptrace::safe_generate_raw_trace(buffer, MaxFrames, 0);

    cpptrace::safe_object_frame frame;
    if (frameCount > 0) {
        cpptrace::get_safe_object_frame(buffer[0], &frame);
    }

    // Record whether we will be doing a signal-safe capture
    canCaptureSignalSafely = cpptrace::can_signal_safe_unwind()
        && cpptrace::can_get_safe_object_frame();
    if (canCaptureSignalSafely) {
        header.flags |= Flags::CaptureWasSignalSafe;
    }
# endif
#endif
}

void Writer::install(const std::string& crashReportDirectory)
{
    header.freecadVersionMajor = extractVersionComponent(FCVersionMajor);
    header.freecadVersionMinor = extractVersionComponent(FCVersionMinor);
    header.freecadVersionPatch = extractVersionComponent(FCVersionPoint);
    header.freecadVersionSuffixStringOffset = addToStringTable(FCVersionSuffix);
#ifdef FCRepositoryHash
    header.buildIDStringOffset = addToStringTable(FCRepositoryHash);
#else
    header.buildIDStringOffset = addToStringTable(FCRevision);
#endif
#ifdef FC_CRASHREPORTER_WINDOWS
    header.processID = GetCurrentProcessId();
#else
    header.processID = getpid();
#endif

#ifdef FC_OS_MACOSX
    header.osID = OS::macOS;
#elif defined(FC_OS_WIN32)
    header.osID = OS::Windows;
#elif defined(FC_OS_LINUX)
    header.osID = OS::Linux;
#elif defined(FC_OS_BSD)
    header.osID = OS::BSDFamily;
#else
    // Cygwin has no enumerator, so it keeps the initialized OS::None.
#endif

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
    header.architectureID = Architecture::x64;
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    header.architectureID = Architecture::aarch64;
#else
    // Not an error: 32-bit ARM, riscv64, and ppc64le builds keep the initialized
    // Architecture::None rather than failing to compile.
#endif

    constexpr char separator = PATHSEP;

    if (FileInfo info(crashReportDirectory); !info.createDirectories()) {
        Console().warning("CrashReporter: Failed to create %s\n", crashReportDirectory);
        return;
    }

    const auto timestamp = std::time(nullptr);
    std::string fcrash = crashReportDirectory + separator + "crash-" + std::to_string(timestamp)
        + "-" + std::to_string(header.processID) + ".fcrash";
    if (fcrash.length() > MaxPathLength - 1) {
        Console().warning("CrashReporter: Crash file path too long: %s\n", fcrash);
        return;
    }
    resolvedCrashFilePath = fcrash;

#ifdef FC_CRASHREPORTER_WINDOWS
    const FileInfo fi(fcrash);
    crashReportFileWindows = fi.toStdWString();
#else
    std::memcpy(crashReportFilePOSIX, fcrash.data(), fcrash.length());
    crashReportFilePOSIX[fcrash.length()] = '\0';
#endif

    // On POSIX systems, if a SEGFAULT was triggered because we ran out of space on the stack,
    // it's possible to use an alternate stack for the signal handler. If we don't then, the attempt
    // to process the signal would itself trigger a secondary fault, causing an immediate abort.
#ifdef FC_CRASHREPORTER_POSIX
    // https://man7.org/linux/man-pages/man2/sigaltstack.2.html
    stack_t ss {};
    ss.ss_sp = alternateStack;
    ss.ss_flags = 0;
    ss.ss_size = sizeof(alternateStack);
    if (sigaltstack(&ss, nullptr) != 0) {
        Console().warning("CrashReporter: sigaltstack failed; crash capture disabled\n");
        return;
    }

    // https://man7.org/linux/man-pages/man2/sigaction.2.html
    struct sigaction sa {};
    sa.sa_sigaction = &crashHandler;
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;  // Because we want the three-arg form
    sigemptyset(&sa.sa_mask);
    for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
        sigaddset(&sa.sa_mask, sig);
    }
    for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
        sigaction(sig, &sa, nullptr);
    }

#endif
}

std::string Writer::crashReportFilePath()
{
    return resolvedCrashFilePath;
}

// NOLINTEND(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)
// NOLINTEND(cppcoreguidelines-pro-bounds-pointer-arithmetic,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
