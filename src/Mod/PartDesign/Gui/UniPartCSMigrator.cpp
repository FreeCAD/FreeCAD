/***************************************************************************
 *   Copyright (c) 2025 Walter Steffè <walter.steffe@hierarchical-electromagnetics.com> *
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

#include "UniPartCSMigrator.h"

#include <App/Application.h>        // App::GetApplication(), signals
#include <App/Document.h>
//#include <Gui/Command.h>
#include <Base/Console.h>

#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ResetBodyPlacement.h>

using Base::Console;

using namespace PartDesignGui;
namespace sp = std::placeholders;

UniPartCSMigrator* UniPartCSMigrator::_instance = nullptr;


bool UniPartCSMigrator::needsMigration(App::Document* doc)
{
    if (!doc) return false;
    auto bodies = doc->getObjectsOfType(PartDesign::Body::getClassTypeId());
    for (auto* o : bodies) {
        auto* body = static_cast<PartDesign::Body*>(o);
        if (!body->Placement.getValue().isIdentity())
            return true;
    }
    return false;
}

UniPartCSMigrator::UniPartCSMigrator()
{
    // mirror WorkflowManager: connect a member slot to App signal
    connectFinishOpenDocument =
        App::GetApplication().signalFinishOpenDocument.connect(
            std::bind(&UniPartCSMigrator::slotFinishOpenDocument, this));
    connectFinishRestoreDocument = App::GetApplication().signalFinishRestoreDocument.connect(
            std::bind( &UniPartCSMigrator::slotFinishRestoreDocument, this, sp::_1 ) );
}

UniPartCSMigrator::~UniPartCSMigrator()
{
    if (connectFinishOpenDocument.connected())
        connectFinishOpenDocument.disconnect();
    if (connectFinishRestoreDocument.connected())
    connectFinishRestoreDocument.disconnect ();
}


void UniPartCSMigrator::slotFinishOpenDocument()
{
    for (App::Document* doc : App::GetApplication().getDocuments()) {
        if (!doc) continue;

        const std::string name = doc->getName();
        if (migrated_.count(name)) continue;      // already done

        // 1) If this doc still needs the legacy recompute, defer migration
        if (doc->testStatus(App::Document::RecomputeOnRestore)) {
            if (!pending_.count(name)) {
                pending_.insert(name);
                // store the connection somewhere so you can disconnect later
                onRecomputedConn_[name] =
                    doc->signalRecomputed.connect(
                        [this, name](const App::Document& d,
                                     const std::vector<App::DocumentObject*>& /*touched*/)
                        {
                            // fence: we might see multiple recomputes
                            if (migrated_.count(name)) return;
                
                            // get a mutable pointer from the application
                            if (auto* mdoc = App::GetApplication().getDocument(d.getName())) {
                                if (needsMigration(mdoc)) {
                                    PartDesign::resetBodiesPlacements(mdoc); // idempotent; recomputes if changed
                                    Base::Console().message(
                                        "[PD-Migrate] UniPartCS migrated after recompute: %s\n",
                                        name.c_str());
                                }
                            }
                
                            migrated_.insert(name);
                            pending_.erase(name);
                
                            // disconnect this per-doc recompute hook
                            auto it = onRecomputedConn_.find(name);
                            if (it != onRecomputedConn_.end()) {
                                it->second.disconnect();
                                onRecomputedConn_.erase(it);
                            }
                        });
                
                            }
                            continue; // skip migration now; we'll do it after recompute
                        }

        // 2) Otherwise, migrate immediately (normal case)
        if (needsMigration(doc)) {
            PartDesign::resetBodiesPlacements(doc);
            Base::Console().message("[PD-Migrate] UniPartCS migrated: %s\n", name.c_str());
        }
        migrated_.insert(name);
    }
}


void UniPartCSMigrator::slotFinishRestoreDocument( const App::Document &doc ) {
}

// ------- singleton lifecycle (like WorkflowManager) -------
void UniPartCSMigrator::init()
{
    if (!_instance)
        _instance = new UniPartCSMigrator();
}

UniPartCSMigrator* UniPartCSMigrator::instance()
{
    if (!_instance)
        UniPartCSMigrator::init();
    return _instance;
}

void UniPartCSMigrator::destruct()
{
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}


