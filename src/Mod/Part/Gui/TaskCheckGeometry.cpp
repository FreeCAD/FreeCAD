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
# include <QBoxLayout>
# include <QCoreApplication>
# include <QHeaderView>
# include <QTextEdit>
# include <QTextStream>
# include <QThread>
# include <QTreeWidget>
# include <Python.h>
# include <Standard_Version.hxx>
# include <BRepCheck_Analyzer.hxx>
# include <BRepCheck_Result.hxx>
# include <BRepCheck_ListIteratorOfListOfStatus.hxx>
# include <BRepBuilderAPI_Copy.hxx>
# include <BRepTools_ShapeSet.hxx>

# if OCC_VERSION_HEX >= 0x060600
#  include <BOPAlgo_ArgumentAnalyzer.hxx>
#  include <BOPAlgo_ListOfCheckResult.hxx>
# endif

# include <TopoDS.hxx>
# include <TopoDS_Compound.hxx>
# include <TopTools_IndexedMapOfShape.hxx>
# include <TopExp.hxx>
# include <TopExp_Explorer.hxx>
# include <Bnd_Box.hxx>
# include <BRepBndLib.hxx>
# include <ShapeAnalysis_FreeBounds.hxx>
# include <gp_Trsf.hxx>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoSwitch.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoCube.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoTransform.h>
# include <Inventor/nodes/SoResetTransform.h>
#endif //_PreComp_

#include "../App/PartFeature.h"
#include <Gui/BitmapFactory.h>
#include <Gui/Selection.h>
#include <Gui/Document.h>
#include <Gui/Application.h>
#include <Gui/ViewProvider.h>
#include <Gui/WaitCursor.h>
#include <Gui/MainWindow.h>
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

QVector<QString> buildBOPCheckResultVector()
{
  QVector<QString> results;
  results.push_back(QObject::tr("BOPAlgo CheckUnknown"));               //BOPAlgo_CheckUnknown
  results.push_back(QObject::tr("BOPAlgo BadType"));                    //BOPAlgo_BadType
  results.push_back(QObject::tr("BOPAlgo SelfIntersect"));              //BOPAlgo_SelfIntersect
  results.push_back(QObject::tr("BOPAlgo TooSmallEdge"));               //BOPAlgo_TooSmallEdge
  results.push_back(QObject::tr("BOPAlgo NonRecoverableFace"));         //BOPAlgo_NonRecoverableFace
  results.push_back(QObject::tr("BOPAlgo IncompatibilityOfVertex"));    //BOPAlgo_IncompatibilityOfVertex
  results.push_back(QObject::tr("BOPAlgo IncompatibilityOfEdge"));      //BOPAlgo_IncompatibilityOfEdge
  results.push_back(QObject::tr("BOPAlgo IncompatibilityOfFace"));      //BOPAlgo_IncompatibilityOfFace
  results.push_back(QObject::tr("BOPAlgo OperationAborted"));           //BOPAlgo_OperationAborted
  results.push_back(QObject::tr("BOPAlgo GeomAbs_C0"));                 //BOPAlgo_GeomAbs_C0
  results.push_back(QObject::tr("BOPAlgo_InvalidCurveOnSurface"));      //BOPAlgo_InvalidCurveOnSurface
  results.push_back(QObject::tr("BOPAlgo NotValid"));                   //BOPAlgo_NotValid

  return results;
}

#if OCC_VERSION_HEX >= 0x060600
QString getBOPCheckString(const BOPAlgo_CheckStatus &status)
{
  static QVector<QString> strings = buildBOPCheckResultVector();
  int index = static_cast<int>(status);
  if (index < 0 || index > strings.size())
    index = 0;
  return strings.at(index);
}
#endif

ResultEntry::ResultEntry()
{
    viewProviderRoot = 0;
    boxSep = 0;
    boxSwitch = 0;
    parent = 0;
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
  ResultEntry *parentEntry = this;
  while(parentEntry->parent != 0)
  {
      ResultEntry *temp = parentEntry->parent;
      if (temp->parent == 0)
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
    Q_UNUSED(parent);
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

    Handle(Message_ProgressIndicator) theProgress = new BOPProgressIndicator(tr("Check geometry"), Gui::getMainWindow());
    theProgress->NewScope("BOP check...");
#if OCC_VERSION_HEX >= 0x060900
    theProgress->Show();
#endif

    selectedCount = static_cast<int>(selection.size());
    for (it = selection.begin(); it != selection.end(); ++it)
    {
        Part::Feature *feature = dynamic_cast<Part::Feature *>((*it).pObject);
        if (!feature)
            continue;
        currentSeparator = Gui::Application::Instance->activeDocument()->getViewProvider(feature)->getRoot();
        if (!currentSeparator)
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

        buildShapeContent(baseName, shape);

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
          //for now, user has edit the config file to turn it on.
          //following line ensures that the config file has the setting.
          group->SetBool("RunBOPCheck", runSignal);
          if (runSignal) {
            std::string label = "Checking ";
            label += feature->Label.getStrValue();
            label += "...";
            theProgress->NewScope(label.c_str());
            invalidShapes += goBOPSingleCheck(shape, theRoot, baseName, theProgress);
            theProgress->EndScope();
            if (theProgress->UserBreak())
              break;
          }
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

void TaskCheckGeometryResults::buildShapeContent(const QString &baseName, const TopoDS_Shape &shape)
{
  std::ostringstream stream;
  if (!shapeContentString.empty())
    stream << std::endl << std::endl;
  stream << baseName.toLatin1().data() << ":" << std::endl;

  BRepTools_ShapeSet set;
  set.Add(shape);
  set.DumpExtent(stream);

  shapeContentString += stream.str();
}

QString TaskCheckGeometryResults::getShapeContentString()
{
  return QString::fromStdString(shapeContentString);
}

int TaskCheckGeometryResults::goBOPSingleCheck(const TopoDS_Shape& shapeIn, ResultEntry *theRoot, const QString &baseName,
                                               const Handle(Message_ProgressIndicator)& theProgress)
{
  //ArgumentAnalyser was moved at version 6.6. no back port for now.
#if OCC_VERSION_HEX >= 0x060600
  //Reference use: src/BOPTest/BOPTest_CheckCommands.cxx

  //I don't why we need to make a copy, but it doesn't work without it.
  //BRepAlgoAPI_Check also makes a copy of the shape.

  //didn't use BRepAlgoAPI_Check because it calls BRepCheck_Analyzer itself and
  //doesn't give us access to it. so I didn't want to run BRepCheck_Analyzer twice to get invalid results.

  //BOPAlgo_ArgumentAnalyzer can check 2 objects with respect to a boolean op.
  //this is left for another time.
  TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(shapeIn).Shape();
  BOPAlgo_ArgumentAnalyzer BOPCheck;
#if OCC_VERSION_HEX >= 0x060900
  BOPCheck.SetProgressIndicator(theProgress);
#else
  Q_UNUSED(theProgress);
#endif
//   BOPCheck.StopOnFirstFaulty() = true; //this doesn't run any faster but gives us less results.
  BOPCheck.SetShape1(BOPCopy);
  //all settings are false by default. so only turn on what we want.
  BOPCheck.ArgumentTypeMode() = true;
  BOPCheck.SelfInterMode() = true;
  BOPCheck.SmallEdgeMode() = true;
  BOPCheck.RebuildFaceMode() = true;
#if OCC_VERSION_HEX >= 0x060700
  BOPCheck.ContinuityMode() = true;
#endif
#if OCC_VERSION_HEX >= 0x060900
  BOPCheck.SetParallelMode(true); //this doesn't help for speed right now(occt 6.9.1).
  BOPCheck.SetRunParallel(true); //performance boost, use all available cores
  BOPCheck.TangentMode() = true; //these 4 new tests add about 5% processing time.
  BOPCheck.MergeVertexMode() = true;
  BOPCheck.CurveOnSurfaceMode() = true;
  BOPCheck.MergeEdgeMode() = true;
#endif

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

#if OCC_VERSION_HEX < 0x070000
    const BOPCol_ListOfShape &faultyShapes1 = current.GetFaultyShapes1();
    BOPCol_ListIteratorOfListOfShape faultyShapes1It(faultyShapes1);
#else
    const TopTools_ListOfShape &faultyShapes1 = current.GetFaultyShapes1();
    TopTools_ListIteratorOfListOfShape faultyShapes1It(faultyShapes1);
#endif
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
    }
  }
  return 1;
#else
  return 0;
#endif
}


void TaskCheckGeometryResults::dispatchError(ResultEntry *entry, const BRepCheck_Status &stat)
{
    std::vector<FunctionMapType>::iterator mapIt;
    for (mapIt = functionMap.begin(); mapIt != functionMap.end(); ++mapIt)
    {
        if ((*mapIt).get<0>() == entry->shape.ShapeType() && (*mapIt).get<1>() == stat)
        {
            ((*mapIt).get<2>())(entry);
            return;
        }
    }
    goSetupResultBoundingBox(entry);
}

void TaskCheckGeometryResults::setupFunctionMap()
{
    functionMap.push_back(FunctionMapType(TopAbs_SHELL, BRepCheck_NotClosed, goSetupResultShellNotClosed));
    functionMap.push_back(FunctionMapType(TopAbs_WIRE, BRepCheck_NotClosed, goSetupResultWireNotClosed));
    functionMap.push_back(FunctionMapType(TopAbs_VERTEX, BRepCheck_InvalidPointOnCurve, goSetupResultInvalidPointCurve));
    functionMap.push_back(FunctionMapType(TopAbs_FACE, BRepCheck_IntersectingWires, goSetupResultIntersectingWires));
    functionMap.push_back(FunctionMapType(TopAbs_EDGE, BRepCheck_InvalidCurveOnSurface, goSetupResultInvalidCurveSurface));
    functionMap.push_back(FunctionMapType(TopAbs_EDGE, BRepCheck_InvalidSameParameterFlag, goSetupResultInvalidSameParameterFlag));
    functionMap.push_back(FunctionMapType(TopAbs_FACE, BRepCheck_UnorientableShape, goSetupResultUnorientableShapeFace));
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
    while(parentEntry->parent != 0)
    {
        ResultEntry *temp = parentEntry->parent;
        if (temp->parent == 0)
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

TaskCheckGeometryDialog::TaskCheckGeometryDialog() : widget(0), contentLabel(0)
{
    this->setButtonPosition(TaskDialog::South);
    widget = new TaskCheckGeometryResults();
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        widget->windowTitle(), false, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    contentLabel = new QTextEdit();
    contentLabel->setText(widget->getShapeContentString());
    shapeContentBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        tr("Shape Content"), true, 0);
    shapeContentBox->groupLayout()->addWidget(contentLabel);
    shapeContentBox->hideGroupBox();
    Content.push_back(shapeContentBox);
}

TaskCheckGeometryDialog::~TaskCheckGeometryDialog()
{
  if (widget)
  {
    delete widget;
    widget = 0;
  }
  if (contentLabel)
  {
    delete contentLabel;
    contentLabel = 0;
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
            myProgress->setLabelText (QString::fromLatin1(aName->ToCString()));
    }

    return Standard_True;
}

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
