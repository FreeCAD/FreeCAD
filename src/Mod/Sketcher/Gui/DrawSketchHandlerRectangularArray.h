/***************************************************************************
 *   Copyright (c) 2022 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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


#ifndef SKETCHERGUI_DrawSketchHandlerRectangularArray_H
#define SKETCHERGUI_DrawSketchHandlerRectangularArray_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "Utils.h"

namespace SketcherGui {

// TODO: replace XPM cursor with SVG file
/* XPM */
static const char *cursor_createrectangulararray[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "................................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    ".......+........................",
    "................................",
    ".+++++...+++++..................",
    ".......................###......",
    ".......+...............###......",
    ".......+...............###......",
    ".......+...............###......",
    ".......+......###......###......",
    ".......+......###......###......",
    "..............###......###......",
    "..............###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###......###......",
    ".....###......###...............",
    ".....###......###...............",
    ".....###......###...............",
    ".....###......###...............",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    ".....###........................",
    "................................",
    "................................"};

class DrawSketchHandlerRectangularArray: public DrawSketchHandler
{
public:
    DrawSketchHandlerRectangularArray(std::string geoidlist, int origingeoid,
                                      Sketcher::PointPos originpos, int nelements, bool clone,
                                      int rows, int cols, bool constraintSeparation,
                                      bool equalVerticalHorizontalSpacing)
        : Mode(STATUS_SEEK_First)
        , snapMode(SnapMode::Free)
        , geoIdList(geoidlist)
        , OriginGeoId(origingeoid)
        , OriginPos(originpos)
        , nElements(nelements)
        , Clone(clone)
        , Rows(rows)
        , Cols(cols)
        , ConstraintSeparation(constraintSeparation)
        , EqualVerticalHorizontalSpacing(equalVerticalHorizontalSpacing)
        , EditCurve(2)
    {
    }

    virtual ~DrawSketchHandlerRectangularArray(){}
    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_End
    };

    enum class SnapMode {
        Free,
        Snap5Degree
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {

            if(QApplication::keyboardModifiers() == Qt::ControlModifier)
                    snapMode = SnapMode::Snap5Degree;
                else
                    snapMode = SnapMode::Free;

            float length = (onSketchPos - EditCurve[0]).Length();
            float angle = (onSketchPos - EditCurve[0]).Angle();

            Base::Vector2d endpoint = onSketchPos;

            if (snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI/36)) * M_PI/36;
                endpoint = EditCurve[0] + length * Base::Vector2d(cos(angle),sin(angle));
            }

            SbString text;
            text.sprintf(" (%.1f, %.1fdeg)", length, angle * 180 / M_PI);
            setPositionText(endpoint, text);

            EditCurve[1] = endpoint;
            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr1, endpoint, Base::Vector2d(0.0, 0.0), AutoConstraint::VERTEX))
            {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }

        }
        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d) override
    {
        if (Mode == STATUS_SEEK_First) {
            drawEdit(EditCurve);
            Mode = STATUS_End;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_End) {
            Base::Vector2d vector = EditCurve[1] - EditCurve[0];
            unsetCursor();
            resetPositionText();

            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create copy of geometry"));

            try {
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "addRectangularArray(%s, App.Vector(%f, %f, 0), %s, %d, %d, %s, %f)",
                                      geoIdList.c_str(), vector.x, vector.y,
                                      (Clone ? "True" : "False"),
                                      Cols, Rows,
                                      (ConstraintSeparation ? "True" : "False"),
                                      (EqualVerticalHorizontalSpacing ? 1.0 : 0.5));
                Gui::Command::commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();
            }

            // add auto constraints for the destination copy
            if (sugConstr1.size() > 0) {
                createAutoConstraints(sugConstr1, OriginGeoId+nElements, OriginPos);
                sugConstr1.clear();
            }
            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            EditCurve.clear();
            drawEdit(EditCurve);

            // no code after this line, Handler is deleted in ViewProvider
            sketchgui->purgeHandler();
        }
        return true;
    }
private:
    virtual void activated() override
    {
        setCursor(QPixmap(cursor_createrectangulararray), 7, 7);
        Origin = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->getPoint(OriginGeoId, OriginPos);
        EditCurve[0] = Base::Vector2d(Origin.x, Origin.y);
    }
protected:
    SelectMode Mode;
    SnapMode snapMode;
    std::string geoIdList;
    Base::Vector3d Origin;
    int OriginGeoId;
    Sketcher::PointPos OriginPos;
    int nElements;
    bool Clone;
    int Rows;
    int Cols;
    bool ConstraintSeparation;
    bool EqualVerticalHorizontalSpacing;
    std::vector<Base::Vector2d> EditCurve;
    std::vector<AutoConstraint> sugConstr1;
};

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerRectangularArray_H

