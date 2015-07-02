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
#include <bitset>

#include <boost/signal.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/iteration_macros.hpp>
#include <boost/graph/reverse_graph.hpp>
#include <boost/graph/topological_sort.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/breadth_first_search.hpp>

#include <QGraphicsScene>
#include <QGraphicsRectItem>
#include <QBrush>
#include <QLineEdit>

class QGraphicsSceneHoverEvent;
class QGraphicsProxyWidget;

namespace App{class DocumentObject;}

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
      LineEdit(QWidget *parentIn = 0);
    Q_SIGNALS:
      void acceptedSignal();
      void rejectedSignal();
    protected:
    virtual void keyPressEvent(QKeyEvent*);
    };
    
    /*all right I give up! the parenting combined with the zvalues is fubar!
     * you can't control any kind of layering between children of separate parents
     */
    class ViewEntryRectItem : public QGraphicsRectItem
    {
    public:
      ViewEntryRectItem(QGraphicsItem* parent = 0);
      void setBackgroundBrush(const QBrush &brushIn){backgroundBrush = brushIn;}
      void setPreselectionBrush(const QBrush &brushIn){preSelectionBrush = brushIn;}
      void setSelectionBrush(const QBrush &brushIn){selectionBrush = brushIn;}
      void setBothBrush(const QBrush &brushIn){bothBrush = brushIn;}
      void setEditingBrush(const QBrush &brushIn){editBrush = brushIn;}
      void preHighlightOn(){preSelected = true;}
      void preHighlightOff(){preSelected = false;}
      void selectionOn(){selected = true;}
      void selectionOff(){selected = false;}
      bool isSelected(){return selected;}
      bool isPreSelected(){return preSelected;}
      void editingStart(){editing = true;}
      void editingFinished(){editing = false;}
      bool isEditing(){return editing;}
    protected:
      virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);
    private:
      QBrush backgroundBrush; //!< brush used for background. not used yet.
      QBrush selectionBrush; //!< brush used when selected.
      QBrush preSelectionBrush; //!< brush used when pre selected.
      QBrush bothBrush; //!< brush for when both selected and preSelected.
      QBrush editBrush; //!< brush used when object is in edit mode.
      //start with booleans, may expand to state.
      bool selected;
      bool preSelected;
      bool editing;
    };
    
    enum class VisibilityState
    {
      None = 0, //<! not determined.
      On, //<! shown
      Off //<! hidden
    };
    
    enum class FeatureState
    {
      None = 0, //<! not determined.
      Pass,     //<! feature updated ok.
      Fail     //<! feature failed to update.
    };
    
    //limit of column width? boost::dynamic_bitset?
    //did a trial run with this set at 4096, not much difference.
    //going to leave a big number by default and see how it goes.
    typedef std::bitset<1024> ColumnMask;
    
    /*! @brief Graph vertex information
   *
   * My data stored for each vertex;
   */
    struct VertexProperty
    {
      VertexProperty();
      std::shared_ptr<ViewEntryRectItem> rectangle; //!< background
      std::shared_ptr<QGraphicsEllipseItem> point; //!< point
      std::shared_ptr<QGraphicsPixmapItem> visibleIcon; //!< visible Icon
      std::shared_ptr<QGraphicsPixmapItem> stateIcon; //!< visible Icon
      std::shared_ptr<QGraphicsPixmapItem> icon; //!< icon
      std::shared_ptr<QGraphicsTextItem> text; //!< text
      int row; //!< row for this entry.
      ColumnMask column; //!< column number containing the point.
      int topoSortIndex;
      VisibilityState lastVisibleState; //!< visibility test.
      FeatureState lastFeatureState; //!< feature state test.
      bool dagVisible; //!< should entry be visible in the DAG view.
    };
    /*! @brief boost data for each vertex.
     *
     * needed to create an internal index for vertex. needed for listS.
     * color is needed by some algorithms */
    typedef boost::property
    <
      boost::vertex_index_t, std::size_t,
      boost::property <boost::vertex_color_t, boost::default_color_type, VertexProperty>
    > vertex_prop;
    
    /*! @brief Graph edge information
    *
    * My data stored for each edge;
    */
    struct EdgeProperty
    {
      //! Feature relation meta data. Not used right now.
      enum class BranchTag
      {
        None = 0,       //!< not defined.
        Create,         //!< create a new branch.
        Continue,       //!< continue a branch.
        Terminate       //!< terminate a branch.
      };
      EdgeProperty();
      BranchTag relation;
      std::shared_ptr <QGraphicsPathItem> connector; //!< line representing link between nodes.
    };
    /*! @brief needed to create an internal index for graph edges. needed for setS.*/
    typedef boost::property<boost::edge_index_t, std::size_t, EdgeProperty> edge_prop;
    
    typedef boost::adjacency_list<boost::setS, boost::listS, boost::bidirectionalS, vertex_prop, edge_prop> Graph;
    typedef boost::graph_traits<Graph>::vertex_descriptor Vertex;
    typedef boost::graph_traits<Graph>::edge_descriptor Edge;
    typedef boost::graph_traits<Graph>::vertex_iterator VertexIterator;
    typedef boost::graph_traits<Graph>::edge_iterator EdgeIterator;
    typedef boost::graph_traits<Graph>::in_edge_iterator InEdgeIterator;
    typedef boost::graph_traits<Graph>::out_edge_iterator OutEdgeIterator;
    typedef boost::graph_traits<Graph>::adjacency_iterator VertexAdjacencyIterator;
    typedef boost::reverse_graph<Graph, Graph&> GraphReversed;
    typedef std::vector<Vertex> Path; //!< a path or any array of vertices
    
    template <class GraphEW>
    class Edge_writer {
    public:
      Edge_writer(const GraphEW &graphEWIn) : graphEW(graphEWIn) {}
      template <class EdgeW>
      void operator()(std::ostream& out, const EdgeW& edgeW) const
      {
        out << "[label=\"";
        out << "edge";
        out << "\"]";
      }
    private:
      const GraphEW &graphEW;
    };
    
    template <class GraphVW>
    class Vertex_writer {
    public:
      Vertex_writer(const GraphVW &graphVWIn) : graphVW(graphVWIn) {}
      template <class VertexW>
      void operator()(std::ostream& out, const VertexW& vertexW) const
      {
        out << "[label=\"";
        out << graphVW[vertexW].text->toPlainText().toAscii().data(); 
        out << "\"]";
      }
    private:
      const GraphVW &graphVW;
    };
    
    template <class GraphIn>
    void outputGraphviz(const GraphIn &graphIn, const std::string &filePath)
    {
      std::ofstream file(filePath.c_str());
      boost::write_graphviz(file, graphIn, Vertex_writer<GraphIn>(graphIn),
                            Edge_writer<GraphIn>(graphIn));
    }
    
    //! get all the leaves of the templated graph. Not used right now.
    template <class GraphIn>
    class RakeLeaves
    {
      typedef boost::graph_traits<Graph>::vertex_descriptor GraphInVertex;
      typedef std::vector<GraphInVertex> GraphInVertices;
    public:
      RakeLeaves(const GraphIn &graphIn) : graph(graphIn) {}
      GraphInVertices operator()() const
      {
        GraphInVertices out;
        BGL_FORALL_VERTICES_T(currentVertex, graph, GraphIn)
        {
          if (boost::out_degree(currentVertex, graph) == 0)
            out.push_back(currentVertex);
        }
        return out;
      }
    private:
      const GraphIn &graph;
    };
    
    //! get all the roots of the templated graph. Not used right now.
    template <class GraphIn>
    class DigRoots
    {
      typedef boost::graph_traits<Graph>::vertex_descriptor GraphInVertex;
      typedef std::vector<GraphInVertex> GraphInVertices;
    public:
      DigRoots(const GraphIn &graphIn) : graph(graphIn) {}
      GraphInVertices operator()() const
      {
        GraphInVertices out;
        BGL_FORALL_VERTICES_T(currentVertex, graph, GraphIn)
        {
          if (boost::in_degree(currentVertex, graph) == 0)
            out.push_back(currentVertex);
        }
        return out;
      }
    private:
      const GraphIn &graph;
    };
    
    /*! Multi_index record. */
    struct GraphLinkRecord
    {
      const App::DocumentObject *DObject; //!< document object
      const Gui::ViewProviderDocumentObject *VPDObject; //!< view provider
      const ViewEntryRectItem *rectItem; //!< qgraphics item.
      std::string uniqueName; //!< name for document object.
      Vertex vertex; //!< vertex in graph.
      
      //@{
      //! used as tags.
      struct ByDObject{};
      struct ByVPDObject{}; 
      struct ByRectItem{}; 
      struct ByUniqueName{};
      struct ByVertex{};
      //@}
    };
    
    namespace BMI = boost::multi_index;
    typedef boost::multi_index_container
    <
      GraphLinkRecord,
      BMI::indexed_by
      <
        BMI::ordered_unique
        <
          BMI::tag<GraphLinkRecord::ByDObject>,
          BMI::member<GraphLinkRecord, const App::DocumentObject*, &GraphLinkRecord::DObject>
        >,
        BMI::ordered_unique
        <
          BMI::tag<GraphLinkRecord::ByVPDObject>,
          BMI::member<GraphLinkRecord, const Gui::ViewProviderDocumentObject*, &GraphLinkRecord::VPDObject>
        >,
        BMI::ordered_unique
        <
          BMI::tag<GraphLinkRecord::ByRectItem>,
          BMI::member<GraphLinkRecord, const ViewEntryRectItem*, &GraphLinkRecord::rectItem>
        >,
        BMI::ordered_unique
        <
          BMI::tag<GraphLinkRecord::ByUniqueName>,
          BMI::member<GraphLinkRecord, std::string, &GraphLinkRecord::uniqueName>
        >,
        BMI::ordered_unique
        <
          BMI::tag<GraphLinkRecord::ByVertex>,
          BMI::member<GraphLinkRecord, Vertex, &GraphLinkRecord::vertex>
        >
      >
    > GraphLinkContainer;
    
    class Model : public QGraphicsScene
    {
      Q_OBJECT
    public:
      Model(QObject *parentIn, const Gui::Document &documentIn);
      virtual ~Model() override;
      void awake(); //!< hooked up to event dispatcher for update when idle.
      void selectionChanged(const SelectionChanges& msg);
      
    protected:
      virtual void mouseMoveEvent(QGraphicsSceneMouseEvent* event) override;
      virtual void mousePressEvent(QGraphicsSceneMouseEvent* event) override;
      virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent* event) override;
      virtual void contextMenuEvent(QGraphicsSceneContextMenuEvent* event) override;
      
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
      typedef boost::BOOST_SIGNALS_NAMESPACE::connection Connection;
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
      
      std::shared_ptr<GraphLinkContainer> graphLink;
      std::shared_ptr<Graph> theGraph;
      bool graphDirty;
      
      const GraphLinkRecord& findRecord(Vertex vertexIn);
      const GraphLinkRecord& findRecord(const App::DocumentObject* dObjectIn);
      const GraphLinkRecord& findRecord(const Gui::ViewProviderDocumentObject* VPDObjectIn);
      const GraphLinkRecord& findRecord(const ViewEntryRectItem* rectIn);
      const GraphLinkRecord& findRecord(const std::string &stringIn);
      void eraseRecord(const Gui::ViewProviderDocumentObject* VPDObjectIn);
      
      void indexVerticesEdges();
      void removeAllItems();
      void addVertexItemsToScene(const Vertex &vertexIn);
      void removeVertexItemsFromScene(const Vertex &vertexIn);
      void updateStates();
      
      ViewEntryRectItem* getRectFromPosition(const QPointF &position); //!< can be nullptr
      
    //! @name View Constants for spacing
    //@{
      float fontHeight;                           //!< height of the current qApp default font.
      float verticalSpacing;                      //!< pixels between top and bottom of text to background rectangle.
      float rowHeight;                            //!< height of background rectangle.
      float iconSize;                             //!< size of icon to match font.
      float pointSize;                            //!< size of the connection point.
      float pointSpacing;                         //!< spacing between pofloat columns.
      float pointToIcon;                          //!< spacing from last column points to first icon.
      float iconToIcon;                           //!< spacing between icons.
      float iconToText;                           //!< spacing between last icon and text.
      float rowPadding;                           //!< spaces added to rectangle bacground width ends.
      std::vector<QBrush> backgroundBrushes;      //!< brushes to paint background rectangles.
      std::vector<QBrush> forgroundBrushes;       //!< brushes to paint points, connectors, text.
      void setupViewConstants();
    //@}
      
      ViewEntryRectItem *currentPrehighlight;
      
      enum class SelectionMode
      {
        Single,
        Multiple
      };
      SelectionMode selectionMode;
      std::vector<Vertex> getAllSelected();
      void visiblyIsolate(Vertex sourceIn); //!< hide any connected feature and turn on sourceIn.
      
      QPointF lastPick;
      bool lastPickValid = false;
      
      QPixmap visiblePixmapEnabled;
      QPixmap visiblePixmapDisabled;
      QPixmap passPixmap;
      QPixmap failPixmap;
      
      QAction *renameAction;
      QAction *editingFinishedAction;
      QGraphicsProxyWidget *proxy = nullptr;
      void finishRename();
    };
  
    /*! @brief Get connected components.
    */
    class ConnectionVisitor : public boost::default_bfs_visitor
    {
    public:
      ConnectionVisitor(std::vector<Vertex> &verticesIn) : vertices(verticesIn){}
      
      template<typename TVertex, typename TGraph>
      void discover_vertex(TVertex vertex, TGraph &graph)
      {
        vertices.push_back(vertex);
      }
    private:
      std::vector<Vertex> &vertices;
    };
  }

}

#endif // DAGMODEL_H
