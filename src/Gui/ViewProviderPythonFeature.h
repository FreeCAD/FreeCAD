/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_VIEWPROVIDERPYTHONFEATURE_H
#define GUI_VIEWPROVIDERPYTHONFEATURE_H

#include <App/Application.h>
#include <App/AutoTransaction.h>
#include <Gui/ViewProviderGeometryObject.h>
#include <Gui/Document.h>
#include <App/PropertyPythonObject.h>
#include <App/DynamicProperty.h>
#include <App/FeaturePython.h>

class SoSensor;
class SoDragger;
class SoNode;

namespace Gui {

class GuiExport ViewProviderPythonFeatureImp
{
public:
    enum ValueT {
        NotImplemented = 0, // not handled
        Accepted = 1, // handled and accepted
        Rejected = 2  // handled and rejected
    };

    /// constructor.
    ViewProviderPythonFeatureImp(ViewProviderDocumentObject*, App::PropertyPythonObject &);
    /// destructor.
    ~ViewProviderPythonFeatureImp();

    // Returns the icon
    QIcon getIcon() const;
    std::vector<App::DocumentObject*> claimChildren(std::vector<App::DocumentObject*>&&) const;
    bool useNewSelectionModel() const;
    ValueT getElementPicked(const SoPickedPoint *pp, std::string &subname) const;
    std::string getElement(const SoDetail *det) const;
    SoDetail* getDetail(const char*) const;
    ValueT getDetailPath(const char *name, SoFullPath *path, bool append, SoDetail *&det) const;
    std::vector<Base::Vector3d> getSelectionShape(const char* Element) const;
    ValueT setEdit(int ModNum);
    ValueT unsetEdit(int ModNum);
    bool setEditViewer(View3DInventorViewer*, int ModNum);
    bool unsetEditViewer(View3DInventorViewer*);
    ValueT doubleClicked(void);
    void setupContextMenu(QMenu* menu);

    /** @name Update data methods*/
    //@{
    void attach(App::DocumentObject *pcObject);
    void updateData(const App::Property*);
    void onChanged(const App::Property* prop);
    void startRestoring();
    void finishRestoring();
    bool onDelete(const std::vector<std::string> & sub);
    ValueT canDelete(App::DocumentObject *obj) const;
    //@}

    /** @name Display methods */
    //@{
    /// Returns true if the icon must always appear enabled in the tree view
    bool isShow() const;
    /// get the default display mode
    const char* getDefaultDisplayMode() const;
    /// returns a list of all possible modes
    std::vector<std::string> getDisplayModes(void) const;
    /// set the display mode
    std::string setDisplayMode(const char* ModeName);
    //@}

    ValueT canRemoveChildrenFromRoot() const;

    /** @name Drag and drop */
    //@{
    /// Returns true if the view provider generally supports dragging objects
    ValueT canDragObjects() const;
    /// Check whether the object can be removed from the view provider by drag and drop
    ValueT canDragObject(App::DocumentObject*) const;
    /// Starts to drag the object
    ValueT dragObject(App::DocumentObject*);
    /// Returns true if the view provider generally accepts dropping of objects
    ValueT canDropObjects() const;
    /// Check whether the object can be dropped to the view provider by drag and drop
    ValueT canDropObject(App::DocumentObject*) const;
    /// If the dropped object type is accepted the object will be added as child
    ValueT dropObject(App::DocumentObject*);
    /** Return false to force drop only operation for a give object*/
    ValueT canDragAndDropObject(App::DocumentObject*) const;
    /** Query object dropping with full quanlified name */
    ValueT canDropObjectEx(App::DocumentObject *obj, App::DocumentObject *, 
            const char *,const std::vector<std::string> &elements) const;
    /** Add an object with full quanlified name to the view provider by drag and drop */
    ValueT dropObjectEx(App::DocumentObject *obj, App::DocumentObject *,
            const char *, const std::vector<std::string> &elements, std::string &ret);
    ValueT replaceObject(App::DocumentObject *, App::DocumentObject *);
    //@}

    ViewProviderDocumentObject *getLinkedViewProvider(bool recursive) const;

    bool canAddToSceneGraph() const;

    bool getDropPrefix(std::string &prefix) const;

private:
    ViewProviderDocumentObject* object;
    App::PropertyPythonObject &Proxy;
    bool has__object__;

#define FC_PY_VIEW_OBJECT \
    FC_PY_ELEMENT(getIcon) \
    FC_PY_ELEMENT(claimChildren) \
    FC_PY_ELEMENT(useNewSelectionModel) \
    FC_PY_ELEMENT(getElementPicked) \
    FC_PY_ELEMENT(getElement) \
    FC_PY_ELEMENT(getDetail) \
    FC_PY_ELEMENT(getDetailPath) \
    FC_PY_ELEMENT(getSelectionShape) \
    FC_PY_ELEMENT(setEdit) \
    FC_PY_ELEMENT(unsetEdit) \
    FC_PY_ELEMENT(setEditViewer) \
    FC_PY_ELEMENT(unsetEditViewer) \
    FC_PY_ELEMENT(doubleClicked) \
    FC_PY_ELEMENT(setupContextMenu) \
    FC_PY_ELEMENT(attach) \
    FC_PY_ELEMENT(updateData) \
    FC_PY_ELEMENT(onChanged) \
    FC_PY_ELEMENT(startRestoring) \
    FC_PY_ELEMENT(finishRestoring) \
    FC_PY_ELEMENT(onDelete) \
    FC_PY_ELEMENT(canDelete) \
    FC_PY_ELEMENT(isShow) \
    FC_PY_ELEMENT(getDefaultDisplayMode) \
    FC_PY_ELEMENT(getDisplayModes) \
    FC_PY_ELEMENT(setDisplayMode) \
    FC_PY_ELEMENT(canRemoveChildrenFromRoot) \
    FC_PY_ELEMENT(canDragObjects) \
    FC_PY_ELEMENT(canDragObject) \
    FC_PY_ELEMENT(dragObject) \
    FC_PY_ELEMENT(canDropObjects) \
    FC_PY_ELEMENT(canDropObject) \
    FC_PY_ELEMENT(dropObject) \
    FC_PY_ELEMENT(canDragAndDropObject) \
    FC_PY_ELEMENT(canDropObjectEx) \
    FC_PY_ELEMENT(dropObjectEx) \
    FC_PY_ELEMENT(canAddToSceneGraph) \
    FC_PY_ELEMENT(getDropPrefix) \
    FC_PY_ELEMENT(replaceObject) \
    FC_PY_ELEMENT(getLinkedViewProvider) \

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_ELEMENT_DEFINE(_name)

    FC_PY_VIEW_OBJECT

#undef FC_PY_ELEMENT
#define FC_PY_ELEMENT(_name) FC_PY_ELEMENT_FLAG(_name)

    enum Flag {
        FC_PY_VIEW_OBJECT
        FlagMax,
    };
    typedef std::bitset<FlagMax> Flags;
    mutable Flags _Flags;

public:
    void init(PyObject *pyobj);
};

template <class ViewProviderT>
class ViewProviderPythonFeatureT : public ViewProviderT
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderPythonFeatureT<ViewProviderT>);

public:
    /// constructor.
    ViewProviderPythonFeatureT() : _attached(false) {
        ADD_PROPERTY(Proxy,(Py::Object()));
        imp = new ViewProviderPythonFeatureImp(this,Proxy);
    }
    /// destructor.
    virtual ~ViewProviderPythonFeatureT() {
        delete imp;
    }

    // Returns the icon
    QIcon getIcon() const override {
        QIcon icon = imp->getIcon();
        if (icon.isNull())
            icon = ViewProviderT::getIcon();
        return icon;
    }

    std::vector<App::DocumentObject*> claimChildren() const override {
        return imp->claimChildren(ViewProviderT::claimChildren());
    }

    /** @name Nodes */
    //@{
    virtual SoSeparator* getRoot() override {
        return ViewProviderT::getRoot();
    }
    virtual SoSeparator* getFrontRoot() const override {
        return ViewProviderT::getFrontRoot();
    }
    // returns the root node of the Provider (3D)
    virtual SoSeparator* getBackRoot() const override {
        return ViewProviderT::getBackRoot();
    }
    //@}

    /** @name Selection handling */
    //@{
    virtual bool useNewSelectionModel() const override {
        return imp->useNewSelectionModel();
    }
    virtual bool getElementPicked(const SoPickedPoint *pp, std::string &subname) const override {
        auto ret = imp->getElementPicked(pp,subname);
        if(ret == ViewProviderPythonFeatureImp::NotImplemented)
            return ViewProviderT::getElementPicked(pp,subname);
        else if(ret == ViewProviderPythonFeatureImp::Accepted)
            return true;
        return false;
    }
    virtual std::string getElement(const SoDetail *det) const override {
        std::string name = imp->getElement(det);
        if (!name.empty()) return name;
        return ViewProviderT::getElement(det);
    }
    virtual SoDetail* getDetail(const char* name) const override {
        SoDetail* det = imp->getDetail(name);
        if (det) return det;
        return ViewProviderT::getDetail(name);
    }
    virtual bool getDetailPath(const char *name, SoFullPath *path, bool append,SoDetail *&det) const override {
        auto ret = imp->getDetailPath(name,path,append,det);
        if(ret == ViewProviderPythonFeatureImp::NotImplemented)
            return ViewProviderT::getDetailPath(name,path,append,det);
        return ret == ViewProviderPythonFeatureImp::Accepted;
    }
    virtual std::vector<Base::Vector3d> getSelectionShape(const char* Element) const override {
        return ViewProviderT::getSelectionShape(Element);
    };
    //@}

    /** @name Update data methods*/
    //@{
    virtual void attach(App::DocumentObject *obj) override {
        // delay loading of the actual attach() method because the Python
        // view provider class is not attached yet
        ViewProviderT::pcObject = obj;
    }
    virtual void updateData(const App::Property* prop) override {
        imp->updateData(prop);
        ViewProviderT::updateData(prop);
    }
    virtual void getTaskViewContent(std::vector<Gui::TaskView::TaskContent*>& c) const override {
        ViewProviderT::getTaskViewContent(c);
    }
    virtual bool onDelete(const std::vector<std::string> & sub) override {
        bool ok = imp->onDelete(sub);
        if (!ok) return ok;
        return ViewProviderT::onDelete(sub);
    }
    virtual bool canDelete(App::DocumentObject *obj) const override {
        switch(imp->canDelete(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDelete(obj);
        }
    }
    //@}

    /** @name Restoring view provider from document load */
    //@{
    virtual void startRestoring() override {
        ViewProviderT::startRestoring();
        imp->startRestoring();
    }
    virtual void finishRestoring() override {
        imp->finishRestoring();
        ViewProviderT::finishRestoring();
    }
    //@}

    /** @name Drag and drop */
    //@{
    /// Returns true if the view provider generally supports dragging objects
    virtual bool canDragObjects() const override {
        switch (imp->canDragObjects()) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDragObjects();
        }
    }
    /// Check whether the object can be removed from the view provider by drag and drop
    virtual bool canDragObject(App::DocumentObject* obj) const override {
        switch (imp->canDragObject(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDragObject(obj);
        }
    }
    /// Starts to drag the object
    virtual void dragObject(App::DocumentObject* obj) override {
        App::AutoTransaction committer;
        switch (imp->dragObject(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
        case ViewProviderPythonFeatureImp::Rejected:
            return;
        default:
            return ViewProviderT::dragObject(obj);
        }
    }
    /// Returns true if the view provider generally accepts dropping of objects
    virtual bool canDropObjects() const override {
        switch (imp->canDropObjects()) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDropObjects();
        }
    }
    /// Check whether the object can be dropped to the view provider by drag and drop
    virtual bool canDropObject(App::DocumentObject* obj) const override {
        switch (imp->canDropObject(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDropObject(obj);
        }
    }
    /// If the dropped object type is accepted the object will be added as child
    virtual void dropObject(App::DocumentObject* obj) override {
        App::AutoTransaction committer;
        switch (imp->dropObject(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
        case ViewProviderPythonFeatureImp::Rejected:
            return;
        default:
            return ViewProviderT::dropObject(obj);
        }
    }
    /** Return false to force drop only operation for a give object*/
    virtual bool canDragAndDropObject(App::DocumentObject *obj) const override {
        switch (imp->canDragAndDropObject(obj)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDragAndDropObject(obj);
        }
    }
    virtual bool canDropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner, 
            const char *subname, const std::vector<std::string> &elements) const override
    {
        switch (imp->canDropObjectEx(obj,owner,subname,elements)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::canDropObjectEx(obj,owner,subname,elements);
        }
    }
    /** Add an object with full quanlified name to the view provider by drag and drop */
    virtual std::string dropObjectEx(App::DocumentObject *obj, App::DocumentObject *owner, 
            const char *subname, const std::vector<std::string> &elements) override
    {
        App::AutoTransaction committer;
        std::string ret;
        switch (imp->dropObjectEx(obj,owner,subname,elements,ret)) {
        case ViewProviderPythonFeatureImp::NotImplemented:
            ret = ViewProviderT::dropObjectEx(obj,owner,subname,elements);
            break;
        default:
            break;
        }
        return ret;
    }
    //@}

    /** @name Display methods */
    //@{
    /// Returns true if the icon must always appear enabled in the tree view
    virtual bool isShow() const override {
        bool ok = imp->isShow();
        if (ok) return ok;
        return ViewProviderT::isShow();
    }
    /// get the default display mode
    virtual const char* getDefaultDisplayMode() const override {
        return imp->getDefaultDisplayMode();
    }
    /// returns a list of all possible modes
    virtual std::vector<std::string> getDisplayModes(void) const override {
        std::vector<std::string> modes = ViewProviderT::getDisplayModes();
        std::vector<std::string> more_modes = imp->getDisplayModes();
        modes.insert(modes.end(), more_modes.begin(), more_modes.end());
        return modes;
    }
    /// set the display mode
    virtual void setDisplayMode(const char* ModeName) override {
        std::string mask = imp->setDisplayMode(ModeName);
        ViewProviderT::setDisplayMaskMode(mask.c_str());
        ViewProviderT::setDisplayMode(ModeName);
    }
    //@}

    virtual bool canRemoveChildrenFromRoot() const override {
        switch(imp->canRemoveChildrenFromRoot()) {
        case ViewProviderPythonFeatureImp::NotImplemented:
            return ViewProviderT::canRemoveChildrenFromRoot();
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        default:
            return false;
        }
    }

    PyObject* getPyObject() override {
        return ViewProviderT::getPyObject();
    }

    virtual bool canAddToSceneGraph() const override {
        return ViewProviderT::canAddToSceneGraph() && imp->canAddToSceneGraph();
    }

protected:
    virtual void onChanged(const App::Property* prop) override {
        if (prop == &Proxy) {
            imp->init(Proxy.getValue().ptr());
            if (ViewProviderT::pcObject && !Proxy.getValue().is(Py::_None())) {
                if (!_attached) {
                    _attached = true;
                    imp->attach(ViewProviderT::pcObject);
                    ViewProviderT::attach(ViewProviderT::pcObject);
                    // needed to load the right display mode after they're known now
                    ViewProviderT::DisplayMode.touch();
                    ViewProviderT::setOverrideMode(viewerMode);
                }
                if(!this->testStatus(Gui::isRestoring) && 
                    ViewProviderT::canAddToSceneGraph() && 
                    !imp->canAddToSceneGraph())
                {
                    this->getDocument()->toggleInSceneGraph(this);
                }
                ViewProviderT::updateView();
            }
        }
        else {
            imp->onChanged(prop);
            ViewProviderT::onChanged(prop);
        }
    }
    /// is called by the document when the provider goes in edit mode
    virtual bool setEdit(int ModNum) override
    {
        switch (imp->setEdit(ModNum)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::setEdit(ModNum);
        }
    }
    /// is called when you lose the edit mode
    virtual void unsetEdit(int ModNum) override
    {
        switch (imp->unsetEdit(ModNum)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return;
        case ViewProviderPythonFeatureImp::Rejected:
        default:
            return ViewProviderT::unsetEdit(ModNum);
        }
    }
    virtual void setEditViewer(View3DInventorViewer *viewer, int ModNum) override {
        if(!imp->setEditViewer(viewer,ModNum))
            ViewProviderT::setEditViewer(viewer,ModNum);
    }
    virtual void unsetEditViewer(View3DInventorViewer *viewer) override {
        if(!imp->unsetEditViewer(viewer))
            ViewProviderT::unsetEditViewer(viewer);
    }

    virtual std::string getDropPrefix() const override {
        std::string prefix;
        if(!imp->getDropPrefix(prefix))
            return ViewProviderT::getDropPrefix();
        return prefix;
    }

    virtual int replaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj) override {
        App::AutoTransaction committer;
        switch (imp->replaceObject(oldObj,newObj)) {
        case ViewProviderPythonFeatureImp::Accepted:
            return 1;
        case ViewProviderPythonFeatureImp::Rejected:
            return 0;
        default:
            return ViewProviderT::replaceObject(oldObj,newObj);
        }
    }

//FIXME: ViewProviderPythonFeatureT hides overloaded virtual functions
#if defined(__clang__)
# pragma clang diagnostic push
# pragma clang diagnostic ignored "-Woverloaded-virtual"
#endif

    virtual ViewProviderDocumentObject *getLinkedViewProvider(bool recursive=false) const {
        auto res = imp->getLinkedViewProvider(recursive);
        if(!res)
            res = ViewProviderT::getLinkedViewProvider();
        return res;
    }

#if defined(__clang__)
# pragma clang diagnostic pop
#endif

public:
    virtual void setupContextMenu(QMenu* menu, QObject* recipient, const char* member) override
    {
        ViewProviderT::setupContextMenu(menu, recipient, member);
        imp->setupContextMenu(menu);
    }

protected:
    virtual bool doubleClicked(void) override
    {
        App::AutoTransaction committer;
        switch (imp->doubleClicked()) {
        case ViewProviderPythonFeatureImp::Accepted:
            return true;
        case ViewProviderPythonFeatureImp::Rejected:
            return false;
        default:
            return ViewProviderT::doubleClicked();
        }
    }
    virtual void setOverrideMode(const std::string &mode) override
    {
        ViewProviderT::setOverrideMode(mode);
        viewerMode = mode;
    }

private:
    ViewProviderPythonFeatureImp* imp;
    App::PropertyPythonObject Proxy;
    std::string viewerMode;
    bool _attached;
};

// Special Feature-Python classes
typedef ViewProviderPythonFeatureT<ViewProviderDocumentObject> ViewProviderPythonFeature;
typedef ViewProviderPythonFeatureT<ViewProviderGeometryObject> ViewProviderPythonGeometry;

} // namespace Gui

#endif // GUI_VIEWPROVIDERPYTHONFEATURE_H

