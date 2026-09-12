// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2026 FreeCAD Contributors
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

#include "FileLock.h"
#include "FileInfo.h"

#include <algorithm>
#include <chrono>
#include <thread>

bool Base::FileLock::tryLock(std::chrono::milliseconds timeout)
{
    return tryLockUntil(std::chrono::steady_clock::now() + timeout);
}

bool Base::FileLock::lock()
{
    return tryLockUntil(std::chrono::steady_clock::time_point::max());
}

#if defined(__EMSCRIPTEN__)

// No locks on Emscripten, these are all just no-ops

using namespace Base;

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{}

FileLock::~FileLock() = default;

bool FileLock::tryLockUntil(std::chrono::steady_clock::time_point /*deadline*/)
{
    _locked = true;
    return true;
}

void FileLock::unlock() noexcept
{
    _locked = false;
}

bool FileLock::isLocked() const
{
    return _locked;
}

FileLock::Failure FileLock::lastFailure() const
{
    return _failure;
}

#elif defined(_WIN32)

# include <windows.h>

using namespace Base;

namespace
{
constexpr std::chrono::milliseconds pollInterval {10};

HANDLE openLockFile(const std::wstring& path, DWORD creationDisposition)
{
    return CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        creationDisposition,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
}

/// Make sure that the handle we have still refers to the same file: this is the Windows equivalent
/// of the POSIX race-prevention test suggested by https://stackoverflow.com/a/18745264
bool handleRefersToPath(HANDLE handle, const std::wstring& path)
{
    HANDLE byPath = openLockFile(path, OPEN_EXISTING);
    if (byPath == INVALID_HANDLE_VALUE) {
        return false;
    }
    BY_HANDLE_FILE_INFORMATION held {};
    BY_HANDLE_FILE_INFORMATION named {};
    const bool same = GetFileInformationByHandle(handle, &held)
        && GetFileInformationByHandle(byPath, &named)
        && held.dwVolumeSerialNumber == named.dwVolumeSerialNumber
        && held.nFileIndexHigh == named.nFileIndexHigh && held.nFileIndexLow == named.nFileIndexLow;
    CloseHandle(byPath);
    return same;
}


void removeLockFile(HANDLE handle, const std::wstring& path)
{
    FILE_DISPOSITION_INFO_EX disposition {};
    disposition.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS;
    if (!SetFileInformationByHandle(handle, FileDispositionInfoEx, &disposition, sizeof(disposition))) {
        // POSIX_SEMANTICS unlinks immediately; the legacy fallback only marks the file for deletion
        // once every handle is closed. For our purposes that will generally be fine, though.
        (void)DeleteFileW(path.c_str());
    }
}

bool sleepUntil(std::chrono::steady_clock::time_point deadline)
{
    const auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining <= std::chrono::milliseconds::zero()) {
        return false;
    }
    std::this_thread::sleep_for(
        std::min(pollInterval, std::chrono::duration_cast<std::chrono::milliseconds>(remaining))
    );
    return std::chrono::steady_clock::now() <= deadline;
}
}  // namespace

FileLock::FileLock(std::string path)
    : _path(std::move(path))
    , _handle(INVALID_HANDLE_VALUE)
{}

FileLock::~FileLock()
{
    unlock();
}

bool FileLock::tryLockUntil(std::chrono::steady_clock::time_point deadline)
{
    if (_locked) {
        return true;
    }
    _failure = Failure::None;

    const std::wstring wpath = FileInfo(_path).toStdWString();

    while (true) {
        HANDLE handle = openLockFile(wpath, OPEN_ALWAYS);
        if (handle == INVALID_HANDLE_VALUE) {
            _failure = Failure::Unavailable;
            return false;
        }

        OVERLAPPED ov {};
        if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov)) {
            if (handleRefersToPath(handle, wpath)) {
                _handle = handle;
                _widePath = wpath;
                _locked = true;
                return true;
            }
            // The name moved on to a new file while we waited; this is *not* contention, retry now.
            (void)UnlockFileEx(handle, 0, 1, 0, &ov);
            CloseHandle(handle);
            continue;
        }

        const DWORD err = GetLastError();
        CloseHandle(handle);
        if (err != ERROR_LOCK_VIOLATION) {
            _failure = Failure::Unavailable;
            return false;
        }
        _failure = Failure::Contended;

        if (!sleepUntil(deadline)) {
            return false;
        }
    }
}

void FileLock::unlock() noexcept
{
    if (!_locked) {
        return;
    }
    if (const auto handle = _handle; handle != INVALID_HANDLE_VALUE) {
        // Remove the name while still holding the lock so no waiter can acquire a lock on a file
        // that is about to lose its name.
        removeLockFile(handle, _widePath);
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

FileLock::Failure FileLock::lastFailure() const
{
    return _failure;
}

#else

# include <cerrno>

# include <fcntl.h>
# include <sys/stat.h>
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

// A descriptor outlives the name it was opened by, so a lock only counts while the path still
// leads to the same inode: the previous holder may have unlinked it while we waited. See e.g.
// https://stackoverflow.com/a/18745264
bool stillNamedByPath(int fd, const std::string& path)
{
    struct stat held {};
    struct stat named {};
    return ::fstat(fd, &held) == 0 && ::stat(path.c_str(), &named) == 0
        && held.st_dev == named.st_dev && held.st_ino == named.st_ino;
}

bool sleepUntil(std::chrono::steady_clock::time_point deadline)
{
    const auto remaining = deadline - std::chrono::steady_clock::now();
    if (remaining <= std::chrono::milliseconds::zero()) {
        return false;
    }
    std::this_thread::sleep_for(
        std::min(pollInterval, std::chrono::duration_cast<std::chrono::milliseconds>(remaining))
    );
    return std::chrono::steady_clock::now() <= deadline;
}
}  // namespace

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{}

FileLock::~FileLock()
{
    unlock();
}

bool FileLock::tryLockUntil(std::chrono::steady_clock::time_point deadline)
{
    if (_locked) {
        return true;
    }
    _failure = Failure::None;

    while (true) {
        const int fd = ::open(_path.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            _failure = Failure::Unavailable;
            return false;
        }

        if (tryLockFd(fd)) {
            if (stillNamedByPath(fd, _path)) {
                _fd = fd;
                _locked = true;
                return true;
            }
            // The name moved on to a new inode while we waited; this is not contention, retry now.
            ::close(fd);
            continue;
        }

        const int err = errno;
        ::close(fd);
        errno = err;
        if (err != EACCES && err != EAGAIN) {
            _failure = Failure::Unavailable;
            return false;
        }
        _failure = Failure::Contended;

        if (!sleepUntil(deadline)) {
            return false;
        }
    }
}

void FileLock::unlock() noexcept
{
    if (!_locked) {
        return;
    }

    // Unlink while still holding the lock so no waiter can acquire a lock on an inode that is
    // about to lose its name; closing the descriptor then releases the lock.
    (void)::unlink(_path.c_str());
    ::close(_fd);
    _fd = -1;
    _locked = false;
}

bool FileLock::isLocked() const
{
    return _locked;
}

FileLock::Failure FileLock::lastFailure() const
{
    return _failure;
}

#endif
