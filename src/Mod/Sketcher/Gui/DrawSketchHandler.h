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

namespace Gui {
    class View3DInventorViewer;
    class SoFCSelection;
}

namespace Sketcher {
    class Sketch;
    class SketchObject;
}

namespace SketcherGui {

class ViewProviderSketch;

// A Simple data type to hold basic information for suggested constraints
struct AutoConstraint
{
    enum TargetType
    {
        VERTEX,
        CURVE
    };
    Sketcher::ConstraintType Type;
    int GeoId;
    Sketcher::PointPos PosId;
};

/** Handler to create new sketch geometry
  * This class has to be reimplemented to create geometry in the
  * sketcher while its in editing.
  */
class SketcherGuiExport DrawSketchHandler
{
public:
    DrawSketchHandler();
    virtual ~DrawSketchHandler();

    virtual void activated(ViewProviderSketch *sketchgui){};
    virtual void deactivated(ViewProviderSketch *sketchgui){};
    virtual void mouseMove(Base::Vector2D onSketchPos)=0;
    virtual bool pressButton(Base::Vector2D onSketchPos)=0;
    virtual bool releaseButton(Base::Vector2D onSketchPos)=0;
    virtual bool onSelectionChanged(const Gui::SelectionChanges& msg) { return false; };
    virtual void registerPressedKey(bool pressed, int key){};

    virtual void quit(void);

    friend class ViewProviderSketch;

    // get the actual highest vertex index, the next use will be +1
    int getHighestVertexIndex(void);
    // get the actual highest edge index, the next use will be +1
    int getHighestCurveIndex(void);

    int seekAutoConstraint(std::vector<AutoConstraint> &suggestedConstraints,
                           const Base::Vector2D &Pos, const Base::Vector2D &Dir,
                           AutoConstraint::TargetType type = AutoConstraint::VERTEX);
    void createAutoConstraints(const std::vector<AutoConstraint> &autoConstrs,
                               int geoId, Sketcher::PointPos pointPos=Sketcher::none);

    void setPositionText(const Base::Vector2D &Pos, const SbString &text);
    void setPositionText(const Base::Vector2D &Pos);
    void resetPositionText(void);
    void renderSuggestConstraintsCursor(std::vector<AutoConstraint> &suggestedConstraints);

protected:
    // helpers
    void setCursor( const QPixmap &p,int x,int y );
    void unsetCursor(void);
    void applyCursor(void);
    void applyCursor(QCursor &newCursor);

    ViewProviderSketch *sketchgui;
    QCursor oldCursor;
    QCursor actCursor;
};


} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandler_H

