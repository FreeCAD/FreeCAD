/***************************************************************************
 *   Copyright (c) 2012 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <BRepCheck_Analyzer.hxx>
#include <BRepCheck_Result.hxx>
#include <BRepCheck_ListIteratorOfListOfStatus.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Compound.hxx>
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <gp_Trsf.hxx>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoTransform.h>
#include "../App/PartFeature.h"
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include "TaskCheckGeometry.h"

using namespace PartGui;

QVector<QString> buildShapeEnumVector()
{
   QVector<QString>names;
   names.push_back(QObject::tr("Compound"));             //TopAbs_COMPOUND
   names.push_back(QObject::tr("Compound Solid"));       //TopAbs_COMPSOLID
   names.push_back(QObject::tr("Solid"));                //TopAbs_SOLID
   names.push_back(QObject::tr("Shell"));                //TopAbs_SHELL
   names.push_back(QObject::tr("Face"));                 //TopAbs_FACE
   names.push_back(QObject::tr("Wire"));                 //TopAbs_WIRE
   names.push_back(QObject::tr("Edge"));                 //TopAbs_EDGE
   names.push_back(QObject::tr("Vertex"));               //TopAbs_VERTEX
   names.push_back(QObject::tr("Shape"));                //TopAbs_SHAPE
   return names;
}

QString shapeEnumToString(const int &index)
{
    static QVector<QString> names = buildShapeEnumVector();
    if (index < 0 || index > TopAbs_SHAPE)
        return names.at(8);
    return names.at(index);
}

QVector<QString> buildCheckStatusStringVector()
{
    QVector<QString>names;
    names.push_back(QObject::tr("No Error"));                           //    BRepCheck_NoError
    names.push_back(QObject::tr("Invalid Point On Curve"));             //    BRepCheck_InvalidPointOnCurve
    names.push_back(QObject::tr("Invalid Point On Curve On Surface"));  //    BRepCheck_InvalidPointOnCurveOnSurface
    names.push_back(QObject::tr("Invalid Point On Surface"));           //    BRepCheck_InvalidPointOnSurface
    names.push_back(QObject::tr("No 3D Curve"));                        //    BRepCheck_No3DCurve
    names.push_back(QObject::tr("Multiple 3D Curve"));                  //    BRepCheck_Multiple3DCurve
    names.push_back(QObject::tr("Invalid 3D Curve"));                   //    BRepCheck_Invalid3DCurve
    names.push_back(QObject::tr("No Curve On Surface"));                //    BRepCheck_NoCurveOnSurface
    names.push_back(QObject::tr("Invalid Curve On Surface"));           //    BRepCheck_InvalidCurveOnSurface
    names.push_back(QObject::tr("Invalid Curve On Closed Surface"));    //    BRepCheck_InvalidCurveOnClosedSurface
    names.push_back(QObject::tr("Invalid Same Range Flag"));            //    BRepCheck_InvalidSameRangeFlag
    names.push_back(QObject::tr("Invalid Same Parameter Flag"));        //    BRepCheck_InvalidSameParameterFlag
    names.push_back(QObject::tr("Invalid Degenerated Flag"));           //    BRepCheck_InvalidDegeneratedFlag
    names.push_back(QObject::tr("Free Edge"));                          //    BRepCheck_FreeEdge
    names.push_back(QObject::tr("Invalid MultiConnexity"));             //    BRepCheck_InvalidMultiConnexity
    names.push_back(QObject::tr("Invalid Range"));                      //    BRepCheck_InvalidRange
    names.push_back(QObject::tr("Empty Wire"));                         //    BRepCheck_EmptyWire
    names.push_back(QObject::tr("Redundant Edge"));                     //    BRepCheck_RedundantEdge
    names.push_back(QObject::tr("Self Intersecting Wire"));             //    BRepCheck_SelfIntersectingWire
    names.push_back(QObject::tr("No Surface"));                         //    BRepCheck_NoSurface
    names.push_back(QObject::tr("Invalid Wire"));                       //    BRepCheck_InvalidWire
    names.push_back(QObject::tr("Redundant Wire"));                     //    BRepCheck_RedundantWire
    names.push_back(QObject::tr("Intersecting Wires"));                 //    BRepCheck_IntersectingWires
    names.push_back(QObject::tr("Invalid Imbrication Of Wires"));       //    BRepCheck_InvalidImbricationOfWires
    names.push_back(QObject::tr("Empty Shell"));                        //    BRepCheck_EmptyShell
    names.push_back(QObject::tr("Redundant Face"));                     //    BRepCheck_RedundantFace
    names.push_back(QObject::tr("Unorientable Shape"));                 //    BRepCheck_UnorientableShape
    names.push_back(QObject::tr("Not Closed"));                         //    BRepCheck_NotClosed
    names.push_back(QObject::tr("Not Connected"));                      //    BRepCheck_NotConnected
    names.push_back(QObject::tr("Sub Shape Not In Shape"));             //    BRepCheck_SubshapeNotInShape
    names.push_back(QObject::tr("Bad Orientation"));                    //    BRepCheck_BadOrientation
    names.push_back(QObject::tr("Bad Orientation Of Sub Shape"));       //    BRepCheck_BadOrientationOfSubshape
    names.push_back(QObject::tr("Invalid Tolerance Value"));            //    BRepCheck_InvalidToleranceValue
    names.push_back(QObject::tr("Check Failed"));                       //    BRepCheck_CheckFail

    return names;
}

QString checkStatusToString(const int &index)
{
    static QVector<QString> names = buildCheckStatusStringVector();
    if (index == -1)
    {
        return QString(QObject::tr("No Result"));
    }
    if (index > 33 || index < 0)
    {
        QString message(QObject::tr("Out Of Enum Range: "));
        QString number;
        number.setNum(index);
        message += number;
        return message;
    }
    return names.at(index);
}

ResultEntry::ResultEntry()
{
    viewProvider = 0;
    boxSep = 0;
    boxSwitch = 0;
    parent = 0;
    children.clear();
    selectionStrings.clear();
}

ResultEntry::~ResultEntry()
{
    if (boxSep)
        viewProvider->getRoot()->removeChild(boxSep);
    qDeleteAll(children);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

ResultModel::ResultModel(QObject *parent) : QAbstractItemModel(parent)
{
    root = 0;
}

ResultModel::~ResultModel()
{
    if (root)
        delete root;
}

QModelIndex ResultModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!root)
        return QModelIndex();
    ResultEntry *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return QModelIndex();
    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex ResultModel::parent(const QModelIndex &child) const
{
    ResultEntry *childNode = nodeFromIndex(child);
    if (!childNode)
        return QModelIndex();
    ResultEntry *parentNode = childNode->parent;
    if (!parentNode)
        return QModelIndex();
    ResultEntry *grandParentNode = parentNode->parent;
    if (!grandParentNode)
        return QModelIndex();
    int row = grandParentNode->children.indexOf(parentNode);
    return createIndex(row, 0, parentNode);
}

int ResultModel::rowCount(const QModelIndex &parent) const
{
    ResultEntry *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return 0;
    return parentNode->children.size();
}

int ResultModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant ResultModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return QVariant();
    ResultEntry *node = nodeFromIndex(index);
    if (!node)
        return QVariant();
    switch (index.column())
    {
    case 0:
        return QVariant(node->name);
    case 1:
        return QVariant(node->type);
    case 2:
        return QVariant(node->error);
    }
    return QVariant();
}

QVariant ResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QVariant();
    switch (section)
    {
    case 0:
        return QVariant(QString(tr("Name")));
    case 1:
        return QVariant(QString(tr("Type")));
    case 2:
        return QVariant(QString(tr("Error")));
    }
    return QVariant();
}

void ResultModel::setResults(ResultEntry *resultsIn)
{
#if QT_VERSION >= 0x040600
    this->beginResetModel();
#endif
    if (root)
        delete root;
    root = resultsIn;
#if QT_VERSION >= 0x040600
    this->endResetModel();
#else
    this->reset();
#endif
}

ResultEntry* ResultModel::getEntry(const QModelIndex &index)
{    
    return nodeFromIndex(index);
}

ResultEntry* ResultModel::nodeFromIndex(const QModelIndex &index) const
{
    if (index.isValid())
        return static_cast<ResultEntry *>(index.internalPointer());
    else
        return root;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

TaskCheckGeometryResults::TaskCheckGeometryResults(QWidget *parent) : QWidget(parent)
{
    this->setWindowTitle(tr("Check Geometry"));
    setupInterface();
    setupFunctionMap();
    goCheck();
}

TaskCheckGeometryResults::~TaskCheckGeometryResults()
{
    Gui::Selection().clearSelection();
}

void TaskCheckGeometryResults::setupInterface()
{
    message = new QLabel(this);
    model = new ResultModel(this);
    treeView = new QTreeView(this);
    treeView->setModel(model);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(treeView->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            this, SLOT(currentRowChanged(QModelIndex,QModelIndex)));

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(message);
    layout->addWidget(treeView);
    this->setLayout(layout);    
}

void TaskCheckGeometryResults::goCheck()
{
    Gui::WaitCursor wc;
    int selectedCount(0), checkedCount(0), invalidShapes(0);
    std::vector<Gui::SelectionSingleton::SelObj> selection = Gui::Selection().getSelection();
    std::vector<Gui::SelectionSingleton::SelObj>::iterator it;
    ResultEntry *theRoot = new ResultEntry();
    for (it = selection.begin(); it != selection.end(); ++it)
    {
        selectedCount++;
        Part::Feature *feature = dynamic_cast<Part::Feature *>((*it).pObject);
        if (!feature)
            continue;
        currentProvider = Gui::Application::Instance->activeDocument()->getViewProvider(feature);
        if (!currentProvider)
            continue;
        TopoDS_Shape shape = feature->Shape.getValue();
        QString baseName;
        QTextStream baseStream(&baseName);
        baseStream << (*it).DocName;
        baseStream << "." << (*it).FeatName;
        if (strlen((*it).SubName) > 0)
        {
            shape = feature->Shape.getShape().getSubShape((*it).SubName);
            baseStream << "." << (*it).SubName;
        }

        if (shape.IsNull())
            continue;
        checkedCount++;
        checkedMap.Clear();

        BRepCheck_Analyzer shapeCheck(shape);
        if (!shapeCheck.IsValid())
        {
            invalidShapes++;
            ResultEntry *entry = new ResultEntry();
            entry->parent = theRoot;
            entry->shape = shape;
            entry->name = baseName;
            entry->type = shapeEnumToString(shape.ShapeType());
            entry->error = QObject::tr("Invalid");
            entry->viewProvider = currentProvider;
            getSetupResultBoundingBoxObject().go(entry);
            theRoot->children.push_back(entry);
            recursiveCheck(shapeCheck, shape, entry);
        }
    }
    model->setResults(theRoot);
    treeView->expandAll();
    treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    QString aMessage;
    QTextStream aStream(&aMessage);
    aStream << checkedCount << " processed out of " << selectedCount << " selected\n";
    aStream << invalidShapes << " invalid shapes.";
    message->setText(aMessage);
    Gui::Selection().clearSelection();
}

void TaskCheckGeometryResults::recursiveCheck(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                                              ResultEntry *parent)
{
    ResultEntry *branchNode = parent;
    BRepCheck_ListIteratorOfListOfStatus listIt;
    if (!shapeCheck.Result(shape).IsNull() && !checkedMap.Contains(shape))
    {
        listIt.Initialize(shapeCheck.Result(shape)->Status());
        if (listIt.Value() != BRepCheck_NoError)
        {
            ResultEntry *entry = new ResultEntry();
            entry->parent = parent;
            entry->shape = shape;
            entry->type = shapeEnumToString(shape.ShapeType());
            entry->error = checkStatusToString(listIt.Value());
            entry->viewProvider = currentProvider;
            dispatchError(entry, listIt.Value());
            parent->children.push_back(entry);
            branchNode = entry;
        }
    }
    checkedMap.Add(shape);

    if (shape.ShapeType() == TopAbs_SOLID)
        checkSub(shapeCheck, shape, TopAbs_SHELL, branchNode);
    if (shape.ShapeType() == TopAbs_EDGE)
        checkSub(shapeCheck, shape, TopAbs_VERTEX, branchNode);
    if (shape.ShapeType() == TopAbs_FACE)
    {
        checkSub(shapeCheck, shape, TopAbs_WIRE, branchNode);
        checkSub(shapeCheck, shape, TopAbs_EDGE, branchNode);
        checkSub(shapeCheck, shape, TopAbs_VERTEX, branchNode);
    }

    for (TopoDS_Iterator it(shape); it.More(); it.Next())
        recursiveCheck(shapeCheck, it.Value(), branchNode);
}

void TaskCheckGeometryResults::checkSub(const BRepCheck_Analyzer &shapeCheck, const TopoDS_Shape &shape,
                                        const TopAbs_ShapeEnum subType, ResultEntry *parent)
{
    BRepCheck_ListIteratorOfListOfStatus itl;
    TopExp_Explorer exp;
    for (exp.Init(shape,subType); exp.More(); exp.Next())
    {
        const Handle(BRepCheck_Result)& res = shapeCheck.Result(exp.Current());
        const TopoDS_Shape& sub = exp.Current();
        for (res->InitContextIterator(); res->MoreShapeInContext(); res->NextShapeInContext())
        {
            if (res->ContextualShape().IsSame(shape))
            {
                for (itl.Initialize(res->StatusOnShape()); itl.More(); itl.Next())
                {
                     if (itl.Value() == BRepCheck_NoError)
                         break;
                     checkedMap.Add(sub);
                     ResultEntry *entry = new ResultEntry();
                     entry->parent = parent;
                     entry->shape = sub;
                     entry->type = shapeEnumToString(sub.ShapeType());
                     entry->error = checkStatusToString(itl.Value());
                     entry->viewProvider = currentProvider;
                     dispatchError(entry, itl.Value());
                     parent->children.push_back(entry);
                }
            }
        }
    }
}

void TaskCheckGeometryResults::dispatchError(ResultEntry *entry, const BRepCheck_Status &stat)
{
    std::vector<FunctionMapType>::iterator mapIt;
    for (mapIt = functionMap.begin(); mapIt != functionMap.end(); ++mapIt)
    {
        if ((*mapIt).get<0>() == entry->shape.ShapeType() && (*mapIt).get<1>() == stat)
        {
            ((*mapIt).get<2>())->go(entry);
            return;
        }
    }
    getSetupResultBoundingBoxObject().go(entry);
}

void TaskCheckGeometryResults::setupFunctionMap()
{
    functionMap.push_back(FunctionMapType(TopAbs_SHELL, BRepCheck_NotClosed, &getSetupResultShellNotClosedObject()));
    functionMap.push_back(FunctionMapType(TopAbs_WIRE, BRepCheck_NotClosed, &getSetupResultWireNotClosedObject()));
    functionMap.push_back(FunctionMapType(TopAbs_VERTEX, BRepCheck_InvalidPointOnCurve, &getSetupResultInvalidPointCurveObject()));
    functionMap.push_back(FunctionMapType(TopAbs_FACE, BRepCheck_IntersectingWires, &getSetupResultIntersectingWiresObject()));
    functionMap.push_back(FunctionMapType(TopAbs_EDGE, BRepCheck_InvalidCurveOnSurface, &getSetupResultInvalidCurveSurfaceObject()));
    functionMap.push_back(FunctionMapType(TopAbs_EDGE, BRepCheck_InvalidSameParameterFlag, &getSetupResultInvalidSameParameterFlagObject()));
    functionMap.push_back(FunctionMapType(TopAbs_FACE, BRepCheck_UnorientableShape, &getSetupResultUnorientableShapeFaceObject()));
}

void TaskCheckGeometryResults::currentRowChanged (const QModelIndex &current, const QModelIndex &previous)
{
    Gui::Selection().clearSelection();
    if (previous.isValid())
    {
        ResultEntry *entry = model->getEntry(previous);
        if (entry)
        {
            if (entry->boxSwitch)
                entry->boxSwitch->whichChild.setValue(SO_SWITCH_NONE);
        }
    }
    if (current.isValid())
    {
        ResultEntry *entry = model->getEntry(current);
        if (entry)
        {
            if (entry->boxSwitch)
                entry->boxSwitch->whichChild.setValue(0);
            QStringList::Iterator stringIt;
            for (stringIt = entry->selectionStrings.begin(); stringIt != entry->selectionStrings.end(); ++stringIt)
            {
                //need unique delimiter.
                QString doc, object, sub;
                if (!this->split((*stringIt), doc, object, sub))
                    continue;
                Gui::Selection().addSelection(doc.toAscii(), object.toAscii(), sub.toAscii());
            }
        }
    }
}

bool TaskCheckGeometryResults::split(QString &input, QString &doc, QString &object, QString &sub)
{
    QStringList strings = input.split(QString::fromAscii("."));
    if (strings.size() != 3)
        return false;
    doc = strings.at(0);
    object = strings.at(1);
    sub = strings.at(2);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

QString SetupResultBase::selectionName(ResultEntry *entry, const TopoDS_Shape &shape)
{
    ResultEntry *parentEntry = entry;
    while(parentEntry->name.isEmpty())
        parentEntry = parentEntry->parent;

    QString stringOut;
    QTextStream stream(&stringOut);
    stream << parentEntry->name;
    stream << '.';
    TopTools_IndexedMapOfShape shapeMap;
    int index(-1);

    switch (shape.ShapeType())
    {
    case TopAbs_FACE:
        TopExp::MapShapes(parentEntry->shape, TopAbs_FACE, shapeMap);
        stream << "Face";
        break;
    case TopAbs_EDGE:
        TopExp::MapShapes(parentEntry->shape, TopAbs_EDGE, shapeMap);
        stream << "Edge";
        break;
    case TopAbs_VERTEX:
        TopExp::MapShapes(parentEntry->shape, TopAbs_VERTEX, shapeMap);
        stream << "Vertex";
        break;
    default:
        stream << "Unexpected shape type";
        break;
    }

    index = shapeMap.FindIndex(shape);
    stream << index;
    return stringOut;
}

void SetupResultBase::addTypedSelection(ResultEntry *entry, const TopoDS_Shape &shape, TopAbs_ShapeEnum type)
{
    TopExp_Explorer it;
    for (it.Init(shape, type); it.More(); it.Next())
    {
        QString name = selectionName(entry, (it.Current()));
        if (!name.isEmpty())
            entry->selectionStrings.append(name);
    }
}

void SetupResultBoundingBox::go(ResultEntry *entry)
{
    entry->boxSep = new SoSeparator();
    entry->viewProvider->getRoot()->addChild(entry->boxSep);
    entry->boxSwitch = new SoSwitch();
    entry->boxSep->addChild(entry->boxSwitch);
    SoGroup *group = new SoGroup();
    entry->boxSwitch->addChild(group);
    entry->boxSwitch->whichChild.setValue(SO_SWITCH_NONE);
    SoDrawStyle *dStyle = new SoDrawStyle();
    dStyle->style.setValue(SoDrawStyle::LINES);
    dStyle->linePattern.setValue(0xc0c0);
    group->addChild(dStyle);
    SoMaterial *material = new SoMaterial();
    material->diffuseColor.setValue(255.0, 255.0, 255.0);
    material->ambientColor.setValue(255.0, 255.0, 255.0);
    group->addChild(material);

    Bnd_Box boundingBox;
    BRepBndLib::Add(entry->shape, boundingBox);
    Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
    boundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    double xCenter, yCenter, zCenter;
    xCenter = (xmax - xmin)/2 + xmin;
    yCenter = (ymax - ymin)/2 + ymin;
    zCenter = (zmax - zmin)/2 + zmin;

    SbVec3f boundCenter(xCenter, yCenter, zCenter);
    Standard_Real x, y, z;
    entry->shape.Location().Transformation().TranslationPart().Coord(x, y, z);
    boundCenter -= SbVec3f(x, y, z);

    SoTransform *position = new SoTransform();
    position->translation.setValue(boundCenter);
    group->addChild(position);

    SoCube *cube = new SoCube();
    cube->width.setValue(xmax - xmin);
    cube->height.setValue(ymax - ymin);
    cube->depth.setValue(zmax - zmin);
    group->addChild(cube);
}

SetupResultBoundingBox& PartGui::getSetupResultBoundingBoxObject()
{
    static SetupResultBoundingBox object;
    return object;
}

void SetupResultShellNotClosed::go(ResultEntry *entry)
{
    ShapeAnalysis_FreeBounds shellCheck(entry->shape);
    TopoDS_Compound closedWires = shellCheck.GetClosedWires();
    TopoDS_Compound openWires = shellCheck.GetOpenWires();

    addTypedSelection(entry, closedWires, TopAbs_EDGE);
    addTypedSelection(entry, openWires, TopAbs_EDGE);
}

SetupResultShellNotClosed& PartGui::getSetupResultShellNotClosedObject()
{
    static SetupResultShellNotClosed object;
    return object;
}

void SetupResultWireNotClosed::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_EDGE);
}

SetupResultWireNotClosed& PartGui::getSetupResultWireNotClosedObject()
{
    static SetupResultWireNotClosed object;
    return object;
}

void SetupResultInvalidPointCurve::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_VERTEX);
}

SetupResultInvalidPointCurve& PartGui::getSetupResultInvalidPointCurveObject()
{
    static SetupResultInvalidPointCurve object;
    return object;
}

void SetupResultIntersectingWires::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_FACE);
    getSetupResultBoundingBoxObject().go(entry);
}

SetupResultIntersectingWires& PartGui::getSetupResultIntersectingWiresObject()
{
    static SetupResultIntersectingWires object;
    return object;
}

void SetupResultInvalidCurveSurface::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_EDGE);
}

SetupResultInvalidCurveSurface& PartGui::getSetupResultInvalidCurveSurfaceObject()
{
    static SetupResultInvalidCurveSurface object;
    return object;
}

void SetupResultInvalidSameParameterFlag::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_EDGE);
}

SetupResultInvalidSameParameterFlag& PartGui::getSetupResultInvalidSameParameterFlagObject()
{
    static SetupResultInvalidSameParameterFlag object;
    return object;
}

void SetupResultUnorientableShapeFace::go(ResultEntry *entry)
{
    addTypedSelection(entry, entry->shape, TopAbs_FACE);
    getSetupResultBoundingBoxObject().go(entry);
}

SetupResultUnorientableShapeFace& PartGui::getSetupResultUnorientableShapeFaceObject()
{
    static SetupResultUnorientableShapeFace object;
    return object;
}

////////////////////////////////////////////////////////////////////////////////////////////////

TaskCheckGeometryDialog::TaskCheckGeometryDialog()
{
    this->setButtonPosition(TaskDialog::South);
    widget = new TaskCheckGeometryResults();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskCheckGeometryDialog::~TaskCheckGeometryDialog()
{

}

#include "moc_TaskCheckGeometry.cpp"
