/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <algorithm>
# include <QAbstractButton>
# include <qapplication.h>
# include <qdir.h>
# include <qfileinfo.h>
# include <QKeySequence>
# include <qmessagebox.h>
# include <qstatusbar.h>
# include <boost/signals2.hpp>
# include <boost/bind.hpp>
# include <Inventor/actions/SoSearchAction.h>
# include <Inventor/nodes/SoSeparator.h>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Reader.h>
#include <Base/Writer.h>
#include <Base/Tools.h>

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/DocumentObjectGroup.h>
#include <App/Transactions.h>

#include "Application.h"
#include "MainWindow.h"
#include "Tree.h"
#include "Document.h"
#include "DocumentPy.h"
#include "Command.h"
#include "Control.h"
#include "FileDialog.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "BitmapFactory.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderDocumentObjectGroup.h"
#include "Selection.h"
#include "WaitCursor.h"
#include "Thumbnail.h"

using namespace Gui;

namespace Gui {

// Pimpl class
struct DocumentP
{
    Thumbnail thumb;
    int        _iWinCount;
    int        _iDocId;
    bool       _isClosing;
    bool       _isModified;
    ViewProvider*   _editViewProvider;
    Application*    _pcAppWnd;
    // the doc/Document
    App::Document*  _pcDocument;
    /// List of all registered views
    std::list<Gui::BaseView*> baseViews;
    /// List of all registered views
    std::list<Gui::BaseView*> passiveViews;
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*> _ViewProviderMap;
    std::map<std::string,ViewProvider*> _ViewProviderMapAnnotation;

    typedef boost::signals2::connection Connection;
    Connection connectNewObject;
    Connection connectDelObject;
    Connection connectCngObject;
    Connection connectRenObject;
    Connection connectActObject;
    Connection connectSaveDocument;
    Connection connectRestDocument;
    Connection connectStartLoadDocument;
    Connection connectFinishLoadDocument;
    Connection connectExportObjects;
    Connection connectImportObjects;
    Connection connectUndoDocument;
    Connection connectRedoDocument;
    Connection connectTransactionAppend;
    Connection connectTransactionRemove;
    typedef boost::signals2::shared_connection_block ConnectionBlock;
    ConnectionBlock connectActObjectBlocker;
};

} // namespace Gui

/* TRANSLATOR Gui::Document */

/// @namespace Gui @class Document

int Document::_iDocCount = 0;

Document::Document(App::Document* pcDocument,Application * app)
{
    d = new DocumentP;
    d->_iWinCount = 1;
    // new instance
    d->_iDocId = (++_iDocCount);
    d->_isClosing = false;
    d->_isModified = false;
    d->_pcAppWnd = app;
    d->_pcDocument = pcDocument;
    d->_editViewProvider = 0;

    // Setup the connections
    d->connectNewObject = pcDocument->signalNewObject.connect
        (boost::bind(&Gui::Document::slotNewObject, this, _1));
    d->connectDelObject = pcDocument->signalDeletedObject.connect
        (boost::bind(&Gui::Document::slotDeletedObject, this, _1));
    d->connectCngObject = pcDocument->signalChangedObject.connect
        (boost::bind(&Gui::Document::slotChangedObject, this, _1, _2));
    d->connectRenObject = pcDocument->signalRelabelObject.connect
        (boost::bind(&Gui::Document::slotRelabelObject, this, _1));
    d->connectActObject = pcDocument->signalActivatedObject.connect
        (boost::bind(&Gui::Document::slotActivatedObject, this, _1));
    d->connectActObjectBlocker = boost::signals2::shared_connection_block
        (d->connectActObject, false);
    d->connectSaveDocument = pcDocument->signalSaveDocument.connect
        (boost::bind(&Gui::Document::Save, this, _1));
    d->connectRestDocument = pcDocument->signalRestoreDocument.connect
        (boost::bind(&Gui::Document::Restore, this, _1));
    d->connectStartLoadDocument = App::GetApplication().signalStartRestoreDocument.connect
        (boost::bind(&Gui::Document::slotStartRestoreDocument, this, _1));
    d->connectFinishLoadDocument = App::GetApplication().signalFinishRestoreDocument.connect
        (boost::bind(&Gui::Document::slotFinishRestoreDocument, this, _1));

    d->connectExportObjects = pcDocument->signalExportViewObjects.connect
        (boost::bind(&Gui::Document::exportObjects, this, _1, _2));
    d->connectImportObjects = pcDocument->signalImportViewObjects.connect
        (boost::bind(&Gui::Document::importObjects, this, _1, _2, _3));

    d->connectUndoDocument = pcDocument->signalUndo.connect
        (boost::bind(&Gui::Document::slotUndoDocument, this, _1));
    d->connectRedoDocument = pcDocument->signalRedo.connect
        (boost::bind(&Gui::Document::slotRedoDocument, this, _1));

    d->connectTransactionAppend = pcDocument->signalTransactionAppend.connect
        (boost::bind(&Gui::Document::slotTransactionAppend, this, _1, _2));
    d->connectTransactionRemove = pcDocument->signalTransactionRemove.connect
        (boost::bind(&Gui::Document::slotTransactionRemove, this, _1, _2));
    // pointer to the python class
    // NOTE: As this Python object doesn't get returned to the interpreter we
    // mustn't increment it (Werner Jan-12-2006)
    _pcDocPy = new Gui::DocumentPy(this);

    if (App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/Document")->GetBool("UsingUndo",true)){
        d->_pcDocument->setUndoMode(1);
        // set the maximum stack size
        d->_pcDocument->setMaxUndoStackSize(App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")->GetInt("MaxUndoSize",20));
    }
}

Document::~Document()
{
    // disconnect everything to avoid to be double-deleted
    // in case an exception is raised somewhere
    d->connectNewObject.disconnect();
    d->connectDelObject.disconnect();
    d->connectCngObject.disconnect();
    d->connectRenObject.disconnect();
    d->connectActObject.disconnect();
    d->connectSaveDocument.disconnect();
    d->connectRestDocument.disconnect();
    d->connectStartLoadDocument.disconnect();
    d->connectFinishLoadDocument.disconnect();
    d->connectExportObjects.disconnect();
    d->connectImportObjects.disconnect();
    d->connectUndoDocument.disconnect();
    d->connectRedoDocument.disconnect();
    d->connectTransactionAppend.disconnect();
    d->connectTransactionRemove.disconnect();

    // e.g. if document gets closed from within a Python command
    d->_isClosing = true;
    // calls Document::detachView() and alter the view list
    std::list<Gui::BaseView*> temp = d->baseViews;
    for(std::list<Gui::BaseView*>::iterator it=temp.begin();it!=temp.end();++it)
        (*it)->deleteSelf();

    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::iterator jt;
    for (jt = d->_ViewProviderMap.begin();jt != d->_ViewProviderMap.end(); ++jt)
        delete jt->second;
    std::map<std::string,ViewProvider*>::iterator it2;
    for (it2 = d->_ViewProviderMapAnnotation.begin();it2 != d->_ViewProviderMapAnnotation.end(); ++it2)
        delete it2->second;

    // remove the reference from the object
    Base::PyGILStateLocker lock;
    _pcDocPy->setInvalid();
    _pcDocPy->DecRef();
    delete d;
}

//*****************************************************************************************************
// 3D viewer handling
//*****************************************************************************************************

bool Document::setEdit(Gui::ViewProvider* p, int ModNum)
{
    if (d->_editViewProvider)
        resetEdit();

    // is it really a ViewProvider of this document?
    ViewProviderDocumentObject* vp = dynamic_cast<ViewProviderDocumentObject*>(p);
    if (!vp)
        return false;

    if (d->_ViewProviderMap.find(vp->getObject()) == d->_ViewProviderMap.end())
        return false;

    View3DInventor *activeView = dynamic_cast<View3DInventor *>(getActiveView());
    // if the currently active view is not the 3d view search for it and activate it
    if (!activeView) {
        activeView = dynamic_cast<View3DInventor *>(getViewOfViewProvider(p));
        if (activeView)
            getMainWindow()->setActiveWindow(activeView);
    }

    if (activeView && activeView->getViewer()->setEditingViewProvider(p,ModNum)) {
        d->_editViewProvider = p;
        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
        if (dlg)
            dlg->setDocumentName(this->getDocument()->getName());
        if (d->_editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) 
            signalInEdit(*(static_cast<ViewProviderDocumentObject*>(d->_editViewProvider)));
    }
    else {
        return false;
    }

    return true;
}

void Document::resetEdit(void)
{
    std::list<Gui::BaseView*>::iterator it;
    if (d->_editViewProvider) {
        for (it = d->baseViews.begin();it != d->baseViews.end();++it) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*it);
            if (activeView)
                activeView->getViewer()->resetEditingViewProvider();
        }

        // Nullify the member variable before calling finishEditing().
        // This is to avoid a possible stack overflow when a view provider wrongly
        // invokes the document's resetEdit() method.
        ViewProvider* editViewProvider = d->_editViewProvider;
        d->_editViewProvider = nullptr;

        editViewProvider->finishEditing();
        if (editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
            signalResetEdit(*(static_cast<ViewProviderDocumentObject*>(editViewProvider)));
    }
}

ViewProvider *Document::getInEdit(void) const
{
    if (d->_editViewProvider) {
        // there is only one 3d view which is in edit mode
        View3DInventor *activeView = dynamic_cast<View3DInventor *>(getActiveView());
        if (activeView && activeView->getViewer()->isEditingViewProvider())
            return d->_editViewProvider;
    }

    return 0;
}

void Document::setAnnotationViewProvider(const char* name, ViewProvider *pcProvider)
{
    std::list<Gui::BaseView*>::iterator vIt;

    // already in ?
    std::map<std::string,ViewProvider*>::iterator it = d->_ViewProviderMapAnnotation.find(name);
    if (it != d->_ViewProviderMapAnnotation.end())
        removeAnnotationViewProvider(name);

    // add 
    d->_ViewProviderMapAnnotation[name] = pcProvider;

    // cycling to all views of the document
    for (vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
        View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
        if (activeView)
            activeView->getViewer()->addViewProvider(pcProvider);
    }
}

ViewProvider * Document::getAnnotationViewProvider(const char* name) const
{
    std::map<std::string,ViewProvider*>::const_iterator it = d->_ViewProviderMapAnnotation.find(name);
    return ( (it != d->_ViewProviderMapAnnotation.end()) ? it->second : 0 );
}

void Document::removeAnnotationViewProvider(const char* name)
{
    std::map<std::string,ViewProvider*>::iterator it = d->_ViewProviderMapAnnotation.find(name);
    std::list<Gui::BaseView*>::iterator vIt;

    // cycling to all views of the document
    for (vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
        View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
        if (activeView)
            activeView->getViewer()->removeViewProvider(it->second);
    }

    delete it->second;
    d->_ViewProviderMapAnnotation.erase(it); 
}


ViewProvider* Document::getViewProvider(const App::DocumentObject* Feat) const
{
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator
    it = d->_ViewProviderMap.find( Feat );
    return ( (it != d->_ViewProviderMap.end()) ? it->second : 0 );
}

std::vector<ViewProvider*> Document::getViewProvidersOfType(const Base::Type& typeId) const
{
    std::vector<ViewProvider*> Objects;
    for (std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator it = 
         d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it ) {
        if (it->second->getTypeId().isDerivedFrom(typeId))
            Objects.push_back(it->second);
    }
    return Objects;
}

ViewProvider *Document::getViewProviderByName(const char* name) const
{
    // first check on feature name
    App::DocumentObject *pcFeat = getDocument()->getObject(name);

    if (pcFeat)
    {
        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator
        it = d->_ViewProviderMap.find( pcFeat );

        if (it != d->_ViewProviderMap.end())
            return it->second;
    } else {
        // then try annotation name
        std::map<std::string,ViewProvider*>::const_iterator it2 = d->_ViewProviderMapAnnotation.find( name );

        if (it2 != d->_ViewProviderMapAnnotation.end())
            return it2->second;
    }

    return 0;
}

bool Document::isShow(const char* name)
{
    ViewProvider* pcProv = getViewProviderByName(name);
    return pcProv ? pcProv->isShow() : false;
}

/// put the feature in show
void Document::setShow(const char* name)
{
    ViewProvider* pcProv = getViewProviderByName(name);

    if (pcProv && pcProv->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        ((ViewProviderDocumentObject*)pcProv)->Visibility.setValue(true);
    }
}

/// set the feature in Noshow
void Document::setHide(const char* name)
{
    ViewProvider* pcProv = getViewProviderByName(name);

    if (pcProv && pcProv->getTypeId().isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        ((ViewProviderDocumentObject*)pcProv)->Visibility.setValue(false);
    }
}

/// set the feature in Noshow
void Document::setPos(const char* name, const Base::Matrix4D& rclMtrx)
{
    ViewProvider* pcProv = getViewProviderByName(name);
    if (pcProv)
        pcProv->setTransformation(rclMtrx);

}

//*****************************************************************************************************
// Document
//*****************************************************************************************************
void Document::slotNewObject(const App::DocumentObject& Obj)
{
    ViewProviderDocumentObject* pcProvider = static_cast<ViewProviderDocumentObject*>(getViewProvider(&Obj));
    if (!pcProvider) {
        //Base::Console().Log("Document::slotNewObject() called\n");
        std::string cName = Obj.getViewProviderName();
        if (cName.empty()) {
            // handle document object with no view provider specified
            Base::Console().Log("%s has no view provider specified\n", Obj.getTypeId().getName());
            return;
        }

        setModified(true);
        Base::BaseClass* base = static_cast<Base::BaseClass*>(Base::Type::createInstanceByName(cName.c_str(),true));
        if (base) {
            // type not derived from ViewProviderDocumentObject!!!
            assert(base->getTypeId().isDerivedFrom(Gui::ViewProviderDocumentObject::getClassTypeId()));
            pcProvider = static_cast<ViewProviderDocumentObject*>(base);
            d->_ViewProviderMap[&Obj] = pcProvider;

            try {
                // if successfully created set the right name and calculate the view
                //FIXME: Consider to change argument of attach() to const pointer
                pcProvider->attach(const_cast<App::DocumentObject*>(&Obj));
                pcProvider->updateView();
                pcProvider->setActiveMode();
            }
            catch(const Base::MemoryException& e){
                Base::Console().Error("Memory exception in '%s' thrown: %s\n",Obj.getNameInDocument(),e.what());
            }
            catch(Base::Exception &e){
                e.ReportException();
            }
#ifndef FC_DEBUG
            catch(...){
                Base::Console().Error("App::Document::_RecomputeFeature(): Unknown exception in Feature \"%s\" thrown\n",Obj.getNameInDocument());
            }
#endif
        }
        else {
            Base::Console().Warning("Gui::Document::slotNewObject() no view provider for the object %s found\n",cName.c_str());
        }
    }

    if (pcProvider) {
        std::list<Gui::BaseView*>::iterator vIt;
        // cycling to all views of the document
        for (vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
            if (activeView)
                activeView->getViewer()->addViewProvider(pcProvider);
        }

        // adding to the tree
        signalNewObject(*pcProvider);

        // it is possible that a new viewprovider already claims children
        handleChildren3D(pcProvider);
    }
}

void Document::slotDeletedObject(const App::DocumentObject& Obj)
{
    std::list<Gui::BaseView*>::iterator vIt;
    setModified(true);
    //Base::Console().Log("Document::slotDeleteObject() called\n");
  
    // cycling to all views of the document
    ViewProvider* viewProvider = getViewProvider(&Obj);
#if 0 // With this we can show child objects again if this method was called by undo
    viewProvider->onDelete(std::vector<std::string>());
#endif
    if (viewProvider && viewProvider->getTypeId().isDerivedFrom
        (ViewProviderDocumentObject::getClassTypeId())) {
        // go through the views
        for (vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
            if (activeView) {
                if (d->_editViewProvider == viewProvider)
                    resetEdit();
                activeView->getViewer()->removeViewProvider(viewProvider);
            }
        }

        // removing from tree
        signalDeletedObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
    }
}

void Document::slotChangedObject(const App::DocumentObject& Obj, const App::Property& Prop)
{
    //Base::Console().Log("Document::slotChangedObject() called\n");
    ViewProvider* viewProvider = getViewProvider(&Obj);
    if (viewProvider) {
        try {
            viewProvider->update(&Prop);
        }
        catch(const Base::MemoryException& e) {
            Base::Console().Error("Memory exception in '%s' thrown: %s\n",Obj.getNameInDocument(),e.what());
        }
        catch(Base::Exception& e){
            e.ReportException();
        }
        catch(const std::exception& e){
            Base::Console().Error("C++ exception in '%s' thrown: %s\n",Obj.getNameInDocument(),e.what());
        }
        catch (...) {
            Base::Console().Error("Cannot update representation for '%s'.\n", Obj.getNameInDocument());
        }

        handleChildren3D(viewProvider);

        if (viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
            signalChangedObject(static_cast<ViewProviderDocumentObject&>(*viewProvider), Prop);
    }

    // a property of an object has changed
    setModified(true);
}

void Document::slotRelabelObject(const App::DocumentObject& Obj)
{
    ViewProvider* viewProvider = getViewProvider(&Obj);
    if (viewProvider && viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        signalRelabelObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
    }
}

void Document::slotTransactionAppend(const App::DocumentObject& obj, App::Transaction* transaction)
{
    ViewProvider* viewProvider = getViewProvider(&obj);
    if (viewProvider && viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        transaction->addObjectDel(viewProvider);
    }
}

void Document::slotTransactionRemove(const App::DocumentObject& obj, App::Transaction* transaction)
{
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator
    it = d->_ViewProviderMap.find(&obj);
    if (it != d->_ViewProviderMap.end()) {
        ViewProvider* viewProvider = it->second;

        // Issue #0003540:
        // When deleting a view provider that claims the Inventor nodes
        // of its children then we must update the 3d viewers to re-add
        // the root nodes if needed.
        bool rebuild = false;
        SoGroup* childGroup =  viewProvider->getChildRoot();
        if (childGroup && childGroup->getNumChildren() > 0)
            rebuild = true;

        d->_ViewProviderMap.erase(&obj);
        // transaction being a nullptr indicates that undo/redo is off and the object
        // can be safely deleted
        if (transaction)
            transaction->addObjectNew(viewProvider);
        else
            delete viewProvider;

        if (rebuild)
            rebuildRootNodes();
    }
}

void Document::slotActivatedObject(const App::DocumentObject& Obj)
{
    ViewProvider* viewProvider = getViewProvider(&Obj);
    if (viewProvider && viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        signalActivatedObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
    }
}

void Document::slotUndoDocument(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    
    signalUndoDocument(*this);  
}

void Document::slotRedoDocument(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    
    signalRedoDocument(*this);   
}

void Document::addViewProvider(Gui::ViewProviderDocumentObject* vp)
{
    // Hint: The undo/redo first adds the view provider to the Gui
    // document before adding the objects to the App document.

    // the view provider is added by TransactionViewProvider and an
    // object can be there only once
    assert(d->_ViewProviderMap.find(vp->getObject()) == d->_ViewProviderMap.end());
    vp->setStatus(Detach, false);
    d->_ViewProviderMap[vp->getObject()] = vp;
}

void Document::setModified(bool b)
{
    d->_isModified = b;
    
    std::list<MDIView*> mdis = getMDIViews();
    for (std::list<MDIView*>::iterator it = mdis.begin(); it != mdis.end(); ++it) {
        (*it)->setWindowModified(b);
    }
}

bool Document::isModified() const
{
    return d->_isModified;
}


ViewProvider* Document::getViewProviderByPathFromTail(SoPath * path) const
{
    // Get the lowest root node in the pick path!
    for (int i = 0; i < path->getLength(); i++) {
        SoNode *node = path->getNodeFromTail(i);
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator it;
            for(it = d->_ViewProviderMap.begin();it!= d->_ViewProviderMap.end();++it) {
                if (node == it->second->getRoot())
                    return it->second;
            }
         }
    }

    return 0;
}



App::Document* Document::getDocument(void) const
{
    return d->_pcDocument;
}

/// Save the document
bool Document::save(void)
{
    if (d->_pcDocument->isSaved()) {
        try {
            Gui::WaitCursor wc;
            Command::doCommand(Command::Doc,"App.getDocument(\"%s\").save()"
                                           ,d->_pcDocument->getName());
            setModified(false);
        }
        catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Saving document failed"),
                QString::fromLatin1(e.what()));
        }
        return true;
    }
    else {
        return saveAs();
    }
}

/// Save the document under a new file name
bool Document::saveAs(void)
{
    getMainWindow()->showMessage(QObject::tr("Save document under new filename..."));

    QString exe = qApp->applicationName();
    QString fn = FileDialog::getSaveFileName(getMainWindow(), QObject::tr("Save %1 Document").arg(exe), 
        QString(), QString::fromLatin1("%1 %2 (*.FCStd)").arg(exe, QObject::tr("Document")));
    if (!fn.isEmpty()) {
        QFileInfo fi;
        fi.setFile(fn);

        const char * DocName = App::GetApplication().getDocumentName(getDocument());

        // save as new file name
        try {
            Gui::WaitCursor wc;
            std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(fn.toUtf8());
            Command::doCommand(Command::Doc,"App.getDocument(\"%s\").saveAs(u\"%s\")"
                                           , DocName, escapedstr.c_str());
            setModified(false);
            getMainWindow()->appendRecentFile(fi.filePath());
        }
        catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), QObject::tr("Saving document failed"),
                QString::fromLatin1(e.what()));
        }
        return true;
    }
    else {
        getMainWindow()->showMessage(QObject::tr("Saving aborted"), 2000);
        return false;
    }
}

/// Save a copy of the document under a new file name
bool Document::saveCopy(void)
{
    getMainWindow()->showMessage(QObject::tr("Save a copy of the document under new filename..."));

    QString exe = qApp->applicationName();
    QString fn = FileDialog::getSaveFileName(getMainWindow(), QObject::tr("Save %1 Document").arg(exe), 
                                             QString(), QObject::tr("%1 document (*.FCStd)").arg(exe));
    if (!fn.isEmpty()) {
        QFileInfo fi;
        fi.setFile(fn);

        const char * DocName = App::GetApplication().getDocumentName(getDocument());

        // save as new file name
        Gui::WaitCursor wc;
        Command::doCommand(Command::Doc,"App.getDocument(\"%s\").saveCopy(\"%s\")"
                                       , DocName, (const char*)fn.toUtf8());

        return true;
    }
    else {
        getMainWindow()->showMessage(QObject::tr("Saving aborted"), 2000);
        return false;
    }
}

unsigned int Document::getMemSize (void) const
{
    unsigned int size = 0;

    // size of the view providers in the document
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator it;
    for (it = d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it)
        size += it->second->getMemSize();
    return size;
}

/** 
 * Adds a separate XML file to the projects file that contains information about the view providers.
 */
void Document::Save (Base::Writer &writer) const
{
    // It's only possible to add extra information if force of XML is disabled
    if (writer.isForceXML() == false) {
        writer.addFile("GuiDocument.xml", this);

        if (App::GetApplication().GetParameterGroupByPath
            ("User parameter:BaseApp/Preferences/Document")->GetBool("SaveThumbnail",false)) {
            std::list<MDIView*> mdi = getMDIViews();
            for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
                if ((*it)->getTypeId().isDerivedFrom(View3DInventor::getClassTypeId())) {
                    View3DInventorViewer* view = static_cast<View3DInventor*>(*it)->getViewer();
                    d->thumb.setFileName(d->_pcDocument->FileName.getValue());
                    d->thumb.setSize(128);
                    d->thumb.setViewer(view);
                    d->thumb.Save(writer);
                    break;
                }
            }
        }
    }
}

/** 
 * Loads a separate XML file from the projects file with information about the view providers.
 */
void Document::Restore(Base::XMLReader &reader)
{
    reader.addFile("GuiDocument.xml",this);
    // hide all elements to avoid to update the 3d view when loading data files
    // RestoreDocFile then restores the visibility status again
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::iterator it;
    for (it = d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it) {
        it->second->startRestoring();
    }
}

/**
 * Restores the properties of the view providers.
 */
void Document::RestoreDocFile(Base::Reader &reader)
{
    // We must create an XML parser to read from the input stream
    Base::XMLReader xmlReader("GuiDocument.xml", reader);
    xmlReader.FileVersion = reader.getFileVersion();

    int i,Cnt;

    xmlReader.readElement("Document");
    long scheme = xmlReader.getAttributeAsInteger("SchemaVersion");
    xmlReader.DocumentSchema = scheme;

    // At this stage all the document objects and their associated view providers exist.
    // Now we must restore the properties of the view providers only.
    //
    // SchemeVersion "1"
    if (scheme == 1) {
        // read the viewproviders itself
        xmlReader.readElement("ViewProviderData");
        Cnt = xmlReader.getAttributeAsInteger("Count");
        for (i=0 ;i<Cnt ;i++) {
            xmlReader.readElement("ViewProvider");
            std::string name = xmlReader.getAttribute("name");
            bool expanded = false;
            if (xmlReader.hasAttribute("expanded")) {
                const char* attr = xmlReader.getAttribute("expanded");
                if (strcmp(attr,"1") == 0) {
                    expanded = true;
                }
            }
            ViewProvider* pObj = getViewProviderByName(name.c_str());
            if (pObj) // check if this feature has been registered
                pObj->Restore(xmlReader);
            if (pObj && expanded) {
                Gui::ViewProviderDocumentObject* vp = static_cast<Gui::ViewProviderDocumentObject*>(pObj);
                this->signalExpandObject(*vp, Gui::Expand);
            }
            xmlReader.readEndElement("ViewProvider");
        }
        xmlReader.readEndElement("ViewProviderData");

        // read camera settings
        xmlReader.readElement("Camera");
        const char* ppReturn = xmlReader.getAttribute("settings");
        std::string sMsg = "SetCamera ";
        sMsg += ppReturn;
        if (strcmp(ppReturn, "") != 0) { // non-empty attribute
            try {
                const char** pReturnIgnore=0;
                std::list<MDIView*> mdi = getMDIViews();
                for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
                    if ((*it)->onHasMsg("SetCamera"))
                        (*it)->onMsg(sMsg.c_str(), pReturnIgnore);
                }
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
            }
        }
    }

    xmlReader.readEndElement("Document");

    // In the file GuiDocument.xml new data files might be added
    if (!xmlReader.getFilenames().empty())
        xmlReader.readFiles(static_cast<zipios::ZipInputStream&>(reader.getStream()));

    // reset modified flag
    setModified(false);
}

void Document::slotStartRestoreDocument(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    // disable this signal while loading a document
    d->connectActObjectBlocker.block();
}

void Document::slotFinishRestoreDocument(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    d->connectActObjectBlocker.unblock();
    App::DocumentObject* act = doc.getActiveObject();
    if (act) {
        ViewProvider* viewProvider = getViewProvider(act);
        if (viewProvider && viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
            signalActivatedObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
        }
    }
    // some post-processing of view providers
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::iterator it;
    for (it = d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it) {
        it->second->finishRestoring();
    }

    // reset modified flag
    setModified(false);
}

/**
 * Saves the properties of the view providers.
 */
void Document::SaveDocFile (Base::Writer &writer) const
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << std::endl
                    << "<!--" << std::endl
                    << " FreeCAD Document, see http://www.freecadweb.org for more information..."
                    << std::endl << "-->" << std::endl;

    writer.Stream() << "<Document SchemaVersion=\"1\">" << std::endl;

    std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator it;

    // writing the view provider names itself
    writer.incInd(); // indentation for 'ViewProviderData Count'
    writer.Stream() << writer.ind() << "<ViewProviderData Count=\"" 
                    << d->_ViewProviderMap.size() <<"\">" << std::endl;

    bool xml = writer.isForceXML();
    //writer.setForceXML(true);
    writer.incInd(); // indentation for 'ViewProvider name'
    for(it = d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it) {
        const App::DocumentObject* doc = it->first;
        ViewProvider* obj = it->second;
        writer.Stream() << writer.ind() << "<ViewProvider name=\""
                        << doc->getNameInDocument() << "\" "
                        << "expanded=\"" << (doc->testStatus(App::Expand) ? 1:0) << "\"";
        if(obj->hasExtensions())
            writer.Stream() << " Extensions=\"True\"";
        
        writer.Stream() << ">" << std::endl;
        obj->Save(writer);
        writer.Stream() << writer.ind() << "</ViewProvider>" << std::endl;
    }
    writer.setForceXML(xml);

    writer.decInd(); // indentation for 'ViewProvider name'
    writer.Stream() << writer.ind() << "</ViewProviderData>" << std::endl;
    writer.decInd();  // indentation for 'ViewProviderData Count'

    // set camera settings
    QString viewPos;
    std::list<MDIView*> mdi = getMDIViews();
    for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
        if ((*it)->onHasMsg("GetCamera")) {
            const char* ppReturn=0;
            (*it)->onMsg("GetCamera",&ppReturn);

            // remove the first line because it's a comment like '#Inventor V2.1 ascii'
            QStringList lines = QString(QString::fromLatin1(ppReturn)).split(QLatin1String("\n"));
            if (lines.size() > 1) {
                lines.pop_front();
                viewPos = lines.join(QLatin1String(" "));
                break;
            }
        }
    }

    writer.incInd(); // indentation for camera settings
    writer.Stream() << writer.ind() << "<Camera settings=\"" 
                    << (const char*)viewPos.toLatin1() <<"\"/>" << std::endl;
    writer.decInd(); // indentation for camera settings
    writer.Stream() << "</Document>" << std::endl;
}

void Document::exportObjects(const std::vector<App::DocumentObject*>& obj, Base::Writer& writer)
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>" << std::endl;
    writer.Stream() << "<Document SchemaVersion=\"1\">" << std::endl;

    std::map<const App::DocumentObject*,ViewProvider*> views;
    for (std::vector<App::DocumentObject*>::const_iterator it = obj.begin(); it != obj.end(); ++it) {
        Document* doc = Application::Instance->getDocument((*it)->getDocument());
        if (doc) {
            ViewProvider* vp = doc->getViewProvider(*it);
            if (vp) views[*it] = vp;
        }
    }

    // writing the view provider names itself
    writer.incInd(); // indentation for 'ViewProviderData Count'
    writer.Stream() << writer.ind() << "<ViewProviderData Count=\"" 
                    << views.size() <<"\">" << std::endl;

    bool xml = writer.isForceXML();
    //writer.setForceXML(true);
    writer.incInd(); // indentation for 'ViewProvider name'
    std::map<const App::DocumentObject*,ViewProvider*>::const_iterator jt;
    for (jt = views.begin(); jt != views.end(); ++jt) {
        const App::DocumentObject* doc = jt->first;
        ViewProvider* obj = jt->second;
        writer.Stream() << writer.ind() << "<ViewProvider name=\""
                        << doc->getNameInDocument() << "\" type=\""
                        << obj->getTypeId().getName()
                        << "\">" << std::endl;
        obj->Save(writer);
        writer.Stream() << writer.ind() << "</ViewProvider>" << std::endl;
    }
    writer.setForceXML(xml);

    writer.decInd(); // indentation for 'ViewProvider name'
    writer.Stream() << writer.ind() << "</ViewProviderData>" << std::endl;
    writer.decInd();  // indentation for 'ViewProviderData Count'
    writer.incInd(); // indentation for camera settings
    writer.Stream() << writer.ind() << "<Camera settings=\"\"/>" << std::endl;
    writer.decInd(); // indentation for camera settings
    writer.Stream() << "</Document>" << std::endl;
}

void Document::importObjects(const std::vector<App::DocumentObject*>& obj, Base::Reader& reader,
                             const std::map<std::string, std::string>& nameMapping)
{
    // We must create an XML parser to read from the input stream
    Base::XMLReader xmlReader("GuiDocument.xml", reader);
    xmlReader.readElement("Document");
    long scheme = xmlReader.getAttributeAsInteger("SchemaVersion");

    // At this stage all the document objects and their associated view providers exist.
    // Now we must restore the properties of the view providers only.
    //
    // SchemeVersion "1"
    if (scheme == 1) {
        // read the viewproviders itself
        xmlReader.readElement("ViewProviderData");
        int Cnt = xmlReader.getAttributeAsInteger("Count");
        std::vector<App::DocumentObject*>::const_iterator it = obj.begin();
        for (int i=0;i<Cnt&&it!=obj.end();++i,++it) {
            // The stored name usually doesn't match with the current name anymore
            // thus we try to match by type. This should work because the order of
            // objects should not have changed
            xmlReader.readElement("ViewProvider");
            std::string name = xmlReader.getAttribute("name");
            std::map<std::string, std::string>::const_iterator jt = nameMapping.find(name);
            if (jt != nameMapping.end())
                name = jt->second;
            Gui::ViewProvider* pObj = this->getViewProviderByName(name.c_str());
            if (pObj)
                pObj->Restore(xmlReader);
            xmlReader.readEndElement("ViewProvider");
            if (it == obj.end())
                break;
        }
        xmlReader.readEndElement("ViewProviderData");
    }

    xmlReader.readEndElement("Document");

    // In the file GuiDocument.xml new data files might be added
    if (!xmlReader.getFilenames().empty())
        xmlReader.readFiles(static_cast<zipios::ZipInputStream&>(reader.getStream()));
}

void Document::addRootObjectsToGroup(const std::vector<App::DocumentObject*>& obj, App::DocumentObjectGroup* grp)
{
    std::map<App::DocumentObject*, bool> rootMap;
    for (std::vector<App::DocumentObject*>::const_iterator it = obj.begin(); it != obj.end(); ++it) {
        rootMap[*it] = true;
    }
    // get the view providers and check which objects are children
    for (std::vector<App::DocumentObject*>::const_iterator it = obj.begin(); it != obj.end(); ++it) {
        Gui::ViewProvider* vp = getViewProvider(*it);
        if (vp) {
            std::vector<App::DocumentObject*> child = vp->claimChildren();
            for (std::vector<App::DocumentObject*>::iterator jt = child.begin(); jt != child.end(); ++jt) {
                std::map<App::DocumentObject*, bool>::iterator kt = rootMap.find(*jt);
                if (kt != rootMap.end()) {
                    kt->second = false;
                }
            }
        }
    }

    // all objects that are not children of other objects can be added to the group
    for (std::map<App::DocumentObject*, bool>::iterator it = rootMap.begin(); it != rootMap.end(); ++it) {
        if (it->second)
            grp->addObject(it->first);
    }
}

void Document::createView(const Base::Type& typeId)
{
    if (!typeId.isDerivedFrom(MDIView::getClassTypeId()))
        return;

    std::list<MDIView*> theViews = this->getMDIViewsOfType(typeId);
    if (typeId == View3DInventor::getClassTypeId()) {
        QtGLWidget* shareWidget = 0;
        // VBO rendering doesn't work correctly when we don't share the OpenGL widgets
        if (!theViews.empty()) {
            View3DInventor* firstView = static_cast<View3DInventor*>(theViews.front());
            shareWidget = qobject_cast<QtGLWidget*>(firstView->getViewer()->getGLWidget());
        }

        View3DInventor* view3D = new View3DInventor(this, getMainWindow(), shareWidget);
        if (!theViews.empty()) {
            View3DInventor* firstView = static_cast<View3DInventor*>(theViews.front());
            std::string overrideMode = firstView->getViewer()->getOverrideMode();
            view3D->getViewer()->setOverrideMode(overrideMode);
        }

        // attach the viewproviders. we need to make sure that we only attach the toplevel ones
        // and not viewproviders which are claimed by other providers. To ensure this we first
        // add all providers and then remove the ones already claimed
        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator It1;
        std::vector<App::DocumentObject*> child_vps;
        for (It1=d->_ViewProviderMap.begin();It1!=d->_ViewProviderMap.end();++It1) {
            view3D->getViewer()->addViewProvider(It1->second);
            std::vector<App::DocumentObject*> children = It1->second->claimChildren3D();
            child_vps.insert(child_vps.end(), children.begin(), children.end());
        }
        std::map<std::string,ViewProvider*>::const_iterator It2;
        for (It2=d->_ViewProviderMapAnnotation.begin();It2!=d->_ViewProviderMapAnnotation.end();++It2) {
            view3D->getViewer()->addViewProvider(It2->second);
            std::vector<App::DocumentObject*> children = It2->second->claimChildren3D();
            child_vps.insert(child_vps.end(), children.begin(), children.end());
        }
        
        for(App::DocumentObject* obj : child_vps) 
            view3D->getViewer()->removeViewProvider(getViewProvider(obj));

        const char* name = getDocument()->Label.getValue();
        QString title = QString::fromLatin1("%1 : %2[*]")
            .arg(QString::fromUtf8(name)).arg(d->_iWinCount++);

        view3D->setWindowTitle(title);
        view3D->setWindowModified(this->isModified());
        view3D->setWindowIcon(QApplication::windowIcon());
        view3D->resize(400, 300);
        getMainWindow()->addWindow(view3D);
    }
}

Gui::MDIView* Document::cloneView(Gui::MDIView* oldview)
{
    if (!oldview)
        return 0;

    if (oldview->getTypeId() == View3DInventor::getClassTypeId()) {
        View3DInventor* view3D = new View3DInventor(this, getMainWindow());

        // attach the viewprovider
        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator It1;
        for (It1=d->_ViewProviderMap.begin();It1!=d->_ViewProviderMap.end();++It1)
            view3D->getViewer()->addViewProvider(It1->second);
        std::map<std::string,ViewProvider*>::const_iterator It2;
        for (It2=d->_ViewProviderMapAnnotation.begin();It2!=d->_ViewProviderMapAnnotation.end();++It2)
            view3D->getViewer()->addViewProvider(It2->second);

        view3D->setWindowTitle(oldview->windowTitle());
        view3D->setWindowModified(oldview->isWindowModified());
        view3D->setWindowIcon(oldview->windowIcon());
        view3D->resize(oldview->size());

        return view3D;
    }

    return 0;
}

void Document::attachView(Gui::BaseView* pcView, bool bPassiv)
{
    if (!bPassiv)
        d->baseViews.push_back(pcView);
    else
        d->passiveViews.push_back(pcView);
}

void Document::detachView(Gui::BaseView* pcView, bool bPassiv)
{
    if (bPassiv) {
        if (find(d->passiveViews.begin(),d->passiveViews.end(),pcView)
            != d->passiveViews.end())
        d->passiveViews.remove(pcView);
    }
    else {
        if (find(d->baseViews.begin(),d->baseViews.end(),pcView)
            != d->baseViews.end())
        d->baseViews.remove(pcView);

        // last view?
        if (d->baseViews.size() == 0) {
            // decouple a passive view
            std::list<Gui::BaseView*>::iterator it = d->passiveViews.begin();
            while (it != d->passiveViews.end()) {
                (*it)->setDocument(0);
                it = d->passiveViews.begin();
            }

            // is already closing the document
            if (d->_isClosing == false)
                d->_pcAppWnd->onLastWindowClosed(this);
        }
    }
}

void Document::onUpdate(void)
{
#ifdef FC_LOGUPDATECHAIN
    Base::Console().Log("Acti: Gui::Document::onUpdate()");
#endif

    std::list<Gui::BaseView*>::iterator it;

    for (it = d->baseViews.begin();it != d->baseViews.end();++it) {
        (*it)->onUpdate();
    }

    for (it = d->passiveViews.begin();it != d->passiveViews.end();++it) {
        (*it)->onUpdate();
    }
}

void Document::onRelabel(void)
{
#ifdef FC_LOGUPDATECHAIN
    Base::Console().Log("Acti: Gui::Document::onRelabel()");
#endif

    std::list<Gui::BaseView*>::iterator it;

    for (it = d->baseViews.begin();it != d->baseViews.end();++it) {
        (*it)->onRelabel(this);
    }

    for (it = d->passiveViews.begin();it != d->passiveViews.end();++it) {
        (*it)->onRelabel(this);
    }
}

bool Document::isLastView(void)
{
    if (d->baseViews.size() <= 1)
        return true;
    return false;
}

/** 
 *  This method checks if the document can be closed. It checks on
 *  the save state of the document and is able to abort the closing.
 */
bool Document::canClose ()
{
    if (d->_isClosing)
        return true;
    if (!getDocument()->isClosable()) {
        QMessageBox::warning(getActiveView(),
            QObject::tr("Document not closable"),
            QObject::tr("The document is not closable for the moment."));
        return false;
    }
    //else if (!Gui::Control().isAllowedAlterDocument()) {
    //    std::string name = Gui::Control().activeDialog()->getDocumentName();
    //    if (name == this->getDocument()->getName()) {
    //        QMessageBox::warning(getActiveView(),
    //            QObject::tr("Document not closable"),
    //            QObject::tr("The document is in editing mode and thus cannot be closed for the moment.\n"
    //                        "You either have to finish or cancel the editing in the task panel."));
    //        Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    //        if (dlg) Gui::Control().showDialog(dlg);
    //        return false;
    //    }
    //}

    bool ok = true;
    if (isModified()) {
        QMessageBox box(getActiveView());
        box.setIcon(QMessageBox::Question);
        box.setWindowTitle(QObject::tr("Unsaved document"));
        box.setText(QObject::tr("Do you want to save your changes to document '%1' before closing?")
                    .arg(QString::fromUtf8(getDocument()->Label.getValue())));
        box.setInformativeText(QObject::tr("If you don't save, your changes will be lost."));
        box.setStandardButtons(QMessageBox::Discard | QMessageBox::Cancel | QMessageBox::Save);
        box.setDefaultButton(QMessageBox::Save);
        box.setEscapeButton(QMessageBox::Cancel);

        // add shortcuts
        QAbstractButton* saveBtn = box.button(QMessageBox::Save);
        if (saveBtn->shortcut().isEmpty()) {
            QString text = saveBtn->text();
            text.prepend(QLatin1Char('&'));
            saveBtn->setShortcut(QKeySequence::mnemonic(text));
        }

        QAbstractButton* discardBtn = box.button(QMessageBox::Discard);
        if (discardBtn->shortcut().isEmpty()) {
            QString text = discardBtn->text();
            text.prepend(QLatin1Char('&'));
            discardBtn->setShortcut(QKeySequence::mnemonic(text));
        }

        switch (box.exec())
        {
        case QMessageBox::Save:
            ok = save();
            break;
        case QMessageBox::Discard:
            ok = true;
            break;
        case QMessageBox::Cancel:
            ok = false;
            break;
        }
    }

    if (ok) {
        // If a task dialog is open that doesn't allow other commands to modify
        // the document it must be closed by resetting the edit mode of the
        // corresponding view provider.
        if (!Gui::Control().isAllowedAlterDocument()) {
            std::string name = Gui::Control().activeDialog()->getDocumentName();
            if (name == this->getDocument()->getName()) {
                if (this->getInEdit())
                    this->resetEdit();
            }
        }
    }

    return ok;
}

std::list<MDIView*> Document::getMDIViews() const
{
    std::list<MDIView*> views;
    for (std::list<BaseView*>::const_iterator it = d->baseViews.begin();
         it != d->baseViews.end(); ++it) {
        MDIView* view = dynamic_cast<MDIView*>(*it);
        if (view)
            views.push_back(view);
    }

    return views;
}

std::list<MDIView*> Document::getMDIViewsOfType(const Base::Type& typeId) const
{
    std::list<MDIView*> views;
    for (std::list<BaseView*>::const_iterator it = d->baseViews.begin();
         it != d->baseViews.end(); ++it) {
        MDIView* view = dynamic_cast<MDIView*>(*it);
        if (view && view->isDerivedFrom(typeId))
            views.push_back(view);
    }

    return views;
}

/// send messages to the active view
bool Document::sendMsgToViews(const char* pMsg)
{
    std::list<Gui::BaseView*>::iterator it;
    const char** pReturnIgnore=0;

    for (it = d->baseViews.begin();it != d->baseViews.end();++it) {
        if ((*it)->onMsg(pMsg,pReturnIgnore)) {
            return true;
        }
    }

    for (it = d->passiveViews.begin();it != d->passiveViews.end();++it) {
        if ((*it)->onMsg(pMsg,pReturnIgnore)) {
            return true;
        }
    }

    return false;
}

bool Document::sendMsgToFirstView(const Base::Type& typeId, const char* pMsg, const char** ppReturn)
{
    // first try the active view
    Gui::MDIView* view = getActiveView();
    if (view && view->isDerivedFrom(typeId)) {
        if (view->onMsg(pMsg, ppReturn))
            return true;
    }

    // now try the other views
    std::list<Gui::MDIView*> views = getMDIViewsOfType(typeId);
    for (std::list<Gui::MDIView*>::iterator it = views.begin(); it != views.end(); ++it) {
        if ((*it != view) && (*it)->onMsg(pMsg, ppReturn)) {
            return true;
        }
    }

    return false;
}

/// Getter for the active view
MDIView* Document::getActiveView(void) const
{
    // get the main window's active view 
    MDIView* active = getMainWindow()->activeWindow();

    // get all MDI views of the document
    std::list<MDIView*> mdis = getMDIViews();

    // check whether the active view is part of this document
    bool ok=false;
    for (std::list<MDIView*>::const_iterator it = mdis.begin(); it != mdis.end(); ++it) {
        if ((*it) == active) {
            ok = true;
            break;
        }
    }

    // the active view is not part of this document, just use the last view
    if (!ok && !mdis.empty())
        active = mdis.back();

    return active;
}

/**
 * @brief Document::setActiveWindow
 * If this document is active and the view is part of it then it will be
 * activated. If the document is not active of the view is already active
 * nothing is done.
 * @param view
 */
void Document::setActiveWindow(Gui::MDIView* view)
{
    // get the main window's active view
    MDIView* active = getMainWindow()->activeWindow();

    // view is already active
    if (active == view)
        return;

    // get all MDI views of the document
    std::list<MDIView*> mdis = getMDIViews();

    // this document is not active
    if (std::find(mdis.begin(), mdis.end(), active) == mdis.end())
        return;

    // the view is not part of the document
    if (std::find(mdis.begin(), mdis.end(), view) == mdis.end())
        return;

    getMainWindow()->setActiveWindow(view);
}

Gui::MDIView* Document::getViewOfNode(SoNode* node) const
{
    std::list<MDIView*> mdis = getMDIViewsOfType(View3DInventor::getClassTypeId());
    for (std::list<MDIView*>::const_iterator it = mdis.begin(); it != mdis.end(); ++it) {
        View3DInventor* view = static_cast<View3DInventor*>(*it);
        if (view->getViewer()->searchNode(node))
            return *it;
    }

    return 0;
}

Gui::MDIView* Document::getViewOfViewProvider(Gui::ViewProvider* vp) const
{
    return getViewOfNode(vp->getRoot());
}

Gui::MDIView* Document::getEditingViewOfViewProvider(Gui::ViewProvider* vp) const
{
    std::list<MDIView*> mdis = getMDIViewsOfType(View3DInventor::getClassTypeId());
    for (std::list<MDIView*>::const_iterator it = mdis.begin(); it != mdis.end(); ++it) {
        View3DInventor* view = static_cast<View3DInventor*>(*it);
        View3DInventorViewer* viewer = view->getViewer();
        // there is only one 3d view which is in edit mode
        if (viewer->hasViewProvider(vp) && viewer->isEditingViewProvider())
            return *it;
    }

    return 0;
}

//--------------------------------------------------------------------------
// UNDO REDO transaction handling  
//--------------------------------------------------------------------------
/** Open a new Undo transaction on the active document
 *  This method opens a new UNDO transaction on the active document. This transaction
 *  will later appear in the UNDO/REDO dialog with the name of the command. If the user 
 *  recall the transaction everything changed on the document between OpenCommand() and 
 *  CommitCommand will be undone (or redone). You can use an alternative name for the 
 *  operation default is the command name.
 *  @see CommitCommand(),AbortCommand()
 */
void Document::openCommand(const char* sName)
{
    getDocument()->openTransaction(sName);
}

void Document::commitCommand(void)
{
    getDocument()->commitTransaction();
}

void Document::abortCommand(void)
{
    getDocument()->abortTransaction();
}

bool Document::hasPendingCommand(void) const
{
    return getDocument()->hasPendingTransaction();
}

/// Get a string vector with the 'Undo' actions
std::vector<std::string> Document::getUndoVector(void) const
{
    return getDocument()->getAvailableUndoNames();
}

/// Get a string vector with the 'Redo' actions
std::vector<std::string> Document::getRedoVector(void) const
{
    return getDocument()->getAvailableRedoNames();
}

/// Will UNDO one or more steps
void Document::undo(int iSteps)
{
    for (int i=0;i<iSteps;i++) {
        getDocument()->undo();
    }
}

/// Will REDO one or more steps
void Document::redo(int iSteps)
{
    for (int i=0;i<iSteps;i++) {
        getDocument()->redo();
    }
}

PyObject* Document::getPyObject(void)
{
    _pcDocPy->IncRef();
    return _pcDocPy;
}

void Document::handleChildren3D(ViewProvider* viewProvider)
{
    // check for children
    bool rebuild = false;
    if (viewProvider && viewProvider->getChildRoot()) {
        std::vector<App::DocumentObject*> children = viewProvider->claimChildren3D();
        SoGroup* childGroup =  viewProvider->getChildRoot();

        // size not the same -> build up the list new
        if (childGroup->getNumChildren() != static_cast<int>(children.size())) {

            rebuild = true;
            childGroup->removeAllChildren();

            for (std::vector<App::DocumentObject*>::iterator it=children.begin();it!=children.end();++it) {
                ViewProvider* ChildViewProvider = getViewProvider(*it);
                if (ChildViewProvider) {
                    SoSeparator* childRootNode =  ChildViewProvider->getRoot();
                    childGroup->addChild(childRootNode);

                    // cycling to all views of the document to remove the viewprovider from the viewer itself
                    for (std::list<Gui::BaseView*>::iterator vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
                        View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
                        if (activeView && activeView->getViewer()->hasViewProvider(ChildViewProvider)) {

                            // @Note hasViewProvider()
                            // remove the viewprovider serves the purpose of detaching the inventor nodes from the
                            // top level root in the viewer. However, if some of the children were grouped beneath the object
                            // earlier they are not anymore part of the toplevel inventor node. we need to check for that.
                            if (d->_editViewProvider == ChildViewProvider)
                                resetEdit();
                            activeView->getViewer()->removeViewProvider(ChildViewProvider);
                        }
                    }
                }
            }
        }
    } 
    
    //find all unclaimed viewproviders and add them back to the document (this happens if a 
    //viewprovider has been claimed before, but the object dropped it. 
    if (rebuild) {
        rebuildRootNodes();
    }
}

void Document::rebuildRootNodes()
{
    auto vpmap = d->_ViewProviderMap;
    for (auto& pair  : d->_ViewProviderMap) {
        auto claimed = pair.second->claimChildren3D();
        for (auto obj : claimed) {
            auto it = vpmap.find(obj);
            if (it != vpmap.end())
                vpmap.erase(it);
        }
    }

    for (auto& pair : vpmap) {
        // cycling to all views of the document to add the viewprovider to the viewer itself
        for (std::list<Gui::BaseView*>::iterator vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
            if (activeView && !activeView->getViewer()->hasViewProvider(pair.second)) {
                activeView->getViewer()->addViewProvider(pair.second);
            }
        }
    }
}
