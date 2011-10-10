/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <Inventor/nodes/SoGroup.h>
# include <QHeaderView>
#endif

#include "SceneInspector.h"
#include "ui_SceneInspector.h"
#include "MainWindow.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::SceneModel */

SceneModel::SceneModel(QObject* parent)
    : QStandardItemModel(parent)
{
}

SceneModel::~SceneModel()
{
}

int SceneModel::columnCount (const QModelIndex & parent) const
{
    return 1;
}

Qt::ItemFlags SceneModel::flags (const QModelIndex & index) const
{
    return QAbstractItemModel::flags(index);
}

QVariant SceneModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return QVariant();
        if (section == 0)
            return tr("Inventor Tree");
    }

    return QVariant();
}

bool SceneModel::setHeaderData (int, Qt::Orientation, const QVariant &, int)
{
    return false;
}

void SceneModel::setNode(SoNode* node)
{
    this->clear();
    this->setHeaderData(0, Qt::Horizontal, tr("Nodes"), Qt::DisplayRole);

    this->insertColumns(0,1);
    this->insertRows(0,1);
    setNode(this->index(0, 0),node);
}

void SceneModel::setNode(QModelIndex index, SoNode* node)
{
    this->setData(index, QVariant(QString::fromAscii(node->getTypeId().getName())));
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        SoGroup *group = static_cast<SoGroup*>(node);
        // insert SoGroup icon
        this->insertColumns(0,1,index);
        this->insertRows(0,group->getNumChildren(), index);
        for (int i=0; i<group->getNumChildren();i++) {
            SoNode* child = group->getChild(i);
            setNode(this->index(i, 0, index), child);
        }
    }
    // insert icon
}

// --------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgInspector */

DlgInspector::DlgInspector(QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), ui(new Ui_SceneInspector())
{
    ui->setupUi(this);
    setWindowTitle(tr("Scene Inspector"));

    SceneModel* model = new SceneModel(this);
    ui->treeView->setModel(model);
    ui->treeView->setRootIsDecorated(true);
}

/*  
 *  Destroys the object and frees any allocated resources
 */
DlgInspector::~DlgInspector()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgInspector::setNode(SoNode* node)
{
    SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
    model->setNode(node);
    
    QHeaderView* header = ui->treeView->header();
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setMovable(false);
}

void DlgInspector::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        setWindowTitle(tr("Scene Inspector"));
    }
    QDialog::changeEvent(e);
}

void DlgInspector::on_refreshButton_clicked()
{
    View3DInventor* child = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    if (child) {
        View3DInventorViewer* viewer = child->getViewer();
        setNode(viewer->getSceneGraph());
        ui->treeView->expandToDepth(3);
    }
    else {
        SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
        model->clear();
    }
}

#include "moc_SceneInspector.cpp"
