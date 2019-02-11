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
# include <QListWidget>
# include <QListWidgetItem>
# include <QLineEdit>
# include <QTextStream>
# include <QToolButton>
# include <QMenu>
# include <QLabel>
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <App/Document.h>
#include <App/GeoFeature.h>
#include "SelectionView.h"
#include "Command.h"
#include "Application.h"
#include "Document.h"
#include "ViewProvider.h"
#include "BitmapFactory.h"



using namespace Gui;
using namespace Gui::DockWnd;


/* TRANSLATOR Gui::DockWnd::SelectionView */

SelectionView::SelectionView(Gui::Document* pcDocument, QWidget *parent)
  : DockWindow(pcDocument,parent)
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

    selectionView = new QListWidget(this);
    selectionView->setContextMenuPolicy(Qt::CustomContextMenu);
    vLayout->addWidget( selectionView );
    resize(200, 200);

    connect(clearButton, SIGNAL(clicked()), searchBox, SLOT(clear()));
    connect(searchBox, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
    connect(searchBox, SIGNAL(editingFinished()), this, SLOT(validateSearch()));
    connect(selectionView, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(select(QListWidgetItem*)));
    connect(selectionView, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(onItemContextMenu(QPoint)));

    Gui::Selection().Attach(this);
}

SelectionView::~SelectionView()
{
    Gui::Selection().Detach(this);
}

/// @cond DOXERR
void SelectionView::OnChange(Gui::SelectionSingleton::SubjectType &rCaller,
                             Gui::SelectionSingleton::MessageType Reason)
{
    Q_UNUSED(rCaller);
    QString selObject;
    QTextStream str(&selObject);
    if (Reason.Type == SelectionChanges::AddSelection) {
        // save as user data
        QStringList list;
        list << QString::fromLatin1(Reason.pDocName);
        list << QString::fromLatin1(Reason.pObjectName);

        // insert the selection as item
        str << Reason.pDocName;
        str << ".";
        str << Reason.pObjectName;
        if (Reason.pSubName[0] != 0 ) {
            str << ".";
            str << Reason.pSubName;
            list << QString::fromLatin1(Reason.pSubName);
        }

        App::Document* doc = App::GetApplication().getDocument(Reason.pDocName);
        App::DocumentObject* obj = doc->getObject(Reason.pObjectName);
        str << " (";
        str << QString::fromUtf8(obj->Label.getValue());
        str << ")";

        QListWidgetItem* item = new QListWidgetItem(selObject, selectionView);
        item->setData(Qt::UserRole, list);
    }
    else if (Reason.Type == SelectionChanges::ClrSelection) {
        // remove all items
        selectionView->clear();
    }
    else if (Reason.Type == SelectionChanges::RmvSelection) {
        // build name
        str << Reason.pDocName;
        str << ".";
        str << Reason.pObjectName;
        if (Reason.pSubName[0] != 0) {
            str << ".";
            str << Reason.pSubName;
        }
        App::Document* doc = App::GetApplication().getDocument(Reason.pDocName);
        App::DocumentObject* obj = doc->getObject(Reason.pObjectName);
        str << " (";
        str << QString::fromUtf8(obj->Label.getValue());
        str << ")";

        // remove all items
        QList<QListWidgetItem *> l = selectionView->findItems(selObject,Qt::MatchExactly);
        if (l.size() == 1)
            delete l[0];

    }
    else if (Reason.Type == SelectionChanges::SetSelection) {
        // remove all items
        selectionView->clear();
        std::vector<SelectionSingleton::SelObj> objs = Gui::Selection().getSelection(Reason.pDocName);
        for (std::vector<SelectionSingleton::SelObj>::iterator it = objs.begin(); it != objs.end(); ++it) {
            // save as user data
            QStringList list;
            list << QString::fromLatin1(it->DocName);
            list << QString::fromLatin1(it->FeatName);

            // build name
            str << it->DocName;
            str << ".";
            str << it->FeatName;
            if (it->SubName && it->SubName[0] != '\0') {
                str << ".";
                str << it->SubName;
                list << QString::fromLatin1(it->SubName);
            }

            App::Document* doc = App::GetApplication().getDocument(it->DocName);
            App::DocumentObject* obj = doc->getObject(it->FeatName);
            str << " (";
            str << QString::fromUtf8(obj->Label.getValue());
            str << ")";

            QListWidgetItem* item = new QListWidgetItem(selObject, selectionView);
            item->setData(Qt::UserRole, list);
            selObject.clear();
        }
    }

    countLabel->setText(QString::number(selectionView->count()));
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
                    // save as user data
                    QString selObject;
                    QTextStream str(&selObject);
                    QStringList list;
                    list << QString::fromLatin1(doc->getName());
                    list << QString::fromLatin1((*it)->getNameInDocument());
                    // build name
                    str << doc->getName();
                    str << ".";
                    str << (*it)->getNameInDocument();
                    str << " (";
                    str << label;
                    str << ")";
                    QListWidgetItem* item = new QListWidgetItem(selObject, selectionView);
                    item->setData(Qt::UserRole, list);
                }
            }
            countLabel->setText(QString::number(selectionView->count()));
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
                if (!Gui::Selection().hasSelection((*it)->getNameInDocument())) {
                    Gui::Selection().addSelection(doc->getName(),(*it)->getNameInDocument(),0);
                }
            }
        }
    }
}

void SelectionView::select(QListWidgetItem* item)
{
    if (!item)
        item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.size() < 2)
        return;

    //Gui::Selection().clearSelection();
    Gui::Command::runCommand(Gui::Command::Gui,"Gui.Selection.clearSelection()");
    //Gui::Selection().addSelection(elements[0].toLatin1(),elements[1].toLatin1(),0);
    QString cmd = QString::fromLatin1("Gui.Selection.addSelection(App.getDocument(\"%1\").getObject(\"%2\"))").arg(elements[0], elements[1]);
    Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
}

void SelectionView::deselect(void)
{
    QListWidgetItem *item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.size() < 2)
        return;

    //Gui::Selection().rmvSelection(elements[0].toLatin1(),elements[1].toLatin1(),0);
    QString cmd = QString::fromLatin1("Gui.Selection.removeSelection(App.getDocument(\"%1\").getObject(\"%2\"))").arg(elements[0], elements[1]);
    Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
}

void SelectionView::zoom(void)
{
    select();
    Gui::Command::runCommand(Gui::Command::Gui,"Gui.SendMsgToActiveView(\"ViewSelection\")");
}

void SelectionView::treeSelect(void)
{
    select();
    Gui::Command::runCommand(Gui::Command::Gui,"Gui.runCommand(\"Std_TreeSelection\")");
}

void SelectionView::touch(void)
{
    QListWidgetItem *item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.size() < 2)
        return;
    QString cmd = QString::fromLatin1("App.getDocument(\"%1\").getObject(\"%2\").touch()").arg(elements[0], elements[1]);
    Gui::Command::runCommand(Gui::Command::Doc,cmd.toLatin1());
}

void SelectionView::toPython(void)
{
    QListWidgetItem *item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.size() < 2)
        return;

    try {
        QString cmd = QString::fromLatin1("obj = App.getDocument(\"%1\").getObject(\"%2\")").arg(elements[0], elements[1]);
        Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
        if (elements.length() > 2) {
            App::Document* doc = App::GetApplication().getDocument(elements[0].toLatin1());
            App::DocumentObject* obj = doc->getObject(elements[1].toLatin1());
            QString property = getProperty(obj);

            cmd = QString::fromLatin1("shp = App.getDocument(\"%1\").getObject(\"%2\").%3")
                    .arg(elements[0], elements[1], property);
            Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());

            if (supportPart(obj, elements[2])) {
                cmd = QString::fromLatin1("elt = App.getDocument(\"%1\").getObject(\"%2\").%3.%4")
                        .arg(elements[0], elements[1], property, elements[2]);
                Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
            }
        }
    }
    catch (const Base::Exception& e) {
        e.ReportException();
    }
}

void SelectionView::showPart(void)
{
    QListWidgetItem *item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.length() > 2) {
        App::Document* doc = App::GetApplication().getDocument(elements[0].toLatin1());
        App::DocumentObject* obj = doc->getObject(elements[1].toLatin1());
        QString module = getModule(obj->getTypeId().getName());
        QString property = getProperty(obj);
        if (!module.isEmpty() && !property.isEmpty() && supportPart(obj, elements[2])) {
            try {
                Gui::Command::addModule(Gui::Command::Gui, module.toLatin1());
                QString cmd = QString::fromLatin1("%1.show(App.getDocument(\"%2\").getObject(\"%3\").%4.%5)")
                        .arg(module, elements[0], elements[1], property, elements[2]);
                Gui::Command::runCommand(Gui::Command::Gui,cmd.toLatin1());
            }
            catch (const Base::Exception& e) {
                e.ReportException();
            }
        }
    }
}

QString SelectionView::getModule(const char* type) const
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

    return QString::fromStdString(prefix);
}

QString SelectionView::getProperty(App::DocumentObject* obj) const
{
    QString property;
    if (obj->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
        App::GeoFeature* geo = static_cast<App::GeoFeature*>(obj);
        const App::PropertyComplexGeoData* data = geo->getPropertyOfGeometry();
        const char* name = data ? data->getName() : nullptr;
        if (name) {
            property = QString::fromLatin1(name);
        }
    }

    return property;
}

bool SelectionView::supportPart(App::DocumentObject* obj, const QString& part) const
{
    if (obj->getTypeId().isDerivedFrom(App::GeoFeature::getClassTypeId())) {
        App::GeoFeature* geo = static_cast<App::GeoFeature*>(obj);
        const App::PropertyComplexGeoData* data = geo->getPropertyOfGeometry();
        if (data) {
            const Data::ComplexGeoData* geometry = data->getComplexData();
            std::vector<const char*> types = geometry->getElementTypes();
            for (auto it : types) {
                if (part.startsWith(QString::fromLatin1(it)))
                    return true;
            }
        }
    }

    return false;
}

void SelectionView::onItemContextMenu(const QPoint& point)
{
    QListWidgetItem *item = selectionView->itemAt(point);
    if (!item)
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

    QStringList elements = item->data(Qt::UserRole).toStringList();
    if (elements.length() > 2) {
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
/// @endcond

#include "moc_SelectionView.cpp"
