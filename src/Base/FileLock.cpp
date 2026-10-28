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
    _failure = tryLockUntil(std::chrono::steady_clock::now() + timeout);
    return !_failure;
}

bool Base::FileLock::lock()
{
    _failure = tryLockUntil(std::chrono::steady_clock::time_point::max());
    return !_failure;
}

bool Base::FileLock::isLocked() const
{
    return _locked;
}

std::optional<Base::FileLock::Failure> Base::FileLock::lastFailure() const
{
    return _failure;
}

#if defined(__EMSCRIPTEN__)

// No locks on Emscripten: report that and let the caller decide how to proceed

using namespace Base;

FileLock::FileLock(std::string path)
    : _path(std::move(path))
{}

FileLock::~FileLock() = default;

std::optional<FileLock::Failure> FileLock::tryLockUntil(
    std::chrono::steady_clock::time_point /*deadline*/
)
{
    return Failure::Unavailable;
}

void FileLock::unlock() noexcept
{}

#elif defined(_WIN32)

# include <windows.h>

using namespace Base;

namespace
{
constexpr std::chrono::milliseconds pollInterval {10};

/// How long to keep retrying an open that failed for a reason that is usually momentary, before
/// concluding that the lock file genuinely cannot be opened here.
constexpr std::chrono::milliseconds transientOpenWindow {1000};

HANDLE openLockFile(const std::wstring& path, DWORD creationDisposition)
{
    return CreateFileW(
        path.c_str(),
        GENERIC_READ | GENERIC_WRITE | DELETE,
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
    (void)CloseHandle(byPath);
    return same;
}

/// A file that a releasing holder has marked for deletion cannot be opened until its last handle
/// closes, and antivirus or indexing services briefly open freshly created files without sharing:
/// both fail the open even though the lock will shortly be obtainable.
bool isTransientOpenFailure(DWORD error)
{
    return error == ERROR_ACCESS_DENIED || error == ERROR_SHARING_VIOLATION;
}


void removeLockFile(HANDLE handle, const std::wstring& path)
{
    FILE_DISPOSITION_INFO_EX disposition {};
    disposition.Flags = FILE_DISPOSITION_FLAG_DELETE | FILE_DISPOSITION_FLAG_POSIX_SEMANTICS;
    if (!SetFileInformationByHandle(handle, FileDispositionInfoEx, &disposition, sizeof(disposition))) {
        // With POSIX semantics the name disappears when this handle closes, even if other handles
        // are open; the legacy fallback waits for every handle to close.
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

std::optional<FileLock::Failure> FileLock::tryLockUntil(std::chrono::steady_clock::time_point deadline)
{
    if (_locked) {
        return std::nullopt;
    }

    const std::wstring wpath = FileInfo(_path).toStdWString();
    std::optional<std::chrono::steady_clock::time_point> openRetryDeadline;

    while (true) {
        HANDLE handle = openLockFile(wpath, OPEN_ALWAYS);
        if (handle == INVALID_HANDLE_VALUE) {
            if (!isTransientOpenFailure(GetLastError())) {
                return Failure::Unavailable;
            }
            if (!openRetryDeadline) {
                openRetryDeadline
                    = std::min(deadline, std::chrono::steady_clock::now() + transientOpenWindow);
            }
            if (!sleepUntil(*openRetryDeadline)) {
                return Failure::Unavailable;
            }
            continue;
        }
        openRetryDeadline.reset();

        OVERLAPPED ov {};
        if (LockFileEx(handle, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY, 0, 1, 0, &ov)) {
            if (handleRefersToPath(handle, wpath)) {
                _handle = handle;
                _widePath = wpath;
                _locked = true;
                return std::nullopt;
            }
            // The name moved on to a new file while we waited: the previous holder just released.
            (void)UnlockFileEx(handle, 0, 1, 0, &ov);
            (void)CloseHandle(handle);
            if (!sleepUntil(deadline)) {
                return Failure::Contended;
            }
            continue;
        }

        const DWORD err = GetLastError();
        (void)CloseHandle(handle);
        if (err != ERROR_LOCK_VIOLATION) {
            return Failure::Unavailable;
        }
        if (!sleepUntil(deadline)) {
            return Failure::Contended;
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
        (void)CloseHandle(handle);
    }
    _handle = INVALID_HANDLE_VALUE;
    _locked = false;
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

std::optional<FileLock::Failure> FileLock::tryLockUntil(std::chrono::steady_clock::time_point deadline)
{
    if (_locked) {
        return std::nullopt;
    }

    while (true) {
        constexpr int openFlag = O_RDWR | O_CREAT | O_CLOEXEC | O_NOFOLLOW;
        const int fd = ::open(_path.c_str(), openFlag, S_IRUSR | S_IWUSR);
        if (fd < 0) {
            if (errno == EINTR) {
                continue;
            }
            return Failure::Unavailable;
        }

        if (tryLockFd(fd)) {
            if (stillNamedByPath(fd, _path)) {
                _fd = fd;
                _locked = true;
                return std::nullopt;
            }
            // The name moved on to a new inode while we waited: the previous holder just released.
            (void)::close(fd);
            if (!sleepUntil(deadline)) {
                return Failure::Contended;
            }
            continue;
        }

        const int err = errno;
        (void)::close(fd);
        errno = err;
        if (err == EINTR) {
            continue;
        }
        if (err != EACCES && err != EAGAIN) {
            return Failure::Unavailable;
        }
        if (!sleepUntil(deadline)) {
            return Failure::Contended;
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
    (void)::close(_fd);
    _fd = -1;
    _locked = false;
}

#endif
