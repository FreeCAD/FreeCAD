/***************************************************************************
 *   Copyright (c) 2004 J�rgen Riegel <juergen.riegel@web.de>              *
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

namespace Base
{
  class Matrix4D;
}

namespace Gui {

class ViewProvider;
class ViewProviderDocumentObject;
class Application;
class DocumentPy;

/** The Gui Document
 *  This is the document on GUI level. Its main responsibility is keeping
 *  track off open windows for a document and warning on unsaved closes.
 *  All handled views on the document must inherit from MDIView
 *  @see App::Document 
 *  @see MDIView
 *  @author J�rgen Riegel
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
    void slotRenamedObject(const App::DocumentObject&);
    void slotActivatedObject(const App::DocumentObject&);
    void slotRestoredDocument(const App::Document&);
    //@}

public:
    /** @name Signals of the document */
    //@{
    /// signal on new Object
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalNewObject;
    /// signal on deleted Object
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalDeletedObject;
    /// signal on changed Object, the 2nd argument is the changed property
    /// of the referenced document object, not of the view provider
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&,
                                const App::Property&)> signalChangedObject;
    /// signal on renamed Object
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalRenamedObject;
    /// signal on activated Object
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalActivatedObject;
    /// signal on goes in edti mode
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalInEdit;
   /// signal on leave edit mode
    mutable boost::signal<void (const Gui::ViewProviderDocumentObject&)> signalResetEdit;
    //@}

    /** @name I/O of the document */
    //@{
    unsigned int getMemSize (void) const;
    /// Save the document
    bool save(void);
    /// Save the document under a new file name
    bool saveAs(void);
    /// This method is used to save properties or very small amounts of data to an XML document.
    virtual void Save (Base::Writer &writer) const;
    /// This method is used to restore properties from an XML document.
    virtual void Restore(Base::XMLReader &reader);
    /// This method is used to save large amounts of data to a binary file.
    virtual void SaveDocFile (Base::Writer &writer) const;
    /// This method is used to restore large amounts of data from a binary file.
    virtual void RestoreDocFile(Base::Reader &reader);
    void exportObjects(const std::vector<App::DocumentObject*>&, Base::Writer&);
    void importObjects(const std::vector<App::DocumentObject*>&, Base::Reader&);
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
    /// Creat a new view
    void createView(const char* sType); 
    /** send messages to the active view 
     * Send a specific massage to the active view and is able to recive a
     * return massage
     */
    /// send Messages to all views
    bool sendMsgToViews(const char* pMsg);
    /// Attach a view (get called by the MDIView constructor)
    void attachView(Gui::BaseView* pcView, bool bPassiv=false);
    /// Detach a view (get called by the MDIView destructor)
    void detachView(Gui::BaseView* pcView, bool bPassiv=false);
    /// call update on all attached views
    void onUpdate(void);
    /// call relabel to all attached views
    void onRelabel(void);
    /// returns a list of all attached MDI views
    std::list<MDIView*> getMDIViews() const;
    /// returns a list of all MDI views of a certain type
    std::list<MDIView*> getMDIViewsOfType(const Base::Type& typeId) const;
    //@}

    /** @name View provider handling  */
    //@{
    /// Get the view provider for that object
    ViewProvider* getViewProvider(const App::DocumentObject *) const;
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
    bool setEdit(Gui::ViewProvider* p, int ModNum=0);
    /// reset from edit mode
    void resetEdit(void);
    /// get the in edit ViewProvider or NULL
    ViewProvider *getInEdit(void) const;
    //@}

    /** @name methods for the UNDO REDO handling */
    //@{
    /// Open a new Undo transaction on the document
    void openCommand(const char* sName=0);
    /// Commit the Undo transaction on the document
    void commitCommand(void);
    /// Abort the Undo transaction on the document
    void abortCommand(void);
    /// Get an Undo string vector with the Undo names
    std::vector<std::string> getUndoVector(void) const;
    /// Get an Redo string vector with the Redo names
    std::vector<std::string> getRedoVector(void) const;
    /// Will UNDO  one or more steps
    void undo(int iSteps);
    /// Will REDO  one or more steps
    void redo(int iSteps) ;
    //@}

    /// handels the application close event
    bool canClose();
    bool isLastView(void);

    virtual PyObject *getPyObject(void);

protected:
    // pointer to the python class
    Gui::DocumentPy *_pcDocPy;

private:
    struct DocumentP* d;
    static int _iDocCount;

    /** @name attributes for the UNDO REDO facility
     */
    //@{
    /// undo names list
    std::list<std::string> listUndoNames;
    /// redo names list
    std::list<std::string> listRedoNames;
    //@}
};

} // namespace Gui


#endif // GUI_DOCUMENT_H
