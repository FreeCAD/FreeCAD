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

#include <boost/algorithm/string/predicate.hpp>
#include <cctype>

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
#include <App/AutoTransaction.h>
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

FC_LOG_LEVEL_INIT("Gui",true,true)

using namespace Gui;

namespace Gui {

struct CameraInfo {
    int id;
    int binding;
    std::string settings;

    CameraInfo(int i, int b, std::string &&s)
        :id(i), binding(b), settings(std::move(s))
    {}
};

// Pimpl class
struct DocumentP
{
    Thumbnail thumb;
    int        _iWinCount;
    int        _iDocId;
    bool       _isClosing;
    bool       _isModified;
    bool       _isTransacting;
    bool       _hasExpansion;
    bool       _changeViewTouchDocument;
    int                         _editMode;
    ViewProvider*               _editViewProvider;
    App::DocumentObject*        _editingObject;
    ViewProviderDocumentObject* _editViewProviderParent;
    std::string                 _editSubname;
    std::string                 _editSubElement;
    Base::Matrix4D              _editingTransform;
    View3DInventorViewer*       _editingViewer;
    std::set<const App::DocumentObject*> _editObjs;

    std::vector<CameraInfo>     _savedViews;

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
    std::list<ViewProviderDocumentObject*> _redoViewProviders;

    // cache map from view provider to its 3D claimed children
    std::unordered_map<const ViewProvider*,std::vector<App::DocumentObject*> > _ChildrenMap;

    // Reference counted view providers that are 3D claimed by other object.
    // These view providers shouldn't appear at secen graph root.
    std::unordered_map<const ViewProvider*, int> _ClaimedViewProviders;

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
    Connection connectPurgeTouchedObject;
    Connection connectChangePropertyEditor;
    Connection connectChanged;

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
    d->_hasExpansion = false;
    d->_pcAppWnd = app;
    d->_pcDocument = pcDocument;
    d->_editViewProvider = 0;
    d->_editingObject = 0;
    d->_editViewProviderParent = 0;
    d->_editingViewer = 0;
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
    d->connectPurgeTouchedObject = pcDocument->signalPurgeTouchedObject.connect
        (boost::bind(&Gui::Document::slotTouchedObject, this, _1));

    d->connectTransactionAppend = pcDocument->signalTransactionAppend.connect
        (boost::bind(&Gui::Document::slotTransactionAppend, this, _1, _2));
    d->connectTransactionRemove = pcDocument->signalTransactionRemove.connect
        (boost::bind(&Gui::Document::slotTransactionRemove, this, _1, _2));

    d->connectChanged = pcDocument->signalChanged.connect(
        [this](const App::Document &, const App::Property &Prop) {
            FC_LOG(Prop.getFullName() << " modified");
            setModified(true);
        });

    // pointer to the python class
    // NOTE: As this Python object doesn't get returned to the interpreter we
    // mustn't increment it (Werner Jan-12-2006)
    _pcDocPy = new Gui::DocumentPy(this);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    if (hGrp->GetBool("UsingUndo",true)) {
        d->_pcDocument->setUndoMode(1);
        // set the maximum stack size
        d->_pcDocument->setMaxUndoStackSize(hGrp->GetInt("MaxUndoSize",20));
    }

    d->_changeViewTouchDocument = hGrp->GetBool("ChangeViewProviderTouchDocument", true);
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
    d->connectPurgeTouchedObject.disconnect();
    d->connectChangePropertyEditor.disconnect();
    d->connectChanged.disconnect();

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

struct EditDocumentGuard {
    EditDocumentGuard():active(true) {}

    ~EditDocumentGuard() {
        if(active)
            Application::Instance->setEditDocument(0);
    }

    bool active;
};

bool Document::setEdit(Gui::ViewProvider* p, int ModNum, const char *subname)
{
    ViewProviderDocumentObject* vp = dynamic_cast<ViewProviderDocumentObject*>(p);
    if (!vp) {
        FC_ERR("cannot edit non ViewProviderDocumentObject");
        return false;
    }

    // Fix regression: https://forum.freecadweb.org/viewtopic.php?f=19&t=43629&p=371972#p371972
    // When an object is already in edit mode a subsequent call for editing is only possible
    // when resetting the currently edited object.
    if (d->_editViewProvider) {
        _resetEdit();
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
        // editing document, and the actually editing sub object can be
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

    View3DInventor *view3d = dynamic_cast<View3DInventor *>(getActiveView());
    // if the currently active view is not the 3d view search for it and activate it
    if (view3d) 
        getMainWindow()->setActiveWindow(view3d);
    else
        view3d = dynamic_cast<View3DInventor *>(setActiveView(vp));

    EditDocumentGuard guard;
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

    auto sobjs = obj->getSubObjectList(subname);
    d->_editObjs.clear();
    d->_editObjs.insert(sobjs.begin(),sobjs.end());
    d->_editingObject = sobj;

    d->_editMode = ModNum;
    d->_editViewProvider = svp->startEditing(ModNum);
    if(!d->_editViewProvider) {
        d->_editViewProviderParent = 0;
        d->_editObjs.clear();
        d->_editingObject = 0;
        FC_LOG("object '" << sobj->getFullName() << "' refuse to edit");
        return false;
    }

    if(view3d) {
        view3d->getViewer()->setEditingViewProvider(d->_editViewProvider,ModNum);
        d->_editingViewer = view3d->getViewer();
    }
    Gui::TaskView::TaskDialog* dlg = Gui::Control().activeDialog();
    if (dlg)
        dlg->setDocumentName(this->getDocument()->getName());
    if (d->_editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
        auto vpd = static_cast<ViewProviderDocumentObject*>(d->_editViewProvider);
        vpd->getDocument()->signalInEdit(*vpd);
    }
    guard.active = false;
    App::AutoTransaction::setEnable(false);
    return true;
}

const Base::Matrix4D &Document::getEditingTransform() const {
    return d->_editingTransform;
}

void Document::setEditingTransform(const Base::Matrix4D &mat) {
    d->_editObjs.clear();
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

        // Have to check d->_editViewProvider below, because there is a chance
        // the editing object gets deleted inside the above call to
        // 'finishEditing()', which will trigger our slotDeletedObject(), which
        // nullifies _editViewProvider.
        if (d->_editViewProvider && d->_editViewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) 
            signalResetEdit(*(static_cast<ViewProviderDocumentObject*>(d->_editViewProvider)));
        d->_editViewProvider = 0;

        // The logic below is not necessary anymore, because this method is
        // changed into a private one,  _resetEdit(). And the exposed
        // resetEdit() above calls into Application->setEditDocument(0) which
        // will prevent recursive calling.
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
        App::GetApplication().closeActiveTransaction();
    }
    d->_editViewProviderParent = 0;
    d->_editingViewer = 0;
    d->_editObjs.clear();
    d->_editingObject = 0;
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
        for(;;) {
            if (cName.empty()) {
                // handle document object with no view provider specified
                FC_LOG(Obj.getFullName() << " has no view provider specified");
                return;
            }
            Base::BaseClass* base = static_cast<Base::BaseClass*>(
                    Base::Type::createInstanceByName(cName.c_str(),true));
            pcProvider = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(base);
            if (!pcProvider) {
                // type not derived from ViewProviderDocumentObject!!!
                FC_ERR("Invalid view provider type '" << cName << "' for " << Obj.getFullName());
                delete base;
                return;
            } else if (cName!=Obj.getViewProviderName() && !pcProvider->allowOverride(Obj)) {
                FC_WARN("View provider type '" << cName << "' does not support " << Obj.getFullName());
                delete base;
                pcProvider = 0;
                cName = Obj.getViewProviderName();
            } else
                break;
        }

        setModified(true);
        d->_ViewProviderMap[&Obj] = pcProvider;
        d->_CoinMap[pcProvider->getRoot()] = pcProvider;
        pcProvider->setStatus(Gui::ViewStatus::TouchDocument, d->_changeViewTouchDocument);

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
        pcProvider->pcDocument = this;

        // it is possible that a new viewprovider already claims children
        handleChildren3D(pcProvider);
        if (d->_isTransacting) {
            d->_redoViewProviders.push_back(pcProvider);
        }
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
        ViewProvider::clearBoundingBoxCache();
        try {
            viewProvider->update(&Prop);
            if(d->_editingViewer 
                    && d->_editingObject
                    && d->_editViewProviderParent 
                    && (Prop.isDerivedFrom(App::PropertyPlacement::getClassTypeId())
                        // Issue ID 0004230 : getName() can return null in which case strstr() crashes
                        || (Prop.getName() && strstr(Prop.getName(),"Scale")))
                    && d->_editObjs.count(&Obj)) 
            {
                Base::Matrix4D mat;
                auto sobj = d->_editViewProviderParent->getObject()->getSubObject(
                                                        d->_editSubname.c_str(),0,&mat);
                if(sobj == d->_editingObject && d->_editingTransform!=mat) {
                    d->_editingTransform = mat;
                    d->_editingViewer->setEditingTransform(d->_editingTransform);
                }
            }
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
    if(!Prop.testStatus(App::Property::NoModify) && !isModified()) {
        FC_LOG(Prop.getFullName() << " modified");
        setModified(true);
    }

    getMainWindow()->updateActions(true);
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

void Document::slotTouchedObject(const App::DocumentObject &Obj)
{
    getMainWindow()->updateActions(true);
    if(!isModified()) {
        FC_LOG(Obj.getFullName() << (Obj.isTouched()?" touched":" purged"));
        setModified(true);
    }
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

    signalChangedModified(*this);
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
                ret.emplace_back(it->second,i);
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
                        QObject::tr("The file contains external dependencies. "
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
            escapedstr = Base::Tools::escapeEncodeFilename(escapedstr);
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
                QObject::tr("Documents contains cyclic dependencies. Do you still want to save them?"),
                QMessageBox::Yes,QMessageBox::No);
        if(ret!=QMessageBox::Yes)
            return;
        docs = App::GetApplication().getDocuments();
    }
    std::map<App::Document *, bool> dmap;
    for(auto doc : docs)
        dmap[doc] = doc->mustExecute();
    for(auto doc : docs) {
        if(doc->testStatus(App::Document::PartialDoc) || doc->testStatus(App::Document::TempDoc))
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
        const char * DocName = App::GetApplication().getDocumentName(getDocument());

        // save as new file name
        Gui::WaitCursor wc;
        QString pyfn = Base::Tools::escapeEncodeFilename(fn);
        Command::doCommand(Command::Doc,"App.getDocument(\"%s\").saveCopy(\"%s\")"
                                       , DocName, (const char*)pyfn.toUtf8());

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
    writer.addFile("GuiDocument.xml", this);

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document");
    if (hGrp->GetBool("SaveThumbnail",false)) {
        int size = hGrp->GetInt("ThumbnailSize", 128);
        size = Base::clamp<int>(size, 64, 512);
        std::list<MDIView*> mdi = getMDIViews();
        for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
            if ((*it)->getTypeId().isDerivedFrom(View3DInventor::getClassTypeId())) {
                View3DInventorViewer* view = static_cast<View3DInventor*>(*it)->getViewer();
                d->thumb.setFileName(d->_pcDocument->FileName.getValue());
                d->thumb.setSize(size);
                d->thumb.setViewer(view);
                d->thumb.Save(writer);
                break;
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

void Document::readObject(Base::XMLReader &xmlReader) {
    std::string name = xmlReader.getAttribute("name");
    bool expanded = !d->_hasExpansion && !!xmlReader.getAttributeAsInteger("expanded","0");
    ViewProvider* pObj = getViewProviderByName(name.c_str());
    if (pObj) // check if this feature has been registered
        pObj->Restore(xmlReader);
    if (pObj && expanded) {
        Gui::ViewProviderDocumentObject* vp = static_cast<Gui::ViewProviderDocumentObject*>(pObj);
        this->signalExpandObject(*vp, TreeItemMode::ExpandItem,0,0);
    }
}

#define FC_GUI_SCHEMA_VER 1
#define FC_XML_GUI_POSTFIX ".Gui.xml"
#define FC_ATTR_SPLIT_XML "Split"
#define FC_ATTR_TREE_EXPANSION "HasExpansion"

/**
 * Restores the properties of the view providers.
 */
void Document::RestoreDocFile(Base::Reader &reader)
{
    Base::XMLReader xmlReader(reader);
    xmlReader.readElement("Document");
    xmlReader.DocumentSchema = xmlReader.getAttributeAsInteger("SchemaVersion","");
    if(!xmlReader.DocumentSchema)
        xmlReader.DocumentSchema = reader.getDocumentSchema();
    xmlReader.FileVersion = xmlReader.getAttributeAsInteger("FileVersion","");
    if(!xmlReader.FileVersion)
        xmlReader.FileVersion = reader.getFileVersion();

    if(boost::ends_with(reader.getFileName(),FC_XML_GUI_POSTFIX)) {
        xmlReader.readElement("ViewProvider");
        readObject(xmlReader);
        return;
    }

    bool split = !!xmlReader.getAttributeAsInteger(FC_ATTR_SPLIT_XML,"0");

    d->_hasExpansion = !!xmlReader.getAttributeAsInteger(FC_ATTR_TREE_EXPANSION,"0");
    if(d->_hasExpansion) {
        auto tree = TreeWidget::instance();
        if(tree) {
            auto docItem = tree->getDocumentItem(this);
            if(docItem)
                docItem->Restore(xmlReader);
        }
    }

    // At this stage all the document objects and their associated view providers exist.
    // Now we must restore the properties of the view providers only.
    //
    // SchemeVersion "1"
    if (xmlReader.DocumentSchema == 1) {

        if(!split) {
            // read the viewproviders itself
            xmlReader.readElement("ViewProviderData");
            int Cnt = xmlReader.getAttributeAsInteger("Count");
            for (int i=0; i<Cnt; i++) {
                int guard;
                xmlReader.readElement("ViewProvider",&guard);
                readObject(xmlReader);
                xmlReader.readEndElement("ViewProvider",&guard);
            }
            xmlReader.readEndElement("ViewProviderData");
        } else {
            for(const auto &v : d->_ViewProviderMap)
                xmlReader.addFile(std::string(v.first->getNameInDocument())+FC_XML_GUI_POSTFIX,this);
        }

        // read camera settings
        xmlReader.readElement("Camera");

        int cameraExtra = xmlReader.getAttributeAsInteger("extra", "0");
        int cameraBinding = xmlReader.getAttributeAsInteger("binding", "0");
        int cameraId = xmlReader.getAttributeAsInteger("id", "0");

        cameraSettings.clear();
        if(xmlReader.hasAttribute("settings"))
            saveCameraSettings(xmlReader.getAttribute("settings"));
        else
            saveCameraSettings(xmlReader.readCharacters().c_str());

        if(cameraSettings.size()) {
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

        d->_savedViews.clear();
        if(cameraExtra) {
            d->_savedViews.emplace_back(cameraId, cameraBinding, std::string(cameraSettings));
            for(int i=0; i<cameraExtra; ++i) {
                xmlReader.readElement("CameraExtra");
                int id = xmlReader.getAttributeAsInteger("id");
                int binding = xmlReader.getAttributeAsInteger("binding", "0");
                std::string settings;
                saveCameraSettings(xmlReader.readCharacters().c_str(),&settings);
                d->_savedViews.emplace_back(id, binding, std::move(settings));
            }

            auto views = getMDIViewsOfType(View3DInventor::getClassTypeId());
            if(views.size()) {
                while(views.size() < d->_savedViews.size())
                    views.push_back(createView(View3DInventor::getClassTypeId()));

                std::map<int,View3DInventor*> viewMap;
                size_t i=0;
                for(auto v : views) {
                    if(i == d->_savedViews.size())
                        break;
                    auto &info = d->_savedViews[i++];
                    auto view = static_cast<View3DInventor*>(v);
                    const char *ppReturn = 0;
                    view->onMsg(info.settings.c_str(), &ppReturn);
                    viewMap[info.id] = view;
                }
                i=0;
                for(auto v : views) {
                    if(i == d->_savedViews.size())
                        break;
                    auto &info = d->_savedViews[i++];
                    auto view = static_cast<View3DInventor*>(v);
                    auto it = viewMap.find(info.binding);
                    if(it != viewMap.end())
                        view->bindCamera(it->second->getCamera());
                }
            }
        }
    }

    xmlReader.readEndElement("Document");

    // In the file GuiDocument.xml new data files might be added
    if (!xmlReader.getFilenames().empty())
        xmlReader.readFiles();

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

    // Refresh ViewProviderDocumentObject isShowable status. Since it is
    // calculated based on parent status, so must call it reverse dependency
    // order.
    const auto &objs = doc.getObjects();
    auto sorted = doc.getDependencyList(objs,App::Document::DepSort);
    for (auto rit=sorted.rbegin(); rit!=sorted.rend(); ++rit) {
        auto obj = *rit;
        if(obj->getDocument() != &doc)
            continue;
        auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(getViewProvider(*rit));
        if(vpd)
            vpd->isShowable(true);
    }

    d->connectActObjectBlocker.unblock();
    App::DocumentObject* act = doc.getActiveObject();
    if (act) {
        ViewProvider* viewProvider = getViewProvider(act);
        if (viewProvider && viewProvider->isDerivedFrom(ViewProviderDocumentObject::getClassTypeId())) {
            signalActivatedObject(*(static_cast<ViewProviderDocumentObject*>(viewProvider)));
        }
    }

    setModified(doc.testStatus(App::Document::LinkStampChanged));
}

void Document::slotShowHidden(const App::Document& doc)
{
    if (d->_pcDocument != &doc)
        return;

    Application::Instance->signalShowHidden(*this);
}

void Document::writeObject(Base::Writer &writer, 
        const App::DocumentObject *doc, const ViewProvider *obj) const
{
    writer.Stream() << writer.ind() << "<ViewProvider name=\"" 
        << doc->getNameInDocument() << "\" expanded=\"" 
        << (doc->testStatus(App::Expand) ? 1:0) << "\"";

    if (obj->hasExtensions())
        writer.Stream() << " Extensions=\"True\"";

    writer.Stream() << ">\n";
    obj->Save(writer);
    writer.Stream() << writer.ind() << "</ViewProvider>\n";
}

/**
 * Saves the properties of the view providers.
 */
void Document::SaveDocFile (Base::Writer &writer) const
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>\n";

    if(boost::ends_with(writer.getCurrentFileName(),FC_XML_GUI_POSTFIX)) {
        const std::string &name = writer.getCurrentFileName();
        static const std::size_t plen = std::strlen(FC_XML_GUI_POSTFIX);
        std::string objName = name.substr(0,name.size()-plen);
        auto obj = getDocument()->getObject(objName.c_str());
        auto it = d->_ViewProviderMap.find(obj);
        if(it == d->_ViewProviderMap.end())
            FC_ERR("View object not fount: " << getDocument()->getName() << '#' << objName);
        else {
            writer.Stream() << "<!-- FreeCAD ViewProvider -->\n"
                << "<Document SchemaVersion=\"" << FC_GUI_SCHEMA_VER 
                << "\" FileVersion=\"" << writer.getFileVersion() 
                << "\">\n";
            writeObject(writer,it->first,it->second);
            writer.Stream() << "</Document>\n";
        }
        return;
    }

    writer.Stream() << "<!--\n"
                    << " FreeCAD Document, see http://www.freecadweb.org for more information..."
                    << "\n-->\n";

    writer.Stream() << "<Document SchemaVersion=\"" << FC_GUI_SCHEMA_VER 
        << "\" FileVersion=\"" << writer.getFileVersion() << "\" "
        << FC_ATTR_SPLIT_XML "=\"" << (writer.isSplitXML()?1:0) << "\"";

    auto tree = TreeWidget::instance();
    bool hasExpansion = false;
    if(tree) {
        auto docItem = tree->getDocumentItem(this);
        if(docItem) {
            hasExpansion = true;
            writer.Stream() << " " FC_ATTR_TREE_EXPANSION "=\"1\">\n";
            docItem->Save(writer);
        }
    }
    if(!hasExpansion)
        writer.Stream() << ">\n";

    if(writer.isSplitXML()) {
        for(const auto &v : d->_ViewProviderMap)
            writer.addFile(std::string(v.first->getNameInDocument())+FC_XML_GUI_POSTFIX,this);
    } else {
        writer.incInd(); 

        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator it;

        // writing the view provider names itself
        writer.Stream() << writer.ind() << "<ViewProviderData Count=\"" 
                        << d->_ViewProviderMap.size() <<"\">\n";

        writer.incInd(); // indentation for 'ViewProvider name'
        for(it = d->_ViewProviderMap.begin(); it != d->_ViewProviderMap.end(); ++it)
            writeObject(writer,it->first, it->second);
        writer.decInd(); // indentation for 'ViewProvider name'
        writer.Stream() << writer.ind() << "</ViewProviderData>\n";
        writer.decInd();  // indentation for 'ViewProviderData Count'
    }

    writer.incInd(); // indentation for camera settings

    // save camera settings
    std::list<MDIView*> mdi = getMDIViews();
    std::vector<CameraInfo> cameraInfo;
    bool first = true;
    for (std::list<MDIView*>::iterator it = mdi.begin(); it != mdi.end(); ++it) {
        auto v = *it;
        if (v->onHasMsg("GetCamera")) {
            const char* ppReturn=0;
            v->onMsg("GetCamera",&ppReturn);

            std::string settings;
            if(!saveCameraSettings(ppReturn, &settings))
                continue;
            if(first) {
                first = false;
                cameraSettings = settings;
            }

            auto view = Base::freecad_dynamic_cast<View3DInventor>(v);
            if(!view)
                continue;
            auto binding = view->boundView();
            cameraInfo.emplace_back(view->getID(),
                    binding?binding->getID():0, std::move(settings));
        }
    }

    writer.Stream() << writer.ind() << "<Camera";
    if(cameraInfo.size())
        writer.Stream() << " extra=\"" << cameraInfo.size()-1 << "\" id=\""
            << cameraInfo[0].id << "\" binding=\"" << cameraInfo[0].binding << "\"";
    if(writer.getFileVersion() > 1) {
        writer.Stream() << ">\n";
        writer.beginCharStream(false) << '\n' << getCameraSettings();
        writer.endCharStream() << '\n' << writer.ind() << "</Camera>\n";
    } else {
        writer.Stream() << " settings=\"" 
            << encodeAttribute(getCameraSettings()) << "\"/>\n";
    }
    if(cameraInfo.size()>1) {
        for(size_t i=1; i<cameraInfo.size(); ++i) {
            auto &info = cameraInfo[i];
            writer.Stream() << writer.ind() << "<CameraExtra id=\""
                << info.id << "\" binding=\"" << info.binding << "\">\n";
            writer.beginCharStream(false) << '\n' << getCameraSettings(&info.settings);
            writer.endCharStream() << '\n' << writer.ind() << "</CameraExtra>\n";
        }
    }
    d->_savedViews = std::move(cameraInfo);

    writer.decInd(); // indentation for camera settings
    writer.Stream() << "</Document>\n";
}

void Document::exportObjects(const std::vector<App::DocumentObject*>& obj, Base::Writer& writer)
{
    writer.Stream() << "<?xml version='1.0' encoding='utf-8'?>\n";
    writer.Stream() << "<Document SchemaVersion=\"" << FC_GUI_SCHEMA_VER << "\">\n";

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
                    << views.size() <<"\">\n";

    writer.incInd(); // indentation for 'ViewProvider name'
    std::map<const App::DocumentObject*,ViewProvider*>::const_iterator jt;
    for (jt = views.begin(); jt != views.end(); ++jt)
        writeObject(writer,jt->first, jt->second);

    writer.decInd(); // indentation for 'ViewProvider name'
    writer.Stream() << writer.ind() << "</ViewProviderData>\n";
    writer.decInd();  // indentation for 'ViewProviderData Count'
    writer.incInd(); // indentation for camera settings
    writer.Stream() << writer.ind() << "<Camera settings=\"\"/>\n";
    writer.decInd(); // indentation for camera settings
    writer.Stream() << "</Document>\n";
}

void Document::importObjects(const std::vector<App::DocumentObject*>& obj, Base::Reader& reader,
                             const std::map<std::string, std::string>& nameMapping)
{
    // We must create an XML parser to read from the input stream
    Base::XMLReader xmlReader(reader);
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
                    this->signalExpandObject(*vpd, TreeItemMode::ExpandItem,0,0);
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
        xmlReader.readFiles();
}

void Document::slotFinishImportObjects(const std::vector<App::DocumentObject*> &objs) {
    (void)objs;
    // finishRestoring() is now triggered by signalFinishRestoreObject
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

        // Views can now have independent draw styles (i.e. override modes)
        //
        // if (!theViews.empty()) {
        //     View3DInventor* firstView = static_cast<View3DInventor*>(theViews.front());
        //     std::string overrideMode = firstView->getViewer()->getOverrideMode();
        //     view3D->getViewer()->setOverrideMode(overrideMode);
        // }

        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator It1;
        for (It1=d->_ViewProviderMap.begin();It1!=d->_ViewProviderMap.end();++It1) {
            view3D->getViewer()->addViewProvider(It1->second);
        }
        std::map<std::string,ViewProvider*>::const_iterator It2;
        for (It2=d->_ViewProviderMapAnnotation.begin();It2!=d->_ViewProviderMapAnnotation.end();++It2) {
            view3D->getViewer()->addViewProvider(It2->second);
        }

        const char* name = getDocument()->Label.getValue();
        QString title = QString::fromLatin1("%1 : %2[*]")
            .arg(QString::fromUtf8(name)).arg(d->_iWinCount++);

        view3D->setWindowTitle(title);
        view3D->setWindowModified(this->isModified());
        view3D->setWindowIcon(QApplication::windowIcon());
        view3D->resize(400, 300);

        if (!cameraSettings.empty()) {
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

        View3DInventor* firstView = static_cast<View3DInventor*>(oldview);
        std::string overrideMode = firstView->getViewer()->getOverrideMode();
        view3D->getViewer()->setOverrideMode(overrideMode);

        std::map<const App::DocumentObject*,ViewProviderDocumentObject*>::const_iterator It1;
        for (It1=d->_ViewProviderMap.begin();It1!=d->_ViewProviderMap.end();++It1) {
            view3D->getViewer()->addViewProvider(It1->second);
        }
        std::map<std::string,ViewProvider*>::const_iterator It2;
        for (It2=d->_ViewProviderMapAnnotation.begin();It2!=d->_ViewProviderMapAnnotation.end();++It2) {
            view3D->getViewer()->addViewProvider(It2->second);
        }

        view3D->setWindowTitle(oldview->windowTitle());
        view3D->setWindowModified(oldview->isWindowModified());
        view3D->setWindowIcon(oldview->windowIcon());
        view3D->resize(oldview->size());

        // FIXME: Add parameter to define behaviour by the calling instance
        // View provider editing
        if (d->_editViewProvider) {
            firstView->getViewer()->resetEditingViewProvider();
            view3D->getViewer()->setEditingViewProvider(d->_editViewProvider, d->_editMode);
        }

        return view3D;
    }

    return 0;
}

const char *Document::getCameraSettings(const std::string *settings) const {
    if(!settings)
        settings = &cameraSettings;
    return settings->size()>10?settings->c_str()+10:settings->c_str();
}

bool Document::saveCameraSettings(const char *settings, std::string *dst) const {
    if(!settings)
        return false;

    if(!dst)
        dst = &cameraSettings;

    // skip starting comment lines
    bool skipping = false;
    char c = *settings;
    for(;c;c=*(++settings)) {
        if(skipping) {
            if(c == '\n')
                skipping = false;
        } else if(c == '#')
            skipping = true;
        else if(!std::isspace(c))
            break;
    }

    if(!c)
        return false;

    *dst = std::string("SetCamera ") + settings;
    return true;
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

    if (getDocument()->testStatus(App::Document::TempDoc))
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
                // getInEdit() only checks if the currently active MDI view is
                // a 3D view and that it is in edit mode. However, when closing a
                // document then the edit mode must be reset independent of the
                // active view.
                if (d->_editViewProvider)
                    this->_resetEdit();
            }
        }
    }

    return ok;
}

const std::list<BaseView*> &Document::getViews() const {
    return d->baseViews;
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

MDIView *Document::setActiveView(ViewProviderDocumentObject *vp, Base::Type typeId)
{
    MDIView *view = nullptr;
    if (!vp) {
        view = getActiveView();
    }
    else {
        view = vp->getMDIView();
        if (!view) {
            auto obj = vp->getObject();
            if (!obj) {
                view = getActiveView();
            }
            else {
                auto linked = obj->getLinkedObject(true);
                if (linked!=obj) {
                    auto vpLinked = dynamic_cast<ViewProviderDocumentObject*>(
                                Application::Instance->getViewProvider(linked));
                    if (vpLinked)
                        view = vpLinked->getMDIView();
                }

                if (!view && typeId.isBad()) {
                    MDIView* active = getActiveView();
                    if (active && active->containsViewProvider(vp))
                        view = active;
                    else
                        typeId = View3DInventor::getClassTypeId();
                }
            }
        }
    }

    if (!view || (!typeId.isBad() && !view->isDerivedFrom(typeId))) {
        view = nullptr;
        for (auto *v : d->baseViews) {
            if (v->isDerivedFrom(MDIView::getClassTypeId()) &&
               (typeId.isBad() || v->isDerivedFrom(typeId))) {
                view = static_cast<MDIView*>(v);
                break;
            }
        }
    }

    if (!view && !typeId.isBad())
        view = createView(typeId);

    if (view)
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
    (void)vp;
    std::list<MDIView*> mdis = getMDIViewsOfType(View3DInventor::getClassTypeId());
    for (std::list<MDIView*>::const_iterator it = mdis.begin(); it != mdis.end(); ++it) {
        View3DInventor* view = static_cast<View3DInventor*>(*it);
        View3DInventorViewer* viewer = view->getViewer();
        // there is only one 3d view which is in edit mode
        if (viewer->isEditingViewProvider())
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
    if(!iSteps)
        return false;

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
        int i=0;
        for(auto doc : prompts) {
            if(i++==5) {
                str << "...\n";
                break;
            }
            str << "    " << doc->getName() << "\n";
        }
        int ret = QMessageBox::warning(getMainWindow(), 
                    undo?QObject::tr("Undo"):QObject::tr("Redo"),
                    QString::fromLatin1("%1,\n%2%3")
                        .arg(QObject::tr(
                            "There are grouped transactions in the following documents with "
                            "other preceding transactions"))
                        .arg(QString::fromUtf8(str.str().c_str()))
                        .arg(QObject::tr("Choose 'Yes' to roll back all preceding transactions.\n"
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

    Gui::Selection().clearCompleteSelection();

    {
        App::TransactionGuard guard;

        if(!checkTransactionID(true,iSteps))
            return;

        for (int i=0;i<iSteps;i++) {
            getDocument()->undo();
        }
    }
    App::GetApplication().signalUndo();
}

/// Will REDO one or more steps
void Document::redo(int iSteps)
{
    Base::FlagToggler<> flag(d->_isTransacting);

    Gui::Selection().clearCompleteSelection();

    {
        App::TransactionGuard guard;

        if(!checkTransactionID(false,iSteps))
            return;

        for (int i=0;i<iSteps;i++) {
            getDocument()->redo();
        }
    }
    App::GetApplication().signalRedo();

    for (auto it : d->_redoViewProviders)
        handleChildren3D(it);
    d->_redoViewProviders.clear();
}

PyObject* Document::getPyObject(void)
{
    _pcDocPy->IncRef();
    return _pcDocPy;
}

void Document::handleChildren3D(ViewProvider* viewProvider, bool deleting)
{
    if(!viewProvider)
        return;
    SoGroup* childGroup =  viewProvider->getChildRoot();
    if(!childGroup)
        return;

    std::vector<App::DocumentObject*> children, *childCache;

    if(deleting) {
        // When we are deleting this view provider, do not call
        // claimChildren3D(), but fetch the last claimed result from cache.
        auto it = d->_ChildrenMap.find(viewProvider);
        childCache = &children;
        if(it != d->_ChildrenMap.end()) {
            children = std::move(it->second);
            d->_ChildrenMap.erase(it);
        }
    } else {
        // If not deleting, check if children have changed
        children = viewProvider->claimChildren3D();
        childCache = &d->_ChildrenMap[viewProvider];
        if(children == *childCache)
            return;
    }

    // Obtained the old view provider
    std::set<ViewProviderDocumentObject*> oldChildren;
    for(auto child : *childCache) {
        auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(getViewProvider(child));
        if(vp)
            oldChildren.insert(vp);
    }

    if(deleting) 
        Gui::coinRemoveAllChildren(childGroup);
    else {

        bool handled = viewProvider->handleChildren3D(children);
        if(!handled)
            Gui::coinRemoveAllChildren(childGroup);

        for(auto it=children.begin();it!=children.end();) {
            auto child = *it;
            auto vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(getViewProvider(child));
            if(!vp || !vp->getRoot()) {
                it = children.erase(it);
                continue;
            }
            ++it;

            if(!handled) {
                // If the view provider does not handle its own children, we do it by
                // simply adding the child root node to child group node.
                childGroup->addChild(vp->getRoot());
            }

            auto iter = oldChildren.find(vp);
            if(iter!=oldChildren.end())
                oldChildren.erase(iter);
            else if(++d->_ClaimedViewProviders[vp] == 1) {
                foreachView<View3DInventor>([=](View3DInventor* view){
                    view->getViewer()->toggleViewProvider(vp);
                });
            }
        }

        *childCache = std::move(children);
    }

    for(auto it=oldChildren.begin();it!=oldChildren.end();) {
        auto iter = d->_ClaimedViewProviders.find(*it);
        if(iter != d->_ClaimedViewProviders.end()) {
            if(--iter->second > 0) {
                it = oldChildren.erase(it);
                continue;
            } else
                d->_ClaimedViewProviders.erase(iter);
        }
        ++it;
    }

    // add the remaining old children back to toplevel invertor node
    foreachView<View3DInventor>([&](View3DInventor *view) {
        for(auto vpd : oldChildren) {
            auto obj = vpd->getObject();
            if(obj && obj->getNameInDocument())
                view->getViewer()->toggleViewProvider(vpd);
        }
    });
}

bool Document::isClaimed3D(ViewProvider *vp) const {
    return d->_ClaimedViewProviders.count(vp)!=0;
}

void Document::toggleInSceneGraph(ViewProvider *vp) {
    foreachView<View3DInventor>([&](View3DInventor *view) {
        view->getViewer()->toggleViewProvider(vp);
    });
}

void Document::slotChangePropertyEditor(const App::Document &doc, const App::Property &Prop) {
    if(getDocument() == &doc) {
        FC_LOG(Prop.getFullName() << " editor changed");
        setModified(true);
    }
}

