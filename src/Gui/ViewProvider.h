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

#include <map>
#include <vector>
#include <string>
#include <bitset>
#include <QIcon>
#include <boost/signals2.hpp>

#include <App/TransactionalObject.h>
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

enum ViewStatus {
    UpdateData = 0,
    Detach = 1,
    isRestoring = 2,
    UpdatingView = 3,
};



/** General interface for all visual stuff in FreeCAD
  * This class is used to generate and handle all around 
  * visualizing and presenting objects from the FreeCAD 
  * App layer to the user. This class and its descendents 
  * have to be implemented for any object type in order to 
  * show them in the 3DView and TreeView.
  */
class GuiExport ViewProvider : public App::TransactionalObject
{
    PROPERTY_HEADER(Gui::ViewProvider);

public:
    /// constructor.
    ViewProvider();

    /// destructor.
    virtual ~ViewProvider();

    // returns the root node of the Provider (3D)
    virtual SoSeparator* getRoot(void){return pcRoot;}
    // returns the root for the Annotations. 
    SoSeparator* getAnnotation(void);
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getFrontRoot(void) const;
    // returns the root node where the children gets collected(3D)
    virtual SoGroup* getChildRoot(void) const;
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getBackRoot(void) const;
    /** deliver the children belonging to this object
      * this method is used to deliver the objects to 
      * the 3DView which should be grouped under its 
      * scene graph. This affects the visibility and the 3D 
      * position of the object. 
      */
    virtual std::vector<App::DocumentObject*> claimChildren3D(void) const;

    /** @name Selection handling
      * This group of methods do the selection handling.
      * Here you can define how the selection for your ViewProfider
      * works. 
     */
    //@{

    /// indicates if the ViewProvider use the new Selection model
    virtual bool useNewSelectionModel(void) const {return false;}
    /// indicates if the ViewProvider can be selected
    virtual bool isSelectable(void) const {return true;}
    /// return a hit element to the selection path or 0
    virtual std::string getElement(const SoDetail *) const { return std::string(); }
    virtual SoDetail* getDetail(const char*) const { return 0; }
    virtual std::vector<Base::Vector3d> getModelPoints(const SoPickedPoint *) const;
    /// return the highlight lines for a given element or the whole shape
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const {
        (void)Element;
        return std::vector<Base::Vector3d>();
    }
    /**
     * Get called if the object is about to get deleted.
     * Here you can delete other objects, switch their visibility or prevent the deletion of the object.
     * @param subNames  list of selected subelements
     * @return          true if the deletion is approved by the view provider.
     */
    virtual bool onDelete(const std::vector<std::string> &subNames);
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
      * can reimplemnt these methods.
     */
    //@{
    /// deliver the icon shown in the tree view
    virtual QIcon getIcon(void) const;
    /** deliver the children belonging to this object
      * this method is used to deliver the objects to 
      * the tree framework which should be grouped under its 
      * label. Obvious is the usage in the group but it can
      * be used for any kind of grouping needed for a special 
      * purpose.
      */
    virtual std::vector<App::DocumentObject*> claimChildren(void) const;
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
    /** Tell the tree view if this object should appear there */
    virtual bool showInTree() const
    {
      return true;
    }
    /** Remove a child from the view provider by drag and drop */
    virtual void dragObject(App::DocumentObject*);
    /** Check whether objects can be added to the view provider by drag and drop */
    virtual bool canDropObjects() const;
    /** Check whether the object can be dropped to the view provider by drag and drop */
    virtual bool canDropObject(App::DocumentObject*) const;
    /** Add an object to the view provider by drag and drop */
    virtual void dropObject(App::DocumentObject*);
    /** Replace an object to the view provider by drag and drop */
    virtual void replaceObject(App::DocumentObject*, App::DocumentObject*);
    //@}

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
    void update(const App::Property*);
    virtual void updateData(const App::Property*);
    bool isUpdatesEnabled () const;
    void setUpdatesEnabled (bool enable);

    /// return the status bits
    unsigned long getStatus() const {return StatusBits.to_ulong();}
    bool testStatus(ViewStatus pos) const {return StatusBits.test((size_t)pos);}
    void setStatus(ViewStatus pos, bool on) {StatusBits.set((size_t)pos, on);}

    std::string toString() const;
    PyObject* getPyObject();

    /** @name Display mode methods 
     */
    //@{
    std::string getActiveDisplayMode(void) const;
    /// set the display mode
    virtual void setDisplayMode(const char* ModeName);
    /// get the default display mode
    virtual const char* getDefaultDisplayMode() const;
    /// returns a list of all possible display modes
    virtual std::vector<std::string> getDisplayModes(void) const;
    /// Hides the view provider
    virtual void hide(void);
    /// Shows the view provider
    virtual void show(void);
    /// checks whether the view provider is visible or not
    virtual bool isShow(void) const;
    void setVisible(bool);
    bool isVisible() const;
    /// Overrides the display mode with mode.
    virtual void setOverrideMode(const std::string &mode);
    const std::string getOverrideMode();
    //@}


    /** @name Edit methods
     * if the Viewprovider goes in edit mode
     * you can handle most of the events in the viewer by yourself
     */
    //@{
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
    bool startEditing(int ModNum = 0);
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
    /// is called by the tree if the user double click on the object
    virtual bool doubleClicked(void) { return false; }
    /// is called when the provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s &cursorPos, View3DInventorViewer* viewer);
    /// is called when the Provider is in edit and the mouse is clicked 
    virtual bool mouseButtonPressed(int button, bool pressed, const SbVec2s &cursorPos,
                                    const View3DInventorViewer* viewer);
    /// set up the context-menu with the supported edit modes
    virtual void setupContextMenu(QMenu*, QObject*, const char*) {}

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
    SbMatrix convert(const Base::Matrix4D &rcMatrix) const;
    //@}

public:
    // this method is called by the viewer when the ViewProvider is in edit
    static void eventCallback(void * ud, SoEventCallback * node);
    
    //restoring the object from document: this may itnerest extensions, hence call them
    virtual void Restore(Base::XMLReader& reader);
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
    //@}
    
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
    void onChanged(const App::Property* prop);

protected:
    /// The root Separator of the ViewProvider
    SoSeparator *pcRoot;
    /// this is transformation for the provider
    SoTransform *pcTransform;
    const char* sPixmap;
    /// this is the mode switch, all the different viewing modes are collected here
    SoSwitch    *pcModeSwitch;
    /// The root separator for annotations
    SoSeparator *pcAnnotation;
    ViewProviderPy* pyViewObject;
    std::string overrideMode;
    std::bitset<32> StatusBits;

private:
    void setModeSwitch();
    int _iActualMode;
    int _iEditMode;
    int viewOverrideMode;
    std::string _sCurrentMode;
    std::map<std::string, int> _sDisplayMaskModes;
};

} // namespace Gui

#endif // GUI_VIEWPROVIDER_H

