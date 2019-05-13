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
#include <App/GeoFeatureGroupExtension.h>

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
#include "ViewProviderLink.h"

FC_LOG_LEVEL_INIT("Gui::Document",true,true)

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
    bool       _isTransacting;
    int                         _editMode;
    ViewProvider*               _editViewProvider;
    ViewProviderDocumentObject* _editViewProviderParent;
    std::string                 _editSubname;
    std::string                 _editSubElement;
    Base::Matrix4D              _editingTransform;
    Application*    _pcAppWnd;
    // the doc/Document
    App::Document*  _pcDocument;
    /// List of all registered views
    std::list<Gui::BaseView*> baseViews;
    /// List of all registered views
    std::list<Gui::BaseView*> passiveViews;
    std::map<const App::DocumentObject*,ViewProviderDocumentObject*> _ViewProviderMap;
    std::map<SoSeparator *,ViewProviderDocumentObject*> _CoinMap;
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
    Connection connectShowHidden;
    Connection connectFinishRestoreObject;
    Connection connectExportObjects;
    Connection connectImportObjects;
    Connection connectFinishImportObjects;
    Connection connectUndoDocument;
    Connection connectRedoDocument;
    Connection connectRecomputed;
    Connection connectSkipRecompute;
    Connection connectTransactionAppend;
    Connection connectTransactionRemove;
    Connection connectTouchedObject;
    Connection connectChangePropertyEditor;

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
    d->_isTransacting = false;
    d->_pcAppWnd = app;
    d->_pcDocument = pcDocument;
    d->_editViewProvider = 0;
    d->_editViewProviderParent = 0;
    d->_editMode = 0;

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
    d->connectShowHidden = App::GetApplication().signalShowHidden.connect
        (boost::bind(&Gui::Document::slotShowHidden, this, _1));

    d->connectChangePropertyEditor = pcDocument->signalChangePropertyEditor.connect
        (boost::bind(&Gui::Document::slotChangePropertyEditor, this, _1, _2));
    d->connectFinishRestoreObject = pcDocument->signalFinishRestoreObject.connect
        (boost::bind(&Gui::Document::slotFinishRestoreObject, this, _1));
    d->connectExportObjects = pcDocument->signalExportViewObjects.connect
        (boost::bind(&Gui::Document::exportObjects, this, _1, _2));
    d->connectImportObjects = pcDocument->signalImportViewObjects.connect
        (boost::bind(&Gui::Document::importObjects, this, _1, _2, _3));
    d->connectFinishImportObjects = pcDocument->signalFinishImportObjects.connect
        (boost::bind(&Gui::Document::slotFinishImportObjects, this, _1));
        
    d->connectUndoDocument = pcDocument->signalUndo.connect
        (boost::bind(&Gui::Document::slotUndoDocument, this, _1));
    d->connectRedoDocument = pcDocument->signalRedo.connect
        (boost::bind(&Gui::Document::slotRedoDocument, this, _1));
    d->connectRecomputed = pcDocument->signalRecomputed.connect
        (boost::bind(&Gui::Document::slotRecomputed, this, _1));
    d->connectSkipRecompute = pcDocument->signalSkipRecompute.connect
        (boost::bind(&Gui::Document::slotSkipRecompute, this, _1, _2));
    d->connectTouchedObject = pcDocument->signalTouchedObject.connect
        (boost::bind(&Gui::Document::slotTouchedObject, this, _1));

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
    d->connectShowHidden.disconnect();
    d->connectFinishRestoreObject.disconnect();
    d->connectExportObjects.disconnect();
    d->connectImportObjects.disconnect();
    d->connectFinishImportObjects.disconnect();
    d->connectUndoDocument.disconnect();
    d->connectRedoDocument.disconnect();
    d->connectRecomputed.disconnect();
    d->connectSkipRecompute.disconnect();
    d->connectTransactionAppend.disconnect();
    d->connectTransactionRemove.disconnect();
    d->connectTouchedObject.disconnect();
    d->connectChangePropertyEditor.disconnect();

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

bool Document::setEdit(Gui::ViewProvider* p, int ModNum, const char *subname)
{
    ViewProviderDocumentObject* vp = dynamic_cast<ViewProviderDocumentObject*>(p);
    if (!vp) {
        FC_ERR("cannot edit non ViewProviderDocumentObject");
        return false;
    }
    auto obj = vp->getObject();
    if(!obj->getNameInDocument()) {
        FC_ERR("cannot edit detached object");
        return false;
    }

    std::string _subname;
    if(!subname || !subname[0]) {
        // No subname reference is given, we try to extract one from the current
        // selection in order to obtain the correct transformation matrix below
        auto sels = Gui::Selection().getCompleteSelection(false);
        App::DocumentObject *parentObj = 0;
        for(auto &sel : sels) {
            if(!sel.pObject || !sel.pObject->getNameInDocument())
                continue;
            if(!parentObj)
                parentObj = sel.pObject;
            else if(parentObj!=sel.pObject) {
                FC_LOG("Cannot deduce subname for editing, more than one parent?");
                parentObj = 0;
                break;
            }
            auto sobj = parentObj->getSubObject(sel.SubName);
            if(!sobj || (sobj!=obj && sobj->getLinkedObject(true)!= obj)) {
                FC_LOG("Cannot deduce subname for editing, subname mismatch");
                parentObj = 0;
                break;
            }
            _subname = sel.SubName;
        }
        if(parentObj) {
            FC_LOG("deduced editing reference " << parentObj->getFullName() << '.' << _subname);
            subname = _subname.c_str();
            obj = parentObj;
            vp = dynamic_cast<ViewProviderDocumentObject*>(
                    Application::Instance->getViewProvider(obj));
            if(!vp || !vp->getDocument()) {
                FC_ERR("invliad view provider for parent object");
                return false;
            }
            if(vp->getDocument()!=this)
                return vp->getDocument()->setEdit(vp,ModNum,subname);
        }
    }

    if (d->_ViewProviderMap.find(obj) == d->_ViewProviderMap.end()) {
        // We can actually support editing external object, by calling
        // View3DInventViewer::setupEditingRoot() before exiting from
        // ViewProvider::setEditViewer(), which transfer all child node of the view
        // provider into an editing node inside the viewer of this document. And
        // that's may actually be the case, as the subname referenced sub object
        // is allowed to be in other documents. 
        //
        // We just disabling editing external parent object here, for bug
        // tracking purpose. Because, bringing an unrelated external object to
        // the current view for editing will confuse user, and is certainly a
        // bug. By right, the top parent object should always belong to the
        // editing document, and the acutally editing sub object can be
        // external.
        //
        // So, you can either call setEdit() with subname set to 0, which cause
        // the code above to auto detect selection context, and dispatch the
        // editing call to the correct document. Or, supply subname yourself,
        // and make sure you get the document right.
        //
        FC_ERR("cannot edit object '" << obj->getNameInDocument() << "': not found in document "
                << "'" << getDocument()->getName() << "'");
        return false;
    }

    d->_editingTransform = Base::Matrix4D();
    // Geo feature group now handles subname like link group. So no need of the
    // following code.
    //
    // if(!subname || !subname[0]) {
    //     auto group = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
    //     if(group) {
    //         auto ext = group->getExtensionByType<App::GeoFeatureGroupExtension>();
    //         d->_editingTransform = ext->globalGroupPlacement().toMatrix();
    //     }
    // }
    auto sobj = obj->getSubObject(subname,0,&d->_editingTransform);
    if(!sobj || !sobj->getNameInDocument()) {
        FC_ERR("Invalid sub object '" << obj->getFullName() 
                << '.' << (subname?subname:"") << "'");
        return false;
    }
    auto svp = vp;
    if(sobj!=obj) {
        svp = dynamic_cast<ViewProviderDocumentObject*>(
                Application::Instance->getViewProvider(sobj));
        if(!svp) {
            FC_ERR("Cannot edit '" << sobj->getFullName() << "' without view provider");
            return false;
        }
    }

    View3DInventor *activeView = dynamic_cast<View3DInventor *>(getActiveView());
    // if the currently active view is not the 3d view search for it and activate it
    if (!activeView) {
        activeView = dynamic_cast<View3DInventor *>(getViewOfViewProvider(vp));
        if(!activeView){
            FC_ERR("cannot edit without active view");
            return false;
        }
    }
    getMainWindow()->setActiveWindow(activeView);
    Application::Instance->setEditDocument(this);

    d->_editViewProviderParent = vp;
    d->_editSubElement.clear();
    d->_editSubname.clear();
    if(subname) {
        const char *element = Data::ComplexGeoData::findElementName(subname);
        if(element) {
            d->_editSubname = std::string(subname,element-subname);
            d->_editSubElement = element;
        }else
            d->_editSubname = subname;
    }

    d->_editMode = ModNum;
    d->_editViewProvider = svp->startEditing(ModNum);
    if(!d->_editViewProvider) {
        d->_editViewProviderParent = 0;
        FC_LOG("object '" << sobj->getFullName() << "' refuse to edit");
        return false;
    }
    activeView->getViewer()->setEditingViewProvider(d->_editViewProvider,ModNum);
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg)
        dlg->setDocumentName(this->getDocument()->getName());
    if (d->_editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) 
        signalInEdit(*(static_cast<ViewProviderDocumentObject*>(d->_editViewProvider)));
    return true;
}

const Base::Matrix4D &Document::getEditingTransform() const {
    return d->_editingTransform;
}

void Document::setEditingTransform(const Base::Matrix4D &mat) {
    d->_editingTransform = mat;
    View3DInventor *activeView = dynamic_cast<View3DInventor *>(getActiveView());
    if (activeView) 
        activeView->getViewer()->setEditingTransform(mat);
}

void Document::resetEdit(void) {
    Application::Instance->setEditDocument(0);
}

void Document::_resetEdit(void)
{
    std::list<Gui::BaseView*>::iterator it;
    if (d->_editViewProvider) {
        for (it = d->baseViews.begin();it != d->baseViews.end();++it) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*it);
            if (activeView)
                activeView->getViewer()->resetEditingViewProvider();
        }

        d->_editViewProvider->finishEditing();
        if (d->_editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) 
            signalResetEdit(*(static_cast<ViewProviderDocumentObject*>(d->_editViewProvider)));
        d->_editViewProvider = 0;

        // The logic below is not necessary anymore, because this method is
        // changed into a private one,  _resetEdit(). And the exposed
        // resetEdit() above calls into Application->setEditDocument(0) which
        // will prevent recrusive calling.
#if 0
        // Nullify the member variable before calling finishEditing().
        // This is to avoid a possible stack overflow when a view provider wrongly
        // invokes the document's resetEdit() method.
        ViewProvider* editViewProvider = d->_editViewProvider;
        d->_editViewProvider = nullptr;

        editViewProvider->finishEditing();
        if (editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
            signalResetEdit(*(static_cast<ViewProviderDocumentObject*>(editViewProvider)));
#endif
    }
    d->_editViewProviderParent = 0;
    if(Application::Instance->editDocument() == this)
        Application::Instance->setEditDocument(0);
}

ViewProvider *Document::getInEdit(ViewProviderDocumentObject **parentVp, 
        std::string *subname, int *mode, std::string *subelement) const
{
    if(parentVp) *parentVp = d->_editViewProviderParent;
    if(subname) *subname = d->_editSubname;
    if(subelement) *subelement = d->_editSubElement;
    if(mode) *mode = d->_editMode;

    if (d->_editViewProvider) {
        // there is only one 3d view which is in edit mode
        View3DInventor *activeView = dynamic_cast<View3DInventor *>(getActiveView());
        if (activeView && activeView->getViewer()->isEditingViewProvider())
            return d->_editViewProvider;
    }

    return 0;
}

void Document::setInEdit(ViewProviderDocumentObject *parentVp, const char *subname) {
    if (d->_editViewProvider) {
        d->_editViewProviderParent = parentVp;
        d->_editSubname = subname?subname:"";
    }
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
        std::string cName = Obj.getViewProviderNameStored();
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
            d->_CoinMap[pcProvider->getRoot()] = pcProvider;

            try {
                // if successfully created set the right name and calculate the view
                //FIXME: Consider to change argument of attach() to const pointer
                pcProvider->attach(const_cast<App::DocumentObject*>(&Obj));
                pcProvider->updateView();
                pcProvider->setActiveMode();
            }
            catch(const Base::MemoryException& e){
                FC_ERR("Memory exception in " << Obj.getFullName() << " thrown: " << e.what());
            }
            catch(Base::Exception &e){
                e.ReportException();
            }
#ifndef FC_DEBUG
            catch(...){
                FC_ERR("Unknown exception in Feature " << Obj.getFullName() << " thrown");
            }
#endif
        }
        else {
            FC_WARN("no view provider for the object " << cName << " found");
        }
    }else{
        try {
            pcProvider->reattach(const_cast<App::DocumentObject*>(&Obj));
        } catch(Base::Exception &e){
            e.ReportException();
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
    if(!viewProvider) return;

    if (d->_editViewProvider==viewProvider || d->_editViewProviderParent==viewProvider)
        _resetEdit();
    else if(Application::Instance->editDocument()) {
        auto editDoc = Application::Instance->editDocument();
        if(editDoc->d->_editViewProvider==viewProvider ||
           editDoc->d->_editViewProviderParent==viewProvider)
            Application::Instance->setEditDocument(0);
    }

    handleChildren3D(viewProvider,true);

#if 0 // With this we can show child objects again if this method was called by undo
    viewProvider->onDelete(std::vector<std::string>());
#endif
    if (viewProvider && viewProvider->getTypeId().isDerivedFrom
        (ViewProviderDocumentObject::getClassTypeId())) {
        // go through the views
        for (vIt = d->baseViews.begin();vIt != d->baseViews.end();++vIt) {
            View3DInventor *activeView = dynamic_cast<View3DInventor *>(*vIt);
            if (activeView)
                activeView->getViewer()->removeViewProvider(viewProvider);
        }

        // removing from tree
        signalDeletedObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
    }

    viewProvider->beforeDelete();
}

void Document::beforeDelete() {
    auto editDoc = Application::Instance->editDocument();
    if(editDoc) {
        auto vp = dynamic_cast<ViewProviderDocumentObject*>(editDoc->d->_editViewProvider);
        auto vpp = dynamic_cast<ViewProviderDocumentObject*>(editDoc->d->_editViewProviderParent);
        if(editDoc == this || 
           (vp && vp->getDocument()==this) ||
           (vpp && vpp->getDocument()==this))
        {
            Application::Instance->setEditDocument(0);
        }
    }
    for(auto &v : d->_ViewProviderMap)
        v.second->beforeDelete();
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
            FC_ERR("Memory exception in " << Obj.getFullName() << " thrown: " << e.what());
        }
        catch(Base::Exception& e){
            e.ReportException();
        }
        catch(const std::exception& e){
            FC_ERR("C++ exception in " << Obj.getFullName() << " thrown " << e.what());
        }
        catch (...) {
            FC_ERR("Cannot update representation for " << Obj.getFullName());
        }

        handleChildren3D(viewProvider);

        if (viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
            signalChangedObject(static_cast<ViewProviderDocumentObject&>(*viewProvider), Prop);
    }

    // a property of an object has changed
    if(!Prop.testStatus(App::Property::NoModify))
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

        auto itC = d->_CoinMap.find(viewProvider->getRoot());
        if(itC != d->_CoinMap.end())
            d->_CoinMap.erase(itC);

        d->_ViewProviderMap.erase(&obj);
        // transaction being a nullptr indicates that undo/redo is off and the object
        // can be safely deleted
        if (transaction)
            transaction->addObjectNew(viewProvider);
        else
            delete viewProvider;
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
    getMainWindow()->updateActions();
}

void Document::slotRedoDocument(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    
    signalRedoDocument(*this);   
    getMainWindow()->updateActions();
}

void Document::slotRecomputed(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;
    getMainWindow()->updateActions();
    TreeWidget::updateStatus();
}

// This function is called when some asks to recompute a document that is marked
// as 'SkipRecompute'. We'll check if we are the current document, and if either
// not given an explicit recomputing object list, or the given single object is
// the eidting object or the active object. If the conditions are met, we'll
// force recompute only that object and all its dependent objects.
void Document::slotSkipRecompute(const App::Document& doc, const std::vector<App::DocumentObject*> &objs)
{
    if (d->_pcDocument != &doc)
        return;
    if(objs.size()>1 || 
       App::GetApplication().getActiveDocument()!=&doc || 
       !doc.testStatus(App::Document::AllowPartialRecompute))
        return;
    App::DocumentObject *obj = 0;
    auto editDoc = Application::Instance->editDocument();
    if(editDoc) {
        auto vp = dynamic_cast<ViewProviderDocumentObject*>(editDoc->getInEdit());
        if(vp)
            obj = vp->getObject();
    }
    if(!obj)
        obj = doc.getActiveObject();
    if(!obj || !obj->getNameInDocument() || (objs.size() && objs.front()!=obj))
        return;
    obj->recomputeFeature(true);
}

void Document::slotTouchedObject(const App::DocumentObject &)
{
    getMainWindow()->updateActions(true);
    TreeWidget::updateStatus(true);
    setModified(true);
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
    d->_CoinMap[vp->getRoot()] = vp;
}

void Document::setModified(bool b)
{
    if(d->_isModified == b)
        return;
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


ViewProviderDocumentObject* Document::getViewProviderByPathFromTail(SoPath * path) const
{
    // Get the lowest root node in the pick path!
    for (int i = 0; i < path->getLength(); i++) {
        SoNode *node = path->getNodeFromTail(i);
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            auto it = d->_CoinMap.find(static_cast<SoSeparator*>(node));
            if(it!=d->_CoinMap.end())
                return it->second;
        }
    }

    return 0;
}

ViewProviderDocumentObject* Document::getViewProviderByPathFromHead(SoPath * path) const
{
    for (int i = 0; i < path->getLength(); i++) {
        SoNode *node = path->getNode(i);
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            auto it = d->_CoinMap.find(static_cast<SoSeparator*>(node));
            if(it!=d->_CoinMap.end())
                return it->second;
        }
    }

    return 0;
}

ViewProviderDocumentObject *Document::getViewProvider(SoNode *node) const {
    if(!node || !node->isOfType(SoSeparator::getClassTypeId()))
        return 0;
    auto it = d->_CoinMap.find(static_cast<SoSeparator*>(node));
    if(it!=d->_CoinMap.end())
        return it->second;
    return 0;
}

std::vector<std::pair<ViewProviderDocumentObject*,int> > Document::getViewProvidersByPath(SoPath * path) const
{
    std::vector<std::pair<ViewProviderDocumentObject*,int> > ret;
    for (int i = 0; i < path->getLength(); i++) {
        SoNode *node = path->getNodeFromTail(i);
        if (node->isOfType(SoSeparator::getClassTypeId())) {
            auto it = d->_CoinMap.find(static_cast<SoSeparator*>(node));
            if(it!=d->_CoinMap.end())
                ret.push_back(std::make_pair(it->second,i));
        }
    }
    return ret;
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
            std::vector<std::pair<App::Document*,bool> > docs;
            try {
                for(auto doc : getDocument()->getDependentDocuments()) {
                    auto gdoc = Application::Instance->getDocument(doc);
                    if(gdoc && (gdoc==this || gdoc->isModified()))
                        docs.emplace_back(doc,doc->mustExecute());
                }
            }catch(const Base::RuntimeError &e) {
                FC_ERR(e.what());
                docs.emplace_back(getDocument(),getDocument()->mustExecute());
            }
            if(docs.size()>1) {
                int ret = QMessageBox::question(getMainWindow(),
                        QObject::tr("Save dependent files"),
                        QObject::tr("The file contain external depencencies. "
                        "Do you want to save the dependent files, too?"),
                        QMessageBox::Yes,QMessageBox::No);
                if (ret != QMessageBox::Yes) {
                    docs.clear();
                    docs.emplace_back(getDocument(),getDocument()->mustExecute());
                }
            }
            Gui::WaitCursor wc;
            // save all documents
            for(auto v : docs) {
                auto doc = v.first;
                // Changed 'mustExecute' status may be triggered by saving external document
                if(!v.second && doc->mustExecute()) {
                    App::AutoTransaction trans("Recompute");
                    Command::doCommand(Command::Doc,"App.getDocument(\"%s\").recompute()",doc->getName());
                }
                Command::doCommand(Command::Doc,"App.getDocument(\"%s\").save()",doc->getName());
                auto gdoc = Application::Instance->getDocument(doc);
                if(gdoc) gdoc->setModified(false);
            }
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
        QString::fromUtf8(getDocument()->FileName.getValue()), 
        QString::fromLatin1("%1 %2 (*.FCStd)").arg(exe).arg(QObject::tr("Document")));
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

void Document::saveAll() {
    std::vector<App::Document*> docs;
    try {
        docs = App::Document::getDependentDocuments(App::GetApplication().getDocuments(),true);
    }catch(Base::Exception &e) {
        e.ReportException();
        int ret = QMessageBox::critical(getMainWindow(), QObject::tr("Failed to save document"),
                QObject::tr("Documents contains cyclic dependices. Do you still want to save them?"),
                QMessageBox::Yes,QMessageBox::No);
        if(ret!=QMessageBox::Yes)
            return;
        docs = App::GetApplication().getDocuments();
    }
    std::map<App::Document *, bool> dmap;
    for(auto doc : docs)
        dmap[doc] = doc->mustExecute();
    for(auto doc : docs) {
        if(doc->testStatus(App::Document::PartialDoc))
            continue;
        auto gdoc = Application::Instance->getDocument(doc);
        if(!gdoc)
            continue;
        if(!doc->isSaved()) {
            if(!gdoc->saveAs())
                break;
        }
        Gui::WaitCursor wc;

        try {
            // Changed 'mustExecute' status may be triggered by saving external document
            if(!dmap[doc] && doc->mustExecute()) {
                App::AutoTransaction trans("Recompute");
                Command::doCommand(Command::Doc,"App.getDocument('%s').recompute()",doc->getName());
            }
            Command::doCommand(Command::Doc,"App.getDocument('%s').save()",doc->getName());
            gdoc->setModified(false);
        } catch (const Base::Exception& e) {
            QMessageBox::critical(getMainWindow(), 
                    QObject::tr("Failed to save document") + 
                        QString::fromLatin1(": %1").arg(QString::fromUtf8(doc->getName())), 
                    QString::fromLatin1(e.what()));
            break;
        }
    }
}

/// Save a copy of the document under a new file name
bool Document::saveCopy(void)
{
    getMainWindow()->showMessage(QObject::tr("Save a copy of the document under new filename..."));

    QString exe = qApp->applicationName();
    QString fn = FileDialog::getSaveFileName(getMainWindow(), QObject::tr("Save %1 Document").arg(exe), 
                                             QString::fromUtf8(getDocument()->FileName.getValue()), 
                                             QObject::tr("%1 document (*.FCStd)").arg(exe));
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
        it->second->setStatus(Gui::isRestoring,true);
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
        int Cnt = xmlReader.getAttributeAsInteger("Count");
        for (int i=0; i<Cnt; i++) {
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
                this->signalExpandObject(*vp, Gui::ExpandItem,0,0);
            }
            xmlReader.readEndElement("ViewProvider");
        }
        xmlReader.readEndElement("ViewProviderData");

        // read camera settings
        xmlReader.readElement("Camera");
        const char* ppReturn = xmlReader.getAttribute("settings");
        cameraSettings.clear();
        if(ppReturn && ppReturn[0]) {
            saveCameraSettings(ppReturn);
            try {
                const char** pReturnIgnore=0;
                std::list<MDIView*> mdi = getMDIViews();
                for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
                    if ((*it)->onHasMsg("SetCamera"))
                        (*it)->onMsg(cameraSettings.c_str(), pReturnIgnore);
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

void Document::slotFinishRestoreObject(const App::DocumentObject &obj) {
    auto vpd = dynamic_cast<ViewProviderDocumentObject*>(getViewProvider(&obj));
    if(vpd) {
        vpd->setStatus(Gui::isRestoring,false);
        vpd->finishRestoring();
        if(!vpd->canAddToSceneGraph())
            toggleInSceneGraph(vpd);
    }
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

    // reset modified flag
    setModified(false);
}

void Document::slotShowHidden(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;

    Application::Instance->signalShowHidden(*this);
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
        if (obj->hasExtensions())
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
                        << doc->getExportName() << "\" "
                        << "expanded=\"" << (doc->testStatus(App::Expand) ? 1:0) << "\"";
        if (obj->hasExtensions())
            writer.Stream() << " Extensions=\"True\"";

        writer.Stream() << ">" << std::endl;
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
            bool expanded = false;
            if (xmlReader.hasAttribute("expanded")) {
                const char* attr = xmlReader.getAttribute("expanded");
                if (strcmp(attr,"1") == 0) {
                    expanded = true;
                }
            }
            Gui::ViewProvider* pObj = this->getViewProviderByName(name.c_str());
            if (pObj) {
                pObj->setStatus(Gui::isRestoring,true);
                auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(pObj);
                if(vpd) vpd->startRestoring();
                pObj->Restore(xmlReader);
                if (expanded && vpd) 
                    this->signalExpandObject(*vpd, Gui::ExpandItem,0,0);
            }
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

void Document::slotFinishImportObjects(const std::vector<App::DocumentObject*> &objs) {
    (void)objs;
    // finishRestoring() is now trigged by signalFinishRestoreObject
    //
    // for(auto obj : objs) {
    //     auto vp = getViewProvider(obj);
    //     if(!vp) continue;
    //     vp->setStatus(Gui::isRestoring,false);
    //     auto vpd = dynamic_cast<ViewProviderDocumentObject*>(vp);
    //     if(vpd) vpd->finishRestoring();
    // }
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

MDIView *Document::createView(const Base::Type& typeId)
{
    if (!typeId.isDerivedFrom(MDIView::getClassTypeId()))
        return 0;

    std::list<MDIView*> theViews = this->getMDIViewsOfType(typeId);
    if (typeId == View3DInventor::getClassTypeId()) {

        QtGLWidget* shareWidget = 0;
        // VBO rendering doesn't work correctly when we don't share the OpenGL widgets
        if (!theViews.empty()) {
            View3DInventor* firstView = static_cast<View3DInventor*>(theViews.front());
            shareWidget = qobject_cast<QtGLWidget*>(firstView->getViewer()->getGLWidget());

            const char *ppReturn = 0;
            firstView->onMsg("GetCamera",&ppReturn);
            saveCameraSettings(ppReturn);
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

        if(cameraSettings.size()) {
            const char *ppReturn = 0;
            view3D->onMsg(cameraSettings.c_str(),&ppReturn);
        }
        getMainWindow()->addWindow(view3D);
        return view3D;
    }
    return 0;
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

const std::string &Document::getCameraSettings() const {
    return cameraSettings;
}

void Document::saveCameraSettings(const char *settings) {
    if(settings && settings[0])
        cameraSettings = std::string("SetCamera ") + settings;
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

            // is already closing the document, and is not linked by other documents
            if (d->_isClosing == false &&
                App::PropertyXLink::getDocumentInList(getDocument()).empty())
            {
                d->_pcAppWnd->onLastWindowClosed(this);
            }
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
bool Document::canClose (bool checkModify, bool checkLink)
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

    if (checkLink && App::PropertyXLink::getDocumentInList(getDocument()).size())
        return true;

    bool ok = true;
    if (checkModify && isModified() && !getDocument()->testStatus(App::Document::PartialDoc)) {
        int res = getMainWindow()->confirmSave(getDocument()->Label.getValue(),getActiveView());
        if(res>0)
            ok = save();
        else
            ok = res<0;
    }

    if (ok) {
        // If a task dialog is open that doesn't allow other commands to modify
        // the document it must be closed by resetting the edit mode of the
        // corresponding view provider.
        if (!Gui::Control().isAllowedAlterDocument()) {
            std::string name = Gui::Control().activeDialog()->getDocumentName();
            if (name == this->getDocument()->getName()) {
                if (this->getInEdit())
                    this->_resetEdit();
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

    if (ok) 
        return active;

    // the active view is not part of this document, just use the last view
    const auto &windows = Gui::getMainWindow()->windows();
    for(auto rit=mdis.rbegin();rit!=mdis.rend();++rit) {
        // Some view is removed from window list for some reason, e.g. TechDraw
        // hidden page has view but not in the list. By right, the view will
        // self delete, but not the case for TechDraw, especially during
        // document restore.
        if(windows.contains(*rit) || (*rit)->isDerivedFrom(View3DInventor::getClassTypeId()))
            return *rit;
    }
    return 0;
}

MDIView *Document::setActiveView(ViewProviderDocumentObject *vp, Base::Type typeId) {
    MDIView *view = 0;
    if(!vp)
        view = getActiveView();
    else{
        view = vp->getMDIView();
        if(!view) {
            auto obj = vp->getObject();
            if(!obj)
                view = getActiveView();
            else {
                auto linked = obj->getLinkedObject(true);
                if(linked!=obj) {
                    auto vpLinked = dynamic_cast<ViewProviderDocumentObject*>(
                                Application::Instance->getViewProvider(linked));
                    if(vpLinked)
                        view = vpLinked->getMDIView();
                }
                if(!view && typeId.isBad())
                    typeId = View3DInventor::getClassTypeId();
            }
        }
    }
    if(!view || (!typeId.isBad() && !view->isDerivedFrom(typeId))) {
        view = 0;
        for (auto *v : d->baseViews) {
            if(v->isDerivedFrom(MDIView::getClassTypeId()) && 
               (typeId.isBad() || v->isDerivedFrom(typeId))) 
            {
                view = static_cast<MDIView*>(v);
                break;
            }
        }
    }
    if(!view && !typeId.isBad()) 
        view = createView(typeId);
    if(view)
        getMainWindow()->setActiveWindow(view);
    return view;
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

bool Document::checkTransactionID(bool undo, int iSteps) {
    std::vector<int> ids;
    for (int i=0;i<iSteps;i++) {
        int id = getDocument()->getTransactionID(undo,i);
        if(!id) break;
        ids.push_back(id);
    }
    std::set<App::Document*> prompts;
    std::map<App::Document*,int> dmap;
    for(auto doc : App::GetApplication().getDocuments()) {
        if(doc == getDocument())
            continue;
        for(auto id : ids) {
            int steps = undo?doc->getAvailableUndos(id):doc->getAvailableRedos(id);
            if(!steps) continue;
            int &currentSteps = dmap[doc];
            if(currentSteps+1 != steps)
                prompts.insert(doc);
            if(currentSteps < steps)
                currentSteps = steps;
        }
    }
    if(prompts.size()) {
        std::ostringstream str;
        for(auto doc : prompts)
            str << "    " << doc->getName() << "\n";
        int ret = QMessageBox::warning(getMainWindow(), 
                    undo?QObject::tr("Undo"):QObject::tr("Redo"),
                    QString::fromLatin1("%1,\n%2%3")
                        .arg(QObject::tr(
                            "There are grouped transactions in the following documents with "
                            "other preceding transactions"))
                        .arg(QString::fromUtf8(str.str().c_str()))
                        .arg(QObject::tr("Choose 'Yes' to roll back all preceeding transactions.\n"
                                         "Choose 'No' to roll back in the active document only.\n"
                                         "Choose 'Abort' to abort")),
                    QMessageBox::Yes|QMessageBox::No|QMessageBox::Abort, QMessageBox::Yes);
        if(ret == QMessageBox::Abort)
            return false;
        if(ret == QMessageBox::No)
            return true;
    }
    for(auto &v : dmap) {
        for(int i=0;i<v.second;++i) {
            if(undo)
                v.first->undo();
            else
                v.first->redo();
        }
    }
    return true;
}

bool Document::isPerformingTransaction() const {
    return d->_isTransacting;
}

/// Will UNDO one or more steps
void Document::undo(int iSteps)
{
    Base::FlagToggler<> flag(d->_isTransacting);

    if(!checkTransactionID(true,iSteps))
        return;

    for (int i=0;i<iSteps;i++) {
        getDocument()->undo();
    }
}

/// Will REDO one or more steps
void Document::redo(int iSteps)
{
    Base::FlagToggler<> flag(d->_isTransacting);

    if(!checkTransactionID(false,iSteps))
        return;

    for (int i=0;i<iSteps;i++) {
        getDocument()->redo();
    }
}

PyObject* Document::getPyObject(void)
{
    _pcDocPy->IncRef();
    return _pcDocPy;
}

void Document::handleChildren3D(ViewProvider* viewProvider, bool deleting)
{
    // check for children
    if (viewProvider && viewProvider->getChildRoot()) {
        std::vector<App::DocumentObject*> children = viewProvider->claimChildren3D();
        SoGroup* childGroup =  viewProvider->getChildRoot();

        // size not the same -> build up the list new
        if (deleting || childGroup->getNumChildren() != static_cast<int>(children.size())) {

            std::set<ViewProviderDocumentObject*> oldChildren;
            for(int i=0,count=childGroup->getNumChildren();i<count;++i) {
                auto it = d->_CoinMap.find(static_cast<SoSeparator*>(childGroup->getChild(i)));
                if(it == d->_CoinMap.end()) continue;
                oldChildren.insert(it->second);
            }

            childGroup->removeAllChildren();

            if(!deleting) {
                for (std::vector<App::DocumentObject*>::iterator it=children.begin();it!=children.end();++it) {
                    ViewProvider* ChildViewProvider = getViewProvider(*it);
                    if (ChildViewProvider) {
                        auto itOld = oldChildren.find(static_cast<ViewProviderDocumentObject*>(ChildViewProvider));
                        if(itOld!=oldChildren.end()) oldChildren.erase(itOld);

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
                                activeView->getViewer()->removeViewProvider(ChildViewProvider);
                            }
                        }
                    }
                }
            }

            // add the remaining old children back to toplevel invertor node
            for(auto vpd : oldChildren) {
                auto obj = vpd->getObject();
                if(!obj || !obj->getNameInDocument())
                    continue;

                for (BaseView* view : d->baseViews) {
                    View3DInventor *activeView = dynamic_cast<View3DInventor *>(view);
                    if (activeView && !activeView->getViewer()->hasViewProvider(vpd))
                        activeView->getViewer()->addViewProvider(vpd);
                }
            }
        }
    } 
}

void Document::toggleInSceneGraph(ViewProvider *vp) {
    for (auto view : d->baseViews) {
        View3DInventor *activeView = dynamic_cast<View3DInventor *>(view);
        if (!activeView)
            continue;
        auto root = vp->getRoot();
        if(!root)
            continue;
        auto scenegraph = dynamic_cast<SoGroup*>(
                activeView->getViewer()->getSceneGraph());
        if(!scenegraph)
            continue;
        int idx = scenegraph->findChild(root);
        if(idx<0) {
            if(vp->canAddToSceneGraph())
                scenegraph->addChild(root);
        }else if(!vp->canAddToSceneGraph())
            scenegraph->removeChild(idx);
    }
}

void Document::slotChangePropertyEditor(const App::Document &doc, const App::Property &) {
    if(getDocument() == &doc)
        setModified(true);
}

