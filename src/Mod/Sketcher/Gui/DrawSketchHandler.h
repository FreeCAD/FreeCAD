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

#ifndef SKETCHERGUI_DrawSketchHandler_H
#define SKETCHERGUI_DrawSketchHandler_H

#include <Inventor/SbString.h>

#include <Base/Tools2D.h>
#include <Gui/Selection.h>
#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/Constraint.h>

#include "AutoConstraint.h"


class QPixmap;

namespace Sketcher
{
class Sketch;
class SketchObject;
}  // namespace Sketcher

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

    std::list<std::vector<Base::Vector2d>>
    toVector2DList(const std::vector<Part::Geometry*>& geometries);

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
    static inline void
    setPositionText(ViewProviderSketch& vp, const Base::Vector2d& Pos, const SbString& txt);
    static inline void setPositionText(ViewProviderSketch& vp, const Base::Vector2d& Pos);
    static inline void resetPositionText(ViewProviderSketch& vp);
    static inline void drawEdit(ViewProviderSketch& vp,
                                const std::vector<Base::Vector2d>& EditCurve);
    static inline void drawEdit(ViewProviderSketch& vp,
                                const std::list<std::vector<Base::Vector2d>>& list);
    static inline void drawEditMarkers(ViewProviderSketch& vp,
                                       const std::vector<Base::Vector2d>& EditMarkers,
                                       unsigned int augmentationlevel = 0);
    static inline void setAxisPickStyle(ViewProviderSketch& vp, bool on);
    static inline void moveCursorToSketchPoint(ViewProviderSketch& vp, Base::Vector2d point);
    static inline void preselectAtPoint(ViewProviderSketch& vp, Base::Vector2d point);
    static inline void setAngleSnapping(ViewProviderSketch& vp,
                                        bool enable,
                                        Base::Vector2d referencePoint = Base::Vector2d(0., 0.));

    static inline int getPreselectPoint(const ViewProviderSketch& vp);
    static inline int getPreselectCurve(const ViewProviderSketch& vp);
    static inline int getPreselectCross(const ViewProviderSketch& vp);

    static inline void
    moveConstraint(ViewProviderSketch& vp, int constNum, const Base::Vector2d& toPos);

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
class SketcherGuiExport DrawSketchHandler
{
public:
    DrawSketchHandler();
    virtual ~DrawSketchHandler();

    void activate(ViewProviderSketch*);
    void deactivate();

    virtual void mouseMove(Base::Vector2d onSketchPos) = 0;
    virtual bool pressButton(Base::Vector2d onSketchPos) = 0;
    virtual bool releaseButton(Base::Vector2d onSketchPos) = 0;
    virtual bool onSelectionChanged(const Gui::SelectionChanges&)
    {
        return false;
    }
    virtual void registerPressedKey(bool /*pressed*/, int /*key*/)
    {}

    virtual void quit();

    friend class ViewProviderSketch;

    // get the actual highest vertex index, the next use will be +1
    int getHighestVertexIndex();
    // get the actual highest edge index, the next use will be +1
    int getHighestCurveIndex();

    int seekAutoConstraint(std::vector<AutoConstraint>& suggestedConstraints,
                           const Base::Vector2d& Pos,
                           const Base::Vector2d& Dir,
                           AutoConstraint::TargetType type = AutoConstraint::VERTEX);
    // createowncommand indicates whether a separate command shall be create and committed (for
    // example for undo purposes) or not is not it is the responsibility of the developer to create
    // and commit the command appropriately.
    void createAutoConstraints(const std::vector<AutoConstraint>& autoConstrs,
                               int geoId,
                               Sketcher::PointPos pointPos = Sketcher::PointPos::none,
                               bool createowncommand = true);

    void setPositionText(const Base::Vector2d& Pos, const SbString& text);
    void setPositionText(const Base::Vector2d& Pos);
    void resetPositionText();
    void renderSuggestConstraintsCursor(std::vector<AutoConstraint>& suggestedConstraints);

private:  // NVI
    virtual void preActivated();
    virtual void activated()
    {}
    virtual void deactivated()
    {}
    virtual void postDeactivated()
    {}

protected:  // NVI requiring base implementation
    virtual QString getCrosshairCursorSVGName() const;

protected:
    // helpers
    /**
     * Sets a cursor for 3D inventor view.
     * pixmap as a cursor image in device independent pixels.
     *
     * \param autoScale - set this to false if pixmap already scaled for HiDPI
     **/

    /** @name Icon helpers */
    //@{
    void setCursor(const QPixmap& pixmap, int x, int y, bool autoScale = true);

    /// updates the actCursor with the icon by calling getCrosshairCursorSVGName(),
    /// enabling to set data member dependent icons (i.e. for different construction methods)
    void updateCursor();

    /// restitutes the cursor that was in use at the moment of starting the DrawSketchHandler (i.e.
    /// oldCursor)
    void unsetCursor();

    /// restitutes the DSH cached cursor (e.g. without any tail due to autoconstraints, ...)
    void applyCursor();

    /// returns the color to be used for the crosshair (configurable as a parameter)
    unsigned long getCrosshairColor();

    /// functions to set the cursor to a given svgName (to be migrated to NVI style)

    qreal devicePixelRatio();
    //@}

    void drawEdit(const std::vector<Base::Vector2d>& EditCurve);
    void drawEdit(const std::list<std::vector<Base::Vector2d>>& list);
    void drawEdit(const std::vector<Part::Geometry*>& geometries);
    void drawEditMarkers(const std::vector<Base::Vector2d>& EditMarkers,
                         unsigned int augmentationlevel = 0);
    void setAxisPickStyle(bool on);
    void moveCursorToSketchPoint(Base::Vector2d point);
    void preselectAtPoint(Base::Vector2d point);

    void drawPositionAtCursor(const Base::Vector2d& position);
    void drawDirectionAtCursor(const Base::Vector2d& position, const Base::Vector2d& origin);

    int getPreselectPoint() const;
    int getPreselectCurve() const;
    int getPreselectCross() const;

    Sketcher::SketchObject* getSketchObject();

    void setAngleSnapping(bool enable, Base::Vector2d referencePoint = Base::Vector2d(0., 0.));

    void moveConstraint(int constNum, const Base::Vector2d& toPos);

private:
    void setSvgCursor(const QString& svgName,
                      int x,
                      int y,
                      const std::map<unsigned long, unsigned long>& colorMapping =
                          std::map<unsigned long, unsigned long>());

    void addCursorTail(std::vector<QPixmap>& pixmaps);

    void applyCursor(QCursor& newCursor);

    void setCrosshairCursor(const QString& svgName);
    void setCrosshairCursor(const char* svgName);

protected:
    /**
     * Returns constraints icons scaled to width.
     **/
    std::vector<QPixmap>
    suggestedConstraintsPixmaps(std::vector<AutoConstraint>& suggestedConstraints);

    ViewProviderSketch* sketchgui;
    QCursor oldCursor;
    QCursor actCursor;
    QPixmap actCursorPixmap;
};


}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandler_H
