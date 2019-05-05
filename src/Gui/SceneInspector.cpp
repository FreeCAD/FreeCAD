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
# include <Inventor/nodes/SoSeparator.h>
# include <QHeaderView>
# include <QTextStream>
#endif

#include "SceneInspector.h"
#include "ui_SceneInspector.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObject.h"
#include "Document.h"
#include "Application.h"
#include <App/Document.h>

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
    Q_UNUSED(parent); 
    return 2;
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
        else if (section == 1)
            return tr("Name");
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

    this->insertColumns(0,2);
    this->insertRows(0,1);
    setNode(this->index(0, 0), node);
}

void SceneModel::setNode(QModelIndex index, SoNode* node)
{
    this->setData(index, QVariant(QString::fromLatin1(node->getTypeId().getName())));
    if (node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId())) {
        SoGroup *group = static_cast<SoGroup*>(node);
        // insert SoGroup icon
        this->insertColumns(0,2,index);
        this->insertRows(0,group->getNumChildren(), index);
        for (int i=0; i<group->getNumChildren();i++) {
            SoNode* child = group->getChild(i);
            setNode(this->index(i, 0, index), child);

            QHash<SoNode*, QString>::iterator it = nodeNames.find(child);
            QString name;
            QTextStream stream(&name);
            stream << child << ", ";
            if(child->isOfType(SoSwitch::getClassTypeId())) {
                auto pcSwitch = static_cast<SoSwitch*>(child);
                stream << pcSwitch->whichChild.getValue() << ", ";
            } else if (child->isOfType(SoSeparator::getClassTypeId())) {
                auto pcSeparator = static_cast<SoSeparator*>(child);
                stream << pcSeparator->renderCaching.getValue() << ", ";
            }
            if (it != nodeNames.end())
                stream << it.value();
            else
                stream << child->getName();
            this->setData(this->index(i, 1, index), QVariant(name));
        }
    }
    // insert icon
}

void SceneModel::setNodeNames(const QHash<SoNode*, QString>& names)
{
    nodeNames = names;
}

// --------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgInspector */

DlgInspector::DlgInspector(QWidget* parent, Qt::WindowFlags fl)
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

void DlgInspector::setDocument(Gui::Document* doc)
{
    setNodeNames(doc);

    View3DInventor* view = qobject_cast<View3DInventor*>(doc->getActiveView());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        setNode(viewer->getSceneGraph());
        ui->treeView->expandToDepth(3);
    }
}

void DlgInspector::setNode(SoNode* node)
{
    SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
    model->setNode(node);
    
    QHeaderView* header = ui->treeView->header();
#if QT_VERSION >= 0x050000
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    header->setSectionsMovable(false);
#else
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setMovable(false);
#endif
}

void DlgInspector::setNodeNames(Gui::Document* doc)
{
    std::vector<Gui::ViewProvider*> vps = doc->getViewProvidersOfType
            (Gui::ViewProviderDocumentObject::getClassTypeId());
    QHash<SoNode*, QString> nodeNames;
    for (std::vector<Gui::ViewProvider*>::iterator it = vps.begin(); it != vps.end(); ++it) {
        Gui::ViewProviderDocumentObject* vp = static_cast<Gui::ViewProviderDocumentObject*>(*it);
        App::DocumentObject* obj = vp->getObject();
        if (obj) {
            QString label = QString::fromUtf8(obj->Label.getValue());
            nodeNames[vp->getRoot()] = label;
        }

        std::vector<std::string> modes = vp->getDisplayMaskModes();
        for (std::vector<std::string>::iterator jt = modes.begin(); jt != modes.end(); ++jt) {
            SoNode* node = vp->getDisplayMaskMode(jt->c_str());
            if (node) {
                nodeNames[node] = QString::fromStdString(*jt);
            }
        }
    }

    SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
    model->setNodeNames(nodeNames);
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
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        setNodeNames(doc);

        View3DInventor* view = qobject_cast<View3DInventor*>(doc->getActiveView());
        if (view) {
            View3DInventorViewer* viewer = view->getViewer();
            setNode(viewer->getSceneGraph());
            ui->treeView->expandToDepth(3);
        }
    }
    else {
        SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
        model->clear();
    }
}

#include "moc_SceneInspector.cpp"
