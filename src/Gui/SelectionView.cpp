/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QVBoxLayout>
# include <QTreeWidget>
# include <QTreeWidgetItem>
# include <QLineEdit>
# include <QTextStream>
# include <QToolButton>
# include <QCheckBox>
# include <QMenu>
# include <QLabel>
# include <QApplication>
# include <QHeaderView>
# include <QFile>
#endif

#include <QHelpEvent>
#include <QToolTip>

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <Base/Console.h>
#include <App/Document.h>
#include <App/GeoFeature.h>
#include <App/DocumentObserver.h>
#include "SelectionView.h"
#include "CommandT.h"
#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"
#include "BitmapFactory.h"
#include "MetaTypes.h"
#include "MainWindow.h"
#include "Widgets.h"
#include "PieMenu.h"

FC_LOG_LEVEL_INIT("Selection",true,true,true)

using namespace Gui;
using namespace Gui::DockWnd;

/* TRANSLATOR Gui::DockWnd::SelectionView */

enum ColumnIndex {
    LabelIndex,
    ElementIndex,
    PathIndex,
    LastIndex,
};

SelectionView::SelectionView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
  , SelectionObserver(false,0)
  , x(0.0f), y(0.0f), z(0.0f)
{
    setWindowTitle(tr("Selection View"));

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    vLayout->setMargin (0);

    QLineEdit* searchBox = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    searchBox->setPlaceholderText(tr("Search"));
#endif
    searchBox->setToolTip(tr("Searches object labels"));
    QHBoxLayout* hLayout = new QHBoxLayout();
    hLayout->setSpacing(2);
    QToolButton* clearButton = new QToolButton(this);
    clearButton->setFixedSize(18, 21);
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet(QString::fromUtf8("QToolButton {margin-bottom:1px}"));
    clearButton->setIcon(BitmapFactory().pixmap(":/icons/edit-cleartext.svg"));
    clearButton->setToolTip(tr("Clears the search field"));
    clearButton->setAutoRaise(true);
    countLabel = new QLabel(this);
    countLabel->setText(QString::fromUtf8("0"));
    countLabel->setToolTip(tr("The number of selected items"));
    hLayout->addWidget(searchBox);
    hLayout->addWidget(clearButton,0,Qt::AlignRight);
    hLayout->addWidget(countLabel,0,Qt::AlignRight);
    vLayout->addLayout(hLayout);

    selectionView = new QTreeWidget(this);
    selectionView->setColumnCount(LastIndex);
    selectionView->headerItem()->setText(LabelIndex, tr("Label"));
    selectionView->headerItem()->setText(ElementIndex, tr("Element"));
    selectionView->headerItem()->setText(PathIndex, tr("Path"));

    selectionView->setContextMenuPolicy(Qt::CustomContextMenu);
    vLayout->addWidget( selectionView );

    enablePickList = new QCheckBox(this);
    enablePickList->setText(tr("Picked object list"));
    hLayout->addWidget(enablePickList);

    pickList = new QTreeWidget(this);
    pickList->header()->setSortIndicatorShown(true);
    pickList->setColumnCount(LastIndex);
    pickList->sortByColumn(ElementIndex, Qt::AscendingOrder);
    pickList->headerItem()->setText(LabelIndex, tr("Label"));
    pickList->headerItem()->setText(ElementIndex, tr("Element"));
    pickList->headerItem()->setText(PathIndex, tr("Path"));
    pickList->setVisible(false);
    vLayout->addWidget(pickList);

    for(int i=0; i<LastIndex; ++i) {
#if QT_VERSION >= 0x050000
        selectionView->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
        pickList->header()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
#else
        selectionView->header()->setResizeMode(i, QHeaderView::ResizeToContents);
        pickList->header()->setResizeMode(i, QHeaderView::ResizeToContents);
#endif
    }

#if QT_VERSION >= 0x040200
    selectionView->setMouseTracking(true); // needed for itemEntered() to work
    pickList->setMouseTracking(true);
#endif

    resize(200, 200);

    connect(clearButton, SIGNAL(clicked()), searchBox, SLOT(clear()));
    connect(searchBox, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
    connect(searchBox, SIGNAL(editingFinished()), this, SLOT(validateSearch()));
    connect(selectionView, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(toggleSelect(QTreeWidgetItem*)));
    connect(selectionView, SIGNAL(itemEntered(QTreeWidgetItem*, int)), this, SLOT(preselect(QTreeWidgetItem*)));
    connect(pickList, SIGNAL(itemDoubleClicked(QTreeWidgetItem*, int)), this, SLOT(toggleSelect(QTreeWidgetItem*)));
    connect(pickList, SIGNAL(itemEntered(QTreeWidgetItem*, int)), this, SLOT(preselect(QTreeWidgetItem*)));
    connect(selectionView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onItemContextMenu(QPoint)));
    connect(enablePickList, SIGNAL(stateChanged(int)), this, SLOT(onEnablePickList()));

}

SelectionView::~SelectionView()
{
}

void SelectionView::leaveEvent(QEvent *)
{
    Selection().rmvPreselect();
}

static void addItem(QTreeWidget *tree, const App::SubObjectT &objT)
{
    auto obj = objT.getSubObject();
    if(!obj)
        return;

    auto* item = new QTreeWidgetItem(tree);
    item->setText(LabelIndex, QString::fromUtf8(obj->Label.getStrValue().c_str()));

    item->setText(ElementIndex, QString::fromLatin1(objT.getOldElementName().c_str()));

    item->setText(PathIndex, QString::fromLatin1("%1#%2.%3").arg(
                QString::fromLatin1(objT.getDocumentName().c_str()),
                QString::fromLatin1(objT.getObjectName().c_str()),
                QString::fromLatin1(objT.getSubName().c_str())));

    auto vp = Application::Instance->getViewProvider(obj);
    if(vp)
        item->setIcon(0, vp->getIcon());

    // save as user data
    item->setData(0, Qt::UserRole, QVariant::fromValue(objT));
}

/// @cond DOXERR
void SelectionView::onSelectionChanged(const SelectionChanges &Reason)
{
    QString selObject;
    QTextStream str(&selObject);
    if (Reason.Type == SelectionChanges::AddSelection) {
        addItem(selectionView, Reason.Object);
    }
    else if (Reason.Type == SelectionChanges::ClrSelection) {
        if(!Reason.pDocName[0]) {
            // remove all items
            selectionView->clear();
        }else{
            // build name
            str << Reason.pDocName;
            str << "#";
            // remove all items
            for(auto item : selectionView->findItems(selObject,Qt::MatchStartsWith,PathIndex))
                delete item;
        }
    }
    else if (Reason.Type == SelectionChanges::RmvSelection) {
        // build name
        str << Reason.pDocName << "#" << Reason.pObjectName << "." << Reason.pSubName;

        // remove all items
        for(auto item : selectionView->findItems(selObject,Qt::MatchExactly,PathIndex))
            delete item;
    }
    else if (Reason.Type == SelectionChanges::SetSelection) {
        // remove all items
        selectionView->clear();
        for(auto &objT : Gui::Selection().getSelectionT("*",0))
            addItem(selectionView, objT);
    }
    else if (Reason.Type == SelectionChanges::PickedListChanged) {
        bool picking = Selection().needPickedList();
        enablePickList->setChecked(picking);
        pickList->setVisible(picking);
        pickList->clear();
        if(picking) {
            pickList->setSortingEnabled(false);
            for(auto &objT : Selection().getPickedList("*"))
                addItem(pickList, objT);
            pickList->setSortingEnabled(true);
        }
    }

    countLabel->setText(QString::number(selectionView->topLevelItemCount()));
}

void SelectionView::search(const QString& text)
{
    if (!text.isEmpty()) {
        searchList.clear();
        App::Document* doc = App::GetApplication().getActiveDocument();
        std::vector<App::DocumentObject*> objects;
        if (doc) {
            objects = doc->getObjects();
            selectionView->clear();
            for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it) {
                QString label = QString::fromUtf8((*it)->Label.getValue());
                if (label.contains(text,Qt::CaseInsensitive)) {
                    searchList.push_back(*it);
                    addItem(selectionView, App::SubObjectT(*it, ""));
                }
            }
            countLabel->setText(QString::number(selectionView->topLevelItemCount()));
        }
    }
}

void SelectionView::validateSearch(void)
{
    if (!searchList.empty()) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        if (doc) {
            Gui::Selection().clearSelection();
            for (std::vector<App::DocumentObject*>::iterator it = searchList.begin(); it != searchList.end(); ++it) {
                Gui::Selection().addSelection(doc->getName(),(*it)->getNameInDocument(),0);
            }
        }
    }
}

void SelectionView::select(QTreeWidgetItem* item)
{
    if (!item)
        item = selectionView->currentItem();
    if (!item)
        return;

    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    if(!objT.getSubObject())
        return;

    try {
        doCommandT(Command::Gui, "Gui.Selection.clearSelection()");
        doCommandT(Command::Gui, "Gui.Selection.addSelection(%s, '%s')",
                Command::getObjectCmd(objT.getObject()), objT.getSubName());
    }catch(Base::Exception &e) {
        e.ReportException();
    }
}

void SelectionView::deselect(void)
{
    auto item = selectionView->currentItem();
    if (!item)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    if(!objT.getSubObject())
        return;

    try {
        doCommandT(Command::Gui, "Gui.Selection.removeSelection(%s, '%s')",
                Command::getObjectCmd(objT.getObject()), objT.getSubName());
    }catch(Base::Exception &e) {
        e.ReportException();
    }
}

void SelectionView::toggleSelect(QTreeWidgetItem* item)
{
    if (!item) return;

    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    if(!objT.getSubObject())
        return;

    try {
        bool selected = Gui::Selection().isSelected(objT.getDocumentName().c_str(),
                                                    objT.getObjectName().c_str(),
                                                    objT.getSubName().c_str());
        doCommandT(Command::Gui, "Gui.Selection.%s(%s,'%s')",
                selected ? "removeSelection" : "addSelection",
                Command::getObjectCmd(objT.getObject()),
                objT.getSubName());

    }catch(Base::Exception &e) {
        e.ReportException();
    }
}

void SelectionView::preselect(QTreeWidgetItem* item)
{
    if (!item) return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    if(!objT.getSubObject())
        return;
    Gui::Selection().setPreselect(objT.getDocumentName().c_str(),
                                  objT.getObjectName().c_str(),
                                  objT.getSubName().c_str(),0,0,0,2);
}

void SelectionView::zoom(void)
{
    select();
    try {
        Gui::Command::runCommand(Gui::Command::Gui,"Gui.SendMsgToActiveView(\"ViewSelection\")");
    }catch(Base::Exception &e) {
        e.ReportException();
    }
}

void SelectionView::treeSelect(void)
{
    select();
    try {
        Gui::Command::runCommand(Gui::Command::Gui,"Gui.runCommand(\"Std_TreeSelection\")");
    }catch(Base::Exception &e) {
        e.ReportException();
    }
}

void SelectionView::touch(void)
{
    auto item = selectionView->currentItem();
    if (!item)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    auto sobj = objT.getSubObject();
    if(!sobj)
        return;
    if(sobj) {
        try {
            cmdAppObject(sobj,"touch()");
        }catch(Base::Exception &e) {
            e.ReportException();
        }
    }
}

void SelectionView::toPython(void)
{
    auto item = selectionView->currentItem();
    if (!item)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    auto sobj = objT.getSubObject();
    if(!sobj)
        return;
    try {
        doCommandT(Command::Gui, "_obj, _matrix, _shp = %s.getSubObject('%s', retType=2)",
                Command::getObjectCmd(objT.getObject()), objT.getSubName());
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

static std::string getModule(const char* type)
{
    // go up the inheritance tree and find the module name of the first
    // sub-class that has not the prefix "App::"
    std::string prefix;
    Base::Type typeId = Base::Type::fromName(type);

    while (!typeId.isBad()) {
        std::string temp(typeId.getName());
        std::string::size_type pos = temp.find_first_of("::");

        std::string module;
        if (pos != std::string::npos)
            module = std::string(temp,0,pos);
        if (module != "App")
            prefix = module;
        else
            break;
        typeId = typeId.getParent();
    }

    return prefix;
}

void SelectionView::showPart(void)
{
    auto *item = selectionView->currentItem();
    if (!item)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    auto sobj = objT.getSubObject();
    if(!sobj)
        return;
    std::string module = getModule(sobj->getTypeId().getName());
    if (!module.empty()) {
        try {
            doCommandT(Command::Gui, "%s.show(%s.getSubObject('%s'))",
                    module, Command::getObjectCmd(objT.getObject()), objT.getSubName());
        }
        catch (const Base::Exception& e) {
            e.ReportException();
        }
    }
}

void SelectionView::onItemContextMenu(const QPoint& point)
{
    auto item = selectionView->itemAt(point);
    if (!item)
        return;
    auto objT = qvariant_cast<App::SubObjectT>(item->data(0, Qt::UserRole));
    auto sobj = objT.getSubObject();
    if(!sobj)
        return;

    QMenu menu;
    QAction *selectAction = menu.addAction(tr("Select only"),this,SLOT(select()));
    selectAction->setIcon(QIcon::fromTheme(QString::fromLatin1("view-select")));
    selectAction->setToolTip(tr("Selects only this object"));
    QAction *deselectAction = menu.addAction(tr("Deselect"),this,SLOT(deselect()));
    deselectAction->setIcon(QIcon::fromTheme(QString::fromLatin1("view-unselectable")));
    deselectAction->setToolTip(tr("Deselects this object"));
    QAction *zoomAction = menu.addAction(tr("Zoom fit"),this,SLOT(zoom()));
    zoomAction->setIcon(QIcon::fromTheme(QString::fromLatin1("zoom-fit-best")));
    zoomAction->setToolTip(tr("Selects and fits this object in the 3D window"));
    QAction *gotoAction = menu.addAction(tr("Go to selection"),this,SLOT(treeSelect()));
    gotoAction->setToolTip(tr("Selects and locates this object in the tree view"));
    QAction *touchAction = menu.addAction(tr("Mark to recompute"),this,SLOT(touch()));
    touchAction->setIcon(QIcon::fromTheme(QString::fromLatin1("view-refresh")));
    touchAction->setToolTip(tr("Mark this object to be recomputed"));
    QAction *toPythonAction = menu.addAction(tr("To python console"),this,SLOT(toPython()));
    toPythonAction->setIcon(QIcon::fromTheme(QString::fromLatin1("applications-python")));
    toPythonAction->setToolTip(tr("Reveals this object and its subelements in the python console."));

    if (objT.getOldElementName().size()) {
        // subshape-specific entries
        QAction *showPart = menu.addAction(tr("Duplicate subshape"),this,SLOT(showPart()));
        showPart->setIcon(QIcon(QString::fromLatin1(":/icons/ClassBrowser/member.svg")));
        showPart->setToolTip(tr("Creates a standalone copy of this subshape in the document"));
    }
    menu.exec(selectionView->mapToGlobal(point));
}

void SelectionView::onUpdate(void)
{
}

bool SelectionView::onMsg(const char* /*pMsg*/,const char** /*ppReturn*/)
{
    return false;
}

void SelectionView::hideEvent(QHideEvent *ev) {
    FC_TRACE(this << " detaching selection observer");
    this->detachSelection();
    DockWindow::hideEvent(ev);
}

void SelectionView::showEvent(QShowEvent *ev) {
    FC_TRACE(this << " attaching selection observer");
    this->attachSelection();

    selectionView->clear();
    for(auto &objT : Gui::Selection().getSelectionT("*",0))
        addItem(selectionView, objT);

    bool picking = Selection().needPickedList();
    enablePickList->setChecked(picking);
    pickList->clear();
    if(picking) {
        pickList->setSortingEnabled(false);
        for(auto &objT : Selection().getPickedList("*"))
            addItem(pickList, objT);
        pickList->setSortingEnabled(true);
    }

    Gui::DockWindow::showEvent(ev);
}

void SelectionView::onEnablePickList() {
    bool enabled = enablePickList->isChecked();
    Selection().enablePickedList(enabled);
    pickList->setVisible(enabled);
}

/// @endcond

////////////////////////////////////////////////////////////////////////

#if QT_VERSION  >= 0x050000
static QString _DefaultStyle = QLatin1String(
"*{ background-color: transparent;"
   "border: none;}"
"QMenu { background-color: rgba(255,255,255,130); menu-scrollable:1; }"
"QMenu::item { color: palette(text); background-color: transparent; }"
"QMenu::separator { height: 1px; margin: 6px 4px; }"
"QMenu::item:selected, QMenu::item:pressed {"
  "background-color: rgba(0,0,130,130);}"
"QMenu::item:disabled { color: palette(mid); }");
#else
static QString _DefaultStyle = QLatin1String("QMenu {menu-scrollable:1}");
#endif

namespace Gui {
void setupMenuStyle(QWidget *menu)
{
#if QT_VERSION  >= 0x050000
    auto hGrp = App::GetApplication().GetParameterGroupByPath(
                    "User parameter:BaseApp/Preferences/MainWindow");
    static QString _Name;
    static QString _Stylesheet;
    QString name = QString::fromUtf8(hGrp->GetASCII("MenuStyleSheet").c_str());
    if(name.isEmpty()) {
        QString mainstyle = QString::fromUtf8(hGrp->GetASCII("StyleSheet").c_str());
        if(mainstyle.indexOf(QLatin1String("dark"),0,Qt::CaseInsensitive)>=0)
            name = QString::fromLatin1("qssm:Dark.qss");
        else if(mainstyle.indexOf(QLatin1String("light"),0,Qt::CaseInsensitive)>=0)
            name = QString::fromLatin1("qssm:Light.qss");
        else
            name = QString::fromLatin1("qssm:Default.qss");
    } else if (!QFile::exists(name))
        name = QString::fromLatin1("qssm:%1").arg(name);
    if(_Name != name) {
        _Name = name;
        _Stylesheet.clear();
        if(QFile::exists(name)) {
            QFile f(name);
            if(f.open(QFile::ReadOnly)) {
                QTextStream str(&f);
                _Stylesheet = str.readAll();
            }
        }
    }
    if(_Stylesheet.isEmpty())
        _Stylesheet = _DefaultStyle;

    if (menu->styleSheet() == _Stylesheet)
        return;

    menu->setStyleSheet(_Stylesheet);
    menu->setWindowFlags(menu->windowFlags() | Qt::FramelessWindowHint);
    menu->setAttribute(Qt::WA_NoSystemBackground, true);
    menu->setAttribute(Qt::WA_TranslucentBackground, true);
#else
    if (menu->styleSheet() != _DefaultStyle)
        menu->setStyleSheet(_DefaultStyle);
#endif
}
}

SelectionMenu::SelectionMenu(QWidget *parent)
    :QMenu(parent),pSelList(0),tooltipIndex(0)
{
    connect(this, SIGNAL(aboutToShow()), this, SLOT(beforeShow()));
    setupMenuStyle(this);
}

void SelectionMenu::beforeShow()
{
#if QT_VERSION  >= 0x050000
    for(auto child : findChildren<QMenu*>()) {
        child->setWindowFlags(child->windowFlags() | Qt::FramelessWindowHint);
        child->setAttribute(Qt::WA_NoSystemBackground, true);
        child->setAttribute(Qt::WA_TranslucentBackground, true);
    }
#endif
}

struct ElementInfo {
    QMenu *menu = nullptr;
    QIcon icon;
    std::vector<int> indices;
};

struct SubMenuInfo {
    QMenu *menu = nullptr;

    // Map from sub-object label to map from object path to element info. The
    // reason of the second map is to disambiguate sub-object with the same
    // label, but different object or object path
    std::map<std::string, std::map<std::string, ElementInfo> > items;
};

void SelectionMenu::doPick(const std::vector<App::SubObjectT> &sels) {
    clear();

    pSelList = &sels;
    std::ostringstream ss;
    std::map<std::string, SubMenuInfo> menus;
    std::map<App::DocumentObject*, QIcon> icons;

    int i=-1;
    for(auto &sel : sels) {
        ++i;
        auto sobj = sel.getSubObject();
        if(!sobj)
            continue;

        ss.str("");
        int index = -1;
        std::string element = sel.getOldElementName(&index);
        if(index < 0)
            continue;
        ss << sel.getObjectName() << '.' << sel.getSubNameNoElement();
        std::string key = ss.str();

        auto &icon = icons[sobj];
        if(icon.isNull()) {
            auto vp = Application::Instance->getViewProvider(sobj);
            if(vp)
                icon = vp->getIcon();
        }

        auto &elementInfo = menus[element].items[sobj->Label.getStrValue()][key];
        elementInfo.icon = icon;
        elementInfo.indices.push_back(i);

        auto geoFeature = Base::freecad_dynamic_cast<App::GeoFeature>(
                sobj->getLinkedObject(true));
        if (geoFeature) {
            for(auto element : geoFeature->getElementTypes(true))
                menus[element];
        }
    }

    for(auto &v : menus) {
        auto &info = v.second;
        if (info.items.empty()) {
            QAction *action = addAction(QLatin1String(v.first.c_str()));
            action->setDisabled(true);
            continue;
        }

        info.menu = addMenu(QLatin1String(v.first.c_str()));
        info.menu->installEventFilter(this);

        bool groupMenu = false;
        if(info.items.size() > 20)
            groupMenu = true;
        else {
            std::size_t objCount = 0;
            std::size_t count = 0;
            for(auto &vv : info.items) {
                objCount += vv.second.size();
                for(auto &vvv : vv.second) 
                    count += vvv.second.indices.size();
                if(count > 20 && objCount>1) {
                    groupMenu = true;
                    break;
                }
            }
        }

        for(auto &vv : info.items) {
            const std::string &label = vv.first;

            for(auto &vvv : vv.second) {
                auto &elementInfo = vvv.second;

                if(!groupMenu) {
                    for(int idx : elementInfo.indices) {
                        ss.str("");
                        ss << label << " (" << sels[idx].getOldElementName() << ")";
                        QAction *action = info.menu->addAction(elementInfo.icon, QString::fromUtf8(ss.str().c_str()));
                        action->setData(idx+1);
                        connect(info.menu, SIGNAL(hovered(QAction*)), this, SLOT(onHover(QAction*)));
                    }
                    continue;
                }
                if(!elementInfo.menu) {
                    elementInfo.menu = info.menu->addMenu(elementInfo.icon, QString::fromUtf8(label.c_str()));
                    elementInfo.menu->installEventFilter(this);
                    connect(elementInfo.menu, SIGNAL(aboutToShow()),this,SLOT(onSubMenu()));
                    connect(elementInfo.menu, SIGNAL(hovered(QAction*)), this, SLOT(onHover(QAction*)));
                }
                for(int idx : elementInfo.indices) {
                    QAction *action = elementInfo.menu->addAction(
                            QString::fromUtf8(sels[idx].getOldElementName().c_str()));
                    action->setData(idx+1);
                }
            }
        }
    }
    bool toggle = !Gui::Selection().needPickedList();
    if(toggle)
        Gui::Selection().enablePickedList(true);

    Gui::Selection().rmvPreselect();

    QAction* picked = PieMenu::exec(this, QCursor::pos(), "Std_PickGeometry");

    ToolTip::hideText();

    if(picked) {
        int idx = picked->data().toInt();
        if(idx>0 && idx<=(int)sels.size()) {
            auto &sel = sels[idx-1];
            auto modifier = QApplication::queryKeyboardModifiers();
            if (modifier == Qt::ShiftModifier) {
                TreeWidget::selectUp(sel);
            } else {
                if (modifier == Qt::ControlModifier) {
                    Gui::Selection().selStackPush();
                    Gui::Selection().clearSelection();
                }
                Gui::Selection().addSelection(sel.getDocumentName().c_str(),
                        sel.getObjectName().c_str(), sel.getSubName().c_str());
                Gui::Selection().selStackPush();
            }
        }
    }
    pSelList = 0;
    if(toggle)
        Gui::Selection().enablePickedList(false);
}

void SelectionMenu::onHover(QAction *action) {
    if(!pSelList) {
        ToolTip::hideText();
        return;
    }
    int idx = action->data().toInt();
    if(idx<=0 || idx>(int)pSelList->size()) {
        ToolTip::hideText();
        return;
    }

    auto &sel = (*pSelList)[idx-1];
    Gui::Selection().setPreselect(sel.getDocumentName().c_str(),
            sel.getObjectName().c_str(), sel.getSubName().c_str(),0,0,0,2, true);
    tooltipIndex = idx;
    showToolTip();
}

void SelectionMenu::leaveEvent(QEvent *event) {
    ToolTip::hideText();
    QMenu::leaveEvent(event);
}

void SelectionMenu::showToolTip() {
    if(QApplication::queryKeyboardModifiers() != Qt::ShiftModifier) {
        ToolTip::hideText();
        return;
    }

    bool needElement = tooltipIndex > 0;
    if(!needElement)
        tooltipIndex = -tooltipIndex;

    if(tooltipIndex <= 0 || tooltipIndex > (int)pSelList->size())
        return;

    auto &sel = (*pSelList)[tooltipIndex-1];
    auto sobj = sel.getSubObject();
    QString tooltip;

    QString element;
    if(needElement)
        element = QString::fromLatin1(sel.getOldElementName().c_str());

    if(sobj)
        tooltip = QString::fromLatin1("%1 (%2.%3%4)").arg(
                        QString::fromUtf8(sobj->Label.getValue()),
                        QString::fromLatin1(sel.getObjectName().c_str()),
                        QString::fromLatin1(sel.getSubNameNoElement().c_str()),
                        element);
    else
        tooltip = QString::fromLatin1("%1.%2%3").arg(
                        QString::fromLatin1(sel.getObjectName().c_str()),
                        QString::fromLatin1(sel.getSubNameNoElement().c_str()),
                        element);
    ToolTip::showText(QCursor::pos(), tooltip, this);
}

void SelectionMenu::onSubMenu() {
    auto submenu = qobject_cast<QMenu*>(sender());
    if(!submenu)
        return;
    auto actions = submenu->actions();
    if(!actions.size())
        return;
    int idx = actions.front()->data().toInt();
    if(idx<=0 || idx>(int)pSelList->size())
        return;

    ToolTip::hideText();

    auto &sel = (*pSelList)[idx-1];

    const char *element = sel.getElementName();
    std::string subname(sel.getSubName().c_str(),element);

    Gui::Selection().setPreselect(sel.getDocumentName().c_str(),
            sel.getObjectName().c_str(), subname.c_str(),0,0,0,2,true);

    tooltipIndex = -idx;
    showToolTip();
}

bool SelectionMenu::eventFilter(QObject *o, QEvent *ev)
{
    if(tooltipIndex == 0 || std::abs(tooltipIndex) > (int)pSelList->size())
        return QMenu::eventFilter(o, ev);

    switch(ev->type()) {
    case QEvent::MouseButtonRelease: {
        auto me = static_cast<QMouseEvent*>(ev);
        if (me->button() == Qt::RightButton) {
            activeMenu = qobject_cast<QMenu*>(o);
            if (activeMenu) {
                QMetaObject::invokeMethod(this, "onSelUpMenu", Qt::QueuedConnection);
                return true;
            }
        }
        break;
    }
    default:
        break;
    }
    return QMenu::eventFilter(o, ev);
}

void SelectionMenu::onSelUpMenu()
{
    int idx = std::abs(tooltipIndex);
    if(!activeMenu || idx == 0 || idx > (int)pSelList->size())
        return;

    auto &sel = (*pSelList)[idx-1];
    auto sobj = sel.getSubObject();
    if (!sobj)
        return;

    if(QApplication::queryKeyboardModifiers() == Qt::ShiftModifier) {
        TreeWidget::selectUp(sel, this);
        return;
    }
    SelUpMenu menu(activeMenu);
    TreeWidget::populateSelUpMenu(&menu, &sel);
    TreeWidget::execSelUpMenu(&menu, QCursor::pos());
}

// --------------------------------------------------------------------

SelUpMenu::SelUpMenu(QWidget *parent, bool trigger)
    :QMenu(parent)
{
    if (trigger)
        connect(this, SIGNAL(triggered(QAction*)), this, SLOT(onTriggered(QAction *)));
    connect(this, SIGNAL(hovered(QAction*)), this, SLOT(onHovered(QAction *)));
    setupMenuStyle(this);
}

bool SelUpMenu::event(QEvent *e)
{
    return QMenu::event(e);
}

void SelUpMenu::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        QAction *action = actionAt(e->pos());
        if (action) {
            TreeWidget::selectUp(action, this);
            return;
        }
    }
    QMenu::mouseReleaseEvent(e);
}

void SelUpMenu::mousePressEvent(QMouseEvent *e)
{
	for (QWidget *w = this; w; w = qobject_cast<QMenu*>(w->parentWidget())) {
        if (w->rect().contains(w->mapFromGlobal(e->globalPos()))) {
            if (w == this)
                QMenu::mousePressEvent(e);
            break;
        }
        w->hide();
    }
}

void SelUpMenu::onTriggered(QAction *action)
{
    TreeWidget::selectUp(action);
}

void SelUpMenu::onHovered(QAction *action)
{
    ToolTip::hideText();

    App::SubObjectT objT = qvariant_cast<App::SubObjectT>(action->data());
    auto obj = objT.getSubObject();
    if (!obj) {
        getMainWindow()->showMessage(QString());
        Selection().rmvPreselect();
    } else {
        Selection().setPreselect(objT.getDocumentName().c_str(),
                objT.getObjectName().c_str(), objT.getSubName().c_str(), 0,0,0,2,true);

        QString status;
        status = QString::fromLatin1("%1 (%2#%3.%4)%5").arg(
                QString::fromLatin1(obj->getNameInDocument()),
                QString::fromLatin1(objT.getDocumentName().c_str()),
                QString::fromLatin1(objT.getObjectName().c_str()),
                QString::fromLatin1(objT.getSubName().c_str()),
                status);

        if ( (obj->isTouched() || obj->mustExecute() == 1) && !obj->isError()) {
            status += QString::fromLatin1(", ");
            status += tr("Touched");
        }

        getMainWindow()->showMessage(status);
    }

    if(QApplication::queryKeyboardModifiers() == Qt::ShiftModifier) {
        const QString &toolTip = action->toolTip();
        ToolTip::showText(QCursor::pos(), toolTip, this);
    }
}

#include "moc_SelectionView.cpp"
