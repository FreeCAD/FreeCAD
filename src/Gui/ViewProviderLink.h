/****************************************************************************
 *   Copyright (c) 2017 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#pragma once

#include <App/Link.h>
#include <unordered_map>
#include <map>
#include <string>
#include <vector>
#include <memory>

#include "Selection/SoFCUnifiedSelection.h"
#include "ViewProviderDocumentObject.h"
#include "ViewProviderExtension.h"
#include "ViewProviderFeaturePython.h"


class SoBase;
class SoDragger;
class SoMaterialBinding;
class SoPickStyle;

namespace Gui
{

class LinkInfo;
using LinkInfoPtr = boost::intrusive_ptr<LinkInfo>;

#if defined(_MSC_VER)
// forward declaration to please MSVC
void intrusive_ptr_add_ref(Gui::LinkInfo* px);
void intrusive_ptr_release(Gui::LinkInfo* px);
#endif

class GuiExport ViewProviderLinkObserver: public ViewProviderExtension
{
    EXTENSION_TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    ViewProviderLinkObserver();
    ~ViewProviderLinkObserver() override;
    void extensionReattach(App::DocumentObject*) override;
    void extensionBeforeDelete() override;
    void extensionOnChanged(const App::Property*) override;
    void extensionUpdateData(const App::Property*) override;
    void extensionFinishRestoring() override;
    bool extensionCanDragObject(App::DocumentObject*) const override
    {
        return false;
    }
    bool extensionCanDropObject(App::DocumentObject*) const override
    {
        return false;
    }
    void extensionModeSwitchChange() override;

    bool isLinkVisible() const;
    void setLinkVisible(bool);

    LinkInfoPtr linkInfo;
};

class GuiExport LinkOwner
{
public:
    virtual void unlink(LinkInfoPtr)
    {}
    virtual void onLinkedIconChange(LinkInfoPtr)
    {}
    virtual void onLinkedUpdateData(LinkInfoPtr, const App::Property*)
    {}

protected:
    virtual ~LinkOwner() = default;
};

class GuiExport LinkView: public Base::BaseClass, public LinkOwner
{
    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    LinkView();
    ~LinkView() override;
    LinkView& operator=(const LinkView&) = delete;
    LinkView(const LinkView&) = delete;

    PyObject* getPyObject() override;

    void unlink(LinkInfoPtr) override;
    void onLinkedIconChange(LinkInfoPtr) override;
    void onLinkedUpdateData(LinkInfoPtr, const App::Property*) override;

    bool isLinked() const;

    SoFCSelectionRoot* getLinkRoot() const
    {
        return pcLinkRoot;
    }

    QIcon getLinkedIcon(QPixmap overlay) const;

    void updateLink();

    void setLink(
        App::DocumentObject* obj,
        const std::vector<std::string>& subs = std::vector<std::string>()
    );

    void setLinkViewObject(
        ViewProviderDocumentObject* vpd,
        const std::vector<std::string>& subs = std::vector<std::string>()
    );

    std::vector<ViewProviderDocumentObject*> getChildren() const;

    void setMaterial(int index, const App::Material* material);
    void setDrawStyle(int linePattern, double lineWidth = 0, double pointSize = 0);
    void setTransform(int index, const Base::Matrix4D& mat);
    void renderDoubleSide(bool);
    void setSize(int size);

    int getSize() const
    {
        return nodeArray.size();
    }

    static void setTransform(SoTransform* pcTransform, const Base::Matrix4D& mat);

    enum SnapshotType
    {
        // three type of snapshot to override linked root node:

        // override transform and visibility
        SnapshotTransform = 0,
        // override visibility
        SnapshotVisible = 1,
        // override none (for child objects of a container)
        SnapshotChild = 2,

        SnapshotMax,

        // special type for sub object linking
        SnapshotContainer = -1,
        // sub object linking with transform override
        SnapshotContainerTransform = -2,
    };
    void setNodeType(SnapshotType type, bool sublink = true);

    void setChildren(
        const std::vector<App::DocumentObject*>& children,
        const boost::dynamic_bitset<>& vis,
        SnapshotType type = SnapshotVisible
    );

    bool linkGetDetailPath(const char*, SoFullPath*, SoDetail*&) const;
    bool linkGetElementPicked(const SoPickedPoint*, std::string&) const;

    void setElementVisible(int index, bool visible);
    bool isElementVisible(int index) const;

    ViewProviderDocumentObject* getOwner() const;
    void setOwner(ViewProviderDocumentObject* vpd);

    bool hasSubs() const;

    std::vector<std::string> getSubNames() const;
    ViewProviderDocumentObject* getLinkedView() const;

    Base::BoundBox3d getBoundBox(ViewProviderDocumentObject* vpd = nullptr) const;

    void setInvalid();

protected:
    void replaceLinkedRoot(SoSeparator*);
    void resetRoot();
    bool getGroupHierarchy(int index, SoFullPath* path) const;

protected:
    LinkInfoPtr linkOwner;
    LinkInfoPtr linkInfo;
    CoinPtr<SoFCSelectionRoot> pcLinkRoot;
    CoinPtr<SoTransform> pcTransform;
    CoinPtr<SoSeparator> pcLinkedRoot;
    CoinPtr<SoDrawStyle> pcDrawStyle;    // for override line width and point size
    CoinPtr<SoShapeHints> pcShapeHints;  // for override double side rendering for mirror
    SnapshotType nodeType;
    SnapshotType childType;
    bool autoSubLink;  // auto delegate to linked sub object if there is only one sub object

    class SubInfo;
    friend class SubInfo;
    std::map<std::string, std::unique_ptr<SubInfo>> subInfo;

    class Element;
    std::vector<std::unique_ptr<Element>> nodeArray;
    std::unordered_map<SoNode*, int> nodeMap;

    Py::Object PythonObject;
};

class GuiExport ViewProviderLink: public ViewProviderDragger
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderLink);
    using inherited = ViewProviderDragger;

public:
    App::PropertyBool OverrideMaterial;
    App::PropertyMaterial ShapeMaterial;
    App::PropertyEnumeration DrawStyle;
    App::PropertyFloatConstraint LineWidth;
    App::PropertyFloatConstraint PointSize;
    App::PropertyMaterialList MaterialList;
    App::PropertyBoolList OverrideMaterialList;
    App::PropertyBool Selectable;
    App::PropertyColorList OverrideColorList;
    App::PropertyPersistentObject ChildViewProvider;

    ViewProviderLink();
    ~ViewProviderLink() override;

    void attach(App::DocumentObject* pcObj) override;
    void reattach(App::DocumentObject* pcObj) override;

    bool isSelectable() const override;

    bool useNewSelectionModel() const override
    {
        return true;
    }

    void updateData(const App::Property*) override;
    void onChanged(const App::Property* prop) override;
    std::vector<App::DocumentObject*> claimChildren() const override;
    bool getElementPicked(const SoPickedPoint*, std::string&) const override;
    bool getDetailPath(const char*, SoFullPath*, bool, SoDetail*&) const override;

    void finishRestoring() override;

    QIcon getIcon() const override;

    bool canDragObjects() const override;
    bool canDragObject(App::DocumentObject*) const override;
    void dragObject(App::DocumentObject*) override;
    bool canDropObjects() const override;
    bool canDragAndDropObject(App::DocumentObject*) const override;
    bool canDropObjectEx(
        App::DocumentObject* obj,
        App::DocumentObject* owner,
        const char* subname,
        const std::vector<std::string>& subElements
    ) const override;
    std::string dropObjectEx(
        App::DocumentObject*,
        App::DocumentObject*,
        const char* subname,
        const std::vector<std::string>& subElements
    ) override;

    bool onDelete(const std::vector<std::string>&) override;
    bool canDelete(App::DocumentObject* obj) const override;

    std::vector<std::string> getDisplayModes() const override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;

    virtual QPixmap getOverlayPixmap() const;

    ViewProvider* startEditing(int ModNum) override;
    bool doubleClicked() override;

    PyObject* getPyObject() override;
    PyObject* getPyLinkView();

    static void updateLinks(ViewProvider* vp);

    std::map<std::string, Base::Color> getElementColors(const char* subname = nullptr) const override;
    void setElementColors(const std::map<std::string, Base::Color>& colors) override;

    void setOverrideMode(const std::string& mode) override;

    void onBeforeChange(const App::Property*) override;
    ViewProviderDocumentObject* getChildViewProvider() const
    {
        return childVp;
    }

    App::Property* getPropertyByName(const char* name) const override;
    void getPropertyMap(std::map<std::string, App::Property*>& Map) const override;
    /// See PropertyContainer::visitProperties for semantics
    void visitProperties(const std::function<void(App::Property*)>& visitor) const override;
    void getPropertyList(std::vector<App::Property*>& List) const override;

    ViewProviderDocumentObject* getLinkedViewProvider(
        std::string* subname = nullptr,
        bool recursive = false
    ) const override;

    bool allowOverride(const App::DocumentObject&) const override
    {
        return true;
    }

    void setTransformation(const Base::Matrix4D& rcMatrix) override;
    void setTransformation(const SbMatrix& rcMatrix) override;

protected:
    bool setEdit(int ModNum) override;
    void setEditViewer(View3DInventorViewer*, int ModNum) override;
    void unsetEditViewer(View3DInventorViewer*) override;
    bool linkEdit(const App::LinkBaseExtension* ext = nullptr) const;
    void _setupContextMenu(App::LinkBaseExtension* ext, QMenu*, QObject*, const char*);

    enum LinkType
    {
        LinkTypeNone,
        LinkTypeNormal,
        LinkTypeSubs,
    };

    bool hasElements(const App::LinkBaseExtension* ext = nullptr) const;
    bool isGroup(const App::LinkBaseExtension* ext = nullptr, bool plainGroup = false) const;
    const App::LinkBaseExtension* getLinkExtension() const;
    App::LinkBaseExtension* getLinkExtension();

    void updateDataPrivate(App::LinkBaseExtension* ext, const App::Property*);
    void updateElementList(App::LinkBaseExtension* ext);

    bool setLinkType(App::LinkBaseExtension* ext);

    void onChangeIcon() const;
    std::vector<App::DocumentObject*> claimChildrenPrivate() const;

    void applyMaterial();
    void applyColors();

    void checkIcon(const App::LinkBaseExtension* ext = nullptr);

    ViewProvider* getLinkedView(bool real, const App::LinkBaseExtension* ext = nullptr) const;

    bool initDraggingPlacement();
    bool callDraggerProxy(const char* fname);

private:
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragFinishCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

protected:
    LinkView* linkView;
    LinkType linkType;
    bool hasSubName;
    bool hasSubElement;

    struct DraggerContext
    {
        Base::Matrix4D preTransform;
        Base::Placement initialPlacement;
        Base::Matrix4D mat;
        Base::BoundBox3d bbox;
        bool cmdPending;
    };
    std::unique_ptr<DraggerContext> dragCtx;
    ViewProviderDocumentObject* childVp;
    LinkInfoPtr childVpLink;
    mutable qint64 overlayCacheKey;

private:
    SoPickStyle* pcPickStyle {nullptr};
};

using ViewProviderLinkPython = ViewProviderFeaturePythonT<ViewProviderLink>;

}  // namespace Gui
