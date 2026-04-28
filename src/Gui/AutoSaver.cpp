/***************************************************************************
 *   Copyright (c) 2015 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <QApplication>
#include <QTimer>
#include <QThread>

#include <App/Application.h>
#include <App/Document.h>
#include <App/RecoverySnapshot.h>
#include <Base/Console.h>
#include <Base/TimeInfo.h>
#include <Base/Tools.h>

#include "AutoSaver.h"
#include "Application.h"
#include "Document.h"
#include "MainWindow.h"
#include "WaitCursor.h"

FC_LOG_LEVEL_INIT("App", true, true)

using namespace Gui;

namespace
{

bool isGuiDocumentStableForAutoSave(const App::Document* doc)
{
    if (!doc) {
        return false;
    }

    if (auto* app = Gui::Application::Instance) {
        if (auto* guiDoc = app->getDocument(doc)) {
            if (guiDoc->isPerformingTransaction()) {
                return false;
            }
        }
    }

    return true;
}

}  // namespace

AutoSaver* AutoSaver::self = nullptr;
const int AutoSaveTimeout = 900000;

AutoSaver::AutoSaver(QObject* parent)
    : QObject(parent)
    , timeout(AutoSaveTimeout)
    , compressed(true)
{
    App::GetApplication().signalNewDocument.connect([this](const App::Document& doc, bool) {
        slotCreateDocument(doc);
    });
    App::GetApplication().signalDeleteDocument.connect([this](const App::Document& doc) {
        slotDeleteDocument(doc);
    });
}

AutoSaver::~AutoSaver() = default;

AutoSaver* AutoSaver::instance()
{
    if (!self) {
        self = new AutoSaver(QApplication::instance());
    }
    return self;
}

void AutoSaver::flushPendingSave(const QString& documentName)
{
    // This runs from a queued singleShot after signalBecameStable(). Re-check
    // that the document still exists and let saveDocument() consume any
    // outstanding pending/scheduled state for this pass.
    const auto name = documentName.toStdString();
    auto it = saverMap.find(name);
    if (it == saverMap.end()) {
        return;
    }

    try {
        saveDocument(it->first, *it->second);
    }
    catch (...) {
        Base::Console().error("Failed to auto-save document '%s'\n", it->first.c_str());
    }
}

void AutoSaver::setTimeout(int ms)
{
    timeout = Base::clamp<int>(ms, 0, 3600000);  // between 0 and 60 min

    // go through the attached documents and apply the new timeout
    for (auto& it : saverMap) {
        if (it.second->timerId > 0) {
            killTimer(it.second->timerId);
        }
        int id = timeout > 0 ? startTimer(timeout) : 0;
        it.second->timerId = id;
    }
}

void AutoSaver::setCompressed(bool on)
{
    this->compressed = on;
}

void AutoSaver::slotCreateDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    int id = timeout > 0 ? startTimer(timeout) : 0;
    AutoSaveProperty* as = new AutoSaveProperty(&Doc);
    as->timerId = id;
    saverMap.insert(std::make_pair(name, as));
}

void AutoSaver::slotDeleteDocument(const App::Document& Doc)
{
    std::string name = Doc.getName();
    std::map<std::string, AutoSaveProperty*>::iterator it = saverMap.find(name);
    if (it != saverMap.end()) {
        if (it->second->timerId > 0) {
            killTimer(it->second->timerId);
        }
        delete it->second;
        saverMap.erase(it);
    }
}

void AutoSaver::saveDocument(const std::string& name, AutoSaveProperty& saver)
{
    Q_ASSERT(QThread::currentThread() == thread());

    App::Document* doc = App::GetApplication().getDocument(name.c_str());
    if (!doc) {
        return;
    }

    // Claim the currently pending work for this save attempt. If new document
    // changes arrive while the snapshot is being written they will call
    // markPendingAutosave() again, and the post-save check below will schedule
    // another pass.
    if (!saver.consumePendingAutosave()) {
        return;
    }

    if (!doc->canWriteRecoverySnapshot() || !isGuiDocumentStableForAutoSave(doc)) {
        // Keep timer-triggered saves out of unstable document states. Instead
        // of trying to save during recompute or an open transaction, just keep
        // the save pending. signalBecameStable() will queue a retry once the
        // document returns to a state where a consistent full snapshot can be
        // written.
        saver.markPendingAutosave();
        return;
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Document"
    );
    App::RecoverySnapshotSaveOptions options;
    options.compressed = this->compressed;
    options.saveBinaryBrep = !this->compressed || hGrp->GetBool("SaveBinaryBrep", true);
    options.saveThumbnail = false;

    Gui::WaitCursor wc;
    getMainWindow()->showMessage(tr("Wait until the auto-recovery file has been saved…"), 5000);
    // qApp->processEvents();

    Base::TimeElapsed startTime;
    try {
        App::writeRecoverySnapshotToTransientDir(*doc, options);
    }
    catch (...) {
        saver.restorePendingAutosave();
        throw;
    }

    Base::Console().log(
        "Save auto-recovery file in %fs\n",
        Base::TimeElapsed::diffTimeF(startTime, Base::TimeElapsed())
    );
    if (saver.hasPendingAutosave()) {
        saver.schedulePendingAutosaveRetry();
    }
}

void AutoSaver::timerEvent(QTimerEvent* event)
{
    int id = event->timerId();
    for (auto& it : saverMap) {
        if (it.second->timerId == id) {
            try {
                saveDocument(it.first, *it.second);
                break;
            }
            catch (...) {
                Base::Console().error("Failed to auto-save document '%s'\n", it.first.c_str());
            }
        }
    }
}

// ----------------------------------------------------------------------------

AutoSaveProperty::AutoSaveProperty(const App::Document* doc)
    : timerId(-1)
{
    auto* mutableDoc = const_cast<App::Document*>(doc);
    documentChanged = mutableDoc->signalChanged.connect(
        [this](const App::Document&, const App::Property&) { markPendingAutosave(); }
    );
    documentNew = mutableDoc->signalNewObject.connect([this](const App::DocumentObject&) {
        markPendingAutosave();
    });
    documentDeleted = mutableDoc->signalDeletedObject.connect([this](const App::DocumentObject&) {
        markPendingAutosave();
    });
    documentMod = mutableDoc->signalChangedObject.connect(
        [this](const App::DocumentObject&, const App::Property&) { markPendingAutosave(); }
    );
    documentUndo = mutableDoc->signalUndo.connect([this](const App::Document&) {
        markPendingAutosave();
    });
    documentRedo = mutableDoc->signalRedo.connect([this](const App::Document&) {
        markPendingAutosave();
    });
    documentStable = mutableDoc->signalBecameStable.connect([this](const App::Document& changedDoc) {
        slotDocumentBecameStable(changedDoc);
    });

    documentName = doc->getName();
}

AutoSaveProperty::~AutoSaveProperty()
{
    documentChanged.disconnect();
    documentNew.disconnect();
    documentDeleted.disconnect();
    documentMod.disconnect();
    documentUndo.disconnect();
    documentRedo.disconnect();
    documentStable.disconnect();
}

void AutoSaveProperty::markPendingAutosave()
{
    // The save itself is deferred until the document becomes stable again or
    // until the next timer pass notices the pending flag.
    pendingAutosave = true;
}

bool AutoSaveProperty::consumePendingAutosave()
{
    if (!pendingAutosave) {
        retryScheduled = false;
        return false;
    }

    pendingAutosave = false;
    retryScheduled = false;
    return true;
}

void AutoSaveProperty::restorePendingAutosave()
{
    pendingAutosave = true;
    retryScheduled = false;
}

bool AutoSaveProperty::hasPendingAutosave() const
{
    return pendingAutosave;
}

void AutoSaveProperty::schedulePendingAutosaveRetry()
{
    if (!pendingAutosave || retryScheduled) {
        return;
    }

    retryScheduled = true;
    const QString qDocumentName = QString::fromStdString(documentName);

    // Queue a later GUI-thread pass instead of flushing inline from
    // signalBecameStable(). retryScheduled coalesces repeated stability
    // notifications, so there is at most one queued retry outstanding at a
    // time.
    QTimer::singleShot(0, AutoSaver::instance(), [qDocumentName]() {
        AutoSaver::instance()->flushPendingSave(qDocumentName);
    });
}

void AutoSaveProperty::slotDocumentBecameStable(const App::Document&)
{
    // Stability only means "it is now legal to try again". The pending and
    // scheduled flags decide whether there is actually anything left to flush.
    schedulePendingAutosaveRetry();
}


#include "moc_AutoSaver.cpp"
