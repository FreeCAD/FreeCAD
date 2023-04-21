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

#include "PreCompiled.h"

#include <Base/Interpreter.h>

#include "AutoTransaction.h"
#include "Application.h"
#include "Document.h"
#include "Transactions.h"


FC_LOG_LEVEL_INIT("App", true, true)

using namespace App;

static int _TransactionLock;
static int _TransactionClosed;

AutoTransaction::AutoTransaction(const char *name, bool tmpName) {
    auto &app = GetApplication();
    if(name && app._activeTransactionGuard>=0) {
        if(!app.getActiveTransaction()
                || (!tmpName && app._activeTransactionTmpName))
        {
            FC_LOG("auto transaction '" << name << "', " << tmpName);
            tid = app.setActiveTransaction(name);
            app._activeTransactionTmpName = tmpName;
        }
    }
    // We use negative transaction guard to disable auto transaction from here
    // and any stack below. This is to support user setting active transaction
    // before having any existing AutoTransaction on stack, or 'persist'
    // transaction that can out live AutoTransaction.
    if(app._activeTransactionGuard<0)
        --app._activeTransactionGuard;
    else if(tid || app._activeTransactionGuard>0)
        ++app._activeTransactionGuard;
    else if(app.getActiveTransaction()) {
        FC_LOG("auto transaction disabled because of '" << app._activeTransactionName << "'");
        --app._activeTransactionGuard;
    } else
        ++app._activeTransactionGuard;
    FC_TRACE("construct auto Transaction " << app._activeTransactionGuard);
}

AutoTransaction::~AutoTransaction() {
    auto &app = GetApplication();
    FC_TRACE("before destruct auto Transaction " << app._activeTransactionGuard);
    if(app._activeTransactionGuard<0)
        ++app._activeTransactionGuard;
    else if(!app._activeTransactionGuard) {
#ifdef FC_DEBUG
        FC_ERR("Transaction guard error");
#endif
    } else if(--app._activeTransactionGuard == 0) {
        try {
            // We don't call close() here, because close() only closes
            // transaction that we opened during construction time. However,
            // when _activeTransactionGuard reaches zero here, we are supposed
            // to close any transaction opened.
            app.closeActiveTransaction();
        } catch(Base::Exception &e) {
            e.ReportException();
        } catch(...)
        {}
    }
    FC_TRACE("destruct auto Transaction " << app._activeTransactionGuard);
}

void AutoTransaction::close(bool abort) {
    if(tid || abort) {
        GetApplication().closeActiveTransaction(abort,abort?0:tid);
        tid = 0;
    }
}

void AutoTransaction::setEnable(bool enable) {
    auto &app = GetApplication();
    if(!app._activeTransactionGuard)
        return;
    if((enable && app._activeTransactionGuard>0)
            || (!enable && app._activeTransactionGuard<0))
        return;
    app._activeTransactionGuard = -app._activeTransactionGuard;
    FC_TRACE("toggle auto Transaction " << app._activeTransactionGuard);
    if(!enable && app._activeTransactionTmpName) {
        bool close = true;
        for(auto &v : app.DocMap) {
            if(v.second->hasPendingTransaction()) {
                close = false;
                break;
            }
        }
        if(close)
            app.closeActiveTransaction();
    }
}

int Application::setActiveTransaction(const char *name, bool persist) {
    if(!name || !name[0])
        name = "Command";

    if(_activeTransactionGuard>0 && getActiveTransaction()) {
        if(_activeTransactionTmpName) {
            FC_LOG("transaction rename to '" << name << "'");
            for(auto &v : DocMap)
                v.second->renameTransaction(name,_activeTransactionID);
        } else {
            if(persist)
                AutoTransaction::setEnable(false);
            return 0;
        }
    } else if (_TransactionLock) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG))
            FC_WARN("Transaction locked, ignore new transaction '" << name << "'");
        return 0;
    } else {
        FC_LOG("set active transaction '" << name << "'");
        _activeTransactionID = 0;
        for(auto &v : DocMap)
            v.second->_commitTransaction();
        _activeTransactionID = Transaction::getNewID();
    }
    _activeTransactionTmpName = false;
    _activeTransactionName = name;
    if(persist)
        AutoTransaction::setEnable(false);
    return _activeTransactionID;
}

const char *Application::getActiveTransaction(int *id) const {
    int tid = 0;
    if(Transaction::getLastID() == _activeTransactionID)
        tid = _activeTransactionID;
    if (id)
        *id = tid;
    return tid ? _activeTransactionName.c_str() : nullptr;
}

void Application::closeActiveTransaction(bool abort, int id) {
    if(!id) id = _activeTransactionID;
    if(!id)
        return;

    if(_activeTransactionGuard>0 && !abort) {
        FC_LOG("ignore close transaction");
        return;
    }

    if(_TransactionLock) {
        if(_TransactionClosed >= 0)
            _TransactionLock = abort?-1:1;
        FC_LOG("pending " << (abort?"abort":"close") << " transaction");
        return;
    }

    FC_LOG("close transaction '" << _activeTransactionName << "' " << abort);
    _activeTransactionID = 0;

    TransactionSignaller signaller(abort,false);
    for(auto &v : DocMap) {
        if(v.second->getTransactionID(true) != id)
            continue;
        if(abort)
            v.second->_abortTransaction();
        else
            v.second->_commitTransaction();
    }
}

////////////////////////////////////////////////////////////////////////

TransactionLocker::TransactionLocker(bool lock)
    :active(lock)
{
    if(lock)
        ++_TransactionLock;
}

TransactionLocker::~TransactionLocker()
{
    if(active) {
        try {
            activate(false);
            return;
        } catch (Base::Exception &e) {
            e.ReportException();
        } catch (Py::Exception &) {
            Base::PyException e;
            e.ReportException();
        } catch (std::exception &e) {
            FC_ERR(e.what());
        } catch (...) {
        }
        FC_ERR("Exception when unlocking transaction");
    }
}

void TransactionLocker::activate(bool enable)
{
    if(active == enable)
        return;

    active = enable;
    if(active) {
        ++_TransactionLock;
        return;
    }

    if(--_TransactionLock != 0)
        return;

    if(_TransactionClosed) {
        bool abort = (_TransactionClosed<0);
        _TransactionClosed = 0;
        GetApplication().closeActiveTransaction(abort);
    }
}

bool TransactionLocker::isLocked() {
    return _TransactionLock > 0;
}

