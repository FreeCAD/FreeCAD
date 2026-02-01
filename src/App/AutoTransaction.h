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

#ifndef APP_AUTOTRANSACTION_H
#define APP_AUTOTRANSACTION_H

#include <cstddef>
#include <FCGlobal.h>
#include <string>

#include "TransactionDefs.h"

namespace App
{

class Application;
class Document;

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
    /** Constructor
     * 
     * @param tid the ID of the transaction to manage
     * 
     * No action is done in the constructor
     */
    explicit AutoTransaction(int tid);
    AutoTransaction(Document* doc, const std::string& name);
    
    /** Destructor
     * 
     * This destructor attempts to commit the transaction it manages
     */
    ~AutoTransaction();

    /**
     * @brief Close or abort the transaction.
     *
     * This function can be used to explicitly close (i.e. commit / abort) the
     * transaction,
     */
    void close(TransactionCloseMode mode = TransactionCloseMode::Commit);

private:
    int tid { 0 };
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
    TransactionLocker(Document* doc, bool lock = true);

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
    
    friend class Application;

public:
    /// Delete the new operator to prevent heap allocation.
    void* operator new(std::size_t) = delete;

private:
    bool active;
    Document* doc;
};

}  // namespace App

#endif  // APP_AUTOTRANSACTION_H
