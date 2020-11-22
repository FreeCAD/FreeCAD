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
# include <QCheckBox>
# include <QScrollBar>
# include <QTextStream>
# include <QThread>
# include <QTreeWidget>
# include <QPushButton>
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
#include <Base/Interpreter.h>
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
    this->setWindowTitle(tr("Check Geometry Results"));
    setupInterface();
    setupFunctionMap();
}

TaskCheckGeometryResults::~TaskCheckGeometryResults()
{
    Gui::Selection().clearSelection();
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
    auto selection = Gui::Selection().getSelection();

    int selectedCount(0), checkedCount(0), invalidShapes(0);
    ResultEntry *theRoot = new ResultEntry();

#if OCC_VERSION_HEX < 0x070500
    Handle(Message_ProgressIndicator) theProgress = new BOPProgressIndicator(tr("Check geometry"), Gui::getMainWindow());
    theProgress->NewScope("BOP check...");
#if OCC_VERSION_HEX >= 0x060900
    theProgress->Show();
#endif
#else
    Handle(Message_ProgressIndicator) theProgress = new BOPProgressIndicator(tr("Check geometry"), Gui::getMainWindow());
    Message_ProgressRange theRange(theProgress->Start());
    Message_ProgressScope theScope(theRange, TCollection_AsciiString("BOP check..."), selection.size());
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
          group->SetBool("RunBOPCheck", runSignal);
          if (runSignal) {
            std::string label = "Checking ";
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
    QString aMessage;
    QTextStream aStream(&aMessage);
    aStream << checkedCount << " processed out of " << selectedCount << " selected\n";
    aStream << invalidShapes << " invalid shapes.";
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

void TaskCheckGeometryResults::buildShapeContent(const QString &baseName, const TopoDS_Shape &shape)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
            GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Units");
    int decimals = group->GetInt("Decimals", 2);
    group = App::GetApplication().GetUserParameter().
        GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    bool advancedShapeContent = group->GetBool("AdvancedShapeContent", true);
    std::ostringstream stream;
    if (!shapeContentString.empty())
        stream << std::endl << std::endl;
    stream << "Checked object: ";
    std::ostringstream cmdstream;
    cmdstream << "_basename = '" << baseName.toStdString().c_str() << "'" << std::endl;
    cmdstream << "_obj = _basename[_basename.index('.')+1:]" << std::endl;
    cmdstream << "_doc = _basename[:_basename.index(_obj)-1]" << std::endl;
    cmdstream << "_shp = App.ActiveDocument.getObject(_obj).Shape" << std::endl;
    cmdstream << "_type = str(_shp.ShapeType)" << std::endl;
    cmdstream << "_result = _doc+'.'+App.ActiveDocument.getObject(_obj).Label+' ('+_obj+'):\\n'" << std::endl;
    cmdstream << "_result += 'Shape type:  '+_type+'\\n'" << std::endl;
    cmdstream << "_result += 'Vertices:  '+str(len(_shp.Vertexes))+'\\n'" << std::endl;
    cmdstream << "_result += 'Edges:  '+str(len(_shp.Edges))+'\\n'" << std::endl;
    cmdstream << "_result += 'Wires:  '+str(len(_shp.Wires))+'\\n'" << std::endl;
    cmdstream << "_result += 'Faces:  '+str(len(_shp.Faces))+'\\n'" << std::endl;
    cmdstream << "_result += 'Shells:  '+str(len(_shp.Shells))+'\\n'" << std::endl;
    cmdstream << "_result += 'Solids:  '+str(len(_shp.Solids))+'\\n'" << std::endl;
    cmdstream << "_result += 'CompSolids:  '+str(len(_shp.CompSolids))+'\\n'" << std::endl;
    cmdstream << "_result += 'Compounds:  '+str(len(_shp.Compounds))+'\\n'" << std::endl;
    cmdstream << "_result += 'Shapes:  '+str(len(_shp.Vertexes+_shp.Edges+_shp.Wires+_shp.Faces+_shp.Shells+_shp.Solids+_shp.CompSolids+_shp.Compounds))+'\\n'" << std::endl;
    if (advancedShapeContent){
        cmdstream << "_result += '----------\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Area') and not 'Wire' in _type and not 'Edge' in _type and not 'Vertex' in _type:" << std::endl;
        cmdstream << "    _result += 'Area:  '+str(round(_shp.Area, " << decimals << "))+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Volume') and not 'Wire' in _type and not 'Edge' in _type and not 'Vertex' in _type and not 'Face' in _type:" << std::endl;
        cmdstream << "    _result += 'Volume:  '+str(round(_shp.Volume, " << decimals << "))+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Mass'):" << std::endl;
        cmdstream << "    _result += 'Mass:  '+str(round(_shp.Mass, " << decimals << "))+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Length'):" << std::endl;
        cmdstream << "    _result += 'Length:  '+str(round(_shp.Length, " << decimals << "))+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Curve') and hasattr(_shp.Curve,'Radius'):" << std::endl;
        cmdstream << "    _result += 'Radius:  '+str(round(_shp.Curve.Radius, " << decimals << "))+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Curve') and hasattr(_shp.Curve,'Center'):" << std::endl;
        cmdstream << "    _result += 'Curve center:  '+str([round(vv," << decimals << ") for vv in _shp.Curve.Center])+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'Curve') and hasattr(_shp.Curve,'Continuity'):" << std::endl;
        cmdstream << "    _result += 'Continuity:  '+str(_shp.Curve.Continuity)+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'CenterOfMass'):" << std::endl;
        cmdstream << "    _result += 'CenterOfMass:  '+str([round(vv," << decimals << ") for vv in _shp.CenterOfMass])+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp,'normalAt'):" << std::endl;
        cmdstream << "    try:" << std::endl;
        cmdstream << "        _result += 'normalAt(0):  '+str([round(vv," << decimals << ") for vv in _shp.normalAt(0)]) +'\\n'" << std::endl;
        cmdstream << "    except:" << std::endl;
        cmdstream << "        try:" << std::endl;
        cmdstream << "            _result += 'normalAt(0,0):  '+str([round(vv," << decimals << ") for vv in _shp.normalAt(0,0)]) +'\\n'" << std::endl;
        cmdstream << "        except:" << std::endl;
        cmdstream << "            pass" << std::endl;
        cmdstream << "if hasattr(_shp, 'isClosed') and ('Wire' in _type or 'Edge' in _type):" << std::endl;
        cmdstream << "    _result += 'isClosed:  '+str(_shp.isClosed())+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp, 'Orientation'):" << std::endl;
        cmdstream << "    _result += 'Orientation:  '+str(_shp.Orientation)+'\\n'" << std::endl;
        cmdstream << "if hasattr(_shp, 'PrincipalProperties'):" << std::endl;
        cmdstream << "    _props = _shp.PrincipalProperties" << std::endl;
        cmdstream << "    for _p in _props:" << std::endl;
        cmdstream << "        if 'Base.Vector' in str(type(_props[_p])) or 'tuple' in str(type(_props[_p])):" << std::endl;
        cmdstream << "            _result += str(_p)+':  '+str([round(vv," << decimals << ") for vv in _props[_p]]) +'\\n'" << std::endl;
        cmdstream << "        else:" << std::endl;
        cmdstream << "            _result += str(_p)+':  '+str(_props[_p])+'\\n'" << std::endl;
    }

    std::string cmd = cmdstream.str();

    try {
        std::string result = Base::Interpreter().runStringWithKey(cmd.c_str(),"_result");
        stream << result;
    }
    catch (Base::PyException&) { //script had runtime error so fall back on OCCT method
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
#if OCC_VERSION_HEX < 0x070500
  BOPCheck.SetProgressIndicator(theProgress);
#else
  BOPCheck.SetProgressIndicator(theScope);
#endif // 0x070500
#else
  Q_UNUSED(theProgress);
#endif
//   BOPCheck.StopOnFirstFaulty() = true; //this doesn't run any faster but gives us less results.
  BOPCheck.SetShape1(BOPCopy);
  //all settings are false by default. so only turn on what we want.
  BOPCheck.ArgumentTypeMode() = argumentTypeMode;
  BOPCheck.SelfInterMode() = selfInterMode;
  BOPCheck.SmallEdgeMode() = smallEdgeMode;
  BOPCheck.RebuildFaceMode() = rebuildFaceMode;
#if OCC_VERSION_HEX >= 0x060700
  BOPCheck.ContinuityMode() = continuityMode;
#endif
#if OCC_VERSION_HEX >= 0x060900
  BOPCheck.SetParallelMode(!runSingleThreaded); //this doesn't help for speed right now(occt 6.9.1).
  BOPCheck.SetRunParallel(!runSingleThreaded); //performance boost, use all available cores
  BOPCheck.TangentMode() = tangentMode; //these 4 new tests add about 5% processing time.
  BOPCheck.MergeVertexMode() = mergeVertexMode;
  BOPCheck.MergeEdgeMode() = mergeEdgeMode;
  BOPCheck.CurveOnSurfaceMode() = curveOnSurfaceMode;
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

TaskCheckGeometryDialog::TaskCheckGeometryDialog()
    : widget(0), contentLabel(0), okBtn(0), settingsBtn(0), resultsBtn(0)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    bool expandShapeContent = group->GetBool("ExpandShapeContent", false);

    this->setButtonPosition(TaskDialog::South);
    widget = new TaskCheckGeometryResults();

    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);

    contentLabel = new QTextEdit();
    contentLabel->setText(widget->getShapeContentString());
    shapeContentBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        tr("Shape Content"), true, 0);
    shapeContentBox->groupLayout()->addWidget(contentLabel);
    if (!expandShapeContent){
        shapeContentBox->hideGroupBox();
    }
    Content.push_back(shapeContentBox);

    settingsBox = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap("Part_CheckGeometry"),
        tr("Settings"), true, 0);
    Content.push_back(settingsBox);

    autoRunCheckBox = new QCheckBox();
    autoRunCheckBox->setText(tr("Skip settings page"));
    autoRunCheckBox->setToolTip(tr("\
Skip this settings page and run the geometry check automatically.\n\
Default: false"));
    autoRunCheckBox->setChecked(group->GetBool("AutoRun", false));
    connect(autoRunCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_autoRunCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(autoRunCheckBox);

    runBOPCheckBox = new QCheckBox();
    runBOPCheckBox->setText(tr("Run BOP check"));
    runBOPCheckBox->setToolTip(tr("\
Extra boolean operations check that can sometimes find errors that\n\
the standard BRep geometry check misses. These errors do not always \n\
mean the checked object is unusable.  Default: false"));
    runBOPCheckBox->setChecked(group->GetBool("RunBOPCheck", false));
    connect(runBOPCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_runBOPCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(runBOPCheckBox);

    runSingleThreadedCheckBox = new QCheckBox();
    runSingleThreadedCheckBox->setText(tr("Single-threaded"));
    runSingleThreadedCheckBox->setToolTip(tr("\
Run the geometry check in a single thread.  This is slower,\n\
but more stable.  Default: false"));
    runSingleThreadedCheckBox->setChecked(group->GetBool("RunSingleThreaded", false));
    connect(runSingleThreadedCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_runSingleThreadedCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(runSingleThreadedCheckBox);

    logErrorsCheckBox = new QCheckBox();
    logErrorsCheckBox->setText(tr("Log errors"));
    logErrorsCheckBox->setToolTip(tr("Log errors to report view.  Default: true"));
    logErrorsCheckBox->setChecked(group->GetBool("LogErrors", true));
    connect(logErrorsCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_logErrorsCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(logErrorsCheckBox);

    expandShapeContentCheckBox = new QCheckBox();
    expandShapeContentCheckBox->setText(tr("Expand shape content"));
    expandShapeContentCheckBox->setToolTip(tr("\
Expand shape content.  Changes will take effect next time you use \n\
the check geometry tool.  Default: false"));
    expandShapeContentCheckBox->setChecked(group->GetBool("ExpandShapeContent", false));
    connect(expandShapeContentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_expandShapeContentCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(expandShapeContentCheckBox);

    advancedShapeContentCheckBox = new QCheckBox();
    advancedShapeContentCheckBox->setText(tr("Advanced shape content"));
    advancedShapeContentCheckBox->setToolTip(tr("\
Show advanced shape content.  Changes will take effect next time you use \n\
the check geometry tool.  Default: false"));
    advancedShapeContentCheckBox->setChecked(group->GetBool("AdvancedShapeContent", true));
    connect(advancedShapeContentCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_advancedShapeContentCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(advancedShapeContentCheckBox);

    settingsBox->groupLayout()->addWidget(new QLabel(tr("\nIndividual BOP Checks:")));

    argumentTypeModeCheckBox = new QCheckBox();
    argumentTypeModeCheckBox->setText(tr("  Bad type"));
    argumentTypeModeCheckBox->setToolTip(tr("Check for bad argument types.  Default: true"));
    argumentTypeModeCheckBox->setChecked(group->GetBool("ArgumentTypeMode", true));
    connect(argumentTypeModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_argumentTypeModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(argumentTypeModeCheckBox);

    selfInterModeCheckBox = new QCheckBox();
    selfInterModeCheckBox->setText(tr("  Self-intersect"));
    selfInterModeCheckBox->setToolTip(tr("Check for self-intersections.  Default: true"));
    selfInterModeCheckBox->setChecked(group->GetBool("SelfInterMode", true));
    connect(selfInterModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_selfInterModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(selfInterModeCheckBox);

    smallEdgeModeCheckBox = new QCheckBox();
    smallEdgeModeCheckBox->setText(tr("  Too small edge"));
    smallEdgeModeCheckBox->setToolTip(tr("Check for edges that are too small.  Default: true"));
    smallEdgeModeCheckBox->setChecked(group->GetBool("SmallEdgeMode", true));
    connect(smallEdgeModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_smallEdgeModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(smallEdgeModeCheckBox);

    rebuildFaceModeCheckBox = new QCheckBox();
    rebuildFaceModeCheckBox->setText(tr("  Nonrecoverable face"));
    rebuildFaceModeCheckBox->setToolTip(tr("Check for nonrecoverable faces.  Default: true"));
    rebuildFaceModeCheckBox->setChecked(group->GetBool("RebuildFaceMode", true));
    connect(rebuildFaceModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_rebuildFaceModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(rebuildFaceModeCheckBox);

    continuityModeCheckBox = new QCheckBox();
    continuityModeCheckBox->setText(tr("  Continuity"));
    continuityModeCheckBox->setToolTip(tr("Check for continuity.  Default: true"));
    continuityModeCheckBox->setChecked(group->GetBool("ContinuityMode", true));
    connect(continuityModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_continuityModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(continuityModeCheckBox);

    tangentModeCheckBox = new QCheckBox();
    tangentModeCheckBox->setText(tr("  Incompatibility of face"));
    tangentModeCheckBox->setToolTip(tr("Check for incompatible faces.  Default: true"));
    tangentModeCheckBox->setChecked(group->GetBool("TangentMode", true));
    connect(tangentModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_tangentModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(tangentModeCheckBox);

    mergeVertexModeCheckBox = new QCheckBox();
    mergeVertexModeCheckBox->setText(tr("  Incompatibility of vertex"));
    mergeVertexModeCheckBox->setToolTip(tr("Check for incompatible vertices.  Default: true"));
    mergeVertexModeCheckBox->setChecked(group->GetBool("MergeVertexMode", true));
    connect(mergeVertexModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_mergeVertexModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(mergeVertexModeCheckBox);

    mergeEdgeModeCheckBox = new QCheckBox();
    mergeEdgeModeCheckBox->setText(tr("  Incompatibility of edge"));
    mergeEdgeModeCheckBox->setToolTip(tr("Check for incompatible edges.  Default: true"));
    mergeEdgeModeCheckBox->setChecked(group->GetBool("MergeEdgeMode", true));
    connect(mergeEdgeModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_mergeEdgeModeCheckBox_toggled(bool)));
    settingsBox->groupLayout()->addWidget(mergeEdgeModeCheckBox);

    curveOnSurfaceModeCheckBox = new QCheckBox();
    curveOnSurfaceModeCheckBox->setText(tr("  Invalid curve on surface"));
    curveOnSurfaceModeCheckBox->setToolTip(tr("Check for invalid curves on surfaces.  Default: true"));
    curveOnSurfaceModeCheckBox->setChecked(group->GetBool("CurveOnSurfaceMode", true));
    connect(curveOnSurfaceModeCheckBox, SIGNAL(toggled(bool)),
            this, SLOT(on_curveOnSurfaceModeCheckBox_toggled(bool)));
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

void TaskCheckGeometryDialog::on_clicked(QAbstractButton *btn)
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
    connect(box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(on_clicked(QAbstractButton*)));
}

void TaskCheckGeometryDialog::on_autoRunCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("AutoRun", isOn);
}

void TaskCheckGeometryDialog::on_runBOPCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RunBOPCheck", isOn);
}

void TaskCheckGeometryDialog::on_runSingleThreadedCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RunSingleThreaded", isOn);
}

void TaskCheckGeometryDialog::on_logErrorsCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("LogErrors", isOn);
}

void TaskCheckGeometryDialog::on_argumentTypeModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ArgumentTypeMode", isOn);
}

void TaskCheckGeometryDialog::on_expandShapeContentCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ExpandShapeContent", isOn);
}

void TaskCheckGeometryDialog::on_advancedShapeContentCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("AdvancedShapeContent", isOn);
}

void TaskCheckGeometryDialog::on_selfInterModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("SelfInterMode", isOn);
}

void TaskCheckGeometryDialog::on_smallEdgeModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("SmallEdgeMode", isOn);
}

void TaskCheckGeometryDialog::on_rebuildFaceModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("RebuildFaceMode", isOn);
}

void TaskCheckGeometryDialog::on_continuityModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("ContinuityMode", isOn);
}

void TaskCheckGeometryDialog::on_tangentModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("TangentMode", isOn);
}

void TaskCheckGeometryDialog::on_mergeVertexModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("MergeVertexMode", isOn);
}

void TaskCheckGeometryDialog::on_mergeEdgeModeCheckBox_toggled(bool isOn)
{
    ParameterGrp::handle group = App::GetApplication().GetUserParameter().
    GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("Mod")->GetGroup("Part")->GetGroup("CheckGeometry");
    group->SetBool("MergeEdgeMode", isOn);
}

void TaskCheckGeometryDialog::on_curveOnSurfaceModeCheckBox_toggled(bool isOn)
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
            myProgress->setLabelText (QString::fromLatin1(aName->ToCString()));
    }

    return Standard_True;
}
#else
void BOPProgressIndicator::Show (const Message_ProgressScope& theScope,
                                 const Standard_Boolean isForce)
{
    Standard_CString aName = theScope.Name(); //current step
    myProgress->setLabelText (QString::fromLatin1(aName));

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
