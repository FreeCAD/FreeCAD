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

#ifndef BASE_FILELOCK_H
#define BASE_FILELOCK_H

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
 * exist.
 *
 * Note: this is an advisory lock; it does not implement "stale lock" handling (PID checking, etc).
 * On Emscripten/WASM this is a no-op (single-process), and always succeeds.
 */
class BaseExport FileLock
{
public:
    explicit FileLock(std::string path);
    ~FileLock();

    FileLock(const FileLock&) = delete;
    FileLock(FileLock&&) = delete;
    FileLock& operator=(const FileLock&) = delete;
    FileLock& operator=(FileLock&&) = delete;

    bool tryLock(int timeoutMs);
    bool lock();
    void unlock();
    bool isLocked() const;

private:
    std::string _path;

#if defined(__EMSCRIPTEN__)
    bool _locked {false};
#elif defined(_WIN32)
    void* _handle {nullptr};
    bool _locked {false};
#else
    int _fd {-1};
    bool _locked {false};
#endif
};

}  // namespace Base

#endif  // BASE_FILELOCK_H
