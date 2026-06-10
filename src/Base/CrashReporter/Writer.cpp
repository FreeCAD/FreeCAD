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
#include <climits>
#include <csignal>
#include <cstring>
#include <fstream>
#include <string>
#include <string_view>

#include <Build/Version.h>
#include <FCConfig.h>

// NOLINTBEGIN(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)

#ifdef FC_HAVE_CPPTRACE
#include <cpptrace/cpptrace.hpp>
#endif

static std::atomic_flag writing;

#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
constexpr size_t MaxPathLength = PATH_MAX;
static char crashReportFilePOSIX[MaxPathLength];
static char alternateStack[64 * 1024]; // Used in the event of a SIGSEGV stack overrun
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
    std::uint32_t stringPosition = stringTablePosition;
    std::uint16_t stringLength = string.length();
    std::memcpy(&stringTable[stringTablePosition], &stringLength, sizeof(std::uint16_t));
    stringTablePosition += sizeof(std::uint16_t);
    std::memcpy(&stringTable[stringTablePosition], string.data(), string.length());
    stringTablePosition += stringLength;
    return stringPosition;
}

std::uint32_t finishHeader(std::uint32_t frameCount)
{
    header.frameCount = frameCount;

    header.frameTableOffset  = sizeof(Header);
    header.stringTableOffset = sizeof(Header) + frameCount * sizeof(Frame);
    std::uint32_t fileSize = header.stringTableOffset + stringTablePosition + sizeof(Footer);
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
}

#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
extern "C" {

void writeRawBufferPOSIX(std::uint32_t fileSize) {
    // Raw syscalls only!!
    int fd = open(crashReportFilePOSIX, O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (fd != -1) {
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
void crashHandler(int sig, siginfo_t* info, [[maybe_unused]] void* ucontext)
{
    if (writing.test_and_set()) {
        return;
    }
    header.faultAddress = reinterpret_cast<std::uint64_t>(info->si_addr);
    header.code = static_cast<uint32_t>(sig);
    header.timestamp = std::time(nullptr);
    header.threadID = static_cast<std::uint64_t>(pthread_self());

    // Now the call stack, if we have cpptrace:
    std::uint32_t frameCount = 0;
#ifdef FC_HAVE_CPPTRACE
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
        std::memcpy(&fileBuffer[sizeof(Header) + frameCount * sizeof(Frame)],
            &extractedFrame, sizeof(Frame));
        ++frameCount;
    }
#endif
    std::uint32_t fileSize = finishHeader(frameCount);
    if (fileSize > 0) {
        writeRawBufferPOSIX(fileSize);
    }

    std::signal(sig, SIG_DFL);  // Make sure to reset, or we infinite loop
    std::raise(sig);
}
} // extern "C"
#elif defined(FC_OS_WIN32)
namespace
{
void writeRawBufferWindows(std::uint32_t fileSize)
{
    // This does NOT have to be async-signal-safe, it only runs on Windows
    std::ofstream fcrashFile(crashReportFileWindows, std::ios::binary);
    if (!fcrashFile) {
        return;
    }
    fcrashFile.write(fileBuffer, fileSize);
    fcrashFile.close();
}
}

void Writer::handleException(_EXCEPTION_POINTERS* exceptionInfo)
{
    if (writing.test_and_set()) {
        return;
    }
    header.code = exceptionInfo->ExceptionRecord->ExceptionCode;
    header.faultAddress = reinterpret_cast<uint64_t>(exceptionInfo->ExceptionRecord->ExceptionAddress);
    header.threadID = GetCurrentThreadId();
    header.timestamp = std::time(nullptr);

    std::uint32_t frameCount = 0;
#ifdef FC_HAVE_CPPTRACE
    // Windows SEH is NOT async-signal-safe anyway, so no need to take the long path we do on POSIX:
    constexpr std::size_t skip {0};  // Don't skip any frames at this stage
    auto trace = cpptrace::generate_object_trace(skip);
    for (const auto &objectFrame : trace) {
        Frame extractedFrame;
        extractedFrame.rawAddress = objectFrame.raw_address;
        extractedFrame.moduleOffset = objectFrame.object_address;
        extractedFrame.moduleStringOffset = addToStringTable(objectFrame.object_path);
        std::memcpy(&fileBuffer[sizeof(Header) + frameCount * sizeof(Frame)],
            &extractedFrame, sizeof(Frame));
        ++frameCount;
        if (frameCount >= MaxFrames) {
            break;
        }
    }
#endif
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

#endif


void Writer::prewarm()
{
#ifdef FC_HAVE_CPPTRACE

    // Pre-emptively call the two functions we will need in the event of a crash to ensure that
    // any needed dynamic loading mechanism is run (we can't let those loads run during a signal
    // handler).

#ifdef FC_OS_WIN32
    [[maybe_unused]] auto trace = cpptrace::generate_object_trace();
#else
    cpptrace::frame_ptr buffer[MaxFrames];
    auto frameCount = cpptrace::safe_generate_raw_trace(buffer, MaxFrames, 0);

    cpptrace::safe_object_frame frame;
    if (frameCount > 0) {
        cpptrace::get_safe_object_frame(buffer[0], &frame);
    }

    // Record whether we will be doing a signal-safe capture
    if (cpptrace::can_signal_safe_unwind() && cpptrace::can_get_safe_object_frame()) {
        header.flags |= Flags::CaptureWasSignalSafe;
    }
#endif
#endif
}

void Writer::install(const std::string& crashReportDirectory)
{
    header.freecadVersionMajor = extractVersionComponent(FCVersionMajor);
    header.freecadVersionMinor = extractVersionComponent(FCVersionMinor);
    header.freecadVersionPatch = extractVersionComponent(FCVersionPoint);
    header.freecadVersionSuffixStringOffset = addToStringTable(FCVersionSuffix);
    header.buildIDStringOffset = addToStringTable(FCRevision);
#if defined(FC_OS_WIN32)
    header.processID = GetCurrentProcessId();
#else
    header.processID = getpid();
#endif

#if defined(FC_OS_MACOSX)
    header.osID = OS::macOS;
#elif defined (FC_OS_WIN32)
    header.osID = OS::Windows;
#elif defined (FC_OS_LINUX)
    header.osID = OS::Linux;
#endif

#if defined(__x86_64__) || defined(__amd64__) || defined(_M_X64)
    header.architectureID = Architecture::x64;
#elif defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    header.architectureID = Architecture::aarch64;
#endif

#if defined(FC_OS_WIN32)
    char separator = '\\';
#else
    char separator = '/';
#endif

    FileInfo info(crashReportDirectory);
    if (!info.createDirectories()) {
        Console().warning("CrashReporter: Failed to create %s\n", crashReportDirectory);
        return;
    }

    auto timestamp = std::time(nullptr);
    std::string fcrash = crashReportDirectory + separator + "crash-" +
        std::to_string(timestamp) + "-" + std::to_string(header.processID) + ".fcrash";
    if (fcrash.length() > MaxPathLength-1) {
        Console().warning("CrashReporter: Crash file path too long: %s\n", fcrash);
        return;
    }

#ifdef FC_OS_WIN32
    Base::FileInfo fi(fcrash);
    crashReportFileWindows = fi.toStdWString();
#else
    std::memcpy(crashReportFilePOSIX, fcrash.data(), fcrash.length());
    crashReportFilePOSIX[fcrash.length()] = '\0';
#endif

    // On POSIX systems, if a SEGFAULT was triggered because we ran out of space on the stack,
    // it's possible to use an alternate stack for the signal handler. If we don't then, the attempt
    // to process the signal would itself trigger a secondary fault, causing an immediate abort.
#if defined(FC_OS_LINUX) || defined(FC_OS_MACOSX)
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
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK; // Because we want the three-arg form
    sigemptyset(&sa.sa_mask);
    for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
        sigaddset(&sa.sa_mask, sig);
    }
    for (int sig : {SIGSEGV, SIGABRT, SIGBUS, SIGFPE, SIGILL}) {
        sigaction(sig, &sa, nullptr);
    }

#endif
}

// NOLINTEND(modernize-avoid-c-arrays,cppcoreguidelines-avoid-c-arrays,cppcoreguidelines-pro-bounds-constant-array-index,readability-magic-numbers,cppcoreguidelines-avoid-magic-numbers,clang-diagnostic-unsafe-buffer-usage)
