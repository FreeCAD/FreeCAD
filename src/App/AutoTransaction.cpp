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

AutoTransaction::AutoTransaction(int tid)
    : tid(tid)
{

}

AutoTransaction::~AutoTransaction()
{
    close(false);
}

void AutoTransaction::close(bool abort)
{
    if (tid != 0) {
        GetApplication().closeActiveTransaction(abort, tid);
        tid = 0;
    }
}

TransactionToken::TransactionToken(int tid_)
{
    takeToken(tid_);
}
TransactionToken::~TransactionToken()
{
    returnToken();
}

TransactionToken::TransactionToken(TransactionToken&& token) noexcept
{
    tid = token.tid;
    token.tid = -1; // Ensures that the token is not returned twice
}
TransactionToken& TransactionToken::operator=(TransactionToken&& token) noexcept
{
    tid = token.tid;
    token.tid = 0;
    return *this;
}
TransactionToken::TransactionToken(const TransactionToken& token)
{
    takeToken(token.tid);
}
TransactionToken& TransactionToken::operator=(const TransactionToken& token)
{
    takeToken(token.tid);
    return *this;
}
void TransactionToken::abort()
{
    if (tid != -1) {
        GetApplication().closeActiveTransaction(true, tid);
    }
}
void TransactionToken::takeToken(int tid_)
{
    if (tid_ == tid) {
        return; // No change
    }
    returnToken();

    // Take a new token
    tid = tid_;
    GetApplication().takeToken(tid);
}
void TransactionToken::returnToken()
{
    if (tid != -1) { // This is a valid token that must be dealt with
        GetApplication().returnToken(tid);
    }
}

int Application::setActiveTransaction(const char* name, bool persist)
{
    if (!name || !name[0]) {
        name = "Command";
    }

    if (_pActiveDoc != nullptr) {
        return _pActiveDoc->setActiveTransaction(name);
    }
    return openGlobalTransaction(name);
}

const char* Application::getActiveTransaction(int* id) const
{
    if (id != nullptr) {
        *id = _globalTransactionID;
    }
    return _globalTransactionID != 0 ? getTransactionName(_globalTransactionID).c_str() : nullptr;
}
int Application::openGlobalTransaction(const char* name)
{
    if (!name || !name[0]) {
        name = "Command";
    }

    FC_WARN("Setting a global transaction with name='" << name);
    if (_globalTransactionID != 0 && transactionTmpName(_globalTransactionID)) {
        setTransactionName(_globalTransactionID, name);
    } else {
        FC_LOG("set global transaction '" << name << "'");

        if (_globalTransactionID != 0 && !closeActiveTransaction(false, _globalTransactionID)) {
            FC_WARN("could not close current global transaction");
            return _globalTransactionID;
        }

        _globalTransactionID = Transaction::getNewID();
        setTransactionDescription(
            _globalTransactionID,
            TransactionDescription {.initiator = nullptr, .name = name, .tmp = false});
    }

    return _globalTransactionID;
}
int Application::getGlobalTransaction() const
{
    return _globalTransactionID;
}
bool Application::transactionIsActive(int tid) const
{
    return transactionDescription(tid) != std::nullopt;
}
std::string Application::getTransactionName(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->name : "";
}
bool Application::transactionTmpName(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->tmp : false;
}
Document* Application::transactionInitiator(int tid) const
{
    auto desc = transactionDescription(tid);
    return desc ? desc->initiator : nullptr;
}
std::optional<TransactionDescription> Application::transactionDescription(int tid) const
{
    if (tid == 0) {
        return std::nullopt;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    return (found != _activeTransactionDescriptions.end()) ? std::optional<TransactionDescription>(found->second) : std::nullopt;    
}
void Application::setTransactionDescription(int tid, const TransactionDescription& desc)
{
    if (tid == 0) {
        return;
    }
    if (desc.numTokens < 0) {
        FC_ERR("setting transaction #"<<tid<<" description with numToken < 0");
        return;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    bool wasPresent = (found != _activeTransactionDescriptions.end());

    if (wasPresent && found->second.tmp) {
        for (auto& v : DocMap) {
            v.second->renameTransaction(desc.name.c_str(), tid);
        }
    }

    if (!wasPresent || found->second.tmp) {
        _activeTransactionDescriptions[tid] = desc;
        FC_LOG("transaction rename to '" << desc.name << "'");
    }
}
void Application::setTransactionName(int tid, const std::string& name, bool tmp)
{
    if (tid == 0 || !transactionIsActive(tid)) {
        return;
    }
    auto found = _activeTransactionDescriptions.find(tid);
    if (found == _activeTransactionDescriptions.end() || found->second.tmp) {
        _activeTransactionDescriptions[tid].name = name;
        _activeTransactionDescriptions[tid].tmp = tmp;
        FC_LOG("transaction rename to '" << name << "'");
        for (auto& v : DocMap) {
            v.second->renameTransaction(name.c_str(), tid);
        }
    } 
}
void Application::takeToken(int tid)
{
    auto found = _activeTransactionDescriptions.find(tid);
    if (found == _activeTransactionDescriptions.end()) {
        FC_WARN("Taking token for transaction #" << tid << " but it is not active");
        return;
    }
    found->second.numTokens++;
}

void Application::returnToken(int tid)
{
    auto found = _activeTransactionDescriptions.find(tid);
    if (found == _activeTransactionDescriptions.end()) {
        // Might happen if a transaction was aborted
        return;
    }
    if (found->second.numTokens == 0) {
        FC_WARN("Returning token for transaction #" << tid << " but does not have token");
        return;
    }
    found->second.numTokens--;
    if (found->second.numTokens == 0) {
        closeActiveTransaction(false, tid);
    }
}


bool Application::closeActiveTransaction(bool abort, int id)
{
    if (id == 0) {
        if (_pActiveDoc != nullptr && _pActiveDoc->getBookedTransactionID() != 0) {
            id = _pActiveDoc->getBookedTransactionID();
        } else {
            id = _globalTransactionID;
        }
    }
    if (id == 0 || id == currentlyClosingID) {
        return false;
    }
    currentlyClosingID = id;

    auto foundDesc = _activeTransactionDescriptions.find(id);
    if (!abort && foundDesc != _activeTransactionDescriptions.end() && foundDesc->second.numTokens != 0) {
        FC_LOG("Commit blocked by token on transaction #" << id);
        return false;
    }

    std::vector<Document*> docsToPoke;
    for (auto& v : DocMap) {
        if (v.second->getBookedTransactionID() != id) {
            continue;
        }
        if(v.second->isTransactionLocked() || v.second->transacting()) {
            std::cerr<<"Transaction locked..\n";
            FC_LOG("pending " << (abort ? "abort" : "close") << " transaction");
            currentlyClosingID = 0;
            return false;
        }
        if(v.second->transacting()) {
            std::cerr<<"Transaction blocked..\n";
            FC_LOG("pending " << (abort ? "abort" : "close") << " transaction");
            currentlyClosingID = 0;
            return false;
        }
        docsToPoke.push_back(v.second);
    }

    FC_LOG("close transaction '" << _activeTransactionDescriptions[id].name << "' " << abort);
    _activeTransactionDescriptions.erase(id);
    if (id == _globalTransactionID) {
        _globalTransactionID = 0;
    }

    TransactionSignaller signaller(abort, false);

    for (auto& doc : docsToPoke) {
        if (abort) {
            doc->_abortTransaction();
        }
        else {
            doc->_commitTransaction();
        }
    }
    currentlyClosingID = 0;

    return true;
}
bool Application::commitTransaction(int tid)
{
    return  closeActiveTransaction(false, tid);
}
bool Application::abortTransaction(int tid)
{
    return closeActiveTransaction(true, tid);
}

////////////////////////////////////////////////////////////////////////

TransactionLocker::TransactionLocker(Document* doc, bool lock)
    : active(lock)
    , doc(doc)
{
    if (lock) {
        doc->lockTransaction();
    }
}

TransactionLocker::~TransactionLocker()
{
    if (active) {
        try {
            activate(false);
            return;
        }
        catch (Base::Exception& e) {
            e.reportException();
        }
        catch (Py::Exception&) {
            Base::PyException e;
            e.reportException();
        }
        catch (std::exception& e) {
            FC_ERR(e.what());
        }
        catch (...) {
        }
        FC_ERR("Exception when unlocking transaction");
    }
}

void TransactionLocker::activate(bool enable)
{
    if (active == enable) {
        return;
    }

    active = enable;
    if (active) {
        doc->lockTransaction();
        return;
    }

    doc->unlockTransaction();
    // if (doc->isTransactionLocked()) {
    //     return;
    // }

    // if (_TransactionClosed) {
    //     bool abort = (_TransactionClosed < 0);
    //     _TransactionClosed = 0;
    //     GetApplication().closeActiveTransaction(abort);
    // }
}      