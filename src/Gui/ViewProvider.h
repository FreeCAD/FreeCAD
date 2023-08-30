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

#ifndef GUI_VIEWPROVIDER_H
#define GUI_VIEWPROVIDER_H

#include <bitset>
#include <map>
#include <string>
#include <vector>
#include <QIcon>
#include <boost_signals2.hpp>
#include <boost/intrusive_ptr.hpp>

#include <App/Material.h>
#include <App/TransactionalObject.h>
#include <Base/BoundBox.h>
#include <Base/Vector3D.h>


class SbVec2s;
class SbVec3f;
class SoNode;
class SoPath;
class SoSeparator;
class SoEvent;
class SoSwitch;
class SoTransform;
class SbMatrix;
class SoEventCallback;
class SoPickedPoint;
class SoDetail;
class SoFullPath;
class QString;
class QMenu;
class QObject;


namespace Base {
  class Matrix4D;
}
namespace App {
  class Color;
}

class SoGroup;


namespace Gui {
    namespace TaskView {
        class TaskContent;
    }
class View3DInventorViewer;
class ViewProviderPy;
class ObjectItem;
class MDIView;

enum ViewStatus {
    UpdateData = 0,
    Detach = 1,
    isRestoring = 2,
    UpdatingView = 3,
    TouchDocument = 4,
};


/** Convenience smart pointer to wrap coin node.
 *
 * It is basically boost::intrusive plus implicit pointer conversion to save the
 * trouble of typing get() all the time.
 */
template<class T>
class CoinPtr: public boost::intrusive_ptr<T> {
public:
    // Too bad, VC2013 does not support constructor inheritance
    //using boost::intrusive_ptr<T>::intrusive_ptr;
    using inherited = boost::intrusive_ptr<T>;
    CoinPtr() = default;
    CoinPtr(T *p, bool add_ref=true):inherited(p,add_ref){}
    template<class Y> CoinPtr(CoinPtr<Y> const &r):inherited(r){}

    operator T *() const {
        return this->get();
    }//explicit bombs
};

/** Helper function to deal with bug in SoNode::removeAllChildren()
 *
 * @sa https://bitbucket.org/Coin3D/coin/pull-requests/119/fix-sochildlist-auditing/diff
 */
void GuiExport coinRemoveAllChildren(SoGroup *node);

/** General interface for all visual stuff in FreeCAD
  * This class is used to generate and handle all around
  * visualizing and presenting objects from the FreeCAD
  * App layer to the user. This class and its descendents
  * have to be implemented for any object type in order to
  * show them in the 3DView and TreeView.
  */
class GuiExport ViewProvider : public App::TransactionalObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProvider);

public:
    /// constructor.
    ViewProvider();

    /// destructor.
    ~ViewProvider() override;

    // returns the root node of the Provider (3D)
    virtual SoSeparator* getRoot() const {return pcRoot;}
    // return the mode switch node of the Provider (3D)
    SoSwitch *getModeSwitch() const {return pcModeSwitch;}
    SoTransform *getTransformNode() const {return pcTransform;}
    // returns the root for the Annotations.
    SoSeparator* getAnnotation();
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getFrontRoot() const;
    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot() const;
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getBackRoot() const;
    ///Indicate whether to be added to scene graph or not
    virtual bool canAddToSceneGraph() const {return true;}
    // Indicate whether to be added to object group (true) or only to scene graph (false)
    virtual bool isPartOfPhysicalObject() const {return true;}

    /** deliver the children belonging to this object
      * this method is used to deliver the objects to
      * the 3DView which should be grouped under its
      * scene graph. This affects the visibility and the 3D
      * position of the object.
      */
    virtual std::vector<App::DocumentObject*> claimChildren3D() const;

    /** @name Selection handling
      * This group of methods do the selection handling.
      * Here you can define how the selection for your ViewProfider
      * works.
     */
    //@{

    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel() const;
    virtual bool isSelectable() const {return true;}
    /// return a hit element given the picked point which contains the full node path
    virtual bool getElementPicked(const SoPickedPoint *, std::string &subname) const;
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail *) const { return {}; }
    /// return the coin node detail of the subelement
    virtual SoDetail* getDetail(const char *) const { return nullptr; }

    /** return the coin node detail and path to the node of the subelement
     *
     * @param subname: dot separated string reference to the sub element
     * @param pPath: output coin path leading to the returned element detail
     * @param append: If true, pPath will be first appended with the root node and
     * the mode switch node of this view provider.
     *
     * @return the coint detail of the subelement
     *
     * If this view provider links to other view provider, then the
     * implementation of getDetailPath() shall also append all intermediate
     * nodes starting just after the mode switch node up till the mode switch of
     * the linked view provider.
     */
    virtual bool getDetailPath(const char *subname, SoFullPath *pPath, bool append, SoDetail *&det) const;

    /** partial rendering setup
     *
     * @param subelements: a list of dot separated string refer to the sub element
     * @param clear: if true, remove the subelement from partial rendering.
     * If else, add the subelement for rendering.
     *
     * @return Return the number of subelement found
     *
     * Partial rendering only works if there is at least one SoFCSelectRoot node
     * in this view provider
     */
    int partialRender(const std::vector<std::string> &subelements, bool clear);

    virtual std::vector<Base::Vector3d> getModelPoints(const SoPickedPoint *) const;
    /// return the highlight lines for a given element or the whole shape
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const {
        (void)Element;
        return {};
    }

    /** Return the bound box of this view object
     *
     * This method shall work regardless whether the current view object is
     * visible or not.
     */
    Base::BoundBox3d getBoundingBox(const char *subname=nullptr, bool transform=true, MDIView *view=nullptr) const;

    /**
     * Get called if the object is about to get deleted.
     * Here you can delete other objects, switch their visibility or prevent the deletion of the object.
     * @param subNames  list of selected subelements
     * @return          true if the deletion is approved by the view provider.
     */
    virtual bool onDelete(const std::vector<std::string> &subNames);
    /** Called before deletion
     *
     * Unlike onDelete(), this function is guaranteed to be called before
     * deletion, either by Document::remObject(), or on document deletion.
     */
    virtual void beforeDelete();
    /**
     * @brief Asks the view provider if the given object that is part of its
     * outlist can be removed from there without breaking it.
     * @param obj is part of the outlist of the object associated to the view provider
     * @return true if the removal is approved by the view provider.
     */
    virtual bool canDelete(App::DocumentObject* obj) const;
    //@}


    /** @name Methods used by the Tree
      * If you want to take control over the
      * appearance of your object in the tree you
      * can reimplement these methods.
     */
    //@{
    /// deliver the icon shown in the tree view
    virtual QIcon getIcon() const;

     /** @name Methods used by the Tree
     * If you want to take control over the
     * viewprovider specific overlay icons that will be drawn with color
     * regardless of whether the icon is greyed out or not, such as status, you
     * can reimplement this method.
     */
    virtual QIcon mergeColorfulOverlayIcons (const QIcon & orig) const;

    /** deliver the children belonging to this object
      * this method is used to deliver the objects to
      * the tree framework which should be grouped under its
      * label. Obvious is the usage in the group but it can
      * be used for any kind of grouping needed for a special
      * purpose.
      */
    virtual std::vector<App::DocumentObject*> claimChildren() const;
    //@}

    /** @name Drag and drop
     * To enable drag and drop you have to re-implement \ref canDragObjects() and
     * \ref canDropObjects() to return true. For finer control you can also re-implement
     * \ref canDragObject() or \ref canDropObject() to filter certain object types, by
     * default these methods don't filter any types.
     * To take action of drag and drop the method \ref dragObject() and \ref dropObject()
     * must be re-implemented, too.
     */
    //@{
    /** Check whether children can be removed from the view provider by drag and drop */
    virtual bool canDragObjects() const;
    /** Check whether the object can be removed from the view provider by drag and drop */
    virtual bool canDragObject(App::DocumentObject*) const;
    /** Remove a child from the view provider by drag and drop */
    virtual void dragObject(App::DocumentObject*);
    /** Check whether objects can be added to the view provider by drag and drop or drop only */
    virtual bool canDropObjects() const;
    /** Check whether the object can be dropped to the view provider by drag and drop or drop only*/
    virtual bool canDropObject(App::DocumentObject*) const;
    /** Return false to force drop only operation for a given object*/
    virtual bool canDragAndDropObject(App::DocumentObject*) const;
    /** Add an object to the view provider by drag and drop */
    virtual void dropObject(App::DocumentObject*);
    /** Query object dropping with full qualified name
     *
     * Tree view now calls this function instead of canDropObject(), and may
     * query for objects from other document. The default implementation
     * (actually in ViewProviderDocumentObject) inhibites cross document
     * dropping, and calls canDropObject(obj) for the rest. Override this
     * function to enable cross document linking.
     *
     * @param obj: the object being dropped
     *
     * @param owner: the (grand)parent object of the dropping object. Maybe
     * null. This may not be the top parent object, as tree view will try to
     * find a parent of the dropping object relative to this object to avoid
     * cyclic dependency
     *
     * @param subname: subname reference to the dropping object
     *
     * @param elements: non-object sub-elements, e.g. Faces, Edges, selected
     * when the object is being dropped
     *
     * @return Return whether the dropping action is allowed.
     * */
    virtual bool canDropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner,
            const char *subname, const std::vector<std::string> &elements) const;

    /// return a subname referencing the sub-object holding the dropped objects
    virtual std::string getDropPrefix() const { return {}; }

    /** Add an object with full qualified name to the view provider by drag and drop
     *
     * @param obj: the object being dropped
     *
     * @param owner: the (grand)parent object of the dropping object. Maybe
     * null. This may not be the top parent object, as tree view will try to
     * find a parent of the dropping object relative to this object to avoid
     * cyclic dependency
     *
     * @param subname: subname reference to the dropping object
     *
     * @param elements: non-object sub-elements, e.g. Faces, Edges, selected
     * when the object is being dropped
     *
     * @return Optionally returns a subname reference locating the dropped
     * object, which may or may not be the actual dropped object, e.g. it may be
     * a link.
     */
    virtual std::string dropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner,
            const char *subname, const std::vector<std::string> &elements);
    /** Replace an object to the view provider by drag and drop
     *
     * @param oldObj: object to be replaced
     * @param newObj: object to replace with
     *
     * @return Returns 0 if not found, 1 if succeeded, -1 if not supported
     */
    virtual int replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj);
    //@}

    /** Tell the tree view if this object should appear there */
    virtual bool showInTree() const { return true; }
    /** Tell the tree view to remove children items from the tree root*/
    virtual bool canRemoveChildrenFromRoot() const {return true;}

    /** @name Signals of the view provider */
    //@{
    /// signal on icon change
    boost::signals2::signal<void ()> signalChangeIcon;
    /// signal on tooltip change
    boost::signals2::signal<void (const QString&)> signalChangeToolTip;
    /// signal on status tip change
    boost::signals2::signal<void (const QString&)> signalChangeStatusTip;
    //@}

    /** update the content of the ViewProvider
     * this method have to implement the recalculation
     * of the ViewProvider. There are different reasons to
     * update. E.g. only the view attribute has changed, or
     * the data has manipulated.
     */
    virtual void update(const App::Property*);
    virtual void updateData(const App::Property*);
    bool isUpdatesEnabled () const;
    void setUpdatesEnabled (bool enable);

    /// return the status bits
    unsigned long getStatus() const {return StatusBits.to_ulong();}
    bool testStatus(ViewStatus pos) const {return StatusBits.test((size_t)pos);}
    void setStatus(ViewStatus pos, bool on) {StatusBits.set((size_t)pos, on);}

    std::string toString() const;
    PyObject* getPyObject() override;

    /** @name Display mode methods
     */
    //@{
    std::string getActiveDisplayMode() const;
    /// set the display mode
    virtual void setDisplayMode(const char* ModeName);
    /// get the default display mode
    virtual const char* getDefaultDisplayMode() const;
    /// returns a list of all possible display modes
    virtual std::vector<std::string> getDisplayModes() const;
    /// Hides the view provider
    virtual void hide();
    /// Shows the view provider
    virtual void show();
    /// checks whether the view provider is visible or not
    virtual bool isShow() const;
    void setVisible(bool);
    bool isVisible() const;
    void setLinkVisible(bool);
    bool isLinkVisible() const;
    /// Overrides the display mode with mode.
    virtual void setOverrideMode(const std::string &mode);
    const std::string getOverrideMode();
    //@}

    /** @name Color management methods
     */
    //@{
    virtual std::map<std::string, App::Color> getElementColors(const char *element=nullptr) const {
        (void)element;
        return {};
    }
    virtual void setElementColors(const std::map<std::string, App::Color> &colors) {
        (void)colors;
    }
    static const std::string &hiddenMarker();
    static const char *hasHiddenMarker(const char *subname);
    //@}

    /** @name Edit methods
     * if the Viewprovider goes in edit mode
     * you can handle most of the events in the viewer by yourself
     */
    //@{
    // the below enum is reflected in 'userEditModes' std::map in Application.h
    // so it is possible for the user to choose a default one through GUI
    // if you add a mode here, consider to make it accessible there too
    enum EditMode {Default = 0,
                   Transform,
                   Cutting,
                   Color,
    };
protected:
    /// is called by the document when the provider goes in edit mode
    virtual bool setEdit(int ModNum);
    /// is called when you lose the edit mode
    virtual void unsetEdit(int ModNum);
    /// return the edit mode or -1 if nothing is being edited
    int getEditingMode() const;

public:
    virtual ViewProvider *startEditing(int ModNum=0);
    bool isEditing() const;
    void finishEditing();
    /// adjust viewer settings when editing a view provider
    virtual void setEditViewer(View3DInventorViewer*, int ModNum);
    /// restores viewer settings when leaving editing mode
    virtual void unsetEditViewer(View3DInventorViewer*);
    //@}

    /** @name Task panel
     * With this interface the ViewProvider can steer the
     * appearance of widgets in the task view
     */
    //@{
    /// get a list of TaskBoxes associated with this object
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>&) const {}
    //@}

    /// is called when the provider is in edit and a key event occurs. Only ESC ends edit.
    virtual bool keyPressed(bool pressed, int key);
    /// Is called by the tree if the user double clicks on the object. It returns the string
    /// for the transaction that will be shown in the undo/redo dialog.
    /// If null is returned then no transaction will be opened.
    virtual const char* getTransactionText() const { return nullptr; }
    /// is called by the tree if the user double clicks on the object
    virtual bool doubleClicked() { return false; }
    /// is called when the provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s &cursorPos, View3DInventorViewer* viewer);
    /// is called when the Provider is in edit and the mouse is clicked
    virtual bool mouseButtonPressed(int button, bool pressed, const SbVec2s &cursorPos,
                                    const View3DInventorViewer* viewer);

    virtual bool mouseWheelEvent(int delta, const SbVec2s &cursorPos, const View3DInventorViewer* viewer);
    /// set up the context-menu with the supported edit modes
    virtual void setupContextMenu(QMenu*, QObject*, const char*);

    /** @name direct handling methods
     *  This group of methods is to direct influence the
     *  appearance of the viewed content. It's only for fast
     *  interactions! If you want to set the visual parameters
     *  you have to do it on the object viewed by this provider!
     */
    //@{
    /// set the viewing transformation of the provider
    virtual void setTransformation(const Base::Matrix4D &rcMatrix);
    virtual void setTransformation(const SbMatrix &rcMatrix);
    static SbMatrix convert(const Base::Matrix4D &rcMatrix);
    static Base::Matrix4D convert(const SbMatrix &sbMat);
    //@}

    virtual MDIView *getMDIView() const {
        return nullptr;
    }

public:
    // this method is called by the viewer when the ViewProvider is in edit
    static void eventCallback(void * ud, SoEventCallback * node);

    //restoring the object from document:
    //this may be of interest to extensions, hence call them
    void Restore(Base::XMLReader& reader) override;
    void Restore(Base::DocumentReader& reader,XERCES_CPP_NAMESPACE_QUALIFIER DOMElement *viewProviderEl);
    bool isRestoring() {return testStatus(Gui::isRestoring);}


    /** @name Display mask modes
     * Mainly controls an SoSwitch node which selects the display mask modes.
     * The number of display mask modes doesn't necessarily match with the number
     * of display modes.
     * E.g. various display modes like Gaussian curvature, mean curvature or gray
     * values are displayed by one display mask mode that handles color values.
     */
    //@{
    /// Adds a new display mask mode
    void addDisplayMaskMode( SoNode *node, const char* type );
    /// Activates the display mask mode \a type
    void setDisplayMaskMode( const char* type );
    /// Get the node to the display mask mode \a type
    SoNode* getDisplayMaskMode(const char* type) const;
    /// Returns a list of added display mask modes
    std::vector<std::string> getDisplayMaskModes() const;
    void setDefaultMode(int);
    int getDefaultMode() const;
    //@}

    virtual void setRenderCacheMode(int);

protected:
    /** Helper method to check that the node is valid, i.e. it must not cause
     * and infinite recursion.
     */
    bool checkRecursion(SoNode*);
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(const SbVec2s& pos,
                                 const View3DInventorViewer* viewer) const;
    /** Helper method to get picked entities while editing.
     * It's in the responsibility of the caller to delete the returned instance.
     */
    SoPickedPoint* getPointOnRay(const SbVec3f& pos, const SbVec3f& dir,
                                 const View3DInventorViewer* viewer) const;
    /// Reimplemented from subclass
    void onBeforeChange(const App::Property* prop) override;
    /// Reimplemented from subclass
    void onChanged(const App::Property* prop) override;

    /** @name Methods used by the Tree
     * If you want to take control over the
     * viewprovider specific overlay icons, that will be grayed out together
     * with the base icon, you can reimplement this method.
     */
    virtual QIcon mergeGreyableOverlayIcons (const QIcon & orig) const;

    /// Turn on mode switch
    virtual void setModeSwitch();

protected:
    /// The root Separator of the ViewProvider
    SoSeparator *pcRoot;
    /// this is transformation for the provider
    SoTransform *pcTransform;
    const char* sPixmap;
    /// this is the mode switch, all the different viewing modes are collected here
    SoSwitch    *pcModeSwitch;
    /// The root separator for annotations
    SoSeparator *pcAnnotation{nullptr};
    ViewProviderPy* pyViewObject{nullptr};
    std::string overrideMode;
    std::bitset<32> StatusBits;

private:
    int _iActualMode{-1};
    int _iEditMode{-1};
    int viewOverrideMode{-1};
    std::string _sCurrentMode;
    std::map<std::string, int> _sDisplayMaskModes;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_H
