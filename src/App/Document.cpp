// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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

#include <bitset>
#include <stack>
#include <deque>
#include <iostream>
#include <utility>
#include <set>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <list>
#include <algorithm>
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/bimap.hpp>
#include <boost/graph/strong_components.hpp>
#include <boost/graph/topological_sort.hpp>

#include <boost/regex.hpp>
#include <random>
#include <unordered_map>
#include <unordered_set>

#include <QCryptographicHash>
#include <QCoreApplication>

#include <FCConfig.h>

#include <App/DocumentPy.h>
#include <Base/Interpreter.h>
#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/FileInfo.h>
#include <Base/TimeInfo.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Profiler.h>
#include <Base/Tools.h>
#include <Base/Uuid.h>
#include <Base/Sequencer.h>
#include <Base/Stream.h>
#include <Base/UnitsApi.h>

#include "Document.h"
#include "private/DocumentP.h"
#include "Application.h"
#include "AutoTransaction.h"
#include "BackupPolicy.h"
#include "ExpressionParser.h"
#include "GeoFeature.h"
#include "License.h"
#include "Link.h"
#include "MergeDocuments.h"
#include "StringHasher.h"
#include "Transactions.h"

#ifdef _MSC_VER
#include <zipios++/zipios-config.h>
#endif
#include <zipios++/zipfile.h>
#include <zipios++/zipinputstream.h>
#include <zipios++/zipoutputstream.h>
#include <zipios++/meta-iostreams.h>


FC_LOG_LEVEL_INIT("App", true, true, true)

using Base::Console;
using Base::streq;
using Base::Writer;
using namespace App;
using namespace boost;
using namespace zipios;

#if FC_DEBUG
#define FC_LOGFEATUREUPDATE
#endif

namespace fs = std::filesystem;

namespace App
{

static bool globalIsRestoring;
static bool globalIsRelabeling;

DocumentP::DocumentP()
{
    static std::random_device rd;
    static std::mt19937 rgen(rd());
    static std::uniform_int_distribution<> rdist(0, 5000);
    // Set some random offset to reduce likelihood of ID collision when
    // copying shape from other document. It is probably better to randomize
    // on each object ID.
    lastObjectId = rdist(rgen);
    StatusBits.set((size_t)Document::Closable, true);
    StatusBits.set((size_t)Document::KeepTrailingDigits, true);
    StatusBits.set((size_t)Document::Restoring, false);
}

}  // namespace App

PROPERTY_SOURCE(App::Document, App::PropertyContainer)

bool Document::testStatus(const Status pos) const
{
    return d->StatusBits.test(static_cast<size_t>(pos));
}

void Document::setStatus(const Status pos, const bool on) // NOLINT
{
    d->StatusBits.set(static_cast<size_t>(pos), on);
}

// bool _has_cycle_dfs(const DependencyList & g, vertex_t u, default_color_type * color)
//{
//   color[u] = gray_color;
//   graph_traits < DependencyList >::adjacency_iterator vi, vi_end;
//   for (tie(vi, vi_end) = adjacent_vertices(u, g); vi != vi_end; ++vi)
//     if (color[*vi] == white_color)
//       if (has_cycle_dfs(g, *vi, color))
//         return true;            // cycle detected, return immediately
//       else if (color[*vi] == gray_color)        // *vi is an ancestor!
//         return true;
//   color[u] = black_color;
//   return false;
// }

bool Document::checkOnCycle()
{
    return false;
}

bool Document::undo(const int id)
{
    if (d->iUndoMode != 0) {
        if (id != 0) {
            const auto it = mUndoMap.find(id);
            if (it == mUndoMap.end()) {
                return false;
            }
            if (it->second != d->activeUndoTransaction) {
                while (!mUndoTransactions.empty() && mUndoTransactions.back() != it->second) {
                    undo(0);
                }
            }
        }

        if (d->activeUndoTransaction) {
            _commitTransaction(true);
        }
        if (mUndoTransactions.empty()) {
            return false;
        }
        // redo
        d->activeUndoTransaction = new Transaction(mUndoTransactions.back()->getID());
        d->activeUndoTransaction->Name = mUndoTransactions.back()->Name;

        {
            Base::FlagToggler<bool> flag(d->undoing);
            // applying the undo
            mUndoTransactions.back()->apply(*this, false);

            // save the redo
            mRedoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
            mRedoTransactions.push_back(d->activeUndoTransaction);
            d->activeUndoTransaction = nullptr;

            mUndoMap.erase(mUndoTransactions.back()->getID());
            delete mUndoTransactions.back();
            mUndoTransactions.pop_back();
        }

        for (const auto& obj : d->objectArray) {
            if (obj->testStatus(ObjectStatus::PendingTransactionUpdate)) {
                obj->onUndoRedoFinished();
                obj->setStatus(ObjectStatus::PendingTransactionUpdate, false);
            }
        }

        signalUndo(*this);  // now signal the undo

        return true;
    }

    return false;
}

bool Document::redo(const int id)
{
    if (d->iUndoMode != 0) {
        if (id != 0) {
            const auto it = mRedoMap.find(id);
            if (it == mRedoMap.end()) {
                return false;
            }
            while (!mRedoTransactions.empty() && mRedoTransactions.back() != it->second) {
                redo(0);
            }
        }

        if (d->activeUndoTransaction) {
            _commitTransaction(true);
        }

        assert(mRedoTransactions.size() != 0);

        // undo
        d->activeUndoTransaction = new Transaction(mRedoTransactions.back()->getID());
        d->activeUndoTransaction->Name = mRedoTransactions.back()->Name;

        // do the redo
        {
            Base::FlagToggler<bool> flag(d->undoing);
            mRedoTransactions.back()->apply(*this, true);

            mUndoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
            mUndoTransactions.push_back(d->activeUndoTransaction);
            d->activeUndoTransaction = nullptr;

            mRedoMap.erase(mRedoTransactions.back()->getID());
            delete mRedoTransactions.back();
            mRedoTransactions.pop_back();
        }

        for (const auto& obj : d->objectArray) {
            if (obj->testStatus(ObjectStatus::PendingTransactionUpdate)) {
                obj->onUndoRedoFinished();
                obj->setStatus(ObjectStatus::PendingTransactionUpdate, false);
            }
        }

        signalRedo(*this);
        return true;
    }

    return false;
}

void Document::changePropertyOfObject(TransactionalObject* obj,
                                      const Property* prop,
                                      const std::function<void()>& changeFunc)
{
    if (!prop || !obj || !obj->isAttachedToDocument()) {
        return;
    }
    if ((d->iUndoMode != 0) && !isPerformingTransaction() && !d->activeUndoTransaction) {
        if (!testStatus(Restoring) || testStatus(Importing)) {
            int tid = 0;
            const char* name = GetApplication().getActiveTransaction(&tid);
            if (name && tid > 0) {
                _openTransaction(name, tid);
            }
        }
    }
    if (d->activeUndoTransaction && !d->rollback) {
        changeFunc();
    }
}

void Document::renamePropertyOfObject(TransactionalObject* obj,
                                      const Property* prop, const char* oldName)
{
    changePropertyOfObject(obj, prop, [this, obj, prop, oldName]() {
        d->activeUndoTransaction->renameProperty(obj, prop, oldName);
    });
}

void Document::addOrRemovePropertyOfObject(TransactionalObject* obj,
                                           const Property* prop, const bool add)
{
    changePropertyOfObject(obj, prop, [this, obj, prop, add]() {
        d->activeUndoTransaction->addOrRemoveProperty(obj, prop, add);
    });
}

bool Document::isPerformingTransaction() const
{
    return d->undoing || d->rollback;
}

std::vector<std::string> Document::getAvailableUndoNames() const
{
    std::vector<std::string> vList;
    if (d->activeUndoTransaction) {
        vList.push_back(d->activeUndoTransaction->Name);
    }
    for (auto It = mUndoTransactions.rbegin();
         It != mUndoTransactions.rend();
         ++It) {
        vList.push_back((*It)->Name);
    }
    return vList;
}

std::vector<std::string> Document::getAvailableRedoNames() const
{
    std::vector<std::string> vList;
    for (auto It = mRedoTransactions.rbegin();
         It != mRedoTransactions.rend();
         ++It) {
        vList.push_back((*It)->Name);
    }
    return vList;
}

void Document::openTransaction(const char* name) // NOLINT
{
    if (isPerformingTransaction() || d->committing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot open transaction while transacting");
        }
        return;
    }

    GetApplication().setActiveTransaction(name ? name : "<empty>");
}

int Document::_openTransaction(const char* name, int id)
{
    if (isPerformingTransaction() || d->committing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot open transaction while transacting");
        }
        return 0;
    }

    if (d->iUndoMode != 0) {
        // Avoid recursive calls that is possible while
        // clearing the redo transactions and will cause
        // a double deletion of some transaction and thus
        // a segmentation fault
        if (d->opentransaction) {
            return 0;
        }
        Base::FlagToggler<> flag(d->opentransaction);

        if ((id != 0) && mUndoMap.find(id) != mUndoMap.end()) {
            throw Base::RuntimeError("invalid transaction id");
        }
        if (d->activeUndoTransaction) {
            _commitTransaction(true);
        }
        _clearRedos();

        d->activeUndoTransaction = new Transaction(id);
        if (!name) {
            name = "<empty>";
        }
        d->activeUndoTransaction->Name = name;
        mUndoMap[d->activeUndoTransaction->getID()] = d->activeUndoTransaction;
        id = d->activeUndoTransaction->getID();

        signalOpenTransaction(*this, name);

        auto& app = GetApplication();
        auto activeDoc = app.getActiveDocument();
        if (activeDoc && activeDoc != this && !activeDoc->hasPendingTransaction()) {
            std::string aname("-> ");
            aname += d->activeUndoTransaction->Name;
            FC_LOG("auto transaction " << getName() << " -> " << activeDoc->getName());
            activeDoc->_openTransaction(aname.c_str(), id);
        }
        return id;
    }
    return 0;
}

void Document::renameTransaction(const char* name, const int id) const
{
    if (name && d->activeUndoTransaction && d->activeUndoTransaction->getID() == id) {
        if (boost::starts_with(d->activeUndoTransaction->Name, "-> ")) {
            d->activeUndoTransaction->Name.resize(3);
        }
        else {
            d->activeUndoTransaction->Name.clear();
        }
        d->activeUndoTransaction->Name += name;
    }
}

void Document::_checkTransaction(DocumentObject* pcDelObj, const Property* What, int line)
{
    // if the undo is active but no transaction open, open one!
    if ((d->iUndoMode != 0) && !isPerformingTransaction()) {
        if (!d->activeUndoTransaction) {
            if (!testStatus(Restoring) || testStatus(Importing)) {
                int tid = 0;
                const char* name = GetApplication().getActiveTransaction(&tid);
                if (name && tid > 0) {
                    bool ignore = false;
                    if (What && What->testStatus(Property::NoModify)) {
                        ignore = true;
                    }
                    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
                        if (What) {
                            FC_LOG((ignore ? "ignore" : "auto")
                                   << " transaction (" << line << ") '" << What->getFullName());
                        }
                        else {
                            FC_LOG((ignore ? "ignore" : "auto") << " transaction (" << line << ") '"
                                                                << name << "' in " << getName());
                        }
                    }
                    if (!ignore) {
                        _openTransaction(name, tid);
                    }
                    return;
                }
            }
            if (!pcDelObj) {
                return;
            }
            // When the object is going to be deleted we have to check if it has already been added
            // to the undo transactions
            std::list<Transaction*>::iterator it;
            for (it = mUndoTransactions.begin(); it != mUndoTransactions.end(); ++it) {
                if ((*it)->hasObject(pcDelObj)) {
                    _openTransaction("Delete");
                    break;
                }
            }
        }
    }
}

void Document::_clearRedos()
{
    if (isPerformingTransaction() || d->committing) {
        FC_ERR("Cannot clear redo while transacting");
        return;
    }

    mRedoMap.clear();
    while (!mRedoTransactions.empty()) {
        delete mRedoTransactions.back();
        mRedoTransactions.pop_back();
    }
}

void Document::commitTransaction() // NOLINT
{
    if (isPerformingTransaction() || d->committing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot commit transaction while transacting");
        }
        return;
    }

    if (d->activeUndoTransaction) {
        GetApplication().closeActiveTransaction(false, d->activeUndoTransaction->getID());
    }
}

void Document::_commitTransaction(const bool notify)
{
    if (isPerformingTransaction()) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot commit transaction while transacting");
        }
        return;
    }
    if (d->committing) {
        // for a recursive call return without printing a warning
        return;
    }

    if (d->activeUndoTransaction) {
        Base::FlagToggler<> flag(d->committing);
        Application::TransactionSignaller signaller(false, true);
        const int id = d->activeUndoTransaction->getID();
        mUndoTransactions.push_back(d->activeUndoTransaction);
        d->activeUndoTransaction = nullptr;
        // check the stack for the limits
        if (mUndoTransactions.size() > d->UndoMaxStackSize) {
            mUndoMap.erase(mUndoTransactions.front()->getID());
            delete mUndoTransactions.front();
            mUndoTransactions.pop_front();
        }
        signalCommitTransaction(*this);

        // closeActiveTransaction() may call again _commitTransaction()
        if (notify) {
            GetApplication().closeActiveTransaction(false, id);
        }
    }
}

void Document::abortTransaction() const
{
    if (isPerformingTransaction() || d->committing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot abort transaction while transacting");
        }
        return;
    }
    if (d->activeUndoTransaction) {
        GetApplication().closeActiveTransaction(true, d->activeUndoTransaction->getID());
    }
}

void Document::_abortTransaction()
{
    if (isPerformingTransaction() || d->committing) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Cannot abort transaction while transacting");
        }
    }

    if (d->activeUndoTransaction) {
        Base::FlagToggler<bool> flag(d->rollback);
        Application::TransactionSignaller signaller(true, true);

        // applying the so far made changes
        d->activeUndoTransaction->apply(*this, false);

        // destroy the undo
        mUndoMap.erase(d->activeUndoTransaction->getID());
        delete d->activeUndoTransaction;
        d->activeUndoTransaction = nullptr;
        signalAbortTransaction(*this);
    }
}

bool Document::hasPendingTransaction() const
{
    return d->activeUndoTransaction != nullptr;
}

int Document::getTransactionID(const bool undo, unsigned pos) const
{
    if (undo) {
        if (d->activeUndoTransaction) {
            if (pos == 0) {
                return d->activeUndoTransaction->getID();
            }
            --pos;
        }
        if (pos >= mUndoTransactions.size()) {
            return 0;
        }
        auto rit = mUndoTransactions.rbegin();
        for (; pos != 0U; ++rit, --pos) {}
        return (*rit)->getID();
    }
    if (pos >= mRedoTransactions.size()) {
        return 0;
    }
    auto rit = mRedoTransactions.rbegin();
    for (; pos != 0U; ++rit, --pos) {}
    return (*rit)->getID();
}

bool Document::isTransactionEmpty() const
{
    return !d->activeUndoTransaction;
        // Transactions are now only created when there are actual changes.
        // Empty transaction is now significant for marking external changes. It
        // is used to match ID with transactions in external documents and
        // trigger undo/redo there.

        // return d->activeUndoTransaction->isEmpty();

}

void Document::clearDocument() // NOLINT
{
    d->activeObject = nullptr;

    if (!d->objectArray.empty()) {
        GetApplication().signalDeleteDocument(*this);
        d->clearDocument();
        GetApplication().signalNewDocument(*this, false);
    }

    Base::FlagToggler<> flag(globalIsRestoring, false);

    setStatus(Document::PartialDoc, false);

    d->clearRecomputeLog();
    d->objectLabelManager.clear();
    d->objectArray.clear();
    d->objectMap.clear();
    d->objectNameManager.clear();
    d->objectIdMap.clear();
    d->lastObjectId = 0;
}


void Document::clearUndos()
{
    if (isPerformingTransaction() || d->committing) {
        FC_ERR("Cannot clear undos while transacting");
        return;
    }

    if (d->activeUndoTransaction) {
        _commitTransaction(true);
    }

    mUndoMap.clear();

    // When cleaning up the undo stack we must delete the transactions from front
    // to back because a document object can appear in several transactions but
    // once removed from the document the object can never ever appear in any later
    // transaction. Since the document object may be also deleted when the transaction
    // is deleted we must make sure not access an object once it's destroyed. Thus, we
    // go from front to back and not the other way round.
    while (!mUndoTransactions.empty()) {
        delete mUndoTransactions.front();
        mUndoTransactions.pop_front();
    }
    // while (!mUndoTransactions.empty()) {
    //     delete mUndoTransactions.back();
    //     mUndoTransactions.pop_back();
    // }

    _clearRedos();
}

int Document::getAvailableUndos(const int id) const
{
    if (id != 0) {
        const auto it = mUndoMap.find(id);
        if (it == mUndoMap.end()) {
            return 0;
        }
        int i = 0;
        if (d->activeUndoTransaction) {
            ++i;
            if (d->activeUndoTransaction->getID() == id) {
                return i;
            }
        }
        auto rit = mUndoTransactions.rbegin();
        for (; rit != mUndoTransactions.rend() && *rit != it->second; ++rit) {
            ++i;
        }
        assert(rit != mUndoTransactions.rend());
        return i + 1;
    }
    if (d->activeUndoTransaction) {
        return static_cast<int>(mUndoTransactions.size() + 1);
    }
    return static_cast<int>(mUndoTransactions.size());
}

int Document::getAvailableRedos(const int id) const
{
    if (id != 0) {
        const auto it = mRedoMap.find(id);
        if (it == mRedoMap.end()) {
            return 0;
        }
        int i = 0;
        for (auto rit = mRedoTransactions.rbegin(); *rit != it->second; ++rit) {
            ++i;
        }
        assert(i < static_cast<int>(mRedoTransactions.size()));
        return i + 1;
    }
    return static_cast<int>(mRedoTransactions.size());
}

void Document::setUndoMode(const int iMode)
{
    if ((d->iUndoMode != 0) && (iMode == 0)) {
        clearUndos();
    }

    d->iUndoMode = iMode;
}

int Document::getUndoMode() const
{
    return d->iUndoMode;
}

unsigned int Document::getUndoMemSize() const
{
    return d->UndoMemSize;
}

void Document::setUndoLimit(const unsigned int UndoMemSize) // NOLINT
{
    d->UndoMemSize = UndoMemSize;
}

void Document::setMaxUndoStackSize(const unsigned int UndoMaxStackSize) // NOLINT
{
    d->UndoMaxStackSize = UndoMaxStackSize;
}

unsigned int Document::getMaxUndoStackSize() const
{
    return d->UndoMaxStackSize;
}

void Document::onBeforeChange(const Property* prop)
{
    if (prop == &Label) {
        oldLabel = Label.getValue();
    }
    signalBeforeChange(*this, *prop);
}

void Document::onChanged(const Property* prop)
{
    signalChanged(*this, *prop);

    // the Name property is a label for display purposes
    if (prop == &Label) {
        Base::FlagToggler<> flag(globalIsRelabeling);
        GetApplication().signalRelabelDocument(*this);
    }
    else if (prop == &ShowHidden) {
        GetApplication().signalShowHidden(*this);
    }
    else if (prop == &Uid) {
        std::string new_dir =
            getTransientDirectoryName(this->Uid.getValueStr(), this->FileName.getStrValue());
        std::string old_dir = this->TransientDir.getStrValue();
        Base::FileInfo TransDirNew(new_dir);
        Base::FileInfo TransDirOld(old_dir);
        // this directory should not exist
        if (!TransDirNew.exists()) {
            if (TransDirOld.exists()) {
                if (!TransDirOld.renameFile(new_dir.c_str())) {
                    Base::Console().warning("Failed to rename '%s' to '%s'\n",
                                            old_dir.c_str(),
                                            new_dir.c_str());
                }
                else {
                    this->TransientDir.setValue(new_dir);
                }
            }
            else {
                if (!TransDirNew.createDirectories()) {
                    Base::Console().warning("Failed to create '%s'\n", new_dir.c_str());
                }
                else {
                    this->TransientDir.setValue(new_dir);
                }
            }
        }
        // when reloading an existing document the transient directory doesn't change
        // so we must avoid to generate a new uuid
        else if (TransDirNew.filePath() != TransDirOld.filePath()) {
            // make sure that the uuid is unique
            std::string uuid = this->Uid.getValueStr();
            Base::Uuid id;
            Base::Console().warning("Document with the UUID '%s' already exists, change to '%s'\n",
                                    uuid.c_str(),
                                    id.getValue().c_str());
            // recursive call of onChanged()
            this->Uid.setValue(id);
        }
    }
    else if (prop == &UseHasher) {
        for (auto obj : d->objectArray) {
            auto geofeature = freecad_cast<GeoFeature*>(obj);
            if (geofeature && geofeature->getPropertyOfGeometry()) {
                geofeature->enforceRecompute();
            }
        }
    }
}

void Document::onBeforeChangeProperty(const TransactionalObject* Who, const Property* What)
{
    if (Who->isDerivedFrom<DocumentObject>()) {
        signalBeforeChangeObject(*static_cast<const DocumentObject*>(Who), *What);
    }
    if (!d->rollback && !globalIsRelabeling) {
        _checkTransaction(nullptr, What, __LINE__);
        if (d->activeUndoTransaction) {
            d->activeUndoTransaction->addObjectChange(Who, What);
        }
    }
}

void Document::onChangedProperty(const DocumentObject* Who, const Property* What)
{
    signalChangedObject(*Who, *What);
}

void Document::setTransactionMode(const int iMode) // NOLINT
{
    d->iTransactionMode = iMode;
}

//--------------------------------------------------------------------------
// constructor
//--------------------------------------------------------------------------
Document::Document(const char* documentName)
    : d(new DocumentP), myName(documentName)
{
    // Remark: In a constructor we should never increment a Python object as we cannot be sure
    // if the Python interpreter gets a reference of it. E.g. if we increment but Python don't
    // get a reference then the object wouldn't get deleted in the destructor.
    // So, we must increment only if the interpreter gets a reference.
    // Remark: We force the document Python object to own the DocumentPy instance, thus we don't
    // have to care about ref counting any more.
    setAutoCreated(false);
    Base::PyGILStateLocker lock;
    d->DocumentPythonObject = Py::Object(new DocumentPy(this), true);

#ifdef FC_LOGUPDATECHAIN
    Console().log("+App::Document: %p\n", this);
#endif
    std::string CreationDateString = Base::Tools::currentDateTimeString();
    std::string Author = GetApplication()
                             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                             ->GetASCII("prefAuthor", "");
    std::string AuthorComp =
        GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
            ->GetASCII("prefCompany", "");
    ADD_PROPERTY_TYPE(Label, ("Unnamed"), 0, Prop_ReadOnly, "The name of the document");
    ADD_PROPERTY_TYPE(FileName,
                      (""),
                      0,
                      PropertyType(Prop_Transient | Prop_ReadOnly),
                      "The path to the file where the document is saved to");
    ADD_PROPERTY_TYPE(CreatedBy, (Author.c_str()), 0, Prop_None, "The creator of the document");
    ADD_PROPERTY_TYPE(CreationDate,
                      (CreationDateString.c_str()),
                      0,
                      Prop_ReadOnly,
                      "Date of creation");
    ADD_PROPERTY_TYPE(LastModifiedBy, (""), 0, Prop_None, 0);
    ADD_PROPERTY_TYPE(LastModifiedDate, ("Unknown"), 0, Prop_ReadOnly, "Date of last modification");
    ADD_PROPERTY_TYPE(Company,
                      (AuthorComp.c_str()),
                      0,
                      Prop_None,
                      "Additional tag to save the name of the company");
    ADD_PROPERTY_TYPE(UnitSystem, (""), 0, Prop_None, "Unit system to use in this project");
    // Set up the possible enum values for the unit system

    UnitSystem.setEnums(Base::UnitsApi::getDescriptions());
    // Get the preferences/General unit system as the default for a new document
    ParameterGrp::handle hGrpu =
        GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    UnitSystem.setValue(hGrpu->GetInt("UserSchema", 0));
    ADD_PROPERTY_TYPE(Comment, (""), 0, Prop_None, "Additional tag to save a comment");
    ADD_PROPERTY_TYPE(Meta, (), 0, Prop_None, "Map with additional meta information");
    ADD_PROPERTY_TYPE(Material, (), 0, Prop_None, "Map with material properties");
    // create the uuid for the document
    Base::Uuid id;
    ADD_PROPERTY_TYPE(Id, (""), 0, Prop_None, "ID of the document");
    ADD_PROPERTY_TYPE(Uid, (id), 0, Prop_ReadOnly, "UUID of the document");

    // license stuff
    auto paramGrp {GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document")};
    auto index = static_cast<int>(paramGrp->GetInt("prefLicenseType", 0));
    auto name = "";
    std::string licenseUrl = "";
    if (index >= 0 && index < countOfLicenses) {
        name = licenseItems.at(index).at(posnOfFullName);
        auto url = licenseItems.at(index).at(posnOfUrl);
        licenseUrl = (paramGrp->GetASCII("prefLicenseUrl", url));
    }
    ADD_PROPERTY_TYPE(License, (name), 0, Prop_None, "License string of the Item");
    ADD_PROPERTY_TYPE(LicenseURL,
                      (licenseUrl.c_str()),
                      0,
                      Prop_None,
                      "URL to the license text/contract");
    ADD_PROPERTY_TYPE(ShowHidden,
                      (false),
                      0,
                      PropertyType(Prop_None),
                      "Whether to show hidden object items in the tree view");
    ADD_PROPERTY_TYPE(UseHasher,
                      (true),
                      0,
                      PropertyType(Prop_Hidden),
                      "Whether to use hasher on topological naming");

    // this creates and sets 'TransientDir' in onChanged()
    ADD_PROPERTY_TYPE(TransientDir,
                      (""),
                      0,
                      PropertyType(Prop_Transient | Prop_ReadOnly),
                      "Transient directory, where the files live while the document is open");
    ADD_PROPERTY_TYPE(Tip,
                      (nullptr),
                      0,
                      PropertyType(Prop_Transient),
                      "Link of the tip object of the document");
    ADD_PROPERTY_TYPE(TipName,
                      (""),
                      0,
                      PropertyType(Prop_Hidden | Prop_ReadOnly),
                      "Link of the tip object of the document");
    Uid.touch();
}

Document::~Document()
{
#ifdef FC_LOGUPDATECHAIN
    Console().log("-App::Document: %s %p\n", getName(), this);
#endif

    try {
        clearUndos();
    }
    catch (const boost::exception&) {
    }

#ifdef FC_LOGUPDATECHAIN
    Console().log("-Delete Features of %s \n", getName());
#endif

    d->clearDocument();

    // Remark: The API of Py::Object has been changed to set whether the wrapper owns the passed
    // Python object or not. In the constructor we forced the wrapper to own the object so we need
    // not to dec'ref the Python object any more.
    // But we must still invalidate the Python object because it doesn't need to be
    // destructed right now because the interpreter can own several references to it.
    Base::PyGILStateLocker lock;
    auto* doc = static_cast<Base::PyObjectBase*>(d->DocumentPythonObject.ptr());
    // Call before decrementing the reference counter, otherwise a heap error can occur
    doc->setInvalid();

    // remove Transient directory
    try {
        const Base::FileInfo TransDir(TransientDir.getValue());
        TransDir.deleteDirectoryRecursive();
    }
    catch (const Base::Exception& e) {
        std::cerr << "Removing transient directory failed: " << e.what() << '\n';
    }
    delete d;
}

std::string Document::getTransientDirectoryName(const std::string& uuid,
                                                const std::string& filename) const
{
    // Create a directory name of the form: {ExeName}_Doc_{UUID}_{HASH}_{PID}
    std::stringstream out;
    QCryptographicHash hash(QCryptographicHash::Sha1);
#if QT_VERSION < QT_VERSION_CHECK(6, 3, 0)
    hash.addData(filename.c_str(), filename.size());
#else
    hash.addData(QByteArrayView(filename.c_str(), filename.size()));
#endif
    out << Application::getUserCachePath() << Application::getExecutableName() << "_Doc_"
        << uuid << "_" << hash.result().toHex().left(6).constData() << "_"
        << Application::applicationPid();
    return out.str();
}

//--------------------------------------------------------------------------
// Exported functions
//--------------------------------------------------------------------------

void Document::Save(Base::Writer& writer) const
{
    d->hashers.clear();
    addStringHasher(d->Hasher);

    writer.Stream() << R"(<Document SchemaVersion="4" ProgramVersion=")"
                    << Application::Config()["BuildVersionMajor"] << "."
                    << Application::Config()["BuildVersionMinor"] << "R"
                    << Application::Config()["BuildRevision"] << "\" FileVersion=\""
                    << writer.getFileVersion() << "\" StringHasher=\"1\">\n";

    writer.incInd();

    // NOTE: This differs from LS3 Code. Persisting this table
    //       forces the assertion in Writer.addFile(...): assert(!isForceXML()); to be removed
    //       see: https://github.com/FreeCAD/FreeCAD/issues/27489
    //
    // Original code in LS3:
    //       d->Hasher->setPersistenceFileName(0);
    d->Hasher->setPersistenceFileName("StringHasher.Table");

    for (const auto o : d->objectArray) {
        o->beforeSave();
    }
    beforeSave();

    d->Hasher->Save(writer);

    writer.decInd();

    PropertyContainer::Save(writer);

    // writing the features types
    writeObjects(d->objectArray, writer);
    writer.Stream() << "</Document>" << '\n';
}

void Document::Restore(Base::XMLReader& reader)
{
    d->hashers.clear();
    d->touchedObjs.clear();
    addStringHasher(d->Hasher);
    setStatus(Document::PartialDoc, false);

    reader.readElement("Document");
    const long scheme = reader.getAttribute<long>("SchemaVersion");
    reader.DocumentSchema = static_cast<int>(scheme);
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute<const char*>("ProgramVersion");
    }
    else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = static_cast<int>(reader.getAttribute<unsigned long>("FileVersion"));
    }
    else {
        reader.FileVersion = 0;
    }

    if (reader.hasAttribute("StringHasher")) {
        d->Hasher->Restore(reader);
    }
    else {
        d->Hasher->clear();
    }

    // When this document was created the FileName and Label properties
    // were set to the absolute path or file name, respectively. To save
    // the document to the file it was loaded from or to show the file name
    // in the tree view we must restore them after loading the file because
    // they will be overridden.
    // Note: This does not affect the internal name of the document in any way
    // that is kept in Application.
    const std::string FilePath = FileName.getValue();
    const std::string DocLabel = Label.getValue();

    // read the Document Properties, when reading in Uid the transient directory gets renamed
    // automatically
    PropertyContainer::Restore(reader);

    // We must restore the correct 'FileName' property again because the stored
    // value could be invalid.
    FileName.setValue(FilePath.c_str());
    Label.setValue(DocLabel.c_str());

    // SchemeVersion "2"
    if (scheme == 2) {
        // read the feature types
        reader.readElement("Features");
        for (auto i = 0; i < reader.getAttribute<long>("Count"); i++) {
            reader.readElement("Feature");
            string type = reader.getAttribute<const char*>("type");
            string name = reader.getAttribute<const char*>("name");
            try {
                addObject(type.c_str(), name.c_str(), /*isNew=*/false);
            }
            catch (Base::Exception&) {
                Base::Console().message("Cannot create object '%s'\n", name.c_str());
            }
        }
        reader.readEndElement("Features");

        // read the features itself
        reader.readElement("FeatureData");
        for (auto i = 0; i < reader.getAttribute<long>("Count"); i++) {
            reader.readElement("Feature");
            string name = reader.getAttribute<const char*>("name");
            DocumentObject* pObj = getObject(name.c_str());
            if (pObj) {  // check if this feature has been registered
                pObj->setStatus(ObjectStatus::Restore, true);
                pObj->Restore(reader);
                pObj->setStatus(ObjectStatus::Restore, false);
            }
            reader.readEndElement("Feature");
        }
        reader.readEndElement("FeatureData");
    }  // SchemeVersion "3" or higher
    else if (scheme >= 3) {
        // read the feature types
        readObjects(reader);

        // tip object handling. First the whole document has to be read, then we
        // can restore the Tip link out of the TipName Property:
        Tip.setValue(getObject(TipName.getValue()));
    }

    reader.readEndElement("Document");
}

void DocumentP::checkStringHasher(const Base::XMLReader& reader)
{
    if (reader.hasReadFailed("StringHasher.Table.txt")) {
        Base::Console().error(QT_TRANSLATE_NOOP(
            "Notifications",
            "\nIt is recommended that the user right-click the root of "
            "the document and select Mark to recompute.\n"
            "The user should then click the Refresh button in the main toolbar.\n"));
    }
}

std::pair<bool, int> Document::addStringHasher(const StringHasherRef& hasher) const
{
    if (!hasher) {
        return std::make_pair(false, 0);
    }
    auto ret =
        d->hashers.left.insert(HasherMap::left_map::value_type(hasher, static_cast<int>(d->hashers.size())));
    if (ret.second) {
        hasher->clearMarks();
    }
    return std::make_pair(ret.second, ret.first->second);
}

StringHasherRef Document::getStringHasher(const int idx) const
{
    StringHasherRef hasher;
    if (idx < 0) {
        if (UseHasher.getValue()) {
            return d->Hasher;
        }
        return hasher;
    }
    const auto it = d->hashers.right.find(idx);
    if (it == d->hashers.right.end()) {
        hasher = new StringHasher;
        d->hashers.right.insert(HasherMap::right_map::value_type(idx, hasher));
    }
    else {
        hasher = it->second;
    }
    return hasher;
}

struct DocExportStatus
{
    Document::ExportStatus status;
    std::set<const DocumentObject*> objs;
};

static DocExportStatus exportStatus;

// Exception-safe exporting status setter
class DocumentExporting
{
public:
    explicit DocumentExporting(const std::vector<DocumentObject*>& objs)
    {
        exportStatus.status = Document::Exporting;
        exportStatus.objs.insert(objs.begin(), objs.end());
    }

    ~DocumentExporting()
    {
        exportStatus.status = Document::NotExporting;
        exportStatus.objs.clear();
    }
};

// The current implementation choose to use a static variable for exporting
// status because we can be exporting multiple objects from multiple documents
// at the same time. I see no benefits in distinguish which documents are
// exporting, so just use a static variable for global status. But the
// implementation can easily be changed here if necessary.
Document::ExportStatus Document::isExporting(const DocumentObject* obj) const
{
    if (exportStatus.status != Document::NotExporting
        && ((obj == nullptr) || exportStatus.objs.find(obj) != exportStatus.objs.end())) {
        return exportStatus.status;
    }
    return Document::NotExporting;
}
ExportInfo Document::exportInfo() const
{
    return d->exportInfo;
}
void Document::setExportInfo(const ExportInfo& info)
{
    d->exportInfo = info;
}

void Document::exportObjects(const std::vector<DocumentObject*>& obj, std::ostream& out)
{

    DocumentExporting exporting(obj);
    d->hashers.clear();

    if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
        for (auto o : obj) {
            if (o && o->isAttachedToDocument()) {
                FC_LOG("exporting " << o->getFullName());
                if (!o->getPropertyByName("_ObjectUUID")) {
                    auto prop = static_cast<PropertyUUID*>(
                        o->addDynamicProperty("App::PropertyUUID",
                                              "_ObjectUUID",
                                              nullptr,
                                              nullptr,
                                              Prop_Output | Prop_Hidden));
                    prop->setValue(Base::Uuid::createUuid());
                }
            }
        }
    }

    Base::ZipWriter writer(out);
    writer.putNextEntry("Document.xml");
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << '\n';
    writer.Stream() << R"(<Document SchemaVersion="4" ProgramVersion=")"
                    << Application::Config()["BuildVersionMajor"] << "."
                    << Application::Config()["BuildVersionMinor"] << "R"
                    << Application::Config()["BuildRevision"] << R"(" FileVersion="1">)"
                    << '\n';
    // Add this block to have the same layout as for normal documents
    writer.Stream() << "<Properties Count=\"0\">" << '\n';
    writer.Stream() << "</Properties>" << '\n';

    // writing the object types
    writeObjects(obj, writer);
    writer.Stream() << "</Document>" << '\n';

    // Hook for others to add further data.
    signalExportObjects(obj, writer);

    // write additional files
    writer.writeFiles();
    d->hashers.clear();
}

constexpr auto fcAttrDependencies {"Dependencies"};
constexpr auto fcElementObjectDeps {"ObjectDeps"};
constexpr auto fcAttrDepCount {"Count"};
constexpr auto fcAttrDepObjName {"Name"};
constexpr auto fcAttrDepAllowPartial {"AllowPartial"};
constexpr auto fcElementObjectDep {"Dep"};

void Document::writeObjectDeps(const std::vector<DocumentObject*>& objs,
                               Base::Writer& writer) const
{
    for (auto o : objs) {
        // clang-format off
        const auto& outList = o->getOutList(DocumentObject::OutListNoHidden |
                                            DocumentObject::OutListNoXLinked);
        // clang-format on

        auto objName = o->getNameInDocument();
        writer.Stream() << writer.ind()
                        << "<" << fcElementObjectDeps
                        << " " << fcAttrDepObjName << "=\""
                        << (objName ? objName : "") << "\" "
                        << fcAttrDepCount << "=\""
                        << outList.size();
        if (outList.empty()) {
            writer.Stream() << "\"/>\n";
            continue;
        }
        int partial = o->canLoadPartial();
        if (partial > 0) {
            writer.Stream() << "\" " << fcAttrDepAllowPartial << "=\"" << partial;
        }
        writer.Stream() << "\">\n";
        writer.incInd();
        for (auto dep : outList) {
            auto depName = dep ? dep->getNameInDocument() : "";
            writer.Stream() << writer.ind()
                            << "<" << fcElementObjectDep
                            << " " << fcAttrDepObjName << "=\""
                            << (depName ? depName : "") << "\"/>\n";
        }
        writer.decInd();
        writer.Stream() << writer.ind() << "</" << fcElementObjectDeps << ">\n";
    }
}

void Document::writeObjectType(const std::vector<DocumentObject*>& objs,
                               Base::Writer& writer) const
{
    for (auto it : objs) {
        writer.Stream() << writer.ind() << "<Object "
                        << "type=\"" << it->getTypeId().getName() << "\" "
                        << "name=\"" << it->getExportName() << "\" "
                        << "id=\"" << it->getID() << "\" ";

        // Only write out custom view provider types
        std::string viewType = it->getViewProviderNameStored();
        if (viewType != it->getViewProviderName()) {
            writer.Stream() << "ViewType=\"" << viewType << "\" ";
        }

        // See DocumentObjectPy::getState
        if (it->testStatus(ObjectStatus::Touch)) {
            writer.Stream() << "Touched=\"1\" ";
        }
        if (it->testStatus(ObjectStatus::Error)) {
            writer.Stream() << "Invalid=\"1\" ";
            auto desc = getErrorDescription(it);
            if (desc) {
                writer.Stream() << "Error=\"" << Property::encodeAttribute(desc) << "\" ";
            }
        }
        if (it->isFreezed()) {
            writer.Stream() << "Freeze=\"1\" ";
        }
        writer.Stream() << "/>\n";
    }
}

void Document::writeObjectData(const std::vector<DocumentObject*>& objs,
                               Base::Writer& writer) const
{
    // writing the features itself
    writer.Stream() << writer.ind() << "<ObjectData Count=\"" << objs.size() << "\">\n";

    writer.incInd();  // indentation for 'Object name'
    for (auto it : objs) {
        writer.Stream() << writer.ind() << "<Object name=\"" << it->getExportName() << "\"";
        if (it->hasExtensions()) {
            writer.Stream() << " Extensions=\"True\"";
        }

        writer.Stream() << ">\n";
        it->Save(writer);
        writer.Stream() << writer.ind() << "</Object>\n";
    }
    writer.decInd();  // indentation for 'Object name'

    writer.Stream() << writer.ind() << "</ObjectData>\n";
}

void Document::writeObjects(const std::vector<DocumentObject*>& objs,
                            Base::Writer& writer) const
{
    std::ostream& str = writer.Stream();

    // writing the features types
    writer.incInd();  // indentation for 'Objects count'
    str << writer.ind() << "<Objects Count=\"" << objs.size();
    if (!isExporting(nullptr)) {
        str << "\" " << fcAttrDependencies << "=\"1";
    }
    str << "\">\n";

    writer.incInd();  // indentation for 'Object type'

    if (!isExporting(nullptr)) {
        writeObjectDeps(objs, writer);
    }

    writeObjectType(objs, writer);

    writer.decInd();  // indentation for 'Object type'
    str << writer.ind() << "</Objects>\n";

    writeObjectData(objs, writer);
    writer.decInd();  // indentation for 'Objects count'

    // check for errors
    if (writer.hasFailed()) {
        std::cerr << "Output stream is in error state. As a result the "
                     "Document.xml file may be incomplete.\n";
        // reset the error flags to try to safe the data files
        writer.clear();
    }
}

struct DepInfo
{
    std::unordered_set<std::string> deps;
    int canLoadPartial = 0;
};

static void loadDeps(const std::string& name,
                      std::unordered_map<std::string, bool>& objs,
                      const std::unordered_map<std::string, DepInfo>& deps)
{
    const auto it = deps.find(name);
    if (it == deps.end()) {
        objs.emplace(name, true);
        return;
    }
    if (it->second.canLoadPartial != 0) {
        if (it->second.canLoadPartial == 1) {
            // canLoadPartial==1 means all its children will be created but not
            // restored, i.e. exists as if newly created object, and therefore no
            // need to load dependency of the children
            for (auto& dep : it->second.deps) {
                objs.emplace(dep, false);
            }
            objs.emplace(name, true);
        }
        else {
            objs.emplace(name, false);
        }
        return;
    }
    objs[name] = true;
    // If cannot load partial, then recurse to load all children dependency
    for (auto& dep : it->second.deps) {
        if (auto found = objs.find(dep); found != objs.end() && found->second) {
            continue;
        }
        loadDeps(dep, objs, deps);
    }
}

std::vector<DocumentObject*> Document::readObjects(Base::XMLReader& reader)
{
    d->touchedObjs.clear();
    bool keepDigits = testStatus(Document::KeepTrailingDigits);
    setStatus(Document::KeepTrailingDigits, !reader.doNameMapping());
    std::vector<DocumentObject*> objs;


    // read the object types
    reader.readElement("Objects");
    int Cnt = static_cast<int>(reader.getAttribute<long>("Count"));

    if (!reader.hasAttribute(fcAttrDependencies)) {
        d->partialLoadObjects.clear();
    }
    else if (!d->partialLoadObjects.empty()) {
        std::unordered_map<std::string, DepInfo> deps;
        for (int i = 0; i < Cnt; i++) {
            reader.readElement(fcElementObjectDeps);
            int dcount = static_cast<int>(reader.getAttribute<long>(fcAttrDepCount));
            if (dcount == 0) {
                continue;
            }
            auto& info = deps[reader.getAttribute<const char*>(fcAttrDepObjName)];
            if (reader.hasAttribute(fcAttrDepAllowPartial)) {
                info.canLoadPartial =
                    static_cast<int>(reader.getAttribute<long>(fcAttrDepAllowPartial));
            }
            for (int j = 0; j < dcount; ++j) {
                reader.readElement(fcElementObjectDep);
                const char* name = reader.getAttribute<const char*>(fcAttrDepObjName);
                if (!Base::Tools::isNullOrEmpty(name)) {
                    info.deps.insert(name);
                }
            }
            reader.readEndElement(fcElementObjectDeps);
        }
        std::vector<std::string> strings;
        strings.reserve(d->partialLoadObjects.size());
        for (auto& v : d->partialLoadObjects) {
            strings.emplace_back(v.first.c_str());
        }
        for (auto& name : strings) {
            loadDeps(name, d->partialLoadObjects, deps);
        }
        if (Cnt > static_cast<int>(d->partialLoadObjects.size())) {
            setStatus(Document::PartialDoc, true);
        }
        else {
            for (auto& v : d->partialLoadObjects) {
                if (!v.second) {
                    setStatus(Document::PartialDoc, true);
                    break;
                }
            }
            if (!testStatus(Document::PartialDoc)) {
                d->partialLoadObjects.clear();
            }
        }
    }

    long lastId = 0;
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Object");
        std::string type = reader.getAttribute<const char*>("type");
        std::string name = reader.getAttribute<const char*>("name");
        std::string viewType =
            reader.hasAttribute("ViewType") ? reader.getAttribute<const char*>("ViewType") : "";

        bool partial = false;
        if (!d->partialLoadObjects.empty()) {
            auto it = d->partialLoadObjects.find(name);
            if (it == d->partialLoadObjects.end()) {
                continue;
            }
            partial = !it->second;
        }

        if (!testStatus(Status::Importing) && reader.hasAttribute("id")) {
            // if not importing, then temporary reset lastObjectId and make the
            // following addObject() generate the correct id for this object.
            d->lastObjectId = reader.getAttribute<long>("id") - 1;
        }

        // To prevent duplicate name when export/import of objects from
        // external documents, we append those external object name with
        // @<document name>. Before importing (here means we are called by
        // importObjects), we shall strip the postfix. What the caller
        // (MergeDocument) sees is still the unstripped name mapped to a new
        // internal name, and the rest of the link properties will be able to
        // correctly unmap the names.
        auto pos = name.find('@');
        std::string _obj_name;
        const char* obj_name {nullptr};
        if (pos != std::string::npos) {
            _obj_name = name.substr(0, pos);
            obj_name = _obj_name.c_str();
        }
        else {
            obj_name = name.c_str();
        }

        try {
            // Use name from XML as is and do NOT remove trailing digits because
            // otherwise we may cause a dependency to itself
            // Example: Object 'Cut001' references object 'Cut' and removing the
            // digits we make an object 'Cut' referencing itself.
            DocumentObject* obj =
                addObject(type.c_str(), obj_name, /*isNew=*/false, viewType.c_str(), partial);
            if (obj) {
                if (lastId < obj->_Id) {
                    lastId = obj->_Id;
                }
                objs.push_back(obj);
                // use this name for the later access because an object with
                // the given name may already exist
                reader.addName(name.c_str(), obj->getNameInDocument());

                // restore touch/error status flags
                if (reader.hasAttribute("Touched")) {
                    if (reader.getAttribute<long>("Touched") != 0) {
                        d->touchedObjs.insert(obj);
                    }
                }
                if (reader.hasAttribute("Invalid")) {
                    obj->setStatus(ObjectStatus::Error,
                                   reader.getAttribute<bool>("Invalid"));
                    if (obj->isError() && reader.hasAttribute("Error")) {
                        d->addRecomputeLog(reader.getAttribute<const char*>("Error"), obj);
                    }
                }
                if (reader.hasAttribute("Freeze")) {
                    if (reader.getAttribute<long>("Freeze") != 0) {
                        obj->freeze();
                    }
                }
            }
        }
        catch (const Base::Exception& e) {
            Base::Console().error("Cannot create object '%s': (%s)\n", name.c_str(), e.what());
        }
    }
    if (!testStatus(Status::Importing)) {
        d->lastObjectId = lastId;
    }

    reader.readEndElement("Objects");
    setStatus(Document::KeepTrailingDigits, keepDigits);

    // read the features itself
    reader.clearPartialRestoreDocumentObject();
    reader.readElement("ObjectData");
    Cnt = static_cast<int>(reader.getAttribute<long>("Count"));
    for (int i = 0; i < Cnt; i++) {
        reader.readElement("Object");
        std::string name = reader.getName(reader.getAttribute<const char*>("name"));
        if (DocumentObject* pObj = getObject(name.c_str()); pObj
            && !pObj->testStatus(
                PartialObject)) {  // check if this feature has been registered
            pObj->setStatus(ObjectStatus::Restore, true);
            try {
                FC_TRACE("restoring " << pObj->getFullName());
                pObj->Restore(reader);
            }
            // Try to continue only for certain exception types if not handled
            // by the feature type. For all other exception types abort the process.
            catch (const Base::UnicodeError& e) {
                e.reportException();
            }
            catch (const Base::ValueError& e) {
                e.reportException();
            }
            catch (const Base::IndexError& e) {
                e.reportException();
            }
            catch (const Base::RuntimeError& e) {
                e.reportException();
            }
            catch (const Base::XMLAttributeError& e) {
                e.reportException();
            }

            pObj->setStatus(ObjectStatus::Restore, false);

            if (reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestoreInDocumentObject)) {
                Base::Console().error("Object \"%s\" was subject to a partial restore. As a result "
                                      "geometry may have changed or be incomplete.\n",
                                      name.c_str());
                reader.clearPartialRestoreDocumentObject();
            }
        }
        reader.readEndElement("Object");
    }
    reader.readEndElement("ObjectData");

    return objs;
}

void Document::addRecomputeObject(DocumentObject* obj) // NOLINT
{
    if (testStatus(Status::Restoring) && obj) {
        setStatus(Status::RecomputeOnRestore, true);
        d->touchedObjs.insert(obj);
        obj->touch();
    }
}

std::vector<DocumentObject*> Document::importObjects(Base::XMLReader& reader)
{
    d->hashers.clear();
    Base::FlagToggler<> flag(globalIsRestoring, false);
    Base::ObjectStatusLocker<Status, Document> restoreBit(Status::Restoring, this);
    Base::ObjectStatusLocker<Status, Document> restoreBit2(Status::Importing, this);
    ExpressionParser::ExpressionImporter expImporter(reader);
    reader.readElement("Document");
    const long scheme = reader.getAttribute<long>("SchemaVersion");
    reader.DocumentSchema = static_cast<int>(scheme);
    if (reader.hasAttribute("ProgramVersion")) {
        reader.ProgramVersion = reader.getAttribute<const char*>("ProgramVersion");
    }
    else {
        reader.ProgramVersion = "pre-0.14";
    }
    if (reader.hasAttribute("FileVersion")) {
        reader.FileVersion = static_cast<int>(reader.getAttribute<unsigned long>("FileVersion"));
    }
    else {
        reader.FileVersion = 0;
    }

    std::vector<DocumentObject*> objs = readObjects(reader);
    for (const auto o : objs) {
        if (o && o->isAttachedToDocument()) {
            o->setStatus(ObjImporting, true);
            FC_LOG("importing " << o->getFullName());
            if (const auto propUUID =
                    freecad_cast<PropertyUUID*>(o->getPropertyByName("_ObjectUUID"))) {
                auto propSource =
                    freecad_cast<PropertyUUID*>(o->getPropertyByName("_SourceUUID"));
                if (!propSource) {
                    propSource = static_cast<PropertyUUID*>(
                        o->addDynamicProperty("App::PropertyUUID",
                                              "_SourceUUID",
                                              nullptr,
                                              nullptr,
                                              Prop_Output | Prop_Hidden));
                }
                if (propSource) {
                    propSource->setValue(propUUID->getValue());
                }
                propUUID->setValue(Base::Uuid::createUuid());
            }
        }
    }

    reader.readEndElement("Document");

    signalImportObjects(objs, reader);
    afterRestore(objs, true);

    signalFinishImportObjects(objs);

    for (const auto o : objs) {
        if (o && o->isAttachedToDocument()) {
            o->setStatus(ObjImporting, false);
        }
    }

    d->hashers.clear();
    return objs;
}

unsigned int Document::getMemSize() const
{
    unsigned int size = 0;

    // size of the DocObjects in the document
    for (const auto & it : d->objectArray) {
        size += it->getMemSize();
    }

    size += d->Hasher->getMemSize();

    // size of the document properties...
    size += PropertyContainer::getMemSize();

    // Undo Redo size
    size += getUndoMemSize();

    return size;
}

static std::string checkFileName(const char* file)
{
    std::string fn(file);

    // Append extension if missing. This option is added for security reason, so
    // that the user won't accidentally overwrite other file that may be critical.
    if (GetApplication()
            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
            ->GetBool("CheckExtension", true)) {
        const char* ext = strrchr(file, '.');
        if ((ext == nullptr) || !boost::iequals(ext + 1, "fcstd")) {
            if (ext && ext[1] == 0) {
                fn += "FCStd";
            }
            else {
                fn += ".FCStd";
            }
        }
    }
    return fn;
}

bool Document::saveAs(const char* _file)
{
    const std::string file = checkFileName(_file);
    const Base::FileInfo fi(file.c_str());
    if (this->FileName.getStrValue() != file) {
        this->FileName.setValue(file);
        this->Label.setValue(fi.fileNamePure());
        this->Uid.touch();  // this forces a rename of the transient directory
    }

    return save();
}

bool Document::saveCopy(const char* file) const
{
    const std::string checked = checkFileName(file);
    return this->FileName.getStrValue() != checked ? saveToFile(checked.c_str()) : false;
}

// Save the document under the name it has been opened
bool Document::save()
{
    if (testStatus(Document::PartialDoc)) {
        FC_ERR("Partial loaded document '" << Label.getValue() << "' cannot be saved");
        // TODO We don't make this a fatal error and return 'true' to make it possible to
        // save other documents that depends on this partial opened document. We need better
        // handling to avoid touching partial documents.
        return true;
    }

    if (*(FileName.getValue()) != '\0') {
        // Save the name of the tip object in order to handle in Restore()
        if (Tip.getValue()) {
            TipName.setValue(Tip.getValue()->getNameInDocument());
        }

        const std::string LastModifiedDateString = Base::Tools::currentDateTimeString();
        LastModifiedDate.setValue(LastModifiedDateString.c_str());
        // set author if needed
        const bool saveAuthor =
            GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                ->GetBool("prefSetAuthorOnSave", false);
        if (saveAuthor) {
            const std::string Author =
                GetApplication()
                    .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                    ->GetASCII("prefAuthor", "");
            LastModifiedBy.setValue(Author.c_str());
        }

        return saveToFile(FileName.getValue());
    }

    return false;
}

bool Document::saveToFile(const char* filename) const
{
    signalStartSave(*this, filename);

    auto hGrp = GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document");
    int compression = static_cast<int>(hGrp->GetInt("CompressionLevel", 7));
    compression = Base::clamp<int>(compression, Z_NO_COMPRESSION, Z_BEST_COMPRESSION);

    bool policy = GetApplication()
                      .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                      ->GetBool("BackupPolicy", true);

    auto canonical_path = [](const char* filename) {
        try {
#ifdef FC_OS_WIN32
            QString utf8Name = QString::fromUtf8(filename);
            auto realpath = fs::weakly_canonical(fs::absolute(fs::path(utf8Name.toStdWString())));
            std::string nativePath = QString::fromStdWString(realpath.native()).toStdString();
#else
            auto realpath = fs::weakly_canonical(fs::absolute(fs::path(filename)));
            std::string nativePath = realpath.native();
#endif
            // In case some folders in the path do not exist
            auto parentPath = realpath.parent_path();
            fs::create_directories(parentPath);

            return nativePath;
        }
        catch (const std::exception&) {
#ifdef FC_OS_WIN32
            QString utf8Name = QString::fromUtf8(filename);
            auto parentPath = fs::absolute(fs::path(utf8Name.toStdWString())).parent_path();
#else
            auto parentPath = fs::absolute(fs::path(filename)).parent_path();
#endif
            fs::create_directories(parentPath);

            return std::string(filename);
        }
    };

    // realpath is canonical filename i.e. without symlink
    std::string nativePath = canonical_path(filename);

    // check if file is writeable, then block the save if it is not.
    Base::FileInfo originalFileInfo(nativePath);
    if (originalFileInfo.exists() && !originalFileInfo.isWritable()) {
        throw Base::FileException("Unable to save document because file is marked as read-only or write permission is not available.", originalFileInfo);
    }

    // make a tmp. file where to save the project data first and then rename to
    // the actual file name. This may be useful if overwriting an existing file
    // fails so that the data of the work up to now isn't lost.
    std::string uuid = Base::Uuid::createUuid();
    std::string fn = nativePath;
    if (policy) {
        fn += ".";
        fn += uuid;
    }


    // open extra scope to close ZipWriter properly
    {
        Base::FileInfo tmp(fn);
        Base::ofstream file(tmp, std::ios::out | std::ios::binary);

        Base::ZipWriter writer(file);
        if (!file.is_open()) {
            throw Base::FileException("Failed to open file", tmp);
        }

        writer.setComment("FreeCAD Document");
        writer.setLevel(compression);
        writer.putNextEntry("Document.xml");

        if (hGrp->GetBool("SaveBinaryBrep", false)) {
            writer.setMode("BinaryBrep");
        }

        writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << '\n'
                        << "<!--" << '\n'
                        << " FreeCAD Document, see https://www.freecad.org for more information..."
                        << '\n'
                        << "-->" << '\n';
        Document::Save(writer);

        // Special handling for Gui document.
        signalSaveDocument(writer);

        // write additional files
        writer.writeFiles();
        if (writer.hasErrors()) {
            // retrieve Writer error strings
            std::stringstream message;
            message << "Failed to write all data to file ";
            message << writer.getErrors().front();
            throw Base::FileException(message.str().c_str(), tmp);
        }

        GetApplication().signalSaveDocument(*this);
    }

    if (policy) {
        // if saving the project data succeeded rename to the actual file name
        int count_bak = static_cast<int>(GetApplication()
                            .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                            ->GetInt("CountBackupFiles", 1));
        bool backup = GetApplication()
                          .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                          ->GetBool("CreateBackupFiles", true);
        if (!backup) {
            count_bak = -1;
        }
        bool useFCBakExtension =
            GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                ->GetBool("UseFCBakExtension", true);
        std::string saveBackupDateFormat =
            GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
                ->GetASCII("SaveBackupDateFormat", "%Y%m%d-%H%M%S");

        BackupPolicy backupPolicy;
        if (useFCBakExtension) {
            backupPolicy.setPolicy(BackupPolicy::TimeStamp);
            backupPolicy.useBackupExtension(useFCBakExtension);
            backupPolicy.setDateFormat(saveBackupDateFormat);
        }
        else {
            backupPolicy.setPolicy(BackupPolicy::Standard);
        }
        backupPolicy.setNumberOfFiles(count_bak);
        backupPolicy.apply(fn, nativePath);
    }

    signalFinishSave(*this, filename);

    return true;
}

void Document::registerLabel(const std::string& newLabel)
{
    if (!newLabel.empty()) {
        d->objectLabelManager.addExactName(newLabel);
    }
}

void Document::unregisterLabel(const std::string& oldLabel)
{
    if (!oldLabel.empty()) {
        d->objectLabelManager.removeExactName(oldLabel);
    }
}

bool Document::containsLabel(const std::string& label)
{
    return d->objectLabelManager.containsName(label);
}

std::string Document::makeUniqueLabel(const std::string& modelLabel)
{
    if (modelLabel.empty()) {
        return {};
    }

    return d->objectLabelManager.makeUniqueName(modelLabel, 3);
}

bool Document::isAnyRestoring()
{
    return globalIsRestoring;
}

// Open the document
void Document::restore(const char* filename,
                       bool delaySignal,
                       const std::vector<std::string>& objNames)
{
    clearUndos();
    d->activeObject = nullptr;

    bool signal = false;
    Document* activeDoc = GetApplication().getActiveDocument();
    if (!d->objectArray.empty()) {
        signal = true;
        GetApplication().signalDeleteDocument(*this);
        d->clearDocument();
    }

    Base::FlagToggler<> flag(globalIsRestoring, false);

    setStatus(Document::PartialDoc, false);

    d->clearRecomputeLog();
    d->objectLabelManager.clear();
    d->objectArray.clear();
    d->objectNameManager.clear();
    d->objectMap.clear();
    d->objectIdMap.clear();
    d->lastObjectId = 0;

    if (signal) {
        GetApplication().signalNewDocument(*this, true);
        if (activeDoc == this) {
            GetApplication().setActiveDocument(this);
        }
    }

    if (!filename) {
        filename = FileName.getValue();
    }
    Base::FileInfo fi(filename);
    Base::ifstream file(fi, std::ios::in | std::ios::binary);
    std::streambuf* buf = file.rdbuf();
    std::streamoff size = buf->pubseekoff(0, std::ios::end, std::ios::in);
    buf->pubseekoff(0, std::ios::beg, std::ios::in);
    if (size < 22) {  // an empty zip archive has 22 bytes
        throw Base::FileException("Invalid project file", filename);
    }

    zipios::ZipInputStream zipstream(file);
    Base::XMLReader reader(filename, zipstream);

    if (!reader.isValid()) {
        throw Base::FileException("Error reading compression file", filename);
    }

    GetApplication().signalStartRestoreDocument(*this);
    setStatus(Document::Restoring, true);

    d->partialLoadObjects.clear();
    for (auto& name : objNames) {
        d->partialLoadObjects.emplace(name, true);
    }
    try {
        Document::Restore(reader);
    }
    catch (const Base::Exception& e) {
        Base::Console().error("Invalid Document.xml: %s\n", e.what());
        setStatus(Document::RestoreError, true);
    }

    d->partialLoadObjects.clear();
    d->programVersion = reader.ProgramVersion;

    // Special handling for Gui document, the view representations must already
    // exist, what is done in Restore().
    // Note: This file doesn't need to be available if the document has been created
    // without GUI. But if available then follow after all data files of the App document.
    signalRestoreDocument(reader);
    reader.readFiles(zipstream);

    DocumentP::checkStringHasher(reader);

    if (reader.testStatus(Base::XMLReader::ReaderStatus::PartialRestore)) {
        setStatus(Document::PartialRestore, true);
        Base::Console().error("There were errors while loading the file. Some data might have been "
                              "modified or not recovered at all. Look above for more specific "
                              "information about the objects involved.\n");
    }

    if (!delaySignal) {
        afterRestore(true);
    }
}

bool Document::afterRestore(const bool checkPartial)
{
    Base::FlagToggler<> flag(globalIsRestoring, false);
    if (!afterRestore(d->objectArray, checkPartial)) {
        FC_WARN("Reload partial document " << getName());
        GetApplication().signalPendingReloadDocument(*this);
        return false;
    }
    GetApplication().signalFinishRestoreDocument(*this);
    setStatus(Document::Restoring, false);
    return true;
}

bool Document::afterRestore(const std::vector<DocumentObject*>& objArray, bool checkPartial)
{
    checkPartial = checkPartial && testStatus(Document::PartialDoc);
    if (checkPartial && !d->touchedObjs.empty()) {
        return false;
    }

    // Some link type properties cannot restore link information until other
    // objects have been restored. For example, PropertyExpressionEngine and
    // PropertySheet with expressions containing a label reference. So we add
    // the Property::afterRestore() interface to let them sort it out. Note,
    // this API is not called in object dependency order, because the order
    // information is not ready yet.
    std::map<DocumentObject*, std::vector<Property*>> propMap;
    for (auto obj : objArray) {
        auto& props = propMap[obj];
        obj->getPropertyList(props);
        for (auto prop : props) {
            try {
                prop->afterRestore();
            }
            catch (const Base::Exception& e) {
                FC_ERR("Failed to restore " << obj->getFullName() << '.' << prop->getName() << ": "
                                            << e.what());
            }
        }
    }

    if (checkPartial && !d->touchedObjs.empty()) {
        // partial document touched, signal full reload
        return false;
    }

    std::set<DocumentObject*> objSet(objArray.begin(), objArray.end());
    auto objs = getDependencyList(objArray.empty() ? d->objectArray : objArray, DepSort);
    for (auto obj : objs) {
        if (objSet.find(obj) == objSet.end()) {
            continue;
        }
        try {
            for (auto prop : propMap[obj]) {
                prop->onContainerRestored();
            }
            bool touched = false;
            auto returnCode =
                obj->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteOnRestore, &touched);
            if (returnCode != DocumentObject::StdReturn) {
                FC_ERR("Expression engine failed to restore " << obj->getFullName() << ": "
                                                              << returnCode->Why);
                d->addRecomputeLog(returnCode);
            }
            obj->onDocumentRestored();
            if (touched) {
                d->touchedObjs.insert(obj);
            }
        }
        catch (const Base::Exception& e) {
            d->addRecomputeLog(e.what(), obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << e.what());
        }
        catch (std::exception& e) {
            d->addRecomputeLog(e.what(), obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << e.what());
        }
        catch (...) {

            // If a Python exception occurred, it must be cleared immediately.
            // Otherwise, the interpreter remains in a dirty state, causing
            // Segfaults later when FreeCAD interacts with Python.
            if (PyErr_Occurred()) {
                Base::Console().error("Python error during object restore:\n");
                PyErr_Print(); // Print the traceback to stderr/Console
                PyErr_Clear(); // Reset the interpreter state
            }

            d->addRecomputeLog("Unknown exception on restore", obj);
            FC_ERR("Failed to restore " << obj->getFullName() << ": " << "unknown exception");
        }
        if (obj->isValid()) {
            auto& props = propMap[obj];
            props.clear();
            // refresh properties in case the object changes its property list
            obj->getPropertyList(props);
            for (auto prop : props) {
                auto link = freecad_cast<PropertyLinkBase*>(prop);
                int res {0};
                std::string errMsg;
                if (link && ((res = link->checkRestore(&errMsg)) != 0)) {
                    d->touchedObjs.insert(obj);
                    if (res == 1 || checkPartial) {
                        FC_WARN(obj->getFullName() << '.' << prop->getName() << ": " << errMsg);
                        setStatus(Document::LinkStampChanged, true);
                        if (checkPartial) {
                            return false;
                        }
                    }
                    else {
                        FC_ERR(obj->getFullName() << '.' << prop->getName() << ": " << errMsg);
                        d->addRecomputeLog(errMsg, obj);
                        setStatus(Document::PartialRestore, true);
                    }
                }
            }
        }

        if (checkPartial && !d->touchedObjs.empty()) {
            // partial document touched, signal full reload
            return false;
        }
        if (!d->touchedObjs.contains(obj)) {
            obj->purgeTouched();
        }

        signalFinishRestoreObject(*obj);
    }

    d->touchedObjs.clear();
    return true;
}

bool Document::isSaved() const
{
    const std::string name = FileName.getValue();
    return !name.empty();
}

/** Label is the visible name of a document shown e.g. in the windows title
 * or in the tree view. The label almost (but not always e.g. if you manually change it)
 * matches with the file name where the document is stored to.
 * In contrast to Label the method getName() returns the internal name of the document that only
 * matches with Label when loading or creating a document because then both are set to the same
 * value. Since the internal name cannot be changed during runtime it must differ from the Label
 * after saving the document the first time or saving it under a new file name.
 * @ note More than one document can have the same label name.
 * @ note The internal is always guaranteed to be unique because @ref Application::newDocument()
 * checks for a document with the same name and makes it unique if needed. Hence you cannot rely on
 * that the internal name matches with the name you passed to Application::newDoument(). You should
 * use the method getName() instead.
 */
const char* Document::getName() const
{
    // return GetApplication().getDocumentName(this);
    return myName.c_str();
}

std::string Document::getFullName() const
{
    return myName;
}

void Document::setAutoCreated(bool value) {
    autoCreated = value;
}

bool Document::isAutoCreated() const {
    return autoCreated;
}

const char* Document::getProgramVersion() const
{
    return d->programVersion.c_str();
}

const char* Document::getFileName() const
{
    return testStatus(TempDoc) ? TransientDir.getValue() : FileName.getValue();
}

/// Remove all modifications. After this call The document becomes valid again.
void Document::purgeTouched() // NOLINT
{
    for (const auto It : d->objectArray) {
        It->purgeTouched();
    }
}

bool Document::isTouched() const
{
    for (const auto It : d->objectArray) {
        if (It->isTouched()) {
            return true;
        }
    }
    return false;
}

vector<DocumentObject*> Document::getTouched() const
{
    vector<DocumentObject*> result;

    for (auto It : d->objectArray) {
        if (It->isTouched()) {
            result.push_back(It);
        }
    }

    return result;
}

void Document::setClosable(bool c) // NOLINT
{
    setStatus(Document::Closable, c);
}

bool Document::isClosable() const
{
    return testStatus(Document::Closable);
}

int Document::countObjects() const
{
    return static_cast<int>(d->objectArray.size());
}

void Document::getLinksTo(std::set<DocumentObject*>& links,
                          const DocumentObject* obj,
                          const int options,
                          const int maxCount,
                          const std::vector<DocumentObject*>& objs) const
{
    std::map<const DocumentObject*, std::vector<DocumentObject*>> linkMap;

    for (auto o : !objs.empty() ? objs : d->objectArray) {
        if (o == obj) {
            continue;
        }
        auto linked = o;
        if ((options & GetLinkArrayElement) != 0) {
            linked = o->getLinkedObject(false);
        }
        else {
            const auto ext = o->getExtensionByType<LinkBaseExtension>(true);
            linked =
                ext ? ext->getTrueLinkedObject(false, nullptr, 0, true) : o->getLinkedObject(false);
        }

        if (linked && linked != o) {
            if ((options & GetLinkRecursive) != 0) {
                linkMap[linked].push_back(o);
            }
            else if (linked == obj || !obj) {
                if (((options & GetLinkExternal) != 0) && linked->getDocument() == o->getDocument()) {
                    continue;
                }
                if ((options & GetLinkedObject) != 0) {
                    links.insert(linked);
                }
                else {
                    links.insert(o);
                }
                if ((maxCount != 0) && maxCount <= static_cast<int>(links.size())) {
                    return;
                }
            }
        }
    }

    if ((options & GetLinkRecursive) == 0) {
        return;
    }

    std::vector<const DocumentObject*> current(1, obj);
    for (int depth = 0; !current.empty(); ++depth) {
        if (GetApplication().checkLinkDepth(depth, MessageOption::Error) == 0) {
            break;
        }
        std::vector<const DocumentObject*> next;
        for (const DocumentObject* o : current) {
            auto iter = linkMap.find(o);
            if (iter == linkMap.end()) {
                continue;
            }
            for (DocumentObject* link : iter->second) {
                if (links.insert(link).second) {
                    if ((maxCount != 0) && maxCount <= static_cast<int>(links.size())) {
                        return;
                    }
                    next.push_back(link);
                }
            }
        }
        current = std::move(next);
    }
}

bool Document::hasLinksTo(const DocumentObject* obj) const
{
    std::set<DocumentObject*> links;
    getLinksTo(links, obj, 0, 1);
    return !links.empty();
}

std::vector<DocumentObject*> Document::getInList(const DocumentObject* me) const
{
    // result list
    std::vector<DocumentObject*> result;
    // go through all objects
    for (const auto& [name, object] : d->objectMap) {
        // get the outList and search if me is in that list
        std::vector<DocumentObject*> OutList = object->getOutList();
        for (const auto obj : OutList) {
            if (obj && obj == me) {
                // add the parent object
                result.push_back(object);
            }
        }
    }
    return result;
}

// This function unifies the old _rebuildDependencyList() and
// getDependencyList().  The algorithm basically obtains the object dependency
// by recrusivly visiting the OutList of each object in the given object array.
// It makes sure to call getOutList() of each object once and only once, which
// makes it much more efficient than calling getRecursiveOutList() on each
// individual object.
//
// The problem with the original algorithm is that, it assumes the objects
// inside any OutList are all within the given object array, so it does not
// recursively call getOutList() on those dependent objects inside. This
// assumption is broken by the introduction of PropertyXLink which can link to
// external object.
//
static void buildDependencyList(const std::vector<DocumentObject*>& objectArray,
                                const int options,
                                 std::vector<DocumentObject*>* depObjs,
                                 DependencyList* depList,
                                 std::map<DocumentObject*, Vertex>* objectMap,
                                 bool* touchCheck = nullptr)
{
    std::map<DocumentObject*, std::vector<DocumentObject*>> outLists;
    std::deque<DocumentObject*> objs;

    if (objectMap) {
        objectMap->clear();
    }
    if (depList) {
        depList->clear();
    }

    const int op = ((options & Document::DepNoXLinked) != 0) ? DocumentObject::OutListNoXLinked : 0;
    for (auto obj : objectArray) {
        objs.push_back(obj);
        while (!objs.empty()) {
            auto objF = objs.front();
            objs.pop_front();
            if (!objF || !objF->isAttachedToDocument()) {
                continue;
            }

            auto it = outLists.find(objF);
            if (it != outLists.end()) {
                continue;
            }

            if (touchCheck) {
                if (objF->isTouched() || (objF->mustExecute() != 0)) {
                    // early termination on touch check
                    *touchCheck = true;
                    return;
                }
            }
            if (depObjs) {
                depObjs->push_back(objF);
            }
            if (objectMap && depList) {
                (*objectMap)[objF] = add_vertex(*depList);
            }

            auto& outList = outLists[objF];
            outList = objF->getOutList(op);
            objs.insert(objs.end(), outList.begin(), outList.end());
        }
    }

    if (objectMap && depList) {
        for (const auto& [key, objects] : outLists) {
            for (auto obj : objects) {
                if (obj && obj->isAttachedToDocument()) {
                    add_edge((*objectMap)[key], (*objectMap)[obj], *depList);
                }
            }
        }
    }
}

std::vector<DocumentObject*>
Document::getDependencyList(const std::vector<DocumentObject*>& objs, int options)
{
    std::vector<DocumentObject*> ret;
    if ((options & DepSort) == 0) {
        buildDependencyList(objs, options, &ret, nullptr, nullptr);
        return ret;
    }

    DependencyList depList;
    std::map<DocumentObject*, Vertex> objectMap;
    std::map<Vertex, DocumentObject*> vertexMap;

    buildDependencyList(objs, options, nullptr, &depList, &objectMap);

    for (auto& v : objectMap) {
        vertexMap[v.second] = v.first;
    }

    std::list<Vertex> make_order;
    try {
        boost::topological_sort(depList, std::front_inserter(make_order));
    }
    catch (const std::exception& e) {
        if ((options & DepNoCycle) != 0) {
            // Use boost::strong_components to find cycles. It groups strongly
            // connected vertices as components, and therefore each component
            // forms a cycle.
            std::vector<int> c(vertexMap.size());
            std::map<int, std::vector<Vertex>> components;
            boost::strong_components(
                depList,
                boost::make_iterator_property_map(c.begin(),
                                                  boost::get(boost::vertex_index, depList),
                                                  c[0]));
            for (size_t i = 0; i < c.size(); ++i) {
                components[c[i]].push_back(i);
            }

            FC_ERR("Dependency cycles: ");
            std::ostringstream ss;
            ss << '\n';
            for (auto& [key, vertexes] : components) {
                if (vertexes.size() == 1) {
                    // For components with only one member, we still need to
                    // check if there it is self looping.
                    auto it = vertexMap.find(vertexes[0]);
                    if (it == vertexMap.end()) {
                        continue;
                    }
                    // Try search the object in its own out list
                    for (auto obj : it->second->getOutList()) {
                        if (obj == it->second) {
                            ss << '\n' << it->second->getFullName() << '\n';
                            break;
                        }
                    }
                    continue;
                }
                // For components with more than one member, they form a loop together
                for (size_t i = 0; i < vertexes.size(); ++i) {
                    auto it = vertexMap.find(vertexes[i]);
                    if (it == vertexMap.end()) {
                        continue;
                    }
                    if (i % 6 == 0) {
                        ss << '\n';
                    }
                    ss << it->second->getFullName() << ", ";
                }
                ss << '\n';
            }
            FC_ERR(ss.str());
            FC_THROWM(Base::RuntimeError, e.what());
        }
        FC_ERR(e.what());
        ret = DocumentP::partialTopologicalSort(objs);
        std::reverse(ret.begin(), ret.end());
        return ret;
    }

    for (auto i = make_order.rbegin(); i != make_order.rend(); ++i) {
        ret.push_back(vertexMap[*i]);
    }
    return ret;
}

std::vector<Document*> Document::getDependentDocuments(const bool sort)
{
    return getDependentDocuments({this}, sort);
}

std::vector<Document*> Document::getDependentDocuments(std::vector<Document*> docs,
                                                       const bool sort)
{
    DependencyList depList;
    std::map<Document*, Vertex> docMap;
    std::map<Vertex, Document*> vertexMap;

    std::vector<Document*> ret;
    if (docs.empty()) {
        return ret;
    }

    auto outLists = PropertyXLink::getDocumentOutList();
    std::set<Document*> docSet;
    docSet.insert(docs.begin(), docs.end());
    if (sort) {
        for (auto doc : docs) {
            docMap[doc] = add_vertex(depList);
        }
    }
    while (!docs.empty()) {
        auto doc = docs.back();
        docs.pop_back();

        auto it = outLists.find(doc);
        if (it == outLists.end()) {
            continue;
        }

        const auto& vertex = docMap[doc];
        for (auto depDoc : it->second) {
            if (docSet.insert(depDoc).second) {
                docs.push_back(depDoc);
                if (sort) {
                    docMap[depDoc] = add_vertex(depList);
                }
            }
            add_edge(vertex, docMap[depDoc], depList);
        }
    }

    if (!sort) {
        ret.insert(ret.end(), docSet.begin(), docSet.end());
        return ret;
    }

    std::list<Vertex> make_order;
    try {
        boost::topological_sort(depList, std::front_inserter(make_order));
    }
    catch (const std::exception& e) {
        std::string msg("Document::getDependentDocuments: ");
        msg += e.what();
        throw Base::RuntimeError(msg);
    }

    for (auto& v : docMap) {
        vertexMap[v.second] = v.first;
    }
    for (auto rIt = make_order.rbegin(); rIt != make_order.rend(); ++rIt) {
        ret.push_back(vertexMap[*rIt]);
    }
    return ret;
}

/**
 * @brief Signal that object identifiers, typically a property or document object has been renamed.
 *
 * This function iterates through all document object in the document, and calls its
 * renameObjectIdentifiers functions.
 *
 * @param paths Map with current and new names
 */

void Document::renameObjectIdentifiers(
    const std::map<ObjectIdentifier, ObjectIdentifier>& paths,
    const std::function<bool(const DocumentObject*)>& selector) // NOLINT
{
    std::map<ObjectIdentifier, ObjectIdentifier> extendedPaths;

    auto it = paths.begin();
    while (it != paths.end()) {
        extendedPaths[it->first.canonicalPath()] = it->second.canonicalPath();
        ++it;
    }

    for (const auto object : d->objectArray) {
        if (selector(object)) {
            object->renameObjectIdentifiers(extendedPaths);
        }
    }
}

void Document::setPreRecomputeHook(const PreRecomputeHook& hook)
{
     d->_preRecomputeHook = hook;
}

int Document::recompute(const std::vector<DocumentObject*>& objs,
                        bool force,
                        bool* hasError,
                        int options)
{
    ZoneScoped;

    if (d->undoing || d->rollback) {
        if (FC_LOG_INSTANCE.isEnabled(FC_LOGLEVEL_LOG)) {
            FC_WARN("Ignore document recompute on undo/redo");
        }
        return 0;
    }

    int objectCount = 0;
    if (testStatus(Document::PartialDoc)) {
        if (mustExecute()) {
            FC_WARN("Please reload partial document '" << Label.getValue()
                                                       << "' for recomputation.");
        }
        return 0;
    }
    if (testStatus(Document::Recomputing)) {
        // this is clearly a bug in the calling instance
        FC_ERR("Recursive calling of recompute for document " << getName());
        return 0;
    }
    // The 'SkipRecompute' flag can be (tmp.) set to avoid too many
    // time expensive recomputes
    if (!force && testStatus(Document::SkipRecompute)) {
        signalSkipRecompute(*this, objs);
        return 0;
    }

    // delete recompute log
    d->clearRecomputeLog();

    FC_TIME_INIT(t);

    Base::ObjectStatusLocker<Document::Status, Document> exe(Document::Recomputing, this);

    // This will hop into the main thread, fire signalBeforeRecompute(),
    // and *block* the worker until the main thread is done, avoiding races
    // between any running Python code and the rest of the recompute call.
    if (d->_preRecomputeHook) {
        d->_preRecomputeHook();
    }

    //////////////////////////////////////////////////////////////////////////
    // FIXME Comment by Realthunder:
    // the topologicalSrot() below cannot handle partial recompute, haven't got
    // time to figure out the code yet, simply use back boost::topological_sort
    // for now, that is, rely on getDependencyList() to do the sorting. The
    // downside is, it didn't take advantage of the ready built InList, nor will
    // it report for cyclic dependency.
    //////////////////////////////////////////////////////////////////////////

    /*   // get the sorted vector of all dependent objects and go though it from the end
       auto depObjs = getDependencyList(objs.empty()?d->objectArray:objs);
       vector<DocumentObject*> topoSortedObjects = topologicalSort(depObjs);
       if (topoSortedObjects.size() != depObjs.size()){
           cerr << "Document::recompute(): cyclic dependency detected" << '\n';
           topoSortedObjects = d->partialTopologicalSort(depObjs);
       }
       std::reverse(topoSortedObjects.begin(),topoSortedObjects.end());
   */

    // alt:
    auto topoSortedObjects =
        getDependencyList(objs.empty() ? d->objectArray : objs, DepSort | options);

    for (auto obj : topoSortedObjects) {
        obj->setStatus(ObjectStatus::PendingRecompute, true);
    }

    ParameterGrp::handle hGrp =
        GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    bool canAbort = hGrp->GetBool("CanAbortRecompute", true);

    FC_TIME_INIT(t2);

    try {
        std::set<DocumentObject*> filter;
        size_t idx = 0;
        // maximum two passes to allow some form of dependency inversion
        for (int passes = 0; passes < 2 && idx < topoSortedObjects.size(); ++passes) {
            std::unique_ptr<Base::SequencerLauncher> seq;
            if (canAbort) {
                seq = std::make_unique<Base::SequencerLauncher>("Recompute...",
                                                                topoSortedObjects.size());
            }
            FC_LOG("Recompute pass " << passes);
            for (; idx < topoSortedObjects.size(); ++idx) {
                auto obj = topoSortedObjects[idx];
                if (!obj->isAttachedToDocument() || filter.find(obj) != filter.end()) {
                    continue;
                }
                // ask the object if it should be recomputed
                bool doRecompute = false;
                if (obj->mustRecompute()) {
                    doRecompute = true;
                    ++objectCount;
                    int res = _recomputeFeature(obj);
                    if (res != 0) {
                        if (hasError) {
                            *hasError = true;
                        }
                        if (res < 0) {
                            passes = 2;
                            break;
                        }
                        // if something happened filter all object in its
                        // inListRecursive from the queue then proceed
                        obj->getInListEx(filter, true);
                        filter.insert(obj);
                        continue;
                    }
                }
                if (obj->isTouched() || doRecompute) {
                    signalRecomputedObject(*obj);
                    obj->purgeTouched();
                    // set all dependent object touched to force recompute
                    for (auto inObjIt : obj->getInList()) {
                        inObjIt->enforceRecompute();
                    }
                }
                if (seq) {
                    seq->next(true);
                }
            }
            // check if all objects are recomputed but still thouched
            for (size_t i = 0; i < topoSortedObjects.size(); ++i) {
                auto obj = topoSortedObjects[i];
                obj->setStatus(ObjectStatus::Recompute2, false);
                if (!filter.contains(obj) && obj->isTouched()) {
                    if (passes > 0) {
                        FC_ERR(obj->getFullName() << " still touched after recompute");
                    }
                    else {
                        FC_LOG(obj->getFullName() << " still touched after recompute");
                        if (idx >= topoSortedObjects.size()) {
                            // let's start the next pass on the first touched object
                            idx = i;
                        }
                        obj->setStatus(ObjectStatus::Recompute2, true);
                    }
                }
            }
        }
    }
    catch (Base::Exception& e) {
        e.reportException();
    }

    FC_TIME_LOG(t2, "Recompute");

    for (auto obj : topoSortedObjects) {
        if (!obj->isAttachedToDocument()) {
            continue;
        }
        obj->setStatus(ObjectStatus::PendingRecompute, false);
        obj->setStatus(ObjectStatus::Recompute2, false);
    }

    signalRecomputed(*this, topoSortedObjects);

    FC_TIME_LOG(t, "Recompute total");

    if (!d->_RecomputeLog.empty()) {
        if (!testStatus(Status::IgnoreErrorOnRecompute)) {
            for (auto it : topoSortedObjects) {
                if (it->isError()) {
                    const char* text = getErrorDescription(it);
                    if (text) {
                        Base::Console().error("%s: %s\n", it->Label.getValue(), text);
                    }
                }
            }
        }
    }

    for (auto doc : GetApplication().getDocuments()) {
        decltype(doc->d->pendingRemove) objects;
        objects.swap(doc->d->pendingRemove);
        for (auto& o : objects) {
            try {
                if (auto obj = o.getObject()) {
                    obj->getDocument()->removeObject(obj->getNameInDocument());
                }
            }
            catch (Base::Exception& e) {
                e.reportException();
                FC_ERR("error when removing object " << o.getDocumentName() << '#'
                                                     << o.getObjectName());
            }
        }
    }
    return objectCount;
}

/*!
  Does almost the same as topologicalSort() until no object with an input degree of zero
  can be found. It then searches for objects with an output degree of zero until neither
  an object with input or output degree can be found. The remaining objects form one or
  multiple cycles.
  An alternative to this method might be:
  https://en.wikipedia.org/wiki/Tarjan%E2%80%99s_strongly_connected_components_algorithm
 */
std::vector<DocumentObject*>
DocumentP::partialTopologicalSort(const std::vector<DocumentObject*>& objects)
{
    std::vector<DocumentObject*> ret;
    ret.reserve(objects.size());
    // pairs of input and output degree
    std::map<DocumentObject*, std::pair<int, int>> countMap;

    for (auto objectIt : objects) {
        // we need inlist with unique entries
        auto in = objectIt->getInList();
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());

        // we need outlist with unique entries
        auto out = objectIt->getOutList();
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());

        countMap[objectIt] = std::make_pair(in.size(), out.size());
    }

    std::list<DocumentObject*> degIn;
    std::list<DocumentObject*> degOut;

    bool removeVertex = true;
    while (removeVertex) {
        removeVertex = false;

        // try input degree
        auto degInIt = find_if(countMap.begin(),
                               countMap.end(),
                               [](std::pair<DocumentObject*, std::pair<int, int>> vertex) -> bool {
                                   return vertex.second.first == 0;
                               });

        if (degInIt != countMap.end()) {
            removeVertex = true;
            degIn.push_back(degInIt->first);
            degInIt->second.first = degInIt->second.first - 1;

            // we need outlist with unique entries
            auto out = degInIt->first->getOutList();
            std::sort(out.begin(), out.end());
            out.erase(std::unique(out.begin(), out.end()), out.end());

            for (auto outListIt : out) {
                auto outListMapIt = countMap.find(outListIt);
                if (outListMapIt != countMap.end()) {
                    outListMapIt->second.first = outListMapIt->second.first - 1;
                }
            }
        }
    }

    // make the output degree negative if input degree is negative
    // to mark the vertex as processed
    for (auto& [obj, pair] : countMap) {
        if (pair.first < 0) {
            pair.second = -1;
        }
    }

    removeVertex = degIn.size() != objects.size();
    while (removeVertex) {
        removeVertex = false;

        auto degOutIt = std::find_if(countMap.begin(),
                                     countMap.end(),
                                     [](std::pair<DocumentObject*, std::pair<int, int>> vertex) -> bool {
                                         return vertex.second.second == 0;
                                     });

        if (degOutIt != countMap.end()) {
            removeVertex = true;
            degOut.push_front(degOutIt->first);
            degOutIt->second.second = degOutIt->second.second - 1;

            // we need inlist with unique entries
            auto in = degOutIt->first->getInList();
            std::sort(in.begin(), in.end());
            in.erase(std::unique(in.begin(), in.end()), in.end());

            for (auto inListIt : in) {
                auto inListMapIt = countMap.find(inListIt);
                if (inListMapIt != countMap.end()) {
                    inListMapIt->second.second = inListMapIt->second.second - 1;
                }
            }
        }
    }

    // at this point we have no root object any more
    for (auto countIt : countMap) {
        if (countIt.second.first > 0 && countIt.second.second > 0) {
            degIn.push_back(countIt.first);
        }
    }

    ret.insert(ret.end(), degIn.begin(), degIn.end());
    ret.insert(ret.end(), degOut.begin(), degOut.end());

    return ret;
}

std::vector<DocumentObject*>
DocumentP::topologicalSort(const std::vector<DocumentObject*>& objects) const
{
    // topological sort algorithm described here:
    // https://de.wikipedia.org/wiki/Topologische_Sortierung#Algorithmus_f.C3.BCr_das_Topologische_Sortieren
    std::vector<DocumentObject*> ret;
    ret.reserve(objects.size());
    std::map<DocumentObject*, int> countMap;

    for (auto objectIt : objects) {
        // We now support externally linked objects
        // if(!obj->isAttachedToDocument() || obj->getDocument()!=this)
        if (!objectIt->isAttachedToDocument()) {
            continue;
        }
        // we need inlist with unique entries
        auto in = objectIt->getInList();
        std::sort(in.begin(), in.end());
        in.erase(std::unique(in.begin(), in.end()), in.end());

        countMap[objectIt] = in.size();
    }

    auto rootObjeIt = std::find_if(countMap.begin(),
                                   countMap.end(),
                                   [](std::pair<DocumentObject*, int> count) -> bool {
                                       return count.second == 0;
                                   });

    if (rootObjeIt == countMap.end()) {
        std::cerr << "Document::topologicalSort: cyclic dependency detected (no root object)" << '\n';
        return ret;
    }

    while (rootObjeIt != countMap.end()) {
        rootObjeIt->second = rootObjeIt->second - 1;

        // we need outlist with unique entries
        auto out = rootObjeIt->first->getOutList();
        std::sort(out.begin(), out.end());
        out.erase(std::unique(out.begin(), out.end()), out.end());

        for (auto outListIt : out) {
            auto outListMapIt = countMap.find(outListIt);
            if (outListMapIt != countMap.end()) {
                outListMapIt->second = outListMapIt->second - 1;
            }
        }
        ret.push_back(rootObjeIt->first);

        rootObjeIt = find_if(countMap.begin(),
                             countMap.end(),
                             [](std::pair<DocumentObject*, int> count) -> bool {
                                 return count.second == 0;
                             });
    }

    return ret;
}

std::vector<DocumentObject*> Document::topologicalSort() const
{
    return d->topologicalSort(d->objectArray);
}

const char* Document::getErrorDescription(const DocumentObject* Obj) const
{
    return d->findRecomputeLog(Obj);
}

// call the recompute of the Feature and handle the exceptions and errors.
int Document::_recomputeFeature(DocumentObject* Feat) // NOLINT
{
    FC_LOG("Recomputing " << Feat->getFullName());

    DocumentObjectExecReturn* returnCode = nullptr;
    try {
        returnCode = Feat->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteNonOutput);
        if (returnCode == DocumentObject::StdReturn) {
            returnCode = Feat->recompute();
            if (returnCode == DocumentObject::StdReturn) {
                returnCode =
                    Feat->ExpressionEngine.execute(PropertyExpressionEngine::ExecuteOutput);
            }
        }
    }
    catch (Base::AbortException& e) {
        e.reportException();
        FC_LOG("Failed to recompute " << Feat->getFullName() << ": " << e.what());
        d->addRecomputeLog("User abort", Feat);
        return -1;
    }
    catch (const Base::MemoryException& e) {
        FC_ERR("Memory exception in " << Feat->getFullName() << " thrown: " << e.what());
        d->addRecomputeLog("Out of memory exception", Feat);
        return 1;
    }
    catch (Base::Exception& e) {
        e.reportException();
        FC_LOG("Failed to recompute " << Feat->getFullName() << ": " << e.what());
        d->addRecomputeLog(e.what(), Feat);
        return 1;
    }
    catch (std::exception& e) {
        FC_ERR("Exception in " << Feat->getFullName() << " thrown: " << e.what());
        d->addRecomputeLog(e.what(), Feat);
        return 1;
    }
#ifndef FC_DEBUG
    catch (...) {
        FC_ERR("Unknown exception in " << Feat->getFullName() << " thrown");
        d->addRecomputeLog("Unknown exception!", Feat);
        return 1;
    }
#endif

    if (returnCode == DocumentObject::StdReturn) {
        Feat->resetError();
    }
    else {
        returnCode->Which = Feat;
        d->addRecomputeLog(returnCode);
        FC_LOG("Failed to recompute " << Feat->getFullName() << ": " << returnCode->Why);
        return 1;
    }
    return 0;
}

bool Document::recomputeFeature(DocumentObject* feature, bool recursive)
{
    // delete recompute log
    d->clearRecomputeLog(feature);

    // verify that the feature is (active) part of the document
    if (!feature->isAttachedToDocument()) {
        return false;
    }

    if (recursive) {
        bool hasError = false;
        recompute({feature}, true, &hasError);
        return !hasError;
    }
    _recomputeFeature(feature);
    signalRecomputedObject(*feature);
    return feature->isValid();
}

DocumentObject* Document::addObject(const char* sType,
                                    const char* pObjectName,
                                    const bool isNew,
                                    const char* viewType,
                                    const bool isPartial)
{
    const Base::Type type =
        Base::Type::getTypeIfDerivedFrom(sType, DocumentObject::getClassTypeId(), true);
    if (type.isBad()) {
        std::stringstream str;
        str << "Document::addObject: '" << sType << "' is not a document object type";
        throw Base::TypeError(str.str());
    }

    void* typeInstance = type.createInstance();
    if (!typeInstance) {
        return nullptr;
    }

    auto* pcObject = static_cast<DocumentObject*>(typeInstance);
    pcObject->setDocument(this);

    _addObject(pcObject,
               pObjectName,
               AddObjectOption::SetNewStatus
                   | (isPartial ? AddObjectOption::SetPartialStatus : AddObjectOption::UnsetPartialStatus)
                   | (isNew ? AddObjectOption::DoSetup : AddObjectOption::None)
                   | AddObjectOption::ActivateObject,
               viewType);

    // return the Object
    return pcObject;
}

std::vector<DocumentObject*>
Document::addObjects(const char* sType, const std::vector<std::string>& objectNames, bool isNew)
{
    Base::Type type =
        Base::Type::getTypeIfDerivedFrom(sType, DocumentObject::getClassTypeId(), true);
    if (type.isBad()) {
        std::stringstream str;
        str << "'" << sType << "' is not a document object type";
        throw Base::TypeError(str.str());
    }

    std::vector<DocumentObject*> objects;
    objects.resize(objectNames.size());
    std::generate(objects.begin(), objects.end(), [&] {
        return static_cast<DocumentObject*>(type.createInstance());
    });
    // the type instance could be a null pointer, it is enough to check the first element
    if (!objects.empty() && !objects[0]) {
        objects.clear();
        return objects;
    }

    for (auto it = objects.begin(); it != objects.end(); ++it) {
        size_t index = std::distance(objects.begin(), it);
        DocumentObject* pcObject = *it;
        pcObject->setDocument(this);

        // Add the object but only activate the last one
        bool isLast = index == (objects.size() - 1);
        _addObject(pcObject,
                   objectNames[index].c_str(),
                   AddObjectOption::SetNewStatus
                       | (isNew ? AddObjectOption::DoSetup : AddObjectOption::None)
                       | (isLast ? AddObjectOption::ActivateObject : AddObjectOption::None));
    }

    return objects;
}

void Document::addObject(DocumentObject* obj, const char* name)
{
    if (obj->getDocument()) {
        throw Base::RuntimeError("Document object is already added to a document");
    }

    obj->setDocument(this);

    _addObject(obj, name, AddObjectOption::SetNewStatus | AddObjectOption::ActivateObject);
}

void Document::_addObject(DocumentObject* pcObject, const char* pObjectName, AddObjectOptions options, const char* viewType)
{
    // get unique name
    string ObjectName;
    if (!Base::Tools::isNullOrEmpty(pObjectName)) {
        ObjectName = getUniqueObjectName(pObjectName);
    }
    else {
        ObjectName = getUniqueObjectName(pcObject->getTypeId().getName());
    }

    // insert in the name map
    d->objectMap[ObjectName] = pcObject;
    d->objectNameManager.addExactName(ObjectName);
    // cache the pointer to the name string in the Object (for performance of
    // DocumentObject::getNameInDocument())
    pcObject->pcNameInDocument = &(d->objectMap.find(ObjectName)->first);
    // Register the current Label even though it might be about to change
    registerLabel(pcObject->Label.getStrValue());

    // generate object id and add to id map + object array
    if (pcObject->_Id == 0) {
        pcObject->_Id = ++d->lastObjectId;
    }
    d->objectIdMap[pcObject->_Id] = pcObject;
    d->objectArray.push_back(pcObject);

     // do no transactions if we do a rollback!
    if (!d->rollback) {
        // Undo stuff
        _checkTransaction(nullptr, nullptr, __LINE__);
        if (d->activeUndoTransaction) {
            d->activeUndoTransaction->addObjectDel(pcObject);
        }
     }
    // If we are restoring, don't set the Label object now; it will be restored later. This is to
    // avoid potential duplicate label conflicts later.
    if (options.testFlag(AddObjectOption::SetNewStatus) && !d->StatusBits.test(Restoring)) {
        pcObject->Label.setValue(ObjectName);
    }

    // Call the object-specific initialization
    if (!isPerformingTransaction() && options.testFlag(AddObjectOption::DoSetup)) {
        pcObject->setupObject();
    }

    if (options.testFlag(AddObjectOption::SetNewStatus)) {
        pcObject->setStatus(ObjectStatus::New, true);
    }
    if (options.testFlag(AddObjectOption::SetPartialStatus) || options.testFlag(AddObjectOption::UnsetPartialStatus)) {
        pcObject->setStatus(ObjectStatus::PartialObject, options.testFlag(AddObjectOption::SetPartialStatus));
    }

    if (Base::Tools::isNullOrEmpty(viewType)) {
        viewType = pcObject->getViewProviderNameOverride();
    }
    pcObject->_pcViewProviderName = viewType ? viewType : "";

    signalNewObject(*pcObject);

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        signalTransactionAppend(*pcObject, d->activeUndoTransaction);
    }

    if (options.testFlag(AddObjectOption::ActivateObject)) {
        d->activeObject = pcObject;
        signalActivatedObject(*pcObject);
    }
}

bool Document::containsObject(const DocumentObject* pcObject) const
{
    // We could look for the object in objectMap (keyed by object name),
    // or search in objectArray (a O(n) vector search) but looking by Id
    // in objectIdMap would be fastest.
    auto found = d->objectIdMap.find(pcObject->getID());
    return found != d->objectIdMap.end() && found->second == pcObject;
}

/// Remove an object out of the document
void Document::removeObject(const DocumentObject* object)
{
    if (object->getDocument() == this) {
        removeObject(object->getNameInDocument());
    }
}

/// Remove an object out of the document
void Document::removeObject(const char* sName)
{
    auto pos = d->objectMap.find(sName);
    if (pos == d->objectMap.end()){
        FC_MSG("Object " << sName << " already deleted in document " << getName());
        return;
    }

    if (pos->second->testStatus(ObjectStatus::PendingRecompute)) {
        // TODO: shall we allow removal if there is active undo transaction?
        FC_MSG("pending remove of " << sName << " after recomputing document " << getName());
        d->pendingRemove.emplace_back(pos->second);
        return;
    }

    _removeObject(pos->second, RemoveObjectOption::MayRemoveWhileRecomputing | RemoveObjectOption::MayDestroyOutOfTransaction);
}
void Document::_removeObject(DocumentObject* pcObject, RemoveObjectOptions options)
{
    if (!options.testFlag(RemoveObjectOption::MayRemoveWhileRecomputing) && testStatus(Document::Recomputing)) {
        FC_ERR("Cannot delete " << pcObject->getFullName() << " while recomputing");
        return;
    }

    TransactionLocker tlock;

    _checkTransaction(pcObject, nullptr, __LINE__);

    auto pos = d->objectMap.find(pcObject->getNameInDocument());
    if (pos == d->objectMap.end()) {
        FC_ERR("Internal error, could not find " << pcObject->getFullName() << " to remove");
    }

    if (options.testFlag(RemoveObjectOption::PreserveChildrenVisibility)
        && !d->rollback && d->activeUndoTransaction && pcObject->hasChildElement()) {
        // Preserve link group sub object global visibilities. Normally those
        // claimed object should be hidden in global coordinate space. However,
        // when the group is deleted, the user will naturally try to show the
        // children, which may now in the global space. When the parent is
        // undeleted, having its children shown in both the local and global
        // coordinate space is very confusing. Hence, we preserve the visibility
        // here
        for (auto& sub : pcObject->getSubObjects()) {
            if (sub.empty()) {
                continue;
            }
            if (sub[sub.size() - 1] != '.') {
                sub += '.';
            }
            auto sobj = pcObject->getSubObject(sub.c_str());
            if (sobj && sobj->getDocument() == this && !sobj->Visibility.getValue()) {
                d->activeUndoTransaction->addObjectChange(sobj, &sobj->Visibility);
            }
        }
    }

    if (d->activeObject == pcObject) {
        d->activeObject = nullptr;
    }

    // Mark the object as about to be removed
    pcObject->setStatus(ObjectStatus::Remove, true);
    if (!d->undoing && !d->rollback) {
        pcObject->unsetupObject();
    }
    signalDeletedObject(*pcObject);
    signalTransactionRemove(*pcObject, d->rollback ? nullptr : d->activeUndoTransaction);
    breakDependency(pcObject, true);

    // TODO Check me if it's needed (2015-09-01, Fat-Zer)
    // remove the tip if needed
    if (Tip.getValue() == pcObject) {
        Tip.setValue(nullptr);
        TipName.setValue("");
    }

    // remove from map
    pcObject->setStatus(ObjectStatus::Remove, false);  // Unset the bit to be on the safe side
    d->objectIdMap.erase(pcObject->_Id);
    d->objectNameManager.removeExactName(pos->first);
    unregisterLabel(pcObject->Label.getStrValue());

    // do no transactions if we do a rollback!
    if (!d->rollback && d->activeUndoTransaction) {
        d->activeUndoTransaction->addObjectNew(pcObject);
    }

    std::unique_ptr<DocumentObject> tobedestroyed;
    if ((options.testFlag(RemoveObjectOption::MayDestroyOutOfTransaction) && !d->rollback && !d->activeUndoTransaction)
        || (options.testFlag(RemoveObjectOption::DestroyOnRollback) && d->rollback)) {
        // if not saved in undo -> delete object later
        std::unique_ptr<DocumentObject> delobj(pos->second);
        tobedestroyed.swap(delobj);
        tobedestroyed->setStatus(ObjectStatus::Destroy, true);
    }

    for (auto it = d->objectArray.begin();
         it != d->objectArray.end();
         ++it) {
        if (*it == pcObject) {
            d->objectArray.erase(it);
            break;
        }
    }

    // In case the object gets deleted the pointer must be nullified
    if (tobedestroyed) {
        tobedestroyed->pcNameInDocument = nullptr;
    }

    // Erase last to avoid invalidating pcObject->pcNameInDocument
    // when it is still needed in Transaction::addObjectNew
    d->objectMap.erase(pos);
}

void Document::breakDependency(DocumentObject* pcObject, const bool clear) // NOLINT
{
    // Nullify all dependent objects
    PropertyLinkBase::breakLinks(pcObject, d->objectArray, clear);
}

std::vector<DocumentObject*>
Document::copyObject(const std::vector<DocumentObject*>& objs, bool recursive, bool returnAll)
{
    std::vector<DocumentObject*> deps;
    if (!recursive) {
        deps = objs;
    }
    else {
        deps = getDependencyList(objs, DepNoXLinked | DepSort);
    }

    if (!testStatus(TempDoc) && !isSaved() && PropertyXLink::hasXLink(deps)) {
        throw Base::RuntimeError(
            "Document must be saved at least once before link to external objects");
    }

    MergeDocuments md(this);
    // if not copying recursively then suppress possible warnings
    md.setVerbose(recursive);

    unsigned int memsize = 1000;  // ~ for the meta-information
    for (auto it : deps) {
        memsize += it->getMemSize();
    }

    // if less than ~10 MB
    bool use_buffer = (memsize < 0xA00000);
    QByteArray res;
    try {
        res.reserve(memsize);
    }
    catch (const Base::MemoryException&) {
        use_buffer = false;
    }

    std::vector<DocumentObject*> imported;
    if (use_buffer) {
        Base::ByteArrayOStreambuf obuf(res);
        std::ostream ostr(&obuf);
        exportObjects(deps, ostr);

        Base::ByteArrayIStreambuf ibuf(res);
        std::istream istr(nullptr);
        istr.rdbuf(&ibuf);
        imported = md.importObjects(istr);
    }
    else {
        static Base::FileInfo fi(Application::getTempFileName());
        Base::ofstream ostr(fi, std::ios::out | std::ios::binary);
        exportObjects(deps, ostr);
        ostr.close();

        Base::ifstream istr(fi, std::ios::in | std::ios::binary);
        imported = md.importObjects(istr);
    }

    if (returnAll || imported.size() != deps.size()) {
        return imported;
    }

    std::unordered_map<DocumentObject*, size_t> indices;
    size_t i = 0;
    for (auto o : deps) {
        indices[o] = i++;
    }
    std::vector<DocumentObject*> result;
    result.reserve(objs.size());
    for (auto o : objs) {
        result.push_back(imported[indices[o]]);
    }
    return result;
}

std::vector<DocumentObject*>
Document::importLinks(const std::vector<DocumentObject*>& objs)
{
    std::set<DocumentObject*> links;
    getLinksTo(links, nullptr, GetLinkExternal, 0, objs);

    std::vector<DocumentObject*> vecObjs;
    vecObjs.insert(vecObjs.end(), links.begin(), links.end());
    std::vector<DocumentObject*> depObjs = getDependencyList(vecObjs);
    if (depObjs.empty()) {
        FC_ERR("nothing to import");
        return depObjs;
    }

    for (auto it = depObjs.begin(); it != depObjs.end();) {
        auto obj = *it;
        if (obj->getDocument() == this) {
            it = depObjs.erase(it);
            continue;
        }
        ++it;
        if (obj->testStatus(PartialObject)) {
            throw Base::RuntimeError(
                "Cannot import partial loaded object. Please reload the current document");
        }
    }

    Base::FileInfo fi(Application::getTempFileName());
    {
        // save stuff to temp file
        Base::ofstream str(fi, std::ios::out | std::ios::binary);
        MergeDocuments mimeView(this);
        exportObjects(depObjs, str);
        str.close();
    }
    Base::ifstream str(fi, std::ios::in | std::ios::binary);
    MergeDocuments mimeView(this);
    depObjs = mimeView.importObjects(str);
    str.close();
    fi.deleteFile();

    const auto& nameMap = mimeView.getNameMap();

    // First, find all link type properties that needs to be changed
    std::map<Property*, std::unique_ptr<Property>> propMap;
    std::vector<Property*> propList;
    for (auto obj : links) {
        propList.clear();
        obj->getPropertyList(propList);
        for (auto prop : propList) {
            auto linkProp = freecad_cast<PropertyLinkBase*>(prop);
            if (linkProp && !prop->testStatus(Property::Immutable) && !obj->isReadOnly(prop)) {
                auto copy = linkProp->CopyOnImportExternal(nameMap);
                if (copy) {
                    propMap[linkProp].reset(copy);
                }
            }
        }
    }

    // Then change them in one go. Note that we don't make change in previous
    // loop, because a changed link property may break other depending link
    // properties, e.g. a link sub referring to some sub object of an xlink, If
    // that sub object is imported with a different name, and xlink is changed
    // before this link sub, it will break.
    for (auto& v : propMap) {
        v.first->Paste(*v.second);
    }

    return depObjs;
}

DocumentObject* Document::moveObject(DocumentObject* obj, const bool recursive)
{
    if (!obj) {
        return nullptr;
    }
    Document* that = obj->getDocument();
    if (that == this) {
        return nullptr;  // nothing todo
    }

    // True object move without copy is only safe when undo is off on both
    // documents.
    if (!recursive && (d->iUndoMode == 0) && (that->d->iUndoMode == 0) && !that->d->rollback) {
        // all object of the other document that refer to this object must be nullified
        that->breakDependency(obj, false);
        const std::string objname = getUniqueObjectName(obj->getNameInDocument());
        that->_removeObject(obj);
        this->_addObject(obj, objname.c_str());
        obj->setDocument(this);
        return obj;
    }

    std::vector<DocumentObject*> deps;
    if (recursive) {
        deps = getDependencyList({obj}, DepNoXLinked | DepSort);
    }
    else {
        deps.push_back(obj);
    }

    const auto objs = copyObject(deps, false);
    if (objs.empty()) {
        return nullptr;
    }
    // Some object may delete its children if deleted, so we collect the IDs
    // or all depending objects for safety reason.
    std::vector<int> ids;
    ids.reserve(deps.size());
    for (const auto o : deps) {
        ids.push_back(static_cast<int>(o->getID()));
    }

    // We only remove object if it is the moving object or it has no
    // depending objects, i.e. an empty inList, which is why we need to
    // iterate the depending list backwards.
    for (auto iter = ids.rbegin(); iter != ids.rend(); ++iter) {
        const auto o = that->getObjectByID(*iter);
        if (!o) {
            continue;
        }
        if (iter == ids.rbegin() || o->getInList().empty()) {
            that->removeObject(o->getNameInDocument());
        }
    }
    return objs.back();
}

DocumentObject* Document::getActiveObject() const
{
    return d->activeObject;
}

DocumentObject* Document::getObject(const char* Name) const
{
    const auto pos = d->objectMap.find(Name);

    return pos != d->objectMap.end() ?pos->second:nullptr;
}

DocumentObject* Document::getObjectByID(const long id) const
{
    const auto it = d->objectIdMap.find(id);

    return it != d->objectIdMap.end() ?it->second:nullptr;
}


// Note: This method is only used in Tree.cpp slotChangeObject(), see explanation there
bool Document::isIn(const DocumentObject* pFeat) const
{
    for (const auto& [key, object] : d->objectMap) {
        if (object == pFeat) {
            return true;
        }
    }

    return false;
}

const char* Document::getObjectName(const DocumentObject* pFeat) const
{
    for (const auto& [key, object] : d->objectMap) {
        if (object == pFeat) {
            return key.c_str();
        }
    }

    return nullptr;
}

std::string Document::getUniqueObjectName(const char* proposedName) const
{
    if (!proposedName || *proposedName == '\0') {
        return {};
    }
    std::string cleanName = Base::Tools::getIdentifier(proposedName);

    if (!d->objectNameManager.containsName(cleanName)) {
        // Not in use yet, name is OK
        return cleanName;
    }
    return d->objectNameManager.makeUniqueName(cleanName, 3);
}

    bool
Document::haveSameBaseName(const std::string& name, const std::string& label)
{
    // Both Labels and Names use the same decomposition rules for names,
    // i.e. the default one supplied by UniqueNameManager, so we can use either
    // of the name managers to do this test.
    return d->objectNameManager.haveSameBaseName(name, label);
}

std::string Document::getStandardObjectLabel(const char* modelName, int digitCount) const
{
    return d->objectLabelManager.makeUniqueName(modelName, digitCount);
}

std::vector<DocumentObject*> Document::getDependingObjects() const
{
    return getDependencyList(d->objectArray);
}

const std::vector<DocumentObject*>& Document::getObjects() const
{
    return d->objectArray;
}

std::vector<DocumentObject*> Document::getObjectsOfType(const Base::Type& typeId) const
{
    std::vector<DocumentObject*> Objects;
    for (auto it : d->objectArray) {
        if (it->isDerivedFrom(typeId)) {
            Objects.push_back(it);
        }
    }
    return Objects;
}

std::vector<DocumentObject*> Document::getObjectsOfType(const std::vector<Base::Type>& types) const
{
    std::vector<DocumentObject*> Objects;
    for (auto it : d->objectArray) {
        for (auto& typeId : types) {
            if (it->isDerivedFrom(typeId)) {
                Objects.push_back(it);
                break; // Prevent adding several times the same object.
            }
        }
    }
    return Objects;
}

std::vector<DocumentObject*> Document::getObjectsWithExtension(const Base::Type& typeId,
                                                               const bool derived) const
{

    std::vector<DocumentObject*> Objects;
    for (auto it : d->objectArray) {
        if (it->hasExtension(typeId, derived)) {
            Objects.push_back(it);
        }
    }
    return Objects;
}


std::vector<DocumentObject*>
Document::findObjects(const Base::Type& typeId, const char* objname, const char* label) const
{
    boost::cmatch what;
    boost::regex rx_name;
    boost::regex rx_label;

    if (objname) {
        rx_name.set_expression(objname);
    }

    if (label) {
        rx_label.set_expression(label);
    }

    std::vector<DocumentObject*> Objects;
    DocumentObject* found = nullptr;
    for (const auto it : d->objectArray) {
        if (it->isDerivedFrom(typeId)) {
            found = it;

            if (!rx_name.empty() && !boost::regex_search(it->getNameInDocument(), what, rx_name)) {
                found = nullptr;
            }

            if (!rx_label.empty() && !boost::regex_search(it->Label.getValue(), what, rx_label)) {
                found = nullptr;
            }

            if (found) {
                Objects.push_back(found);
            }
        }
    }
    return Objects;
}

int Document::countObjectsOfType(const Base::Type& typeId) const
{
    return std::count_if(d->objectMap.begin(), d->objectMap.end(), [&](const auto& it) {
        return it.second->isDerivedFrom(typeId);
    });
}

int Document::countObjectsOfType(const char* typeName) const
{
    const Base::Type type = Base::Type::fromName(typeName);
    return type.isBad() ? 0 : countObjectsOfType(type);
}

PyObject* Document::getPyObject()
{
    return Py::new_reference_to(d->DocumentPythonObject);
}

std::vector<DocumentObject*> Document::getRootObjects() const
{
    std::vector<DocumentObject*> ret;

    for (auto objectIt : d->objectArray) {
        if (objectIt->getInList().empty()) {
            ret.push_back(objectIt);
        }
    }

    return ret;
}

std::vector<DocumentObject*> Document::getRootObjectsIgnoreLinks() const
{
    std::vector<DocumentObject*> ret;

    for (const auto &objectIt : d->objectArray) {
        auto list = objectIt->getInList();
        bool noParents = list.empty();

        if (!noParents) {
            // App::Document getRootObjects returns the root objects of the dependency graph.
            // So if an object is referenced by an App::Link, it will not be returned by that
            // function. So here, as we want the tree-root level objects, we check if all the
            // parents are links. In which case it's still a root object.
            noParents = std::all_of(list.cbegin(), list.cend(), [](DocumentObject* obj) {
                return obj->isDerivedFrom<Link>();
            });
        }

        if (noParents) {
            ret.push_back(objectIt);
        }
    }

    return ret;
}

void DocumentP::findAllPathsAt(const std::vector<Node>& all_nodes,
                               const size_t id,
                               std::vector<Path>& all_paths,
                               Path tmp)
{
    if (std::ranges::find(tmp, id) != tmp.end()) {
        tmp.push_back(id);
        all_paths.push_back(std::move(tmp));
        return;  // a cycle
    }

    tmp.push_back(id);
    if (all_nodes[id].empty()) {
        all_paths.push_back(std::move(tmp));
        return;
    }

    for (size_t i = 0; i < all_nodes[id].size(); i++) {
        const Path& tmp2(tmp);
        findAllPathsAt(all_nodes, all_nodes[id][i], all_paths, tmp2);
    }
}

std::vector<std::list<DocumentObject*>>
Document::getPathsByOutList(const DocumentObject* from, const DocumentObject* to) const
{
    std::map<const DocumentObject*, size_t> indexMap;
    for (size_t i = 0; i < d->objectArray.size(); ++i) {
        indexMap[d->objectArray[i]] = i;
    }

    std::vector<Node> all_nodes(d->objectArray.size());
    for (size_t i = 0; i < d->objectArray.size(); ++i) {
        const DocumentObject* obj = d->objectArray[i];
        std::vector<DocumentObject*> outList = obj->getOutList();
        for (const auto it : outList) {
            all_nodes[i].push_back(indexMap[it]);
        }
    }

    std::vector<std::list<DocumentObject*>> array;
    if (from == to) {
        return array;
    }

    size_t index_from = indexMap[from];
    size_t index_to = indexMap[to];
    std::vector<Path> all_paths;
    DocumentP::findAllPathsAt(all_nodes, index_from, all_paths, Path());

    for (const Path& it : all_paths) {
        auto jt = std::ranges::find(it, index_to);
        if (jt != it.end()) {
            array.push_back({});
            auto& path = array.back();
            for (auto kt = it.begin(); kt != jt; ++kt) {
                path.push_back(d->objectArray[*kt]);
            }

            path.push_back(d->objectArray[*jt]);
        }
    }

    // remove duplicates
    std::sort(array.begin(), array.end());
    array.erase(std::unique(array.begin(), array.end()), array.end());

    return array;
}

bool Document::mustExecute() const
{
    if (PropertyXLink::hasXLink(this)) {
        bool touched = false;
        buildDependencyList(d->objectArray, 0, nullptr, nullptr, nullptr, &touched);
        return touched;
    }

    for (const auto It : d->objectArray) {
        if (It->isTouched() || It->mustExecute() == 1) {
            return true;
        }
    }
    return false;
}
