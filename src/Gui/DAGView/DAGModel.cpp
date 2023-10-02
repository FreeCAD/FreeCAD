/***************************************************************************
 *   Copyright (c) 2015 Thomas Anderson <blobfish[at]gmx.com>              *
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
#include <boost/graph/topological_sort.hpp>
#include <boost_graph_reverse_graph.hpp>
#include <memory>
#include <QBrush>
#include <QColor>
#include <QGraphicsPixmapItem>
#include <QGraphicsProxyWidget>
#include <QGraphicsSceneContextMenuEvent>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QPainter>
#include <QPen>
#include <QString>
#include <QTimer>
#endif

#include <Base/Console.h>
#include <Base/TimeInfo.h>
#include <App/Document.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/MenuManager.h>
#include <Gui/Selection.h>
#include <Gui/ViewProviderDocumentObject.h>

#include "DAGModel.h"


using namespace Gui;
using namespace DAG;
namespace sp = std::placeholders;

LineEdit::LineEdit(QWidget* parentIn): QLineEdit(parentIn)
{

}

void LineEdit::keyPressEvent(QKeyEvent *eventIn)
{
  if (eventIn->key() == Qt::Key_Escape)
  {
    Q_EMIT rejectedSignal();
    eventIn->accept();
    return;
  }
  if (
    (eventIn->key() == Qt::Key_Enter) ||
    (eventIn->key() == Qt::Key_Return)
  )
  {
    Q_EMIT acceptedSignal();
    eventIn->accept();
    return;
  }

  QLineEdit::keyPressEvent(eventIn);
}

//I don't think I should have to call invalidate
//and definitely not on the whole scene!
//if we have performance problems, this will definitely
//be something to re-visit. I am not wasting anymore time on
//this right now.
//   this->scene()->invalidate();
//   this->scene()->invalidate(this->sceneTransform().inverted().mapRect(this->boundingRect()));
//   update(boundingRect());
//note: I haven't tried this again since I turned BSP off.

Model::Model(QObject *parentIn, const Gui::Document &documentIn) : QGraphicsScene(parentIn)
{
  //turned off BSP as it was giving inconsistent discovery of items
  //underneath cursor.
  this->setItemIndexMethod(QGraphicsScene::NoIndex);

  theGraph = std::make_shared<Graph>();
  graphLink = std::make_shared<GraphLinkContainer>();
  setupViewConstants();
  setupFilters();

  graphDirty = false;
  currentPrehighlight = nullptr;

  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DAGView");
    selectionMode = static_cast<SelectionMode>(group->GetInt("SelectionMode", 0));
    group->SetInt("SelectionMode", static_cast<int>(selectionMode)); //ensure entry exists.

  QIcon temp(Gui::BitmapFactory().iconFromTheme("dagViewVisible"));
  visiblePixmapEnabled = temp.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  visiblePixmapDisabled = temp.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);

  QIcon passIcon(Gui::BitmapFactory().iconFromTheme("dagViewPass"));
  passPixmap = passIcon.pixmap(iconSize, iconSize);
  QIcon failIcon(Gui::BitmapFactory().iconFromTheme("dagViewFail"));
  failPixmap = failIcon.pixmap(iconSize, iconSize);
  QIcon pendingIcon(Gui::BitmapFactory().iconFromTheme("dagViewPending"));
  pendingPixmap = pendingIcon.pixmap(iconSize, iconSize);

  renameAction = new QAction(this);
  renameAction->setText(tr("Rename"));
  renameAction->setStatusTip(tr("Rename object"));
#ifndef Q_OS_MAC
  renameAction->setShortcut(Qt::Key_F2);
#endif
  connect(renameAction, &QAction::triggered, this, &Model::renameAcceptedSlot);

  editingFinishedAction = new QAction(this);
  editingFinishedAction->setText(tr("Finish editing"));
  editingFinishedAction->setStatusTip(tr("Finish editing object"));
  connect(this->editingFinishedAction, &QAction::triggered,
          this, &Model::editingFinishedSlot);

  //NOLINTBEGIN
  connectNewObject = documentIn.signalNewObject.connect(std::bind(&Model::slotNewObject, this, sp::_1));
  connectDelObject = documentIn.signalDeletedObject.connect(std::bind(&Model::slotDeleteObject, this, sp::_1));
  connectChgObject = documentIn.signalChangedObject.connect(std::bind(&Model::slotChangeObject, this, sp::_1, sp::_2));
  connectEdtObject = documentIn.signalInEdit.connect(std::bind(&Model::slotInEdit, this, sp::_1));
  connectResObject = documentIn.signalResetEdit.connect(std::bind(&Model::slotResetEdit, this, sp::_1));
  //NOLINTEND

  for (auto obj : documentIn.getDocument()->getObjects()) {
    auto vpd = Base::freecad_dynamic_cast<Gui::ViewProviderDocumentObject>(documentIn.getViewProvider(obj));
    if (vpd)
      slotNewObject(*vpd);
  }
}

Model::~Model()
{
  if (connectNewObject.connected())
    connectNewObject.disconnect();
  if (connectDelObject.connected())
    connectDelObject.disconnect();
  if (connectChgObject.connected())
    connectChgObject.disconnect();
  if(connectEdtObject.connected())
    connectEdtObject.disconnect();
  if(connectResObject.connected())
    connectResObject.disconnect();

  removeAllItems();
}

void Model::setupFilters()
{
//   filters.push_back(std::shared_ptr<FilterOrigin>(new FilterOrigin()));
//   filters.push_back(std::shared_ptr<FilterTyped>(new FilterTyped("PartDesign::Body")));
//   filters.push_back(std::shared_ptr<FilterTyped>(new FilterTyped("App::Part")));
}

void Model::setupViewConstants()
{
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DAGView");

  //get font point size.
  int fontPointSize = group->GetInt("FontPointSize", 0);
  group->SetInt("FontPointSize", fontPointSize); //ensure entry exists.
  if (fontPointSize != 0)
  {
    QFont tempFont(this->font());
    tempFont.setPointSize(fontPointSize);
    this->setFont(tempFont);
  }

  //get direction
  direction = group->GetFloat("Direction", 1.0);
  if (direction != -1.0 && direction != 1.0)
    direction = 1.0;
  group->SetFloat("Direction", direction); //ensure entry exists.

  QFontMetrics fontMetric(this->font());
  fontHeight = fontMetric.height();
  verticalSpacing = 1.0;
  rowHeight = (fontHeight + 2.0 * verticalSpacing) * direction; //pixel space top and bottom.
  iconSize = fontHeight;
  pointSize = fontHeight / 2.0;
  pointSpacing = pointSize;
  pointToIcon = iconSize;
  iconToIcon = iconSize * 0.25;
  iconToText = iconSize / 2.0;
  rowPadding = fontHeight;
  backgroundBrushes = {this->palette().base(), this->palette().alternateBase()};
  forgroundBrushes =
  {
    QBrush(Qt::red),
    QBrush(Qt::darkRed),
    QBrush(Qt::green),
    QBrush(Qt::darkGreen),
    QBrush(Qt::blue),
    QBrush(Qt::darkBlue),
    QBrush(Qt::cyan),
    QBrush(Qt::darkCyan),
    QBrush(Qt::magenta),
    QBrush(Qt::darkMagenta),
//     QBrush(Qt::yellow), can't read
    QBrush(Qt::darkYellow),
    QBrush(Qt::gray),
    QBrush(Qt::darkGray),
    QBrush(Qt::lightGray)
  }; //reserve some of the these for highlight stuff.
}

void Model::slotNewObject(const ViewProviderDocumentObject &VPDObjectIn)
{
  Vertex virginVertex = boost::add_vertex(*theGraph);

  addVertexItemsToScene(virginVertex);

  GraphLinkRecord virginRecord;
  virginRecord.DObject = VPDObjectIn.getObject();
  virginRecord.VPDObject = &VPDObjectIn;
  virginRecord.rectItem = (*theGraph)[virginVertex].rectangle.get();
  virginRecord.uniqueName = std::string(virginRecord.DObject->getNameInDocument());
  virginRecord.vertex = virginVertex;
  graphLink->insert(virginRecord);

  //setup rectangle.
  auto rectangle = (*theGraph)[virginVertex].rectangle.get();
  rectangle->setEditingBrush(QBrush(Qt::yellow));

  auto icon = (*theGraph)[virginVertex].icon;
  icon->setPixmap(VPDObjectIn.getIcon().pixmap(iconSize, iconSize));
  (*theGraph)[virginVertex].stateIcon->setPixmap(passPixmap);
  (*theGraph)[virginVertex].text->setFont(this->font());
  //NOLINTBEGIN
  (*theGraph)[virginVertex].connChangeIcon =
      const_cast<Gui::ViewProviderDocumentObject&>(VPDObjectIn).signalChangeIcon.connect(
          std::bind(&Model::slotChangeIcon, this, boost::cref(VPDObjectIn), icon));
  //NOLINTEND

  graphDirty = true;
  lastAddedVertex = Graph::null_vertex();
}

void Model::slotChangeIcon(const ViewProviderDocumentObject &VPDObjectIn, std::shared_ptr<QGraphicsPixmapItem> icon)
{
  icon->setPixmap(VPDObjectIn.getIcon().pixmap(iconSize, iconSize));
  this->invalidate();
}

void Model::slotDeleteObject(const ViewProviderDocumentObject &VPDObjectIn)
{
  Vertex vertex = findRecord(&VPDObjectIn, *graphLink).vertex;

  //remove items from scene.
  removeVertexItemsFromScene(vertex);

  //remove connector items
  auto outRange = boost::out_edges(vertex, *theGraph);
  for (auto outEdgeIt = outRange.first; outEdgeIt != outRange.second; ++outEdgeIt)
    this->removeItem((*theGraph)[*outEdgeIt].connector.get());
  auto inRange = boost::in_edges(vertex, *theGraph);
  for (auto inEdgeIt = inRange.first; inEdgeIt != inRange.second; ++inEdgeIt)
    this->removeItem((*theGraph)[*inEdgeIt].connector.get());

  if (vertex == lastAddedVertex)
    lastAddedVertex = Graph::null_vertex();

  (*theGraph)[vertex].connChangeIcon.disconnect();

  //remove the actual vertex.
  boost::clear_vertex(vertex, *theGraph);
  boost::remove_vertex(vertex, *theGraph);

  eraseRecord(&VPDObjectIn, *graphLink);
  graphDirty = true;
}

void Model::slotChangeObject(const ViewProviderDocumentObject &VPDObjectIn, const App::Property& propertyIn)
{
  std::string name("Empty Name");
  if (propertyIn.hasName())
    name = propertyIn.getName();
  assert(!name.empty());

//   std::cout << std::endl << "inside changed object." << std::endl <<
//     "Property name is: " <<  name << std::endl <<
//     "Property type is: " << propertyIn.getTypeId().getName() << std::endl << std::endl;

  //renaming of objects.
  if (std::string("Label") == name)
  {
    const GraphLinkRecord &record = findRecord(&VPDObjectIn, *graphLink);
    auto text = (*theGraph)[record.vertex].text.get();
    text->setPlainText(QString::fromUtf8(record.DObject->Label.getValue()));
  }
  else if (propertyIn.isDerivedFrom(App::PropertyLinkBase::getClassTypeId()))
  {
    const GraphLinkRecord &record = findRecord(&VPDObjectIn, *graphLink);
    boost::clear_vertex(record.vertex, *theGraph);
    graphDirty = true;
  }
}

void Model::slotInEdit(const ViewProviderDocumentObject& VPDObjectIn)
{
  RectItem *rect = (*theGraph)[findRecord(&VPDObjectIn, *graphLink).vertex].rectangle.get();
  rect->editingStart();
  this->invalidate();
}

void Model::slotResetEdit(const ViewProviderDocumentObject& VPDObjectIn)
{
  RectItem *rect = (*theGraph)[findRecord(&VPDObjectIn, *graphLink).vertex].rectangle.get();
  rect->editingFinished();
  this->invalidate();
}

void Model::selectionChanged(const SelectionChanges& msg)
{
  //TODO: note that treeview uses set selection which sends a message with just a document name
  //and no object name. Have to explore further.

  auto getAllEdges = [this](const Vertex &vertexIn)
  {
    //is there really no function to get both in and out edges?
    std::vector<Edge> out;

    OutEdgeIterator outIt, outItEnd;
    for (boost::tie(outIt, outItEnd) = boost::out_edges(vertexIn, *theGraph); outIt != outItEnd; ++outIt)
      out.push_back(*outIt);

    InEdgeIterator inIt, inItEnd;
    for (boost::tie(inIt, inItEnd) = boost::in_edges(vertexIn, *theGraph); inIt != inItEnd; ++inIt)
      out.push_back(*inIt);

    return out;
  };

  auto highlightConnectorOn = [this, getAllEdges](const Vertex &vertexIn)
  {
    QColor color = (*theGraph)[vertexIn].text->defaultTextColor();
    QPen pen(color);
    pen.setWidth(3.0);
    auto edges = getAllEdges(vertexIn);
    for (auto edge : edges)
    {
      (*theGraph)[edge].connector->setPen(pen);
      (*theGraph)[edge].connector->setZValue(1.0);
    }
  };

  auto highlightConnectorOff = [this, getAllEdges](const Vertex &vertexIn)
  {
    auto edges = getAllEdges(vertexIn);
    for (auto edge : edges)
    {
      (*theGraph)[edge].connector->setPen(QPen());
      (*theGraph)[edge].connector->setZValue(0.0);
    }
  };

  //lambda for clearing selections.
  auto clearSelection = [this, highlightConnectorOff]()
  {
    BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
    {
      RectItem *rect = (*theGraph)[currentVertex].rectangle.get();
      assert(rect);
      rect->selectionOff();
      highlightConnectorOff(currentVertex);
    }
  };

  //lambda for getting rectangle.
  auto getRectangle = [this](const char *in)
  {
    assert(in);
    std::string name(in);
    assert(!name.empty());
    const GraphLinkRecord &record = findRecord(name, *graphLink);
    RectItem *rect = (*theGraph)[record.vertex].rectangle.get();
    assert(rect);
    return rect;
  };

  if (msg.Type == SelectionChanges::AddSelection)
  {
    if (msg.pObjectName)
    {
      RectItem *rect = getRectangle(msg.pObjectName);
      rect->selectionOn();
      highlightConnectorOn(findRecord(std::string(msg.pObjectName), *graphLink).vertex);
    }
  }
  else if(msg.Type == SelectionChanges::RmvSelection)
  {
    if (msg.pObjectName)
    {
      RectItem *rect = getRectangle(msg.pObjectName);
      rect->selectionOff();
      highlightConnectorOff(findRecord(std::string(msg.pObjectName), *graphLink).vertex);
    }
  }
  else if(msg.Type == SelectionChanges::SetSelection)
  {
    clearSelection();

    auto selections = Gui::Selection().getSelection(msg.pDocName);
    for (const auto &selection : selections)
    {
      assert(selection.FeatName);
      RectItem *rect = getRectangle(selection.FeatName);
      rect->selectionOn();
      highlightConnectorOn(findRecord(selection.FeatName, *graphLink).vertex);
    }
  }
  else if(msg.Type == SelectionChanges::ClrSelection)
  {
    clearSelection();
  }

  this->invalidate();
}

void Model::awake()
{
  if (graphDirty)
  {
    updateSlot();
    this->invalidate();
  }
  updateStates();
}

void Model::updateSlot()
{
  //empty outList means it is a root.
  //empty inList means it is a leaf.

  //NOTE: some of the following loops can/should be combined
  //for speed. Not doing yet, as I want a simple algorithm until
  //a more complete picture is formed.

  Base::TimeInfo startTime;

  //here we will cycle through the graph updating edges.
  //we have to do this first and in isolation because everything is dependent on an up to date graph.
  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    const App::DocumentObject *currentDObject = findRecord(currentVertex, *graphLink).DObject;
    std::vector<App::DocumentObject *> otherDObjects = currentDObject->getOutList();
    for (auto &currentOtherDObject : otherDObjects)
    {
      if (!hasRecord(currentOtherDObject, *graphLink))
          continue;
      Vertex otherVertex = findRecord(currentOtherDObject, *graphLink).vertex;
      bool result;
      Edge edge;
      boost::tie(edge, result) = boost::add_edge(currentVertex, otherVertex, *theGraph);
      if (result)
      {
        (*theGraph)[edge].connector = std::make_shared<QGraphicsPathItem>();
        (*theGraph)[edge].connector->setZValue(0.0);
      }
    }
  }

  //apply filters.
  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    (*theGraph)[currentVertex].dagVisible = true; //default to shown.
    for (const auto &currentFilter : filters)
    {
      if (!currentFilter->enabled || currentFilter->type != FilterBase::Type::Exclusion)
        continue;
      if (currentFilter->goFilter(currentVertex, *theGraph, *graphLink))
        (*theGraph)[currentVertex].dagVisible = false;
    }
  }
  //inclusion takes precedence. Separate loop because filters might probe
  //children and parents. So we want to ensure all exclusions are done
  //before inclusions start.
  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    for (const auto &currentFilter : filters)
    {
      if (!currentFilter->enabled || currentFilter->type != FilterBase::Type::Inclusion)
        continue;
      if (currentFilter->goFilter(currentVertex, *theGraph, *graphLink))
        (*theGraph)[currentVertex].dagVisible = true;
    }
  }

  //sync scene items to graph vertex dagVisible.
  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    if ((*theGraph)[currentVertex].dagVisible && (!(*theGraph)[currentVertex].rectangle->scene()))
      addVertexItemsToScene(currentVertex);
    if ((!(*theGraph)[currentVertex].dagVisible) && (*theGraph)[currentVertex].rectangle->scene())
      removeVertexItemsFromScene(currentVertex);
  }

  //sync scene items for graph edge.
  BGL_FORALL_EDGES(currentEdge, *theGraph, Graph)
  {
    Vertex source = boost::source(currentEdge, *theGraph);
    Vertex target = boost::target(currentEdge, *theGraph);

    bool edgeVisible = (*theGraph)[source].dagVisible && (*theGraph)[target].dagVisible;
    if (edgeVisible && (!(*theGraph)[currentEdge].connector->scene()))
      this->addItem((*theGraph)[currentEdge].connector.get());
    if ((!edgeVisible) && (*theGraph)[currentEdge].connector->scene())
      this->removeItem((*theGraph)[currentEdge].connector.get());
  }

  indexVerticesEdges();
  Path sorted;
  try
  {
    boost::topological_sort(*theGraph, std::back_inserter(sorted));
  }
  catch(const boost::not_a_dag &)
  {
    Base::Console().Error("not a dag exception in DAGView::Model::updateSlot()\n");
    //do not continuously report an error for cyclic graphs
    graphDirty = false;
    return;
  }
  //index the vertices in sort order.
  int tempIndex = 0;
  for (const auto &currentVertex : sorted)
  {
    (*theGraph)[currentVertex].topoSortIndex = tempIndex;
    tempIndex++;
  }

  //draw graph(nodes and connectors).
  int currentRow = 0;
  int currentColumn = -1; //we know first column is going to be root so will be kicked up to 0.
  int maxColumn = currentColumn; //used for determining offset of icons and text.
  float maxTextLength = 0;
  for (const auto &currentVertex : sorted)
  {
    if (!(*theGraph)[currentVertex].dagVisible)
      continue;

    if (boost::out_degree(currentVertex, *theGraph) == 0)
      currentColumn = 0;
    else
    {
      //loop parents and find an acceptable column.
      int farthestParentIndex = sorted.size();
      ColumnMask columnMask;
      Path parentVertices;
      OutEdgeIterator it, itEnd;
      boost::tie(it, itEnd) = boost::out_edges(currentVertex, *theGraph);
      for (;it != itEnd; ++it)
      {
//         std::cout << std::endl << "name: " << findRecord(currentVertex, *graphLink).DObject->Label.getValue() << std::endl;

        Vertex target = boost::target(*it, *theGraph);
        parentVertices.push_back(target);
        int currentParentIndex = (*theGraph)[target].topoSortIndex;
        if (currentParentIndex < farthestParentIndex)
        {
          Path::const_iterator start = sorted.begin() + currentParentIndex + 1; // 1 after
          Path::const_iterator end = sorted.begin() + (*theGraph)[currentVertex].topoSortIndex; // 1 before
          Path::const_iterator it;
          for (it = start; it != end; ++it)
          {
//             std::cout << "    parent: " << findRecord(*it, *graphLink).DObject->Label.getValue() << std::endl;

            columnMask |= (*theGraph)[*it].column;
          }
          farthestParentIndex = currentParentIndex;
        }
      }

      //have to create a smaller subset to get through std::cout.
//       std::bitset<8> testSet;
//       for (unsigned int index = 0; index < testSet.size(); ++index)
//         testSet[index]= columnMask[index];
//       std::cout << "mask for " << findRecord(currentVertex, *graphLink).DObject->Label.getValue() << "      " <<
//         testSet.to_string() << std::endl;

      //now we should have a mask representing the columns that are being used.
      //this is from the lowest parent, in the topo sort, to last entry.

      //try to use the same column as one of the parents.(*theGraph)[*it].column
      int destinationColumn = 0; //default to first column
      for (const auto &currentParent : parentVertices)
      {
        if (((*theGraph)[currentParent].column & columnMask).none())
        {
          //go with first visible parent for now.
          if (!(*theGraph)[currentParent].dagVisible)
            continue;
          destinationColumn = static_cast<int>(columnFromMask((*theGraph)[currentParent].column));
          break;
        }
      }
      //if destination not valid look for the first open column.
      if (columnMask.test(destinationColumn))
      {
        for (std::size_t index = 0; index < columnMask.size(); ++index)
        {
          if (! columnMask.test(index))
          {
            destinationColumn = index;
            break;
          }
        }
      }

      currentColumn = destinationColumn;
    }

    assert(currentColumn < static_cast<int>(ColumnMask().size())); //temp limitation.

    maxColumn = std::max(currentColumn, maxColumn);
    QBrush currentBrush(forgroundBrushes.at(currentColumn % forgroundBrushes.size()));

    auto rectangle = (*theGraph)[currentVertex].rectangle.get();
    rectangle->setRect(-rowPadding, 0.0, rowPadding, rowHeight); //calculate actual length later.
    rectangle->setTransform(QTransform::fromTranslate(0, rowHeight * currentRow));
    rectangle->setBackgroundBrush(backgroundBrushes[currentRow % backgroundBrushes.size()]);

    auto point = (*theGraph)[currentVertex].point.get();
    point->setRect(0.0, 0.0, pointSize, pointSize);
    point->setTransform(QTransform::fromTranslate(pointSpacing * currentColumn,
      rowHeight * currentRow + rowHeight / 2.0 - pointSize / 2.0));
    point->setBrush(currentBrush);

    float cheat = 0.0;
    if (direction == -1)
      cheat = rowHeight;

    auto visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
    visiblePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.

    auto statePixmap = (*theGraph)[currentVertex].stateIcon.get();
    statePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.

    auto pixmap = (*theGraph)[currentVertex].icon.get();
    pixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + cheat)); //calculate x location later.

    auto text = (*theGraph)[currentVertex].text.get();
    text->setPlainText(QString::fromUtf8(findRecord(currentVertex, *graphLink).DObject->Label.getValue()));
    text->setDefaultTextColor(currentBrush.color());
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate
      (0.0, rowHeight * currentRow - verticalSpacing * 2.0 + cheat)); //calculate x location later.
    (*theGraph)[currentVertex].lastVisibleState = VisibilityState::None; //force visual update for color.

    //store column and row int the graph. use for connectors later.
    (*theGraph)[currentVertex].row = currentRow;
    (*theGraph)[currentVertex].column.reset().set((currentColumn));

    //our list is topo sorted so all dependents should be located, so we can build the connectors.
    //will have some more logic for connector path, simple for now.
    float currentX = pointSpacing * currentColumn + pointSize / 2.0;
    float currentY = rowHeight * currentRow + rowHeight / 2.0;
    OutEdgeIterator it, itEnd;
    boost::tie(it, itEnd) = boost::out_edges(currentVertex, *theGraph);
    for (; it != itEnd; ++it)
    {
      Vertex target = boost::target(*it, *theGraph);
      if (!(*theGraph)[target].dagVisible)
        continue; //we don't make it here if source isn't visible. So don't have to worry about that.
      float dependentX = pointSpacing * static_cast<int>(columnFromMask((*theGraph)[target].column)) + pointSize / 2.0; //on center.
      columnFromMask((*theGraph)[target].column);
      float dependentY = rowHeight * (*theGraph)[target].row + rowHeight / 2.0;

      QGraphicsPathItem *pathItem = (*theGraph)[*it].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentX, currentY);
      if (currentColumn == static_cast<int>(columnFromMask((*theGraph)[target].column)))
        path.lineTo(currentX, dependentY); //straight connector in y.
      else
      {
        //connector with bend.
        float radius = pointSpacing / 1.9; //no zero length line.

        path.lineTo(currentX, dependentY + radius * direction);

        float yPosition;
        if (direction == -1.0)
          yPosition = dependentY - 2.0 * radius;
        else
          yPosition = dependentY;
        float width = 2.0 * radius;
        float height = width;
        if (dependentX > currentX) //radius to the right.
        {
          QRectF arcRect(currentX, yPosition, width, height);
          path.arcTo(arcRect, 180.0, 90.0 * -direction);
        }
        else //radius to the left.
        {
          QRectF arcRect(currentX - 2.0 * radius, yPosition, width, height);
          path.arcTo(arcRect, 0.0, 90.0 * direction);
        }
        path.lineTo(dependentX, dependentY);
      }
      pathItem->setPath(path);
    }

    currentRow++;
  }

  //now that we have the graph drawn we know where to place icons and text.
  float columnSpacing = (maxColumn * pointSpacing);
  for (const auto &currentVertex : sorted)
  {
    float localCurrentX = columnSpacing;
    localCurrentX += pointToIcon;
    auto visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
    QTransform visibleIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    visiblePixmap->setTransform(visiblePixmap->transform() * visibleIconTransform);

    localCurrentX += iconSize + iconToIcon;
    auto statePixmap = (*theGraph)[currentVertex].stateIcon.get();
    QTransform stateIconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    statePixmap->setTransform(statePixmap->transform() * stateIconTransform);

    localCurrentX += iconSize + iconToIcon;
    auto pixmap = (*theGraph)[currentVertex].icon.get();
    QTransform iconTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    pixmap->setTransform(pixmap->transform() * iconTransform);

    localCurrentX += iconSize + iconToText;
    auto text = (*theGraph)[currentVertex].text.get();
    QTransform textTransform = QTransform::fromTranslate(localCurrentX, 0.0);
    text->setTransform(text->transform() * textTransform);

    auto rectangle = (*theGraph)[currentVertex].rectangle.get();
    QRectF rect = rectangle->rect();
    rect.setWidth(localCurrentX + maxTextLength + 2.0 * rowPadding);
    rectangle->setRect(rect);
  }

  //Modeling_Challenge_Casting_ta4 with 59 features: "Initialize DAG View time: 0.007"
  //keeping algo simple with extra loops only added 0.002 to above number.
//   std::cout << "Initialize DAG View time: " << Base::TimeInfo::diffTimeF(startTime, Base::TimeInfo()) << std::endl;

//   outputGraphviz<Graph>(*theGraph, "./graphviz.dot");
  graphDirty = false;
}

void Model::indexVerticesEdges()
{
  std::size_t index = 0;

  //index vertices.
  VertexIterator it, itEnd;
  for(boost::tie(it, itEnd) = boost::vertices(*theGraph); it != itEnd; ++it)
  {
    boost::put(boost::vertex_index, *theGraph, *it, index);
    index++;
  }

  //index edges. didn't need this when I put it in.
  EdgeIterator eit, eitEnd;
  index = 0;
  for(boost::tie(eit, eitEnd) = boost::edges(*theGraph); eit != eitEnd; ++eit)
  {
    boost::put(boost::edge_index, *theGraph, *eit, index);
    index++;
  }
}

void Model::removeAllItems()
{
  if (theGraph)
  {
    BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
      removeVertexItemsFromScene(currentVertex);

    BGL_FORALL_EDGES(currentEdge, *theGraph, Graph)
    {
      if ((*theGraph)[currentEdge].connector->scene())
        this->removeItem((*theGraph)[currentEdge].connector.get());
    }
  }
}

void Model::addVertexItemsToScene(const Gui::DAG::Vertex& vertexIn)
{
  //these are either all in or all out. so just test rectangle.
  if ((*theGraph)[vertexIn].rectangle->scene()) //already in the scene.
    return;
  this->addItem((*theGraph)[vertexIn].rectangle.get());
  this->addItem((*theGraph)[vertexIn].point.get());
  this->addItem((*theGraph)[vertexIn].visibleIcon.get());
  this->addItem((*theGraph)[vertexIn].stateIcon.get());
  this->addItem((*theGraph)[vertexIn].icon.get());
  this->addItem((*theGraph)[vertexIn].text.get());
}

void Model::removeVertexItemsFromScene(const Gui::DAG::Vertex& vertexIn)
{
  //these are either all in or all out. so just test rectangle.
  if (!(*theGraph)[vertexIn].rectangle->scene()) //not in the scene.
    return;
  this->removeItem((*theGraph)[vertexIn].rectangle.get());
  this->removeItem((*theGraph)[vertexIn].point.get());
  this->removeItem((*theGraph)[vertexIn].visibleIcon.get());
  this->removeItem((*theGraph)[vertexIn].stateIcon.get());
  this->removeItem((*theGraph)[vertexIn].text.get());
  this->removeItem((*theGraph)[vertexIn].icon.get());
}

void Model::updateStates()
{
  //not sure I want to use the same pixmap merge for failing feature icons.
  //thinking maybe red background or another column of icons for state?

  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    const GraphLinkRecord &record = findRecord(currentVertex, *graphLink);

    auto visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
    VisibilityState currentVisibilityState = (record.VPDObject->isShow()) ? (VisibilityState::On) : (VisibilityState::Off);
    if
    (
      (currentVisibilityState != (*theGraph)[currentVertex].lastVisibleState) ||
      ((*theGraph)[currentVertex].lastVisibleState == VisibilityState::None)
    )
    {
      if (record.VPDObject->isShow())
        visiblePixmap->setPixmap(visiblePixmapEnabled);
      else
        visiblePixmap->setPixmap(visiblePixmapDisabled);
      (*theGraph)[currentVertex].lastVisibleState = currentVisibilityState;
    }

    FeatureState currentFeatureState = FeatureState::Pass;
    if (record.DObject->isError())
      currentFeatureState = FeatureState::Fail;
    else if ((record.DObject->mustExecute() == 1))
      currentFeatureState = FeatureState::Pending;
    if (currentFeatureState != (*theGraph)[currentVertex].lastFeatureState)
    {
      if (currentFeatureState == FeatureState::Pass)
      {
        (*theGraph)[currentVertex].stateIcon->setPixmap(passPixmap);
      }
      else
      {
        if (currentFeatureState == FeatureState::Fail)
          (*theGraph)[currentVertex].stateIcon->setPixmap(failPixmap);
        else
          (*theGraph)[currentVertex].stateIcon->setPixmap(pendingPixmap);
      }
      (*theGraph)[currentVertex].stateIcon->setToolTip(QString::fromLatin1(record.DObject->getStatusString()));
      (*theGraph)[currentVertex].lastFeatureState = currentFeatureState;
    }
  }
}

std::size_t Model::columnFromMask(const ColumnMask &maskIn)
{
  std::string maskString = maskIn.to_string();
  return maskString.size() - maskString.find('1') - 1;
}

RectItem* Model::getRectFromPosition(const QPointF& position)
{
  RectItem *rect = nullptr;
  auto theItems = this->items(position, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  for (auto currentItem : theItems)
  {
    rect = dynamic_cast<RectItem *>(currentItem);
    if (rect) break;
  }

  return rect;
}

void Model::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  auto clearPrehighlight = [this]()
  {
    if (currentPrehighlight)
    {
      currentPrehighlight->preHighlightOff();
      currentPrehighlight = nullptr;
    }
  };

  RectItem *rect = getRectFromPosition(event->scenePos());
  if (!rect)
  {
    clearPrehighlight();
    return;
  }

  if (rect == currentPrehighlight)
    return;

  clearPrehighlight();
  rect->preHighlightOn();
  currentPrehighlight = rect;
  invalidate();

  QGraphicsScene::mouseMoveEvent(event);
}

void Model::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  auto goShiftSelect = [this, event]()
  {
    QPointF currentPickPoint = event->scenePos();
    QGraphicsLineItem intersectionLine(QLineF(lastPick, currentPickPoint));
    QList<QGraphicsItem *>selection = collidingItems(&intersectionLine);
    for (auto currentItem : selection)
    {
      auto rect = dynamic_cast<RectItem *>(currentItem);
      if (!rect) continue;
      const GraphLinkRecord &selectionRecord = findRecord(rect, *graphLink);
      Gui::Selection().addSelection(selectionRecord.DObject->getDocument()->getName(),
                                    selectionRecord.DObject->getNameInDocument());
    }
  };

  auto toggleSelect = [](const App::DocumentObject *dObjectIn, RectItem *rectIn)
  {
    if (rectIn->isSelected())
      Gui::Selection().rmvSelection(dObjectIn->getDocument()->getName(), dObjectIn->getNameInDocument());
    else
      Gui::Selection().addSelection(dObjectIn->getDocument()->getName(), dObjectIn->getNameInDocument());
  };

  if (proxy)
    renameAcceptedSlot();

  if (event->button() == Qt::LeftButton)
  {
    RectItem *rect = getRectFromPosition(event->scenePos());
    if (rect)
    {
        const GraphLinkRecord &record = findRecord(rect, *graphLink);

        //don't like that I am doing this again here after getRectFromPosition call.
        QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
        auto pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
        if (pixmapItem && (pixmapItem == (*theGraph)[record.vertex].visibleIcon.get()))
        {
          //get all selections, but for now just the current pick.
          if ((*theGraph)[record.vertex].lastVisibleState == VisibilityState::Off)
            const_cast<ViewProviderDocumentObject *>(record.VPDObject)->show(); //const hack
          else
            const_cast<ViewProviderDocumentObject *>(record.VPDObject)->hide(); //const hack

          return;
        }

        const App::DocumentObject *dObject = record.DObject;
        if (selectionMode == SelectionMode::Single)
        {
          if (event->modifiers() & Qt::ControlModifier)
          {
            toggleSelect(dObject, rect);
          }
          else if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
          {
            goShiftSelect();
          }
          else
          {
            Gui::Selection().clearSelection(dObject->getDocument()->getName());
            Gui::Selection().addSelection(dObject->getDocument()->getName(), dObject->getNameInDocument());
          }
        }
        if (selectionMode == SelectionMode::Multiple)
        {
          if((event->modifiers() & Qt::ShiftModifier) && lastPickValid)
          {
            goShiftSelect();
          }
          else
          {
            toggleSelect(dObject, rect);
          }
        }
        lastPickValid = true;
        lastPick = event->scenePos();
    }
    else
    {
      lastPickValid = false;
      Gui::Selection().clearSelection(); //get document name?
    }
  }

  QGraphicsScene::mousePressEvent(event);
}

void Model::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event)
{
  if (event->button() == Qt::LeftButton)
  {
    auto selections = getAllSelected();
    if(selections.size() != 1)
      return;
    const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
    Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
    MDIView *view = doc->getActiveView();
    if (view)
      getMainWindow()->setActiveWindow(view);
    const_cast<ViewProviderDocumentObject*>(record.VPDObject)->doubleClicked();
  }

  QGraphicsScene::mouseDoubleClickEvent(event);
}


std::vector<Gui::DAG::Vertex> Model::getAllSelected()
{
  std::vector<Gui::DAG::Vertex> out;

  BGL_FORALL_VERTICES(currentVertex, *theGraph, Graph)
  {
    if ((*theGraph)[currentVertex].rectangle->isSelected())
      out.push_back(currentVertex);
  }

  return out;
}

void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  RectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    const GraphLinkRecord &record = findRecord(rect, *graphLink);

    //don't like that I am doing this again here after getRectFromPosition call.
    QGraphicsItem *item = itemAt(event->scenePos(), QTransform());
    auto pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
    if (pixmapItem && (pixmapItem == (*theGraph)[record.vertex].visibleIcon.get()))
    {
      visiblyIsolate(record.vertex);
      return;
    }

    if (!rect->isSelected())
    {
      Gui::Selection().clearSelection(record.DObject->getDocument()->getName());
      Gui::Selection().addSelection(record.DObject->getDocument()->getName(), record.DObject->getNameInDocument());
      lastPickValid = true;
      lastPick = event->scenePos();
    }

    MenuItem view;
    Gui::Application::Instance->setupContextMenu("Tree", &view);
    QMenu contextMenu;
    MenuManager::getInstance()->setupContextMenu(&view, contextMenu);

    //actions for only one selection.
    std::vector<Gui::DAG::Vertex> selections = getAllSelected();
    if (selections.size() == 1)
    {
      contextMenu.addAction(renameAction);
      //when we have only one selection then we know it is rect from above.
      if (!rect->isEditing())
        const_cast<Gui::ViewProviderDocumentObject*>(record.VPDObject)->setupContextMenu
          (&contextMenu, this, SLOT(editingStartSlot())); //const hack.
      else
        contextMenu.addAction(editingFinishedAction);
    }

    if (contextMenu.actions().count() > 0)
        contextMenu.exec(event->screenPos());
  }

  QGraphicsScene::contextMenuEvent(event);
}

void Model::onRenameSlot()
{
  assert(!proxy);
  std::vector<Gui::DAG::Vertex> selections = getAllSelected();
  assert(selections.size() == 1);

  auto lineEdit = new LineEdit();
  auto text = (*theGraph)[selections.front()].text.get();
  lineEdit->setText(text->toPlainText());
  connect(lineEdit, &LineEdit::acceptedSignal, this, &Model::renameAcceptedSlot);
  connect(lineEdit, &LineEdit::rejectedSignal, this, &Model::renameRejectedSlot);

  proxy = this->addWidget(lineEdit);
  proxy->setGeometry(text->sceneBoundingRect());

  lineEdit->selectAll();
  QTimer::singleShot(0, lineEdit, qOverload<>(&QLineEdit::setFocus));
}

void Model::renameAcceptedSlot()
{
  assert(proxy);

  std::vector<Gui::DAG::Vertex> selections = getAllSelected();
  assert(selections.size() == 1);
  const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);

  auto lineEdit = dynamic_cast<LineEdit*>(proxy->widget());
  assert(lineEdit);
  const_cast<App::DocumentObject*>(record.DObject)->Label.setValue(lineEdit->text().toUtf8().constData()); //const hack

  finishRename();
}

void Model::renameRejectedSlot()
{
  finishRename();
}

void Model::finishRename()
{
  assert(proxy);
  this->removeItem(proxy);
  proxy->deleteLater();
  proxy = nullptr;
  this->invalidate();
}

void Model::editingStartSlot()
{
  auto action = qobject_cast<QAction*>(sender());
  if (action)
  {
    int edit = action->data().toInt();
    auto selections = getAllSelected();
    assert(selections.size() == 1);
    const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
    Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
    MDIView *view = doc->getActiveView();
    if (view)
      getMainWindow()->setActiveWindow(view);
    doc->setEdit(const_cast<ViewProviderDocumentObject*>(record.VPDObject), edit);
  }
}

void Model::editingFinishedSlot()
{
  auto selections = getAllSelected();
  assert(selections.size() == 1);
  const GraphLinkRecord &record = findRecord(selections.front(), *graphLink);
  Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
  doc->commitCommand();
  doc->resetEdit();
  doc->getDocument()->recompute();
}

void Model::visiblyIsolate(Gui::DAG::Vertex sourceIn)
{
  auto buildSkipTypes = []()
  {
    std::vector<Base::Type> out;
    Base::Type type;
    type = Base::Type::fromName("App::DocumentObjectGroup");
    if (type != Base::Type::badType()) out.push_back(type);
    type = Base::Type::fromName("App::Part");
    if (type != Base::Type::badType()) out.push_back(type);
    type = Base::Type::fromName("PartDesign::Body");
    if (type != Base::Type::badType()) out.push_back(type);

    return out;
  };

  auto testSkipType = [](const App::DocumentObject *dObject, const std::vector<Base::Type> &types)
  {
    for (const auto &currentType : types)
    {
      if (dObject->isDerivedFrom(currentType))
        return true;
    }
    return false;
  };

  indexVerticesEdges();
  Path connectedVertices;
  ConnectionVisitor visitor(connectedVertices);
  boost::breadth_first_search(*theGraph, sourceIn, boost::visitor(visitor));
  boost::breadth_first_search(boost::make_reverse_graph(*theGraph), sourceIn, boost::visitor(visitor));

  //note source vertex is added twice to Path. Once for each search.
  static std::vector<Base::Type> skipTypes = buildSkipTypes();
  for (const auto &currentVertex : connectedVertices)
  {
    const GraphLinkRecord &record = findRecord(currentVertex, *graphLink);
    if (testSkipType(record.DObject, skipTypes))
      continue;
    const_cast<ViewProviderDocumentObject *>(record.VPDObject)->hide(); //const hack
  }

  const GraphLinkRecord &sourceRecord = findRecord(sourceIn, *graphLink);
  if (!testSkipType(sourceRecord.DObject, skipTypes))
    const_cast<ViewProviderDocumentObject *>(sourceRecord.VPDObject)->show(); //const hack
}


#include <moc_DAGModel.cpp>
