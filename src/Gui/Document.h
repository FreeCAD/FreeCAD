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


#ifndef GUI_DOCUMENT_H
#define GUI_DOCUMENT_H

#include "MDIView.h"

#include <list>
#include <map>
#include <string>

#include <Base/Persistence.h>
#include <App/Document.h>

#include "Tree.h"

class SoNode;
class SoPath;

namespace Base {
class Matrix4D;
}

namespace App {
class DocumentObjectGroup;
}

namespace Gui {

class ViewProvider;
class ViewProviderDocumentObject;
class Application;
class DocumentPy;
class TransactionViewProvider;
enum  HighlightMode;

/** The Gui Document
 *  This is the document on GUI level. Its main responsibility is keeping
 *  track off open windows for a document and warning on unsaved closes.
 *  All handled views on the document must inherit from MDIView
 *  @see App::Document
 *  @see MDIView
 *  @author Jürgen Riegel
 */
class GuiExport Document : public Base::Persistence
{
public:
    Document(App::Document* pcDocument, Application * app);
    ~Document();

protected:
    /** @name I/O of the document */
    //@{
    /// This slot is connected to the App::Document::signalNewObject(...)
    void slotNewObject(const App::DocumentObject&);
    void slotDeletedObject(const App::DocumentObject&);
    void slotChangedObject(const App::DocumentObject&, const App::Property&);
    void slotRelabelObject(const App::DocumentObject&);
    void slotTransactionAppend(const App::DocumentObject&, App::Transaction*);
    void slotTransactionRemove(const App::DocumentObject&, App::Transaction*);
    void slotActivatedObject(const App::DocumentObject&);
    void slotStartRestoreDocument(const App::Document&);
    void slotFinishRestoreDocument(const App::Document&);
    void slotUndoDocument(const App::Document&);
    void slotRedoDocument(const App::Document&);
    void slotShowHidden(const App::Document&);
    void slotFinishImportObjects(const std::vector<App::DocumentObject*> &);
    void slotFinishRestoreObject(const App::DocumentObject &obj);
    void slotRecomputed(const App::Document&);
    void slotSkipRecompute(const App::Document &doc, const std::vector<App::DocumentObject*> &objs);
    void slotTouchedObject(const App::DocumentObject &);
    void slotChangePropertyEditor(const App::Property &);
    //@}

    void addViewProvider(Gui::ViewProviderDocumentObject*);

public:
    /** @name Signals of the document */
    //@{
    /// signal on new Object
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalNewObject;
    /// signal on deleted Object
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalDeletedObject;
    /** signal on changed Object, the 2nd argument is the changed property
        of the referenced document object, not of the view provider */
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&,
                                const App::Property&)>                   signalChangedObject;
    /// signal on renamed Object
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalRelabelObject;
    /// signal on activated Object
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalActivatedObject;
    /// signal on entering in edit mode
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalInEdit;
    /// signal on leaving edit mode
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalResetEdit;
    /// signal on changed Object, the 2nd argument is the highlite mode to use
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&, 
                                          const Gui::HighlightMode&, 
                                          bool,
                                          App::DocumentObject *parent, 
                                          const char *subname)> signalHighlightObject; 
    /// signal on changed Object, the 2nd argument is the highlite mode to use
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&,
                                          const Gui::TreeItemMode&,
                                          App::DocumentObject *parent, 
                                          const char *subname)> signalExpandObject;
    /// signal on changed ShowInTree property in view provider
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalShowItem;
    /// signal on scrolling to an object
    mutable boost::signals2::signal<void (const Gui::ViewProviderDocumentObject&)> signalScrollToObject;
    /// signal on undo Document
    mutable boost::signals2::signal<void (const Gui::Document& doc)> signalUndoDocument;
    /// signal on redo Document
    mutable boost::signals2::signal<void (const Gui::Document& doc)> signalRedoDocument;
    /// signal on deleting Document
    mutable boost::signals2::signal<void (const Gui::Document& doc)> signalDeleteDocument;
    //@}

    /** @name I/O of the document */
    //@{
    unsigned int getMemSize (void) const;
    /// Save the document
    bool save(void);
    /// Save the document under a new file name
    bool saveAs(void);
    /// Save a copy of the document under a new file name
    bool saveCopy(void);
    /// Save all open document
    static void saveAll();
    /// This method is used to save properties or very small amounts of data to an XML document.
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);
    /// This method is used to save large amounts of data to a binary file.
    virtual void SaveDocFile (Base::Writer &writer) const;
    /// This method is used to restore large amounts of data from a binary file.
    virtual void RestoreDocFile(Base::Reader &reader);
    void exportObjects(const std::vector<App::DocumentObject*>&, Base::Writer&);
    void importObjects(const std::vector<App::DocumentObject*>&, Base::Reader&,
                       const std::map<std::string, std::string>& nameMapping);
    /// Add all root objects of the given array to a group
    void addRootObjectsToGroup(const std::vector<App::DocumentObject*>&, App::DocumentObjectGroup*);
    //@}

    /// Observer message from the App doc
    void setModified(bool);
    bool isModified() const;

    /// Getter for the App Document
    App::Document*  getDocument(void) const;

    /** @name methods for View handling */
    //@{
    /// Getter for the active view
    Gui::MDIView* getActiveView(void) const;
    void setActiveWindow(Gui::MDIView* view);
    Gui::MDIView* getEditingViewOfViewProvider(Gui::ViewProvider*) const;
    Gui::MDIView* getViewOfViewProvider(Gui::ViewProvider*) const;
    Gui::MDIView* getViewOfNode(SoNode*) const;
    /// Create a new view
    MDIView *createView(const Base::Type& typeId);
    /// Create a clone of the given view
    Gui::MDIView* cloneView(Gui::MDIView*);
    /** send messages to the active view
     * Send a specific massage to the active view and is able to receive a
     * return message
     */
    /// send Messages to all views
    bool sendMsgToViews(const char* pMsg);
    /** Sends the message \a pMsg to the views of type \a typeid and stops with
     * the first view that supports the message and returns \a ppReturn. The very
     * first checked view is the current active view.
     * If a view supports the message true is returned and false otherwise.
     */
    bool sendMsgToFirstView(const Base::Type& typeId, const char* pMsg, const char** ppReturn);
    /// Attach a view (get called by the MDIView constructor)
    void attachView(Gui::BaseView* pcView, bool bPassiv=false);
    /// Detach a view (get called by the MDIView destructor)
    void detachView(Gui::BaseView* pcView, bool bPassiv=false);
    /// helper for selection
    ViewProviderDocumentObject* getViewProviderByPathFromTail(SoPath * path) const;
    /// helper for selection
    ViewProviderDocumentObject* getViewProviderByPathFromHead(SoPath * path) const;
    /// Get all view providers along the path and the corresponding node index in the path
    std::vector<std::pair<ViewProviderDocumentObject*,int> > getViewProvidersByPath(SoPath * path) const;
    /// call update on all attached views
    void onUpdate(void);
    /// call relabel to all attached views
    void onRelabel(void);
    /// returns a list of all attached MDI views
    std::list<MDIView*> getMDIViews() const;
    /// returns a list of all MDI views of a certain type
    std::list<MDIView*> getMDIViewsOfType(const Base::Type& typeId) const;
    //@}

    MDIView *setActiveView(ViewProviderDocumentObject *vp=0, Base::Type typeId = Base::Type());

    /** @name View provider handling  */
    //@{
    /// Get the view provider for that object
    ViewProvider* getViewProvider(const App::DocumentObject *) const;
    ViewProviderDocumentObject *getViewProvider(SoNode *node) const;
    /// set an annotation view provider
    void setAnnotationViewProvider(const char* name, ViewProvider *pcProvider);
    /// get an annotation view provider
    ViewProvider * getAnnotationViewProvider(const char* name) const;
    /// remove an annotation view provider
    void removeAnnotationViewProvider(const char* name);
    /// test if the feature is in show
    bool isShow(const char* name);
    /// put the feature in show
    void setShow(const char* name);
    /// set the feature in Noshow
    void setHide(const char* name);
    /// set the feature transformation (only viewing)
    void setPos(const char* name, const Base::Matrix4D& rclMtrx);
    std::vector<ViewProvider*> getViewProvidersOfType(const Base::Type& typeId) const;
    ViewProvider *getViewProviderByName(const char* name) const;
    /// set the ViewProvider in special edit mode
    bool setEdit(Gui::ViewProvider* p, int ModNum=0, const char *subname=0);
    const Base::Matrix4D &getEditingTransform() const;
    void setEditingTransform(const Base::Matrix4D &mat);
    /// reset from edit mode, this cause all document to reset edit
    void resetEdit(void);
    /// reset edit of this document
    void _resetEdit(void);
    /// get the in edit ViewProvider or NULL
    ViewProvider *getInEdit(ViewProviderDocumentObject **parentVp=0, 
            std::string *subname=0, int *mode=0, std::string *subElement=0) const;
    /// set the in edit ViewProvider subname reference
    void setInEdit(ViewProviderDocumentObject *parentVp, const char *subname);
    /** Add or remove view provider from scene graphs of all views
     *
     * It calls ViewProvider::canAddToSceneGraph() to decide whether to add the
     * view provider or remove it
     */
    void toggleInSceneGraph(ViewProvider *vp);
    //@}

    /** @name methods for the UNDO REDO handling */
    //@{
    /// Open a new Undo transaction on the document
    void openCommand(const char* sName=0);
    /// Commit the Undo transaction on the document
    void commitCommand(void);
    /// Abort the Undo transaction on the document
    void abortCommand(void);
    /// Check if an Undo transaction is open
    bool hasPendingCommand(void) const;
    /// Get an Undo string vector with the Undo names
    std::vector<std::string> getUndoVector(void) const;
    /// Get an Redo string vector with the Redo names
    std::vector<std::string> getRedoVector(void) const;
    /// Will UNDO one or more steps
    void undo(int iSteps);
    /// Will REDO one or more steps
    void redo(int iSteps) ;
    /** Check if the document is performing undo/redo transaction
     *
     * Unlike App::Document::isPerformingTransaction(), Gui::Document will
     * report transacting when triggering grouped undo/redo in other documents
     */
    bool isPerformingTransaction() const;
    //@}

    /// handles the application close event
    bool canClose(bool checkModify=true, bool checkLink=false);
    bool isLastView(void);

    /// called by Application before being deleted
    void beforeDelete();

    virtual PyObject *getPyObject(void);

    const std::string &getCameraSettings() const;
    void saveCameraSettings(const char *);

protected:
    // pointer to the python class
    Gui::DocumentPy *_pcDocPy;

private:
    //handles the scene graph nodes to correctly group child and parents
    void handleChildren3D(ViewProvider* viewProvider, bool deleting=false);

    /// Check other documents for the same transaction ID
    bool checkTransactionID(bool undo, int iSteps);

    struct DocumentP* d;
    static int _iDocCount;

    std::string cameraSettings;

    /** @name attributes for the UNDO REDO facility
     */
    //@{
    /// undo names list
    std::list<std::string> listUndoNames;
    /// redo names list
    std::list<std::string> listRedoNames;
    //@}

    friend class TransactionViewProvider;
};

} // namespace Gui


#endif // GUI_DOCUMENT_H
