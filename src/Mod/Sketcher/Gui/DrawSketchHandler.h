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

#include <Base/Tools2D.h>
#include <Mod/Sketcher/App/Constraint.h>
#include <Gui/Selection.h>

class QPixmap;

namespace Sketcher {
    class Sketch;
    class SketchObject;
}

namespace SketcherGui {

class ViewProviderSketch;



/**
 * Class to convert Part::Geometry to Vector2d based collections
 */
class CurveConverter final : public ParameterGrp::ObserverType {

public:
    CurveConverter();

    ~CurveConverter();

    std::vector<Base::Vector2d> toVector2D(const Part::Geometry * geometry);

    std::list<std::vector<Base::Vector2d>> toVector2DList(const std::vector<Part::Geometry *> &geometries);

private:
    void updateCurvedEdgeCountSegmentsParameter();

    /** Observer for parameter group. */
    virtual void OnChange(Base::Subject<const char*> &rCaller, const char * sReason) override;

private:
    int curvedEdgeCountSegments;
};


/**
 * In order to enforce a certain degree of encapsulation and promote a not
 * too tight coupling, while still allowing well defined collaboration,
 * DrawSketchHandler accesses ViewProviderSketch via this Attorney class
 */
class ViewProviderSketchDrawSketchHandlerAttorney {
private:
    static inline void setConstraintSelectability(ViewProviderSketch &vp, bool enabled = true);
    static inline void setPositionText(ViewProviderSketch &vp, const Base::Vector2d &Pos, const SbString &txt);
    static inline void setPositionText(ViewProviderSketch &vp, const Base::Vector2d &Pos);
    static inline void resetPositionText(ViewProviderSketch &vp);
    static inline void drawEdit(ViewProviderSketch &vp, const std::vector<Base::Vector2d> &EditCurve);
    static inline void drawEdit(ViewProviderSketch &vp, const std::list<std::vector<Base::Vector2d>> &list);
    static inline void drawEditMarkers(ViewProviderSketch &vp, const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel = 0);
    static inline void setAxisPickStyle(ViewProviderSketch &vp, bool on);

    static inline int getPreselectPoint(const ViewProviderSketch &vp);
    static inline int getPreselectCurve(const ViewProviderSketch &vp);
    static inline int getPreselectCross(const ViewProviderSketch &vp);

    static inline void signalToolChanged(const ViewProviderSketch &vp, const std::string &toolname);

    friend class DrawSketchHandler;
};

// A Simple data type to hold basic information for suggested constraints
struct AutoConstraint
{
    enum TargetType
    {
        VERTEX,
        CURVE,
        VERTEX_NO_TANGENCY
    };
    Sketcher::ConstraintType Type;
    int GeoId;
    Sketcher::PointPos PosId;
};

/** Handler to create new sketch geometry
  * This class has to be reimplemented to create geometry in the
  * sketcher while its in editing.
  *
  * DrawSketchHandler takes over the responsibility of drawing edit temporal curves and
  * markers necessary to enable visual feedback to the user, as well as the UI interaction during
  * such edits. This is its exclusive responsibility under the Single Responsibility Principle.
  *
  * A plethora of speciliased handlers derive from DrawSketchHandler for each specialised editing (see
  * for example all the handlers for creation of new geometry). These derived classes do * not * have
  * direct access to the ViewProviderSketchDrawSketchHandlerAttorney. This is intended to keep coupling
  * under control. However, generic functionality requiring access to the Attorney can be implemented
  * in DrawSketchHandler and used from its derived classes by virtue of the inheritance. This promotes a
  * concentrating the coupling in a single point (and code reuse).
  */
class SketcherGuiExport DrawSketchHandler
{
public:
    DrawSketchHandler();
    virtual ~DrawSketchHandler();

    void activate(ViewProviderSketch *);
    void deactivate();

    virtual void mouseMove(Base::Vector2d onSketchPos)=0;
    virtual bool pressButton(Base::Vector2d onSketchPos)=0;
    virtual bool releaseButton(Base::Vector2d onSketchPos)=0;
    virtual bool onSelectionChanged(const Gui::SelectionChanges&) { return false; }
    virtual void registerPressedKey(bool /*pressed*/, int /*key*/){}

    virtual void quit(void);

    friend class ViewProviderSketch;

    // get the actual highest vertex index, the next use will be +1
    int getHighestVertexIndex(void);
    // get the actual highest edge index, the next use will be +1
    int getHighestCurveIndex(void);

    int seekAutoConstraint(std::vector<AutoConstraint> &suggestedConstraints,
                           const Base::Vector2d &Pos, const Base::Vector2d &Dir,
                           AutoConstraint::TargetType type = AutoConstraint::VERTEX);
    // createowncommand indicates whether a separate command shall be create and committed (for example for undo purposes) or not
    // is not it is the responsibility of the developer to create and commit the command appropriately.
    void createAutoConstraints(const std::vector<AutoConstraint> &autoConstrs,
                               int geoId, Sketcher::PointPos pointPos=Sketcher::PointPos::none, bool createowncommand = true);

    void setPositionText(const Base::Vector2d &Pos, const SbString &text);
    void setPositionText(const Base::Vector2d &Pos);
    void resetPositionText(void);
    void renderSuggestConstraintsCursor(std::vector<AutoConstraint> &suggestedConstraints);

    void toolWidgetChanged(QWidget * newwidget);

private:
    virtual void preActivated();
    virtual void activated(){}
    virtual void deactivated(){}
    virtual void postDeactivated(){}
    virtual void onWidgetChanged(){}

protected:
    // helpers
    /**
     * Sets a cursor for 3D inventor view.
     * pixmap as a cursor image in device independent pixels.
     *
     * \param autoScale - set this to false if pixmap already scaled for HiDPI
     **/
    void setCursor(const QPixmap &pixmap, int x,int y, bool autoScale=true);
    void setSvgCursor(const QString &svgName, int x, int y,
                      const std::map<unsigned long, unsigned long>& colorMapping = std::map<unsigned long, unsigned long>());
    void addCursorTail(std::vector<QPixmap> &pixmaps);
    void unsetCursor(void);
    void applyCursor(void);
    void applyCursor(QCursor &newCursor);
    unsigned long getCrosshairColor();
    qreal devicePixelRatio();
    void setCrosshairCursor(const QString & svgName);
    void setCrosshairCursor(const char* svgName);

    void drawEdit(const std::vector<Base::Vector2d> &EditCurve);
    void drawEdit(const std::list<std::vector<Base::Vector2d>> &list);
    void drawEdit(const std::vector<Part::Geometry *> &geometries);
    void drawEditMarkers(const std::vector<Base::Vector2d> &EditMarkers, unsigned int augmentationlevel = 0);
    void setAxisPickStyle(bool on);

    int getPreselectPoint(void) const;
    int getPreselectCurve(void) const;
    int getPreselectCross(void) const;

    virtual std::string getToolName() const;
    virtual QString getCrosshairCursorString() const;

    void signalToolChanged() const;

    /**
     * Returns constraints icons scaled to width.
     **/
    std::vector<QPixmap> suggestedConstraintsPixmaps(
            std::vector<AutoConstraint> &suggestedConstraints);

    ViewProviderSketch *sketchgui;
    QCursor oldCursor;
    QCursor actCursor;
    QPixmap actCursorPixmap;

    QWidget * toolwidget;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandler_H

