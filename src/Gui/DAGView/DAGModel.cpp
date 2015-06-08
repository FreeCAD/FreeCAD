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
#include <boost/signals.hpp>
#include <boost/bind.hpp>
#include <boost/graph/topological_sort.hpp>

#include <QAbstractEventDispatcher>
#include <QApplication>
#include <QString>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>
#endif

#include <deque>
#include <unordered_set>

#include <Base/TimeInfo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection.h>

#include "DAGModel.h"

using namespace Gui;
using namespace DAG;

ViewEntryRectItem::ViewEntryRectItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
  selected = false;
  preSelected = false;
}

//I dont think I should have to call invalidate
//and definitely not on the whole scene!
//if we have performance problems, this will definitely
//be something to re-visit. I am not wasting anymore time on
//this right now.
//   this->scene()->invalidate();
//   this->scene()->invalidate(this->sceneTransform().inverted().mapRect(this->boundingRect()));
//   update(boundingRect());
//note: I haven't tried this again since I turned BSP off.

void ViewEntryRectItem::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
  //TODO figure out how to mimic painting of itemviews. QStyle, QStyledItemDelegate.
  
  QBrush brush = backgroundBrush;
  if (selected)
    brush = selectionBrush;
  if (preSelected)
    brush = preSelectionBrush;
  if (selected && preSelected)
    brush = bothBrush;
  
  //heights are negative.
  float radius = std::min(this->rect().width(), std::fabs(this->rect().height())) * 0.1;
  painter->setBrush(brush);
  painter->setPen(this->pen()); //should be Qt::NoPen.
  painter->drawRoundedRect(this->rect(), radius, radius);
  
//   QGraphicsRectItem::paint(painter, option, widget);
}

VertexProperty::VertexProperty() : 
  rectangle(new ViewEntryRectItem()),
  point(new QGraphicsEllipseItem()), 
  icon(new QGraphicsPixmapItem()),
  text(new QGraphicsTextItem()),
  row(0),
  column(0),
  colorIndex(0),
  lastVisibleState(VisibilityState::None)
{
  //All flags are disabled by default.
  this->rectangle->setFlags(QGraphicsItem::ItemIsSelectable);
  
  //set z values.
  this->rectangle->setZValue(-1000.0);
  this->point->setZValue(1000.0);
  this->icon->setZValue(0.0);
  this->text->setZValue(0.0);
}

const GraphLinkRecord& Model::findRecord(Vertex vertexIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByVertex>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByVertex>();
  List::const_iterator it = list.find(vertexIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Model::findRecord(const App::DocumentObject* dObjectIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByDObject>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByDObject>();
  List::const_iterator it = list.find(dObjectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Model::findRecord(const ViewProviderDocumentObject* VPDObjectIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByVPDObject>();
  List::const_iterator it = list.find(VPDObjectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Model::findRecord(const ViewEntryRectItem* rectIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByRectItem>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByRectItem>();
  List::const_iterator it = list.find(rectIn);
  assert(it != list.end());
  return *it;
}

const GraphLinkRecord& Model::findRecord(const std::string &stringIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByUniqueName>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByUniqueName>();
  List::const_iterator it = list.find(stringIn);
  assert(it != list.end());
  return *it;
}

void Model::eraseRecord(const ViewProviderDocumentObject* VPDObjectIn)
{
  typedef GraphLinkContainer::index<GraphLinkRecord::ByVPDObject>::type List;
  const List &list = graphLink->get<GraphLinkRecord::ByVPDObject>();
  List::iterator it = list.find(VPDObjectIn);
  assert(it != list.end());
  graphLink->get<GraphLinkRecord::ByVPDObject>().erase(it);
}

Model::Model(QObject *parentIn, const Gui::Document &documentIn) : QGraphicsScene(parentIn)
{
  //turned off BSP as it was giving inconsistent discovery of items
  //underneath cursor.
  this->setItemIndexMethod(QGraphicsScene::NoIndex);
  
  theGraph = std::shared_ptr<Graph>(new Graph());
  graphLink = std::shared_ptr<GraphLinkContainer>(new GraphLinkContainer());
  setupViewConstants();
  
  graphDirty = false;
  currentPrehighlight = nullptr;
  
  connectNewObject = documentIn.signalNewObject.connect(boost::bind(&Model::slotNewObject, this, _1));
  connectDelObject = documentIn.signalDeletedObject.connect(boost::bind(&Model::slotDeleteObject, this, _1));
  connectChgObject = documentIn.signalChangedObject.connect(boost::bind(&Model::slotChangeObject, this, _1, _2));
}

Model::~Model()
{
  if (connectNewObject.connected())
    connectNewObject.disconnect();
  if (connectDelObject.connected())
    connectDelObject.disconnect();
  if (connectChgObject.connected())
    connectChgObject.disconnect();
  
  removeAllItems();
}

void Model::setupViewConstants()
{
  QFontMetrics fontMetric(qApp->font());
  fontHeight = fontMetric.height();
  verticalSpacing = 1.0;
  rowHeight = (fontHeight + 2.0 * verticalSpacing) * -1.0; //pixel space top and bottom.
  iconSize = fontHeight;
  pointSize = fontHeight / 2.0;
  pointSpacing = pointSize;
  pointToIcon = iconSize;
  rowPadding = fontHeight;
  backgroundBrushes = {qApp->palette().base(), qApp->palette().alternateBase()};
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
  this->addItem((*theGraph)[virginVertex].rectangle.get());
  this->addItem((*theGraph)[virginVertex].point.get());
  this->addItem((*theGraph)[virginVertex].icon.get());
  this->addItem((*theGraph)[virginVertex].text.get());
  
  GraphLinkRecord virginRecord;
  virginRecord.DObject = VPDObjectIn.getObject();
  virginRecord.VPDObject = &VPDObjectIn;
  virginRecord.rectItem = (*theGraph)[virginVertex].rectangle.get();
  virginRecord.uniqueName = std::string(virginRecord.DObject->getNameInDocument());
  virginRecord.vertex = virginVertex;
  graphLink->insert(virginRecord);
  
  //construct pixmaps.
  QIcon baseIcon = VPDObjectIn.getIcon();
  (*theGraph)[virginVertex].pixmapEnabled = baseIcon.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  (*theGraph)[virginVertex].pixmapDisabled = baseIcon.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  
  //setup rectangle.
  auto *rectangle = (*theGraph)[virginVertex].rectangle.get();
  rectangle->setPen(Qt::NoPen);
  QColor preSelectionColor = qApp->palette().highlight().color();
  preSelectionColor.setAlphaF(0.25);
  rectangle->setPreselectionBrush(QBrush(preSelectionColor));
  rectangle->setSelectionBrush(qApp->palette().highlight());
  QColor bothSelectionColor = qApp->palette().highlight().color();
  bothSelectionColor.setAlphaF(0.75);
  rectangle->setBothBrush(QBrush(bothSelectionColor));
  
  //setup point.
  auto *point = (*theGraph)[virginVertex].point.get();
  point->setPen(Qt::NoPen);
  
  graphDirty = true;
}

void Model::slotDeleteObject(const ViewProviderDocumentObject &VPDObjectIn)
{
  Vertex vertex = findRecord(&VPDObjectIn).vertex;
  
  //remove items from scene.
  this->removeItem((*theGraph)[vertex].rectangle.get());
  this->removeItem((*theGraph)[vertex].point.get());
  this->removeItem((*theGraph)[vertex].icon.get());
  this->removeItem((*theGraph)[vertex].text.get());
  
  //remove connector items 
  auto outRange = boost::out_edges(vertex, *theGraph);
  for (auto outEdgeIt = outRange.first; outEdgeIt != outRange.second; ++outEdgeIt)
    this->removeItem((*theGraph)[*outEdgeIt].connector.get());
  auto inRange = boost::in_edges(vertex, *theGraph);
  for (auto inEdgeIt = inRange.first; inEdgeIt != inRange.second; ++inEdgeIt)
    this->removeItem((*theGraph)[*inEdgeIt].connector.get());
  
  //remove the actual vertex.
  boost::clear_vertex(vertex, *theGraph);
  boost::remove_vertex(vertex, *theGraph);
  
  eraseRecord(&VPDObjectIn);
  graphDirty = true;
}

void Model::slotChangeObject(const ViewProviderDocumentObject &VPDObjectIn, const App::Property& propertyIn)
{
  std::string name("Empty Name");
  if (propertyIn.getName()) //getName can return 0.
    name = propertyIn.getName();
  assert(!name.empty());
  
//   std::cout << std::endl << "inside changed object." << std::endl << 
//     "Property name is: " <<  name << std::endl <<
//     "Property type is: " << propertyIn.getTypeId().getName() << std::endl << std::endl;
  
  //renaming of objects.
  if (std::string("Label") == name)
  {
    const GraphLinkRecord &record = findRecord(&VPDObjectIn);
    auto *text = (*theGraph)[record.vertex].text.get();
    text->setPlainText(QString::fromUtf8(record.DObject->Label.getValue()));
  }
  
  //link changes. these require a recalculation of connectors.
  const static std::unordered_set<std::string> linkTypes =
  {
    "App::PropertyLink",
    "App::PropertyLinkList",
    "App::PropertyLinkSub",
    "App::PropertyLinkSubList"
  };
  
  if (linkTypes.find(propertyIn.getTypeId().getName()) != linkTypes.end())
  {
    const GraphLinkRecord &record = findRecord(&VPDObjectIn);
    boost::clear_vertex(record.vertex, *theGraph);
    graphDirty = true;
  }
}

void Model::selectionChanged(const SelectionChanges& msg)
{
  //note that treeview uses set selection which sends a message with just a document name
  //and no object name. Have to explore further.
  
  //lamda for clearing selections.
  auto clearSelection = [this]()
  {
    BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
    {
      ViewEntryRectItem *rect = (*theGraph)[currentVertex].rectangle.get();
      assert(rect);
      rect->selectionOff();
    }
  };
  
  //lamda for getting rectangle.
  auto getRectangle = [this](const char *in)
  {
    assert(in);
    std::string name(in);
    assert(!name.empty());
    const GraphLinkRecord &record = findRecord(name);
    ViewEntryRectItem *rect = (*theGraph)[record.vertex].rectangle.get();
    assert(rect);
    return rect;
  };
  
  if (msg.Type == SelectionChanges::AddSelection)
  {
    if (msg.pObjectName)
      getRectangle(msg.pObjectName)->selectionOn();
  }
  else if(msg.Type == SelectionChanges::RmvSelection)
  {
    if (msg.pObjectName)
      getRectangle(msg.pObjectName)->selectionOff();
  }
  else if(msg.Type == SelectionChanges::SetSelection)
  {
    clearSelection();
    
    auto selections = Gui::Selection().getSelection(msg.pDocName);
    for (const auto &selection : selections)
    {
      assert(selection.FeatName);
      getRectangle(selection.FeatName)->selectionOn();
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
  updateVisible();
}

void Model::updateSlot()
{
  Base::TimeInfo startTime;
  
  //here we will cycle through the graph updating edges.
  //empty outList means it is a root.
  //empty inList means it is a leaf.
  
  BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
  {
    bool foundFirst = false; //temp hack.
#if 0
    //based on claim children. don't think this will be enough.
    const auto *VPDObject = findRecord(currentVertex).VPDObject;
    auto children = VPDObject->claimChildren();
    for (const auto *currentChild : children)
    {
      Vertex otherVertex = findRecord(currentChild).vertex;
      bool result;
      Edge edge;
      boost::tie(edge, result) = boost::add_edge(currentVertex, otherVertex, *theGraph);
      if (result)
      {
        if (!foundFirst)
        {
          (*theGraph)[edge].relation = EdgeProperty::BranchTag::Continue;
          foundFirst = true;
        }
        else
          (*theGraph)[edge].relation = EdgeProperty::BranchTag::Terminate;
        
        (*theGraph)[edge].connector = std::shared_ptr<QGraphicsPathItem>(new QGraphicsPathItem());
        (*theGraph)[edge].connector->setZValue(0.0);
        this->addItem((*theGraph)[edge].connector.get());
      }
    }
#else
    //based on outlist. this won't be enough either.
    const App::DocumentObject *currentDObject = findRecord(currentVertex).DObject;
    std::vector<App::DocumentObject *> otherDObjects = currentDObject->getOutList();
    for (auto &currentOtherDObject : otherDObjects)
    {
      Vertex otherVertex = findRecord(currentOtherDObject).vertex;
      bool result;
      Edge edge;
      boost::tie(edge, result) = boost::add_edge(currentVertex, otherVertex, *theGraph);
      if (result)
      {
        if (!foundFirst)
        {
          (*theGraph)[edge].relation = EdgeProperty::BranchTag::Continue;
          foundFirst = true;
        }
        else
          (*theGraph)[edge].relation = EdgeProperty::BranchTag::Terminate;
        
        (*theGraph)[edge].connector = std::shared_ptr<QGraphicsPathItem>(new QGraphicsPathItem());
        (*theGraph)[edge].connector->setZValue(0.0);
        this->addItem((*theGraph)[edge].connector.get());
      }
    }
#endif
  }
  
  indexVerticesEdges();
  Path sorted;
  boost::topological_sort(*theGraph, std::back_inserter(sorted));
  int currentRow = 0;
  int currentColumn = -1; //we know the first one will be a root so we can assume it will get kicked up to 0.
  int maxColumn = currentColumn; //used for determining offset of icons and text.
  float maxTextLength = 0;
  for (const auto &currentVertex : sorted)
  {
    if (boost::out_degree(currentVertex, *theGraph) == 0)
      currentColumn++;
    else
    {
      bool foundTarget = false;
      OutEdgeIterator it, itEnd;
      boost::tie(it, itEnd) = boost::out_edges(currentVertex, *theGraph);
      for (;it != itEnd; ++it)
      {
        if ((*theGraph)[*it].relation == EdgeProperty::BranchTag::Continue)
        {
          Vertex target = boost::target(*it, *theGraph);
          currentColumn = (*theGraph)[target].column;
          foundTarget = true;
          break;
        }
      }
      if (!foundTarget)
        currentColumn++;
    }
    
    maxColumn = std::max(currentColumn, maxColumn);
    QBrush currentBrush(forgroundBrushes.at(currentColumn % forgroundBrushes.size()));
    
    auto *rectangle = (*theGraph)[currentVertex].rectangle.get();
    rectangle->setRect(-rowPadding, 0.0, rowPadding, rowHeight); //calculate actual length later.
    rectangle->setTransform(QTransform::fromTranslate(0, rowHeight * currentRow));
    rectangle->setBackgroundBrush(backgroundBrushes[currentRow % backgroundBrushes.size()]);
    
    auto *point = (*theGraph)[currentVertex].point.get();
    point->setRect(0.0, 0.0, -pointSize, -pointSize);
    point->setTransform(QTransform::fromTranslate(pointSpacing * currentColumn,
                                                  rowHeight * currentRow + rowHeight / 2.0 + pointSize / 2.0));
    point->setBrush(currentBrush);
    
    auto *pixmap = (*theGraph)[currentVertex].icon.get();
    pixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + rowHeight)); //calculate x location later.
    
    auto *text = (*theGraph)[currentVertex].text.get();
    text->setPlainText(QString::fromUtf8(findRecord(currentVertex).DObject->Label.getValue()));
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate
      (0.0, rowHeight * currentRow + rowHeight - verticalSpacing * 2.0)); //calculate x location later.
    (*theGraph)[currentVertex].lastVisibleState = VisibilityState::None; //force visual update for color.
    
    //store column and row int the graph. use for connectors later.
    (*theGraph)[currentVertex].row = currentRow;
    (*theGraph)[currentVertex].column = currentColumn;
    (*theGraph)[currentVertex].colorIndex = currentColumn % forgroundBrushes.size();
    
    //our list is topo sorted so all dependents should be located, so we can build the connectors.
    //will have some more logic for connector path, simple for now.
    float currentX = pointSpacing * currentColumn - pointSize / 2.0;
    float currentY = rowHeight * currentRow + rowHeight / 2.0;
    OutEdgeIterator it, itEnd;
    boost::tie(it, itEnd) = boost::out_edges(currentVertex, *theGraph);
    for (; it != itEnd; ++it)
    {
      Vertex target = boost::target(*it, *theGraph);
      float dependentX = pointSpacing * (*theGraph)[target].column - pointSize / 2.0; //on center.
      float dependentY = rowHeight * (*theGraph)[target].row + rowHeight / 2.0;
      
      QGraphicsPathItem *pathItem = (*theGraph)[*it].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentX, currentY);
      if (currentColumn != (*theGraph)[target].column)
        path.lineTo(dependentX, currentY);
      path.lineTo(dependentX, dependentY); //y is always different.
      pathItem->setPath(path);
    }
    
    currentRow++;
  }
  
  float columnSpacing = (maxColumn * pointSpacing);
  for (const auto &currentVertex : sorted)
  {
    auto *pixmap = (*theGraph)[currentVertex].icon.get();
    QTransform iconTransform = QTransform::fromTranslate(columnSpacing + pointToIcon, 0.0);
    pixmap->setTransform(pixmap->transform() * iconTransform);
    
    auto *text = (*theGraph)[currentVertex].text.get();
    QTransform textTransform = QTransform::fromTranslate(columnSpacing  + pointToIcon + iconSize, 0.0);
    text->setTransform(text->transform() * textTransform);
    
    auto *rectangle = (*theGraph)[currentVertex].rectangle.get();
    QRectF rect = rectangle->rect();
    rect.setWidth(columnSpacing  + pointToIcon + iconSize + maxTextLength + 2.0 * rowPadding);
    rectangle->setRect(rect);
  }
  
  //Modeling_Challenge_Casting_ta4 with 59 features: "Initialize DAG View time: 0.007"
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
    BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
    {
      this->removeItem((*theGraph)[currentVertex].rectangle.get());
      this->removeItem((*theGraph)[currentVertex].point.get());
      this->removeItem((*theGraph)[currentVertex].icon.get());
      this->removeItem((*theGraph)[currentVertex].text.get());
    }
      
    BGL_FORALL_EDGES_T(currentEdge, *theGraph, Graph)
      this->removeItem((*theGraph)[currentEdge].connector.get());
  }
}

void Model::updateVisible()
{
  //not sure I want to use the same pixmap merge for failing feature icons.
  //thinking maybe red background or another column of icons for state?
  
  BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
  {
    const GraphLinkRecord &record = findRecord(currentVertex);
    auto *text = (*theGraph)[currentVertex].text.get();
    auto *pixmap = (*theGraph)[currentVertex].icon.get();
    QIcon baseIcon = record.VPDObject->getIcon();
    VisibilityState currentState = (record.VPDObject->isShow()) ? (VisibilityState::On) : (VisibilityState::Off);
    if
    (
      (currentState != (*theGraph)[currentVertex].lastVisibleState) ||
      ((*theGraph)[currentVertex].lastVisibleState == VisibilityState::None)
    )
    {
      if (record.VPDObject->isShow())
      {
        text->setDefaultTextColor(forgroundBrushes.at((*theGraph)[currentVertex].colorIndex).color());
        pixmap->setPixmap((*theGraph)[currentVertex].pixmapEnabled);
      }
      else
      {
        text->setDefaultTextColor(qApp->palette().color(QPalette::Disabled, QPalette::Text));
        pixmap->setPixmap((*theGraph)[currentVertex].pixmapDisabled);
      }
      (*theGraph)[currentVertex].lastVisibleState = currentState;
    }
  }
}

ViewEntryRectItem* Model::getRectFromPosition(const QPointF& position)
{
  ViewEntryRectItem *rect = nullptr;
  auto theItems = this->items(position, Qt::IntersectsItemBoundingRect, Qt::DescendingOrder);
  for (auto *currentItem : theItems)
  {
    rect = dynamic_cast<ViewEntryRectItem *>(currentItem);
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
  
  ViewEntryRectItem *rect = getRectFromPosition(event->scenePos());
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
  ViewEntryRectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    const App::DocumentObject *dObject = findRecord(rect).DObject;
    Gui::Selection().addSelection(dObject->getDocument()->getName(), dObject->getNameInDocument());
  }
  
  //need an else here to clear the selections.
  //don't have current selection stored yet.
  
  QGraphicsScene::mousePressEvent(event);
}




#include <moc_DAGModel.cpp>
