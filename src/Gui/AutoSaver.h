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


#pragma once

#include <QObject>

#include <map>
#include <string>
#include <fastsignals/signal.h>

namespace App
{
class Document;
}  // namespace App

namespace Gui
{
/**
 * Per-document autosave scheduling state shared between document change
 * notifications, timer callbacks, and queued stable-state retries.
 *
 * State model:
 * - pendingAutosave: the document changed since the last successful autosave,
 *   or a save attempt was deferred/failed and still needs a retry.
 * - retryScheduled: a queued retry already exists, so repeated
 *   signalBecameStable() notifications should not queue another one.
 *
 * Save flow:
 * 1. Document/object changes call markPendingAutosave().
 * 2. A timer pass or queued stable-state retry calls consumePendingAutosave()
 *    to claim the current pending work for one save attempt.
 * 3. saveDocument() writes a full recovery snapshot through App::Document.
 * 4. If the document is unstable or the save fails, pendingAutosave stays set
 *    and schedulePendingAutosaveRetry() retries once the document becomes
 *    stable.
 *
 * All callbacks arrive on the GUI thread: document change signals are delivered
 * via MainThreadSignal, and timer/retry callbacks run on AutoSaver's thread.
 * No additional locking is required.
 */
class AutoSaveProperty
{
public:
    friend class AutoSaver;
    AutoSaveProperty(const App::Document* doc);
    ~AutoSaveProperty();
    int timerId;
    void markPendingAutosave();
    bool consumePendingAutosave();
    void restorePendingAutosave();
    bool hasPendingAutosave() const;

private:
    void schedulePendingAutosaveRetry();
    void slotDocumentBecameStable(const App::Document&);
    using Connection = fastsignals::connection;
    Connection documentChanged;
    Connection documentNew;
    Connection documentDeleted;
    Connection documentMod;
    Connection documentUndo;
    Connection documentRedo;
    Connection documentStable;
    std::string documentName;
    // True when newer unsaved document state still needs a save pass.
    bool pendingAutosave {false};
    // True when schedulePendingAutosaveRetry() has already queued a retry.
    bool retryScheduled {false};
};

/*!
 The class AutoSaver is used to automatically save a document to a temporary file.
 @author Werner Mayer
 */
class AutoSaver: public QObject
{
    Q_OBJECT

private:
    static AutoSaver* self;
    AutoSaver(QObject* parent);
    ~AutoSaver() override;

public:
    static AutoSaver* instance();
    /*!
     Sets the timeout in milliseconds. A value of 0 means that no timer is used.
     */
    void setTimeout(int ms);
    /*!
     Enables or disables to create compreesed recovery files.
     */
    void setCompressed(bool on);

protected:
    void slotCreateDocument(const App::Document& Doc);
    void slotDeleteDocument(const App::Document& Doc);
    void timerEvent(QTimerEvent* event) override;
    void saveDocument(const std::string&, AutoSaveProperty&);

public Q_SLOTS:
    void flushPendingSave(const QString& documentName);

private:
    int timeout; /*!< Timeout in milliseconds */
    bool compressed;
    std::map<std::string, AutoSaveProperty*> saverMap;
};

}  // namespace Gui
