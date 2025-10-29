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

namespace App
{

class Application;
class Document;

/// Helper class to manager transaction (i.e. undo/redo)
class AppExport AutoTransaction
{
public:
    /// Private new operator to prevent heap allocation
    void* operator new(std::size_t) = delete;

public:
    /** Constructor
     * 
     * @param tid the ID of the transaction to manage
     * 
     * No action is done in the constructor
     */
    AutoTransaction(int tid);

    /** Destructor
     * 
     * This destructor attempts to commit the transaction it manages
     */
    ~AutoTransaction();

    /** Close or abort the transaction
     *
     * This function can be used to explicitly close (i.e. commit / abort) the
     * transaction,
     */
    void close(bool abort = false);

private:
    int tid { 0 };
};

/** Helper class to lock a transaction from being closed or aborted.
 *
 * The helper class is used to protect some critical transaction from being
 * closed prematurely, e.g. when deleting some object.
 */
class AppExport TransactionLocker
{
public:
    /** Constructor
     * @param lock: whether to activate the lock
     */
    TransactionLocker(Document* doc, bool lock = true);

    /** Destructor
     * Unlock the transaction is this locker is active
     */
    ~TransactionLocker();

    /** Activate or deactivate this locker
     * @param enable: whether to activate the locker
     *
     * An internal counter is used to support recursive locker. When activated,
     * the current active transaction cannot be closed or aborted.  But the
     * closing call (Application::closeActiveTransaction()) will be remembered,
     * and performed when the internal lock counter reaches zero.
     */
    void activate(bool enable);

    /// Check if the locker is active
    bool isActive() const
    {
        return active;
    }
    
    friend class Application;

public:
    /// Private new operator to prevent heap allocation
    void* operator new(std::size_t) = delete;

private:
    bool active;
    Document* doc;
};

}  // namespace App

#endif  // APP_AUTOTRANSACTION_H
