// SPDX-License-Identifier: LGPL-2.1-or-later

/****************************************************************************
 *   Copyright (c) 2019 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#pragma once

#include <cstddef>
#include <FCGlobal.h>

namespace App
{

class Application;

/**
 * @brief A helper class to manage transactions (i.e. undo/redo).
 *
 * An AutoTransaction object is meant to be allocated on the stack and governs
 * the transactions in that scope.
 */
class AppExport AutoTransaction
{
public:
     /// Delete the new operator to prevent heap allocation.
    void* operator new(std::size_t) = delete;

public:
    /**
     * @brief Construct an auto transaction.
     *
     * @param[in] name: optional new transaction name on construction
     * @param[in] tmpName: if true and a new transaction is setup, the name given is
     * considered as temporary, and subsequent construction of this class (or
     * calling Application::setActiveTransaction()) can override the transaction
     * name.
     *
     * The constructor increments an internal counter
     * (Application::_activeTransactionGuard). The counter prevents any new
     * active transactions being setup. It also prevents to close
     * (i.e. commits) the current active transaction until it reaches zero. It
     * does not have any effect on aborting transactions though.
     */
    AutoTransaction(const char* name = nullptr, bool tmpName = false);

    /**
     * @brief Destruct an auto transaction.
     *
     * This destructor decrease an internal counter
     * (Application::_activeTransactionGuard), and will commit any current
     * active transaction when the counter reaches zero.
     */
    ~AutoTransaction();

    /**
     * @brief Close or abort the transaction.
     *
     * This function can be used to explicitly close (i.e. commit) the
     * transaction, if the current transaction ID matches the one created inside
     * the constructor. For aborting, it will abort any current transaction.
     *
     * @param[in] abort: if true, abort the transaction; otherwise, commit it.
     */
    void close(bool abort = false);

    /**
     * @brief Enable/Disable any AutoTransaction instance on the current stack.
     *
     * Once disabled, any empty temporary named transaction is closed. If there
     * are non-empty or non-temporary named active transaction, it will not be
     * auto closed.
     *
     * This function may be used in, for example, Gui::Document::setEdit() to
     * allow a transaction live past any command scope.
     *
     * @param[in] enable: if true, enable the AutoTransaction; otherwise, disable it.
     */
    static void setEnable(bool enable);

private:
    int tid = 0;
};


/**
 * @brief Helper class to lock a transaction from being closed or aborted.
 *
 * The helper class is used to protect some critical transactions from being
 * closed prematurely, e.g. when deleting some object.
 */
class AppExport TransactionLocker
{
public:

    /**
     * @brief Construct a transaction locker.
     *
     * @param[in] lock: whether to activate the lock
     */
    TransactionLocker(bool lock = true);

    /**
     * @brief Destruct a transaction locker.
     *
     * Unlock the transaction if this locker is active
     */
    ~TransactionLocker();

    /**
     * @brief Activate or deactivate this locker.
     *
     * An internal counter is used to support recursive locker. When activated,
     * the current active transaction cannot be closed or aborted.  But the
     * closing call (Application::closeActiveTransaction()) will be remembered,
     * and performed when the internal lock counter reaches zero.
     *
     * @param enable: whether to activate the locker
     */
    void activate(bool enable);

    /// Check if the locker is active.
    bool isActive() const
    {
        return active;
    }

    /// Check if transaction is being locked.
    static bool isLocked();

    friend class Application;

public:
    /// Delete the new operator to prevent heap allocation.
    void* operator new(std::size_t) = delete;

private:
    bool active;
};

}  // namespace App