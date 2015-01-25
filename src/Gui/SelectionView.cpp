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
#endif

/// Here the FreeCAD includes sorted by Base,App,Gui......
#include <App/Document.h>
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
    setWindowTitle(tr("Property View"));

    QVBoxLayout* vLayout = new QVBoxLayout(this);
    vLayout->setSpacing(0);
    vLayout->setMargin (0);

    QLineEdit* searchBox = new QLineEdit(this);
#if QT_VERSION >= 0x040700
    searchBox->setPlaceholderText(tr("Search"));
#endif
    searchBox->setToolTip(tr("Searches object labels"));
    QHBoxLayout* hLayout = new QHBoxLayout();
    QToolButton* clearButton = new QToolButton(this);
    clearButton->setFixedSize(18, 21);
    clearButton->setCursor(Qt::ArrowCursor);
    clearButton->setStyleSheet(QString::fromAscii("QToolButton {margin-bottom:6px}"));
    clearButton->setIcon(BitmapFactory().pixmap(":/icons/edit-cleartext.svg"));
    clearButton->setToolTip(tr("Clears the search field"));
    hLayout->addWidget(searchBox);
    hLayout->addWidget(clearButton,0,Qt::AlignRight);
    vLayout->addLayout(hLayout);

    selectionView = new QListWidget(this);
    selectionView->setContextMenuPolicy(Qt::CustomContextMenu);
    vLayout->addWidget( selectionView );
    resize(200, 200);

    connect(clearButton, SIGNAL(clicked()), searchBox, SLOT(clear()));
    connect(searchBox, SIGNAL(textChanged(QString)), this, SLOT(search(QString)));
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
    QString selObject;
    QTextStream str(&selObject);
    if (Reason.Type == SelectionChanges::AddSelection) {
        // insert the selection as item
        str << Reason.pDocName;
        str << ".";
        str << Reason.pObjectName;
        if (Reason.pSubName[0] != 0 ) {
            str << ".";
            str << Reason.pSubName;
        }
        App::Document* doc = App::GetApplication().getDocument(Reason.pDocName);
        App::DocumentObject* obj = doc->getObject(Reason.pObjectName);
        str << " (";
        str << QString::fromUtf8(obj->Label.getValue());
        str << ")";
        
        new QListWidgetItem(selObject, selectionView);
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
            // build name
            str << it->DocName;
            str << ".";
            str << it->FeatName;
            if (it->SubName && it->SubName[0] != '\0') {
                str << ".";
                str << it->SubName;
            }
            App::Document* doc = App::GetApplication().getDocument(it->DocName);
            App::DocumentObject* obj = doc->getObject(it->FeatName);
            str << " (";
            str << QString::fromUtf8(obj->Label.getValue());
            str << ")";
            
            new QListWidgetItem(selObject, selectionView);
        }
    }
}

void SelectionView::search(const QString& text)
{
    if (!text.isEmpty()) {
        App::Document* doc = App::GetApplication().getActiveDocument();
        std::vector<App::DocumentObject*> objects;
        if (doc) {
            Gui::Selection().clearSelection();
            objects = doc->getObjects();
            for (std::vector<App::DocumentObject*>::iterator it = objects.begin(); it != objects.end(); ++it) {
                QString label = QString::fromUtf8((*it)->Label.getValue());
                if (label.contains(text,Qt::CaseInsensitive)) {
                    if (!Gui::Selection().hasSelection((*it)->getNameInDocument())) {
                        Gui::Selection().addSelection(doc->getName(),(*it)->getNameInDocument(),0);
                    }
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
    QStringList elements = item->text().split(QString::fromAscii("."));
    // remove possible space from object name followed by label
    elements[1] = elements[1].split(QString::fromAscii(" "))[0];
    //Gui::Selection().clearSelection();
    Gui::Command::runCommand(Gui::Command::Gui,"Gui.Selection.clearSelection()");
    //Gui::Selection().addSelection(elements[0].toAscii(),elements[1].toAscii(),0);
    QString cmd = QString::fromAscii("Gui.Selection.addSelection(App.getDocument(\"%1\").getObject(\"%2\"))").arg(elements[0]).arg(elements[1]);
    Gui::Command::runCommand(Gui::Command::Gui,cmd.toAscii());
}

void SelectionView::deselect(void)
{
    QListWidgetItem *item = selectionView->currentItem();
    if (!item)
        return;
    QStringList elements = item->text().split(QString::fromAscii("."));
    // remove possible space from object name followed by label
    elements[1] = elements[1].split(QString::fromAscii(" "))[0];
    //Gui::Selection().rmvSelection(elements[0].toAscii(),elements[1].toAscii(),0);
    QString cmd = QString::fromAscii("Gui.Selection.removeSelection(App.getDocument(\"%1\").getObject(\"%2\"))").arg(elements[0]).arg(elements[1]);
    Gui::Command::runCommand(Gui::Command::Gui,cmd.toAscii());
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

void SelectionView::onItemContextMenu(const QPoint& point)
{
    QListWidgetItem *item = selectionView->itemAt(point);
    if (!item)
        return;
    QMenu menu;
    QAction *selectAction = menu.addAction(tr("Select only"),this,SLOT(select()));
    selectAction->setIcon(QIcon(QString::fromAscii(":/icons/view-select.svg")));
    selectAction->setToolTip(tr("Selects only this object"));
    QAction *deselectAction = menu.addAction(tr("Deselect"),this,SLOT(deselect()));
    deselectAction->setIcon(QIcon(QString::fromAscii(":/icons/view-unselectable.svg")));
    deselectAction->setToolTip(tr("Deselects this object"));
    QAction *zoomAction = menu.addAction(tr("Zoom fit"),this,SLOT(zoom()));
    zoomAction->setIcon(QIcon(QString::fromAscii(":/icons/view-zoom-fit.svg")));
    zoomAction->setToolTip(tr("Selects and fits this object in the 3D window"));
    QAction *gotoAction = menu.addAction(tr("Go to selection"),this,SLOT(treeSelect()));
    gotoAction->setToolTip(tr("Selects and locates this object in the tree view"));
    menu.exec(selectionView->mapToGlobal(point));
}

void SelectionView::onUpdate(void)
{
}

bool SelectionView::onMsg(const char* pMsg,const char** ppReturn)
{
    return false;
}
/// @endcond

#include "moc_SelectionView.cpp"
