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


#ifndef GUI_VIEWPROVIDER_DOCUMENTOBJECT_H
#define GUI_VIEWPROVIDER_DOCUMENTOBJECT_H

#include <Inventor/SoType.h>

#include "ViewProvider.h"
#include <App/DocumentObject.h>

class SoMaterial;
class SoDrawStyle;
class SoNode;
class SoType;

namespace App
{
  class DocumentObject;
  class Material;
}


namespace Gui {

class MDIView;
class Document;

class GuiExport ViewProviderDocumentObject : public ViewProvider
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderDocumentObject);

public:
    /// constructor.
    ViewProviderDocumentObject();

    /// destructor.
    virtual ~ViewProviderDocumentObject();

    // Display properties
    App::PropertyEnumeration DisplayMode;
    App::PropertyBool Visibility;
    App::PropertyBool ShowInTree;
    App::PropertyEnumeration OnTopWhenSelected;
    App::PropertyEnumeration SelectionStyle;

    virtual void attach(App::DocumentObject *pcObject);
    virtual void reattach(App::DocumentObject *);
    virtual void update(const App::Property*) override;
    /// Set the active mode, i.e. the first item of the 'Display' property.
    void setActiveMode();
    /// Hide the object in the view
    virtual void hide(void) override;
    /// Show the object in the view
    virtual void show(void) override;
    /// Is called by the tree if the user double clicks on the object. It returns the string
    /// for the transaction that will be shown in the undo/redo dialog.
    /// If null is returned then no transaction will be opened.
    virtual const char* getTransactionText() const override;

    virtual bool canDropObjectEx(App::DocumentObject *, App::DocumentObject *,
            const char *, const std::vector<std::string> &) const override;

    virtual int replaceObject(App::DocumentObject*, App::DocumentObject*) override;

    virtual bool showInTree() const override;

    /// Get a list of TaskBoxes associated with this object
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>&) const override;

    /// Run a redraw
    void updateView();
    /// Get the object of this ViewProvider object
    App::DocumentObject *getObject(void) const {return pcObject;}
    /// Asks the view provider if the given object can be deleted.
    virtual bool canDelete(App::DocumentObject* obj) const override;
    /// Get the GUI document to this ViewProvider object
    Gui::Document* getDocument() const;
    /// Get the python wrapper for that ViewProvider
    PyObject* getPyObject() override;

    /// return a hit element given the picked point which contains the full node path
    virtual bool getElementPicked(const SoPickedPoint *, std::string &subname) const override;
    /// return the coin node detail and path to the node of the subname
    virtual bool getDetailPath(const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const override;

    /* Force update visual
     *
     * These method exists because some view provider skips visual update when
     * hidden (e.g. PartGui::ViewProviderPartExt). Call this function to force
     * visual update.
     */
    //@{
    virtual void forceUpdate(bool enable = true) {(void)enable;}
    virtual bool isUpdateForced() const {return false;}
    //@}

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring();
    virtual void finishRestoring();
    //@}

    virtual bool removeDynamicProperty(const char* prop) override;

    virtual App::Property* addDynamicProperty(
            const char* type, const char* name=0,
            const char* group=0, const char* doc=0,
            short attr=0, bool ro=false, bool hidden=false) override;

    /** Return the linked view object
     *
     * This function is mainly used for GUI navigation (e.g.
     * StdCmdLinkSelectLinked).
     *
     * @param subname: output as the subname referencing the linked object
     * @param recursive: whether to follow the link recursively
     *
     * @return Returns the linked view provider. If none, it shall return
     * itself.
     */
    virtual ViewProviderDocumentObject *getLinkedViewProvider(
            std::string *subname=0, bool recursive=false) const;

    virtual std::string getFullName() const override;

    /** Allow this class to be used as an override for the original view provider of the given object
     *
     * @sa App::DocumentObject::getViewProviderNameOverride()
     */
    virtual bool allowOverride(const App::DocumentObject &) const {
        return false;
    }

    void setShowable(bool enable);
    bool isShowable() const;

protected:
    /*! Get the active mdi view of the document this view provider is part of.
      @note The returned mdi view doesn't need to be a 3d view but can be e.g.
      an image view, an SVG view or something else.
     */
    Gui::MDIView* getActiveView() const;
    /*! Get the mdi view of the document this view provider is part of and
      that is in editing mode.
      @note In case there is no mdi view in editing mode 0 is returned.
      If a value different to 0 is returned it is guaranteed to be a 3d view.
     */
    Gui::MDIView* getEditingView() const;
    /*! Get any mdi view of the document this view provider is part of.
      In case there is an mdi view in editing mode that contains this
      view provider that mdi view is returned. Otherwise any other
      3d view that contains this view provider is returned.
      If a value different to 0 is returned it is guaranteed to be a 3d view.
     */
    Gui::MDIView* getInventorView() const;
    /*! Get the mdi view of the document that contains the given \a node.
     */
    Gui::MDIView* getViewOfNode(SoNode* node) const;
    /// get called before the value is changed
    virtual void onBeforeChange(const App::Property* prop) override;
    /// Gets called by the container whenever a property has been changed
    virtual void onChanged(const App::Property* prop) override;
    /** Searches in all view providers that are attached to an object that
     * is part of the same document as the object this view provider is
     * attached to for an front root of \a type.
     * Before calling this function this view provider has to be attached
     * to an object. The method returns after the first front root node
     * matches. If no front root node matches, 0 is returned.
     */
    SoNode* findFrontRootOfType(const SoType& type) const;

    /** @name Transaction handling
     */
    //@{
    virtual bool isAttachedToDocument() const override;
    virtual const char* detachFromDocument() override;

    /// get called when a property status has changed
    virtual void onPropertyStatusChanged(const App::Property &prop, unsigned long oldStatus) override;

    //@}

    virtual void setModeSwitch() override;

protected:
    App::DocumentObject *pcObject;
    Gui::Document* pcDocument;

private:
    bool _Showable = true;

    std::vector<const char*> aDisplayEnumsArray;
    std::vector<std::string> aDisplayModesArray;

    friend class Document;
};


} // namespace Gui

#endif // GUI_VIEWPROVIDER_DOCUMENTOBJECT_H

