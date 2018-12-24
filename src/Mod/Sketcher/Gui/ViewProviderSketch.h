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

#include <Mod/Part/Gui/ViewProvider2DObject.h>
#include <Mod/Part/App/BodyBase.h>
#include <Inventor/SbImage.h>
#include <Inventor/SbColor.h>
#include <Base/Tools2D.h>
#include <Base/Placement.h>
#include <Gui/Selection.h>
#include <Gui/GLPainter.h>
#include <App/Part.h>
#include <boost/signals2.hpp>
#include <QCoreApplication>
#include <Gui/Document.h>
#include "ShortcutListener.h"


class TopoDS_Shape;
class TopoDS_Face;
class SoSeparator;
class SbLine;
class SbVec3f;
class SoCoordinate3;
class SoInfo;
class SoPointSet;
class SoTransform;
class SoLineSet;
class SoMarkerSet;

class SoImage;
class QImage;
class QColor;

class SoText2;
class SoTranslation;
class SbString;
class SbTime;

struct EditData;

namespace Gui {
    class View3DInventorViewer;
}

namespace Sketcher {
    class Constraint;
    class Sketch;
    class SketchObject;
}

namespace SketcherGui {

class DrawSketchHandler;

/** The Sketch ViewProvider
  * This class handles mainly the drawing and editing of the sketch.
  * It draws the geometry and the constraints applied to the sketch.
  * It uses the class DrawSketchHandler to facilitate the creation
  * of new geometry while editing.
  */
class SketcherGuiExport ViewProviderSketch : public PartGui::ViewProvider2DObject, public Gui::SelectionObserver
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::ViewProviderSketch)
    /// generates a warning message about constraint conflicts and appends it to the given message
    static QString appendConflictMsg(const std::vector<int> &conflicting);
    /// generates a warning message about redundant constraints and appends it to the given message
    static QString appendRedundantMsg(const std::vector<int> &redundant);

    PROPERTY_HEADER(SketcherGui::ViewProviderSketch);

public:
    /// constructor
    ViewProviderSketch();
    /// destructor
    virtual ~ViewProviderSketch();

    App::PropertyBool Autoconstraints;
    App::PropertyPythonObject TempoVis;
    App::PropertyBool HideDependent;
    App::PropertyBool ShowLinks;
    App::PropertyBool ShowSupport;
    App::PropertyBool RestoreCamera;

    /// Draw all constraint icons
    /*! Except maybe the radius and lock ones? */
    void drawConstraintIcons();

    /// draw the sketch in the inventor nodes
    /// temp => use temporary solver solution in SketchObject
    /// recreateinformationscenography => forces a rebuild of the information layer scenography
    void draw(bool temp=false, bool rebuildinformationlayer=true);

    /// draw the edit curve
    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);

    /// Is the view provider selectable
    bool isSelectable(void) const;
    /// Observer message from the Selection
    virtual void onSelectionChanged(const Gui::SelectionChanges& msg);

    /// Show/Hide nodes from information layer
    void showRestoreInformationLayer();

    /** @name handler control */
    //@{
    /// sets an DrawSketchHandler in control
    void activateHandler(DrawSketchHandler *newHandler);
    /// removes the active handler
    void purgeHandler(void);
    /// set the pick style of the sketch coordinate axes
    void setAxisPickStyle(bool on);
    //@}

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
        STATUS_SKETCH_UseRubberBand /**< enum value when making a rubber band selection *//**< enum value a DrawSketchHandler is in control. */
    };
    /// is called by GuiCommands to set the drawing mode
    void setSketchMode(SketchMode mode) {Mode = mode;}
    /// get the sketch mode
    SketchMode getSketchMode(void) const {return Mode;}
    //@}

    /** @name helper functions */
    //@{
    /// give the coordinates of a line on the sketch plane in sketcher (2D) coordinates
    void getCoordsOnSketchPlane(double &u, double &v, const SbVec3f &point,
                                const SbVec3f &normal);

    /// give projecting line of position
    void getProjectingLine(const SbVec2s&,
                           const Gui::View3DInventorViewer *viewer,
                           SbLine&) const;

    /// helper to detect preselection
    bool detectPreselection(const SoPickedPoint *Point,
                            const Gui::View3DInventorViewer *viewer,
                            const SbVec2s &cursorPos);

    /// Helper for detectPreselection(), for constraints only.
    std::set<int> detectPreselectionConstr(const SoPickedPoint *Point,
                                           const Gui::View3DInventorViewer *viewer,
                                           const SbVec2s &cursorPos);

    /*! Look at the center of the bounding of all selected items */
    void centerSelection();

    /// box selection method
    void doBoxSelection(const SbVec2s &startPos, const SbVec2s &endPos,
                        const Gui::View3DInventorViewer *viewer);

    /// helper change the color of the sketch according to selection and solver status
    void updateColor(void);
    /// get the pointer to the sketch document object
    Sketcher::SketchObject *getSketchObject(void) const;

    /// snap points x,y (mouse coordinates) onto grid if enabled
    void snapToGrid(double &x, double &y);

    /// moves a selected constraint
    void moveConstraint(int constNum, const Base::Vector2d &toPos);
    /// finds a free position for placing a constraint icon
    Base::Vector3d seekConstraintPosition(const Base::Vector3d &origPos,
                                          const Base::Vector3d &norm,
                                          const Base::Vector3d &dir, float step,
                                          const SoNode *constraint);

    float getScaleFactor();
    int getPreselectPoint(void) const;
    int getPreselectCurve(void) const;
    int getPreselectCross(void) const;
    //@}

    /** @name base class implementer */
    //@{
    virtual void attach(App::DocumentObject *);
    virtual void updateData(const App::Property *);

    virtual void setupContextMenu(QMenu *menu, QObject *receiver, const char *member);
    /// is called when the Provider is in edit and a deletion request occurs
    virtual bool onDelete(const std::vector<std::string> &);
    /// is called by the tree if the user double click on the object
    virtual bool doubleClicked(void);
    /// is called when the Provider is in edit and the mouse is moved
    virtual bool mouseMove(const SbVec2s &pos, Gui::View3DInventorViewer *viewer);
    /// is called when the Provider is in edit and a key event ocours. Only ESC ends edit.
    virtual bool keyPressed(bool pressed, int key);
    /// is called when the Provider is in edit and the mouse is clicked
    virtual bool mouseButtonPressed(int Button, bool pressed, const SbVec2s& cursorPos, const Gui::View3DInventorViewer* viewer);
    //@}

    void deleteSelected();

    /// updates the visibility of the virtual space
    void updateVirtualSpace(void);
    void setIsShownVirtualSpace(bool isshownvirtualspace);
    bool getIsShownVirtualSpace(void) const;

    friend class DrawSketchHandler;
    friend struct ::EditData;

    /// signals if the constraints list has changed
    boost::signals2::signal<void ()> signalConstraintsChanged;
    /// signals if the sketch has been set up
    boost::signals2::signal<void (QString msg)> signalSetUp;
    /// signals if the sketch has been solved
    boost::signals2::signal<void (QString msg)> signalSolved;
    /// signals if the elements list has changed
    boost::signals2::signal<void ()> signalElementsChanged;

protected:
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    virtual void setEditViewer(Gui::View3DInventorViewer*, int ModNum);
    virtual void unsetEditViewer(Gui::View3DInventorViewer*);
    void deactivateHandler();
    /// update solver information based on last solving at SketchObject
    void UpdateSolverInformation(void);
    /// helper to detect whether the picked point lies on the sketch
    bool isPointOnSketch(const SoPickedPoint *pp) const;
    /// get called by the container whenever a property has been changed
    virtual void onChanged(const App::Property *prop);

    /// get called if a subelement is double clicked while editing
    void editDoubleClicked(void);

    /// set up the edition data structure EditData
    void createEditInventorNodes(void);
    /// pointer to the edit data structure if the ViewProvider is in edit.
    EditData *edit;
    /// build up the visual of the constraints
    void rebuildConstraintsVisual(void);

    void slotUndoDocument(const Gui::Document&);
    void slotRedoDocument(const Gui::Document&);

protected:
    boost::signals2::connection connectUndoDocument;
    boost::signals2::connection connectRedoDocument;

    /// Return display string for constraint including hiding units if
    //requested.
    QString getPresentationString(const Sketcher::Constraint *constraint);

    /** @name Protected helpers for drawing constraint icons*/
    //@{
    QString iconTypeFromConstraint(Sketcher::Constraint *constraint);

    /// Returns a QColor object appropriate for constraint with given id
    /*! In the case of combined icons, the icon color is chosen based on
     *  the constraint with the highest priority from constrColorPriority()
     */
    QColor constrColor(int constraintId);
    /// Used by drawMergedConstraintIcons to decide what color to make icons
    /*! See constrColor() */
    int constrColorPriority(int constraintId);

    /// Internal type used for drawing constraint icons
    struct constrIconQueueItem {
        /// Type of constraint the icon represents.  Eg: "small/Constraint_PointOnObject_sm"
        QString type;

        /// Internal constraint ID number
        /// These map to results of getSketchObject()->Constraints.getValues()
        int constraintId;

        /// Label to be rendered with this icon, if any
        QString label;

        /// Absolute coordinates of the constraint icon
        SbVec3f position;

        /// Pointer to the SoImage object where the icon should be written
        SoImage *destination;

        /// Pointer to SoInfo object where we store the constraint IDs that the icon refers to
        SoInfo *infoPtr;

        /// Angle to rotate an icon
        double iconRotation;
    };

    /// Internal type used for drawing constraint icons
    typedef std::vector<constrIconQueueItem> IconQueue;
    /// For constraint icon bounding boxes
    typedef std::pair<QRect, std::set<int> > ConstrIconBB;
    /// For constraint icon bounding boxes
    typedef std::vector<ConstrIconBB> ConstrIconBBVec;

    void combineConstraintIcons(IconQueue iconQueue);

    /// Renders an icon for a single constraint and sends it to Coin
    void drawTypicalConstraintIcon(const constrIconQueueItem &i);

    /// Combines multiple constraint icons and sends them to Coin
    void drawMergedConstraintIcons(IconQueue iconQueue);

    /// Helper for drawMergedConstraintIcons and drawTypicalConstraintIcon
    QImage renderConstrIcon(const QString &type,
                            const QColor &iconColor,
                            const QStringList &labels,
                            const QList<QColor> &labelColors,
                            double iconRotation,
                            //! Gets populated with bounding boxes (in icon
                            //! image coordinates) for the icon at left, then
                            //! labels for different constraints.
                            std::vector<QRect> *boundingBoxes = NULL,
                            //! If not NULL, gets set to the number of pixels
                            //! that the text extends below the icon base.
                            int *vPad = NULL);

    /// Copies a QImage constraint icon into a SoImage*
    /*! Used by drawTypicalConstraintIcon() and drawMergedConstraintIcons() */
    void sendConstraintIconToCoin(const QImage &icon, SoImage *soImagePtr);

    /// Essentially a version of sendConstraintIconToCoin, with a blank icon
    void clearCoinImage(SoImage *soImagePtr);

    /// Returns the size that Coin should display the indicated image at
    SbVec3s getDisplayedSize(const SoImage *) const;
    //@}

    void setPositionText(const Base::Vector2d &Pos, const SbString &txt);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);

    // handle preselection and selection of points
    void setPreselectPoint(int PreselectPoint);
    void resetPreselectPoint(void);
    void addSelectPoint(int SelectPoint);
    void removeSelectPoint(int SelectPoint);
    void clearSelectPoints(void);

    // modes while sketching
    SketchMode Mode;

    // colors
    static SbColor VertexColor;
    static SbColor CurveColor;
    static SbColor CreateCurveColor;
    static SbColor CurveDraftColor;
    static SbColor CurveExternalColor;
    static SbColor CrossColorV;
    static SbColor CrossColorH;
    static SbColor FullyConstrainedColor;
    static SbColor ConstrDimColor;
    static SbColor ConstrIcoColor;
    static SbColor NonDrivingConstrDimColor;
    static SbColor ExprBasedConstrDimColor;
    static SbColor PreselectColor;
    static SbColor SelectColor;
    static SbColor PreselectSelectedColor;
    static SbColor InformationColor;

    static SbTime prvClickTime;
    static SbVec2s prvClickPos; //used by double-click-detector
    static SbVec2s prvCursorPos;
    static SbVec2s newCursorPos;

    float zCross;
    //float zLines;
    //float zPoints;
    float zLowPoints;
    float zHighPoints;
    float zConstr;
    float zHighlight;
    float zText;
    float zEdit;
    float zHighLine;
    float zInfo;
    float zLowLines;
    float zMidLines;
    float zHighLines;

    // reference coordinates for relative operations
    double xInit,yInit;
    bool relative;

    std::string oldWb;

    Gui::Rubberband* rubberband;

    // information layer variables
    bool visibleInformationChanged;
    double combrepscalehyst;

    // Virtual space variables
    bool isShownVirtualSpace; // indicates whether the present virtual space view is the Real Space or the Virtual Space (virtual space 1 or 2)

    ShortcutListener* listener;
};

} // namespace PartGui


#endif // SKETCHERGUI_VIEWPROVIDERSKETCH_H

