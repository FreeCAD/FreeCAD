// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2026                                                   *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

#include "FileLock.h"

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <thread>

#if defined(__EMSCRIPTEN__)

using namespace Base;

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{}

FileLock::~FileLock() = default;

bool FileLock::tryLock(int /*timeoutMs*/)
{
    _locked = true;
    return true;
}

bool FileLock::lock()
{
    _locked = true;
    return true;
}

void FileLock::unlock()
{
    _locked = false;
}

bool FileLock::isLocked() const
{
    return _locked;
}

#elif defined(_WIN32)

# include <windows.h>

using namespace Base;

namespace
{
constexpr std::chrono::milliseconds pollInterval {10};
}  // namespace

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{
    _handle = INVALID_HANDLE_VALUE;
}

FileLock::~FileLock()
{
    unlock();
}

bool FileLock::tryLock(int timeoutMs)
{
    if (_locked) {
        return true;
    }

    const std::filesystem::path p = std::filesystem::u8path(_path);
    const std::wstring wpath = p.wstring();

    const bool infinite = timeoutMs < 0;
    const auto deadline = infinite
        ? std::chrono::steady_clock::time_point::max()
        : (std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs));

    for (;;) {
        HANDLE handle = static_cast<HANDLE>(_handle);
        if (handle == INVALID_HANDLE_VALUE) {
            handle = CreateFileW(
                wpath.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                nullptr,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                nullptr
            );
            if (handle == INVALID_HANDLE_VALUE) {
                return false;
            }
            _handle = handle;
        }

        OVERLAPPED ov {};
        if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov)) {
            _locked = true;
            return true;
        }

        const DWORD err = GetLastError();
        if (err != ERROR_LOCK_VIOLATION) {
            CloseHandle(handle);
            _handle = INVALID_HANDLE_VALUE;
            break;
        }

        if (timeoutMs == 0) {
            break;
        }

        if (infinite) {
            std::this_thread::sleep_for(pollInterval);
        }
        else {
            const auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::milliseconds::zero()) {
                break;
            }
            std::this_thread::sleep_for(
                std::min(pollInterval, std::chrono::duration_cast<std::chrono::milliseconds>(remaining))
            );
        }

        if (!infinite && std::chrono::steady_clock::now() > deadline) {
            break;
        }
    }

    const auto handle = static_cast<HANDLE>(_handle);
    if (handle != INVALID_HANDLE_VALUE) {
        CloseHandle(handle);
    }
    _handle = INVALID_HANDLE_VALUE;
    return false;
}

bool FileLock::lock()
{
    return tryLock(-1);
}

void FileLock::unlock()
{
    if (!_locked) {
        return;
    }
    auto handle = static_cast<HANDLE>(_handle);
    if (handle != INVALID_HANDLE_VALUE) {
        OVERLAPPED ov {};
        (void)UnlockFileEx(handle, 0, 1, 0, &ov);
        CloseHandle(handle);
    }
    _handle = INVALID_HANDLE_VALUE;
    _locked = false;
}

bool FileLock::isLocked() const
{
    return _locked;
}

#else

# include <cerrno>

# include <fcntl.h>
# include <unistd.h>

using namespace Base;

namespace
{
constexpr std::chrono::milliseconds pollInterval {10};

bool tryLockFd(int fd)
{
    struct flock fl {};
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // whole file

    return ::fcntl(fd, F_SETLK, &fl) == 0;
}
}  // namespace

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{}

FileLock::~FileLock()
{
    unlock();
}

bool FileLock::tryLock(int timeoutMs)
{
    if (_locked) {
        return true;
    }

    if (_fd < 0) {
        _fd = ::open(_path.c_str(), O_RDWR | O_CREAT, 0666);
        if (_fd < 0) {
            return false;
        }
    }

    const bool infinite = timeoutMs < 0;
    const auto deadline = infinite
        ? std::chrono::steady_clock::time_point::max()
        : (std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs));

    for (;;) {
        if (tryLockFd(_fd)) {
            _locked = true;
            return true;
        }

        if (errno != EACCES && errno != EAGAIN) {
            break;
        }

        if (timeoutMs == 0) {
            break;
        }

        if (infinite) {
            std::this_thread::sleep_for(pollInterval);
        }
        else {
            const auto remaining = deadline - std::chrono::steady_clock::now();
            if (remaining <= std::chrono::milliseconds::zero()) {
                break;
            }
            std::this_thread::sleep_for(
                std::min(pollInterval, std::chrono::duration_cast<std::chrono::milliseconds>(remaining))
            );
        }

        if (!infinite && std::chrono::steady_clock::now() > deadline) {
            break;
        }
    }

    const int err = errno;
    ::close(_fd);
    _fd = -1;
    errno = err;
    return false;
}

bool FileLock::lock()
{
    return tryLock(-1);
}

void FileLock::unlock()
{
    if (!_locked) {
        return;
    }

    struct flock fl {};
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;  // whole file
    (void)::fcntl(_fd, F_SETLK, &fl);

    ::close(_fd);
    _fd = -1;
    _locked = false;
}

bool FileLock::isLocked() const
{
    return _locked;
}

#endif
