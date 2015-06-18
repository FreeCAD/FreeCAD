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

#include <QApplication>
#include <QString>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneHoverEvent>
#include <QGraphicsProxyWidget>
#include <QPen>
#include <QBrush>
#include <QColor>
#include <QPainter>
#include <QKeyEvent>
#include <QMenu>
#include <QTimer>
#endif

#include <QAbstractEventDispatcher>

#include <deque>
#include <unordered_set>

#include <Base/TimeInfo.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>
#include <Gui/Selection.h>
#include <Gui/BitmapFactory.h>
#include <Gui/MenuManager.h>
#include <Gui/MainWindow.h>

#include "DAGModel.h"

using namespace Gui;
using namespace DAG;

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


ViewEntryRectItem::ViewEntryRectItem(QGraphicsItem* parent) : QGraphicsRectItem(parent)
{
  selected = false;
  preSelected = false;
  editing = false;
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
  if (editing)
    brush = editBrush;
  
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
  visibleIcon(new QGraphicsPixmapItem()),
  stateIcon(new QGraphicsPixmapItem()),
  icon(new QGraphicsPixmapItem()),
  text(new QGraphicsTextItem()),
  row(0),
  column(0),
  lastVisibleState(VisibilityState::None)
{
  //All flags are disabled by default.
  this->rectangle->setFlags(QGraphicsItem::ItemIsSelectable);
  
  //set z values.
  this->rectangle->setZValue(-1000.0);
  this->point->setZValue(1000.0);
  this->visibleIcon->setZValue(0.0);
  this->stateIcon->setZValue(0.0);
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
  
  ParameterGrp::handle group = App::GetApplication().GetUserParameter().
          GetGroup("BaseApp")->GetGroup("Preferences")->GetGroup("DAGView");
    selectionMode = static_cast<SelectionMode>(group->GetInt("SelectionMode", 0));
    group->SetInt("SelectionMode", static_cast<int>(selectionMode)); //ensure entry exists.
    
  QIcon temp(Gui::BitmapFactory().pixmap("dagViewVisible"));
  visiblePixmapEnabled = temp.pixmap(iconSize, iconSize, QIcon::Normal, QIcon::On);
  visiblePixmapDisabled = temp.pixmap(iconSize, iconSize, QIcon::Disabled, QIcon::Off);
  
  QIcon passIcon(Gui::BitmapFactory().pixmap("dagViewPass"));
  passPixmap = passIcon.pixmap(iconSize, iconSize);
  QIcon failIcon(Gui::BitmapFactory().pixmap("dagViewFail"));
  failPixmap = failIcon.pixmap(iconSize, iconSize);
  
  renameAction = new QAction(this);
  renameAction->setText(tr("Rename"));
  renameAction->setStatusTip(tr("Rename object"));
  renameAction->setShortcut(Qt::Key_F2);
  connect(renameAction, SIGNAL(triggered()), this, SLOT(onRenameSlot()));
  
  editingFinishedAction = new QAction(this);
  editingFinishedAction->setText(tr("Finish editing"));
  editingFinishedAction->setStatusTip(tr("Finish editing object"));
  connect(this->editingFinishedAction, SIGNAL(triggered()),
          this, SLOT(editingFinishedSlot()));
  
  connectNewObject = documentIn.signalNewObject.connect(boost::bind(&Model::slotNewObject, this, _1));
  connectDelObject = documentIn.signalDeletedObject.connect(boost::bind(&Model::slotDeleteObject, this, _1));
  connectChgObject = documentIn.signalChangedObject.connect(boost::bind(&Model::slotChangeObject, this, _1, _2));
  connectEdtObject = documentIn.signalInEdit.connect(boost::bind(&Model::slotInEdit, this, _1));
  connectResObject = documentIn.signalResetEdit.connect(boost::bind(&Model::slotResetEdit, this, _1));
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
  iconToIcon = iconSize * 0.25;
  iconToText = iconSize / 2.0;
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
  this->addItem((*theGraph)[virginVertex].visibleIcon.get());
  this->addItem((*theGraph)[virginVertex].stateIcon.get());
  this->addItem((*theGraph)[virginVertex].icon.get());
  this->addItem((*theGraph)[virginVertex].text.get());
  
  GraphLinkRecord virginRecord;
  virginRecord.DObject = VPDObjectIn.getObject();
  virginRecord.VPDObject = &VPDObjectIn;
  virginRecord.rectItem = (*theGraph)[virginVertex].rectangle.get();
  virginRecord.uniqueName = std::string(virginRecord.DObject->getNameInDocument());
  virginRecord.vertex = virginVertex;
  graphLink->insert(virginRecord);
  
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
  rectangle->setEditingBrush(QBrush(Qt::yellow));
  
  (*theGraph)[virginVertex].icon->setPixmap(VPDObjectIn.getIcon().pixmap(iconSize, iconSize));
  (*theGraph)[virginVertex].stateIcon->setPixmap(passPixmap);
  
  graphDirty = true;
}

void Model::slotDeleteObject(const ViewProviderDocumentObject &VPDObjectIn)
{
  Vertex vertex = findRecord(&VPDObjectIn).vertex;
  
  //remove items from scene.
  this->removeItem((*theGraph)[vertex].rectangle.get());
  this->removeItem((*theGraph)[vertex].point.get());
  this->removeItem((*theGraph)[vertex].visibleIcon.get());
  this->removeItem((*theGraph)[vertex].stateIcon.get());
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

void Model::slotInEdit(const ViewProviderDocumentObject& VPDObjectIn)
{
  ViewEntryRectItem *rect = (*theGraph)[findRecord(&VPDObjectIn).vertex].rectangle.get();
  rect->editingStart();
  this->invalidate();
}

void Model::slotResetEdit(const ViewProviderDocumentObject& VPDObjectIn)
{
  ViewEntryRectItem *rect = (*theGraph)[findRecord(&VPDObjectIn).vertex].rectangle.get();
  rect->editingFinished();
  this->invalidate();
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
  updateStates();
}

void Model::updateSlot()
{
  Base::TimeInfo startTime;
  
  //here we will cycle through the graph updating edges.
  //empty outList means it is a root.
  //empty inList means it is a leaf.
  
  BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
  {
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
        (*theGraph)[edge].connector = std::shared_ptr<QGraphicsPathItem>(new QGraphicsPathItem());
        (*theGraph)[edge].connector->setZValue(0.0);
        this->addItem((*theGraph)[edge].connector.get());
      }
    }
  }
  
  indexVerticesEdges();
  Path sorted;
  boost::topological_sort(*theGraph, std::back_inserter(sorted));
  //index the vertices in sort order.
  int tempIndex = 0;
  for (const auto &currentVertex : sorted)
  {
    (*theGraph)[currentVertex].topoSortIndex = tempIndex;
    tempIndex++;
  }
  
  int currentRow = 0;
  int currentColumn = -1; //we know first column is going to be root so will be kicked up to 0.
  int maxColumn = currentColumn; //used for determining offset of icons and text.
  float maxTextLength = 0;
  for (const auto &currentVertex : sorted)
  {
//     std::cout << std::endl << std::endl;
    
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
        Vertex target = boost::target(*it, *theGraph);
        parentVertices.push_back(target);
        int currentParentIndex = (*theGraph)[target].topoSortIndex;
        if (currentParentIndex < farthestParentIndex)
        {
          Path::const_iterator start = sorted.begin() + currentParentIndex + 1; // 1 after
          Path::const_iterator end = sorted.begin() + (*theGraph)[currentVertex].topoSortIndex; // 1 before
          Path::const_iterator it;
          for (it = start; it != end; ++it)
            columnMask |= (*theGraph)[*it].column;
          farthestParentIndex = currentParentIndex;
        }
      }
      
//       std::cout << "mask for " << findRecord(currentVertex).DObject->Label.getValue() << "      " <<
//         columnMask.to_string() << std::endl;
      
      //now we should have a mask representing the columns that are being used.
      //this is from the lowest parent, in the topo sort, to last entry.
      //try to use the same column as one of the parents.
      int destinationColumn = maxColumn + 1; //default to new column
      for (const auto &currentParent : parentVertices)
      {
        if (((*theGraph)[currentParent].column & columnMask).none())
        {
          //go with first parent for now.
          destinationColumn = static_cast<int>(std::log2((*theGraph)[currentParent].column.to_ulong()));
          break;
        }
      }

      currentColumn = destinationColumn;
    }
    
    assert(currentColumn < static_cast<int>(ColumnMask().size())); //temp limitation.
    
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
    
    auto *visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
    visiblePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + rowHeight)); //calculate x location later.
    
    auto *statePixmap = (*theGraph)[currentVertex].stateIcon.get();
    statePixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + rowHeight)); //calculate x location later.
    
    auto *pixmap = (*theGraph)[currentVertex].icon.get();
    pixmap->setTransform(QTransform::fromTranslate(0.0, rowHeight * currentRow + rowHeight)); //calculate x location later.
    
    auto *text = (*theGraph)[currentVertex].text.get();
    text->setPlainText(QString::fromUtf8(findRecord(currentVertex).DObject->Label.getValue()));
    text->setDefaultTextColor(currentBrush.color());
    maxTextLength = std::max(maxTextLength, static_cast<float>(text->boundingRect().width()));
    text->setTransform(QTransform::fromTranslate
      (0.0, rowHeight * currentRow + rowHeight - verticalSpacing * 2.0)); //calculate x location later.
    (*theGraph)[currentVertex].lastVisibleState = VisibilityState::None; //force visual update for color.
    
    //store column and row int the graph. use for connectors later.
    (*theGraph)[currentVertex].row = currentRow;
    (*theGraph)[currentVertex].column.reset().set((currentColumn));
    
    //our list is topo sorted so all dependents should be located, so we can build the connectors.
    //will have some more logic for connector path, simple for now.
    float currentX = pointSpacing * currentColumn - pointSize / 2.0;
    float currentY = rowHeight * currentRow + rowHeight / 2.0;
    OutEdgeIterator it, itEnd;
    boost::tie(it, itEnd) = boost::out_edges(currentVertex, *theGraph);
    for (; it != itEnd; ++it)
    {
      Vertex target = boost::target(*it, *theGraph);
      float dependentX = pointSpacing * static_cast<int>(std::log2((*theGraph)[target].column.to_ulong())) - pointSize / 2.0; //on center.
      float dependentY = rowHeight * (*theGraph)[target].row + rowHeight / 2.0;
      
      QGraphicsPathItem *pathItem = (*theGraph)[*it].connector.get();
      pathItem->setBrush(Qt::NoBrush);
      QPainterPath path;
      path.moveTo(currentX, currentY);
      if (currentColumn == static_cast<int>(std::log2((*theGraph)[target].column.to_ulong())))
        path.lineTo(currentX, dependentY); //straight connector in y.
      else
      {
        //connector with bend.
        float radius = pointSpacing / 1.9; //no zero length line.
        
        path.lineTo(currentX, dependentY - radius);
      
        float yPosition = dependentY - 2.0 * radius;
        float width = 2.0 * radius;
        float height = width;
        if (dependentX > currentX) //radius to the right.
        {
          QRectF arcRect(currentX, yPosition, width, height);
          path.arcTo(arcRect, 180.0, 90.0);
        }
        else //radius to the left.
        {
          QRectF arcRect(currentX - 2.0 * radius, yPosition, width, height);
          path.arcTo(arcRect, 0.0, -90.0);
        }
        path.lineTo(dependentX, dependentY);
      }
      pathItem->setPath(path);
    }
    
    currentRow++;
  }
  
  float columnSpacing = (maxColumn * pointSpacing);
  for (const auto &currentVertex : sorted)
  {
    float currentX = columnSpacing;
    currentX += pointToIcon;
    auto *visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
    QTransform visibleIconTransform = QTransform::fromTranslate(currentX, 0.0);
    visiblePixmap->setTransform(visiblePixmap->transform() * visibleIconTransform);
    
    currentX += iconSize + iconToIcon;
    auto *statePixmap = (*theGraph)[currentVertex].stateIcon.get();
    QTransform stateIconTransform = QTransform::fromTranslate(currentX, 0.0);
    statePixmap->setTransform(statePixmap->transform() * stateIconTransform);
    
    currentX += iconSize + iconToIcon;
    auto *pixmap = (*theGraph)[currentVertex].icon.get();
    QTransform iconTransform = QTransform::fromTranslate(currentX, 0.0);
    pixmap->setTransform(pixmap->transform() * iconTransform);
    
    currentX += iconSize + iconToText;
    auto *text = (*theGraph)[currentVertex].text.get();
    QTransform textTransform = QTransform::fromTranslate(currentX, 0.0);
    text->setTransform(text->transform() * textTransform);
    
    auto *rectangle = (*theGraph)[currentVertex].rectangle.get();
    QRectF rect = rectangle->rect();
    rect.setWidth(currentX + maxTextLength + 2.0 * rowPadding);
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

void Model::updateStates()
{
  //not sure I want to use the same pixmap merge for failing feature icons.
  //thinking maybe red background or another column of icons for state?
  
  BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
  {
    const GraphLinkRecord &record = findRecord(currentVertex);
    
    auto *visiblePixmap = (*theGraph)[currentVertex].visibleIcon.get();
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
    
    FeatureState currentFeatureState = (record.DObject->isError()) ? FeatureState::Fail : FeatureState::Pass;
    if (currentFeatureState != (*theGraph)[currentVertex].lastFeatureState)
    {
      if (currentFeatureState == FeatureState::Pass)
      {
        (*theGraph)[currentVertex].stateIcon->setPixmap(passPixmap);
        (*theGraph)[currentVertex].stateIcon->setToolTip(QString());
      }
      else
      {
        (*theGraph)[currentVertex].stateIcon->setPixmap(failPixmap);
        (*theGraph)[currentVertex].stateIcon->setToolTip(QString::fromAscii(record.DObject->getStatusString()));
      }
      (*theGraph)[currentVertex].lastFeatureState = currentFeatureState;
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
  auto goShiftSelect = [this, event]()
  {
    QPointF currentPickPoint = event->scenePos();
    QGraphicsLineItem intersectionLine(QLineF(lastPick, currentPickPoint));
    QList<QGraphicsItem *>selection = collidingItems(&intersectionLine);
    for (auto currentItem = selection.begin(); currentItem != selection.end(); ++currentItem)
    {
      ViewEntryRectItem *rect = dynamic_cast<ViewEntryRectItem *>(*currentItem);
      if (!rect) continue;
      const GraphLinkRecord &selectionRecord = findRecord(rect);
      Gui::Selection().addSelection(selectionRecord.DObject->getDocument()->getName(),
                                    selectionRecord.DObject->getNameInDocument());
    }
  };
  
  auto toggleSelect = [](const App::DocumentObject *dObjectIn, ViewEntryRectItem *rectIn)
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
    ViewEntryRectItem *rect = getRectFromPosition(event->scenePos());
    if (rect)
    {
        const GraphLinkRecord &record = findRecord(rect);
        
        //don't like that I am doing this again here after getRectFromPosition call.
        QGraphicsItem *item = itemAt(event->scenePos());
        QGraphicsPixmapItem *pixmapItem = dynamic_cast<QGraphicsPixmapItem *>(item);
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
  auto selections = getAllSelected();
  assert(selections.size() == 1);
  const GraphLinkRecord &record = findRecord(selections.front());
  Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
  MDIView *view = doc->getActiveView();
  if (view)
    getMainWindow()->setActiveWindow(view);
  const_cast<ViewProviderDocumentObject*>(record.VPDObject)->doubleClicked();
  
  QGraphicsScene::mouseDoubleClickEvent(event);
}


std::vector<Gui::DAG::Vertex> Model::getAllSelected()
{
  std::vector<Gui::DAG::Vertex> out;
  
  BGL_FORALL_VERTICES_T(currentVertex, *theGraph, Graph)
  {
    if ((*theGraph)[currentVertex].rectangle->isSelected())
      out.push_back(currentVertex);
  }
  
  return out;
}

void Model::contextMenuEvent(QGraphicsSceneContextMenuEvent* event)
{
  ViewEntryRectItem *rect = getRectFromPosition(event->scenePos());
  if (rect)
  {
    const GraphLinkRecord &record = findRecord(rect);
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
//   std::cout << std::endl << "inside rename slot" << std::endl << std::endl;
  
  assert(proxy == nullptr);
  std::vector<Gui::DAG::Vertex> selections = getAllSelected();
  assert(selections.size() == 1);
  
  LineEdit *lineEdit = new LineEdit();
  auto *text = (*theGraph)[selections.front()].text.get();
  lineEdit->setText(text->toPlainText());
  connect(lineEdit, SIGNAL(acceptedSignal()), this, SLOT(renameAcceptedSlot()));
  connect(lineEdit, SIGNAL(rejectedSignal()), this, SLOT(renameRejectedSlot()));
  
  proxy = this->addWidget(lineEdit);
  proxy->setGeometry(text->sceneBoundingRect());
  
  lineEdit->selectAll();
  QTimer::singleShot(0, lineEdit, SLOT(setFocus())); 
}

void Model::renameAcceptedSlot()
{
  assert(proxy);
  
  std::vector<Gui::DAG::Vertex> selections = getAllSelected();
  assert(selections.size() == 1);
  const GraphLinkRecord &record = findRecord(selections.front());
  
  LineEdit *lineEdit = dynamic_cast<LineEdit*>(proxy->widget());
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
  QAction* action = qobject_cast<QAction*>(sender());
  if (action)
  {
    int edit = action->data().toInt();
    auto selections = getAllSelected();
    assert(selections.size() == 1);
    const GraphLinkRecord &record = findRecord(selections.front());
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
  const GraphLinkRecord &record = findRecord(selections.front());
  Gui::Document* doc = Gui::Application::Instance->getDocument(record.DObject->getDocument());
  doc->commitCommand();
  doc->resetEdit();
  doc->getDocument()->recompute();
}



#include <moc_DAGModel.cpp>
