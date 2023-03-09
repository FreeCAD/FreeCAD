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

#ifndef DAGMODEL_H
#define DAGMODEL_H

#include <memory>
#include <vector>

#include <boost_signals2.hpp>

#include <QBrush>
#include <QGraphicsScene>
#include <QLineEdit>

#include "DAGFilter.h"
#include "DAGModelGraph.h"
#include "DAGRectItem.h"


class QGraphicsSceneHoverEvent;
class QGraphicsProxyWidget;

namespace Gui
{
  class Document;
  class ViewProviderDocumentObject;
  class SelectionChanges;

  namespace DAG
  {
    class LineEdit : public QLineEdit
    {
    Q_OBJECT
    public:
      explicit LineEdit(QWidget *parentIn = nullptr);
    Q_SIGNALS:
      void acceptedSignal();
      void rejectedSignal();
    protected:
    void keyPressEvent(QKeyEvent*) override;
    };

    class Model : public QGraphicsScene
    {
      Q_OBJECT
    public:
      Model(QObject *parentIn, const Gui::Document &documentIn);
      ~Model() override;
      void awake(); //!< hooked up to event dispatcher for update when idle.
      void selectionChanged(const SelectionChanges& msg);

    protected:
      void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
      void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
      void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
      void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;

    private Q_SLOTS:
      void updateSlot();
      void onRenameSlot();
      void renameAcceptedSlot();
      void renameRejectedSlot();
      void editingStartSlot();
      void editingFinishedSlot();

    private:
      Model(){}
      //documentObject slots.
      using Connection = boost::signals2::connection;
      Connection connectNewObject;
      Connection connectDelObject;
      Connection connectChgObject;
      Connection connectRenObject;
      Connection connectActObject;
      Connection connectEdtObject;
      Connection connectResObject;
      Connection connectHltObject;
      Connection connectExpObject;
      void slotNewObject(const Gui::ViewProviderDocumentObject &VPDObjectIn);
      void slotDeleteObject(const Gui::ViewProviderDocumentObject &VPDObjectIn);
      void slotChangeObject(const Gui::ViewProviderDocumentObject &VPDObjectIn, const App::Property& propertyIn);
      void slotInEdit(const Gui::ViewProviderDocumentObject &VPDObjectIn);
      void slotResetEdit(const Gui::ViewProviderDocumentObject &VPDObjectIn);
      void slotChangeIcon(const Gui::ViewProviderDocumentObject &VPDObjectIn, std::shared_ptr<QGraphicsPixmapItem> icon);

      std::shared_ptr<GraphLinkContainer> graphLink;
      std::shared_ptr<Graph> theGraph;
      bool graphDirty;

      void indexVerticesEdges();
      void removeAllItems();
      void addVertexItemsToScene(const Vertex &vertexIn);
      void removeVertexItemsFromScene(const Vertex &vertexIn);
      void updateStates();
      std::size_t columnFromMask(const ColumnMask&);

      RectItem* getRectFromPosition(const QPointF &position); //!< can be nullptr

    //! @name View Constants for spacing
    //@{
      //!< height of the current qApp default font.
      float fontHeight;
      //!< controls top to bottom or bottom to top direction.
      float direction;
      //!< pixels between top and bottom of text to background rectangle.
      float verticalSpacing;
      //!< height of background rectangle.
      float rowHeight;
      //!< size of icon to match font.
      float iconSize;
      //!< size of the connection point.
      float pointSize;
      //!< spacing between pofloat columns.
      float pointSpacing;
      //!< spacing from last column points to first icon.
      float pointToIcon;
      //!< spacing between icons.
      float iconToIcon;
      //!< spacing between last icon and text.
      float iconToText;
      //!< spaces added to rectangle background width ends.
      float rowPadding;
      //!< brushes to paint background rectangles.
      std::vector<QBrush> backgroundBrushes;
      //!< brushes to paint points, connectors, text.
      std::vector<QBrush> forgroundBrushes;
      void setupViewConstants();
    //@}

      RectItem *currentPrehighlight;

      enum class SelectionMode
      {
        Single,
        Multiple
      };
      SelectionMode selectionMode;
      std::vector<Vertex> getAllSelected();
      //!< hide any connected feature and turn on sourceIn.
      void visiblyIsolate(Vertex sourceIn);

      QPointF lastPick;
      bool lastPickValid = false;

      QPixmap visiblePixmapEnabled;
      QPixmap visiblePixmapDisabled;
      QPixmap passPixmap;
      QPixmap failPixmap;
      QPixmap pendingPixmap;
      //!< needed because python objects are not ready.
      Vertex lastAddedVertex = Graph::null_vertex();

      QAction *renameAction;
      QAction *editingFinishedAction;
      QGraphicsProxyWidget *proxy = nullptr;
      void finishRename();

      //filters
      void setupFilters();
      using FilterContainer = std::vector<std::shared_ptr<FilterBase> >;
      FilterContainer filters;
    };
  }
}

#endif // DAGMODEL_H
