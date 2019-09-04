/****************************************************************************
 *   Copyright (c) 2019 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

namespace App {

/// Helper class to manager transaction (i.e. undo/redo)
class AppExport AutoTransaction {
private:
    /// Private new operator to prevent heap allocation
    void* operator new(size_t size);

public:
    /** Constructor
     *
     * @param name: optional new transaction name on construction
     * @param tmpName: if true and a new transaction is setup, the name given is
     * considered as temporary, and subsequent construction of this class (or
     * calling Application::setActiveTransaction()) can override the transaction
     * name.
     *
     * The constructor increments an internal counter
     * (Application::_activeTransactionGuard). The counter prevents any new
     * active transaction being setup. It also prevents close (i.e. commits) the
     * current active transaction until it reaches zero. It does not have any
     * effect on aborting transaction, though.
     */
    AutoTransaction(const char *name=0, bool tmpName=false);

    /** Destructor
     *
     * This destructor decrease an internal counter
     * (Application::_activeTransactionGuard), and will commit any current
     * active transaction when the counter reaches zero.
     */
    ~AutoTransaction();

    /** Close or abort the transaction
     *
     * This function can be used to explicitly close (i.e. commit) the
     * transaction, if the current transaction ID matches the one created inside
     * the constructor. For aborting, it will abort any current transaction
     */
    void close(bool abort=false);

    /** Enable/Disable any AutoTransaction instance in the current stack
     *
     * Once disabled, any empty temporary named transaction is closed. If there
     * are non-empty or non-temporary named active transaction, it will not be
     * auto closed. 
     *
     * This function may be used in, for example, Gui::Document::setEdit() to
     * allow a transaction live past any command scope. 
     */
    static void setEnable(bool enable);

private:
    int tid = 0;
};

} // namespace App

#endif // APP_AUTOTRANSACTION_H
