/***************************************************************************
 *   Copyright (C) 2015 Alexander Golubev (Fat-Zer) <fatzer2@gmail.com>    *
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

#include "PreCompiled.h"

#ifndef _PreComp_
# include <vector>
# include <list>
# include <set>
# include <boost/bind.hpp>
# include <QMessageBox>
# include <QPushButton>
#endif

#include <Base/Exception.h>
#include <App/Application.h>
#include <App/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/Feature.h>
#include "WorkflowManager.h"


using namespace PartDesignGui;


WorkflowManager * WorkflowManager::_instance = nullptr;


WorkflowManager::WorkflowManager() {
    // Fill the map with already opened documents
   for ( auto doc : App::GetApplication().getDocuments() ) {
        slotFinishRestoreDocument ( *doc );
   }

    connectNewDocument = App::GetApplication().signalNewDocument.connect(
            boost::bind( &WorkflowManager::slotNewDocument, this, _1 ) );
    connectFinishRestoreDocument = App::GetApplication().signalFinishRestoreDocument.connect(
            boost::bind( &WorkflowManager::slotFinishRestoreDocument, this, _1 ) );
    connectDeleteDocument = App::GetApplication().signalDeleteDocument.connect(
            boost::bind( &WorkflowManager::slotDeleteDocument, this, _1 ) );
}

WorkflowManager::~WorkflowManager() {
    // they won't be automatically disconnected on destruction!
    connectNewDocument.disconnect ();
    connectFinishRestoreDocument.disconnect ();
    connectDeleteDocument.disconnect ();
}


// Those destruction/construction is not really needed and could be done in the instance()
// but to make things a bit more clear, better to keep them around.
void WorkflowManager::init() {
    if (!_instance) {
        _instance = new WorkflowManager();
    } else {
        //throw Base::RuntimeError( "Trying to init the workflow manager second time." );
    }
}

WorkflowManager *WorkflowManager::instance() {
    if (!_instance) {
        WorkflowManager::init();
    }
    return _instance;
}

void WorkflowManager::destruct() {
    if (_instance) {
        delete _instance;
        _instance = nullptr;
    }
}

void WorkflowManager::slotNewDocument( const App::Document &doc ) {
    // new document always uses new workflow
    dwMap[&doc] = Workflow::Modern;
}


void WorkflowManager::slotFinishRestoreDocument( const App::Document &doc ) {
    Workflow wf = guessWorkflow (&doc);
    // Mark document as undetermined if the guessed workflow is not new
    if( wf != Workflow::Modern ) {
        wf = Workflow::Undetermined;
    }
    dwMap[&doc] = wf;
}

void WorkflowManager::slotDeleteDocument( const App::Document &doc ) {
    dwMap.erase(&doc);
}

Workflow WorkflowManager::getWorkflowForDocument( App::Document *doc) {
    assert (doc);

    auto it = dwMap.find(doc);

    if ( it!=dwMap.end() ) {
        return it->second;
    } else {
        // We haven't yet checked the file workflow
        // May happen if e.g. file not completely loaded yet
        return Workflow::Undetermined;
    }
}

Workflow WorkflowManager::determineWorkflow(App::Document *doc) {
    Workflow rv = getWorkflowForDocument (doc);

    if (rv != Workflow::Undetermined) {
        // Return if workflow is known
        return rv;
    }

    // Guess the workflow again
    rv = guessWorkflow (doc);
    if (rv != Workflow::Modern) {
        QMessageBox msgBox(Gui::getMainWindow());

        if ( rv == Workflow::Legacy ) { // legacy messages
            msgBox.setText( QObject::tr( "The document \"%1\" you are editing was designed with an old version of "
                            "PartDesign workbench." ).arg( QString::fromStdString ( doc->getName()) ) );
            msgBox.setInformativeText (
                    QObject::tr( "Do you want to migrate in order to use modern PartDesign features?" ) );
        }
        else { // The document is already in the middle of migration
            msgBox.setText( QObject::tr( "The document \"%1\" seems to be either in the middle of"
                        " the migration process from legacy PartDesign or have a slightly broken structure."
                        ).arg( QString::fromStdString ( doc->getName()) ) );
            msgBox.setInformativeText (
                    QObject::tr( "Do you want to make the migration automatically?" ) );
        }
        msgBox.setDetailedText( QObject::tr( "Note: If you choose to migrate you won't be able to edit"
                    " the file with an older FreeCAD version.\n"
                    "If you refuse to migrate you won't be able to use new PartDesign features"
                    " like Bodies and Parts. As a result you also won't be able to use your parts"
                    " in the assembly workbench.\n"
                    "Although you will be able to migrate any moment later with 'Part Design -> Migrate'." ) );
        msgBox.setIcon( QMessageBox::Question );
        QPushButton * yesBtn      = msgBox.addButton ( QMessageBox::Yes );
        QPushButton * manuallyBtn = msgBox.addButton (
                QObject::tr ( "Migrate manually" ), QMessageBox::YesRole );

        // If it is already a document in the middle of the migration the user shouldn't refuse to migrate
        if ( rv != Workflow::Undetermined ) {
            msgBox.addButton ( QMessageBox::No  );
        }
        msgBox.setDefaultButton ( yesBtn );
        // TODO: Add some description of manual migration mode (2015-08-09, Fat-Zer)

        msgBox.exec();

        if ( msgBox.clickedButton() == yesBtn ) {
            Gui::Application::Instance->commandManager().runCommandByName("PartDesign_Migrate");
            rv = Workflow::Modern;
        } else if ( msgBox.clickedButton() == manuallyBtn ) {
            rv = Workflow::Undetermined;
        } else {
            rv = Workflow::Legacy;
        }
    }

    // Actually set the result in our map
    dwMap[ doc ] = rv;

    return rv;
}

void WorkflowManager::forceWorkflow(const App::Document *doc, Workflow wf) {
    dwMap[ doc ] = wf;
}

Workflow WorkflowManager::guessWorkflow(const App::Document *doc) {
    // Retrieve bodies of the document
    auto features = doc->getObjectsOfType( PartDesign::Feature::getClassTypeId() );

    if( features.empty() ) {
        // a new file should be done in the new workflow
        return Workflow::Modern;
    } else {
        auto bodies = doc->getObjectsOfType( PartDesign::Body::getClassTypeId() );
        if (bodies.empty()) {
            // If there are no bodies workflow is legacy
            return Workflow::Legacy;
        } else {
            bool features_without_bodies = false;

            for( auto feat: features ) {
                if( !PartDesign::Body::findBodyOf( feat ) ) {
                    features_without_bodies = true;
                    break;
                }
            }
            // if there are features not belonging to any body it means that migration was incomplete, otherwise it's Modern
            return features_without_bodies ? Workflow::Undetermined : Workflow::Modern;
        }
    }
}
