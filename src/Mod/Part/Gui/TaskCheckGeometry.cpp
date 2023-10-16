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
#ifndef _PreComp_
# include <QCheckBox>
# include <QCoreApplication>
# include <QHeaderView>
# include <QPushButton>
# include <QScrollBar>
# include <QTextEdit>
# include <QTextStream>
# include <QThread>
# include <QTreeView>
# include <Standard_Version.hxx>
# include <Bnd_Box.hxx>
# include <BOPAlgo_ArgumentAnalyzer.hxx>
# include <BOPAlgo_ListOfCheckResult.hxx>
# include <BRepBndLib.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepTools_ShapeSet.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoResetTransform.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoTransform.h>
#endif //_PreComp_

#include <Base/Interpreter.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Selection.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Mod/Part/App/PartFeature.h>

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
        QString message(QObject::tr("Out Of Enum Range:") + QStringLiteral(" "));
        QString number;
        number.setNum(index);
        message += number;
        return message;
    }
    return names.at(index);
}

QVector<QString> buildBOPCheckResultVector()
{
  QVector<QString> results;
  results.push_back(QObject::tr("Boolean operation: Unknown check"));               //BOPAlgo_CheckUnknown
  results.push_back(QObject::tr("Boolean operation: Bad type"));                    //BOPAlgo_BadType
  results.push_back(QObject::tr("Boolean operation: Self-intersection found"));     //BOPAlgo_SelfIntersect
  results.push_back(QObject::tr("Boolean operation: Edge too small"));              //BOPAlgo_TooSmallEdge
  results.push_back(QObject::tr("Boolean operation: Non-recoverable face"));        //BOPAlgo_NonRecoverableFace
  results.push_back(QObject::tr("Boolean operation: Incompatibility of vertex"));   //BOPAlgo_IncompatibilityOfVertex
  results.push_back(QObject::tr("Boolean operation: Incompatibility of edge"));     //BOPAlgo_IncompatibilityOfEdge
  results.push_back(QObject::tr("Boolean operation: Incompatibility of face"));     //BOPAlgo_IncompatibilityOfFace
  results.push_back(QObject::tr("Boolean operation: Aborted"));                     //BOPAlgo_OperationAborted
  results.push_back(QObject::tr("Boolean operation: GeomAbs_C0"));                  //BOPAlgo_GeomAbs_C0
  results.push_back(QObject::tr("Boolean operation: Invalid curve on surface"));    //BOPAlgo_InvalidCurveOnSurface
  results.push_back(QObject::tr("Boolean operation: Not valid"));                   //BOPAlgo_NotValid

  return results;
}

QString getBOPCheckString(const BOPAlgo_CheckStatus &status)
{
  static QVector<QString> strings = buildBOPCheckResultVector();
  int index = static_cast<int>(status);
  if (index < 0 || index > strings.size())
    index = 0;
  return strings.at(index);
}

ResultEntry::ResultEntry()
{
    viewProviderRoot = nullptr;
    boxSep = nullptr;
    boxSwitch = nullptr;
    parent = nullptr;
    children.clear();
    selectionStrings.clear();
}

ResultEntry::~ResultEntry()
{
    if (boxSep && viewProviderRoot)
        viewProviderRoot->removeChild(boxSep);
    if (viewProviderRoot)
        viewProviderRoot->unref();
    qDeleteAll(children);
}

void ResultEntry::buildEntryName()
{
    ResultEntry* parentEntry = this;
    while (parentEntry->parent) {
        ResultEntry* temp = parentEntry->parent;
        if (!temp->parent)
            break;
        parentEntry = parentEntry->parent;
    }

  QString stringOut;
  QTextStream stream(&stringOut);
  TopTools_IndexedMapOfShape shapeMap;
  int index(-1);

  switch (this->shape.ShapeType())
  {
  case TopAbs_COMPOUND:
      TopExp::MapShapes(parentEntry->shape, TopAbs_COMPOUND, shapeMap);
      stream << "Compound";
      break;
  case TopAbs_COMPSOLID:
      TopExp::MapShapes(parentEntry->shape, TopAbs_COMPSOLID, shapeMap);
      stream << "CompSolid";
      break;
  case TopAbs_SOLID:
      TopExp::MapShapes(parentEntry->shape, TopAbs_SOLID, shapeMap);
      stream << "Solid";
      break;
  case TopAbs_SHELL:
      TopExp::MapShapes(parentEntry->shape, TopAbs_SHELL, shapeMap);
      stream << "Shell";
      break;
  case TopAbs_WIRE:
      TopExp::MapShapes(parentEntry->shape, TopAbs_WIRE, shapeMap);
      stream << "Wire";
      break;
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

  index = shapeMap.FindIndex(this->shape);
  stream << index;
  this->name = stringOut;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

ResultModel::ResultModel(QObject *parent) : QAbstractItemModel(parent)
{
    root = nullptr;
}

ResultModel::~ResultModel()
{
    if (root)
        delete root;
}

QModelIndex ResultModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!root)
        return {};
    ResultEntry *parentNode = nodeFromIndex(parent);
    if (!parentNode)
        return {};
    return createIndex(row, column, parentNode->children.at(row));
}

QModelIndex ResultModel::parent(const QModelIndex &child) const
{
    ResultEntry *childNode = nodeFromIndex(child);
    if (!childNode)
        return {};
    ResultEntry *parentNode = childNode->parent;
    if (!parentNode)
        return {};
    ResultEntry *grandParentNode = parentNode->parent;
    if (!grandParentNode)
        return {};
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
    Q_UNUSED(parent);
    return 3;
}

QVariant ResultModel::data(const QModelIndex &index, int role) const
{
    if (role != Qt::DisplayRole)
        return {};
    ResultEntry *node = nodeFromIndex(index);
    if (!node)
        return {};
    switch (index.column())
    {
    case 0:
        return QVariant(node->name);
    case 1:
        return QVariant(node->type);
    case 2:
        return QVariant(node->error);
    }
    return {};
}

QVariant ResultModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return {};
    switch (section)
    {
    case 0:
        return QVariant(QString(tr("Name")));
    case 1:
        return QVariant(QString(tr("Type")));
    case 2:
        return QVariant(QString(tr("Error")));
    }
    return {};
}

void ResultModel::setResults(ResultEntry *resultsIn)
{
    this->beginResetModel();
    if (root)
        delete root;
    root = resultsIn;
    this->endResetModel();
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
    this->setWindowTitle(tr("Check Geometry Results"));
    setupInterface();
    setupFunctionMap();
}

TaskCheckGeometryResults::~TaskCheckGeometryResults()
{
    try {
        Gui::Selection().clearSelection();
    }
    catch (const Py::Exception&) {
        Base::PyException e; // extract the Python error text
        e.ReportException();
    }
}

void TaskCheckGeometryResults::setupInterface()
{
    message = new QLabel(this);
    message->setText(tr("Check is running..."));
    model = new ResultModel(this);
    treeView = new QTreeView(this);
    treeView->setModel(model);
    treeView->setSelectionMode(QAbstractItemView::SingleSelection);
    treeView->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(treeView->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &TaskCheckGeometryResults::currentRowChanged);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(message);
    layout->addWidget(treeView);
    this->setLayout(layout);
}

void TaskCheckGeometryResults::goCheck()
{
    Gui::WaitCursor wc;
    auto selection = Gui::Selection().getSelection();

    int selectedCount(0), checkedCount(0), invalidShapes(0);
    ResultEntry *theRoot = new ResultEntry();

    std::string scopeName {tr("Boolean operation check...").toStdString()};
#if OCC_VERSION_HEX < 0x070500
    Handle(Message_ProgressIndicator) theProgress = new BOPProgressIndicator(tr("Check geometry"),
                                                                             Gui::getMainWindow());
    theProgress->NewScope(scopeName.c_str());
    theProgress->Show();
#else
    Handle(Message_ProgressIndicator) theProgress = new BOPProgressIndicator(tr("Check geometry"),
                                                                             Gui::getMainWindow());
    Message_ProgressRange theRange(theProgress->Start());
    Message_ProgressScope theScope(theRange,
                                   TCollection_AsciiString(scopeName.c_str()),
                                   selection.size());
    theScope.Show();
#endif // 0x070500

    for(const auto &sel :  selection) {
        selectedCount++;
        TopoDS_Shape shape = Part::Feature::getShape(sel.pObject,sel.SubName,true);
        if (shape.IsNull())
            continue;
        currentSeparator = Gui::Application::Instance->getViewProvider(sel.pObject)->getRoot();
        if (!currentSeparator)
            continue;
        QString baseName;
        QTextStream baseStream(&baseName);
        baseStream << sel.DocName;
        baseStream << "." << sel.FeatName;
        checkedCount++;
        checkedMap.Clear();

        buildShapeContent(sel.pObject, baseName, shape);

        BRepCheck_Analyzer shapeCheck(shape);
        if (!shapeCheck.IsValid())
        {
            invalidShapes++;
            ResultEntry *entry = new ResultEntry();
            entry->parent = theRoot;
            entry->shape = shape;
            entry->name = baseName;
            entry->type = shapeEnumToString(shape.ShapeType());
            entry->error = tr("Invalid");
            entry->viewProviderRoot = currentSeparator;
            entry->viewProviderRoot->ref();
            goSetupResultBoundingBox(entry);
            theRoot->children.push_back(entry);
            recursiveCheck(shapeCheck, shape, entry);
            continue; //don't run BOPAlgo_ArgumentAnalyzer if BRepCheck_Analyzer finds something.
        }
        else
        {
          //BOPAlgo_ArgumentAnalyzer can be really slow!
          //so only run it when the shape seems valid to BRepCheck_Analyzer And
          //when the option is set.

          ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
          bool runSignal = group->GetBool("RunBOPCheck", false);
          group->SetBool("RunBOPCheck", runSignal);
          if (runSignal) {
            std::string label = tr("Checking").toStdString() + " ";
            label += sel.pObject->Label.getStrValue();
            label += "...";
#if OCC_VERSION_HEX < 0x070500
            theProgress->NewScope(label.c_str());
            invalidShapes += goBOPSingleCheck(shape, theRoot, baseName, theProgress);
            theProgress->EndScope();
            if (theProgress->UserBreak())
              break;
#else
            Message_ProgressScope theInnerScope(theScope.Next(), TCollection_AsciiString(label.c_str()), 1);
            theInnerScope.Show();
            invalidShapes += goBOPSingleCheck(shape, theRoot, baseName, theInnerScope);
            theInnerScope.Close();
            if (theScope.UserBreak())
              break;
#endif
          }
        }
    }
    model->setResults(theRoot);
    treeView->expandAll();
    treeView->header()->resizeSections(QHeaderView::ResizeToContents);
    QString aMessage {tr("%1 processed out of %2 selected").arg(checkedCount).arg(selectedCount)};
    aMessage += QLatin1String("\n ") + tr("%n invalid shapes.", "", invalidShapes);
    message->setText(aMessage);
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
            entry->buildEntryName();
            entry->type = shapeEnumToString(shape.ShapeType());
            entry->error = checkStatusToString(listIt.Value());
            entry->viewProviderRoot = currentSeparator;
            entry->viewProviderRoot->ref();
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
                     entry->buildEntryName();
                     entry->type = shapeEnumToString(sub.ShapeType());
                     entry->error = checkStatusToString(itl.Value());
                     entry->viewProviderRoot = currentSeparator;
                     entry->viewProviderRoot->ref();
                     dispatchError(entry, itl.Value());
                     parent->children.push_back(entry);
                }
            }
        }
    }
}

void TaskCheckGeometryResults::buildShapeContent(App::DocumentObject *pObject, const QString &baseName, const TopoDS_Shape &shape)
{

    bool advancedShapeContent = App::GetApplication().GetUserParameter().GetGroup("BaseApp")->GetGroup("Preferences")->
            GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry")->GetBool("AdvancedShapeContent", true);
    int decimals = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Units")->GetInt("Decimals", 2);
    std::ostringstream stream;
    if (!shapeContentString.empty())
        stream << std::endl << std::endl;
    stream << tr("Checked object").toStdString() << ": ";
    Base::PyGILStateLocker lock;
    try {
        PyObject* module = PyImport_ImportModule("BasicShapes.ShapeContent");
        if (!module) {
            throw Py::Exception();
        }
        Py::Tuple args(3);
        args.setItem(0, Py::asObject(pObject->getPyObject()));
        args.setItem(1, Py::Long(decimals));
        args.setItem(2, Py::Boolean(advancedShapeContent));
        Py::Module shapecontent(module, true);
        Py::String result(shapecontent.callMemberFunction("buildShapeContent", args));
        stream << result.as_std_string("utf-8");
    }
    catch (Py::Exception&) {
        Base::PyException e;
        e.ReportException();
        stream << baseName.toLatin1().data() << std::endl;
        BRepTools_ShapeSet set;
        set.Add(shape);
        set.DumpExtent(stream);
    }
    shapeContentString += stream.str();
}

QString TaskCheckGeometryResults::getShapeContentString()
{
  return QString::fromStdString(shapeContentString);
}

#if OCC_VERSION_HEX < 0x070500
int TaskCheckGeometryResults::goBOPSingleCheck(const TopoDS_Shape& shapeIn, ResultEntry *theRoot, const QString &baseName,
                                               const Handle(Message_ProgressIndicator)& theProgress)
#else
int TaskCheckGeometryResults::goBOPSingleCheck(const TopoDS_Shape& shapeIn, ResultEntry *theRoot, const QString &baseName,
                                               const Message_ProgressScope& theScope)
#endif
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    bool runSingleThreaded = group->GetBool("RunBOPCheckSingleThreaded", false);
    bool logErrors = group->GetBool("LogErrors", true);
    bool argumentTypeMode = group->GetBool("ArgumentTypeMode", true);
    bool selfInterMode = group->GetBool("SelfInterMode", true);
    bool smallEdgeMode = group->GetBool("SmallEdgeMode", true);
    bool rebuildFaceMode = group->GetBool("RebuildFaceMode", true);
    bool continuityMode = group->GetBool("ContinuityMode", true);
    bool tangentMode = group->GetBool("TangentMode", true);
    bool mergeVertexMode = group->GetBool("MergeVertexMode", true);
    bool mergeEdgeMode = group->GetBool("MergeEdgeMode", true);
    bool curveOnSurfaceMode = group->GetBool("CurveOnSurfaceMode", true);

  //Reference use: src/BOPTest/BOPTest_CheckCommands.cxx

  //I don't why we need to make a copy, but it doesn't work without it.
  //BRepAlgoAPI_Check also makes a copy of the shape.

  //didn't use BRepAlgoAPI_Check because it calls BRepCheck_Analyzer itself and
  //doesn't give us access to it. so I didn't want to run BRepCheck_Analyzer twice to get invalid results.

  //BOPAlgo_ArgumentAnalyzer can check 2 objects with respect to a boolean op.
  //this is left for another time.
  TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(shapeIn).Shape();
  BOPAlgo_ArgumentAnalyzer BOPCheck;

#if OCC_VERSION_HEX < 0x070500
  BOPCheck.SetProgressIndicator(theProgress);
#elif OCC_VERSION_HEX < 0x070600
  BOPCheck.SetProgressIndicator(theScope);
#else
  Q_UNUSED(theScope)
#endif // 0x070500


  BOPCheck.SetShape1(BOPCopy);
  //all settings are false by default. so only turn on what we want.
  BOPCheck.ArgumentTypeMode() = argumentTypeMode;
  BOPCheck.SelfInterMode() = selfInterMode;
  BOPCheck.SmallEdgeMode() = smallEdgeMode;
  BOPCheck.RebuildFaceMode() = rebuildFaceMode;
  BOPCheck.ContinuityMode() = continuityMode;

  BOPCheck.SetParallelMode(!runSingleThreaded); //this doesn't help for speed right now(occt 6.9.1).
  BOPCheck.SetRunParallel(!runSingleThreaded); //performance boost, use all available cores
  BOPCheck.TangentMode() = tangentMode; //these 4 new tests add about 5% processing time.
  BOPCheck.MergeVertexMode() = mergeVertexMode;
  BOPCheck.MergeEdgeMode() = mergeEdgeMode;
  BOPCheck.CurveOnSurfaceMode() = curveOnSurfaceMode;

#ifdef FC_DEBUG
  Base::TimeInfo start_time;
#endif

  BOPCheck.Perform();

#ifdef FC_DEBUG
  float bopAlgoTime = Base::TimeInfo::diffTimeF(start_time,Base::TimeInfo());
  std::cout << std::endl << "BopAlgo check time is: " << bopAlgoTime << std::endl << std::endl;
#endif

  if (!BOPCheck.HasFaulty())
      return 0;

  ResultEntry *entry = new ResultEntry();
  entry->parent = theRoot;
  entry->shape = BOPCopy; //this will cause a problem, with existing entry. i.e. entry is true.
  entry->name = baseName;
  entry->type = shapeEnumToString(shapeIn.ShapeType());
  entry->error = QObject::tr("Invalid");
  entry->viewProviderRoot = currentSeparator;
  entry->viewProviderRoot->ref();
  goSetupResultBoundingBox(entry);
  theRoot->children.push_back(entry);

  const BOPAlgo_ListOfCheckResult &BOPResults = BOPCheck.GetCheckResult();
  BOPAlgo_ListIteratorOfListOfCheckResult BOPResultsIt(BOPResults);
  for (; BOPResultsIt.More(); BOPResultsIt.Next())
  {
    const BOPAlgo_CheckResult &current = BOPResultsIt.Value();
    const TopTools_ListOfShape &faultyShapes1 = current.GetFaultyShapes1();
    TopTools_ListIteratorOfListOfShape faultyShapes1It(faultyShapes1);

    for (;faultyShapes1It.More(); faultyShapes1It.Next())
    {
      const TopoDS_Shape &faultyShape = faultyShapes1It.Value();
      ResultEntry *faultyEntry = new ResultEntry();
      faultyEntry->parent = entry;
      faultyEntry->shape = faultyShape;
      faultyEntry->buildEntryName();
      faultyEntry->type = shapeEnumToString(faultyShape.ShapeType());
      faultyEntry->error = getBOPCheckString(current.GetCheckStatus());
      faultyEntry->viewProviderRoot = currentSeparator;
      entry->viewProviderRoot->ref();
      goSetupResultBoundingBox(faultyEntry);

      if (faultyShape.ShapeType() == TopAbs_FACE)
      {
        goSetupResultTypedSelection(faultyEntry, faultyShape, TopAbs_FACE);
      }
      else if (faultyShape.ShapeType() == TopAbs_EDGE)
      {
        goSetupResultTypedSelection(faultyEntry, faultyShape, TopAbs_EDGE);
      }
      else if (faultyShape.ShapeType() == TopAbs_VERTEX)
      {
        goSetupResultTypedSelection(faultyEntry, faultyShape, TopAbs_VERTEX);
      }
      entry->children.push_back(faultyEntry);

      /*log BOPCheck errors to report view*/
      if (logErrors){
          std::clog << faultyEntry->parent->name.toStdString().c_str() << " : "
                    << faultyEntry->name.toStdString().c_str() << " : "
                    << faultyEntry->type.toStdString().c_str() << " : "
                    << faultyEntry->error.toStdString().c_str()
                    << std::endl;
      }
    }
  }
  return 1;
}


void TaskCheckGeometryResults::dispatchError(ResultEntry *entry, const BRepCheck_Status &stat)
{
    std::vector<FunctionMapType>::iterator mapIt;
    for (mapIt = functionMap.begin(); mapIt != functionMap.end(); ++mapIt)
    {
        if (std::get<0>(*mapIt) == entry->shape.ShapeType() && std::get<1>(*mapIt) == stat)
        {
            (std::get<2>(*mapIt))(entry);
            return;
        }
    }
    goSetupResultBoundingBox(entry);
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    bool logErrors = group->GetBool("LogErrors", true); //log errors to report view

    /*log BRepCheck errors to report view*/
    if (logErrors){
        std::clog << entry->parent->name.toStdString().c_str() << " : "
                  << entry->name.toStdString().c_str() << " : "
                  << entry->type.toStdString().c_str() << " : "
                  << entry->error.toStdString().c_str() << " (BRepCheck)"
                  << std::endl;
    }
}

void TaskCheckGeometryResults::setupFunctionMap()
{
    functionMap.emplace_back(TopAbs_SHELL, BRepCheck_NotClosed, goSetupResultShellNotClosed);
    functionMap.emplace_back(TopAbs_WIRE, BRepCheck_NotClosed, goSetupResultWireNotClosed);
    functionMap.emplace_back(TopAbs_VERTEX, BRepCheck_InvalidPointOnCurve, goSetupResultInvalidPointCurve);
    functionMap.emplace_back(TopAbs_FACE, BRepCheck_IntersectingWires, goSetupResultIntersectingWires);
    functionMap.emplace_back(TopAbs_EDGE, BRepCheck_InvalidCurveOnSurface, goSetupResultInvalidCurveSurface);
    functionMap.emplace_back(TopAbs_EDGE, BRepCheck_InvalidSameParameterFlag, goSetupResultInvalidSameParameterFlag);
    functionMap.emplace_back(TopAbs_FACE, BRepCheck_UnorientableShape, goSetupResultUnorientableShapeFace);
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
                Gui::Selection().addSelection(doc.toLatin1(), object.toLatin1(), sub.toLatin1());
            }
        }
    }
}

bool TaskCheckGeometryResults::split(QString &input, QString &doc, QString &object, QString &sub)
{
    QStringList strings = input.split(QString::fromLatin1("."));
    if (strings.size() != 3)
        return false;
    doc = strings.at(0);
    object = strings.at(1);
    sub = strings.at(2);
    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////

QString PartGui::buildSelectionName(const ResultEntry *entry, const TopoDS_Shape &shape)
{
    const ResultEntry *parentEntry = entry;
    while(parentEntry->parent)
    {
        ResultEntry *temp = parentEntry->parent;
        if (!temp->parent)
          break;
        parentEntry = parentEntry->parent;
    }

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

void PartGui::goSetupResultTypedSelection(ResultEntry* entry, const TopoDS_Shape& shape, TopAbs_ShapeEnum type)
{
    TopExp_Explorer it;
    for (it.Init(shape, type); it.More(); it.Next())
    {
        QString name = buildSelectionName(entry, (it.Current()));
        if (!name.isEmpty())
            entry->selectionStrings.append(name);
    }
}

void PartGui::goSetupResultBoundingBox(ResultEntry *entry)
{
    //empty compound throws bounding box error. Mantis #0001426
    try
    {
      Bnd_Box boundingBox;
      BRepBndLib::Add(entry->shape, boundingBox);
      Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
      boundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);
      SbVec3f boundCenter((xmax - xmin)/2 + xmin, (ymax - ymin)/2 + ymin, (zmax - zmin)/2 + zmin);

      entry->boxSep = new SoSeparator();
      entry->viewProviderRoot->addChild(entry->boxSep);
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

      SoResetTransform *reset = new SoResetTransform();
      group->addChild(reset);

      SoTransform *position = new SoTransform();
      position->translation.setValue(boundCenter);
      group->addChild(position);

      SoCube *cube = new SoCube();
      cube->width.setValue(xmax - xmin);
      cube->height.setValue(ymax - ymin);
      cube->depth.setValue(zmax - zmin);
      group->addChild(cube);
    }
    catch (const Standard_Failure &){}
}

void PartGui::goSetupResultShellNotClosed(ResultEntry *entry)
{
    ShapeAnalysis_FreeBounds shellCheck(entry->shape);
    TopoDS_Compound closedWires = shellCheck.GetClosedWires();
    TopoDS_Compound openWires = shellCheck.GetOpenWires();

    goSetupResultTypedSelection(entry, closedWires, TopAbs_EDGE);
    goSetupResultTypedSelection(entry, openWires, TopAbs_EDGE);

    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultWireNotClosed(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_EDGE);
    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultInvalidPointCurve(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_VERTEX);
    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultIntersectingWires(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_FACE);
    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultInvalidCurveSurface(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_EDGE);
    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultInvalidSameParameterFlag(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_EDGE);
    goSetupResultBoundingBox(entry);
}

void PartGui::goSetupResultUnorientableShapeFace(ResultEntry *entry)
{
    goSetupResultTypedSelection(entry, entry->shape, TopAbs_FACE);
    goSetupResultBoundingBox(entry);
}

////////////////////////////////////////////////////////////////////////////////////////////////

TaskCheckGeometryDialog::TaskCheckGeometryDialog()
    : widget(nullptr), contentLabel(nullptr), okBtn(nullptr), settingsBtn(nullptr), resultsBtn(nullptr)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    bool expandShapeContent = group->GetBool("ExpandShapeContent", false);

    this->setButtonPosition(TaskDialog::South);
    widget = new TaskCheckGeometryResults();

    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    contentLabel = new QTextEdit();
    contentLabel->setText(widget->getShapeContentString());
    shapeContentBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        tr("Shape Content"), true, nullptr);
    shapeContentBox->groupLayout()->addWidget(contentLabel);
    if (!expandShapeContent){
        shapeContentBox->hideGroupBox();
    }
    Content.push_back(shapeContentBox);

    settingsBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        tr("Settings"), true, nullptr);
    Content.push_back(settingsBox);

    autoRunCheckBox = new QCheckBox();
    autoRunCheckBox->setText(tr("Skip settings page"));
    autoRunCheckBox->setToolTip(
        tr("Skip this settings page and run the geometry check automatically.")
        + QStringLiteral("\n")
        + tr("Default: false"));
    autoRunCheckBox->setChecked(group->GetBool("AutoRun", false));
    connect(autoRunCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onAutoRunCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(autoRunCheckBox);

    runBOPCheckBox = new QCheckBox();
    runBOPCheckBox->setText(tr("Run boolean operation check"));
    runBOPCheckBox->setToolTip(tr(
        "Extra boolean operations check that can sometimes find errors that\n"
        "the standard BRep geometry check misses. These errors do not always\n"
        "mean the checked object is unusable.  Default: false"));
    runBOPCheckBox->setChecked(group->GetBool("RunBOPCheck", false));
    connect(runBOPCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onRunBOPCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(runBOPCheckBox);

    runSingleThreadedCheckBox = new QCheckBox();
    runSingleThreadedCheckBox->setText(tr("Single-threaded"));
    runSingleThreadedCheckBox->setToolTip(tr(
        "Run the geometry check in a single thread.  This is slower,\n"
        "but more stable.  Default: false"));
    runSingleThreadedCheckBox->setChecked(group->GetBool("RunSingleThreaded", false));
    connect(runSingleThreadedCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onRunSingleThreadedCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(runSingleThreadedCheckBox);

    logErrorsCheckBox = new QCheckBox();
    logErrorsCheckBox->setText(tr("Log errors"));
    logErrorsCheckBox->setToolTip(tr("Log errors to report view.  Default: true"));
    logErrorsCheckBox->setChecked(group->GetBool("LogErrors", true));
    connect(logErrorsCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onLogErrorsCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(logErrorsCheckBox);

    expandShapeContentCheckBox = new QCheckBox();
    expandShapeContentCheckBox->setText(tr("Expand shape content"));
    expandShapeContentCheckBox->setToolTip(tr(
        "Expand shape content.  Changes will take effect next time you use \n"
        "the check geometry tool.  Default: false"));
    expandShapeContentCheckBox->setChecked(group->GetBool("ExpandShapeContent", false));
    connect(expandShapeContentCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onExpandShapeContentCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(expandShapeContentCheckBox);

    advancedShapeContentCheckBox = new QCheckBox();
    advancedShapeContentCheckBox->setText(tr("Advanced shape content"));
    advancedShapeContentCheckBox->setToolTip(tr(
        "Show advanced shape content.  Changes will take effect next time you use \n"
        "the check geometry tool.  Default: false"));
    advancedShapeContentCheckBox->setChecked(group->GetBool("AdvancedShapeContent", true));
    connect(advancedShapeContentCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onAdvancedShapeContentCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(advancedShapeContentCheckBox);

    settingsBox->groupLayout()->addWidget(new QLabel(tr("\nIndividual boolean operation checks:")));

    argumentTypeModeCheckBox = new QCheckBox();
    argumentTypeModeCheckBox->setText(QStringLiteral("  ") + tr("Bad type"));
    argumentTypeModeCheckBox->setToolTip(tr("Check for bad argument types.  Default: true"));
    argumentTypeModeCheckBox->setChecked(group->GetBool("ArgumentTypeMode", true));
    connect(argumentTypeModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onArgumentTypeModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(argumentTypeModeCheckBox);

    selfInterModeCheckBox = new QCheckBox();
    selfInterModeCheckBox->setText(QStringLiteral("  ") + tr("Self-intersect"));
    selfInterModeCheckBox->setToolTip(tr("Check for self-intersections.  Default: true"));
    selfInterModeCheckBox->setChecked(group->GetBool("SelfInterMode", true));
    connect(selfInterModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onSelfInterModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(selfInterModeCheckBox);

    smallEdgeModeCheckBox = new QCheckBox();
    smallEdgeModeCheckBox->setText(QStringLiteral("  ") + tr("Too small edge"));
    smallEdgeModeCheckBox->setToolTip(tr("Check for edges that are too small.  Default: true"));
    smallEdgeModeCheckBox->setChecked(group->GetBool("SmallEdgeMode", true));
    connect(smallEdgeModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onSmallEdgeModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(smallEdgeModeCheckBox);

    rebuildFaceModeCheckBox = new QCheckBox();
    rebuildFaceModeCheckBox->setText(QStringLiteral("  ") + tr("Nonrecoverable face"));
    rebuildFaceModeCheckBox->setToolTip(tr("Check for nonrecoverable faces.  Default: true"));
    rebuildFaceModeCheckBox->setChecked(group->GetBool("RebuildFaceMode", true));
    connect(rebuildFaceModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onRebuildFaceModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(rebuildFaceModeCheckBox);

    continuityModeCheckBox = new QCheckBox();
    continuityModeCheckBox->setText(QStringLiteral("  ") + tr("Continuity"));
    continuityModeCheckBox->setToolTip(tr("Check for continuity.  Default: true"));
    continuityModeCheckBox->setChecked(group->GetBool("ContinuityMode", true));
    connect(continuityModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onContinuityModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(continuityModeCheckBox);

    tangentModeCheckBox = new QCheckBox();
    tangentModeCheckBox->setText(QStringLiteral("  ") + tr("Incompatibility of face"));
    tangentModeCheckBox->setToolTip(tr("Check for incompatible faces.  Default: true"));
    tangentModeCheckBox->setChecked(group->GetBool("TangentMode", true));
    connect(tangentModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onTangentModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(tangentModeCheckBox);

    mergeVertexModeCheckBox = new QCheckBox();
    mergeVertexModeCheckBox->setText(QStringLiteral("  ") + tr("Incompatibility of vertex"));
    mergeVertexModeCheckBox->setToolTip(tr("Check for incompatible vertices.  Default: true"));
    mergeVertexModeCheckBox->setChecked(group->GetBool("MergeVertexMode", true));
    connect(mergeVertexModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onMergeVertexModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(mergeVertexModeCheckBox);

    mergeEdgeModeCheckBox = new QCheckBox();
    mergeEdgeModeCheckBox->setText(QStringLiteral("  ") + tr("Incompatibility of edge"));
    mergeEdgeModeCheckBox->setToolTip(tr("Check for incompatible edges.  Default: true"));
    mergeEdgeModeCheckBox->setChecked(group->GetBool("MergeEdgeMode", true));
    connect(mergeEdgeModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onMergeEdgeModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(mergeEdgeModeCheckBox);

    curveOnSurfaceModeCheckBox = new QCheckBox();
    curveOnSurfaceModeCheckBox->setText(QStringLiteral("  ") + tr("Invalid curve on surface"));
    curveOnSurfaceModeCheckBox->setToolTip(tr("Check for invalid curves on surfaces.  Default: true"));
    curveOnSurfaceModeCheckBox->setChecked(group->GetBool("CurveOnSurfaceMode", true));
    connect(curveOnSurfaceModeCheckBox, &QCheckBox::toggled,
            this, &TaskCheckGeometryDialog::onCurveOnSurfaceModeCheckBoxToggled);
    settingsBox->groupLayout()->addWidget(curveOnSurfaceModeCheckBox);
    if (group->GetBool("AutoRun",false)){
        settingsBox->hide();
        widget->goCheck();
        contentLabel->setText(widget->getShapeContentString());
    } else {
        taskbox->hide();
        shapeContentBox->hide();
    }
}

bool TaskCheckGeometryDialog::accept()
{
    settingsBtn->setEnabled(true);
    settingsBox->hide();
    shapeContentBox->show();
    taskbox->show();
    widget->goCheck();
    QScrollBar *v = contentLabel->verticalScrollBar();
    v->setValue(v->maximum()); //scroll to bottom
    int curval = v->value(); //save position
    contentLabel->setText(widget->getShapeContentString());
    v->setValue(curval+(v->maximum()-curval)/5);
    return false;
}

bool TaskCheckGeometryDialog::reject()
{
    return true;
}

void TaskCheckGeometryDialog::onClicked(QAbstractButton *btn)
{
    /** when ok (run check) is clicked or when close is clicked
     *  the appropriate accept() or reject() is called already
     *  all we need to do here is enable / disable / show / hide
     *  ui elements
     */

    if(btn == okBtn){
        settingsBtn->setEnabled(true);
    } else if (btn == settingsBtn){
        settingsBtn->setEnabled(false);
        taskbox->hide();
        shapeContentBox->hide();
        settingsBox->show();
        resultsBtn->setEnabled(true);
    } else if (btn == resultsBtn){
        settingsBtn->setEnabled(true);
        taskbox->show();
        shapeContentBox->show();
        settingsBox->hide();
        resultsBtn->setEnabled(false);
    }
}

void TaskCheckGeometryDialog::modifyStandardButtons(QDialogButtonBox* box)
{
    okBtn = box->button(QDialogButtonBox::Ok);
    okBtn->setText(tr("Run check"));
    settingsBtn = box->addButton(tr("Settings"),QDialogButtonBox::ActionRole);
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    if(!group->GetBool("AutoRun",false))
        settingsBtn->setEnabled(false);
    resultsBtn = box->addButton(tr("Results"),QDialogButtonBox::ActionRole);
    resultsBtn->setEnabled(false);
    connect(box, &QDialogButtonBox::clicked, this, &TaskCheckGeometryDialog::onClicked);
}

void TaskCheckGeometryDialog::onAutoRunCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("AutoRun", isOn);
}

void TaskCheckGeometryDialog::onRunBOPCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RunBOPCheck", isOn);
}

void TaskCheckGeometryDialog::onRunSingleThreadedCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RunSingleThreaded", isOn);
}

void TaskCheckGeometryDialog::onLogErrorsCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("LogErrors", isOn);
}

void TaskCheckGeometryDialog::onArgumentTypeModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ArgumentTypeMode", isOn);
}

void TaskCheckGeometryDialog::onExpandShapeContentCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ExpandShapeContent", isOn);
}

void TaskCheckGeometryDialog::onAdvancedShapeContentCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("AdvancedShapeContent", isOn);
}

void TaskCheckGeometryDialog::onSelfInterModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("SelfInterMode", isOn);
}

void TaskCheckGeometryDialog::onSmallEdgeModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("SmallEdgeMode", isOn);
}

void TaskCheckGeometryDialog::onRebuildFaceModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RebuildFaceMode", isOn);
}

void TaskCheckGeometryDialog::onContinuityModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ContinuityMode", isOn);
}

void TaskCheckGeometryDialog::onTangentModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("TangentMode", isOn);
}

void TaskCheckGeometryDialog::onMergeVertexModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("MergeVertexMode", isOn);
}

void TaskCheckGeometryDialog::onMergeEdgeModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("MergeEdgeMode", isOn);
}

void TaskCheckGeometryDialog::onCurveOnSurfaceModeCheckBoxToggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("CurveOnSurfaceMode", isOn);
}

TaskCheckGeometryDialog::~TaskCheckGeometryDialog()
{
  if (widget)
  {
    delete widget;
    widget = nullptr;
  }
  if (contentLabel)
  {
    delete contentLabel;
    contentLabel = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////

BOPProgressIndicator::BOPProgressIndicator (const QString& title, QWidget* parent)
{
    steps = 0;
    canceled = false;

    myProgress = new QProgressDialog(parent);
    myProgress->setWindowTitle(title);
    myProgress->setAttribute(Qt::WA_DeleteOnClose);
}

BOPProgressIndicator::~BOPProgressIndicator ()
{
    myProgress->close();
}

#if OCC_VERSION_HEX < 0x070500
Standard_Boolean BOPProgressIndicator::Show (const Standard_Boolean theForce)
{
    if (theForce) {
        steps = 0;
        canceled = false;

        time.start();
        myProgress->show();

        myProgress->setRange(0, 0);
        myProgress->setValue(0);
    }
    else {
        Handle(TCollection_HAsciiString) aName = GetScope(1).GetName(); //current step
        if (!aName.IsNull())
            myProgress->setLabelText (QString::fromUtf8(aName->ToCString()));
    }

    return Standard_True;
}
#else
void BOPProgressIndicator::Show (const Message_ProgressScope& theScope,
                                 const Standard_Boolean isForce)
{
    Standard_CString aName = theScope.Name(); //current step
    myProgress->setLabelText (QString::fromUtf8(aName));

    if (isForce) {
        myProgress->show();
    }

    QCoreApplication::processEvents();
}

void BOPProgressIndicator::Reset()
{
    steps = 0;
    canceled = false;

    time.start();

    myProgress->setRange(0, 0);
    myProgress->setValue(0);
}
#endif

Standard_Boolean BOPProgressIndicator::UserBreak()
{
    QThread *currentThread = QThread::currentThread();
    if (currentThread == myProgress->thread()) {
        // this is needed to check the status outside BOPAlgo_ArgumentAnalyzer
        //
        // Hint: We must make sure to do this only when calling from the GUI
        // thread because when calling it from a worker thread the thrown
        // exception isn't handled anywhere and thus std::terminate is called
        if (canceled)
            return Standard_True;

        // it suffices to update only every second
        // to avoid to unnecessarily process events
        steps++;
        myProgress->setValue(steps);
        if (time.elapsed() > 1000) {
            time.restart();
            QCoreApplication::processEvents();

            canceled = myProgress->wasCanceled();
            return canceled;
        }
    }

    return Standard_False;
}

#include "moc_TaskCheckGeometry.cpp"
