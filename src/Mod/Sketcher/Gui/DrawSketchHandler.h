// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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

#pragma once

#include <QPixmap>
#include <QCoreApplication>

#include <Inventor/SbString.h>

#include <Base/Parameter.h>
#include <Base/Tools2D.h>
#include <Gui/Selection/Selection.h>
#include <Gui/ToolHandler.h>
#include <Gui/InputHint.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>

#include "AutoConstraint.h"
#include "Utils.h"
#include "SnapManager.h"

class QWidget;

namespace Sketcher
{
class Sketch;
class SketchObject;
}  // namespace Sketcher

namespace Gui
{
class View3DInventorViewer;
}

namespace SketcherGui
{

class ViewProviderSketch;


/**
 * Class to convert Part::Geometry to Vector2d based collections
 */
class CurveConverter final: public ParameterGrp::ObserverType
{

public:
    CurveConverter();

    ~CurveConverter() override;

    std::vector<Base::Vector2d> toVector2D(const Part::Geometry* geometry);

    std::list<std::vector<Base::Vector2d>> toVector2DList(
        const std::vector<Part::Geometry*>& geometries
    );

private:
    void updateCurvedEdgeCountSegmentsParameter();

    /** Observer for parameter group. */
    void OnChange(Base::Subject<const char*>& rCaller, const char* sReason) override;

private:
    int curvedEdgeCountSegments;
};

/**
 * In order to enforce a certain degree of encapsulation and promote a not
 * too tight coupling, while still allowing well defined collaboration,
 * DrawSketchHandler accesses ViewProviderSketch via this Attorney class
 */
class ViewProviderSketchDrawSketchHandlerAttorney
{
private:
    static inline void setConstraintSelectability(ViewProviderSketch& vp, bool enabled = true);
    static inline void setPositionText(
        ViewProviderSketch& vp,
        const Base::Vector2d& Pos,
        const SbString& txt
    );
    static inline void setPositionText(ViewProviderSketch& vp, const Base::Vector2d& Pos);
    static inline void resetPositionText(ViewProviderSketch& vp);
    static inline void drawEdit(ViewProviderSketch& vp, const std::vector<Base::Vector2d>& EditCurve);
    static inline void drawEdit(
        ViewProviderSketch& vp,
        const std::list<std::vector<Base::Vector2d>>& list
    );
    static inline void drawEditMarkers(
        ViewProviderSketch& vp,
        const std::vector<Base::Vector2d>& EditMarkers,
        unsigned int augmentationlevel = 0
    );
    static inline void setAxisPickStyle(ViewProviderSketch& vp, bool on);
    static inline void moveCursorToSketchPoint(ViewProviderSketch& vp, Base::Vector2d point);
    static inline void ensureFocus(ViewProviderSketch& vp);
    static inline void preselectAtPoint(ViewProviderSketch& vp, Base::Vector2d point);
    static inline void setAngleSnapping(
        ViewProviderSketch& vp,
        bool enable,
        Base::Vector2d referencePoint = Base::Vector2d(0., 0.)
    );

    static inline int getPreselectPoint(const ViewProviderSketch& vp);
    static inline int getPreselectCurve(const ViewProviderSketch& vp);
    static inline int getPreselectCross(const ViewProviderSketch& vp);

    static inline void moveConstraint(
        ViewProviderSketch& vp,
        int constNum,
        const Base::Vector2d& toPos,
        OffsetMode offset = NoOffset
    );

    static inline void signalToolChanged(const ViewProviderSketch& vp, const std::string& toolname);

    friend class DrawSketchHandler;
};

/** Handler to create new sketch geometry
 * This class has to be reimplemented to create geometry in the
 * sketcher while its in editing.
 *
 * DrawSketchHandler takes over the responsibility of drawing edit temporal curves and
 * markers necessary to enable visual feedback to the user, as well as the UI interaction during
 * such edits. This is its exclusive responsibility under the Single Responsibility Principle.
 *
 * A plethora of speciliased handlers derive from DrawSketchHandler for each specialised editing
 * (see for example all the handlers for creation of new geometry). These derived classes do * not *
 * have direct access to the ViewProviderSketchDrawSketchHandlerAttorney. This is intended to keep
 * coupling under control. However, generic functionality requiring access to the Attorney can be
 * implemented in DrawSketchHandler and used from its derived classes by virtue of the inheritance.
 * This promotes a concentrating the coupling in a single point (and code reuse).
 */
class SketcherGuiExport DrawSketchHandler: public Gui::ToolHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandler)

public:
    DrawSketchHandler();
    virtual ~DrawSketchHandler();

    void activate(ViewProviderSketch*);
    void setSketchGui(ViewProviderSketch* vp);
    void deactivate() override;

    virtual void mouseMove(SnapManager::SnapHandle snapHandle) = 0;
    virtual bool pressButton(Base::Vector2d pos) = 0;
    virtual bool releaseButton(Base::Vector2d pos) = 0;

    virtual void registerPressedKey(bool pressed, int key);
    virtual void pressRightButton(Base::Vector2d pos);

    virtual bool onSelectionChanged(const Gui::SelectionChanges&)
    {
        return false;
    }

    std::list<Gui::InputHint> getToolHints() const override
    {
        return {};
    }

    void quit() override;

    friend class ViewProviderSketch;

    // get the actual highest vertex index, the next use will be +1
    int getHighestVertexIndex();
    // get the actual highest edge index, the next use will be +1
    int getHighestCurveIndex();

    int seekAutoConstraint(
        std::vector<AutoConstraint>& suggestedConstraints,
        const Base::Vector2d& Pos,
        const Base::Vector2d& Dir,
        AutoConstraint::TargetType type = AutoConstraint::VERTEX
    );

    int seekAndRenderAutoConstraint(
        std::vector<AutoConstraint>& suggestedConstraints,
        const Base::Vector2d& Pos,
        const Base::Vector2d& Dir,
        AutoConstraint::TargetType type = AutoConstraint::VERTEX
    );

    // createowncommand indicates whether a separate command shall be create and committed (for
    // example for undo purposes) or not is not it is the responsibility of the developer to create
    // and commit the command appropriately.
    void createAutoConstraints(
        const std::vector<AutoConstraint>& autoConstrs,
        int geoId,
        Sketcher::PointPos pointPos = Sketcher::PointPos::none,
        bool createowncommand = true
    );

    void setPositionText(const Base::Vector2d& Pos, const SbString& text);
    void setPositionText(const Base::Vector2d& Pos);
    void resetPositionText();
    void renderSuggestConstraintsCursor(std::vector<AutoConstraint>& suggestedConstraints);

    /** @name Interfacing with tool dialogs */
    //@{

    /** @brief Slot to receive signaling that a widget intended for the tool has changed and is
     *  available ant the provided pointer.
     */
    void toolWidgetChanged(QWidget* newwidget);

    /** @brief Factory function returning a tool widget of the type necessary for the specific tool.
     * This is a NVI interface and specific handlers must overload the corresponding virtual
     * function.
     */
    std::unique_ptr<QWidget> createToolWidget() const;

    /** @brief Returns whether this tool expects/supports a visible tool widget. Emphasis is in
     * visibility, so to allow one to adapt the interface accordingly.
     * This is an NVI interface and specific handlers must overload the corresponding virtual
     * function.
     */
    bool isToolWidgetVisible() const;
    /** @brief Returns a pixmap icon intended for a visible tool widget.
     * This is an NVI interface and specific handlers must overload the corresponding virtual
     * function.
     */
    QPixmap getToolWidgetHeaderIcon() const;
    /** @brief Returns a header text intended for a visible tool widget.
     * This is an NVI interface and specific handlers must overload the corresponding virtual
     * function.
     */
    QString getToolWidgetHeaderText() const;
    //@}

private:  // NVI
    void preActivated() override;
    virtual void onWidgetChanged()
    {}

protected:  // NVI requiring base implementation
    virtual std::string getToolName() const;

    virtual std::unique_ptr<QWidget> createWidget() const;
    virtual bool isWidgetVisible() const;
    virtual QPixmap getToolIcon() const;
    virtual QString getToolWidgetText() const;

protected:
    void drawEdit(const std::vector<Base::Vector2d>& EditCurve) const;
    void drawEdit(const std::list<std::vector<Base::Vector2d>>& list) const;
    void drawEdit(const std::vector<Part::Geometry*>& geometries) const;
    void drawEditMarkers(
        const std::vector<Base::Vector2d>& EditMarkers,
        unsigned int augmentationlevel = 0
    ) const;

    void clearEdit() const;
    void clearEditMarkers() const;

    void setAxisPickStyle(bool on);
    void moveCursorToSketchPoint(Base::Vector2d point);
    void ensureFocus();
    void preselectAtPoint(Base::Vector2d point);

    void drawPositionAtCursor(const Base::Vector2d& position);
    void drawDirectionAtCursor(const Base::Vector2d& position, const Base::Vector2d& origin);
    void drawWidthHeightAtCursor(const Base::Vector2d& position, const double val1, const double val2);
    void drawDoubleAtCursor(
        const Base::Vector2d& position,
        const double radius,
        Base::Unit unit = Base::Unit::Length
    );

    int getPreselectPoint() const;
    int getPreselectCurve() const;
    int getPreselectCross() const;

    Sketcher::SketchObject* getSketchObject();

    void setAngleSnapping(bool enable, Base::Vector2d referencePoint = Base::Vector2d(0., 0.));

    void moveConstraint(int constNum, const Base::Vector2d& toPos, OffsetMode offset = NoOffset);

    void signalToolChanged() const;

    // Helpers for seekAutoConstraint :
    // Helper structure to hold preselection data
    struct PreselectionData
    {
        int geoId = Sketcher::GeoEnum::GeoUndef;
        Sketcher::PointPos posId = Sketcher::PointPos::none;
        // direction of hit shape (if it is a line, the direction of the line)
        Base::Vector3d hitShapeDir = Base::Vector3d(0, 0, 0);
        bool isLine = false;
    };
    PreselectionData getPreselectionData();

    void seekPreselectionAutoConstraint(
        std::vector<AutoConstraint>& constraints,
        const Base::Vector2d& Pos,
        const Base::Vector2d& Dir,
        AutoConstraint::TargetType type
    );

    bool isLineCenterAutoConstraint(int GeoId, const Base::Vector2d& Pos) const;

    void seekAlignmentAutoConstraint(std::vector<AutoConstraint>& constraints, const Base::Vector2d& Dir);

    void seekTangentAutoConstraint(
        std::vector<AutoConstraint>& constraints,
        const Base::Vector2d& Pos,
        const Base::Vector2d& Dir
    );

protected:
    /**
     * Returns constraints icons scaled to width.
     **/
    std::vector<QPixmap> suggestedConstraintsPixmaps(std::vector<AutoConstraint>& suggestedConstraints);

    ViewProviderSketch* sketchgui;

    QWidget* toolwidget;
};


}  // namespace SketcherGui
