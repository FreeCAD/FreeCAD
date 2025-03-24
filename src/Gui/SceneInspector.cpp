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
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoCoordinate3.h>
# include <QHeaderView>
# include <QTextStream>
#endif

#include <Inventor/actions/SoGetBoundingBoxAction.h>

#include "SceneInspector.h"
#include "ui_SceneInspector.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventor.h"
#include "View3DInventorViewer.h"
#include "ViewProviderDocumentObject.h"
#include "MainWindow.h"

#include <limits>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::SceneModel */

SceneModel::SceneModel(QObject* parent)
    : QStandardItemModel(parent)
{
}

enum class Column: std::int8_t {
    INVENTOR_TREE = 0,
    NAME = 1,
    MEMORY_ADDRESS = 2,
    DATA = 3,
    RENDER_CACHING = 4,
    BOUNDING_BOX_CACHING = 5,
    BOUNDING_BOX = 6,
    COUNT = 7,
};

SceneModel::~SceneModel() = default;

int SceneModel::columnCount (const QModelIndex & parent) const
{
    Q_UNUSED(parent);
    return static_cast<int>(Column::COUNT);
}

Qt::ItemFlags SceneModel::flags (const QModelIndex & index) const
{
    return QAbstractItemModel::flags(index);
}

QVariant SceneModel::headerData (int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role != Qt::DisplayRole)
            return {};

        switch ((Column)section) {
        case Column::INVENTOR_TREE:
            return tr("Inventor Tree");
        case Column::NAME:
            return tr("Name");
        case Column::MEMORY_ADDRESS:
            return tr("Address");
        case Column::DATA:
            return tr("Data");
        case Column::RENDER_CACHING:
            return tr("Render Cache");
        case Column::BOUNDING_BOX_CACHING:
            return tr("Bounds Cache");
        case Column::BOUNDING_BOX:
            return tr("Bounds");
        default:
            assert(0 && "Not handled yet");
        }
    }

    return {};
}

bool SceneModel::setHeaderData (int, Qt::Orientation, const QVariant &, int)
{
    return false;
}

void SceneModel::setNode(SoNode* node)
{
    this->clear();
    this->setHeaderData(0, Qt::Horizontal, tr("Nodes"), Qt::DisplayRole);

    this->insertColumns(0, static_cast<int>(Column::COUNT));
    this->insertRows(0,1);
    setNode(this->index(0, 0), node);
}

static std::string_view formatSoSwitchValue(int32_t value)
{
    switch (value)
    {
    case SO_SWITCH_NONE:
        return {"None"};
    case SO_SWITCH_INHERIT:
        return {"Inherit"};
    case SO_SWITCH_ALL:
        return {"All"};
    default:
        return {"Child"};
    }
}

static std::string_view formatSoSeparatorCacheEnabled(int32_t value)
{
    switch (value)
    {
    case SoSeparator::OFF:
        return {"Off"};
    case SoSeparator::ON:
        return {"On"};
    case SoSeparator::AUTO:
        return {"Auto"};
    default:
        throw Base::ValueError();
    }
}

static std::string_view formatSoDrawStyleElement(int32_t value)
{
    switch (value)
    {
    case SoDrawStyleElement::FILLED:
        return {"Filled"};
    case SoDrawStyleElement::LINES:
        return {"Lines"};
    case SoDrawStyleElement::POINTS:
        return {"Points"};
    case SoDrawStyleElement::INVISIBLE:
        return {"Invisible"};
    default:
        throw Base::ValueError();
    }
}

static std::string_view formatSoPickStyleElement(int32_t value)
{
    switch (value)
    {
    case SoPickStyleElement::SHAPE:
        return {"Shape"};
    case SoPickStyleElement::BOUNDING_BOX:
        return {"BoundingBox"};
    case SoPickStyleElement::UNPICKABLE:
        return {"Unpickable"};
    case SoPickStyleElement::SHAPE_ON_TOP:
        return {"ShapeOnTop"};
    case SoPickStyleElement::BOUNDING_BOX_ON_TOP:
        return {"BoundingBoxOnTop"};
    case SoPickStyleElement::SHAPE_FRONTFACES:
        return {"ShapeFrontFaces"};
    default:
        throw Base::ValueError();
    }
}

static inline bool isAtFloatValueLimit(float v)
{
    return v == std::numeric_limits<float>::lowest() || v == std::numeric_limits<float>::max();
}

static bool isInfinite(const SbVec3f& vec)
{
    float x, y, z; //NOLINT
    vec.getValue(x, y, z);
    return isAtFloatValueLimit(x) && isAtFloatValueLimit(y) && isAtFloatValueLimit(z);
}

static std::string formatSbXfBox3f(const SbXfBox3f& box)
{
    SbVec3f minpoint;
    SbVec3f maxpoint;
    box.getBounds(minpoint, maxpoint);

    if (isInfinite(minpoint) && isInfinite(maxpoint)) {
        return {"Infinite"};
    }

    float minx, miny, minz; //NOLINT
    minpoint.getValue(minx, miny, minz);

    float maxx, maxy, maxz; //NOLINT
    maxpoint.getValue(maxx, maxy, maxz);

    return fmt::format("Min: ({:.3},{:.3},{:.3}), Max: ({:.3},{:.3},{:.3})", minx, miny, minz, maxx, maxy, maxz);
}

void SceneModel::setNode(QModelIndex index, SoNode* node)
{
    this->setData(index.siblingAtColumn(static_cast<int>(Column::INVENTOR_TREE)),
        QVariant(QString::fromLatin1(QByteArray(node->getTypeId().getName()))));

    QHash<SoNode*, QString>::iterator it = nodeNames.find(node);
    const QString name {
        (it != nodeNames.end()) ? it.value()
                                : QString::fromLatin1(QByteArray(node->getName()))
    };
    this->setData(index.siblingAtColumn(static_cast<int>(Column::NAME)), QVariant(name));

    this->setData(index.siblingAtColumn(static_cast<int>(Column::MEMORY_ADDRESS)),
        QVariant(QString::fromStdString(fmt::format("{}", (void*)node))));

    auto view = qobject_cast<View3DInventor*>(getMainWindow()->activeWindow());
    auto vp = view->getViewer()->getSoRenderManager()->getViewportRegion();

    QString data;
    QTextStream stream(&data);
    if(static_cast<bool>(node->isOfType(SoSwitch::getClassTypeId()))) {
        auto pcSwitch = static_cast<SoSwitch*>(node);
        auto value = pcSwitch->whichChild.getValue();
        stream << fmt::format("Which: {} ({})", formatSoSwitchValue(value), value).c_str();
    } else if (static_cast<bool>(node->isOfType(SoSeparator::getClassTypeId()))) {
        auto pcSeparator = static_cast<SoSeparator*>(node);

        auto renderCaching = pcSeparator->renderCaching.getValue();
        this->setData(index.siblingAtColumn(static_cast<int>(Column::RENDER_CACHING)),
            QVariant(QString::fromStdString(std::string{formatSoSeparatorCacheEnabled(renderCaching)})));

        auto boundingBoxCaching = pcSeparator->boundingBoxCaching.getValue();
        this->setData(index.siblingAtColumn(static_cast<int>(Column::BOUNDING_BOX_CACHING)),
            QVariant(QString::fromStdString(std::string{formatSoSeparatorCacheEnabled(boundingBoxCaching)})));

        SoGetBoundingBoxAction getBBox(vp);
        getBBox.apply(pcSeparator);
        this->setData(index.siblingAtColumn(static_cast<int>(Column::BOUNDING_BOX)),
            QVariant(QString::fromStdString(formatSbXfBox3f(getBBox.getXfBoundingBox()))));
    } else if (static_cast<bool>(node->isOfType(SoDrawStyle::getClassTypeId()))) {
        auto pcDrawStyle = static_cast<SoDrawStyle*>(node);
        auto value = pcDrawStyle->style.getValue();
        stream << fmt::format("Style: {} ({})", formatSoDrawStyleElement(value), value).c_str();
    } else if (static_cast<bool>(node->isOfType(SoPickStyle::getClassTypeId()))) {
        auto pcPickStyle = static_cast<SoPickStyle*>(node);
        auto value = pcPickStyle->style.getValue();
        stream << fmt::format("Style: {} ({})", formatSoPickStyleElement(value), value).c_str();
    } else if (static_cast<bool>(node->isOfType(SoCoordinate3::getClassTypeId()))) {
        auto pcCoords = static_cast<SoCoordinate3*>(node);
        auto values = pcCoords->point.getValues(0);
        if (values) {
            float x { 0 };
            float y { 0 };
            float z { 0 };
            values->getValue(x, y, z);
            stream << fmt::format("XYZ: {}, {}, {}", x, y, z).c_str();
        }
    }

    this->setData(index.siblingAtColumn((int)Column::DATA), QVariant(data));

    if (static_cast<bool>(node->getTypeId().isDerivedFrom(SoGroup::getClassTypeId()))) {
        auto group = static_cast<SoGroup*>(node);
        this->insertColumns(0, static_cast<int>(Column::COUNT), index);
        this->insertRows(0, group->getNumChildren(), index);
        for (int i=0; i < group->getNumChildren(); i++) {
            SoNode* child = group->getChild(i);
            setNode(this->index(i, 0, index), child);
        }
    }
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
    connect(ui->refreshButton, &QPushButton::clicked,
            this, &DlgInspector::onRefreshButtonClicked);
    setWindowTitle(tr("Scene Inspector"));

    auto model = new SceneModel(this);
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

    auto view = qobject_cast<View3DInventor*>(doc->getActiveView());
    if (view) {
        View3DInventorViewer* viewer = view->getViewer();
        setNode(viewer->getSceneGraph());
        ui->treeView->expandToDepth(3);
    }
}

void DlgInspector::setNode(SoNode* node)
{
    auto model = static_cast<SceneModel*>(ui->treeView->model());
    model->setNode(node);

    QHeaderView* header = ui->treeView->header();
    header->setSectionResizeMode(static_cast<int>(Column::INVENTOR_TREE), QHeaderView::Interactive);
    header->resizeSection(static_cast<int>(Column::INVENTOR_TREE), 300);
    header->setSectionResizeMode(static_cast<int>(Column::NAME), QHeaderView::Interactive);
    header->resizeSection(static_cast<int>(Column::NAME), 200);
    header->setSectionResizeMode(static_cast<int>(Column::MEMORY_ADDRESS), QHeaderView::Interactive);
    header->resizeSection(static_cast<int>(Column::MEMORY_ADDRESS), 140);
    header->setSectionResizeMode(static_cast<int>(Column::DATA), QHeaderView::Interactive);
    header->resizeSection(static_cast<int>(Column::DATA), 200);

    header->setSectionsMovable(false);
}

void DlgInspector::setNodeNames(Gui::Document* doc)
{
    std::vector<Gui::ViewProvider*> vps = doc->getViewProvidersOfType
            (Gui::ViewProviderDocumentObject::getClassTypeId());
    QHash<SoNode*, QString> nodeNames;
    for (const auto & it : vps) {
        auto vp = static_cast<Gui::ViewProviderDocumentObject*>(it);
        App::DocumentObject* obj = vp->getObject();
        if (obj) {
            QString label = QString::fromUtf8(obj->Label.getValue());
            nodeNames[vp->getRoot()] = label;
        }

        std::vector<std::string> modes = vp->getDisplayMaskModes();
        for (const auto & mode : modes) {
            SoNode* node = vp->getDisplayMaskMode(mode.c_str());
            if (node) {
                nodeNames[node] = QString::fromStdString(mode);
            }
        }
    }

    auto model = static_cast<SceneModel*>(ui->treeView->model());
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

void DlgInspector::onRefreshButtonClicked()
{
    Gui::Document* doc = Application::Instance->activeDocument();
    if (doc) {
        setNodeNames(doc);

        auto view = qobject_cast<View3DInventor*>(doc->getActiveView());
        if (view) {
            View3DInventorViewer* viewer = view->getViewer();
            setNode(viewer->getSceneGraph());
            ui->treeView->expandToDepth(3);
        }
    }
    else {
        auto model = static_cast<SceneModel*>(ui->treeView->model());
        model->clear();
    }
}

#include "moc_SceneInspector.cpp"
