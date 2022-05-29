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
# include <boost_bind_bind.hpp>
# include <QAction>
# include <QActionGroup>
# include <QApplication>
# include <QContextMenuEvent>
# include <QCursor>
# include <QHeaderView>
# include <QMenu>
# include <QMessageBox>
# include <QPixmap>
# include <QTimer>
# include <QToolTip>
# include <QVBoxLayout>
#endif

#include <Base/Console.h>
#include <Base/Reader.h>
#include <Base/Sequencer.h>
#include <Base/Tools.h>
#include <Base/Writer.h>

#include <App/Document.h>
#include <App/DocumentObjectGroup.h>
#include <App/AutoTransaction.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Link.h>

#include "Tree.h"
#include "BitmapFactory.h"
#include "Command.h"
#include "Document.h"
#include "ExpressionCompleter.h"
#include "Macro.h"
#include "MainWindow.h"
#include "MenuManager.h"
#include "View3DInventor.h"
#include "ViewProviderDocumentObject.h"
#include "Widgets.h"
#include "Workbench.h"


FC_LOG_LEVEL_INIT("Tree", false, true, true)

#define _TREE_PRINT(_level,_func,_msg) \
    _FC_PRINT(FC_LOG_INSTANCE,_level,_func, '['<<getTreeName()<<"] " << _msg)
#define TREE_MSG(_msg) _TREE_PRINT(FC_LOGLEVEL_MSG,NotifyMessage,_msg)
#define TREE_WARN(_msg) _TREE_PRINT(FC_LOGLEVEL_WARN,NotifyWarning,_msg)
#define TREE_ERR(_msg) _TREE_PRINT(FC_LOGLEVEL_ERR,NotifyError,_msg)
#define TREE_LOG(_msg) _TREE_PRINT(FC_LOGLEVEL_LOG,NotifyLog,_msg)
#define TREE_TRACE(_msg) _TREE_PRINT(FC_LOGLEVEL_TRACE,NotifyLog,_msg)

using namespace Gui;
namespace bp = boost::placeholders;

/////////////////////////////////////////////////////////////////////////////////

std::unique_ptr<QPixmap>  TreeWidget::documentPixmap;
std::unique_ptr<QPixmap>  TreeWidget::documentPartialPixmap;
std::set<TreeWidget*> TreeWidget::Instances;
static TreeWidget* _LastSelectedTreeWidget;
const int TreeWidget::DocumentType = 1000;
const int TreeWidget::ObjectType = 1001;
bool _DragEventFilter;

TreeParams::TreeParams() {
    handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    handle->Attach(this);


#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF(_name,_type,_Type,_default) \
    _##_name = handle->Get##_Type(#_name,_default);

    FC_TREEPARAM_DEFS
}


#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF(_name,_type,_Type,_default) \
void TreeParams::set##_name(_type value) {\
    if(_##_name != value) {\
        handle->Set##_Type(#_name,value);\
    }\
}

FC_TREEPARAM_DEFS

void TreeParams::OnChange(Base::Subject<const char*>&, const char* sReason) {

#undef FC_TREEPARAM_DEF
#define FC_TREEPARAM_DEF(_name,_type,_Type,_default) \
    if(strcmp(sReason,#_name)==0) {\
        _##_name = handle->Get##_Type(#_name,_default);\
        return;\
    }

#undef FC_TREEPARAM_DEF2
#define FC_TREEPARAM_DEF2(_name,_type,_Type,_default) \
    if(strcmp(sReason,#_name)==0) {\
        _##_name = handle->Get##_Type(#_name,_default);\
        on##_name##Changed();\
        return;\
    }

    FC_TREEPARAM_DEFS
}

void TreeParams::onSyncSelectionChanged() {
    if (!TreeParams::Instance()->SyncSelection() || !Gui::Selection().hasSelection())
        return;
    TreeWidget::scrollItemToTop();
}

void TreeParams::onCheckBoxesSelectionChanged()
{
    TreeWidget::instance()->synchronizeSelectionCheckBoxes();
}

void TreeParams::onDocumentModeChanged() {
    App::GetApplication().setActiveDocument(App::GetApplication().getActiveDocument());
}

TreeParams* TreeParams::Instance() {
    static TreeParams* instance;
    if (!instance)
        instance = new TreeParams;
    return instance;
}

bool TreeParams::getTreeViewStretchDescription() const
{
    return handle->GetBool("TreeViewStretchDescription", false);
}

//////////////////////////////////////////////////////////////////////////////////////
struct Stats {
#define DEFINE_STATS \
    DEFINE_STAT(testStatus1) \
    DEFINE_STAT(testStatus2) \
    DEFINE_STAT(testStatus3) \
    DEFINE_STAT(getIcon) \
    DEFINE_STAT(setIcon) \

#define DEFINE_STAT(_name) \
    FC_DURATION_DECLARE(_name);\
    int _name##_count;

    DEFINE_STATS

        void init() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
        FC_DURATION_INIT(_name);\
        _name##_count = 0;

        DEFINE_STATS
    }

    void print() {
#undef DEFINE_STAT
#define DEFINE_STAT(_name) FC_DURATION_MSG(_name, #_name " count: " << _name##_count);
        DEFINE_STATS
    }

#undef DEFINE_STAT
#define DEFINE_STAT(_name) \
    void time_##_name(FC_TIME_POINT &t) {\
        ++_name##_count;\
        FC_DURATION_PLUS(_name,t);\
    }

    DEFINE_STATS
};

//static Stats _Stats;

struct TimingInfo {
    bool timed = false;
    FC_TIME_POINT t;
    FC_DURATION& d;
    TimingInfo(FC_DURATION& d)
        :d(d)
    {
        _FC_TIME_INIT(t);
    }
    ~TimingInfo() {
        stop();
    }
    void stop() {
        if (!timed) {
            timed = true;
            FC_DURATION_PLUS(d, t);
        }
    }
    void reset() {
        stop();
        _FC_TIME_INIT(t);
    }
};

// #define DO_TIMING
#ifdef DO_TIMING
#define _Timing(_idx,_name) ++_Stats._name##_count; TimingInfo _tt##_idx(_Stats._name)
#define Timing(_name) _Timing(0,_name)
#define _TimingStop(_idx,_name) _tt##_idx.stop();
#define TimingStop(_name) _TimingStop(0,_name);
#define TimingInit() _Stats.init();
#define TimingPrint() _Stats.print();
#else
#define _Timing(...) do{}while(0)
#define Timing(...) do{}while(0)
#define TimingInit() do{}while(0)
#define TimingPrint() do{}while(0)
#define _TimingStop(...) do{}while(0);
#define TimingStop(...) do{}while(0);
#endif

// ---------------------------------------------------------------------------

typedef std::set<DocumentObjectItem*> DocumentObjectItems;

class Gui::DocumentObjectData {
public:
    DocumentItem* docItem;
    DocumentObjectItems items;
    ViewProviderDocumentObject* viewObject;
    DocumentObjectItem* rootItem;
    std::vector<App::DocumentObject*> children;
    std::set<App::DocumentObject*> childSet;
    bool removeChildrenFromRoot;
    bool itemHidden;
    std::string label;
    std::string label2;

    typedef boost::signals2::scoped_connection Connection;

    Connection connectIcon;
    Connection connectTool;
    Connection connectStat;

    DocumentObjectData(DocumentItem* docItem, ViewProviderDocumentObject* vpd)
        : docItem(docItem), viewObject(vpd), rootItem(nullptr)
    {
        // Setup connections
        connectIcon = viewObject->signalChangeIcon.connect(
            boost::bind(&DocumentObjectData::slotChangeIcon, this));
        connectTool = viewObject->signalChangeToolTip.connect(
            boost::bind(&DocumentObjectData::slotChangeToolTip, this, bp::_1));
        connectStat = viewObject->signalChangeStatusTip.connect(
            boost::bind(&DocumentObjectData::slotChangeStatusTip, this, bp::_1));

        removeChildrenFromRoot = viewObject->canRemoveChildrenFromRoot();
        itemHidden = !viewObject->showInTree();
        label = viewObject->getObject()->Label.getValue();
        label2 = viewObject->getObject()->Label2.getValue();
    }

    const char* getTreeName() const {
        return docItem->getTreeName();
    }

    void updateChildren(DocumentObjectDataPtr other) {
        children = other->children;
        childSet = other->childSet;
    }

    bool updateChildren(bool checkVisibility) {
        auto newChildren = viewObject->claimChildren();
        auto obj = viewObject->getObject();
        std::set<App::DocumentObject*> newSet;
        bool updated = false;
        for (auto child : newChildren) {
            auto childVp = docItem->getViewProvider(child);
            if (!childVp)
                continue;
            if (child && child->getNameInDocument()) {
                if (!newSet.insert(child).second) {
                    TREE_WARN("duplicate child item " << obj->getFullName()
                        << '.' << child->getNameInDocument());
                }
                else if (!childSet.erase(child)) {
                    // this means new child detected
                    updated = true;
                    if (child->getDocument() == obj->getDocument() &&
                        child->getDocument() == docItem->document()->getDocument())
                    {
                        auto& parents = docItem->_ParentMap[child];
                        if (parents.insert(obj).second && child->Visibility.getValue()) {
                            bool showable = false;
                            for (auto parent : parents) {
                                if (!parent->hasChildElement()
                                    && parent->getLinkedObject(false) == parent)
                                {
                                    showable = true;
                                    break;
                                }
                            }
                            childVp->setShowable(showable);
                        }
                    }
                }
            }
        }
        for (auto child : childSet) {
            if (newSet.find(child) == newSet.end()) {
                // this means old child removed
                updated = true;
                auto mapIt = docItem->_ParentMap.find(child);

                // If 'child' is not part of the map then it has already been deleted
                // in _slotDeleteObject.
                if (mapIt != docItem->_ParentMap.end()) {
                    docItem->_ParentMap[child].erase(obj);

                    auto childVp = docItem->getViewProvider(child);
                    if (childVp && child->getDocument() == obj->getDocument())
                        childVp->setShowable(docItem->isObjectShowable(child));
                }
            }
        }
        // We still need to check the order of the children
        updated = updated || children != newChildren;
        children.swap(newChildren);
        childSet.swap(newSet);

        if (updated && checkVisibility) {
            for (auto child : children) {
                auto childVp = docItem->getViewProvider(child);
                if (childVp && child->getDocument() == obj->getDocument())
                    childVp->setShowable(docItem->isObjectShowable(child));
            }
        }
        return updated;
    }

    void testStatus(bool resetStatus = false) {
        QIcon icon, icon2;
        for (auto item : items)
            item->testStatus(resetStatus, icon, icon2);
    }

    void slotChangeIcon() {
        testStatus(true);
    }

    void slotChangeToolTip(const QString& tip) {
        for (auto item : items)
            item->setToolTip(0, tip);
    }

    void slotChangeStatusTip(const QString& tip) {
        for (auto item : items)
            item->setStatusTip(0, tip);
    }
};

// ---------------------------------------------------------------------------

class DocumentItem::ExpandInfo :
    public std::unordered_map<std::string, DocumentItem::ExpandInfoPtr>
{
public:
    void restore(Base::XMLReader& reader) {
        int level = reader.level();
        int count = reader.getAttributeAsInteger("count");
        for (int i = 0; i < count; ++i) {
            reader.readElement("Expand");
            auto& entry = (*this)[reader.getAttribute("name")];
            if (!reader.hasAttribute("count"))
                continue;
            entry.reset(new ExpandInfo);
            entry->restore(reader);
        }
        reader.readEndElement("Expand", level - 1);
    }
};

// ---------------------------------------------------------------------------

TreeWidgetEditDelegate::TreeWidgetEditDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{
}

QWidget* TreeWidgetEditDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem&, const QModelIndex& index) const
{
    auto ti = static_cast<QTreeWidgetItem*>(index.internalPointer());
    if (ti->type() != TreeWidget::ObjectType || index.column() > 1)
        return nullptr;
    DocumentObjectItem* item = static_cast<DocumentObjectItem*>(ti);
    App::DocumentObject* obj = item->object()->getObject();
    auto& prop = index.column() ? obj->Label2 : obj->Label;

    std::ostringstream str;
    str << "Change " << obj->getNameInDocument() << '.' << prop.getName();
    App::GetApplication().setActiveTransaction(str.str().c_str());
    FC_LOG("create editor transaction " << App::GetApplication().getActiveTransaction());

    ExpLineEdit* le = new ExpLineEdit(parent);
    le->setFrame(false);
    le->setReadOnly(prop.isReadOnly());
    le->bind(App::ObjectIdentifier(prop));
    le->setAutoApply(true);
    return le;
}

// ---------------------------------------------------------------------------

TreeWidget::TreeWidget(const char* name, QWidget* parent)
    : QTreeWidget(parent), SelectionObserver(true, ResolveMode::NoResolve)
    , contextItem(nullptr)
    , searchObject(nullptr)
    , searchDoc(nullptr)
    , searchContextDoc(nullptr)
    , editingItem(nullptr)
    , currentDocItem(nullptr)
    , myName(name)
{
    Instances.insert(this);
    if (!_LastSelectedTreeWidget)
        _LastSelectedTreeWidget = this;

    this->setDragEnabled(true);
    this->setAcceptDrops(true);
    this->setDropIndicatorShown(false);
    this->setDragDropMode(QTreeWidget::InternalMove);
    this->setRootIsDecorated(false);
    this->setColumnCount(2);
    this->setItemDelegate(new TreeWidgetEditDelegate(this));

    this->showHiddenAction = new QAction(this);
    this->showHiddenAction->setCheckable(true);
    connect(this->showHiddenAction, SIGNAL(triggered()),
        this, SLOT(onShowHidden()));

    this->hideInTreeAction = new QAction(this);
    this->hideInTreeAction->setCheckable(true);
    connect(this->hideInTreeAction, SIGNAL(triggered()),
        this, SLOT(onHideInTree()));

    this->createGroupAction = new QAction(this);
    connect(this->createGroupAction, SIGNAL(triggered()),
        this, SLOT(onCreateGroup()));

    this->relabelObjectAction = new QAction(this);
#ifndef Q_OS_MAC
    this->relabelObjectAction->setShortcut(Qt::Key_F2);
#endif
    connect(this->relabelObjectAction, SIGNAL(triggered()),
        this, SLOT(onRelabelObject()));

    this->finishEditingAction = new QAction(this);
    connect(this->finishEditingAction, SIGNAL(triggered()),
        this, SLOT(onFinishEditing()));

    this->selectDependentsAction = new QAction(this);
    connect(this->selectDependentsAction, SIGNAL(triggered()),
        this, SLOT(onSelectDependents()));

    this->closeDocAction = new QAction(this);
    connect(this->closeDocAction, SIGNAL(triggered()),
        this, SLOT(onCloseDoc()));

    this->reloadDocAction = new QAction(this);
    connect(this->reloadDocAction, SIGNAL(triggered()),
        this, SLOT(onReloadDoc()));

    this->skipRecomputeAction = new QAction(this);
    this->skipRecomputeAction->setCheckable(true);
    connect(this->skipRecomputeAction, SIGNAL(toggled(bool)),
        this, SLOT(onSkipRecompute(bool)));

    this->allowPartialRecomputeAction = new QAction(this);
    this->allowPartialRecomputeAction->setCheckable(true);
    connect(this->allowPartialRecomputeAction, SIGNAL(toggled(bool)),
        this, SLOT(onAllowPartialRecompute(bool)));

    this->markRecomputeAction = new QAction(this);
    connect(this->markRecomputeAction, SIGNAL(triggered()),
        this, SLOT(onMarkRecompute()));

    this->recomputeObjectAction = new QAction(this);
    connect(this->recomputeObjectAction, SIGNAL(triggered()),
        this, SLOT(onRecomputeObject()));
    this->searchObjectsAction = new QAction(this);
    this->searchObjectsAction->setText(tr("Search..."));
    this->searchObjectsAction->setStatusTip(tr("Search for objects"));
    connect(this->searchObjectsAction, SIGNAL(triggered()),
        this, SLOT(onSearchObjects()));

    // Setup connections
    connectNewDocument = Application::Instance->signalNewDocument.connect(boost::bind(&TreeWidget::slotNewDocument, this, bp::_1, bp::_2));
    connectDelDocument = Application::Instance->signalDeleteDocument.connect(boost::bind(&TreeWidget::slotDeleteDocument, this, bp::_1));
    connectRenDocument = Application::Instance->signalRenameDocument.connect(boost::bind(&TreeWidget::slotRenameDocument, this, bp::_1));
    connectActDocument = Application::Instance->signalActiveDocument.connect(boost::bind(&TreeWidget::slotActiveDocument, this, bp::_1));
    connectRelDocument = Application::Instance->signalRelabelDocument.connect(boost::bind(&TreeWidget::slotRelabelDocument, this, bp::_1));
    connectShowHidden = Application::Instance->signalShowHidden.connect(boost::bind(&TreeWidget::slotShowHidden, this, bp::_1));

    // Gui::Document::signalChangedObject informs the App::Document property
    // change, not view provider's own property, which is what the signal below
    // for
    connectChangedViewObj = Application::Instance->signalChangedObject.connect(
        boost::bind(&TreeWidget::slotChangedViewObject, this, bp::_1, bp::_2));

    // make sure to show a horizontal scrollbar if needed
    this->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    this->header()->setStretchLastSection(TreeParams::Instance()->getTreeViewStretchDescription());

    // Add the first main label
    this->rootItem = new QTreeWidgetItem(this);
    this->rootItem->setFlags(Qt::ItemIsEnabled);
    this->expandItem(this->rootItem);
    this->setSelectionMode(QAbstractItemView::ExtendedSelection);

    this->setMouseTracking(true); // needed for itemEntered() to work


    this->preselectTimer = new QTimer(this);
    this->preselectTimer->setSingleShot(true);

    this->statusTimer = new QTimer(this);
    this->statusTimer->setSingleShot(false);

    this->selectTimer = new QTimer(this);
    this->selectTimer->setSingleShot(true);

    connect(this->statusTimer, SIGNAL(timeout()),
        this, SLOT(onUpdateStatus()));
    connect(this, SIGNAL(itemEntered(QTreeWidgetItem*, int)),
        this, SLOT(onItemEntered(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
        this, SLOT(onItemCollapsed(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemExpanded(QTreeWidgetItem*)),
        this, SLOT(onItemExpanded(QTreeWidgetItem*)));
    connect(this, SIGNAL(itemSelectionChanged()),
        this, SLOT(onItemSelectionChanged()));
    connect(this, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
        this, SLOT(onItemChanged(QTreeWidgetItem*, int)));
    connect(this->preselectTimer, SIGNAL(timeout()),
        this, SLOT(onPreSelectTimer()));
    connect(this->selectTimer, SIGNAL(timeout()),
        this, SLOT(onSelectTimer()));
    preselectTime.start();

    setupText();
    if (!documentPixmap) {
        documentPixmap.reset(new QPixmap(Gui::BitmapFactory().pixmap("Document")));
        QIcon icon(*documentPixmap);
        documentPartialPixmap.reset(new QPixmap(icon.pixmap(documentPixmap->size(), QIcon::Disabled)));
    }
}

TreeWidget::~TreeWidget()
{
    connectNewDocument.disconnect();
    connectDelDocument.disconnect();
    connectRenDocument.disconnect();
    connectActDocument.disconnect();
    connectRelDocument.disconnect();
    connectShowHidden.disconnect();
    connectChangedViewObj.disconnect();
    Instances.erase(this);
    if (_LastSelectedTreeWidget == this)
        _LastSelectedTreeWidget = nullptr;
}

const char* TreeWidget::getTreeName() const {
    return myName.c_str();
}

// reimpelement to select only objects in the active document
void TreeWidget::selectAll() {
    auto gdoc = Application::Instance->getDocument(
        App::GetApplication().getActiveDocument());
    if (!gdoc)
        return;
    auto itDoc = DocumentMap.find(gdoc);
    if (itDoc == DocumentMap.end())
        return;
    if (TreeParams::Instance()->RecordSelection())
        Gui::Selection().selStackPush();
    Gui::Selection().clearSelection();
    Gui::Selection().setSelection(gdoc->getDocument()->getName(), gdoc->getDocument()->getObjects());
}

bool TreeWidget::isObjectShowable(App::DocumentObject* obj) {
    if (!obj || !obj->getNameInDocument())
        return true;
    Gui::Document* doc = Application::Instance->getDocument(obj->getDocument());
    if (!doc)
        return true;
    if (Instances.empty())
        return true;
    auto tree = *Instances.begin();
    auto it = tree->DocumentMap.find(doc);
    if (it != tree->DocumentMap.end())
        return it->second->isObjectShowable(obj);
    return true;
}

static bool _DisableCheckTopParent;

void TreeWidget::checkTopParent(App::DocumentObject*& obj, std::string& subname) {
    if (_DisableCheckTopParent)
        return;
    if (Instances.size() && obj && obj->getNameInDocument()) {
        auto tree = *Instances.begin();
        auto it = tree->DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
        if (it != tree->DocumentMap.end()) {
            if (tree->statusTimer->isActive()) {
                bool locked = tree->blockSelection(true);
                tree->_updateStatus(false);
                tree->blockSelection(locked);
            }
            auto parent = it->second->getTopParent(obj, subname);
            if (parent)
                obj = parent;
        }
    }
}

void TreeWidget::resetItemSearch() {
    if (!searchObject)
        return;
    auto it = ObjectTable.find(searchObject);
    if (it != ObjectTable.end()) {
        for (auto& data : it->second) {
            if (!data)
                continue;
            for (auto item : data->items)
                static_cast<DocumentObjectItem*>(item)->restoreBackground();
        }
    }
    searchObject = nullptr;
}

void TreeWidget::startItemSearch(QLineEdit* edit) {
    resetItemSearch();
    searchDoc = nullptr;
    searchContextDoc = nullptr;
    auto sels = selectedItems();
    if (sels.size() == 1) {
        if (sels.front()->type() == DocumentType) {
            searchDoc = static_cast<DocumentItem*>(sels.front())->document();
        }
        else if (sels.front()->type() == ObjectType) {
            auto item = static_cast<DocumentObjectItem*>(sels.front());
            searchDoc = item->object()->getDocument();
            searchContextDoc = item->getOwnerDocument()->document();
        }
    }
    else
        searchDoc = Application::Instance->activeDocument();

    App::DocumentObject* obj = nullptr;
    if (searchContextDoc && searchContextDoc->getDocument()->getObjects().size())
        obj = searchContextDoc->getDocument()->getObjects().front();
    else if (searchDoc && searchDoc->getDocument()->getObjects().size())
        obj = searchDoc->getDocument()->getObjects().front();

    if (obj)
        static_cast<ExpressionLineEdit*>(edit)->setDocumentObject(obj);
}

void TreeWidget::itemSearch(const QString& text, bool select) {
    resetItemSearch();

    auto docItem = getDocumentItem(searchDoc);
    if (!docItem) {
        docItem = getDocumentItem(Application::Instance->activeDocument());
        if (!docItem) {
            FC_TRACE("item search no document");
            resetItemSearch();
            return;
        }
    }

    auto doc = docItem->document()->getDocument();
    const auto& objs = doc->getObjects();
    if (objs.empty()) {
        FC_TRACE("item search no objects");
        return;
    }
    std::string txt(text.toUtf8().constData());
    try {
        if (txt.empty())
            return;
        if (txt.find("<<") == std::string::npos) {
            auto pos = txt.find('.');
            if (pos == std::string::npos)
                txt += '.';
            else if (pos != txt.size() - 1) {
                txt.insert(pos + 1, "<<");
                if (txt.back() != '.')
                    txt += '.';
                txt += ">>.";
            }
        }
        else if (txt.back() != '.')
            txt += '.';
        txt += "_self";
        auto path = App::ObjectIdentifier::parse(objs.front(), txt);
        if (path.getPropertyName() != "_self") {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        auto obj = path.getDocumentObject();
        if (!obj) {
            FC_TRACE("Object " << txt << " not found in " << doc->getName());
            return;
        }
        std::string subname = path.getSubObjectName();
        App::DocumentObject* parent = nullptr;
        if (searchContextDoc) {
            auto it = DocumentMap.find(searchContextDoc);
            if (it != DocumentMap.end()) {
                parent = it->second->getTopParent(obj, subname);
                if (parent) {
                    obj = parent;
                    docItem = it->second;
                    doc = docItem->document()->getDocument();
                }
            }
        }
        if (!parent) {
            parent = docItem->getTopParent(obj, subname);
            while (!parent) {
                if (docItem->document()->getDocument() == obj->getDocument()) {
                    // this shouldn't happen
                    FC_LOG("Object " << txt << " not found in " << doc->getName());
                    return;
                }
                auto it = DocumentMap.find(Application::Instance->getDocument(obj->getDocument()));
                if (it == DocumentMap.end())
                    return;
                docItem = it->second;
                parent = docItem->getTopParent(obj, subname);
            }
            obj = parent;
        }
        auto item = docItem->findItemByObject(true, obj, subname.c_str());
        if (!item) {
            FC_TRACE("item " << txt << " not found in " << doc->getName());
            return;
        }
        scrollToItem(item);
        Selection().setPreselect(obj->getDocument()->getName(),
            obj->getNameInDocument(), subname.c_str(), 0, 0, 0,
            SelectionChanges::MsgSource::TreeView);
        if (select) {
            Gui::Selection().selStackPush();
            Gui::Selection().clearSelection();
            Gui::Selection().addSelection(obj->getDocument()->getName(),
                obj->getNameInDocument(), subname.c_str());
            Gui::Selection().selStackPush();
        }
        else {
            searchObject = item->object()->getObject();
            item->setBackground(0, QColor(255, 255, 0, 100));
        }
        FC_TRACE("found item " << txt);
    }
    catch (...)
    {
        FC_TRACE("item " << txt << " search exception in " << doc->getName());
    }
}

Gui::Document* TreeWidget::selectedDocument() {
    for (auto tree : Instances) {
        if (!tree->isVisible())
            continue;
        auto sels = tree->selectedItems();
        if (sels.size() == 1 && sels[0]->type() == DocumentType)
            return static_cast<DocumentItem*>(sels[0])->document();
    }
    return nullptr;
}

void TreeWidget::updateStatus(bool delay) {
    for (auto tree : Instances)
        tree->_updateStatus(delay);
}

void TreeWidget::_updateStatus(bool delay) {
    if (!delay) {
        if (ChangedObjects.size() || NewObjects.size())
            onUpdateStatus();
        return;
    }
    int timeout = TreeParams::Instance()->StatusTimeout();
    if (timeout < 0)
        timeout = 1;
    statusTimer->start(timeout);
}

void TreeWidget::contextMenuEvent(QContextMenuEvent* e)
{
    // ask workbenches and view provider, ...
    MenuItem view;
    Gui::Application::Instance->setupContextMenu("Tree", &view);

    view << "Std_Expressions";
    Workbench::createLinkMenu(&view);

    QMenu contextMenu;

    QMenu subMenu;
    QMenu editMenu;
    QActionGroup subMenuGroup(&subMenu);
    subMenuGroup.setExclusive(true);
    connect(&subMenuGroup, SIGNAL(triggered(QAction*)),
        this, SLOT(onActivateDocument(QAction*)));
    MenuManager::getInstance()->setupContextMenu(&view, contextMenu);

    // get the current item
    this->contextItem = itemAt(e->pos());

    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        App::GetApplication().setActiveDocument(doc);
        showHiddenAction->setChecked(docitem->showHidden());
        contextMenu.addAction(this->showHiddenAction);
        contextMenu.addAction(this->searchObjectsAction);
        contextMenu.addAction(this->closeDocAction);
        if (doc->testStatus(App::Document::PartialDoc))
            contextMenu.addAction(this->reloadDocAction);
        else {
            for (auto d : doc->getDependentDocuments()) {
                if (d->testStatus(App::Document::PartialDoc)) {
                    contextMenu.addAction(this->reloadDocAction);
                    break;
                }
            }
            contextMenu.addAction(this->selectDependentsAction);
            this->skipRecomputeAction->setChecked(doc->testStatus(App::Document::SkipRecompute));
            contextMenu.addAction(this->skipRecomputeAction);
            this->allowPartialRecomputeAction->setChecked(doc->testStatus(App::Document::AllowPartialRecompute));
            if (doc->testStatus(App::Document::SkipRecompute))
                contextMenu.addAction(this->allowPartialRecomputeAction);
            contextMenu.addAction(this->markRecomputeAction);
            contextMenu.addAction(this->createGroupAction);
        }
        contextMenu.addSeparator();
    }
    else if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);

        // check that the selection is not across several documents
        bool acrossDocuments = false;
        auto SelectedObjectsList = Selection().getCompleteSelection();
        // get the object's document as reference
        App::Document* doc = objitem->object()->getObject()->getDocument();
        for (auto it = SelectedObjectsList.begin(); it != SelectedObjectsList.end(); ++it) {
            if ((*it).pDoc != doc) {
                acrossDocuments = true;
                break;
            }
        }

        showHiddenAction->setChecked(doc->ShowHidden.getValue());
        contextMenu.addAction(this->showHiddenAction);

        hideInTreeAction->setChecked(!objitem->object()->showInTree());
        contextMenu.addAction(this->hideInTreeAction);

        if (!acrossDocuments) { // is only sensible for selections within one document
            if (objitem->object()->getObject()->isDerivedFrom(App::DocumentObjectGroup::getClassTypeId()))
                contextMenu.addAction(this->createGroupAction);
            // if there are dependent objects in the selection, add context menu to add them to selection
            if (CheckForDependents())
                contextMenu.addAction(this->selectDependentsAction);
        }

        contextMenu.addSeparator();
        contextMenu.addAction(this->markRecomputeAction);
        contextMenu.addAction(this->recomputeObjectAction);
        contextMenu.addSeparator();

        // relabeling is only possible for a single selected document
        if (SelectedObjectsList.size() == 1)
            contextMenu.addAction(this->relabelObjectAction);

        auto selItems = this->selectedItems();
        // if only one item is selected, setup the edit menu
        if (selItems.size() == 1) {
            objitem->object()->setupContextMenu(&editMenu, this, SLOT(onStartEditing()));
            QList<QAction*> editAct = editMenu.actions();
            if (!editAct.isEmpty()) {
                QAction* topact = contextMenu.actions().front();
                for (QList<QAction*>::iterator it = editAct.begin(); it != editAct.end(); ++it)
                    contextMenu.insertAction(topact, *it);
                QAction* first = editAct.front();
                contextMenu.setDefaultAction(first);
                if (objitem->object()->isEditing())
                    contextMenu.insertAction(topact, this->finishEditingAction);
                contextMenu.insertSeparator(topact);
            }
        }
    }


    // add a submenu to active a document if two or more exist
    std::vector<App::Document*> docs = App::GetApplication().getDocuments();
    if (docs.size() >= 2) {
        contextMenu.addSeparator();
        App::Document* activeDoc = App::GetApplication().getActiveDocument();
        subMenu.setTitle(tr("Activate document"));
        contextMenu.addMenu(&subMenu);
        QAction* active = nullptr;
        for (std::vector<App::Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            QString label = QString::fromUtf8((*it)->Label.getValue());
            QAction* action = subMenuGroup.addAction(label);
            action->setCheckable(true);
            action->setStatusTip(tr("Activate document %1").arg(label));
            action->setData(QByteArray((*it)->getName()));
            if (*it == activeDoc) active = action;
        }

        if (active)
            active->setChecked(true);
        subMenu.addActions(subMenuGroup.actions());
    }

    if (contextMenu.actions().count() > 0) {
        try {
            contextMenu.exec(QCursor::pos());
        }
        catch (Base::Exception& e) {
            e.ReportException();
        }
        catch (std::exception& e) {
            FC_ERR("C++ exception: " << e.what());
        }
        catch (...) {
            FC_ERR("Unknown exception");
        }
        contextItem = nullptr;
    }
}

void TreeWidget::hideEvent(QHideEvent* ev) {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
#if 0
    TREE_TRACE("detaching selection observer");
    this->detachSelection();
    selectTimer->stop();
#endif
    QTreeWidget::hideEvent(ev);
}

void TreeWidget::showEvent(QShowEvent* ev) {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
#if 0
    TREE_TRACE("attaching selection observer");
    this->attachSelection();
    int timeout = TreeParams::Instance()->SelectionTimeout();
    if (timeout <= 0)
        timeout = 1;
    selectTimer->start(timeout);
    _updateStatus();
#endif
    QTreeWidget::showEvent(ev);
}

void TreeWidget::onCreateGroup()
{
    QString name = tr("Group");
    App::AutoTransaction trans("Create group");
    if (this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        QString cmd = QString::fromLatin1("App.getDocument(\"%1\").addObject"
            "(\"App::DocumentObjectGroup\",\"%2\")")
            .arg(QString::fromLatin1(doc->getName()), name);
        Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
    }
    else if (this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        App::DocumentObject* obj = objitem->object()->getObject();
        App::Document* doc = obj->getDocument();
        QString cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\")"
            ".newObject(\"App::DocumentObjectGroup\",\"%3\")")
            .arg(QString::fromLatin1(doc->getName()),
                QString::fromLatin1(obj->getNameInDocument()),
                name);
        Gui::Command::runCommand(Gui::Command::App, cmd.toUtf8());
    }
}

void TreeWidget::onRelabelObject()
{
    QTreeWidgetItem* item = currentItem();
    if (item)
        editItem(item);
}

void TreeWidget::onStartEditing()
{
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        if (this->contextItem && this->contextItem->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
                (this->contextItem);
            int edit = action->data().toInt();

            App::DocumentObject* obj = objitem->object()->getObject();
            if (!obj || !obj->getNameInDocument())
                return;
            auto doc = const_cast<Document*>(objitem->getOwnerDocument()->document());
            MDIView* view = doc->getActiveView();
            if (view) getMainWindow()->setActiveWindow(view);

            // Always open a transaction here doesn't make much sense because:
            // - many objects open transactions when really changing some properties
            // - this leads to certain inconsistencies with the doubleClicked() method
            // So, only the view provider class should decide what to do
#if 0
            // open a transaction before starting edit mode
            std::string cmd("Edit ");
            cmd += obj->Label.getValue();
            doc->openCommand(cmd.c_str());
            bool ok = doc->setEdit(objitem->object(), edit);
            if (!ok) doc->abortCommand();
#else
            editingItem = objitem;
            if (!doc->setEdit(objitem->object(), edit))
                editingItem = nullptr;
#endif
        }
    }
}

void TreeWidget::onFinishEditing()
{
    if (this->contextItem && this->contextItem->type() == ObjectType) {
        DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>
            (this->contextItem);
        App::DocumentObject* obj = objitem->object()->getObject();
        if (!obj)
            return;
        Gui::Document* doc = Gui::Application::Instance->getDocument(obj->getDocument());
        doc->commitCommand();
        doc->resetEdit();
        doc->getDocument()->recompute();
    }
}

// check if selection has dependent objects
bool TreeWidget::CheckForDependents()
{
    // if the selected object is a document
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        return true;
    }
    // it can be an object
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                App::DocumentObject* obj = objitem->object()->getObject();
                // get dependents
                auto subObjectList = obj->getOutList();
                if (subObjectList.size() > 0)
                    return true;
            }
        }
    }

    return false;
}

// adds an App::DocumentObject* and its dependent objects to the selection
void TreeWidget::addDependentToSelection(App::Document* doc, App::DocumentObject* docObject)
{
    // add the docObject to the selection
    Selection().addSelection(doc->getName(), docObject->getNameInDocument());
    // get the dependent
    auto subObjectList = docObject->getOutList();
    // the dependent can in turn have dependents, thus add them recursively
    for (auto itDepend = subObjectList.begin(); itDepend != subObjectList.end(); ++itDepend)
        addDependentToSelection(doc, (*itDepend));
}

// add dependents of the selected tree object to selection
void TreeWidget::onSelectDependents()
{
    // We only have this context menu entry if the selection is within one document but it
    // might be not the active document. Therefore get the document not here but later by casting.
    App::Document* doc;

    // if the selected object is a document
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        doc = docitem->document()->getDocument();
        std::vector<App::DocumentObject*> obj = doc->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
            Selection().addSelection(doc->getName(), (*it)->getNameInDocument());
    }
    // it can be an object
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                doc = objitem->object()->getObject()->getDocument();
                App::DocumentObject* obj = objitem->object()->getObject();
                // the dependents can also have dependents, thus add them recursively via a separate void
                addDependentToSelection(doc, obj);
            }
        }
    }
}

void TreeWidget::onSkipRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::SkipRecompute, on);
    }
}

void TreeWidget::onAllowPartialRecompute(bool on)
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        doc->setStatus(App::Document::AllowPartialRecompute, on);
    }
}

void TreeWidget::onMarkRecompute()
{
    // if a document item is selected then touch all objects
    if (this->contextItem && this->contextItem->type() == DocumentType) {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        App::Document* doc = docitem->document()->getDocument();
        std::vector<App::DocumentObject*> obj = doc->getObjects();
        for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it)
            (*it)->enforceRecompute();
    }
    // mark all selected objects
    else {
        QList<QTreeWidgetItem*> items = this->selectedItems();
        for (QList<QTreeWidgetItem*>::iterator it = items.begin(); it != items.end(); ++it) {
            if ((*it)->type() == ObjectType) {
                DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(*it);
                App::DocumentObject* obj = objitem->object()->getObject();
                obj->enforceRecompute();
            }
        }
    }
}

void TreeWidget::onRecomputeObject() {
    std::vector<App::DocumentObject*> objs;
    for (auto ti : selectedItems()) {
        if (ti->type() == ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(ti);
            objs.push_back(objitem->object()->getObject());
            objs.back()->enforceRecompute();
        }
    }
    if (objs.empty())
        return;
    App::AutoTransaction committer("Recompute object");
    objs.front()->getDocument()->recompute(objs, true);
}


DocumentItem* TreeWidget::getDocumentItem(const Gui::Document* doc) const {
    auto it = DocumentMap.find(doc);
    if (it != DocumentMap.end())
        return it->second;
    return nullptr;
}

void TreeWidget::selectAllInstances(const ViewProviderDocumentObject& vpd) {
    if (!isSelectionAttached())
        return;

    if (selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    for (const auto& v : DocumentMap)
        v.second->selectAllInstances(vpd);
}

TreeWidget* TreeWidget::instance() {
    auto res = _LastSelectedTreeWidget;
    if (res && res->isVisible())
        return res;
    for (auto inst : Instances) {
        if (!res) res = inst;
        if (inst->isVisible())
            return inst;
    }
    return res;
}

std::vector<TreeWidget::SelInfo> TreeWidget::getSelection(App::Document* doc)
{
    std::vector<SelInfo> ret;

    TreeWidget* tree = instance();
    if (!tree || !tree->isSelectionAttached()) {
        for (auto pTree : Instances)
            if (pTree->isSelectionAttached()) {
                tree = pTree;
                break;
            }
    }
    if (!tree)
        return ret;

    if (tree->selectTimer->isActive())
        tree->onSelectTimer();
    else
        tree->_updateStatus(false);

    for (auto ti : tree->selectedItems()) {
        if (ti->type() != ObjectType) continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        auto vp = item->object();
        auto obj = vp->getObject();
        if (!obj || !obj->getNameInDocument()) {
            FC_WARN("skip invalid object");
            continue;
        }
        if (doc && obj->getDocument() != doc) {
            FC_LOG("skip objects not from current document");
            continue;
        }
        ViewProviderDocumentObject* parentVp = nullptr;
        auto parent = item->getParentItem();
        if (parent) {
            parentVp = parent->object();
            if (!parentVp->getObject()->getNameInDocument()) {
                FC_WARN("skip '" << obj->getFullName() << "' with invalid parent");
                continue;
            }
        }
        ret.emplace_back();
        auto& sel = ret.back();
        sel.topParent = nullptr;
        std::ostringstream ss;
        item->getSubName(ss, sel.topParent);
        if (!sel.topParent)
            sel.topParent = obj;
        else
            ss << obj->getNameInDocument() << '.';
        sel.subname = ss.str();
        sel.parentVp = parentVp;
        sel.vp = vp;
    }
    return ret;
}

void TreeWidget::selectAllLinks(App::DocumentObject* obj) {
    if (!isSelectionAttached())
        return;

    if (!obj || !obj->getNameInDocument()) {
        TREE_ERR("invalid object");
        return;
    }

    if (selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    for (auto link : App::GetApplication().getLinksTo(obj, App::GetLinkRecursive))
    {
        if (!link || !link->getNameInDocument()) {
            TREE_ERR("invalid linked object");
            continue;
        }
        auto vp = dynamic_cast<ViewProviderDocumentObject*>(
            Application::Instance->getViewProvider(link));
        if (!vp) {
            TREE_ERR("invalid view provider of the linked object");
            continue;
        }
        for (auto& v : DocumentMap)
            v.second->selectAllInstances(*vp);
    }
}

void TreeWidget::onSearchObjects()
{
    emitSearchObjects();
}

void TreeWidget::onActivateDocument(QAction* active)
{
    // activate the specified document
    QByteArray docname = active->data().toByteArray();
    Gui::Document* doc = Application::Instance->getDocument((const char*)docname);
    if (doc && !doc->setActiveView())
        doc->setActiveView(nullptr, View3DInventor::getClassTypeId());
}

Qt::DropActions TreeWidget::supportedDropActions() const
{
    return Qt::LinkAction | Qt::CopyAction | Qt::MoveAction;
}

bool TreeWidget::event(QEvent* e)
{
#if 0
    if (e->type() == QEvent::ShortcutOverride) {
        QKeyEvent* ke = static_cast<QKeyEvent*>(e);
        switch (ke->key()) {
        case Qt::Key_Delete:
            ke->accept();
        }
    }
#endif
    return QTreeWidget::event(e);
}

bool TreeWidget::eventFilter(QObject*, QEvent* ev) {
    switch (ev->type()) {
    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        QKeyEvent* ke = static_cast<QKeyEvent*>(ev);
        if (ke->key() != Qt::Key_Escape) {
            // Qt 5 only recheck key modifier on mouse move, so generate a fake
            // event to trigger drag cursor change
            QMouseEvent* mouseEvent = new QMouseEvent(QEvent::MouseMove,
                mapFromGlobal(QCursor::pos()), QCursor::pos(), Qt::NoButton,
                QApplication::mouseButtons(), QApplication::queryKeyboardModifiers());
            QApplication::postEvent(this, mouseEvent);
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void TreeWidget::keyPressEvent(QKeyEvent* event)
{
#if 0
    if (event && event->matches(QKeySequence::Delete)) {
        event->ignore();
    }
#endif
    if (event->matches(QKeySequence::Find)) {
        event->accept();
        onSearchObjects();
        return;
    }
    else if (event->modifiers() == Qt::AltModifier) {
        if (event->key() == Qt::Key_Left) {
            for (auto& item : selectedItems()) {
                item->setExpanded(false);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Right) {
            for (auto& item : selectedItems()) {
                item->setExpanded(true);
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Up) {
            for (auto& item : selectedItems()) {
                item->setExpanded(true);
                for (auto& child : childrenOfItem(*item)) {
                    child->setExpanded(false);
                }
            }
            event->accept();
            return;
        }
        else if (event->key() == Qt::Key_Down) {
            for (auto& item : selectedItems()) {
                item->setExpanded(true);
                for (auto& child : childrenOfItem(*item)) {
                    child->setExpanded(true);
                }
            }
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_Left) {
        auto index = currentIndex();
        if (index.column() == 1) {
            setCurrentIndex(model()->index(index.row(), 0, index.parent()));
            event->accept();
            return;
        }
    }
    else if (event->key() == Qt::Key_Right) {
        auto index = currentIndex();
        if (index.column() == 0) {
            setCurrentIndex(model()->index(index.row(), 1, index.parent()));
            event->accept();
            return;
        }
    }
    QTreeWidget::keyPressEvent(event);
}

void TreeWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    QTreeWidgetItem* item = itemAt(event->pos());
    if (!item)
        return;

    try {
        if (item->type() == TreeWidget::DocumentType) {
            //QTreeWidget::mouseDoubleClickEvent(event);
            Gui::Document* doc = static_cast<DocumentItem*>(item)->document();
            if (!doc)
                return;
            if (doc->getDocument()->testStatus(App::Document::PartialDoc)) {
                contextItem = item;
                onReloadDoc();
                return;
            }
            if (!doc->setActiveView())
                doc->setActiveView(nullptr, View3DInventor::getClassTypeId());
        }
        else if (item->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* objitem = static_cast<DocumentObjectItem*>(item);
            ViewProviderDocumentObject* vp = objitem->object();

            objitem->getOwnerDocument()->document()->setActiveView(vp);
            auto manager = Application::Instance->macroManager();
            auto lines = manager->getLines();

            std::ostringstream ss;
            ss << Command::getObjectCmd(vp->getObject())
                << ".ViewObject.doubleClicked()";

            const char* commandText = vp->getTransactionText();
            if (commandText) {
                auto editDoc = Application::Instance->editDocument();
                App::AutoTransaction committer(commandText, true);

                if (!vp->doubleClicked())
                    QTreeWidget::mouseDoubleClickEvent(event);
                else if (lines == manager->getLines())
                    manager->addLine(MacroManager::Gui, ss.str().c_str());

                // If the double click starts an editing, let the transaction persist
                if (!editDoc && Application::Instance->editDocument())
                    committer.setEnable(false);
            }
            else {
                if (!vp->doubleClicked())
                    QTreeWidget::mouseDoubleClickEvent(event);
                else if (lines == manager->getLines())
                    manager->addLine(MacroManager::Gui, ss.str().c_str());
            }
        }
    }
    catch (Base::Exception& e) {
        e.ReportException();
    }
    catch (std::exception& e) {
        FC_ERR("C++ exception: " << e.what());
    }
    catch (...) {
        FC_ERR("Unknown exception");
    }
}

void TreeWidget::startDragging() {
    if (state() != NoState)
        return;
    if (selectedItems().empty())
        return;

    setState(DraggingState);
    startDrag(model()->supportedDragActions());
    setState(NoState);
    stopAutoScroll();
}

void TreeWidget::startDrag(Qt::DropActions supportedActions)
{
    QTreeWidget::startDrag(supportedActions);
    if (_DragEventFilter) {
        _DragEventFilter = false;
        qApp->removeEventFilter(this);
    }
}

QMimeData* TreeWidget::mimeData(const QList<QTreeWidgetItem*> items) const
{
#if 0
    // all selected items must reference an object from the same document
    App::Document* doc = 0;
    for (QList<QTreeWidgetItem*>::ConstIterator it = items.begin(); it != items.end(); ++it) {
        if ((*it)->type() != TreeWidget::ObjectType)
            return 0;
        App::DocumentObject* obj = static_cast<DocumentObjectItem*>(*it)->object()->getObject();
        if (!doc)
            doc = obj->getDocument();
        else if (doc != obj->getDocument())
            return 0;
    }
#endif
    return QTreeWidget::mimeData(items);
}

bool TreeWidget::dropMimeData(QTreeWidgetItem* parent, int index,
    const QMimeData* data, Qt::DropAction action)
{
    return QTreeWidget::dropMimeData(parent, index, data, action);
}

void TreeWidget::dragEnterEvent(QDragEnterEvent* event)
{
    QTreeWidget::dragEnterEvent(event);
}

void TreeWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QTreeWidget::dragLeaveEvent(event);
}

void TreeWidget::dragMoveEvent(QDragMoveEvent* event)
{
    // Qt5 does not change drag cursor in response to modifier key press,
    // because QDrag installs a event filter that eats up key event. We install
    // a filter after Qt and generate fake mouse move event in response to key
    // press event, which triggers QDrag to update its cursor
    if (!_DragEventFilter) {
        _DragEventFilter = true;
        qApp->installEventFilter(this);
    }

    QTreeWidget::dragMoveEvent(event);
    if (!event->isAccepted())
        return;

    auto modifier = QApplication::queryKeyboardModifiers();
    QTreeWidgetItem* targetItem = itemAt(event->pos());
    if (!targetItem || targetItem->isSelected()) {
        leaveEvent(nullptr);
        event->ignore();
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        leaveEvent(nullptr);
        if (modifier == Qt::ControlModifier)
            event->setDropAction(Qt::CopyAction);
        else if (modifier == Qt::AltModifier)
            event->setDropAction(Qt::LinkAction);
        else
            event->setDropAction(Qt::MoveAction);
    }
    else if (targetItem->type() == TreeWidget::ObjectType) {
        onItemEntered(targetItem);

        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();

        try {
            auto items = selectedItems();

            if (modifier == Qt::ControlModifier)
                event->setDropAction(Qt::CopyAction);
            else if (modifier == Qt::AltModifier && items.size() == 1)
                event->setDropAction(Qt::LinkAction);
            else
                event->setDropAction(Qt::MoveAction);
            auto da = event->dropAction();
            bool dropOnly = da == Qt::CopyAction || da == Qt::MoveAction;

            if (da != Qt::LinkAction && !vp->canDropObjects()) {
                if (!(event->possibleActions() & Qt::LinkAction) || items.size() != 1) {
                    TREE_TRACE("cannot drop");
                    event->ignore();
                    return;
                }
            }
            for (auto ti : items) {
                if (ti->type() != TreeWidget::ObjectType) {
                    TREE_TRACE("cannot drop");
                    event->ignore();
                    return;
                }
                auto item = static_cast<DocumentObjectItem*>(ti);

                auto obj = item->object()->getObject();

                if (!dropOnly && !vp->canDragAndDropObject(obj)) {
                    // check if items can be dragged
                    auto parentItem = item->getParentItem();
                    if (parentItem
                        && (!parentItem->object()->canDragObjects()
                            || !parentItem->object()->canDragObject(item->object()->getObject())))
                    {
                        if (!(event->possibleActions() & Qt::CopyAction)) {
                            TREE_TRACE("Cannot drag object");
                            event->ignore();
                            return;
                        }
                        event->setDropAction(Qt::CopyAction);
                    }
                }

                std::ostringstream str;
                auto owner = item->getRelativeParent(str, targetItemObj);
                auto subname = str.str();

                // let the view provider decide to accept the object or ignore it
                if (da != Qt::LinkAction && !vp->canDropObjectEx(obj, owner, subname.c_str(), item->mySubs)) {
                    if (event->possibleActions() & Qt::LinkAction) {
                        if (items.size() > 1) {
                            TREE_TRACE("Cannot replace with more than one object");
                            event->ignore();
                            return;
                        }
                        if (!targetItemObj->getParentItem()) {
                            TREE_TRACE("Cannot replace without parent");
                            event->ignore();
                            return;
                        }
                        event->setDropAction(Qt::LinkAction);
                        return;
                    }

                    TREE_TRACE("cannot drop " << obj->getFullName() << ' '
                        << (owner ? owner->getFullName() : "<No Owner>") << '.' << subname);
                    event->ignore();
                    return;
                }
            }
        }
        catch (Base::Exception& e) {
            e.ReportException();
            event->ignore();
        }
        catch (std::exception& e) {
            FC_ERR("C++ exception: " << e.what());
            event->ignore();
        }
        catch (...) {
            FC_ERR("Unknown exception");
            event->ignore();
        }
    }
    else {
        leaveEvent(nullptr);
        event->ignore();
    }
}

struct ItemInfo {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string ownerDoc;
    std::string owner;
    std::string subname;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
    std::vector<std::string> subs;
    bool dragging = false;
};
struct ItemInfo2 {
    std::string doc;
    std::string obj;
    std::string parentDoc;
    std::string parent;
    std::string topDoc;
    std::string topObj;
    std::string topSubname;
};

void TreeWidget::dropEvent(QDropEvent* event)
{
    //FIXME: This should actually be done inside dropMimeData

    bool touched = false;
    QTreeWidgetItem* targetItem = itemAt(event->pos());
    // not dropped onto an item
    if (!targetItem)
        return;
    // one of the source items is also the destination item, that's not allowed
    if (targetItem->isSelected())
        return;

    App::Document* thisDoc;

    Base::EmptySequencer seq;

    // filter out the selected items we cannot handle
    std::vector<std::pair<DocumentObjectItem*, std::vector<std::string> > > items;
    auto sels = selectedItems();
    items.reserve(sels.size());
    for (auto ti : sels) {
        if (ti->type() != TreeWidget::ObjectType)
            continue;
        // ignore child elements if the parent is selected
        if (sels.contains(ti->parent()))
            continue;
        if (ti == targetItem)
            continue;
        auto item = static_cast<DocumentObjectItem*>(ti);
        items.emplace_back();
        auto& info = items.back();
        info.first = item;
        info.second.insert(info.second.end(), item->mySubs.begin(), item->mySubs.end());
    }

    if (items.empty())
        return; // nothing needs to be done

    std::string errMsg;

    if (QApplication::keyboardModifiers() == Qt::ControlModifier)
        event->setDropAction(Qt::CopyAction);
    else if (QApplication::keyboardModifiers() == Qt::AltModifier
        && (items.size() == 1 || targetItem->type() == TreeWidget::DocumentType))
        event->setDropAction(Qt::LinkAction);
    else
        event->setDropAction(Qt::MoveAction);
    auto da = event->dropAction();
    bool dropOnly = da == Qt::CopyAction || da == Qt::LinkAction;

    if (targetItem->type() == TreeWidget::ObjectType) {
        // add object to group
        DocumentObjectItem* targetItemObj = static_cast<DocumentObjectItem*>(targetItem);
        thisDoc = targetItemObj->getOwnerDocument()->document()->getDocument();
        Gui::ViewProviderDocumentObject* vp = targetItemObj->object();

        if (!vp || !vp->getObject() || !vp->getObject()->getNameInDocument()) {
            TREE_TRACE("invalid object");
            return;
        }

        if (da != Qt::LinkAction && !vp->canDropObjects()) {
            if (!(event->possibleActions() & Qt::LinkAction) || items.size() != 1) {
                TREE_TRACE("Cannot drop objects");
                return; // no group like object
            }
        }

        std::ostringstream targetSubname;
        App::DocumentObject* targetParent = nullptr;
        targetItemObj->getSubName(targetSubname, targetParent);
        Selection().selStackPush();
        Selection().clearCompleteSelection();
        if (targetParent) {
            targetSubname << vp->getObject()->getNameInDocument() << '.';
            Selection().addSelection(targetParent->getDocument()->getName(),
                targetParent->getNameInDocument(), targetSubname.str().c_str());
        }
        else {
            targetParent = targetItemObj->object()->getObject();
            Selection().addSelection(targetParent->getDocument()->getName(),
                targetParent->getNameInDocument());
        }

        bool syncPlacement = TreeParams::Instance()->SyncPlacement() && targetItemObj->isGroup();

        bool setSelection = true;
        std::vector<std::pair<App::DocumentObject*, std::string> > droppedObjects;

        std::vector<ItemInfo> infos;
        // Only keep text names here, because you never know when doing drag
        // and drop some object may delete other objects.
        infos.reserve(items.size());
        for (auto& v : items) {
            infos.emplace_back();
            auto& info = infos.back();
            auto item = v.first;
            Gui::ViewProviderDocumentObject* vpc = item->object();
            App::DocumentObject* obj = vpc->getObject();

            std::ostringstream str;
            App::DocumentObject* topParent = nullptr;
            auto owner = item->getRelativeParent(str, targetItemObj, &topParent, &info.topSubname);
            if (syncPlacement && topParent) {
                info.topDoc = topParent->getDocument()->getName();
                info.topObj = topParent->getNameInDocument();
            }
            info.subname = str.str();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if (owner) {
                info.ownerDoc = owner->getDocument()->getName();
                info.owner = owner->getNameInDocument();
            }

            info.subs.swap(v.second);

            // check if items can be dragged
            if (!dropOnly &&
                item->myOwner == targetItemObj->myOwner &&
                vp->canDragAndDropObject(item->object()->getObject()))
            {
                // check if items can be dragged
                auto parentItem = item->getParentItem();
                if (!parentItem)
                    info.dragging = true;
                else if (parentItem->object()->canDragObjects()
                    && parentItem->object()->canDragObject(item->object()->getObject()))
                {
                    info.dragging = true;
                    auto vpp = parentItem->object();
                    info.parent = vpp->getObject()->getNameInDocument();
                    info.parentDoc = vpp->getObject()->getDocument()->getName();
                }
            }

            if (da != Qt::LinkAction
                && !vp->canDropObjectEx(obj, owner, info.subname.c_str(), item->mySubs))
            {
                if (event->possibleActions() & Qt::LinkAction) {
                    if (items.size() > 1) {
                        TREE_TRACE("Cannot replace with more than one object");
                        return;
                    }
                    auto ext = vp->getObject()->getExtensionByType<App::LinkBaseExtension>(true);
                    if ((!ext || !ext->getLinkedObjectProperty()) && !targetItemObj->getParentItem()) {
                        TREE_TRACE("Cannot replace without parent");
                        return;
                    }
                    da = Qt::LinkAction;
                }
            }
        }

        // Open command
        App::AutoTransaction committer("Drop object");
        try {
            auto targetObj = targetItemObj->object()->getObject();

            std::set<App::DocumentObject*> inList;
            auto parentObj = targetObj;
            if (da == Qt::LinkAction && targetItemObj->getParentItem())
                parentObj = targetItemObj->getParentItem()->object()->getObject();
            inList = parentObj->getInListEx(true);
            inList.insert(parentObj);

            std::string target = targetObj->getNameInDocument();
            auto targetDoc = targetObj->getDocument();
            for (auto& info : infos) {
                auto& subname = info.subname;
                targetObj = targetDoc->getObject(target.c_str());
                vp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
                    Application::Instance->getViewProvider(targetObj));
                if (!vp) {
                    FC_ERR("Cannot find drop target object " << target);
                    break;
                }

                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if (!doc) {
                    FC_WARN("Cannot find document " << info.doc);
                    continue;
                }
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                    Application::Instance->getViewProvider(obj));
                if (!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                ViewProviderDocumentObject* vpp = nullptr;
                if (da != Qt::LinkAction && info.parentDoc.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if (parentDoc) {
                        auto parent = parentDoc->getObject(info.parent.c_str());
                        vpp = dynamic_cast<ViewProviderDocumentObject*>(
                            Application::Instance->getViewProvider(parent));
                    }
                    if (!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }
                }

                App::DocumentObject* owner = nullptr;
                if (info.ownerDoc.size()) {
                    auto ownerDoc = App::GetApplication().getDocument(info.ownerDoc.c_str());
                    if (ownerDoc)
                        owner = ownerDoc->getObject(info.owner.c_str());
                    if (!owner) {
                        FC_WARN("Cannot find dragging object's top parent " << info.owner);
                        continue;
                    }
                }

                Base::Matrix4D mat;
                App::PropertyPlacement* propPlacement = nullptr;
                if (syncPlacement) {
                    if (info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if (doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if (topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(), nullptr, &mat);
                                if (sobj == obj) {
                                    propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                        obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }
                    else {
                        propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                            obj->getPropertyByName("Placement"));
                        if (propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                auto dropParent = targetParent;

                auto manager = Application::Instance->macroManager();
                std::ostringstream ss;
                if (vpp) {
                    auto lines = manager->getLines();
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    vpp->dragObject(obj);
                    if (manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui, ss.str().c_str());
                    owner = nullptr;
                    subname.clear();
                    ss.str("");

                    obj = doc->getObject(info.obj.c_str());
                    if (!obj || !obj->getNameInDocument()) {
                        FC_WARN("Dropping object deleted: " << info.doc << '#' << info.obj);
                        continue;
                    }
                }

                if (da == Qt::MoveAction) {
                    // Try to adjust relative links to avoid cyclic dependency, may
                    // throw exception if failed
                    ss.str("");
                    ss << Command::getObjectCmd(obj) << ".adjustRelativeLinks("
                        << Command::getObjectCmd(targetObj) << ")";
                    manager->addLine(MacroManager::Gui, ss.str().c_str());

                    std::set<App::DocumentObject*> visited;
                    if (obj->adjustRelativeLinks(inList, &visited)) {
                        inList = parentObj->getInListEx(true);
                        inList.insert(parentObj);

                        // TODO: link adjustment and placement adjustment does
                        // not work together at the moment.
                        propPlacement = nullptr;
                    }
                }

                if (inList.count(obj))
                    FC_THROWM(Base::RuntimeError,
                        "Dependency loop detected for " << obj->getFullName());

                std::string dropName;
                ss.str("");
                if (da == Qt::LinkAction) {
                    auto parentItem = targetItemObj->getParentItem();
                    if (parentItem) {
                        ss << Command::getObjectCmd(
                            parentItem->object()->getObject(), nullptr, ".replaceObject(", true)
                            << Command::getObjectCmd(targetObj) << ","
                            << Command::getObjectCmd(obj) << ")";

                        std::ostringstream ss;

                        dropParent = nullptr;
                        parentItem->getSubName(ss, dropParent);
                        if (dropParent)
                            ss << parentItem->object()->getObject()->getNameInDocument() << '.';
                        else
                            dropParent = parentItem->object()->getObject();
                        ss << obj->getNameInDocument() << '.';
                        dropName = ss.str();
                    }
                    else {
                        TREE_WARN("ignore replace operation without parent");
                        continue;
                    }

                    Gui::Command::runCommand(Gui::Command::App, ss.str().c_str());

                }
                else {
                    ss << Command::getObjectCmd(vp->getObject())
                        << ".ViewObject.dropObject(" << Command::getObjectCmd(obj);
                    if (owner) {
                        ss << "," << Command::getObjectCmd(owner)
                            << ",'" << subname << "',[";
                    }
                    else
                        ss << ",None,'',[";
                    for (auto& sub : info.subs)
                        ss << "'" << sub << "',";
                    ss << "])";
                    auto lines = manager->getLines();
                    dropName = vp->dropObjectEx(obj, owner, subname.c_str(), info.subs);
                    if (manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui, ss.str().c_str());
                    if (dropName.size())
                        dropName = targetSubname.str() + dropName;
                }

                touched = true;

                // Construct the subname pointing to the dropped object
                if (dropName.empty()) {
                    auto pos = targetSubname.tellp();
                    targetSubname << obj->getNameInDocument() << '.' << std::ends;
                    dropName = targetSubname.str();
                    targetSubname.seekp(pos);
                }

                Base::Matrix4D newMat;
                auto sobj = dropParent->getSubObject(dropName.c_str(), nullptr, &newMat);
                if (!sobj) {
                    FC_LOG("failed to find dropped object "
                        << dropParent->getFullName() << '.' << dropName);
                    setSelection = false;
                    continue;
                }

                if (da != Qt::CopyAction && propPlacement) {
                    // try to adjust placement
                    if ((info.dragging && sobj == obj) ||
                        (!info.dragging && sobj->getLinkedObject(false) == obj))
                    {
                        if (!info.dragging)
                            propPlacement = Base::freecad_dynamic_cast<App::PropertyPlacement>(
                                sobj->getPropertyByName("Placement"));
                        if (propPlacement) {
                            newMat *= propPlacement->getValue().inverse().toMatrix();
                            newMat.inverseGauss();
                            Base::Placement pla(newMat * mat);
                            propPlacement->setValueIfChanged(pla);
                        }
                    }
                }
                droppedObjects.emplace_back(dropParent, dropName);
            }
            Base::FlagToggler<> guard(_DisableCheckTopParent);
            if (setSelection && droppedObjects.size()) {
                Selection().selStackPush();
                Selection().clearCompleteSelection();
                for (auto& v : droppedObjects)
                    Selection().addSelection(v.first->getDocument()->getName(),
                        v.first->getNameInDocument(), v.second.c_str());
                Selection().selStackPush();
            }
        }
        catch (const Base::Exception& e) {
            e.ReportException();
            errMsg = e.what();
        }
        catch (std::exception& e) {
            FC_ERR("C++ exception: " << e.what());
            errMsg = e.what();
        }
        catch (...) {
            FC_ERR("Unknown exception");
            errMsg = "Unknown exception";
        }
        if (errMsg.size()) {
            committer.close(true);
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                QString::fromUtf8(errMsg.c_str()));
            return;
        }
    }
    else if (targetItem->type() == TreeWidget::DocumentType) {
        auto targetDocItem = static_cast<DocumentItem*>(targetItem);
        thisDoc = targetDocItem->document()->getDocument();

        std::vector<ItemInfo2> infos;
        infos.reserve(items.size());
        bool syncPlacement = TreeParams::Instance()->SyncPlacement();

        // check if items can be dragged
        for (auto& v : items) {
            auto item = v.first;
            auto obj = item->object()->getObject();
            auto parentItem = item->getParentItem();
            if (!parentItem) {
                if (da == Qt::MoveAction && obj->getDocument() == thisDoc)
                    continue;
            }
            else if (dropOnly || item->myOwner != targetItem) {
                // We will not drag item out of parent if either, 1) the CTRL
                // key is held, or 2) the dragging item is not inside the
                // dropping document tree.
                parentItem = nullptr;
            }
            else if (!parentItem->object()->canDragObjects()
                || !parentItem->object()->canDragObject(obj))
            {
                TREE_ERR("'" << obj->getFullName() << "' cannot be dragged out of '" <<
                    parentItem->object()->getObject()->getFullName() << "'");
                return;
            }
            infos.emplace_back();
            auto& info = infos.back();
            info.doc = obj->getDocument()->getName();
            info.obj = obj->getNameInDocument();
            if (parentItem) {
                auto parent = parentItem->object()->getObject();
                info.parentDoc = parent->getDocument()->getName();
                info.parent = parent->getNameInDocument();
            }
            if (syncPlacement) {
                std::ostringstream ss;
                App::DocumentObject* topParent = nullptr;
                item->getSubName(ss, topParent);
                if (topParent) {
                    info.topDoc = topParent->getDocument()->getName();
                    info.topObj = topParent->getNameInDocument();
                    ss << obj->getNameInDocument() << '.';
                    info.topSubname = ss.str();
                }
            }
        }
        // Because the existence of subname, we must de-select the drag the
        // object manually. Just do a complete clear here for simplicity
        Selection().selStackPush();
        Selection().clearCompleteSelection();

        // Open command
        auto manager = Application::Instance->macroManager();
        App::AutoTransaction committer(
            da == Qt::LinkAction ? "Link object" :
            da == Qt::CopyAction ? "Copy object" : "Move object");
        try {
            std::vector<App::DocumentObject*> droppedObjs;
            for (auto& info : infos) {
                auto doc = App::GetApplication().getDocument(info.doc.c_str());
                if (!doc) continue;
                auto obj = doc->getObject(info.obj.c_str());
                auto vpc = dynamic_cast<ViewProviderDocumentObject*>(
                    Application::Instance->getViewProvider(obj));
                if (!vpc) {
                    FC_WARN("Cannot find dragging object " << info.obj);
                    continue;
                }

                Base::Matrix4D mat;
                App::PropertyPlacement* propPlacement = nullptr;
                if (syncPlacement) {
                    if (info.topObj.size()) {
                        auto doc = App::GetApplication().getDocument(info.topDoc.c_str());
                        if (doc) {
                            auto topObj = doc->getObject(info.topObj.c_str());
                            if (topObj) {
                                auto sobj = topObj->getSubObject(info.topSubname.c_str(), nullptr, &mat);
                                if (sobj == obj) {
                                    propPlacement = dynamic_cast<App::PropertyPlacement*>(
                                        obj->getPropertyByName("Placement"));
                                }
                            }
                        }
                    }
                    else {
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                            obj->getPropertyByName("Placement"));
                        if (propPlacement)
                            mat = propPlacement->getValue().toMatrix();
                    }
                }

                if (da == Qt::LinkAction) {
                    std::string name = thisDoc->getUniqueObjectName("Link");
                    FCMD_DOC_CMD(thisDoc, "addObject('App::Link','" << name << "').setLink("
                        << Command::getObjectCmd(obj) << ")");
                    auto link = thisDoc->getObject(name.c_str());
                    if (!link)
                        continue;
                    FCMD_OBJ_CMD(link, "Label='" << obj->getLinkedObject(true)->Label.getValue() << "'");
                    propPlacement = dynamic_cast<App::PropertyPlacement*>(link->getPropertyByName("Placement"));
                    if (propPlacement)
                        propPlacement->setValueIfChanged(Base::Placement(mat));
                    droppedObjs.push_back(link);
                }
                else if (info.parent.size()) {
                    auto parentDoc = App::GetApplication().getDocument(info.parentDoc.c_str());
                    if (!parentDoc) {
                        FC_WARN("Canont find document " << info.parentDoc);
                        continue;
                    }
                    auto parent = parentDoc->getObject(info.parent.c_str());
                    auto vpp = dynamic_cast<ViewProviderDocumentObject*>(
                        Application::Instance->getViewProvider(parent));
                    if (!vpp) {
                        FC_WARN("Cannot find dragging object's parent " << info.parent);
                        continue;
                    }

                    std::ostringstream ss;
                    ss << Command::getObjectCmd(vpp->getObject())
                        << ".ViewObject.dragObject(" << Command::getObjectCmd(obj) << ')';
                    auto lines = manager->getLines();
                    vpp->dragObject(obj);
                    if (manager->getLines() == lines)
                        manager->addLine(MacroManager::Gui, ss.str().c_str());

                    //make sure it is not part of a geofeaturegroup anymore.
                    //When this has happen we need to handle all removed
                    //objects
                    auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
                    if (grp) {
                        FCMD_OBJ_CMD(grp, "removeObject(" << Command::getObjectCmd(obj) << ")");
                    }

                    // check if the object has been deleted
                    obj = doc->getObject(info.obj.c_str());
                    if (!obj || !obj->getNameInDocument())
                        continue;
                    droppedObjs.push_back(obj);
                    if (propPlacement)
                        propPlacement->setValueIfChanged(Base::Placement(mat));
                }
                else {
                    std::ostringstream ss;
                    ss << "App.getDocument('" << thisDoc->getName() << "')."
                        << (da == Qt::CopyAction ? "copyObject(" : "moveObject(")
                        << Command::getObjectCmd(obj) << ", True)";
                    App::DocumentObject* res = nullptr;
                    if (da == Qt::CopyAction) {
                        auto copied = thisDoc->copyObject({ obj }, true);
                        if (copied.size())
                            res = copied.back();
                    }
                    else
                        res = thisDoc->moveObject(obj, true);
                    if (res) {
                        propPlacement = dynamic_cast<App::PropertyPlacement*>(
                            res->getPropertyByName("Placement"));
                        if (propPlacement)
                            propPlacement->setValueIfChanged(Base::Placement(mat));
                        droppedObjs.push_back(res);
                    }
                    manager->addLine(MacroManager::App, ss.str().c_str());
                }
            }
            touched = true;
            Base::FlagToggler<> guard(_DisableCheckTopParent);
            Selection().setSelection(thisDoc->getName(), droppedObjs);

        }
        catch (const Base::Exception& e) {
            e.ReportException();
            errMsg = e.what();
        }
        catch (std::exception& e) {
            FC_ERR("C++ exception: " << e.what());
            errMsg = e.what();
        }
        catch (...) {
            FC_ERR("Unknown exception");
            errMsg = "Unknown exception";
        }
        if (errMsg.size()) {
            committer.close(true);
            QMessageBox::critical(getMainWindow(), QObject::tr("Drag & drop failed"),
                QString::fromUtf8(errMsg.c_str()));
            return;
        }
    }

    if (touched && TreeParams::Instance()->RecomputeOnDrop())
        thisDoc->recompute();

    if (touched && TreeParams::Instance()->SyncView()) {
        auto gdoc = Application::Instance->getDocument(thisDoc);
        if (gdoc)
            gdoc->setActiveView();
    }
}

void TreeWidget::drawRow(QPainter* painter, const QStyleOptionViewItem& options, const QModelIndex& index) const
{
    QTreeWidget::drawRow(painter, options, index);
    // Set the text and highlighted text color of a hidden object to a dark
    //QTreeWidgetItem * item = itemFromIndex(index);
    //if (item->type() == ObjectType && !(static_cast<DocumentObjectItem*>(item)->previousStatus & 1)) {
    //    QStyleOptionViewItem opt(options);
    //    opt.state ^= QStyle::State_Enabled;
    //    QColor c = opt.palette.color(QPalette::Inactive, QPalette::Dark);
    //    opt.palette.setColor(QPalette::Inactive, QPalette::Text, c);
    //    opt.palette.setColor(QPalette::Inactive, QPalette::HighlightedText, c);
    //    QTreeWidget::drawRow(painter, opt, index);
    //}
    //else {
    //    QTreeWidget::drawRow(painter, options, index);
    //}
}

void TreeWidget::slotNewDocument(const Gui::Document& Doc, bool isMainDoc)
{
    if (Doc.getDocument()->testStatus(App::Document::TempDoc))
        return;
    DocumentItem* item = new DocumentItem(&Doc, this->rootItem);
    if (isMainDoc)
        this->expandItem(item);
    item->setIcon(0, *documentPixmap);
    item->setText(0, QString::fromUtf8(Doc.getDocument()->Label.getValue()));
    DocumentMap[&Doc] = item;
}

void TreeWidget::slotStartOpenDocument() {
    // No longer required. Visibility is now handled inside onUpdateStatus() by
    // UpdateDisabler.
    //
    // setVisible(false);
}

void TreeWidget::slotFinishOpenDocument() {
    // setVisible(true);
}

void TreeWidget::onReloadDoc() {
    if (!this->contextItem || this->contextItem->type() != DocumentType)
        return;
    DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
    App::Document* doc = docitem->document()->getDocument();
    std::string name = doc->FileName.getValue();
    Application::Instance->reopen(doc);
    for (auto& v : DocumentMap) {
        if (name == v.first->getDocument()->FileName.getValue()) {
            scrollToItem(v.second);
            App::GetApplication().setActiveDocument(v.first->getDocument());
            break;
        }
    }
}

void TreeWidget::onCloseDoc()
{
    if (!this->contextItem || this->contextItem->type() != DocumentType)
        return;
    try {
        DocumentItem* docitem = static_cast<DocumentItem*>(this->contextItem);
        Gui::Document* gui = docitem->document();
        App::Document* doc = gui->getDocument();
        if (gui->canClose(true, true))
            Command::doCommand(Command::Doc, "App.closeDocument(\"%s\")", doc->getName());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
    catch (std::exception& e) {
        FC_ERR("C++ exception: " << e.what());
    }
    catch (...) {
        FC_ERR("Unknown exception");
    }
}

void TreeWidget::slotRenameDocument(const Gui::Document& Doc)
{
    // do nothing here
    Q_UNUSED(Doc);
}

void TreeWidget::slotChangedViewObject(const Gui::ViewProvider& vp, const App::Property& prop)
{
    if (!App::GetApplication().isRestoring()
        && vp.isDerivedFrom(ViewProviderDocumentObject::getClassTypeId()))
    {
        const auto& vpd = static_cast<const ViewProviderDocumentObject&>(vp);
        if (&prop == &vpd.ShowInTree) {
            ChangedObjects.emplace(vpd.getObject(), 0);
            _updateStatus();
        }
    }
}

void TreeWidget::slotTouchedObject(const App::DocumentObject& obj) {
    ChangedObjects.emplace(const_cast<App::DocumentObject*>(&obj), 0);
    _updateStatus();
}

void TreeWidget::slotShowHidden(const Gui::Document& Doc)
{
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end())
        it->second->updateItemsVisibility(it->second, it->second->showHidden());
}

void TreeWidget::slotRelabelDocument(const Gui::Document& Doc)
{
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        it->second->setText(0, QString::fromUtf8(Doc.getDocument()->Label.getValue()));
    }
}

void TreeWidget::slotActiveDocument(const Gui::Document& Doc)
{
    auto jt = DocumentMap.find(&Doc);
    if (jt == DocumentMap.end())
        return; // signal is emitted before the item gets created
    int displayMode = TreeParams::Instance()->DocumentMode();
    for (auto it = DocumentMap.begin();
        it != DocumentMap.end(); ++it)
    {
        QFont f = it->second->font(0);
        f.setBold(it == jt);
        it->second->setHidden(0 == displayMode && it != jt);
        if (2 == displayMode) {
            it->second->setExpanded(it == jt);
        }
        // this must be done as last step
        it->second->setFont(0, f);
    }
}

struct UpdateDisabler {
    QWidget& widget;
    int& blocked;
    bool visible;
    bool focus;

    // Note! DO NOT block signal here, or else
    // QTreeWidgetItem::setChildIndicatorPolicy() does not work
    UpdateDisabler(QWidget& w, int& blocked)
        : widget(w), blocked(blocked), visible(false), focus(false)
    {
        if (++blocked > 1)
            return;
        focus = widget.hasFocus();
        visible = widget.isVisible();
        if (visible) {
            // setUpdatesEnabled(false) does not seem to speed up anything.
            // setVisible(false) on the other hand makes QTreeWidget::setData
            // (i.e. any change to QTreeWidgetItem) faster by 10+ times.
            //
            // widget.setUpdatesEnabled(false);

            widget.setVisible(false);
        }
    }
    ~UpdateDisabler() {
        if (blocked <= 0 || --blocked != 0)
            return;

        if (visible) {
            widget.setVisible(true);
            // widget.setUpdatesEnabled(true);
            if (focus)
                widget.setFocus();
        }
    }
};

void TreeWidget::onUpdateStatus(void)
{
    if (this->state() == DraggingState || App::GetApplication().isRestoring()) {
        _updateStatus();
        return;
    }

    for (auto& v : DocumentMap) {
        if (v.first->isPerformingTransaction()) {
            // We have to delay item creation until undo/redo is done, because the
            // object re-creation while in transaction may break tree view item
            // update logic. For example, a parent object re-created before its
            // children, but the parent's link property already contains all the
            // (detached) children.
            _updateStatus();
            return;
        }
    }

    FC_LOG("begin update status");

    UpdateDisabler disabler(*this, updateBlocked);

    std::vector<App::DocumentObject*> errors;

    // Checking for new objects
    for (auto& v : NewObjects) {
        auto doc = App::GetApplication().getDocument(v.first.c_str());
        if (!doc)
            continue;
        auto gdoc = Application::Instance->getDocument(doc);
        if (!gdoc)
            continue;
        auto docItem = getDocumentItem(gdoc);
        if (!docItem)
            continue;
        for (auto id : v.second) {
            auto obj = doc->getObjectByID(id);
            if (!obj)
                continue;
            if (obj->isError())
                errors.push_back(obj);
            if (docItem->ObjectMap.count(obj))
                continue;
            auto vpd = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(gdoc->getViewProvider(obj));
            if (vpd)
                docItem->createNewItem(*vpd);
        }
    }
    NewObjects.clear();

    // Update children of changed objects
    for (auto& v : ChangedObjects) {
        auto obj = v.first;

        auto iter = ObjectTable.find(obj);
        if (iter == ObjectTable.end())
            continue;

        if (v.second.test(CS_Error) && obj->isError())
            errors.push_back(obj);

        if (iter->second.size()) {
            auto data = *iter->second.begin();
            bool itemHidden = !data->viewObject->showInTree();
            if (data->itemHidden != itemHidden) {
                for (auto& data : iter->second) {
                    data->itemHidden = itemHidden;
                    if (data->docItem->showHidden())
                        continue;
                    for (auto item : data->items)
                        item->setHidden(itemHidden);
                }
            }
        }

        updateChildren(iter->first, iter->second, v.second.test(CS_Output), false);
    }
    ChangedObjects.clear();

    FC_LOG("update item status");
    TimingInit();
    for (auto pos = DocumentMap.begin(); pos != DocumentMap.end(); ++pos) {
        pos->second->testStatus();
    }
    TimingPrint();

    // Checking for just restored documents
    for (auto& v : DocumentMap) {
        auto docItem = v.second;

        for (auto obj : docItem->PopulateObjects)
            docItem->populateObject(obj);
        docItem->PopulateObjects.clear();

        auto doc = v.first->getDocument();

        if (!docItem->connectChgObject.connected()) {
            docItem->connectChgObject = docItem->document()->signalChangedObject.connect(
                boost::bind(&TreeWidget::slotChangeObject, this, bp::_1, bp::_2));
            docItem->connectTouchedObject = doc->signalTouchedObject.connect(
                boost::bind(&TreeWidget::slotTouchedObject, this, bp::_1));
        }

        if (doc->testStatus(App::Document::PartialDoc))
            docItem->setIcon(0, *documentPartialPixmap);
        else if (docItem->_ExpandInfo) {
            for (auto& entry : *docItem->_ExpandInfo) {
                const char* name = entry.first.c_str();
                bool legacy = name[0] == '*';
                if (legacy)
                    ++name;
                auto obj = doc->getObject(name);
                if (!obj)
                    continue;
                auto iter = docItem->ObjectMap.find(obj);
                if (iter == docItem->ObjectMap.end())
                    continue;
                if (iter->second->rootItem)
                    docItem->restoreItemExpansion(entry.second, iter->second->rootItem);
                else if (legacy && iter->second->items.size()) {
                    auto item = *iter->second->items.begin();
                    item->setExpanded(true);
                }
            }
        }
        docItem->_ExpandInfo.reset();
    }

    if (Selection().hasSelection() && !selectTimer->isActive() && !this->isSelectionBlocked()) {
        this->blockSelection(true);
        currentDocItem = nullptr;
        for (auto& v : DocumentMap) {
            v.second->setSelected(false);
            v.second->selectItems();
        }
        this->blockSelection(false);
    }

    auto activeDocItem = getDocumentItem(Application::Instance->activeDocument());

    QTreeWidgetItem* errItem = nullptr;
    for (auto obj : errors) {
        DocumentObjectDataPtr data;
        if (activeDocItem) {
            auto it = activeDocItem->ObjectMap.find(obj);
            if (it != activeDocItem->ObjectMap.end())
                data = it->second;
        }
        if (!data) {
            auto docItem = getDocumentItem(
                Application::Instance->getDocument(obj->getDocument()));
            if (docItem) {
                auto it = docItem->ObjectMap.find(obj);
                if (it != docItem->ObjectMap.end())
                    data = it->second;
            }
        }
        if (data) {
            auto item = data->rootItem;
            if (!item && data->items.size()) {
                item = *data->items.begin();
                data->docItem->showItem(item, false, true);
            }
            if (!errItem)
                errItem = item;
        }
    }
    if (errItem)
        scrollToItem(errItem);

    updateGeometries();
    statusTimer->stop();

    FC_LOG("done update status");
}

void TreeWidget::onItemEntered(QTreeWidgetItem* item)
{
    // object item selected
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
        objItem->displayStatusInfo();

        if (TreeParams::Instance()->PreSelection()) {
            int timeout = TreeParams::Instance()->PreSelectionDelay();
            if (timeout < 0)
                timeout = 1;
            if (preselectTime.elapsed() < timeout)
                onPreSelectTimer();
            else {
                timeout = TreeParams::Instance()->PreSelectionTimeout();
                if (timeout < 0)
                    timeout = 1;
                preselectTimer->start(timeout);
                Selection().rmvPreselect();
            }
        }
    }
    else if (TreeParams::Instance()->PreSelection())
        Selection().rmvPreselect();
}

void TreeWidget::leaveEvent(QEvent*) {
    if (!updateBlocked && TreeParams::Instance()->PreSelection()) {
        preselectTimer->stop();
        Selection().rmvPreselect();
    }
}

void TreeWidget::onPreSelectTimer() {
    if (!TreeParams::Instance()->PreSelection())
        return;
    auto item = itemAt(viewport()->mapFromGlobal(QCursor::pos()));
    if (!item || item->type() != TreeWidget::ObjectType)
        return;

    preselectTime.restart();
    DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
    auto vp = objItem->object();
    auto obj = vp->getObject();
    std::ostringstream ss;
    App::DocumentObject* parent = nullptr;
    objItem->getSubName(ss, parent);
    if (!parent)
        parent = obj;
    else if (!obj->redirectSubName(ss, parent, nullptr))
        ss << obj->getNameInDocument() << '.';
    Selection().setPreselect(parent->getDocument()->getName(), parent->getNameInDocument(),
        ss.str().c_str(), 0, 0, 0, SelectionChanges::MsgSource::TreeView);
}

void TreeWidget::onItemCollapsed(QTreeWidgetItem* item)
{
    // object item collapsed
    if (item && item->type() == TreeWidget::ObjectType) {
        static_cast<DocumentObjectItem*>(item)->setExpandedStatus(false);
    }
}

void TreeWidget::onItemExpanded(QTreeWidgetItem* item)
{
    // object item expanded
    if (item && item->type() == TreeWidget::ObjectType) {
        DocumentObjectItem* objItem = static_cast<DocumentObjectItem*>(item);
        objItem->setExpandedStatus(true);
        objItem->getOwnerDocument()->populateItem(objItem, false, false);
    }
}

void TreeWidget::scrollItemToTop()
{
    auto doc = Application::Instance->activeDocument();
    for (auto tree : Instances) {
        if (!tree->isSelectionAttached() || tree->isSelectionBlocked())
            continue;

        tree->_updateStatus(false);

        if (doc && Gui::Selection().hasSelection(doc->getDocument()->getName(), ResolveMode::NoResolve)) {
            auto it = tree->DocumentMap.find(doc);
            if (it != tree->DocumentMap.end()) {
                bool lock = tree->blockSelection(true);
                it->second->selectItems(DocumentItem::SR_FORCE_EXPAND);
                tree->blockSelection(lock);
            }
        }
        else {
            tree->blockSelection(true);
            for (int i = 0; i < tree->rootItem->childCount(); i++) {
                auto docItem = dynamic_cast<DocumentItem*>(tree->rootItem->child(i));
                if (!docItem)
                    continue;
                auto doc = docItem->document()->getDocument();
                if (Gui::Selection().hasSelection(doc->getName())) {
                    tree->currentDocItem = docItem;
                    docItem->selectItems(DocumentItem::SR_FORCE_EXPAND);
                    tree->currentDocItem = nullptr;
                    break;
                }
            }
            tree->blockSelection(false);
        }
        tree->selectTimer->stop();
        tree->_updateStatus(false);
    }
}

void TreeWidget::expandSelectedItems(TreeItemMode mode)
{
    if (!isSelectionAttached())
        return;

    for (auto item : selectedItems()) {
        switch (mode) {
        case TreeItemMode::ExpandPath: {
            QTreeWidgetItem* parentItem = item->parent();
            while (parentItem) {
                parentItem->setExpanded(true);
                parentItem = parentItem->parent();
            }
            item->setExpanded(true);
            break;
        }
        case TreeItemMode::ExpandItem:
            item->setExpanded(true);
            break;
        case TreeItemMode::CollapseItem:
            item->setExpanded(false);
            break;
        case TreeItemMode::ToggleItem:
            if (item->isExpanded())
                item->setExpanded(false);
            else
                item->setExpanded(true);
            break;
        }
    }
}

void TreeWidget::setupText()
{
    this->headerItem()->setText(0, tr("Labels & Attributes"));
    this->headerItem()->setText(1, tr("Description"));
    this->rootItem->setText(0, tr("Application"));

    this->showHiddenAction->setText(tr("Show hidden items"));
    this->showHiddenAction->setStatusTip(tr("Show hidden tree view items"));

    this->hideInTreeAction->setText(tr("Hide item"));
    this->hideInTreeAction->setStatusTip(tr("Hide the item in tree"));

    this->createGroupAction->setText(tr("Create group..."));
    this->createGroupAction->setStatusTip(tr("Create a group"));

    this->relabelObjectAction->setText(tr("Rename"));
    this->relabelObjectAction->setStatusTip(tr("Rename object"));

    this->finishEditingAction->setText(tr("Finish editing"));
    this->finishEditingAction->setStatusTip(tr("Finish editing object"));

    this->selectDependentsAction->setText(tr("Add dependent objects to selection"));
    this->selectDependentsAction->setStatusTip(tr("Adds all dependent objects to the selection"));

    this->closeDocAction->setText(tr("Close document"));
    this->closeDocAction->setStatusTip(tr("Close the document"));

    this->reloadDocAction->setText(tr("Reload document"));
    this->reloadDocAction->setStatusTip(tr("Reload a partially loaded document"));

    this->skipRecomputeAction->setText(tr("Skip recomputes"));
    this->skipRecomputeAction->setStatusTip(tr("Enable or disable recomputations of document"));

    this->allowPartialRecomputeAction->setText(tr("Allow partial recomputes"));
    this->allowPartialRecomputeAction->setStatusTip(
        tr("Enable or disable recomputating editing object when 'skip recomputation' is enabled"));

    this->markRecomputeAction->setText(tr("Mark to recompute"));
    this->markRecomputeAction->setStatusTip(tr("Mark this object to be recomputed"));
    this->markRecomputeAction->setIcon(BitmapFactory().iconFromTheme("Std_MarkToRecompute"));

    this->recomputeObjectAction->setText(tr("Recompute object"));
    this->recomputeObjectAction->setStatusTip(tr("Recompute the selected object"));
    this->recomputeObjectAction->setIcon(BitmapFactory().iconFromTheme("view-refresh"));
}

void TreeWidget::syncView(ViewProviderDocumentObject* vp)
{
    if (currentDocItem && TreeParams::Instance()->SyncView()) {
        bool focus = hasFocus();
        currentDocItem->document()->setActiveView(vp);
        if (focus)
            setFocus();
    }
}

void TreeWidget::onShowHidden()
{
    if (!this->contextItem)
        return;
    DocumentItem* docItem = nullptr;
    if (this->contextItem->type() == DocumentType)
        docItem = static_cast<DocumentItem*>(contextItem);
    else if (this->contextItem->type() == ObjectType)
        docItem = static_cast<DocumentObjectItem*>(contextItem)->getOwnerDocument();
    if (docItem)
        docItem->setShowHidden(showHiddenAction->isChecked());
}

void TreeWidget::onHideInTree()
{
    if (this->contextItem && this->contextItem->type() == ObjectType) {
        auto item = static_cast<DocumentObjectItem*>(contextItem);
        item->object()->ShowInTree.setValue(!hideInTreeAction->isChecked());
    }
}

void TreeWidget::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange)
        setupText();

    QTreeWidget::changeEvent(e);
}

void TreeWidget::onItemSelectionChanged()
{
    if (!this->isSelectionAttached()
        || this->isSelectionBlocked()
        || updateBlocked)
        return;

    _LastSelectedTreeWidget = this;

    // block tmp. the connection to avoid to notify us ourself
    bool lock = this->blockSelection(true);

    if (selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    auto selItems = selectedItems();

    // do not allow document item multi-selection
    if (selItems.size()) {
        auto firstType = selItems.back()->type();
        for (auto it = selItems.begin(); it != selItems.end();) {
            auto item = *it;
            if ((firstType == ObjectType && item->type() != ObjectType)
                || (firstType == DocumentType && item != selItems.back()))
            {
                item->setSelected(false);
                it = selItems.erase(it);
            }
            else
                ++it;
        }
    }

    if (selItems.size() <= 1) {
        if (TreeParams::Instance()->RecordSelection())
            Gui::Selection().selStackPush();

        // This special handling to deal with possible discrepancy of
        // Gui.Selection and Tree view selection because of newly added
        // DocumentObject::redirectSubName()
        Selection().clearCompleteSelection();
        DocumentObjectItem* item = nullptr;
        if (selItems.size()) {
            if (selItems.front()->type() == ObjectType)
                item = static_cast<DocumentObjectItem*>(selItems.front());
            else if (selItems.front()->type() == DocumentType) {
                auto ditem = static_cast<DocumentItem*>(selItems.front());
                if (TreeParams::Instance()->SyncView()) {
                    bool focus = hasFocus();
                    ditem->document()->setActiveView();
                    if (focus)
                        setFocus();
                }
                // For triggering property editor refresh
                Gui::Selection().signalSelectionChanged(SelectionChanges());
            }
        }
        for (auto& v : DocumentMap) {
            currentDocItem = v.second;
            v.second->clearSelection(item);
            currentDocItem = nullptr;
        }
        if (TreeParams::Instance()->RecordSelection())
            Gui::Selection().selStackPush();
    }
    else {
        for (auto pos = DocumentMap.begin(); pos != DocumentMap.end(); ++pos) {
            currentDocItem = pos->second;
            pos->second->updateSelection(pos->second);
            currentDocItem = nullptr;
        }
        if (TreeParams::Instance()->RecordSelection())
            Gui::Selection().selStackPush(true, true);
    }

    this->blockSelection(lock);
}

static bool isSelectionCheckBoxesEnabled() {
    return TreeParams::Instance()->CheckBoxesSelection();
}

void TreeWidget::synchronizeSelectionCheckBoxes() {
    const bool useCheckBoxes = isSelectionCheckBoxesEnabled();
    for (QTreeWidgetItemIterator it(this); *it; ++it) {
        if (const auto item = dynamic_cast<DocumentObjectItem*>(*it)) {
            if (useCheckBoxes)
                item->QTreeWidgetItem::setCheckState(0, item->isSelected() ? Qt::Checked : Qt::Unchecked);
            else
                item->setData(0, Qt::CheckStateRole, QVariant());
        }
    }
    resizeColumnToContents(0);
}

QList<QTreeWidgetItem*> TreeWidget::childrenOfItem(const QTreeWidgetItem& item) const {
    QList children = QList<QTreeWidgetItem*>();

    // check item is in this tree
    if (!this->indexFromItem(&item).isValid())
        return children;

    for (int i = 0; i < item.childCount(); i++) {
        children.append(item.child(i));
    }
    return children;
}

void TreeWidget::onItemChanged(QTreeWidgetItem* item, int column) {
    if (column == 0 && isSelectionCheckBoxesEnabled()) {
        bool selected = item->isSelected();
        bool checked = item->checkState(0) == Qt::Checked;
        if (checked != selected) {
            item->setSelected(checked);
        }
    }
}

void TreeWidget::onSelectTimer() {

    _updateStatus(false);

    bool syncSelect = TreeParams::Instance()->SyncSelection();
    bool locked = this->blockSelection(true);
    if (Selection().hasSelection()) {
        for (auto& v : DocumentMap) {
            v.second->setSelected(false);
            currentDocItem = v.second;
            v.second->selectItems(syncSelect ? DocumentItem::SR_EXPAND : DocumentItem::SR_SELECT);
            currentDocItem = nullptr;
        }
    }
    else {
        for (auto& v : DocumentMap)
            v.second->clearSelection();
    }
    this->blockSelection(locked);
    selectTimer->stop();
    return;
}

void TreeWidget::onSelectionChanged(const SelectionChanges& msg)
{
    switch (msg.Type)
    {
    case SelectionChanges::AddSelection:
    case SelectionChanges::RmvSelection:
    case SelectionChanges::SetSelection:
    case SelectionChanges::ClrSelection: {
        int timeout = TreeParams::Instance()->SelectionTimeout();
        if (timeout <= 0)
            timeout = 1;
        selectTimer->start(timeout);
        break;
    }
    default:
        break;
    }
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreePanel */

TreePanel::TreePanel(const char* name, QWidget* parent)
    : QWidget(parent)
{
    this->treeWidget = new TreeWidget(name, this);
    int indent = TreeParams::Instance()->Indentation();
    if (indent)
        this->treeWidget->setIndentation(indent);

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin(0);
    pLayout->addWidget(this->treeWidget);
    connect(this->treeWidget, SIGNAL(emitSearchObjects()),
        this, SLOT(showEditor()));

    this->searchBox = new Gui::ExpressionLineEdit(this, true);
    static_cast<ExpressionLineEdit*>(this->searchBox)->setExactMatch(Gui::ExpressionParameter::instance()->isExactMatch());
    pLayout->addWidget(this->searchBox);
    this->searchBox->hide();
    this->searchBox->installEventFilter(this);
    this->searchBox->setPlaceholderText(tr("Search"));
    connect(this->searchBox, SIGNAL(returnPressed()),
        this, SLOT(accept()));
    connect(this->searchBox, SIGNAL(textChanged(QString)),
        this, SLOT(itemSearch(QString)));
}

TreePanel::~TreePanel()
{
}

void TreePanel::accept()
{
    QString text = this->searchBox->text();
    hideEditor();
    this->treeWidget->setFocus();
    this->treeWidget->itemSearch(text, true);
}

bool TreePanel::eventFilter(QObject* obj, QEvent* ev)
{
    if (obj != this->searchBox)
        return false;

    if (ev->type() == QEvent::KeyPress) {
        bool consumed = false;
        int key = static_cast<QKeyEvent*>(ev)->key();
        switch (key) {
        case Qt::Key_Escape:
            hideEditor();
            consumed = true;
            treeWidget->setFocus();
            break;

        default:
            break;
        }

        return consumed;
    }

    return false;
}

void TreePanel::showEditor()
{
    this->searchBox->show();
    this->searchBox->setFocus();
    this->treeWidget->startItemSearch(searchBox);
}

void TreePanel::hideEditor()
{
    static_cast<ExpressionLineEdit*>(this->searchBox)->setDocumentObject(nullptr);
    this->searchBox->clear();
    this->searchBox->hide();
    this->treeWidget->resetItemSearch();
    auto sels = this->treeWidget->selectedItems();
    if (sels.size())
        this->treeWidget->scrollToItem(sels.front());
}

void TreePanel::itemSearch(const QString& text)
{
    this->treeWidget->itemSearch(text, false);
}

// ----------------------------------------------------------------------------

/* TRANSLATOR Gui::TreeDockWidget */

TreeDockWidget::TreeDockWidget(Gui::Document* pcDocument, QWidget* parent)
    : DockWindow(pcDocument, parent)
{
    setWindowTitle(tr("Tree view"));
    auto panel = new TreePanel("TreeView", this);
    QGridLayout* pLayout = new QGridLayout(this);
    pLayout->setSpacing(0);
    pLayout->setMargin(0);
    pLayout->addWidget(panel, 0, 0);
}

TreeDockWidget::~TreeDockWidget()
{
}

void TreeWidget::selectLinkedObject(App::DocumentObject* linked) {
    if (!isSelectionAttached() || isSelectionBlocked())
        return;

    auto linkedVp = Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
        Application::Instance->getViewProvider(linked));
    if (!linkedVp) {
        TREE_ERR("invalid linked view provider");
        return;
    }
    auto linkedDoc = getDocumentItem(linkedVp->getDocument());
    if (!linkedDoc) {
        TREE_ERR("cannot find document of linked object");
        return;
    }

    if (selectTimer->isActive())
        onSelectTimer();
    else
        _updateStatus(false);

    auto it = linkedDoc->ObjectMap.find(linked);
    if (it == linkedDoc->ObjectMap.end()) {
        TREE_ERR("cannot find tree item of linked object");
        return;
    }
    auto linkedItem = it->second->rootItem;
    if (!linkedItem)
        linkedItem = *it->second->items.begin();

    if (linkedDoc->showItem(linkedItem, true))
        scrollToItem(linkedItem);

    if (linkedDoc->document()->getDocument() != App::GetApplication().getActiveDocument()) {
        bool focus = hasFocus();
        linkedDoc->document()->setActiveView(linkedItem->object());
        if (focus)
            setFocus();
    }
}

// ----------------------------------------------------------------------------

DocumentItem::DocumentItem(const Gui::Document* doc, QTreeWidgetItem* parent)
    : QTreeWidgetItem(parent, TreeWidget::DocumentType), pDocument(const_cast<Gui::Document*>(doc))
{
    // Setup connections
    connectNewObject = doc->signalNewObject.connect(boost::bind(&DocumentItem::slotNewObject, this, bp::_1));
    connectDelObject = doc->signalDeletedObject.connect(
        boost::bind(&TreeWidget::slotDeleteObject, getTree(), bp::_1));
    if (!App::GetApplication().isRestoring()) {
        connectChgObject = doc->signalChangedObject.connect(
            boost::bind(&TreeWidget::slotChangeObject, getTree(), bp::_1, bp::_2));
        connectTouchedObject = doc->getDocument()->signalTouchedObject.connect(
            boost::bind(&TreeWidget::slotTouchedObject, getTree(), bp::_1));
    }
    connectEdtObject = doc->signalInEdit.connect(boost::bind(&DocumentItem::slotInEdit, this, bp::_1));
    connectResObject = doc->signalResetEdit.connect(boost::bind(&DocumentItem::slotResetEdit, this, bp::_1));
    connectHltObject = doc->signalHighlightObject.connect(
        boost::bind(&DocumentItem::slotHighlightObject, this, bp::_1, bp::_2, bp::_3, bp::_4, bp::_5));
    connectExpObject = doc->signalExpandObject.connect(
        boost::bind(&DocumentItem::slotExpandObject, this, bp::_1, bp::_2, bp::_3, bp::_4));
    connectScrObject = doc->signalScrollToObject.connect(boost::bind(&DocumentItem::slotScrollToObject, this, bp::_1));
    auto adoc = doc->getDocument();
    connectRecomputed = adoc->signalRecomputed.connect(boost::bind(&DocumentItem::slotRecomputed, this, bp::_1, bp::_2));
    connectRecomputedObj = adoc->signalRecomputedObject.connect(
        boost::bind(&DocumentItem::slotRecomputedObject, this, bp::_1));

    setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable/*|Qt::ItemIsEditable*/);

    treeName = getTree()->getTreeName();
}

DocumentItem::~DocumentItem()
{
    connectNewObject.disconnect();
    connectDelObject.disconnect();
    connectChgObject.disconnect();
    connectTouchedObject.disconnect();
    connectEdtObject.disconnect();
    connectResObject.disconnect();
    connectHltObject.disconnect();
    connectExpObject.disconnect();
    connectScrObject.disconnect();
    connectRecomputed.disconnect();
    connectRecomputedObj.disconnect();
}

TreeWidget* DocumentItem::getTree() const {
    return static_cast<TreeWidget*>(treeWidget());
}

const char* DocumentItem::getTreeName() const {
    return treeName;
}

#define FOREACH_ITEM(_item, _obj) \
    auto _it = ObjectMap.end();\
    if(_obj.getObject() && _obj.getObject()->getNameInDocument())\
        _it = ObjectMap.find(_obj.getObject());\
    if(_it != ObjectMap.end()) {\
        for(auto _item : _it->second->items) {

#define FOREACH_ITEM_ALL(_item) \
    for(auto _v : ObjectMap) {\
        for(auto _item : _v.second->items) {

#define END_FOREACH_ITEM }}


void DocumentItem::slotInEdit(const Gui::ViewProviderDocumentObject& v)
{
    (void)v;

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
    unsigned long col = hGrp->GetUnsigned("TreeEditColor", 4294902015);
    QColor color((col >> 24) & 0xff, (col >> 16) & 0xff, (col >> 8) & 0xff);

    if (!getTree()->editingItem) {
        auto doc = Application::Instance->editDocument();
        if (!doc)
            return;
        ViewProviderDocumentObject* parentVp = nullptr;
        std::string subname;
        auto vp = doc->getInEdit(&parentVp, &subname);
        if (!parentVp)
            parentVp = dynamic_cast<ViewProviderDocumentObject*>(vp);
        if (parentVp)
            getTree()->editingItem = findItemByObject(true, parentVp->getObject(), subname.c_str());
    }

    if (getTree()->editingItem)
        getTree()->editingItem->setBackground(0, color);
    else {
        FOREACH_ITEM(item, v)
            item->setBackground(0, color);
        END_FOREACH_ITEM
    }
}

void DocumentItem::slotResetEdit(const Gui::ViewProviderDocumentObject& v)
{
    auto tree = getTree();
    FOREACH_ITEM_ALL(item)
        if (tree->editingItem) {
            if (item == tree->editingItem) {
                item->setData(0, Qt::BackgroundRole, QVariant());
                break;
            }
        }
        else if (item->object() == &v)
            item->setData(0, Qt::BackgroundRole, QVariant());
    END_FOREACH_ITEM
        tree->editingItem = nullptr;
}

void DocumentItem::slotNewObject(const Gui::ViewProviderDocumentObject& obj) {
    if (!obj.getObject() || !obj.getObject()->getNameInDocument()) {
        FC_ERR("view provider not attached");
        return;
    }
    getTree()->NewObjects[pDocument->getDocument()->getName()].push_back(obj.getObject()->getID());
    getTree()->_updateStatus();
}

bool DocumentItem::createNewItem(const Gui::ViewProviderDocumentObject& obj,
    QTreeWidgetItem* parent, int index, DocumentObjectDataPtr data)
{
    const char* name;
    if (!obj.getObject() ||
        !(name = obj.getObject()->getNameInDocument()) ||
        obj.getObject()->testStatus(App::PartialObject))
        return false;

    if (!data) {
        auto& pdata = ObjectMap[obj.getObject()];
        if (!pdata) {
            pdata = std::make_shared<DocumentObjectData>(
                this, const_cast<ViewProviderDocumentObject*>(&obj));
            auto& entry = getTree()->ObjectTable[obj.getObject()];
            if (entry.size())
                pdata->updateChildren(*entry.begin());
            else
                pdata->updateChildren(true);
            entry.insert(pdata);
        }
        else if (pdata->rootItem && parent == nullptr) {
            Base::Console().Warning("DocumentItem::slotNewObject: Cannot add view provider twice.\n");
            return false;
        }
        data = pdata;
    }

    DocumentObjectItem* item = new DocumentObjectItem(this, data);
    if (!parent || parent == this) {
        parent = this;
        data->rootItem = item;
        if (index < 0)
            index = findRootIndex(obj.getObject());
    }
    if (index < 0)
        parent->addChild(item);
    else
        parent->insertChild(index, item);
    assert(item->parent() == parent);
    item->setText(0, QString::fromUtf8(data->label.c_str()));
    if (data->label2.size())
        item->setText(1, QString::fromUtf8(data->label2.c_str()));
    if (!obj.showInTree() && !showHidden())
        item->setHidden(true);
    item->testStatus(true);

    populateItem(item);
    return true;
}

ViewProviderDocumentObject* DocumentItem::getViewProvider(App::DocumentObject* obj) {
    return Base::freecad_dynamic_cast<ViewProviderDocumentObject>(
            Application::Instance->getViewProvider(obj));
}

void TreeWidget::slotDeleteDocument(const Gui::Document& Doc)
{
    NewObjects.erase(Doc.getDocument()->getName());
    auto it = DocumentMap.find(&Doc);
    if (it != DocumentMap.end()) {
        UpdateDisabler disabler(*this, updateBlocked);
        auto docItem = it->second;
        for (auto& v : docItem->ObjectMap) {
            for (auto item : v.second->items)
                item->myOwner = nullptr;
            auto obj = v.second->viewObject->getObject();
            if (obj->getDocument() == Doc.getDocument()) {
                _slotDeleteObject(*v.second->viewObject, docItem);
                continue;
            }
            auto it = ObjectTable.find(obj);
            assert(it != ObjectTable.end());
            assert(it->second.size() > 1);
            it->second.erase(v.second);
        }
        this->rootItem->takeChild(this->rootItem->indexOfChild(docItem));
        delete docItem;
        DocumentMap.erase(it);
    }
}

void TreeWidget::slotDeleteObject(const Gui::ViewProviderDocumentObject& view) {
    _slotDeleteObject(view, nullptr);
}

void TreeWidget::_slotDeleteObject(const Gui::ViewProviderDocumentObject& view, DocumentItem* deletingDoc)
{
    auto obj = view.getObject();
    auto itEntry = ObjectTable.find(obj);
    if (itEntry == ObjectTable.end())
        return;

    if (itEntry->second.empty()) {
        ObjectTable.erase(itEntry);
        return;
    }

    TREE_LOG("delete object " << obj->getFullName());

    bool needUpdate = false;

    for (auto data : itEntry->second) {
        DocumentItem* docItem = data->docItem;
        if (docItem == deletingDoc)
            continue;

        auto doc = docItem->document()->getDocument();
        auto& items = data->items;

        if (obj->getDocument() == doc)
            docItem->_ParentMap.erase(obj);

        bool lock = blockSelection(true);
        for (auto cit = items.begin(), citNext = cit; cit != items.end(); cit = citNext) {
            ++citNext;
            (*cit)->myOwner = nullptr;
            delete* cit;
        }
        blockSelection(lock);

        // Check for any child of the deleted object that is not in the tree, and put it
        // under document item.
        for (auto child : data->children) {
            auto childVp = docItem->getViewProvider(child);
            if (!childVp || child->getDocument() != doc)
                continue;
            docItem->_ParentMap[child].erase(obj);
            auto cit = docItem->ObjectMap.find(child);
            if (cit == docItem->ObjectMap.end() || cit->second->items.empty()) {
                if (docItem->createNewItem(*childVp))
                    needUpdate = true;
            }
            else {
                auto childItem = *cit->second->items.begin();
                if (childItem->requiredAtRoot(false)) {
                    if (docItem->createNewItem(*childItem->object(), docItem, -1, childItem->myData))
                        needUpdate = true;
                }
            }
            childVp->setShowable(docItem->isObjectShowable(child));
        }
        docItem->ObjectMap.erase(obj);
    }
    ObjectTable.erase(itEntry);

    if (needUpdate)
        _updateStatus();
}

bool DocumentItem::populateObject(App::DocumentObject* obj) {
    // make sure at least one of the item corresponding to obj is populated
    auto it = ObjectMap.find(obj);
    if (it == ObjectMap.end())
        return false;
    auto& items = it->second->items;
    if (items.empty())
        return false;
    for (auto item : items) {
        if (item->populated)
            return true;
    }
    TREE_LOG("force populate object " << obj->getFullName());
    auto item = *items.begin();
    item->populated = true;
    populateItem(item, true);
    return true;
}

void DocumentItem::populateItem(DocumentObjectItem* item, bool refresh, bool delay)
{
    (void)delay;

    if (item->populated && !refresh)
        return;

    // Lazy loading policy: We will create an item for each children object if
    // a) the item is expanded, or b) there is at least one free child, i.e.
    // child originally located at root.

    item->setChildIndicatorPolicy(item->myData->children.empty() ?
        QTreeWidgetItem::DontShowIndicator : QTreeWidgetItem::ShowIndicator);

    if (!item->populated && !item->isExpanded()) {
        bool doPopulate = false;

        bool external = item->object()->getDocument() != item->getOwnerDocument()->document();
        if (external)
            return;
        auto obj = item->object()->getObject();
        auto linked = obj->getLinkedObject(true);
        if (linked && linked->getDocument() != obj->getDocument())
            return;
        for (auto child : item->myData->children) {
            auto it = ObjectMap.find(child);
            if (it == ObjectMap.end() || it->second->items.empty()) {
                auto vp = getViewProvider(child);
                if (!vp) continue;
                doPopulate = true;
                break;
            }
            if (item->myData->removeChildrenFromRoot) {
                if (it->second->rootItem) {
                    doPopulate = true;
                    break;
                }
            }
        }

        if (!doPopulate)
            return;
    }

    item->populated = true;
    bool checkHidden = !showHidden();
    bool updated = false;

    int i = -1;
    // iterate through the claimed children, and try to synchronize them with the
    // children tree item with the same order of appearance.
    int childCount = item->childCount();
    for (auto child : item->myData->children) {

        ++i; // the current index of the claimed child

        bool found = false;
        for (int j = i; j < childCount; ++j) {
            QTreeWidgetItem* ci = item->child(j);
            if (ci->type() != TreeWidget::ObjectType)
                continue;

            DocumentObjectItem* childItem = static_cast<DocumentObjectItem*>(ci);
            if (childItem->object()->getObject() != child)
                continue;

            found = true;
            if (j != i) { // fix index if it is changed
                childItem->setHighlight(false);
                item->removeChild(ci);
                item->insertChild(i, ci);
                assert(ci->parent() == item);
                if (checkHidden)
                    updateItemsVisibility(ci, false);
            }

            // Check if the item just changed its policy of whether to remove
            // children item from the root.
            if (item->myData->removeChildrenFromRoot) {
                if (childItem->myData->rootItem) {
                    assert(childItem != childItem->myData->rootItem);
                    bool lock = getTree()->blockSelection(true);
                    delete childItem->myData->rootItem;
                    getTree()->blockSelection(lock);
                }
            }
            else if (childItem->requiredAtRoot()) {
                createNewItem(*childItem->object(), this, -1, childItem->myData);
                updated = true;
            }
            break;
        }

        if (found)
            continue;

        // This algo will be recursively applied to newly created child items
        // through slotNewObject -> populateItem

        auto it = ObjectMap.find(child);
        if (it == ObjectMap.end() || it->second->items.empty()) {
            auto vp = getViewProvider(child);
            if (!vp || !createNewItem(*vp, item, i, it == ObjectMap.end() ? DocumentObjectDataPtr() : it->second))
                --i;
            else
                updated = true;
            continue;
        }

        if (!item->myData->removeChildrenFromRoot || !it->second->rootItem) {
            DocumentObjectItem* childItem = *it->second->items.begin();
            if (!createNewItem(*childItem->object(), item, i, it->second))
                --i;
            else
                updated = true;
        }
        else {
            DocumentObjectItem* childItem = it->second->rootItem;
            if (item == childItem || item->isChildOfItem(childItem)) {
                TREE_ERR("Cyclic dependency in "
                    << item->object()->getObject()->getFullName()
                    << '.' << childItem->object()->getObject()->getFullName());
                --i;
                continue;
            }
            it->second->rootItem = nullptr;
            childItem->setHighlight(false);
            this->removeChild(childItem);
            item->insertChild(i, childItem);
            assert(childItem->parent() == item);
            if (checkHidden)
                updateItemsVisibility(childItem, false);
        }
    }

    for (++i; item->childCount() > i;) {
        QTreeWidgetItem* ci = item->child(i);
        if (ci->type() == TreeWidget::ObjectType) {
            DocumentObjectItem* childItem = static_cast<DocumentObjectItem*>(ci);
            if (childItem->requiredAtRoot()) {
                item->removeChild(childItem);
                auto index = findRootIndex(childItem->object()->getObject());
                if (index >= 0)
                    this->insertChild(index, childItem);
                else
                    this->addChild(childItem);
                assert(childItem->parent() == this);
                if (checkHidden)
                    updateItemsVisibility(childItem, false);
                childItem->myData->rootItem = childItem;
                continue;
            }
        }

        bool lock = getTree()->blockSelection(true);
        delete ci;
        getTree()->blockSelection(lock);
    }
    if (updated)
        getTree()->_updateStatus();
}

int DocumentItem::findRootIndex(App::DocumentObject* childObj) {
    if (!TreeParams::Instance()->KeepRootOrder() || !childObj || !childObj->getNameInDocument())
        return -1;

    // object id is monotonically increasing, so use this as a hint to insert
    // object back so that we can have a stable order in root level.

    int count = this->childCount();
    if (!count)
        return -1;

    int first, last;

    // find the last item
    for (last = count - 1; last >= 0; --last) {
        auto citem = this->child(last);
        if (citem->type() == TreeWidget::ObjectType) {
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if (obj->getID() <= childObj->getID())
                return last + 1;
            break;
        }
    }

    // find the first item
    for (first = 0; first < count; ++first) {
        auto citem = this->child(first);
        if (citem->type() == TreeWidget::ObjectType) {
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if (obj->getID() >= childObj->getID())
                return first;
            break;
        }
    }

    // now do a binary search to find the lower bound, assuming the root level
    // object is already in order
    count = last - first;
    int pos;
    while (count > 0) {
        int step = count / 2;
        pos = first + step;
        for (; pos <= last; ++pos) {
            auto citem = this->child(pos);
            if (citem->type() != TreeWidget::ObjectType)
                continue;
            auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
            if (obj->getID() < childObj->getID()) {
                first = ++pos;
                count -= step + 1;
            }
            else
                count = step;
            break;
        }
        if (pos > last)
            return -1;
    }
    if (first > last)
        return -1;
    return first;
}

void TreeWidget::slotChangeObject(
    const Gui::ViewProviderDocumentObject& view, const App::Property& prop) {

    auto obj = view.getObject();
    if (!obj || !obj->getNameInDocument())
        return;

    auto itEntry = ObjectTable.find(obj);
    if (itEntry == ObjectTable.end() || itEntry->second.empty())
        return;

    _updateStatus();

    // Let's not waste time on the newly added Visibility property in
    // DocumentObject.
    if (&prop == &obj->Visibility)
        return;

    if (&prop == &obj->Label) {
        const char* label = obj->Label.getValue();
        auto firstData = *itEntry->second.begin();
        if (firstData->label != label) {
            for (auto data : itEntry->second) {
                data->label = label;
                auto displayName = QString::fromUtf8(label);
                for (auto item : data->items)
                    item->setText(0, displayName);
            }
        }
        return;
    }

    if (&prop == &obj->Label2) {
        const char* label = obj->Label2.getValue();
        auto firstData = *itEntry->second.begin();
        if (firstData->label2 != label) {
            for (auto data : itEntry->second) {
                data->label2 = label;
                auto displayName = QString::fromUtf8(label);
                for (auto item : data->items)
                    item->setText(1, displayName);
            }
        }
        return;
    }

    auto& s = ChangedObjects[obj];
    if (prop.testStatus(App::Property::Output)
        || prop.testStatus(App::Property::NoRecompute))
    {
        s.set(CS_Output);
    }
}

void TreeWidget::updateChildren(App::DocumentObject* obj,
    const std::set<DocumentObjectDataPtr>& dataSet, bool propOutput, bool force)
{
    bool childrenChanged = false;
    std::vector<App::DocumentObject*> children;
    bool removeChildrenFromRoot = true;

    DocumentObjectDataPtr found;
    for (auto data : dataSet) {
        if (!found) {
            found = data;
            childrenChanged = found->updateChildren(force);
            removeChildrenFromRoot = found->viewObject->canRemoveChildrenFromRoot();
            if (!childrenChanged && found->removeChildrenFromRoot == removeChildrenFromRoot)
                return;
        }
        else if (childrenChanged)
            data->updateChildren(found);
        data->removeChildrenFromRoot = removeChildrenFromRoot;
        DocumentItem* docItem = data->docItem;
        for (auto item : data->items)
            docItem->populateItem(item, true);
    }

    if (force)
        return;

    if (childrenChanged && propOutput) {
        // When a property is marked as output, it will not touch its object,
        // and thus, its property change will not be propagated through
        // recomputation. So we have to manually check for each links here.
        for (auto link : App::GetApplication().getLinksTo(obj, App::GetLinkRecursive)) {
            if (ChangedObjects.count(link))
                continue;
            std::vector<App::DocumentObject*> linkedChildren;
            DocumentObjectDataPtr found;
            auto it = ObjectTable.find(link);
            if (it == ObjectTable.end())
                continue;
            for (auto data : it->second) {
                if (!found) {
                    found = data;
                    if (!found->updateChildren(false))
                        break;
                }
                data->updateChildren(found);
                DocumentItem* docItem = data->docItem;
                for (auto item : data->items)
                    docItem->populateItem(item, true);
            }
        }
    }

    if (childrenChanged) {
        if (!selectTimer->isActive())
            onSelectionChanged(SelectionChanges());

        //if the item is in a GeoFeatureGroup we may need to update that too, as the claim children
        //of the geofeaturegroup depends on what the childs claim
        auto grp = App::GeoFeatureGroupExtension::getGroupOfObject(obj);
        if (grp && !ChangedObjects.count(grp)) {
            auto iter = ObjectTable.find(grp);
            if (iter != ObjectTable.end())
                updateChildren(grp, iter->second, true, false);
        }
    }
}

void DocumentItem::slotHighlightObject(const Gui::ViewProviderDocumentObject& obj,
    const Gui::HighlightMode& high, bool set, const App::DocumentObject* parent, const char* subname)
{
    getTree()->_updateStatus(false);
    if (parent && parent->getDocument() != document()->getDocument()) {
        auto it = getTree()->DocumentMap.find(Application::Instance->getDocument(parent->getDocument()));
        if (it != getTree()->DocumentMap.end())
            it->second->slotHighlightObject(obj, high, set, parent, subname);
        return;
    }
    FOREACH_ITEM(item, obj)
        if (parent) {
            App::DocumentObject* topParent = nullptr;
            std::ostringstream ss;
            item->getSubName(ss, topParent);
            if (!topParent) {
                if (parent != obj.getObject())
                    continue;
            }
        }
    item->setHighlight(set, high);
    if (parent)
        return;
    END_FOREACH_ITEM
}

static unsigned int countExpandedItem(const QTreeWidgetItem* item) {
    unsigned int size = 0;
    for (int i = 0, count = item->childCount(); i < count; ++i) {
        auto citem = item->child(i);
        if (citem->type() != TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if (obj->getNameInDocument())
            size += strlen(obj->getNameInDocument()) + countExpandedItem(citem);
    }
    return size;
}

unsigned int DocumentItem::getMemSize(void) const {
    return countExpandedItem(this);
}

static void saveExpandedItem(Base::Writer& writer, const QTreeWidgetItem* item) {
    int itemCount = 0;
    for (int i = 0, count = item->childCount(); i < count; ++i) {
        auto citem = item->child(i);
        if (citem->type() != TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if (obj->getNameInDocument())
            ++itemCount;
    }

    if (!itemCount) {
        writer.Stream() << "/>" << std::endl;
        return;
    }

    writer.Stream() << " count=\"" << itemCount << "\">" << std::endl;
    writer.incInd();
    for (int i = 0, count = item->childCount(); i < count; ++i) {
        auto citem = item->child(i);
        if (citem->type() != TreeWidget::ObjectType || !citem->isExpanded())
            continue;
        auto obj = static_cast<const DocumentObjectItem*>(citem)->object()->getObject();
        if (obj->getNameInDocument()) {
            writer.Stream() << writer.ind() << "<Expand name=\""
                << obj->getNameInDocument() << "\"";
            saveExpandedItem(writer, static_cast<const DocumentObjectItem*>(citem));
        }
    }
    writer.decInd();
    writer.Stream() << writer.ind() << "</Expand>" << std::endl;
}

void DocumentItem::Save(Base::Writer& writer) const {
    writer.Stream() << writer.ind() << "<Expand ";
    saveExpandedItem(writer, this);
}

void DocumentItem::Restore(Base::XMLReader& reader) {
    reader.readElement("Expand");
    if (!reader.hasAttribute("count"))
        return;
    _ExpandInfo.reset(new ExpandInfo);
    _ExpandInfo->restore(reader);
    for (auto inst : TreeWidget::Instances) {
        if (inst != getTree()) {
            auto docItem = inst->getDocumentItem(document());
            if (docItem)
                docItem->_ExpandInfo = _ExpandInfo;
        }
    }
}

void DocumentItem::restoreItemExpansion(const ExpandInfoPtr& info, DocumentObjectItem* item) {
    item->setExpanded(true);
    if (!info)
        return;
    for (int i = 0, count = item->childCount(); i < count; ++i) {
        auto citem = item->child(i);
        if (citem->type() != TreeWidget::ObjectType)
            continue;
        auto obj = static_cast<DocumentObjectItem*>(citem)->object()->getObject();
        if (!obj->getNameInDocument())
            continue;
        auto it = info->find(obj->getNameInDocument());
        if (it != info->end())
            restoreItemExpansion(it->second, static_cast<DocumentObjectItem*>(citem));
    }
}

void DocumentItem::slotExpandObject(const Gui::ViewProviderDocumentObject& obj,
    const Gui::TreeItemMode& mode, const App::DocumentObject* parent, const char* subname)
{
    getTree()->_updateStatus(false);

    if ((mode == TreeItemMode::ExpandItem ||
        mode == TreeItemMode::ExpandPath) &&
        obj.getDocument()->getDocument()->testStatus(App::Document::Restoring)) {
        if (!_ExpandInfo)
            _ExpandInfo.reset(new ExpandInfo);
        _ExpandInfo->emplace(std::string("*") + obj.getObject()->getNameInDocument(), ExpandInfoPtr());
        return;
    }

    if (parent && parent->getDocument() != document()->getDocument()) {
        auto it = getTree()->DocumentMap.find(Application::Instance->getDocument(parent->getDocument()));
        if (it != getTree()->DocumentMap.end())
            it->second->slotExpandObject(obj, mode, parent, subname);
        return;
    }

    FOREACH_ITEM(item, obj)
        // All document object items must always have a parent, either another
        // object item or document item. If not, then there is a bug somewhere
        // else.
        assert(item->parent());

    switch (mode) {
    case TreeItemMode::ExpandPath:
        if (!parent) {
            QTreeWidgetItem* parentItem = item->parent();
            while (parentItem) {
                parentItem->setExpanded(true);
                parentItem = parentItem->parent();
            }
            item->setExpanded(true);
            break;
        }
        // fall through
    case TreeItemMode::ExpandItem:
        if (!parent) {
            if (item->parent()->isExpanded())
                item->setExpanded(true);
        }
        else {
            App::DocumentObject* topParent = nullptr;
            std::ostringstream ss;
            item->getSubName(ss, topParent);
            if (!topParent) {
                if (parent != obj.getObject())
                    continue;
            }
            else if (topParent != parent)
                continue;
            showItem(item, false, true);
            item->setExpanded(true);
        }
        break;
    case TreeItemMode::CollapseItem:
        item->setExpanded(false);
        break;
    case TreeItemMode::ToggleItem:
        if (item->isExpanded())
            item->setExpanded(false);
        else
            item->setExpanded(true);
        break;

    default:
        break;
    }
    if (item->isExpanded())
        populateItem(item);
    if (parent)
        return;
    END_FOREACH_ITEM
}

void DocumentItem::slotScrollToObject(const Gui::ViewProviderDocumentObject& obj)
{
    if (!obj.getObject() || !obj.getObject()->getNameInDocument())
        return;
    auto it = ObjectMap.find(obj.getObject());
    if (it == ObjectMap.end() || it->second->items.empty())
        return;
    auto item = it->second->rootItem;
    if (!item)
        item = *it->second->items.begin();
    getTree()->_updateStatus(false);
    getTree()->scrollToItem(item);
}

void DocumentItem::slotRecomputedObject(const App::DocumentObject& obj) {
    if (obj.isValid())
        return;
    slotRecomputed(*obj.getDocument(), { const_cast<App::DocumentObject*>(&obj) });
}

void DocumentItem::slotRecomputed(const App::Document&, const std::vector<App::DocumentObject*>& objs) {
    auto tree = getTree();
    for (auto obj : objs) {
        if (!obj->isValid())
            tree->ChangedObjects[obj].set(TreeWidget::CS_Error);
    }
    if (tree->ChangedObjects.size())
        tree->_updateStatus();
}

Gui::Document* DocumentItem::document() const
{
    return this->pDocument;
}

//void DocumentItem::markItem(const App::DocumentObject* Obj,bool mark)
//{
//    // never call without Object!
//    assert(Obj);
//
//
//    std::map<std::string,DocumentObjectItem*>::iterator pos;
//    pos = ObjectMap.find(Obj);
//    if (pos != ObjectMap.end()) {
//        QFont f = pos->second->font(0);
//        f.setUnderline(mark);
//        pos->second->setFont(0,f);
//    }
//}

void DocumentItem::testStatus(void)
{
    for (const auto& v : ObjectMap)
        v.second->testStatus();
}

void DocumentItem::setData(int column, int role, const QVariant& value)
{
    if (role == Qt::EditRole) {
        QString label = value.toString();
        pDocument->getDocument()->Label.setValue((const char*)label.toUtf8());
    }

    QTreeWidgetItem::setData(column, role, value);
}

void DocumentItem::clearSelection(DocumentObjectItem* exclude)
{
    // Block signals here otherwise we get a recursion and quadratic runtime
    bool ok = treeWidget()->blockSignals(true);
    FOREACH_ITEM_ALL(item);
    if (item == exclude) {
        if (item->selected > 0)
            item->selected = -1;
        else
            item->selected = 0;
        updateItemSelection(item);
    }
    else {
        item->selected = 0;
        item->mySubs.clear();
        item->setSelected(false);
        item->setCheckState(false);
    }
    END_FOREACH_ITEM;
    treeWidget()->blockSignals(ok);
}

void DocumentItem::updateSelection(QTreeWidgetItem* ti, bool unselect) {
    for (int i = 0, count = ti->childCount(); i < count; ++i) {
        auto child = ti->child(i);
        if (child && child->type() == TreeWidget::ObjectType) {
            auto childItem = static_cast<DocumentObjectItem*>(child);
            if (unselect) {
                childItem->setSelected(false);
                childItem->setCheckState(false);
            }
            updateItemSelection(childItem);
            if (unselect && childItem->isGroup()) {
                // If the child item being force unselected by its group parent
                // is itself a group, propagate the unselection to its own
                // children
                updateSelection(childItem, true);
            }
        }
    }

    if (unselect)
        return;
    for (int i = 0, count = ti->childCount(); i < count; ++i)
        updateSelection(ti->child(i));
}

void DocumentItem::updateItemSelection(DocumentObjectItem* item) {
    bool selected = item->isSelected();
    bool checked = item->checkState(0) == Qt::Checked;

    if (selected && !checked)
        item->setCheckState(true);

    if (!selected && checked)
        item->setCheckState(false);

    if ((selected && item->selected > 0) || (!selected && !item->selected)) {
        return;
    }
    if (item->selected != -1)
        item->mySubs.clear();
    item->selected = selected;

    auto obj = item->object()->getObject();
    if (!obj || !obj->getNameInDocument())
        return;

    std::ostringstream str;
    App::DocumentObject* topParent = nullptr;
    item->getSubName(str, topParent);
    if (topParent) {
        if (topParent->hasExtension(App::GeoFeatureGroupExtension::getExtensionClassTypeId())) {
            // remove legacy selection, i.e. those without subname
            Gui::Selection().rmvSelection(obj->getDocument()->getName(),
                obj->getNameInDocument(), nullptr);
        }
        if (!obj->redirectSubName(str, topParent, nullptr))
            str << obj->getNameInDocument() << '.';
        obj = topParent;
    }
    const char* objname = obj->getNameInDocument();
    const char* docname = obj->getDocument()->getName();
    const auto& subname = str.str();

    if (subname.size()) {
        auto parentItem = item->getParentItem();
        assert(parentItem);
        if (selected && parentItem->selected) {
            // When a group item is selected, all its children objects are
            // highlighted in the 3D view. So, when an item of some group is
            // newly selected, we must force unselect its parent in order to
            // show the selection highlight. Besides, select both the parent
            // group and its children doesn't make much sense.
            //
            // UPDATE: There are legit use case of both parent and child
            // selection, for example, to disambiguate under which group to
            // operate on the child.
            //
            // TREE_TRACE("force unselect parent");
            // parentItem->setSelected(false);
            // updateItemSelection(parentItem);
        }
    }

    if (selected && item->isGroup()) {
        // Same reasoning as above. When a group item is newly selected, We
        // choose to force unselect all its children to void messing up the
        // selection highlight
        //
        // UPDATE: same as above, child and parent selection is now re-enabled.
        //
        // TREE_TRACE("force unselect all children");
        // updateSelection(item,true);
    }

    if (!selected) {
        Gui::Selection().rmvSelection(docname, objname, subname.c_str());
        return;
    }
    selected = false;
    if (item->mySubs.size()) {
        for (auto& sub : item->mySubs) {
            if (Gui::Selection().addSelection(docname, objname, (subname + sub).c_str()))
                selected = true;
        }
    }
    if (!selected) {
        item->mySubs.clear();
        if (!Gui::Selection().addSelection(docname, objname, subname.c_str())) {
            item->selected = 0;
            item->setSelected(false);
            item->setCheckState(false);
            return;
        }
    }
    getTree()->syncView(item->object());
}

App::DocumentObject* DocumentItem::getTopParent(App::DocumentObject* obj, std::string& subname) {
    auto it = ObjectMap.find(obj);
    if (it == ObjectMap.end() || it->second->items.empty())
        return nullptr;

    // already a top parent
    if (it->second->rootItem)
        return obj;

    for (auto item : it->second->items) {
        // non group object do not provide a coordinate system, hence its
        // claimed child is still in the global coordinate space, so the
        // child can still be considered a top level object
        if (!item->isParentGroup())
            return obj;
    }

    // If no top level item, find an item that is closest to the top level
    std::multimap<int, DocumentObjectItem*> items;
    for (auto item : it->second->items) {
        int i = 0;
        for (auto parent = item->parent(); parent; ++i, parent = parent->parent()) {
            if (parent->isHidden())
                i += 1000;
            ++i;
        }
        items.emplace(i, item);
    }

    App::DocumentObject* topParent = nullptr;
    std::ostringstream ss;
    items.begin()->second->getSubName(ss, topParent);
    if (!topParent) {
        // this shouldn't happen
        FC_WARN("No top parent for " << obj->getFullName() << '.' << subname);
        return obj;
    }
    ss << obj->getNameInDocument() << '.' << subname;
    FC_LOG("Subname correction " << obj->getFullName() << '.' << subname
        << " -> " << topParent->getFullName() << '.' << ss.str());
    subname = ss.str();
    return topParent;
}

DocumentObjectItem* DocumentItem::findItemByObject(
    bool sync, App::DocumentObject* obj, const char* subname, bool select)
{
    if (!subname)
        subname = "";

    auto it = ObjectMap.find(obj);
    if (it == ObjectMap.end() || it->second->items.empty())
        return nullptr;

    // prefer top level item of this object
    if (it->second->rootItem)
        return findItem(sync, it->second->rootItem, subname, select);

    for (auto item : it->second->items) {
        // non group object do not provide a coordinate system, hence its
        // claimed child is still in the global coordinate space, so the
        // child can still be considered a top level object
        if (!item->isParentGroup())
            return findItem(sync, item, subname, select);
    }

    // If no top level item, find an item that is closest to the top level
    std::multimap<int, DocumentObjectItem*> items;
    for (auto item : it->second->items) {
        int i = 0;
        for (auto parent = item->parent(); parent; ++i, parent = parent->parent())
            ++i;
        items.emplace(i, item);
    }
    for (auto& v : items) {
        auto item = findItem(sync, v.second, subname, select);
        if (item)
            return item;
    }
    return nullptr;
}

DocumentObjectItem* DocumentItem::findItem(
    bool sync, DocumentObjectItem* item, const char* subname, bool select)
{
    if (item->isHidden())
        item->setHidden(false);

    if (!subname || *subname == 0) {
        if (select) {
            item->selected += 2;
            item->mySubs.clear();
        }
        return item;
    }

    TREE_TRACE("find next " << subname);

    // try to find the next level object name
    const char* nextsub = nullptr;
    const char* dot = nullptr;
    if ((dot = strchr(subname, '.')))
        nextsub = dot + 1;
    else {
        if (select) {
            item->selected += 2;
            if (std::find(item->mySubs.begin(), item->mySubs.end(), subname) == item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    std::string name(subname, nextsub - subname);
    auto obj = item->object()->getObject();
    auto subObj = obj->getSubObject(name.c_str());
    if (!subObj || subObj == obj) {
        if (!subObj && !getTree()->searchDoc)
            TREE_LOG("sub object not found " << item->getName() << '.' << name.c_str());
        if (select) {
            item->selected += 2;
            if (std::find(item->mySubs.begin(), item->mySubs.end(), subname) == item->mySubs.end())
                item->mySubs.push_back(subname);
        }
        return item;
    }

    if (select)
        item->mySubs.clear();

    if (!item->populated && sync) {
        //force populate the item
        item->populated = true;
        populateItem(item, true);
    }

    for (int i = 0, count = item->childCount(); i < count; ++i) {
        auto ti = item->child(i);
        if (!ti || ti->type() != TreeWidget::ObjectType) continue;
        auto child = static_cast<DocumentObjectItem*>(ti);

        if (child->object()->getObject() == subObj)
            return findItem(sync, child, nextsub, select);
    }

    // The sub object is not found. This could happen for geo group, since its
    // children may be in more than one hierarchy down.
    bool found = false;
    DocumentObjectItem* res = nullptr;
    auto it = ObjectMap.find(subObj);
    if (it != ObjectMap.end()) {
        for (auto child : it->second->items) {
            if (child->isChildOfItem(item)) {
                found = true;
                res = findItem(sync, child, nextsub, select);
                if (!select)
                    return res;
            }
        }
    }

    if (select && !found) {
        // The sub object is still not found. Maybe it is a non-object sub-element.
        // Select the current object instead.
        TREE_TRACE("element " << subname << " not found");
        item->selected += 2;
        if (std::find(item->mySubs.begin(), item->mySubs.end(), subname) == item->mySubs.end())
            item->mySubs.push_back(subname);
    }
    return res;
}

void DocumentItem::selectItems(SelectionReason reason) {
    const auto& sels = Selection().getSelection(pDocument->getDocument()->getName(), ResolveMode::NoResolve);

    bool sync = (sels.size() > 50 || reason == SR_SELECT) ? false : true;

    for (const auto& sel : sels)
        findItemByObject(sync, sel.pObject, sel.SubName, true);

    DocumentObjectItem* newSelect = nullptr;
    DocumentObjectItem* oldSelect = nullptr;

    FOREACH_ITEM_ALL(item)
        if (item->selected == 1) {
            // this means it is the old selection and is not in the current
            // selection
            item->selected = 0;
            item->mySubs.clear();
            item->setSelected(false);
            item->setCheckState(false);
        }
        else if (item->selected) {
            if (sync) {
                if (item->selected == 2 && showItem(item, false, reason == SR_FORCE_EXPAND)) {
                    // This means newly selected and can auto expand
                    if (!newSelect)
                        newSelect = item;
                }
                if (!newSelect && !oldSelect && !item->isHidden()) {
                    bool visible = true;
                    for (auto parent = item->parent(); parent; parent = parent->parent()) {
                        if (!parent->isExpanded() || parent->isHidden()) {
                            visible = false;
                            break;
                        }
                    }
                    if (visible)
                        oldSelect = item;
                }
            }
            item->selected = 1;
            item->setSelected(true);
            item->setCheckState(true);
        }
    END_FOREACH_ITEM;

    if (sync) {
        if (!newSelect)
            newSelect = oldSelect;
        else
            getTree()->syncView(newSelect->object());
        if (newSelect)
            getTree()->scrollToItem(newSelect);
    }
}

void DocumentItem::populateParents(const ViewProvider* vp, ViewParentMap& parentMap) {
    auto it = parentMap.find(vp);
    if (it == parentMap.end())
        return;
    for (auto parent : it->second) {
        auto it = ObjectMap.find(parent->getObject());
        if (it == ObjectMap.end())
            continue;

        populateParents(parent, parentMap);
        for (auto item : it->second->items) {
            if (!item->isHidden() && !item->populated) {
                item->populated = true;
                populateItem(item, true);
            }
        }
    }
}

void DocumentItem::selectAllInstances(const ViewProviderDocumentObject& vpd) {
    ViewParentMap parentMap;
    auto pObject = vpd.getObject();
    if (ObjectMap.find(pObject) == ObjectMap.end())
        return;

    bool lock = getTree()->blockSelection(true);

    // We are trying to select all items corresponding to a given view
    // provider, i.e. all appearance of the object inside all its parent items
    //
    // Build a map of object to all its parent
    for (auto& v : ObjectMap) {
        if (v.second->viewObject == &vpd) continue;
        for (auto child : v.second->viewObject->claimChildren()) {
            auto vp = getViewProvider(child);
            if (!vp) continue;
            parentMap[vp].push_back(v.second->viewObject);
        }
    }

    // now make sure all parent items are populated. In order to do that, we
    // need to populate the oldest parent first
    populateParents(&vpd, parentMap);

    DocumentObjectItem* first = nullptr;
    FOREACH_ITEM(item, vpd);
    if (showItem(item, true) && !first)
        first = item;
    END_FOREACH_ITEM;

    getTree()->blockSelection(lock);
    if (first) {
        treeWidget()->scrollToItem(first);
        updateSelection();
    }
}

bool DocumentItem::showHidden() const {
    return pDocument->getDocument()->ShowHidden.getValue();
}

void DocumentItem::setShowHidden(bool show) {
    pDocument->getDocument()->ShowHidden.setValue(show);
}

bool DocumentItem::showItem(DocumentObjectItem* item, bool select, bool force) {
    auto parent = item->parent();
    if (item->isHidden()) {
        if (!force)
            return false;
        item->setHidden(false);
    }

    if (parent->type() == TreeWidget::ObjectType) {
        if (!showItem(static_cast<DocumentObjectItem*>(parent), false))
            return false;
        auto pitem = static_cast<DocumentObjectItem*>(parent);
        if (force || !pitem->object()->getObject()->testStatus(App::NoAutoExpand))
            parent->setExpanded(true);
        else if (!select)
            return false;
    }
    else
        parent->setExpanded(true);

    if (select) {
        item->setSelected(true);
        item->setCheckState(true);
    }
    return true;
}

void DocumentItem::updateItemsVisibility(QTreeWidgetItem* item, bool show) {
    if (item->type() == TreeWidget::ObjectType) {
        auto objitem = static_cast<DocumentObjectItem*>(item);
        objitem->setHidden(!show && !objitem->object()->showInTree());
    }
    for (int i = 0; i < item->childCount(); ++i)
        updateItemsVisibility(item->child(i), show);
}

void DocumentItem::updateSelection() {
    bool lock = getTree()->blockSelection(true);
    updateSelection(this, false);
    getTree()->blockSelection(lock);
}

// ----------------------------------------------------------------------------

static int countItems;

DocumentObjectItem::DocumentObjectItem(DocumentItem* ownerDocItem, DocumentObjectDataPtr data)
    : QTreeWidgetItem(TreeWidget::ObjectType)
    , myOwner(ownerDocItem), myData(data), previousStatus(-1), selected(0), populated(false)
{
    setFlags(flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    setCheckState(false);

    myData->items.insert(this);
    ++countItems;
    TREE_LOG("Create item: " << countItems << ", " << object()->getObject()->getFullName());
}

DocumentObjectItem::~DocumentObjectItem()
{
    --countItems;
    TREE_LOG("Delete item: " << countItems << ", " << object()->getObject()->getFullName());
    auto it = myData->items.find(this);
    if (it == myData->items.end())
        assert(0);
    else
        myData->items.erase(it);

    if (myData->rootItem == this)
        myData->rootItem = nullptr;

    if (myOwner && myData->items.empty()) {
        auto it = myOwner->_ParentMap.find(object()->getObject());
        if (it != myOwner->_ParentMap.end() && it->second.size()) {
            myOwner->PopulateObjects.push_back(*it->second.begin());
            myOwner->getTree()->_updateStatus();
        }
    }
}

void DocumentObjectItem::restoreBackground() {
    this->setBackground(0, this->bgBrush);
}

void DocumentObjectItem::setHighlight(bool set, Gui::HighlightMode high) {
    QFont f = this->font(0);
    auto highlight = [=](const QColor& col) {
        if (set)
            this->setBackground(0, col);
        else
            this->setBackground(0, QBrush());
        this->bgBrush = this->background(0);
    };

    switch (high) {
    case HighlightMode::Bold:
        f.setBold(set);
        break;
    case HighlightMode::Italic:
        f.setItalic(set);
        break;
    case HighlightMode::Underlined:
        f.setUnderline(set);
        break;
    case HighlightMode::Overlined:
        f.setOverline(set);
        break;
    case HighlightMode::Blue:
        highlight(QColor(200, 200, 255));
        break;
    case HighlightMode::LightBlue:
        highlight(QColor(230, 230, 255));
        break;
    case HighlightMode::UserDefined:
    {
        QColor color(230, 230, 255);
        if (set) {
            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/TreeView");
            bool bold = hGrp->GetBool("TreeActiveBold", true);
            bool italic = hGrp->GetBool("TreeActiveItalic", false);
            bool underlined = hGrp->GetBool("TreeActiveUnderlined", false);
            bool overlined = hGrp->GetBool("TreeActiveOverlined", false);
            f.setBold(bold);
            f.setItalic(italic);
            f.setUnderline(underlined);
            f.setOverline(overlined);

            unsigned long col = hGrp->GetUnsigned("TreeActiveColor", 3873898495);
            color = QColor((col >> 24) & 0xff, (col >> 16) & 0xff, (col >> 8) & 0xff);
        }
        else {
            f.setBold(false);
            f.setItalic(false);
            f.setUnderline(false);
            f.setOverline(false);
        }
        highlight(color);
    }   break;
    default:
        break;
    }
    this->setFont(0, f);
}

const char* DocumentObjectItem::getTreeName() const
{
    return myData->getTreeName();
}

Gui::ViewProviderDocumentObject* DocumentObjectItem::object() const
{
    return myData->viewObject;
}

void DocumentObjectItem::testStatus(bool resetStatus)
{
    QIcon icon, icon2;
    testStatus(resetStatus, icon, icon2);
}

void DocumentObjectItem::testStatus(bool resetStatus, QIcon& icon1, QIcon& icon2)
{
    App::DocumentObject* pObject = object()->getObject();

    int visible = -1;
    auto parentItem = getParentItem();
    if (parentItem) {
        Timing(testStatus1);
        auto parent = parentItem->object()->getObject();
        auto ext = parent->getExtensionByType<App::GroupExtension>(true, false);
        if (!ext)
            visible = parent->isElementVisible(pObject->getNameInDocument());
        else {
            // We are dealing with a plain group. It has special handling when
            // linked, which allows it to have indpenedent visibility control.
            // We need to go up the hierarchy and see if there is any link to
            // it.
            for (auto pp = parentItem->getParentItem(); pp; pp = pp->getParentItem()) {
                auto obj = pp->object()->getObject();
                if (!obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false)) {
                    visible = pp->object()->getObject()->isElementVisible(pObject->getNameInDocument());
                    break;
                }
            }
        }
    }

    Timing(testStatus2);

    if (visible < 0)
        visible = object()->isShow() ? 1 : 0;

    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    bool external = object()->getDocument() != getOwnerDocument()->document() ||
        (linked && linked->getDocument() != obj->getDocument());

    int currentStatus =
        ((external ? 0 : 1) << 4) |
        ((object()->showInTree() ? 0 : 1) << 3) |
        ((pObject->isError() ? 1 : 0) << 2) |
        ((pObject->isTouched() || pObject->mustExecute() == 1 ? 1 : 0) << 1) |
        (visible ? 1 : 0);

    TimingStop(testStatus2);

    if (!resetStatus && previousStatus == currentStatus)
        return;

    _Timing(1, testStatus3);

    previousStatus = currentStatus;

    QIcon::Mode mode = QIcon::Normal;
    if (currentStatus & 1) { // visible
        // Note: By default the foreground, i.e. text color is invalid
        // to make use of the default color of the tree widget's palette.
        // If we temporarily set this color to dark and reset to an invalid
        // color again we cannot do it with setTextColor() or setForeground(),
        // respectively, because for any reason the color would always switch
        // to black which will lead to unreadable text if the system background
        // hss already a dark color.
        // However, it works if we set the appropriate role to an empty QVariant().
        this->setData(0, Qt::ForegroundRole, QVariant());
    }
    else { // invisible
        QStyleOptionViewItem opt;
        // it can happen that a tree item is not attached to the tree widget (#0003025)
        if (this->treeWidget())
            opt.initFrom(this->treeWidget());
        this->setForeground(0, opt.palette.color(QPalette::Disabled, QPalette::Text));
        mode = QIcon::Disabled;
    }

    _TimingStop(1, testStatus3);

    QIcon& icon = mode == QIcon::Normal ? icon1 : icon2;

    if (icon.isNull()) {
        Timing(getIcon);
        QPixmap px;
        if (currentStatus & 4) {
            static QPixmap pxError;
            if (pxError.isNull()) {
                // object is in error state
                const char* const feature_error_xpm[] = {
                    "9 9 3 1",
                    ". c None",
                    "# c #ff0000",
                    "a c #ffffff",
                    "...###...",
                    ".##aaa##.",
                    ".##aaa##.",
                    "###aaa###",
                    "###aaa###",
                    "#########",
                    ".##aaa##.",
                    ".##aaa##.",
                    "...###..." };
                pxError = QPixmap(feature_error_xpm);
            }
            px = pxError;
        }
        else if (currentStatus & 2) {
            static QPixmap pxRecompute;
            if (pxRecompute.isNull()) {
                // object must be recomputed
                const char* const feature_recompute_xpm[] = {
                    "9 9 3 1",
                    ". c None",
                    "# c #0000ff",
                    "a c #ffffff",
                    "...###...",
                    ".######aa",
                    ".#####aa.",
                    "#####aa##",
                    "#aa#aa###",
                    "#aaaa####",
                    ".#aa####.",
                    ".#######.",
                    "...###..." };
                pxRecompute = QPixmap(feature_recompute_xpm);
            }
            px = pxRecompute;
        }

        // get the original icon set
        QIcon icon_org = object()->getIcon();

        int w = getTree()->viewOptions().decorationSize.width();

        QPixmap pxOn, pxOff;

        // if needed show small pixmap inside
        if (!px.isNull()) {
            pxOff = BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::Off),
                px, BitmapFactoryInst::TopRight);
            pxOn = BitmapFactory().merge(icon_org.pixmap(w, w, mode, QIcon::On),
                px, BitmapFactoryInst::TopRight);
        }
        else {
            pxOff = icon_org.pixmap(w, w, mode, QIcon::Off);
            pxOn = icon_org.pixmap(w, w, mode, QIcon::On);
        }

        if (currentStatus & 8) {// hidden item
            static QPixmap pxHidden;
            if (pxHidden.isNull()) {
                const char* const feature_hidden_xpm[] = {
                    "9 7 3 1",
                    ". c None",
                    "# c #000000",
                    "a c #ffffff",
                    "...###...",
                    "..#aaa#..",
                    ".#a###a#.",
                    "#aa###aa#",
                    ".#a###a#.",
                    "..#aaa#..",
                    "...###..." };
                pxHidden = QPixmap(feature_hidden_xpm);
            }
            pxOff = BitmapFactory().merge(pxOff, pxHidden, BitmapFactoryInst::TopLeft);
            pxOn = BitmapFactory().merge(pxOn, pxHidden, BitmapFactoryInst::TopLeft);
        }

        if (external) {// external item
            static QPixmap pxExternal;
            if (pxExternal.isNull()) {
                const char* const feature_external_xpm[] = {
                    "7 7 3 1",
                    ". c None",
                    "# c #000000",
                    "a c #ffffff",
                    "..###..",
                    ".#aa##.",
                    "..#aa##",
                    "..##aa#",
                    "..#aa##",
                    ".#aa##.",
                    "..###.." };
                pxExternal = QPixmap(feature_external_xpm);
            }
            pxOff = BitmapFactory().merge(pxOff, pxExternal, BitmapFactoryInst::BottomRight);
            pxOn = BitmapFactory().merge(pxOn, pxExternal, BitmapFactoryInst::BottomRight);
        }

        icon.addPixmap(pxOn, QIcon::Normal, QIcon::On);
        icon.addPixmap(pxOff, QIcon::Normal, QIcon::Off);

        icon = object()->mergeColorfulOverlayIcons(icon);
    }


    _Timing(2, setIcon);
    this->setIcon(0, icon);
}

void DocumentObjectItem::displayStatusInfo()
{
    App::DocumentObject* Obj = object()->getObject();

    QString info = QApplication::translate(Obj->getTypeId().getName(), Obj->getStatusString());

    if (Obj->mustExecute() == 1 && !Obj->isError())
        info += TreeWidget::tr(" (but must be executed)");

    QString status = TreeWidget::tr("%1, Internal name: %2")
        .arg(info, QString::fromLatin1(Obj->getNameInDocument()));

    if (!Obj->isError())
        getMainWindow()->showMessage(status);
    else {
        getMainWindow()->showStatus(MainWindow::Err, status);
        QTreeWidget* tree = this->treeWidget();
        QPoint pos = tree->visualItemRect(this).topRight();
        QToolTip::showText(tree->mapToGlobal(pos), info);
    }
}

void DocumentObjectItem::setExpandedStatus(bool on)
{
    if (getOwnerDocument()->document() == object()->getDocument())
        object()->getObject()->setStatus(App::Expand, on);
}

void DocumentObjectItem::setData(int column, int role, const QVariant& value)
{
    QVariant myValue(value);
    if (role == Qt::EditRole && column <= 1) {
        auto obj = object()->getObject();
        auto& label = column ? obj->Label2 : obj->Label;
        std::ostringstream ss;
        ss << "Change " << getName() << '.' << label.getName();
        App::AutoTransaction committer(ss.str().c_str());
        label.setValue((const char*)value.toString().toUtf8());
        myValue = QString::fromUtf8(label.getValue());
    }
    QTreeWidgetItem::setData(column, role, myValue);
}

bool DocumentObjectItem::isChildOfItem(DocumentObjectItem* item)
{
    for (auto pitem = parent(); pitem; pitem = pitem->parent())
        if (pitem == item)
            return true;
    return false;
}

bool DocumentObjectItem::requiredAtRoot(bool excludeSelf) const {
    if (myData->rootItem || object()->getDocument() != getOwnerDocument()->document())
        return false;
    bool checkMap = true;
    for (auto item : myData->items) {
        if (excludeSelf && item == this) continue;
        auto pi = item->getParentItem();
        if (!pi || pi->myData->removeChildrenFromRoot)
            return false;
        checkMap = false;
    }
    if (checkMap && myOwner) {
        auto it = myOwner->_ParentMap.find(object()->getObject());
        if (it != myOwner->_ParentMap.end()) {
            // Reaching here means all items of this corresponding object is
            // going to be deleted, but the object itself is not deleted and
            // still being referred to by some parent item that is not expanded
            // yet. So, we force populate at least one item of the parent
            // object to make sure that there is at least one corresponding
            // item for each object.
            //
            // PS: practically speaking, it won't hurt much to delete all the
            // items, because the item will be auto created once the user
            // expand its parent item. It only causes minor problems, such as,
            // tree scroll to object command won't work properly.

            for (auto parent : it->second) {
                if (getOwnerDocument()->populateObject(parent))
                    return false;
            }
        }
    }
    return true;
}

bool DocumentObjectItem::isLink() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && obj != linked;
}

bool DocumentObjectItem::isLinkFinal() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(false);
    return linked && linked == linked->getLinkedObject(true);
}


bool DocumentObjectItem::isParentLink() const {
    auto pi = getParentItem();
    return pi && pi->isLink();
}

enum GroupType {
    NotGroup = 0,
    LinkGroup = 1,
    PartGroup = 2,
    SuperGroup = 3, //reversed for future
};

int DocumentObjectItem::isGroup() const {
    auto obj = object()->getObject();
    auto linked = obj->getLinkedObject(true);
    if (linked && linked->hasExtension(
        App::GeoFeatureGroupExtension::getExtensionClassTypeId()))
        return PartGroup;
    if (obj->hasChildElement())
        return LinkGroup;
    if (obj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false)) {
        for (auto parent = getParentItem(); parent; parent = parent->getParentItem()) {
            auto pobj = parent->object()->getObject();
            if (pobj->hasExtension(App::GroupExtension::getExtensionClassTypeId(), false))
                continue;
            if (pobj->isElementVisible(obj->getNameInDocument()) >= 0)
                return LinkGroup;
        }
    }
    return NotGroup;
}

bool DocumentItem::isObjectShowable(App::DocumentObject* obj) {
    auto itParents = _ParentMap.find(obj);
    if (itParents == _ParentMap.end() || itParents->second.empty())
        return true;
    bool showable = true;
    for (auto parent : itParents->second) {
        if (parent->getDocument() != obj->getDocument())
            continue;
        if (!parent->hasChildElement()
            && parent->getLinkedObject(false) == parent)
            return true;
        showable = false;
    }
    return showable;
}

int DocumentObjectItem::isParentGroup() const {
    auto pi = getParentItem();
    return pi ? pi->isGroup() : 0;
}

DocumentObjectItem* DocumentObjectItem::getParentItem() const {
    if (parent()->type() != TreeWidget::ObjectType)
        return nullptr;
    return static_cast<DocumentObjectItem*>(parent());
}

const char* DocumentObjectItem::getName() const {
    const char* name = object()->getObject()->getNameInDocument();
    return name ? name : "";
}

int DocumentObjectItem::getSubName(std::ostringstream& str, App::DocumentObject*& topParent) const
{
    auto parent = getParentItem();
    if (!parent)
        return NotGroup;
    int ret = parent->getSubName(str, topParent);
    if (ret != SuperGroup) {
        int group = parent->isGroup();
        if (group == NotGroup) {
            if (ret != PartGroup) {
                // Handle this situation,
                //
                // LinkGroup
                //    |--PartExtrude
                //           |--Sketch
                //
                // This function traverse from top down, so, when seeing a
                // non-group object 'PartExtrude', its following children should
                // not be grouped, so must reset any previous parents here.
                topParent = nullptr;
                str.str(""); //reset the current subname
                return NotGroup;
            }
            group = PartGroup;
        }
        ret = group;
    }

    auto obj = parent->object()->getObject();
    if (!obj || !obj->getNameInDocument()) {
        topParent = nullptr;
        str.str("");
        return NotGroup;
    }
    if (!topParent)
        topParent = obj;
    else if (!obj->redirectSubName(str, topParent, object()->getObject()))
        str << obj->getNameInDocument() << '.';
    return ret;
}

App::DocumentObject* DocumentObjectItem::getFullSubName(
    std::ostringstream& str, DocumentObjectItem* parent) const
{
    auto pi = getParentItem();
    if (this == parent || !pi || (!parent && !pi->isGroup()))
        return object()->getObject();

    auto ret = pi->getFullSubName(str, parent);
    str << getName() << '.';
    return ret;
}

App::DocumentObject* DocumentObjectItem::getRelativeParent(
    std::ostringstream& str, DocumentObjectItem* cousin,
    App::DocumentObject** topParent, std::string* topSubname) const
{
    std::ostringstream str2;
    App::DocumentObject* top = nullptr, * top2 = nullptr;
    getSubName(str, top);
    if (topParent)
        *topParent = top;
    if (!top)
        return nullptr;
    if (topSubname)
        *topSubname = str.str() + getName() + '.';
    cousin->getSubName(str2, top2);
    if (top != top2) {
        str << getName() << '.';
        return top;
    }

    auto subname = str.str();
    auto subname2 = str2.str();
    const char* sub = subname.c_str();
    const char* sub2 = subname2.c_str();
    while (1) {
        const char* dot = strchr(sub, '.');
        if (!dot) {
            str.str("");
            return nullptr;
        }
        const char* dot2 = strchr(sub2, '.');
        if (!dot2 || dot - sub != dot2 - sub2 || strncmp(sub, sub2, dot - sub) != 0) {
            auto substr = subname.substr(0, dot - subname.c_str() + 1);
            auto ret = top->getSubObject(substr.c_str());
            if (!top) {
                FC_ERR("invalid subname " << top->getFullName() << '.' << substr);
                str.str("");
                return nullptr;
            }
            str.str("");
            str << dot + 1 << getName() << '.';
            return ret;
        }
        sub = dot + 1;
        sub2 = dot2 + 1;
    }
    str.str("");
    return nullptr;
}

void DocumentObjectItem::setCheckState(bool checked) {
    if (isSelectionCheckBoxesEnabled())
        QTreeWidgetItem::setCheckState(0, checked ? Qt::Checked : Qt::Unchecked);
    else
        setData(0, Qt::CheckStateRole, QVariant());
}

DocumentItem* DocumentObjectItem::getParentDocument() const {
    return getTree()->getDocumentItem(object()->getDocument());
}

DocumentItem* DocumentObjectItem::getOwnerDocument() const {
    return myOwner;
}

TreeWidget* DocumentObjectItem::getTree() const {
    return static_cast<TreeWidget*>(treeWidget());
}

#include "moc_Tree.cpp"
