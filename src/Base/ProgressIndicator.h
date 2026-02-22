// SPDX-License-Identifier: LGPL-2.1-or-later
/***************************************************************************
 *   Copyright (c) 2025 James Stanley <james@incoherency.co.uk>            *
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

#pragma once

#include "FCGlobal.h"

namespace Base
{

/**
 * @class ProgressIndicator
 * @brief Base interface for reporting and controlling long-running operations.
 *
 * This abstract base class provides a uniform way to:
 *   - Display progress updates via @c show().
 *   - Query whether the user has requested cancellation via @c userBreak().
 *
 * By default:
 *   - @c userBreak() returns false (no cancellation).
 *   - @c show() is a no-op.
 *
 * Derived classes should override these methods to integrate with
 * specific UI frameworks or console output.
 */
class BaseExport ProgressIndicator
{
public:
    /**
     * @brief Flags to control behavior of the show() update.
     */
    enum class ShowFlags
    {
        None = 0,  ///< Default behavior; update only if significant progress.
        Force = 1  ///< Force update regardless of progress threshold.
    };

    ProgressIndicator();
    ProgressIndicator(const ProgressIndicator&);
    ProgressIndicator(ProgressIndicator&&) = delete;
    ProgressIndicator& operator=(const ProgressIndicator&);
    ProgressIndicator& operator=(ProgressIndicator&&) = delete;
    virtual ~ProgressIndicator();

    /**
     * @brief Check if the user has requested to abort the operation.
     *
     * Override to implement cancellation logic.
     *
     * @return @c true if the user requested a break, @c false otherwise.
     */
    virtual bool userBreak()
    {
        return false;
    }

    /**
     * @brief Update the progress display.
     *
     * This method is called whenever the progress position changes.
     *
     * The parameter `position` is a normalized progress value:
     *   - Range [0.0, 1.0] for determinate progress (e.g., 0.57 for 57% complete).
     *   - A value of -1.0 indicates infinite (indeterminate) progress.
     *
     * @param position Normalized progress value or -1.0 for infinite progress.
     * @param isForce  If true, indicates a forced update even if the change threshold (≈1%) hasn’t
     * been met.
     */
    virtual void show([[maybe_unused]] float position, [[maybe_unused]] ShowFlags flags = ShowFlags::None)
    {}
};

}  // namespace Base
