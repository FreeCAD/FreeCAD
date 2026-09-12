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

#ifndef BASE_FILELOCK_H
#define BASE_FILELOCK_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>

#ifndef FC_GLOBAL_H
# include <FCGlobal.h>
#endif

namespace Base
{

/**
 * Small cross-platform advisory file lock.
 *
 * This takes and holds an exclusive lock on a given path. The file is created if it does not
 * exist and removed again on unlock.
 *
 * Note: this is an advisory lock; it does not implement "stale lock" handling (PID checking, etc).
 * On Emscripten/WASM no lock is available, so locking always fails with Failure::Unavailable.
 */
class BaseExport FileLock
{
public:
    /// Why the most recent tryLock() or lock() call did not acquire the lock.
    enum class Failure : std::uint8_t
    {
        /// Another process holds the lock.
        Contended,

        /// No lock can be established here: the lock file cannot be created or the filesystem
        /// does not support locking.
        Unavailable
    };

    explicit FileLock(std::string path);
    ~FileLock();

    FileLock(const FileLock&) = delete;
    FileLock(FileLock&&) = delete;
    FileLock& operator=(const FileLock&) = delete;
    FileLock& operator=(FileLock&&) = delete;

    /// Keep trying for up to @p timeout; a zero timeout makes a single attempt.
    bool tryLock(std::chrono::milliseconds timeout);
    /// Keep trying until the lock is acquired.
    bool lock();
    void unlock() noexcept;
    bool isLocked() const;
    std::optional<Failure> lastFailure() const;

private:
    std::optional<Failure> tryLockUntil(std::chrono::steady_clock::time_point deadline);

    std::string _path;
    std::optional<Failure> _failure;
    bool _locked {false};

#if defined(_WIN32)
    void* _handle;
    std::wstring _widePath;
#elif !defined(__EMSCRIPTEN__)
    int _fd {-1};
#endif
};

}  // namespace Base

#endif  // BASE_FILELOCK_H
