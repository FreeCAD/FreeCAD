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
# include <Inventor/nodes/SoIndexedShape.h>
# include <QHeaderView>
# include <QTextStream>
# include <QContextMenuEvent>
# include <QMenu>
#endif

#include <boost/algorithm/string.hpp>

#include <Inventor/misc/SoChildList.h>
#include <Inventor/fields/SoFields.h>
#include <Base/Tools.h>
#include <Base/Console.h>
#include <App/Document.h>
#include "SceneInspector.h"
#include "ui_SceneInspector.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObject.h"
#include "SoFCUnifiedSelection.h"
#include "Document.h"
#include "Application.h"
#include "Macro.h"
#include "ViewProviderLink.h"

FC_LOG_LEVEL_INIT("Gui", true, true);

Q_DECLARE_METATYPE(Gui::CoinPtr<SoNode>)

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::SceneModel */

SceneModel::SceneModel(QObject* parent)
    : QAbstractItemModel(parent)
{
}

SceneModel::~SceneModel()
{
}

void SceneModel::setNodeNames(const QHash<SoNode*, QString>& names)
{
    nodeNames = names;
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
    this->beginResetModel();
    items.clear();
    rootItem.node = node;
    this->endResetModel();
}

QVariant SceneModel::data(const QModelIndex & index, int role) const
{
    if (role != Qt::DisplayRole || index.column() > 1)
        return QVariant();

    auto it = items.find(index);
    if (it == items.end())
        return QVariant();

    auto &item = it.value();

    if (index.column() == 0)
        return QString::fromLatin1(item.node->getTypeId().getName());

    SoNode *node = item.node.get();
    auto itName = nodeNames.find(node);
    QString name;
    QTextStream stream(&name);
    stream << item.node.get() << ", ";

    auto obj = ViewProviderLink::linkedObjectByNode(node);
    if (obj) {
        stream << QLatin1String(" -> ");
        if (obj->getDocument() != App::GetApplication().getActiveDocument())
            stream << QString::fromLatin1(obj->getFullName().c_str());
        else
            stream << QString::fromLatin1(obj->getNameInDocument());
        if (obj->Label.getStrValue() != obj->getNameInDocument())
            stream << QString::fromLatin1(" (%1)").arg(QString::fromUtf8(obj->Label.getValue()));
    }
    else if (node->getName() != SbName::empty())
        stream << node->getName();
    else if (itName != nodeNames.end())
        stream << itName.value();
    return name;
}

QModelIndex SceneModel::parent(const QModelIndex & index) const
{
    auto it = items.find(index);
    if (it == items.end())
        return QModelIndex();
    return it.value().parent;
}

QModelIndex SceneModel::index(int row, int column, const QModelIndex &parent) const
{
    const Item *item = nullptr;
    if (parent.isValid()) {
        auto it = items.find(parent);
        if (it == items.end())
            return QModelIndex();
        item = &it.value();
    } else
        item = &rootItem;

    SoNode *node = item->node;
    if (!item->expand)
        return QModelIndex();

    QModelIndex index = createIndex(row, column, (void*)item);

    if (node->isOfType(SoFCPathAnnotation::getClassTypeId())) {
        auto path = static_cast<SoFCPathAnnotation*>(node)->getPath();
        if (path && row >= 0 && row < path->getLength()) {
            auto &child = items[index];
            if (!child.node) {
                child.node = path->getNode(row);
                child.expand = false;
                child.parent = parent;
            }
            return index;
        }
    }

    if (auto children = node->getChildren()) {
        if (row < 0 || row >= children->getLength())
            return QModelIndex();

        auto &child = items[index];
        if (!child.node) {
            child.node = (*children)[row];
            child.parent = parent;
        }
        return index;
    }
    return QModelIndex();
}

int SceneModel::rowCount(const QModelIndex & parent) const
{
    const Item *item = nullptr;
    if (parent.isValid()) {
        auto it = items.find(parent);
        if (it == items.end())
            return 0;
        item = &it.value();
    } else
        item = &rootItem;
    SoNode *node = item->node;
    if (node->isOfType(SoFCPathAnnotation::getClassTypeId())) {
        auto path = static_cast<SoFCPathAnnotation*>(node)->getPath();
        if (path && path->getLength())
            return path->getLength();
    }
    if (auto children = node->getChildren()) {
        int count = children->getLength();
        if (count==1 && !item->expand)
            return 0;
        return count;
    }
    return 0;
}

int SceneModel::columnCount(const QModelIndex &) const
{
    return 2;
}

// --------------------------------------------------------

/* TRANSLATOR Gui::Dialog::DlgInspector */

DlgInspector::DlgInspector(QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), ui(new Ui_SceneInspector())
{
    ui->setupUi(this);
    ui->fieldView->header()->setStretchLastSection(false);
    ui->fieldView->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->fieldView->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    ui->fieldView->setAlternatingRowColors(true);
    ui->fieldView->setRootIsDecorated(true);
    ui->fieldView->setExpandsOnDoubleClick(true);

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

void DlgInspector::contextMenuEvent(QContextMenuEvent *ev)
{
    auto index = ui->treeView->indexAt(
            ui->treeView->viewport()->mapFromGlobal(ev->globalPos()));
    SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
    auto it = model->items.find(index);
    if (it == model->items.end())
        return;

    QMenu menu;
    auto action = menu.addAction(tr("Get node in console"));
    if (menu.exec(QCursor::pos()) == action) {
        Base::PyGILStateLocker lock;
        PyObject *pyobj = nullptr;
        std::string type;
        try {
            type = "So";
            type += it.value().node->getTypeId().getName().getString();
            type += " *";
            pyobj = Base::Interpreter().createSWIGPointerObj(
                    "pivy.coin",type.c_str(), it.value().node, 1);
            Base::Interpreter().addVariable("__coin_node", Py::Object(pyobj));
        } catch (Base::Exception &) {
            try {
                FC_WARN("Failed to create Python wrapper for type " << type);
                type = "SoNode *";
                pyobj = Base::Interpreter().createSWIGPointerObj(
                        "pivy.coin", "SoNode *", it.value().node, 1);
                Base::Interpreter().addVariable("__coin_node", Py::Object(pyobj));
            } catch (Base::Exception &e) {
                e.ReportException();
            }
        }
        if (pyobj) {
            std::ostringstream ss;
            ss << "# __coin_node = " << Py::Object(pyobj).as_string();
            Application::Instance->macroManager()->addLine(
                    MacroManager::Cmt, ss.str().c_str());
        }
    }
}

void DlgInspector::setDocument(Gui::Document* doc)
{
    setNodeNames(doc);

    View3DInventor* view = qobject_cast<View3DInventor*>(doc->getActiveView());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        setNode(viewer->getSoRenderManager()->getSceneGraph());
        initExpand();
    }
}

void DlgInspector::setNode(SoNode* node)
{
    SceneModel* model = static_cast<SceneModel*>(ui->treeView->model());
    model->setNode(node);

    QHeaderView* header = ui->treeView->header();
    header->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    header->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    header->setSectionsMovable(false);
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
                nodeNames[node] = QString::fromStdString(std::string("DisplayMode(")+*jt+")");
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
    ui->fieldView->clear();
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        setNodeNames(doc);

        View3DInventor* view = qobject_cast<View3DInventor*>(doc->getActiveView());
        if (view) {
            View3DInventorViewer* viewer = view->getViewer();
            setNode(viewer->getSoRenderManager()->getSceneGraph());
            initExpand();
        }
    }
}

void DlgInspector::initExpand()
{
    auto model = static_cast<SceneModel*>(ui->treeView->model());
    for (int i=0, c=model->rowCount(); i<c; ++i) {
        auto index = model->index(i,0);
        auto it = model->items.find(index);
        if (it != model->items.end()
            && it.value().node->isOfType(SoFCUnifiedSelection::getClassTypeId())) {
            ui->treeView->setExpanded(index, true);
            break;
        }
    }
}

void DlgInspector::populateFieldView(QTreeWidgetItem *parent, SoNode *n)
{
    if (!n)
        return;
    CoinPtr<SoNode> node(n);
    SoFieldList fields;
    int count = node->getFields(fields);
    SbString val;
    std::string sval;
    for (int i=0; i<count; ++i) {
        auto item = parent ? new QTreeWidgetItem(parent) : new QTreeWidgetItem(ui->fieldView);
        auto field = fields[i];
        SbName name;
        node->getFieldName(field, name);
        QString sname = QString::fromLatin1(name);
        if (field->isIgnored())
            sname += QLatin1String("*");
        item->setText(0, sname);
        item->setToolTip(0, QString::fromLatin1(field->getTypeId().getName()));
        item->setData(0, Qt::UserRole, QVariant::fromValue(node));
        item->setData(1, Qt::UserRole, i);

        if (field->isOfType(SoSFNode::getClassTypeId())) {
            auto nfield = static_cast<SoSFNode*>(field);
            if (!nfield->getValue())
                item->setText(1, QString::fromLatin1("NULL"));
            else {
                QString txt;
                QTextStream stream(&txt);
                stream << nfield->getValue()->getTypeId().getName() << " " <<nfield->getValue();
                item->setText(1, txt);
                item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            }
        }
        else if (field->isOfType(SoMFNode::getClassTypeId())) {
            auto mfield = static_cast<SoMField*>(field);
            item->setText(1, QString::fromLatin1("count: %1").arg(mfield->getNum()));
            item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
        else if (field->isOfType(SoMField::getClassTypeId())) {
            auto mfield = static_cast<SoMField*>(field);
            item->setToolTip(1, QString::fromLatin1("count: %1").arg(mfield->getNum()));
            QString txt;
            QTextStream stream(&txt);
            int i=0;
            bool first = true;
            for (int c=std::min(5, mfield->getNum()); i<c; ++i) {
                mfield->get1(i, val);
                if (first)
                    first = false;
                else
                    stream << " | ";
                sval = val.getString();
                val.makeEmpty(false);
                boost::replace_all(sval, "\n", "; ");
                if (sval.size() > 256)
                    sval.resize(256);
                stream << sval.c_str();
                if (txt.size() >= 256)
                    break;
            }
            if (i < mfield->getNum())
                stream << "...";
            item->setText(1, txt);
            if (i < mfield->getNum() || mfield->getNum() > 1)
                item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        } else {
            field->get(val);
            sval = val.getString();
            val.makeEmpty(false);
            boost::replace_all(sval, "\n", "; ");
            if (sval.size() > 256) {
                sval.resize(256);
                sval += "...";
            }
            item->setText(1, QString::fromLatin1(sval.c_str()));
        }
    }

    if (parent && node && node->getChildren() && node->getChildren()->getLength()) {
        auto children = node->getChildren();
        auto item = new QTreeWidgetItem(parent);
        item->setText(0, QString::fromLatin1("Children Nodes"));
        item->setText(1, QString::fromLatin1("count: %1").arg(children->getLength()));
        for (int i=0, c=children->getLength(); i<c; ++i) {
            auto child = new QTreeWidgetItem(item);
            child->setText(0, QString::fromLatin1("%1").arg(i+1));
            CoinPtr<SoNode> childNode((*children)[i]);
            QString txt;
            QTextStream stream(&txt);
            stream << childNode.get() << " " << childNode->getTypeId().getName();
            child->setText(1, txt);
            child->setData(0, Qt::UserRole, QVariant::fromValue(childNode));
            child->setData(1, Qt::UserRole, -1);
            child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
        }
    }
}

void DlgInspector::on_treeView_pressed(const QModelIndex &index)
{
    ui->fieldView->clear();
    auto model = static_cast<SceneModel*>(ui->treeView->model());
    auto it = model->items.find(index);
    if (it == model->items.end())
        return;

    populateFieldView(nullptr, it.value().node);
}

void DlgInspector::on_fieldView_itemExpanded(QTreeWidgetItem *item)
{
    expandItem(item, false);
}

void DlgInspector::expandItem(QTreeWidgetItem *item, bool force)
{
    if (item->childCount() && !force)
        return;

    auto node = qvariant_cast<CoinPtr<SoNode> >(item->data(0, Qt::UserRole));
    int col = item->data(1, Qt::UserRole).toInt();
    if (col < 0) {
        if (col == -1)
            populateFieldView(item, node);
        else if (col == -2)
            expandItem(item->parent(), true);
        return;
    }
    SoFieldList fields;
    if (col >= node->getFields(fields))
        return;
    auto field = fields[col];
    if (field->isOfType(SoSFNode::getClassTypeId())) {
        auto &sfield = *static_cast<SoSFNode*>(field);
        populateFieldView(item, sfield.getValue());
        return;
    }
    if (field->isOfType(SoMFNode::getClassTypeId())) {
        auto &mfield = *static_cast<SoMFNode*>(field);
        for (int i=0, c=mfield.getNum(); i<c; ++i) {
            auto child = new QTreeWidgetItem(item);
            child->setText(0, QString::fromLatin1("%1").arg(i+1));
            if (!mfield[i])
                child->setText(1, QString::fromLatin1("NULL"));
            else {
                CoinPtr<SoNode> childNode(mfield[i]);
                QString txt;
                QTextStream stream(&txt);
                stream << mfield[i] << " " << mfield[i]->getTypeId().getName();
                child->setText(1, txt);
                child->setData(0, Qt::UserRole, QVariant::fromValue(childNode));
                child->setData(1, Qt::UserRole, -1);
                child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
            }
        }
    }

    if (!field->isOfType(SoMField::getClassTypeId()))
        return;

    auto mfield = static_cast<SoMField*>(field);
    QString txt;
    QTextStream stream(&txt);
    int i = item->childCount();
    if (i > 0) {
        auto lastChild = item->child(i-1);
        if (lastChild->text(0) == QLatin1String("...")) {
            delete lastChild;
            --i;
        }
    }
    if (i >= mfield->getNum())
        return;

    SbString val;
    std::string sval;
    int total = 0;
    int prevcount = 0;
    auto addChild = [&]() {
        boost::replace_all(sval, "\n", "; ");
        auto child = new QTreeWidgetItem(item);
        if (prevcount)
            child->setText(0, QString::fromLatin1("%1 ~ %2").arg(i-prevcount-1).arg(i-1));
        else
            child->setText(0, QString::fromLatin1("%1").arg(i-1));
        child->setText(1, QString::fromLatin1(sval.c_str()));
    };

    mfield->get1(i++, val);
    sval = val.getString();
    for (int c=mfield->getNum(); i<c; ++i) {
        val.makeEmpty(false);
        mfield->get1(i, val);
        if (sval == val.getString()) {
            ++prevcount;
            continue;
        }
        addChild();
        sval = val.getString();
        prevcount = 0;
        if (++total > 100) {
            ++i;
            break;
        }
    }
    addChild();
    if (i < mfield->getNum()) {
        auto child = new QTreeWidgetItem(item);
        child->setText(0, QString::fromLatin1("..."));
        child->setData(1, Qt::UserRole, -2);
        child->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
}

#include "moc_SceneInspector.cpp"
