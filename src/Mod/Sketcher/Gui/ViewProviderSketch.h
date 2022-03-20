/***************************************************************************
 *   Copyright (c) 2009 Juergen Riegel <juergen.riegel@web.de>             *
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


#ifndef SKETCHERGUI_VIEWPROVIDERSKETCH_H
#define SKETCHERGUI_VIEWPROVIDERSKETCH_H

#include <memory>

#include <Mod/Part/Gui/ViewProvider2DObject.h>
#include <Mod/Part/Gui/ViewProviderAttachExtension.h>
#include <Mod/Part/App/BodyBase.h>
#include <Base/Tools2D.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>
#include <Gui/Selection.h>
#include <Gui/GLPainter.h>
#include <App/Part.h>
#include <boost_signals2.hpp>
#include <QCoreApplication>
#include <Gui/Document.h>
#include "ShortcutListener.h"

#include <Mod/Sketcher/App/GeoList.h>

class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbLine;
class SbVec2f;
class SbVec3f;
class SoCoordinate3;
class SoInfo;
class SoPointSet;
class SoTransform;
class SoLineSet;
class SoMarkerSet;
class SoPickedPoint;
class SoRayPickAction;

class SoImage;
class QImage;
class QColor;

class SoText2;
class SoTranslation;
class SbString;
class SbTime;

namespace Part {
    class Geometry;
}

namespace Gui {
    class View3DInventorViewer;
}

namespace Sketcher {
    class Constraint;
    class Sketch;
    class SketchObject;
}

namespace SketcherGui {

class EditModeCoinManager;
class DrawSketchHandler;

using GeoList = Sketcher::GeoList;
using GeoListFacade = Sketcher::GeoListFacade;

/** @brief The Sketch ViewProvider
 *
 * @details
 *
 * As any ViewProvider, this class is responsible for the view representation
 * of Sketches.
 *
 * Functionality inherited from parent classes deal with the majority of the
 * representation of a sketch when it is ** not ** in edit mode.
 *
 * This class handles mainly the drawing and editing of the sketch.
 *
 * The class delegates a substantial part of this functionality on two main
 * classes, DrawSketchHandler and EditModeCoinManager.
 *
 * In order to enforce a certain degree of encapsulation and promote a not
 * too tight coupling, while still allowing well defined collaboration,
 * DrawSketchHandler and EditModeCoinManager access ViewProviderSketch via
 * two Attorney classes (Attorney-Client pattern), ViewProviderSketchDrawSketchHandlerAttorney
 * and ViewProviderSketchCoinAttorney.
 *
 * Given the substantial amount of code involved in coin node management, EditModeCoinManager
 * further delegates on other specialised helper classes. Some of them share the
 * ViewProviderSketchCoinAttorney, which defines the maximum coupling and minimum encapsulation.
 *
 * DrawSketchHandler aids temporary edit mode drawing and is extensively used for the creation
 * of geometry.
 *
 * EditModeCoinManager takes over the responsibility of creating the Coin (Inventor) scenograph
 * and modifying it, including all the drawing of geometry, constraints and overlay layer. This
 * is an exclusive responsibility under the Single Responsibility Principle.
 *
 * EditModeCoinManager exposes a public interface to be used by ViewProviderSketch. Where,
 * EditModeCoinManager needs special access to facilities of ViewProviderSketch in order to fulfil
 * its responsibility, this access is defined by ViewProviderSketchCoinAttorney.
 *
 * Similarly, DrawSketchHandler takes over the responsibility of drawing edit temporal curves and
 * markers necessary to enable visual feedback to the user, as well as the UI interaction during
 * such edits. This is its exclusive responsibility under the Single Responsibility Principle.
 *
 * A plethora of speciliased handlers derive from DrawSketchHandler for each specialised editing (see
 * for example all the handlers for creation of new geometry). These derived classes do * not * have
 * direct access to the ViewProviderSketchDrawSketchHandlerAttorney. This is intended to keep coupling
 * under control. However, generic functionality requiring access to the Attorney can be implemented
 * in DrawSketchHandler and used from its derived classes by virtue of the inheritance. This promotes a
 * concentrating the coupling in a single point (and code reuse).
 *
 */
class SketcherGuiExport ViewProviderSketch : public PartGui::ViewProvider2DObjectGrid
                                           , public PartGui::ViewProviderAttachExtension
                                           , public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::ViewProviderSketch)

    PROPERTY_HEADER_WITH_OVERRIDE(SketcherGui::ViewProviderSketch);

private:
    /**
     * @brief
     * This nested class is responsible for attaching to the parameters relevant for
     * ViewProviderSketch, initialising the ViewProviderSketch to the current configuration
     * and handle in real time any change to their values.
     */
    class ParameterObserver : public ParameterGrp::ObserverType
    {
    public:
        ParameterObserver(ViewProviderSketch & client);
        ~ParameterObserver();

        void initParameters();

        void subscribeToParameters();

        void unsubscribeToParameters();

        /** Observer for parameter group. */
        void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

    private:

        void updateBoolProperty(const std::string & string, App::Property * property, bool defaultvalue);
        void updateGridSize(const std::string & string, App::Property * property);

        // Only for colors outside of edit mode, edit mode colors are handled by EditModeCoinManager.
        void updateColorProperty(const std::string & string, App::Property * property, float r, float g, float b);

        void updateEscapeKeyBehaviour(const std::string & string, App::Property * property);

        void updateAutoRecompute(const std::string & string, App::Property * property);

        void updateRecalculateInitialSolutionWhileDragging(const std::string & string, App::Property * property);

    private:
        ViewProviderSketch &Client;
        std::map<std::string, std::tuple<std::function<void(const std::string & string, App::Property *)>, App::Property * >> parameterMap;
    };

    /** @name Classes storing the state of Dragging, Selection and Preselection
     * All these classes enable the identification of a Vertex, a Curve, the root
     * Point, axes, and constraints.
     *
     * A word on indices and ways to identify elements and parts of them:
     *
     * The Sketcher has a general main way to identify geometry, {GeoId, PointPos},
     * these can be provided as separate data, or using Sketcher::GeoElementId. The
     * latter defines comparison and equality operators enabling, for example to use
     * it as key in a std::map.
     *
     * While it is indeed possible to refer to any point using that nomenclature, creating
     * maps in certain circumnstances leads to a performance drawback. Additionally, the
     * legacy selection mechanism refers to positive indexed Vertices (for both normal and
     * external vertices). Both reasons discourage moving to a single identification. This
     * situation has been identified at different levels:
     *
     * (1) In sketch.cpp, the solver facade defining the interface with GCS, both
     * {GeoId, PointPos} and PointId are used.
     *
     * (2) In SketchObject.cpp, the actual document object, both {GeoId, PointPos} and VertexId
     * are used (see VertexId2GeoId and VertexId2GeoPosId).
     *
     * (3) In ViewProviderSketch, both {GeoId, PointPos} an Point indices are used (see these structures)
     *
     * (4) At CoinManager level, {GeoId, PointPos}, Point indices (for selection) and MultiFieldIds (specific
     * structure defining a coin multifield index and layer) are used.
     *
     * Using a single index instead of a multi-index field, allows mappings to be implemented via std::vectors
     * instead of std::maps. Direct mappings using std::vectors are accessed in constant time. Multi-index mappings
     * relying on std::maps involve a search for the key. This leads to a drop in performance.
     *
     * What are these indices and how do they relate each other?
     * 1. PointId, VertexId depend on the order of the geometries in the sketch. GeoList and GeoListFacade enable to
     * convert between indices.
     * 2. CurveId is basically the GeoId assuming PointPos to be PointPos::none (edge)
     * 3. Sometimes, Axes and root point are separated into a third index or enum. Legacy reasons aside, the root point
     * is has GeoId=-1, which is sometimes used as invalid value in positive only indices. Additionally, root point and
     * the Horizontal Axes both have GeoId=-1 (differing in PointPos only). Following the decision not to rely on PointPos,
     * creating a separate index, best when enum-ed, appears justified.
     */
    //@{

    /** @brief Class to store vector and item Id for dragging.
      *
      * @details
      * Ids are zero-indexed points and curves.
      *
      * The DragPoint indexing matches PreselectPoint indexing.
      *
      */
    class Drag {
    public:
        enum SpecialValues {
            InvalidPoint = -1,
            InvalidCurve = -1
        };

        Drag() {
            resetVector();
            resetIds();
        }

        void resetVector() {
            xInit = 0;
            yInit = 0;
            relative = false;
        }

        void resetIds() {
            DragPoint = InvalidPoint;
            DragCurve = InvalidCurve;
            DragConstraintSet.clear();
        }

        bool isDragPointValid() { return DragPoint > InvalidPoint;}
        bool isDragCurveValid() { return DragCurve > InvalidCurve;}

        double xInit, yInit;                // starting point of the dragging operation
        bool relative;                      // whether the dragging move vector is relative or absolute


        int DragPoint;                      // dragged point id (only positive integers)
        int DragCurve;                      // dragged curve id (only positive integers), negative external curves cannot be dragged.
        std::set<int> DragConstraintSet;    // dragged constraints ids
    };

    // TODO: Selection and Preselection should use a same structure. Probably Drag should use the same structure too. To be refactored separately.

    /** @brief Class to store preselected element ids.
      *
      * @details
      *
      * PreselectPoint is the positive VertexId.
      *
      * PreselectCurve is the GeoID, but without the Axes (indices -1 and -2).
      *
      * VertexN, with N = PreselectPoint + 1, same as DragPoint indexing (NOTE -1 is NOT the root point)
      *
      * EdgeN, with N = PreselectCurve + 1 for positive values ; ExternalEdgeN, with N = -PreselectCurve - 2
      *
      * The PreselectPoint indexing matches DragPoint indexing (it further includes negative edges, which are
      * not meaningful for Dragging).
      *
      */
    class Preselection {
    public:
        enum SpecialValues {
            InvalidPoint = -1,
            InvalidCurve = -1,
            ExternalCurve = -3
        };

        enum class Axes {
            None = -1,
            RootPoint = 0,
            HorizontalAxis = 1,
            VerticalAxis = 2
        };

        Preselection() {
            reset();
        }

        void reset(){
            PreselectPoint = InvalidPoint;
            PreselectCurve = InvalidCurve;
            PreselectCross = Axes::None;
            PreselectConstraintSet.clear();
            blockedPreselection = false;
        }

        bool isPreselectPointValid() const { return PreselectPoint > InvalidPoint;}
        bool isPreselectCurveValid() const { return PreselectCurve > InvalidCurve || PreselectCurve <= ExternalCurve;}
        bool isCrossPreselected() const { return PreselectCross != Axes::None;}
        bool isEdge() const { return PreselectCurve > InvalidCurve;}
        bool isExternalEdge() const { return PreselectCurve <= ExternalCurve;}

        int getPreselectionVertexIndex() const { return PreselectPoint + 1;}
        int getPreselectionEdgeIndex() const { return PreselectCurve + 1;}
        int getPreselectionExternalEdgeIndex() const { return -PreselectCurve - 2;}

        int PreselectPoint;                     // VertexN, with N = PreselectPoint + 1, same as DragPoint indexing (NOTE -1 is NOT the root point)
        int PreselectCurve;                     // EdgeN, with N = PreselectCurve + 1 for positive values ; ExternalEdgeN, with N = -PreselectCurve - 2
        Axes PreselectCross;                    // 0 => rootPoint, 1 => HAxis, 2 => VAxis
        std::set<int> PreselectConstraintSet;   // ConstraintN, N = index + 1
        bool blockedPreselection;
    };

    /** @brief Class to store selected element ids.
      *
      * @details
      * Selection follows yet a different mechanism than preselection.
      *
      * SelPointSet indices as PreselectPoint, with the addition that -1 is indeed the rootpoint.
      *
      * SelCurvSet indices as PreselectCurve, with the addition that -1 is the HAxis and -2 is the VAxis
      *
      */
    class Selection {
    public:
        enum SpecialValues {
            RootPoint = -1,
            HorizontalAxis = -1,
            VerticalAxis = -2
        };

        Selection() {
            reset();
        }

        void reset() {
            SelPointSet.clear();
            SelCurvSet.clear();
            SelConstraintSet.clear();
        }

        std::set<int> SelPointSet;              // Indices as PreselectPoint (and -1 for rootpoint)
        std::set<int> SelCurvSet;               // also holds cross axes at -1 and -2
        std::set<int> SelConstraintSet;         // ConstraintN, N = index + 1.
    };
    //@}

    /** @brief Private struct maintaining information necessary for detecting double click.
     */
    struct DoubleClick {
        static SbTime prvClickTime;
        static SbVec2s prvClickPos; //used by double-click-detector
        static SbVec2s prvCursorPos;
        static SbVec2s newCursorPos;
    };

    /** @brief Private struct grouping ViewProvider parameters and internal variables
     */
    struct ViewProviderParameters {
        bool handleEscapeButton = false;
        bool autoRecompute = false;
        bool recalculateInitialSolutionWhileDragging = false;

        bool isShownVirtualSpace = false; // indicates whether the present virtual space view is the Real Space or the Virtual Space (virtual space 1 or 2)
        bool buttonPress = false;
    };

public:
    /// constructor
    ViewProviderSketch();
    /// destructor
    virtual ~ViewProviderSketch();

    /** @name Properties */
    //@{
    App::PropertyBool Autoconstraints;
    App::PropertyBool AvoidRedundant;
    App::PropertyPythonObject TempoVis;
    App::PropertyBool HideDependent;
    App::PropertyBool ShowLinks;
    App::PropertyBool ShowSupport;
    App::PropertyBool RestoreCamera;
    App::PropertyBool ForceOrtho;
    App::PropertyBool SectionView;
    App::PropertyString EditingWorkbench;
    //@}

    // TODO: It is difficult to imagine that these functions are necessary in the public interface. This requires review at a second stage and possibly
    // refactor it.
    /** @name handler control */
    //@{
    /// sets an DrawSketchHandler in control
    void activateHandler(DrawSketchHandler *newHandler);
    /// removes the active handler
    void purgeHandler(void);
    //@}


    // TODO: SketchMode should be refactored. DrawSketchHandler, its inheritance and free functions should access this mode via the DrawSketchHandler
    // Attorney. I will not refactor this at this moment, as the refactor will be even more extensive and difficult to review. But this should be done
    // in a second stage.

    /** @name modus handling */
    //@{
    /// mode table
    enum SketchMode{
        STATUS_NONE,              /**< enum value View provider is in neutral. */
        STATUS_SELECT_Point,      /**< enum value a point was selected. */
        STATUS_SELECT_Edge,       /**< enum value an edge was selected. */
        STATUS_SELECT_Constraint, /**< enum value a constraint was selected. */
        STATUS_SELECT_Cross,      /**< enum value the base coordinate system was selected. */
        STATUS_SKETCH_DragPoint,  /**< enum value while dragging a point. */
        STATUS_SKETCH_DragCurve,  /**< enum value while dragging a curve. */
        STATUS_SKETCH_DragConstraint,  /**< enum value while dragging a compatible constraint. */
        STATUS_SKETCH_UseHandler, /**< enum value a DrawSketchHandler is in control. */
        STATUS_SKETCH_StartRubberBand, /**< enum value for initiating a rubber band selection */
        STATUS_SKETCH_UseRubberBand /**< enum value when making a rubber band selection */
    };

    /// is called by GuiCommands to set the drawing mode
    void setSketchMode(SketchMode mode) {Mode = mode;}
    /// get the sketch mode
    SketchMode getSketchMode(void) const {return Mode;}
    //@}

    /** @name Drawing functions */
    //@{
    /// draw the sketch in the inventor nodes
    /// temp => use temporary solver solution in SketchObject
    /// recreateinformationscenography => forces a rebuild of the information overlay scenography
    void draw(bool temp=false, bool rebuildinformationoverlay=true);

    /// helper change the color of the sketch according to selection and solver status
    void updateColor(void);
    //@}

    /** @name Selection functions */
    //@{
    /// Is the view provider selectable
    bool isSelectable(void) const override;

    /// Observer message from the Selection
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg) override;
    //@}

    /** @name Access to Sketch and Solver objects */
    //@{
    /// get the pointer to the sketch document object
    Sketcher::SketchObject *getSketchObject(void) const;

    /** returns a const reference to the last solved sketch object. It guarantees that
     *  the solver object does not lose synchronisation with the SketchObject properties.
     *
     * NOTE: Operations requiring * write * access to the solver must be done via SketchObject
     * interface. See for example functions:
     * -> inline void setRecalculateInitialSolutionWhileMovingPoint(bool recalculateInitialSolutionWhileMovingPoint)
     * -> inline int initTemporaryMove(int geoId, PointPos pos, bool fine=true)
     * -> inline int moveTemporaryPoint(int geoId, PointPos pos, Base::Vector3d toPoint, bool relative=false)
     * -> inline void updateSolverExtension(int geoId, std::unique_ptr<Part::GeometryExtension> && ext)
     */
    const Sketcher::Sketch &getSolvedSketch(void) const;
    //@}

    /** @name miscelanea utilities */
    //@{
    /*! Look at the center of the bounding of all selected items */
    void centerSelection();
    /// returns the scale factor
    float getScaleFactor() const;
    //@}

    /** @name constraint Virtual Space visibility management */
    //@{
    /// updates the visibility of the virtual space of constraints
    void updateVirtualSpace(void);
    /// determines whether the constraints in the normal space or the ones in the virtual are to be shown
    void setIsShownVirtualSpace(bool isshownvirtualspace);
    /// returns whether the virtual space is being shown
    bool getIsShownVirtualSpace(void) const;
    //@}

    /** @name base class implementer */
    //@{
    virtual void attach(App::DocumentObject *) override;
    virtual void updateData(const App::Property *) override;

    virtual void setupContextMenu(QMenu *menu, QObject *receiver, const char *member) override;
    /// is called when the Provider is in edit and a deletion request occurs
    virtual bool onDelete(const std::vector<std::string> &) override;
    /// Is called by the tree if the user double clicks on the object. It returns the string
    /// for the transaction that will be shown in the undo/redo dialog.
    /// If null is returned then no transaction will be opened.
    virtual const char* getTransactionText() const override { return nullptr; }
    /// is called by the tree if the user double clicks on the object
    virtual bool doubleClicked(void) override;
    /// is called when the Provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s &pos, Gui::View3DInventorViewer *viewer) override;
    /// is called when the Provider is in edit and a key event ocours. Only ESC ends edit.
    virtual bool keyPressed(bool pressed, int key) override;
    /// is called when the Provider is in edit and the mouse is clicked
    virtual bool mouseButtonPressed(int Button, bool pressed, const SbVec2s& cursorPos, const Gui::View3DInventorViewer* viewer) override;
    virtual bool mouseWheelEvent(int delta, const SbVec2s &cursorPos, const Gui::View3DInventorViewer* viewer) override;
    //@}

    /// Control the overlays appearing on the Tree and reflecting different sketcher states
    virtual QIcon mergeColorfulOverlayIcons (const QIcon & orig) const override;

    /** @name Signals for controlling information in Task dialogs */
    //@{
    /// signals if the constraints list has changed
    boost::signals2::signal<void ()> signalConstraintsChanged;
    /// signals if the sketch has been set up
    boost::signals2::signal<void (const QString &state, const QString &msg, const QString &url, const QString &linkText)> signalSetUp;
    /// signals if the elements list has changed
    boost::signals2::signal<void ()> signalElementsChanged;
    //@}

    /** @name Attorneys for collaboration with helper classes */
    //@{
    friend class ViewProviderSketchDrawSketchHandlerAttorney;
    friend class ViewProviderSketchCoinAttorney;
    friend class ViewProviderSketchShortcutListenerAttorney;
    //@}
protected:
    /** @name enter/exit edit mode */
    //@{
    virtual bool setEdit(int ModNum) override;
    virtual void unsetEdit(int ModNum) override;
    virtual void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;
    virtual void unsetEditViewer(Gui::View3DInventorViewer*) override;
    //@}

    /** @name miscelanea editing functions */
    //@{
    /// purges the DrawHandler if existing and tidies up
    void deactivateHandler();
    /// get called if a subelement is double clicked while editing
    void editDoubleClicked(void);
    //@}

    /** @name Solver Information */
    //@{
    /// update solver information based on last solving at SketchObject
    void UpdateSolverInformation(void);

    /// Auxiliary function to generate messages about conflicting, redundant and malformed constraints
    static QString appendConstraintMsg( const QString & singularmsg,
                                        const QString & pluralmsg,
                                        const std::vector<int> &vector);
    //@}

    /** @name manage updates during undo/redo operations */
    //@{
    void slotUndoDocument(const Gui::Document&);
    void slotRedoDocument(const Gui::Document&);
    void forceUpdateData();
    //@}

    /** @name base class implementer */
    //@{
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property *prop) override;
    //@}

private:
    /// function to handle OCCT BSpline weight calculation singularities and representation
    void scaleBSplinePoleCirclesAndUpdateSolverAndSketchObjectGeometry(
                        GeoListFacade & geolist,
                        bool geometrywithmemoryallocation);

    /** @name geometry and coordinates auxiliary functions */
    //@{
    /// give the coordinates of a line on the sketch plane in sketcher (2D) coordinates
    void getCoordsOnSketchPlane(const SbVec3f &point, const SbVec3f &normal, double &u, double &v) const;

    /// give projecting line of position
    void getProjectingLine(const SbVec2s&,
                           const Gui::View3DInventorViewer *viewer,
                           SbLine&) const;
    //@}

    /** @name preselection functions */
    //@{
    /// helper to detect preselection
    bool detectAndShowPreselection (SoPickedPoint * Point, const SbVec2s &cursorPos);
    int getPreselectPoint(void) const;
    int getPreselectCurve(void) const;
    int getPreselectCross(void) const;
    void setPreselectPoint(int PreselectPoint);
    void setPreselectRootPoint();
    void resetPreselectPoint(void);

    bool setPreselect(const std::string &subNameSuffix, float x = 0, float y = 0, float z = 0);
    //@}

    /** @name Selection functions */
    //@{
    /// box selection method
    void doBoxSelection(const SbVec2s &startPos, const SbVec2s &endPos,
                        const Gui::View3DInventorViewer *viewer);

    void addSelectPoint(int SelectPoint);
    void removeSelectPoint(int SelectPoint);
    void clearSelectPoints(void);

    bool isSelected(const std::string & ss) const;
    void rmvSelection(const std::string &subNameSuffix);
    bool addSelection(const std::string &subNameSuffix, float x = 0, float y = 0, float z = 0);
    bool addSelection2(const std::string &subNameSuffix, float x = 0, float y = 0, float z = 0);
    //@}

    /** @name miscelanea utilities */
    //@{
    /// snap points x,y (mouse coordinates) onto grid if enabled
    void snapToGrid(double &x, double &y);

    /// moves a selected constraint
    void moveConstraint(int constNum, const Base::Vector2d &toPos);

    /// returns whether the sketch is in edit mode.
    bool isInEditMode() const;
    //@}

    /** @name Attorney functions*/
    //@{
    /* private functions to decouple Attorneys and Clients from the internal implementation of
    the ViewProvider and its members, such as sketchObject (see friend attorney classes) and
    improve encapsulation.
    */

    //********* ViewProviderSketchCoinAttorney ***********************

    bool constraintHasExpression(int constrid) const;

    const std::vector<Sketcher::Constraint *> getConstraints() const;

    // gets the list of geometry of the sketchobject or of the solver instance
    const GeoList getGeoList() const;

    GeoListFacade getGeoListFacade() const;

    Base::Placement getEditingPlacement() const;

    std::unique_ptr<SoRayPickAction> getRayPickAction() const;

    SbVec2f getScreenCoordinates(SbVec2f sketchcoordinates) const;

    QFont getApplicationFont() const;

    int defaultFontSizePixels() const;

    int getApplicationLogicalDPIX() const;

    double getRotation(SbVec3f pos0, SbVec3f pos1) const;

    bool isSketchInvalid() const;

    bool isSketchFullyConstrained() const;

    bool haveConstraintsInvalidGeometry() const;

    void addNodeToRoot(SoSeparator * node);

    void removeNodeFromRoot(SoSeparator * node);

    bool isConstraintPreselected(int constraintId) const;

    bool isPointSelected(int pointId) const;

    void executeOnSelectionPointSet(std::function<void(const int)> && operation) const;

    bool isCurveSelected(int curveId) const;

    bool isConstraintSelected(int constraintId) const;

    //********* ViewProviderSketchShortcutListenerAttorney ***********//
    void deleteSelected();

    //********* ViewProviderSketchDrawSketchHandlerAttorney **********//
    void setConstraintSelectability(bool enabled = true);
    void setPositionText(const Base::Vector2d &Pos, const SbString &txt);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);

    /// draw the edit curve
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);
    void drawEdit(const std::list<std::vector<Base::Vector2d>> &list);
    /// draw the edit markers
    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel = 0);
    /// set the pick style of the sketch coordinate axes
    void setAxisPickStyle(bool on);
    //@}

private:
   /** @name Solver message creation*/
    //@{
    /* private functions to decouple Attorneys and Clients from the internal implementation of
    the ViewProvider and its members, such as sketchObject (see friend attorney classes) and
    improve encapsulation.
    */
    /// generates a warning message about constraint conflicts and appends it to the given message
    static QString appendConflictMsg(const std::vector<int> &conflicting);
    /// generates a warning message about redundant constraints and appends it to the given message
    static QString appendRedundantMsg(const std::vector<int> &redundant);
    /// generates a warning message about partially redundant constraints and appends it to the given message
    static QString appendPartiallyRedundantMsg(const std::vector<int> &partiallyredundant);
    /// generates a warning message about redundant constraints and appends it to the given message
    static QString appendMalformedMsg(const std::vector<int> &redundant);
    //@}

private:
    boost::signals2::connection connectUndoDocument;
    boost::signals2::connection connectRedoDocument;

    // modes while sketching
    SketchMode Mode;

    // reference coordinates for relative operations
    Drag drag;

    Preselection preselection;
    Selection selection;

    std::unique_ptr<Gui::Rubberband> rubberband;

    std::string editDocName;
    std::string editObjName;
    std::string editSubName;

    ShortcutListener* listener;

    std::unique_ptr<EditModeCoinManager> editCoinManager;

    std::unique_ptr<ViewProviderSketch::ParameterObserver> pObserver;

    std::unique_ptr<DrawSketchHandler> sketchHandler;

    ViewProviderParameters viewProviderParameters;
};

} // namespace PartGui


#endif // SKETCHERGUI_VIEWPROVIDERSKETCH_H

