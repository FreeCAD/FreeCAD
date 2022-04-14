/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <Inventor/events/SoKeyboardEvent.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <QApplication>
# include <QInputDialog>
# include <QMessageBox>
# include <QString>
# include <cstdlib>
# include <qdebug.h>
# include <GC_MakeEllipse.hxx>
# include <boost/math/special_functions/fpclassify.hpp>
# include <memory>
#endif

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Tools.h>

#include <App/OriginFeature.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Document.h>
#include <Gui/CommandT.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludePropertyExternal.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Part/App/DatumFeature.h>
#include <Mod/Part/App/BodyBase.h>
#include <Mod/Part/App/Geometry2d.h>
#include <Mod/Sketcher/App/Constraint.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "Utils.h"

#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/SoFCUnifiedSelection.h>

#include <Gui/ToolBarManager.h>

#include "GeometryCreationMode.h"

#include "SketcherRegularPolygonDialog.h"

#include "SketcherToolDefaultWidget.h"

#include "DrawSketchDefaultWidgetHandler.h"

using namespace std;
using namespace SketcherGui;

namespace bp = boost::placeholders;

namespace SketcherGui {
GeometryCreationMode geometryCreationMode=Normal;
}

/* helper functions ======================================================*/

// Return counter-clockwise angle from horizontal out of p1 to p2 in radians.
double GetPointAngle (const Base::Vector2d &p1, const Base::Vector2d &p2)
{
  double dX = p2.x - p1.x;
  double dY = p2.y - p1.y;
  return dY >= 0 ? atan2(dY, dX) : atan2(dY, dX) + 2*M_PI;
}

void ActivateHandler(Gui::Document *doc, DrawSketchHandler *handler)
{
    std::unique_ptr<DrawSketchHandler> ptr(handler);
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch* vp = static_cast<SketcherGui::ViewProviderSketch*> (doc->getInEdit());
            vp->purgeHandler();
            vp->activateHandler(ptr.release());
        }
    }
}

bool isCreateGeoActive(Gui::Document *doc)
{
    if (doc) {
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom
            (SketcherGui::ViewProviderSketch::getClassTypeId())) {
            /*if (dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())->
                getSketchMode() == ViewProviderSketch::STATUS_NONE)*/
                return true;
        }
    }
    return false;
}

SketcherGui::ViewProviderSketch* getSketchViewprovider(Gui::Document *doc)
{
    if (doc) {
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom
            (SketcherGui::ViewProviderSketch::getClassTypeId()) )
            return dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
    }
    return nullptr;
}

void removeRedundantHorizontalVertical(Sketcher::SketchObject* psketch,
                                       std::vector<AutoConstraint> &sug1,
                                       std::vector<AutoConstraint> &sug2)
{
    if(!sug1.empty() && !sug2.empty()) {

        bool rmvhorvert = false;

        // we look for:
        // 1. Coincident to external on both endpoints
        // 2. Coincident in one endpoint to origin and pointonobject/tangent to an axis on the other
        auto detectredundant = [psketch](std::vector<AutoConstraint> &sug, bool &ext, bool &orig, bool &axis) {

            ext = false;
            orig = false;
            axis = false;

            for(std::vector<AutoConstraint>::const_iterator it = sug.begin(); it!=sug.end(); ++it) {
                if( (*it).Type == Sketcher::Coincident && ext == false) {
                    const std::map<int, Sketcher::PointPos> coincidents = psketch->getAllCoincidentPoints((*it).GeoId, (*it).PosId);

                    if(!coincidents.empty()) {
                        // the keys are ordered, so if the first is negative, it is coincident with external
                        ext = coincidents.begin()->first < 0;

                        std::map<int, Sketcher::PointPos>::const_iterator geoId1iterator;

                        geoId1iterator = coincidents.find(-1);

                        if( geoId1iterator != coincidents.end()) {
                            if( (*geoId1iterator).second == Sketcher::PointPos::start )
                                orig = true;
                        }
                    }
                    else { // it may be that there is no constraint at all, but there is external geometry
                        ext = (*it).GeoId < 0;
                        orig = ((*it).GeoId == -1 && (*it).PosId == Sketcher::PointPos::start);
                    }
                }
                else if( (*it).Type == Sketcher::PointOnObject && axis == false) {
                    axis = (((*it).GeoId == -1 && (*it).PosId == Sketcher::PointPos::none) || ((*it).GeoId == -2 && (*it).PosId == Sketcher::PointPos::none));
                }

            }
        };

        bool firstext = false, secondext = false, firstorig = false, secondorig = false, firstaxis = false, secondaxis = false;

        detectredundant(sug1, firstext, firstorig, firstaxis);
        detectredundant(sug2, secondext, secondorig, secondaxis);


        rmvhorvert = ((firstext && secondext)   ||  // coincident with external on both endpoints
                      (firstorig && secondaxis) ||  // coincident origin and point on object on other
                      (secondorig && firstaxis));

        if(rmvhorvert) {
            for(std::vector<AutoConstraint>::reverse_iterator it = sug2.rbegin(); it!=sug2.rend(); ++it) {
                if( (*it).Type == Sketcher::Horizontal || (*it).Type == Sketcher::Vertical) {
                    sug2.erase(std::next(it).base());
                    it = sug2.rbegin(); // erase invalidates the iterator
                }
            }
        }
    }
}

void ConstraintToAttachment(Sketcher::GeoElementId element, Sketcher::GeoElementId attachment, double distance, App::DocumentObject* obj) {
    if (distance == 0.) {

        if(attachment.isCurve()) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                element.GeoId, element.posIdAsInt(), attachment.GeoId);

        }
        else {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                element.GeoId, element.posIdAsInt(), attachment.GeoId, attachment.posIdAsInt());
        }
    }
    else {
        if(attachment == Sketcher::GeoElementId::VAxis) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
                element.GeoId, element.posIdAsInt(), distance);
        }
        else if(attachment == Sketcher::GeoElementId::HAxis) {
            Gui::cmdAppObjectArgs(obj, "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
                element.GeoId, element.posIdAsInt(), distance);
        }
    }
}



/* Sketch commands =======================================================*/

// DrawSketchHandlerLine: An example of using DrawSketchDefaultWidgetHandler and specialisation for both handler and widget

class DrawSketchHandlerLine;

namespace SketcherGui::ConstructionMethods {

enum class LineConstructionMethod {
    OnePointLengthAngle,
    TwoPoints,
    End // Must be the last one
};

}

using DrawSketchHandlerLineBase = DrawSketchDefaultWidgetHandler<   DrawSketchHandlerLine,
                                                                    /*SelectModeT*/ StateMachines::TwoSeekEnd,
                                                                    /*PEditCurveSize =*/ 2,
                                                                    /*PAutoConstraintSize =*/ 2,
                                                                    /*WidgetParametersT =*/WidgetParameters<4, 4>,
                                                                    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
                                                                    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                                                    ConstructionMethods::LineConstructionMethod,
                                                                    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerLine: public DrawSketchHandlerLineBase
{
    friend DrawSketchHandlerLineBase; // allow DrawSketchHandlerRectangleBase specialisations access DrawSketchHandlerRectangle private members
public:
    DrawSketchHandlerLine(ConstructionMethod constrMethod = ConstructionMethod::OnePointLengthAngle): DrawSketchHandlerLineBase(constrMethod){};
    virtual ~DrawSketchHandlerLine() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch(state()) {
            case SelectMode::SeekFirst:
            {
                drawPositionAtCursor(onSketchPos);

                EditCurve[0] = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f,0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                drawDirectionAtCursor(onSketchPos, EditCurve[0]);

                EditCurve[1] = onSketchPos;

                drawEdit(EditCurve);

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, onSketchPos - EditCurve[0])) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            break;
            default:
                break;
        }
    }

    virtual void executeCommands() override {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch line"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                        EditCurve[0].x,EditCurve[0].y,EditCurve[1].x,EditCurve[1].y,
                        geometryCreationMode==Construction?"True":"False");

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add line: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {
        if(avoidRedundants)
            removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),sugConstraints[0],sugConstraints[1]);

        // add auto constraints for the line segment start
        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::start);
            sugConstraints[0].clear();
        }

        // add auto constraints for the line segment end
        if (!sugConstraints[1].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), Sketcher::PointPos::end);
            sugConstraints[1].clear();
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Line";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_Line");
    }

};

// Function responsible for updating the DrawSketchHandler data members when widget parameters change
template <> void DrawSketchHandlerLineBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
        switch(parameterindex) {
            case WParameter::First:
                handler->EditCurve[0].x = value;
                break;
            case WParameter::Second:
                handler->EditCurve[0].y = value;
                break;
            case WParameter::Third:
            {
                auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                auto endpoint = handler->EditCurve[0] + Base::Vector2d::FromPolar(value, angle);
                handler->EditCurve[1].x =  endpoint.x;
            }
                break;
            case WParameter::Fourth:
            {
                auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                auto endpoint = handler->EditCurve[0] + Base::Vector2d::FromPolar(value, angle);
                handler->EditCurve[1].y = endpoint.y;
            }
                break;
        }
    }
    else { // ConstructionMethod::TwoPoints
        switch(parameterindex) {
            case WParameter::First:
                handler->EditCurve[0].x = value;
                break;
            case WParameter::Second:
                handler->EditCurve[0].y = value;
                break;
            case WParameter::Third:
                handler->EditCurve[1].x = value;
                break;
            case WParameter::Fourth:
                handler->EditCurve[1].y = value;
                break;
        }
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::configureToolWidget() {
    if (!init) { // Code to be executed only upon initialisation
        QStringList names = { QStringLiteral("Point, length, angle"), QStringLiteral("2 points") };
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_line", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_line", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_length_line", "Length"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_angle_line", "Angle"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_line", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_line", "y of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_line", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_line", "x of 2nd point"));
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {

            double length = (onSketchPos - dHandler->EditCurve[0]).Length();

            if (toolWidget->isParameterSet(WParameter::Third)) {
                length = toolWidget->getParameter(WParameter::Third);
                Base::Vector2d v = onSketchPos - dHandler->EditCurve[0];
                onSketchPos = dHandler->EditCurve[0] + v * length / v.Length();
            }

            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                onSketchPos.x = dHandler->EditCurve[0].x + cos(angle) * length;
                onSketchPos.y = dHandler->EditCurve[0].y + sin(angle) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerLineBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (handler->constructionMethod() == ConstructionMethod::OnePointLengthAngle) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, (onSketchPos - handler->EditCurve[0]).Length());

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, (onSketchPos - handler->EditCurve[0]).Angle() * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth,onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

// Function responsible to add widget mandated constraints (it is executed before creating autoconstraints)
template <> void DrawSketchHandlerLineBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);
    auto p3 = toolWidget->getParameter(WParameter::Third);
    auto p4 = toolWidget->getParameter(WParameter::Fourth);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto p3set = toolWidget->isParameterSet(WParameter::Third);
    auto p4set = toolWidget->isParameterSet(WParameter::Fourth);

    using namespace Sketcher;

    if(x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::RtPnt,
                                    x0, handler->sketchgui->getObject());
    } else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::VAxis,
                                    x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::HAxis,
                                    y0,  handler->sketchgui->getObject());
    }

    if (handler->constructionMethod() == DrawSketchHandlerLine::ConstructionMethod::OnePointLengthAngle) {

        if (p3set)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                firstCurve, p3);

        if (p4set) {
            double angle = p4 / 180 * M_PI;
            if (fabs(angle - M_PI) < Precision::Confusion() || fabs(angle + M_PI) < Precision::Confusion() || fabs(angle) < Precision::Confusion()) {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", firstCurve);
            }
            else if (fabs(angle - M_PI / 2) < Precision::Confusion() || fabs(angle + M_PI / 2) < Precision::Confusion()) {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d)) ", firstCurve);
            }
            else {
                Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                    Sketcher::GeoEnum::HAxis, firstCurve, angle);
            }
        }
    }
    else {
        if (p3set && p4set && p3 == 0. && p4 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::end), GeoElementId::RtPnt,
                p3, handler->sketchgui->getObject());
        }
        else {
            if (p3set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::end), GeoElementId::VAxis,
                    p3, handler->sketchgui->getObject());

            if (p4set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::end), GeoElementId::HAxis,
                    p4, handler->sketchgui->getObject());
        }
    }
}

DEF_STD_CMD_AU(CmdSketcherCreateLine)

CmdSketcherCreateLine::CmdSketcherCreateLine()
  : Command("Sketcher_CreateLine")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create line");
    sToolTipText    = QT_TR_NOOP("Create a line in the sketch");
    sWhatsThis      = "Sketcher_CreateLine";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateLine";
    sAccel          = "G, L";
    eType           = ForEdit;
}

void CmdSketcherCreateLine::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLine() );
}

void CmdSketcherCreateLine::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateLine_Constr"));
        break;
    }
}

bool CmdSketcherCreateLine::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


/* Create Rectangle =======================================================*/

// DrawSketchHandlerRectangle: An example of deriving from DrawSketchDefaultWidgetHandler with NVI for handler and specialisation for widgetmanager.

class DrawSketchHandlerRectangle;

namespace SketcherGui::ConstructionMethods {

enum class RectangleConstructionMethod {
    Diagonal,
    CenterAndCorner,
    End // Must be the last one
};

}

using DrawSketchHandlerRectangleBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerRectangle,
                                                                        StateMachines::FourSeekEnd,
                                                                        /*PEditCurveSize =*/ 5,
                                                                        /*PAutoConstraintSize =*/ 2,
                                                                        /*WidgetParametersT =*/WidgetParameters<4, 4>,
                                                                        /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
                                                                        /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
                                                                        ConstructionMethods::RectangleConstructionMethod,
                                                                        /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerRectangle: public DrawSketchHandlerRectangleBase
{
    friend DrawSketchHandlerRectangleBase; // allow DrawSketchHandlerRectangleBase specialisations access DrawSketchHandlerRectangle private members
public:

    DrawSketchHandlerRectangle(ConstructionMethod constrMethod = ConstructionMethod::Diagonal) :
        DrawSketchHandlerRectangleBase(constrMethod),
        roundCorners(false),
        makeFrame(false),
        thickness(0.) {}

    virtual ~DrawSketchHandlerRectangle() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch(state()) {
            case SelectMode::SeekFirst:
            {
                drawPositionAtCursor(onSketchPos);

                if(constructionMethod() == ConstructionMethod::Diagonal)
                    firstCorner = onSketchPos;
                else //(constructionMethod == ConstructionMethod::CenterAndCorner)
                    center = onSketchPos;

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f,0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            break;
            case SelectMode::SeekSecond:
            {
                if(constructionMethod() == ConstructionMethod::Diagonal) {
                    drawDirectionAtCursor(onSketchPos, firstCorner);

                    thirdCorner = onSketchPos;
                    secondCorner = Base::Vector2d(onSketchPos.x ,firstCorner.y);
                    fourthCorner = Base::Vector2d(firstCorner.x,onSketchPos.y);

                }
                else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
                    drawDirectionAtCursor(onSketchPos, center);

                    firstCorner = center - (onSketchPos - center);
                    secondCorner = Base::Vector2d(onSketchPos.x, firstCorner.y);
                    thirdCorner = onSketchPos;
                    fourthCorner = Base::Vector2d(firstCorner.x, onSketchPos.y);
                }

                if (roundCorners) {
                    if (fabs(length) > fabs(width)) {
                        radius = fabs(width) / 6;
                    }
                    else {
                        radius = fabs(length) / 6;
                    }
                }
                else {
                    radius = 0.;
                }

                try {
                    drawEdit(getRectangleGeometries());
                }
                catch(const Base::ValueError &) {} // equal points while hovering raise an objection that can be safely ignored

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.0,0.0))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            break;
            case SelectMode::SeekThird:
            {
                double dx, dy, minX, minY, maxX, maxY;
                minX = min(firstCorner.x, thirdCorner.x);
                maxX = max(firstCorner.x, thirdCorner.x);
                minY = min(firstCorner.y, thirdCorner.y);
                maxY = max(firstCorner.y, thirdCorner.y);

                if (roundCorners) {
                    if (onSketchPos.x < minX || onSketchPos.y < minY || onSketchPos.x > maxX || onSketchPos.y > maxY) {
                        radius = 0.;
                    }
                    else {
                        dx = onSketchPos.x - minX;
                        dy = onSketchPos.y - minY;
                        if (dx < abs(length / 2)) {
                            dx = (onSketchPos.x - minX);
                        }
                        else {
                            dx = -(onSketchPos.x - maxX);
                        }
                        dy = onSketchPos.y - minY;
                        if (dy < abs(width / 2)) {
                            dy = (onSketchPos.y - minY);
                        }
                        else {
                            dy = -(onSketchPos.y - maxY);
                        }
                        radius = min((dx + dy + sqrt(2 * dx * dy)), min(abs(length / 2), abs(width / 2)) * 0.99);
                    }
                   SbString text;
                    text.sprintf(" (%.1f radius)", radius);
                    setPositionText(onSketchPos, text);
                }
                else {//This is the case of frame of normal rectangle.

                    dx = min(abs(onSketchPos.x - minX), abs(onSketchPos.x - maxX));
                    dy = min(abs(onSketchPos.y - minY), abs(onSketchPos.y - maxY));
                    if (onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0 && !(onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0))
                        thickness = dy;
                    else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && !(onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0))
                        thickness = dx;
                    else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0)
                        thickness = -min(dx, dy);
                    else
                        thickness = max(dx, dy);

                    firstCornerFrame.x = firstCorner.x == minX ? minX - thickness : maxX + thickness;
                    firstCornerFrame.y = firstCorner.y == minY ? minY - thickness : maxY + thickness;
                    thirdCornerFrame.x = thirdCorner.x == minX ? minX - thickness : maxX + thickness;
                    thirdCornerFrame.y = thirdCorner.y == minY ? minY - thickness : maxY + thickness;
                    secondCornerFrame = Base::Vector2d(thirdCornerFrame.x, firstCornerFrame.y);
                    fourthCornerFrame = Base::Vector2d(firstCornerFrame.x, thirdCornerFrame.y);

                    SbString text;
                    text.sprintf(" (%.1fT)", thickness);
                    setPositionText(onSketchPos, text);
                }

                drawEdit(getRectangleGeometries());
            }
            break;
            case SelectMode::SeekFourth:
            {
                //This is the case of frame of round corner rectangle.
                double dx, dy, minX, minY, maxX, maxY;
                minX = min(firstCorner.x, thirdCorner.x);
                maxX = max(firstCorner.x, thirdCorner.x);
                minY = min(firstCorner.y, thirdCorner.y);
                maxY = max(firstCorner.y, thirdCorner.y);

                dx = min(abs(onSketchPos.x - minX), abs(onSketchPos.x - maxX));
                dy = min(abs(onSketchPos.y - minY), abs(onSketchPos.y - maxY));
                if (onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0 && !(onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0))
                    thickness = dy;
                else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && !(onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0))
                    thickness = dx;
                else if (onSketchPos.y - minY > 0 && onSketchPos.y - maxY < 0 && onSketchPos.x - minX > 0 && onSketchPos.x - maxX < 0)
                    thickness = -min(dx, dy);
                else
                    thickness = max(dx, dy);

                firstCornerFrame.x = firstCorner.x == minX ? minX - thickness : maxX + thickness;
                firstCornerFrame.y = firstCorner.y == minY ? minY - thickness : maxY + thickness;
                thirdCornerFrame.x = thirdCorner.x == minX ? minX - thickness : maxX + thickness;
                thirdCornerFrame.y = thirdCorner.y == minY ? minY - thickness : maxY + thickness;
                secondCornerFrame = Base::Vector2d(thirdCornerFrame.x, firstCornerFrame.y);
                fourthCornerFrame = Base::Vector2d(firstCornerFrame.x, thirdCornerFrame.y);

                SbString text;
                text.sprintf(" (%.1fT)", thickness);
                setPositionText(onSketchPos, text);

                drawEdit(getRectangleGeometries());
            }
            break;
            default:
                break;
        }
    }

    virtual void executeCommands() override {
        firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch box"));

            //create geometries
            Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
            std::vector<Part::Geometry*> geometriesToAdd = getRectangleGeometries();
            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                Part::GeomPoint* point = new Part::GeomPoint();
                point->setPoint(Base::Vector3d(center.x, center.y, 0.));
                Sketcher::GeometryFacade::setConstruction(point, true);
                geometriesToAdd.push_back(point);

            }

            centerPointId = firstCurve + geometriesToAdd.size() - 1;

            if (fabs(thickness) > Precision::Confusion()) {
                //Create construction lines. Not needed for round corner rectangle.
                if (radius < Precision::Confusion() || (radiusFrame < Precision::Confusion() && radius > Precision::Confusion())) {
                    Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
                    Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                    Part::GeomLineSegment* line3 = new Part::GeomLineSegment();
                    Part::GeomLineSegment* line4 = new Part::GeomLineSegment();
                    if (radius < Precision::Confusion()) {
                        line1->setPoints(Base::Vector3d(firstCorner.x, firstCorner.y, 0.), Base::Vector3d(firstCornerFrame.x, firstCornerFrame.y, 0.));
                        line2->setPoints(Base::Vector3d(secondCorner.x, secondCorner.y, 0.), Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y, 0.));
                        line3->setPoints(Base::Vector3d(thirdCorner.x, thirdCorner.y, 0.), Base::Vector3d(thirdCornerFrame.x, thirdCornerFrame.y, 0.));
                        line4->setPoints(Base::Vector3d(fourthCorner.x, fourthCorner.y, 0.), Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y, 0.));
                    }
                    else {
                        line1->setPoints(arc1Center, Base::Vector3d(firstCornerFrame.x, firstCornerFrame.y, 0.));
                        line2->setPoints(arc2Center, Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y, 0.));
                        line3->setPoints(arc3Center, Base::Vector3d(thirdCornerFrame.x, thirdCornerFrame.y, 0.));
                        line4->setPoints(arc4Center, Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y, 0.));
                    }
                    Sketcher::GeometryFacade::setConstruction(line1, true);
                    Sketcher::GeometryFacade::setConstruction(line2, true);
                    Sketcher::GeometryFacade::setConstruction(line3, true);
                    Sketcher::GeometryFacade::setConstruction(line4, true);
                    geometriesToAdd.push_back(line1);
                    geometriesToAdd.push_back(line2);
                    geometriesToAdd.push_back(line3);
                    geometriesToAdd.push_back(line4);
                }
            }

            Obj->addGeometry(std::move(geometriesToAdd));


            if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                Gui::cmdAppObjectArgs(Obj, "addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d)) ",
                    firstCurve + 1, 2, firstCurve + 3, 2, centerPointId, 1);
            }

            int a = signX * signY > 0. ? 2 : 1;
            int b = signX * signY > 0. ? 1 : 2;
            if (radius > Precision::Confusion()) {
                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                    "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                    "conList.append(Sketcher.Constraint('Vertical', %i))\n"
                    "conList.append(Sketcher.Constraint('Vertical', %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve, firstCurve + 4, a,     // tangent 1
                    firstCurve, firstCurve + 5, b,    // tangent 2
                    firstCurve + 1, firstCurve + 5, a, // tangent 3
                    firstCurve + 1, firstCurve + 6, b, // tangent 4
                    firstCurve + 2, firstCurve + 6, a, // tangent 5
                    firstCurve + 2, firstCurve + 7, b, // tangent 6
                    firstCurve + 3, firstCurve + 7, a, // tangent 7
                    firstCurve + 3, firstCurve + 4, b, // tangent 8
                    firstCurve, // horizontal constraint
                    firstCurve + 2, // horizontal constraint
                    firstCurve + 1, // vertical constraint
                    firstCurve + 3, // vertical constraint
                    firstCurve + 4, firstCurve + 5, // equal  1
                    firstCurve + 5, firstCurve + 6, // equal  2
                    firstCurve + 6, firstCurve + 7, // equal  3
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

                constructionPointOneId = firstCurve + 9;
                constructionPointTwoId = firstCurve + 8;
                if (fabs(thickness) > Precision::Confusion()) {
                    constructionPointOneId += 8;
                    constructionPointTwoId += 8; //+8 in both case because if the 4 arcs are lost then we need 4 construction lines instead.
                    if (radiusFrame > Precision::Confusion()) {
                        Gui::Command::doCommand(Gui::Command::Doc,
                            "conList = []\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, %i))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,3,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,3,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,3,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,3,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                            "conList.append(Sketcher.Constraint('Horizontal', %i))\n"
                            "conList.append(Sketcher.Constraint('Vertical', %i))\n"
                            "%s.addConstraint(conList)\n"
                            "del conList\n",
                            firstCurve + 8, firstCurve + 12, a,    // tangent 1
                            firstCurve + 8, firstCurve + 13, b,    // tangent 2
                            firstCurve + 9, firstCurve + 13, a,    // tangent 3
                            firstCurve + 9, firstCurve + 14, b,    // tangent 4
                            firstCurve + 10, firstCurve + 14, a,   // tangent 5
                            firstCurve + 10, firstCurve + 15, b,   // tangent 6
                            firstCurve + 11, firstCurve + 15, a,   // tangent 7
                            firstCurve + 11, firstCurve + 12, b,   // tangent 8
                            firstCurve + 4, firstCurve + 12,    // coincidence 1
                            firstCurve + 5, firstCurve + 13,    // coincidence 2
                            firstCurve + 6, firstCurve + 14,    // coincidence 3
                            firstCurve + 7, firstCurve + 15,    // coincidence 4
                            firstCurve + 8, // horizontal constraint
                            firstCurve + 10, // horizontal constraint
                            firstCurve + 9, // vertical constraint
                            Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                    }
                    else { //case where inner rectangle lost its arcs.Gui::Command::doCommand(Gui::Command::Doc,
                        Gui::Command::doCommand(Gui::Command::Doc,
                            "conList = []\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"

                            "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,3))\n"
                            "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"

                            "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                            "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                            "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                            "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                            "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                            "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                            "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                            "%s.addConstraint(conList)\n"
                            "del conList\n",
                            firstCurve + 8, firstCurve + 9, // coincident5
                            firstCurve + 9, firstCurve + 10, // coincident6
                            firstCurve + 10, firstCurve + 11, // coincident7
                            firstCurve + 11, firstCurve + 8, // coincident8

                            centerPointId + 1, firstCurve + 4, // coincident1-support
                            centerPointId + 1, firstCurve + 8, // coincident2-support
                            centerPointId + 2, firstCurve + 5, // coincident3-support
                            centerPointId + 2, firstCurve + 9, // coincident4-support
                            centerPointId + 3, firstCurve + 6, // coincident5-support
                            centerPointId + 3, firstCurve + 10, // coincident6-support
                            centerPointId + 4, firstCurve + 7, // coincident7-support
                            centerPointId + 4, firstCurve + 11, // coincident8-support

                            firstCurve + 8, // horizontal1
                            firstCurve + 10, // horizontal2
                            firstCurve + 9, // vertical1
                            firstCurve + 11, // vertical2
                            centerPointId + 1, centerPointId + 2, // Perpendicular of support lines
                            centerPointId + 2, centerPointId + 3, // Perpendicular of support lines
                            centerPointId + 3, centerPointId + 4, // Perpendicular of support lines
                            Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                    }
                }

                if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
                    // now add construction geometry - one point used to take suggested constraints
                    Gui::Command::doCommand(Gui::Command::Doc,
                        "geoList = []\n"
                        "geoList.append(Part.Point(App.Vector(%f, %f, 0)))\n"
                        "%s.addGeometry(geoList, True)\n" // geometry as construction
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "%s.addConstraint(conList)\n"
                        "del geoList, conList\n",
                        thirdCorner.x, thirdCorner.y,     // point at EndPos
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(), // the sketch
                        constructionPointOneId, firstCurve + 1, // point on object constraint
                        constructionPointOneId, firstCurve + 2, // point on object constraint
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                }
                else {
                    // now add construction geometry - two points used to take suggested constraints
                    Gui::Command::doCommand(Gui::Command::Doc,
                        "geoList = []\n"
                        "geoList.append(Part.Point(App.Vector(%f, %f, 0)))\n"
                        "geoList.append(Part.Point(App.Vector(%f, %f, 0)))\n"
                        "%s.addGeometry(geoList, True)\n" // geometry as construction
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "conList.append(Sketcher.Constraint('PointOnObject', %i, 1, %i, ))\n"
                        "%s.addConstraint(conList)\n"
                        "del geoList, conList\n",
                        firstCorner.x, firstCorner.y, // point at StartPos
                        thirdCorner.x, thirdCorner.y,     // point at EndPos
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(), // the sketch
                        constructionPointTwoId, firstCurve, // point on object constraint
                        constructionPointTwoId, firstCurve + 3, // point on object constraint
                        constructionPointOneId, firstCurve + 1, // point on object constraint
                        constructionPointOneId, firstCurve + 2, // point on object constraint
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                }
            }
            else {
                if (fabs(thickness) > Precision::Confusion()) {
                    Gui::Command::doCommand(Gui::Command::Doc,
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"

                        "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"

                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                        "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                        "conList.append(Sketcher.Constraint('Perpendicular',%i,%i))\n"
                        "%s.addConstraint(conList)\n"
                        "del conList\n",
                        firstCurve, firstCurve + 1, // coincident1
                        firstCurve + 1, firstCurve + 2, // coincident2
                        firstCurve + 2, firstCurve + 3, // coincident3
                        firstCurve + 3, firstCurve, // coincident4
                        firstCurve + 4, firstCurve + 5, // coincident5
                        firstCurve + 5, firstCurve + 6, // coincident6
                        firstCurve + 6, firstCurve + 7, // coincident7
                        firstCurve + 7, firstCurve + 4, // coincident8

                        centerPointId + 1, firstCurve, // coincident1-support
                        centerPointId + 1, firstCurve + 4, // coincident2-support
                        centerPointId + 2, firstCurve + 1, // coincident3-support
                        centerPointId + 2, firstCurve + 5, // coincident4-support
                        centerPointId + 3, firstCurve + 2, // coincident5-support
                        centerPointId + 3, firstCurve + 6, // coincident6-support
                        centerPointId + 4, firstCurve + 3, // coincident7-support
                        centerPointId + 4, firstCurve + 7, // coincident8-support

                        firstCurve, // horizontal1
                        firstCurve + 2, // horizontal2
                        firstCurve + 1, // vertical1
                        firstCurve + 3, // vertical2
                        firstCurve + 4, // horizontal3
                        firstCurve + 6, // horizontal4
                        firstCurve + 5, // vertical3
                        firstCurve + 7, // vertical4
                        centerPointId + 1, centerPointId + 2, // Perpendicular of support lines
                        centerPointId + 2, centerPointId + 3, // Perpendicular of support lines
                        centerPointId + 3, centerPointId + 4, // Perpendicular of support lines
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                }
                else {
                    Gui::Command::doCommand(Gui::Command::Doc,
                        "conList = []\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Horizontal',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "conList.append(Sketcher.Constraint('Vertical',%i))\n"
                        "%s.addConstraint(conList)\n"
                        "del conList\n",
                        firstCurve, firstCurve + 1, // coincident1
                        firstCurve + 1, firstCurve + 2, // coincident2
                        firstCurve + 2, firstCurve + 3, // coincident3
                        firstCurve + 3, firstCurve, // coincident4
                        firstCurve, // horizontal1
                        firstCurve + 2, // horizontal2
                        firstCurve + 1, // vertical1
                        firstCurve + 3, // vertical2
                        Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
                }
            }

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add box: %s\n", e.what());
            Gui::Command::abortCommand();
        }

        thickness = 0.;
    }

    virtual void createAutoConstraints() override {
        if(constructionMethod() == ConstructionMethod::Diagonal) {
            // add auto constraints at the start of the first side
            if (radius > Precision::Confusion()) {
                if (!sugConstraints[0].empty()) {
                    DrawSketchHandler::createAutoConstraints(sugConstraints[0], constructionPointTwoId, Sketcher::PointPos::start);
                    sugConstraints[0].clear();
                }

                if (!sugConstraints[1].empty()) {
                    DrawSketchHandler::createAutoConstraints(sugConstraints[1], constructionPointOneId, Sketcher::PointPos::start);
                    sugConstraints[1].clear();
                }
            }
            else {
                if (!sugConstraints[0].empty()) {
                    DrawSketchHandler::createAutoConstraints(sugConstraints[0], firstCurve, Sketcher::PointPos::start);
                    sugConstraints[0].clear();
                }

                if (!sugConstraints[1].empty()) {
                    DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::end);
                    sugConstraints[1].clear();
                }
            }
        }
        else if (constructionMethod() == ConstructionMethod::CenterAndCorner) {
            // add auto constraints at center
            if (!sugConstraints[0].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], centerPointId, Sketcher::PointPos::start);
                sugConstraints[0].clear();
            }

            // add auto constraints for the line segment end
            if (!sugConstraints[1].empty()) {
                if (radius > Precision::Confusion())
                    DrawSketchHandler::createAutoConstraints(sugConstraints[1], constructionPointOneId, Sketcher::PointPos::start);
                else
                    DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::end);

                sugConstraints[1].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Rectangle";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (roundCorners && !makeFrame)
            return QString::fromLatin1("Sketcher_Pointer_Oblong");
        else if (makeFrame)
            return QString::fromLatin1("Sketcher_CreateFrame");
        else
            return QString::fromLatin1("Sketcher_Pointer_Create_Box");
    }

    //reimplement because if not radius then it's 2 steps
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        this->updateDataAndDrawToPosition(onSketchPos);

        if (state() == SelectMode::SeekSecond && !roundCorners && !makeFrame) {
            setState(SelectMode::End);
        }
        else if ((state() == SelectMode::SeekThird && roundCorners && !makeFrame) || (state() == SelectMode::SeekThird && !roundCorners && makeFrame)) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

private:
    Base::Vector2d center, firstCorner, secondCorner, thirdCorner, fourthCorner, firstCornerFrame, secondCornerFrame, thirdCornerFrame, fourthCornerFrame;
    Base::Vector3d arc1Center, arc2Center, arc3Center, arc4Center;
    bool roundCorners, makeFrame;
    double radius, length, width, thickness, radiusFrame;
    int signX, signY, firstCurve, constructionPointOneId, constructionPointTwoId, centerPointId;

    std::vector<Part::Geometry*> getRectangleGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        length = thirdCorner.x - firstCorner.x;
        width = thirdCorner.y - firstCorner.y;
        signX = Base::sgn(length);
        signY = Base::sgn(width);

        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line3 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line4 = new Part::GeomLineSegment();
        line1->setPoints(Base::Vector3d(firstCorner.x + signX * radius, firstCorner.y, 0.), Base::Vector3d(secondCorner.x - signX * radius, secondCorner.y, 0.));
        line2->setPoints(Base::Vector3d(secondCorner.x, secondCorner.y + signY * radius, 0.), Base::Vector3d(thirdCorner.x, thirdCorner.y - signY * radius, 0.));
        line3->setPoints(Base::Vector3d(thirdCorner.x - signX * radius, thirdCorner.y, 0.), Base::Vector3d(fourthCorner.x + signX * radius, fourthCorner.y, 0.));
        line4->setPoints(Base::Vector3d(fourthCorner.x, fourthCorner.y - signY * radius, 0.), Base::Vector3d(firstCorner.x, firstCorner.y + signY * radius, 0.));
        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line3, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line4, geometryCreationMode);
        geometriesToAdd.push_back(line1);
        geometriesToAdd.push_back(line2);
        geometriesToAdd.push_back(line3);
        geometriesToAdd.push_back(line4);

        if (roundCorners && radius > Precision::Confusion()) {
            double start = 0;
            double end = M_PI / 2;
            if (signX > 0 && signY > 0) {
                start = -M_PI;
                end = -M_PI / 2;
            }
            else if (signX > 0 && signY < 0) {
                start = M_PI / 2;
                end = M_PI;
            }
            else if (signX < 0 && signY > 0) {
                start = -M_PI / 2;
                end = 0;
            }

            Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
            Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
            Part::GeomArcOfCircle* arc3 = new Part::GeomArcOfCircle();
            Part::GeomArcOfCircle* arc4 = new Part::GeomArcOfCircle();

            //center points required later for special case of round corner frame with radiusFrame = 0.
            arc1Center = Base::Vector3d(firstCorner.x + signX * radius, firstCorner.y + signY * radius, 0.);
            arc2Center = Base::Vector3d(secondCorner.x - signX * radius, secondCorner.y + signY * radius, 0.);
            arc3Center = Base::Vector3d(thirdCorner.x - signX * radius, thirdCorner.y - signY * radius, 0.);
            arc4Center = Base::Vector3d(fourthCorner.x + signX * radius, fourthCorner.y - signY * radius, 0.);
            arc1->setCenter(arc1Center);
            arc2->setCenter(arc2Center);
            arc3->setCenter(arc3Center);
            arc4->setCenter(arc4Center);
            arc1->setRange(start, end, true);
            arc2->setRange((signX * signY > 0) ? end - 2 * M_PI : end - M_PI, (signX * signY > 0) ? end - 1.5 * M_PI : end - 0.5 * M_PI, true);
            arc3->setRange(end - 1.5 * M_PI, end - M_PI, true);
            arc4->setRange((signX * signY > 0) ? end - M_PI : end - 2 * M_PI, (signX * signY > 0) ? end - 0.5 * M_PI : end - 1.5 * M_PI, true);
            arc1->setRadius(radius);
            arc2->setRadius(radius);
            arc3->setRadius(radius);
            arc4->setRadius(radius);
            Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(arc2, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(arc3, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(arc4, geometryCreationMode);
            geometriesToAdd.push_back(arc1);
            geometriesToAdd.push_back(arc2);
            geometriesToAdd.push_back(arc3);
            geometriesToAdd.push_back(arc4);

       }

        if (makeFrame && state() != SelectMode::SeekSecond && fabs(thickness) > Precision::Confusion()) {
            if (radius < Precision::Confusion()) {
                radiusFrame = 0.;
            }
            else {
                radiusFrame = radius + thickness;
                if (radiusFrame < 0.) {
                    radiusFrame = 0.;
                }
            }
            Part::GeomLineSegment* line5 = new Part::GeomLineSegment();
            Part::GeomLineSegment* line6 = new Part::GeomLineSegment();
            Part::GeomLineSegment* line7 = new Part::GeomLineSegment();
            Part::GeomLineSegment* line8 = new Part::GeomLineSegment();
            line5->setPoints(Base::Vector3d(firstCornerFrame.x + signX * radiusFrame, firstCornerFrame.y, 0.), Base::Vector3d(secondCornerFrame.x - signX * radiusFrame, secondCornerFrame.y, 0.));
            line6->setPoints(Base::Vector3d(secondCornerFrame.x, secondCornerFrame.y + signY * radiusFrame, 0.), Base::Vector3d(thirdCornerFrame.x, thirdCornerFrame.y - signY * radiusFrame, 0.));
            line7->setPoints(Base::Vector3d(thirdCornerFrame.x - signX * radiusFrame, thirdCornerFrame.y, 0.), Base::Vector3d(fourthCornerFrame.x + signX * radiusFrame, fourthCornerFrame.y, 0.));
            line8->setPoints(Base::Vector3d(fourthCornerFrame.x, fourthCornerFrame.y - signY * radiusFrame, 0.), Base::Vector3d(firstCornerFrame.x, firstCornerFrame.y + signY * radiusFrame, 0.));
            Sketcher::GeometryFacade::setConstruction(line5, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(line6, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(line7, geometryCreationMode);
            Sketcher::GeometryFacade::setConstruction(line8, geometryCreationMode);
            geometriesToAdd.push_back(line5);
            geometriesToAdd.push_back(line6);
            geometriesToAdd.push_back(line7);
            geometriesToAdd.push_back(line8);


            if (roundCorners && radiusFrame > Precision::Confusion()) {
                double start = 0;
                double end = M_PI / 2;
                if (signX > 0 && signY > 0) {
                    start = -M_PI;
                    end = -M_PI / 2;
                }
                else if (signX > 0 && signY < 0) {
                    start = M_PI / 2;
                    end = M_PI;
                }
                else if (signX < 0 && signY > 0) {
                    start = -M_PI / 2;
                    end = 0;
                }

                Part::GeomArcOfCircle* arc5 = new Part::GeomArcOfCircle();
                Part::GeomArcOfCircle* arc6 = new Part::GeomArcOfCircle();
                Part::GeomArcOfCircle* arc7 = new Part::GeomArcOfCircle();
                Part::GeomArcOfCircle* arc8 = new Part::GeomArcOfCircle();
                arc5->setCenter(Base::Vector3d(firstCornerFrame.x + signX * radiusFrame, firstCornerFrame.y + signY * radiusFrame, 0.));
                arc6->setCenter(Base::Vector3d(secondCornerFrame.x - signX * radiusFrame, secondCornerFrame.y + signY * radiusFrame, 0.));
                arc7->setCenter(Base::Vector3d(thirdCornerFrame.x - signX * radiusFrame, thirdCornerFrame.y - signY * radiusFrame, 0.));
                arc8->setCenter(Base::Vector3d(fourthCornerFrame.x + signX * radiusFrame, fourthCornerFrame.y - signY * radiusFrame, 0.));
                arc5->setRange(start, end, true);
                arc6->setRange((signX * signY > 0) ? end - 2 * M_PI : end - M_PI, (signX * signY > 0) ? end - 1.5 * M_PI : end - 0.5 * M_PI, true);
                arc7->setRange(end - 1.5 * M_PI, end - M_PI, true);
                arc8->setRange((signX * signY > 0) ? end - M_PI : end - 2 * M_PI, (signX * signY > 0) ? end - 0.5 * M_PI : end - 1.5 * M_PI, true);
                arc5->setRadius(radiusFrame);
                arc6->setRadius(radiusFrame);
                arc7->setRadius(radiusFrame);
                arc8->setRadius(radiusFrame);
                Sketcher::GeometryFacade::setConstruction(arc5, geometryCreationMode);
                Sketcher::GeometryFacade::setConstruction(arc6, geometryCreationMode);
                Sketcher::GeometryFacade::setConstruction(arc7, geometryCreationMode);
                Sketcher::GeometryFacade::setConstruction(arc8, geometryCreationMode);
                geometriesToAdd.push_back(arc5);
                geometriesToAdd.push_back(arc6);
                geometriesToAdd.push_back(arc7);
                geometriesToAdd.push_back(arc8);
            }
        }

        return geometriesToAdd;
    }
};

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Diagonal corners"), QStringLiteral("Center and corner")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if(dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal){
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rectangle", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rectangle", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (X axis)"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (Y axis)"));
    }
    else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_rectangle", "x of center point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_rectangle", "y of center point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_rectangle", "Length (X axis)"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_rectangle", "Width (Y axis)"));
    }

    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_rectangle", "Corner radius"));
    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_rectangle", "Rounded corners"));
    if (!toolWidget->getCheckboxChecked(WCheckbox::FirstBox)) {
        toolWidget->setParameterVisible(WParameter::Fifth, false);
    }

    toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_rectangle", "Thickness"));
    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_rectangle", "Frame"));
    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_rectangle", "Frame"));
    if (!toolWidget->getCheckboxChecked(WCheckbox::SecondBox)) {
        toolWidget->setParameterVisible(WParameter::Sixth, false);
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if(dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal){
        switch(parameterindex) {
            case WParameter::First:
                dHandler->firstCorner.x = value;
                break;
            case WParameter::Second:
                dHandler->firstCorner.y = value;
                break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::CenterAndCorner)
        switch(parameterindex) {
            case WParameter::First:
                dHandler->center.x = value;
                break;
            case WParameter::Second:
                dHandler->center.y = value;
                break;
        }
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {
    Q_UNUSED(checkboxindex);

    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->roundCorners = value;
        toolWidget->setParameterVisible(WParameter::Fifth, value);
        break;
    case WCheckbox::SecondBox:
        dHandler->makeFrame = value;
        toolWidget->setParameterVisible(WParameter::Sixth, value);
        break;
    }

    handler->updateDataAndDrawToPosition(prevCursorPosition);
    onHandlerModeChanged(); //re-focus/select spinbox
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                double length = toolWidget->getParameter(WParameter::Third);
                if (onSketchPos.x - dHandler->firstCorner.x < 0) {
                    length = -length;
                }
                onSketchPos.x = dHandler->firstCorner.x + length;
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double width = toolWidget->getParameter(WParameter::Fourth);
                if (onSketchPos.y - dHandler->firstCorner.y < 0) {
                    width = -width;
                }
                onSketchPos.y = dHandler->firstCorner.y + width;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                double length = toolWidget->getParameter(WParameter::Third);
                onSketchPos.x = dHandler->center.x + length/2;
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double width = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.y = dHandler->center.y + width / 2;
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {
            double radius = toolWidget->getParameter(WParameter::Fifth);
            if(dHandler->firstCorner.x - dHandler->thirdCorner.x > 0.)
                onSketchPos.x = dHandler->firstCorner.x - radius;
            else
                onSketchPos.x = dHandler->firstCorner.x + radius;
            onSketchPos.y = dHandler->firstCorner.y;
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, fabs(onSketchPos.x - dHandler->firstCorner.x));

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, fabs(onSketchPos.y - dHandler->firstCorner.y));
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, fabs(onSketchPos.x - dHandler->center.x)*2);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, fabs(onSketchPos.y - dHandler->center.y)*2);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->roundCorners) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->radius);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->thickness);
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (!toolWidget->isParameterSet(WParameter::Sixth))
            toolWidget->updateVisualValue(WParameter::Sixth, dHandler->thickness);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth) &&
                dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::CenterAndCorner ) {
                if (dHandler->roundCorners || dHandler->makeFrame) {
                    handler->setState(SelectMode::SeekThird);
                }
                else {
                    handler->setState(SelectMode::End);
                    handler->finish();
                }

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->roundCorners && toolWidget->isParameterSet(WParameter::Fifth)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints


            if (dHandler->makeFrame) {
                handler->setState(SelectMode::SeekFourth);
            }
            else {
                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
        else if (dHandler->makeFrame && toolWidget->isParameterSet(WParameter::Sixth)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerRectangleBase::ToolWidgetManager::addConstraints() {
    int firstCurve = dHandler->firstCurve;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);
    auto length = toolWidget->getParameter(WParameter::Third);
    auto width = toolWidget->getParameter(WParameter::Fourth);
    auto radius = toolWidget->getParameter(WParameter::Fifth);
    auto thickness = toolWidget->getParameter(WParameter::Sixth);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto lengthSet = toolWidget->isParameterSet(WParameter::Third);
    auto widthSet = toolWidget->isParameterSet(WParameter::Fourth);
    auto radiusSet = toolWidget->isParameterSet(WParameter::Fifth);
    auto thicknessSet = toolWidget->isParameterSet(WParameter::Sixth);

    using namespace Sketcher;

    int firstCornerId = firstCurve;
    if (dHandler->constructionMethod() == DrawSketchHandlerRectangle::ConstructionMethod::Diagonal) {
         if (dHandler->radius > Precision::Confusion())
            firstCornerId = dHandler->constructionPointTwoId;
    }
    else {
        firstCornerId = dHandler->centerPointId;
    }

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCornerId, PointPos::start), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(firstCornerId, PointPos::start), GeoElementId::VAxis,
                x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(firstCornerId, PointPos::start), GeoElementId::HAxis,
                y0, handler->sketchgui->getObject());
    }

    if (lengthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve + 1, 1, firstCurve + 3, 2, length);

    if (widthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve, 1, firstCurve + 2, 2, width);

    if (radiusSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            firstCurve + 5, radius);

    if (thicknessSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
            firstCurve, 1, firstCurve + (dHandler->roundCorners == true ? 8 : 4), 1, fabs(thickness));
}

DEF_STD_CMD_AU(CmdSketcherCreateRectangle)

CmdSketcherCreateRectangle::CmdSketcherCreateRectangle()
  : Command("Sketcher_CreateRectangle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create rectangle");
    sToolTipText    = QT_TR_NOOP("Create a rectangle in the sketch");
    sWhatsThis      = "Sketcher_CreateRectangle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateRectangle";
    sAccel          = "G, R";
    eType           = ForEdit;
}

void CmdSketcherCreateRectangle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::Diagonal) );
}

void CmdSketcherCreateRectangle::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
        break;
    }
}

bool CmdSketcherCreateRectangle::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Polygon ================================================================================*/
class DrawSketchHandlerPolygon;

using DrawSketchHandlerPolygonBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerPolygon,
    StateMachines::TwoSeekEnd,
    /*PEditCurveSize =*/ 7,
    /*PAutoConstraintSize =*/ 2,
    /*WidgetParametersT =*/ WidgetParameters<5>,
    /*WidgetCheckboxesT =*/ WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/ WidgetComboboxes<0>>;

class DrawSketchHandlerPolygon : public DrawSketchHandlerPolygonBase
{
    friend DrawSketchHandlerPolygonBase;
public:

    DrawSketchHandlerPolygon() :
        Corners(6),
        AngleOfSeparation(2.0 * M_PI / static_cast<double>(Corners)),
        cos_v(cos(AngleOfSeparation)),
        sin_v(sin(AngleOfSeparation)){}
    virtual ~DrawSketchHandlerPolygon() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            centerPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            Base::Console().Error("EditCurve size: %f\n", EditCurve[0].x, EditCurve[0].y);
            EditCurve[0] = Base::Vector2d(onSketchPos.x, onSketchPos.y);
            EditCurve[Corners] = Base::Vector2d(onSketchPos.x, onSketchPos.y);

            Base::Vector2d dV = onSketchPos - centerPoint;
            double rx = dV.x;
            double ry = dV.y;
            for (int i = 1; i < static_cast<int>(Corners); i++) {
                const double old_rx = rx;
                rx = cos_v * rx - sin_v * ry;
                ry = cos_v * ry + sin_v * old_rx;
                EditCurve[i] = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
            }

            // Display radius for user
            const float radius = dV.Length();
            const float angle = (180.0 / M_PI) * atan2(dV.y, dV.x);

            SbString text;
            text.sprintf(" (%.1fR %.1fdeg)", radius, angle);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        unsetCursor();
        resetPositionText();
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add polygon"));

        try {
            Gui::Command::doCommand(Gui::Command::Doc,
                "import ProfileLib.RegularPolygon\n"
                "ProfileLib.RegularPolygon.makeRegularPolygon(%s,%i,App.Vector(%f,%f,0),App.Vector(%f,%f,0),%s)",
                Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(),
                Corners,
                centerPoint.x, centerPoint.y, EditCurve[0].x, EditCurve[0].y,
                geometryCreationMode == Construction ? "True" : "False");

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add hexagon: %s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
    }

    virtual void createAutoConstraints() override {
        // add auto constraints at the center of the polygon
        if (sugConstraints[0].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::mid);
            sugConstraints[0].clear();
        }

        // add auto constraints to the last side of the polygon
        if (sugConstraints[1].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 1, Sketcher::PointPos::end);
            sugConstraints[1].clear();
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Polygon";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Regular_Polygon");
    }

private:
    unsigned int Corners;
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double AngleOfSeparation, cos_v, sin_v;
};

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_polygon", "x of center"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_polygon", "y of center"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_polygon", "Radius"));
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "Angle"));
    toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p4", "Side number"));
    toolWidget->configureParameterInitialValue(WParameter::Fifth, dHandler->EditCurve.size() - 1);
}

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        case WParameter::Fifth: {
            dHandler->Corners = max(3, static_cast<int>(value));
            dHandler->AngleOfSeparation = 2.0 * M_PI / static_cast<double>(dHandler->Corners);
            dHandler->cos_v = cos(dHandler->AngleOfSeparation);
            dHandler->sin_v = sin(dHandler->AngleOfSeparation);
            dHandler->EditCurve.clear();
            dHandler->EditCurve.resize(dHandler->Corners + 1);
        }
            break;
    }
}

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        double length = (onSketchPos - dHandler->centerPoint).Length();
        if (toolWidget->isParameterSet(WParameter::Third)) {
            double radius = toolWidget->getParameter(WParameter::Third);
            if (length != 0.) {
                onSketchPos.x = dHandler->centerPoint.x + (onSketchPos.x - dHandler->centerPoint.x) * radius / length;
                onSketchPos.y = dHandler->centerPoint.y + (onSketchPos.y - dHandler->centerPoint.y) * radius / length;
            }
        }
        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            double angle = toolWidget->getParameter(WParameter::Fourth);
            onSketchPos.x = dHandler->centerPoint.x + cos(angle * M_PI / 180) * length;
            onSketchPos.y = dHandler->centerPoint.y + sin(angle * M_PI / 180) * length;
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {

        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, (onSketchPos - dHandler->centerPoint).Length());

        if (!toolWidget->isParameterSet(WParameter::Fourth))
            toolWidget->updateVisualValue(WParameter::Fourth, (onSketchPos - dHandler->centerPoint).Angle() * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth) &&
                toolWidget->isParameterSet(WParameter::Fifth)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerPolygonBase::ToolWidgetManager::addConstraints() {
    int lastCurve = handler->getHighestCurveIndex();

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);
    auto radius = toolWidget->getParameter(WParameter::Third);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto radiusSet = toolWidget->isParameterSet(WParameter::Third);

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(lastCurve, PointPos::mid), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(lastCurve, PointPos::mid), GeoElementId::VAxis,
                x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(lastCurve, PointPos::mid), GeoElementId::HAxis,
                y0, handler->sketchgui->getObject());
    }

    if (radiusSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            lastCurve, radius);
}

DEF_STD_CMD_A(CmdSketcherCreatePolygon)

CmdSketcherCreatePolygon::CmdSketcherCreatePolygon()
    : Command("Sketcher_CreatePolygon")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create polygon");
    sToolTipText = QT_TR_NOOP("Create a polygon in the sketch");
    sWhatsThis = "Sketcher_CreatePolygon";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreatePolygon";
    sAccel = "G, P";
    eType = ForEdit;
}

void CmdSketcherCreatePolygon::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon());
}

bool CmdSketcherCreatePolygon::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


/* Rectangles Comp command ==============================================================*/

DEF_STD_CMD_ACLU(CmdSketcherCompCreateRectangles)

CmdSketcherCompCreateRectangles::CmdSketcherCompCreateRectangles()
    : Command("Sketcher_CompCreateRectangles")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create rectangles");
    sToolTipText = QT_TR_NOOP("Creates a rectangle in the sketch");
    sWhatsThis = "Sketcher_CompCreateRectangles";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateRectangles::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerRectangle(ConstructionMethods::RectangleConstructionMethod::Diagonal));
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPolygon());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateRectangles::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateRectangles::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateRectangle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolygon_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateRectangles::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* rectangle1 = a[0];
    rectangle1->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Rectangle"));
    rectangle1->setToolTip(QApplication::translate("Sketcher_CreateRectangle", "Create a rectangle"));
    rectangle1->setStatusTip(rectangle1->toolTip());
    QAction* rectangle2 = a[1];
    rectangle2->setText(QApplication::translate("CmdSketcherCompCreateRectangles", "Polygon"));
    rectangle2->setToolTip(QApplication::translate("Sketcher_CreatePolygon", "Create a regular polygon"));
    rectangle2->setStatusTip(rectangle2->toolTip());
}

bool CmdSketcherCompCreateRectangles::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Polyline ================================================================================*/

class DrawSketchHandlerLineSet: public DrawSketchHandler
{
public:
    DrawSketchHandlerLineSet()
      : Mode(STATUS_SEEK_First), SegmentMode(SEGMENT_MODE_Line)
      , TransitionMode(TRANSITION_MODE_Free)
      , SnapMode(SNAP_MODE_Free)
      , suppressTransition(false)
      , EditCurve(2)
      , firstCurve(-1)
      , previousCurve(-1)
      , firstPosId(Sketcher::PointPos::none)
      , previousPosId(Sketcher::PointPos::none)
      , startAngle(0)
      , endAngle(0)
      , arcRadius(0)
      , firstsegment(true) {}

    virtual ~DrawSketchHandlerLineSet() = default;

    /// mode table
    enum SELECT_MODE {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_SEEK_Second,     /**< enum value ----. */
        STATUS_Do,
        STATUS_Close
    };

    enum SEGMENT_MODE
    {
        SEGMENT_MODE_Arc,
        SEGMENT_MODE_Line
    };

    enum TRANSITION_MODE
    {
        TRANSITION_MODE_Free,
        TRANSITION_MODE_Tangent,
        TRANSITION_MODE_Perpendicular_L,
        TRANSITION_MODE_Perpendicular_R
    };

    enum SNAP_MODE
    {
        SNAP_MODE_Free,
        SNAP_MODE_45Degree
    };

    virtual void registerPressedKey(bool pressed, int key) override
    {
        if (Mode != STATUS_SEEK_Second)
            return; // SegmentMode can be changed only in STATUS_SEEK_Second mode

        if (key == SoKeyboardEvent::M && pressed && previousCurve != -1) {
            // loop through the following modes:
            // SEGMENT_MODE_Line, TRANSITION_MODE_Free / TRANSITION_MODE_Tangent
            // SEGMENT_MODE_Line, TRANSITION_MODE_Perpendicular_L
            // SEGMENT_MODE_Line, TRANSITION_MODE_Tangent / TRANSITION_MODE_Free
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Tangent
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Perpendicular_L
            // SEGMENT_MODE_Arc, TRANSITION_MODE_Perpendicular_R

            SnapMode = SNAP_MODE_Free;

            Base::Vector2d onSketchPos;
            if (SegmentMode == SEGMENT_MODE_Line)
                onSketchPos = EditCurve[EditCurve.size()-1];
            else
                onSketchPos = EditCurve[29];

            const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(previousCurve);

            if (SegmentMode == SEGMENT_MODE_Line) {
                switch (TransitionMode) {
                    case TRANSITION_MODE_Free:
                        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) { // 3rd mode
                            SegmentMode = SEGMENT_MODE_Arc;
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        else // 1st mode
                            TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        break;
                    case TRANSITION_MODE_Perpendicular_L: // 2nd mode
                        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
                            TransitionMode = TRANSITION_MODE_Free;
                        else
                            TransitionMode = TRANSITION_MODE_Tangent;
                        break;
                    case TRANSITION_MODE_Tangent:
                        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) // 1st mode
                            TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        else { // 3rd mode
                            SegmentMode = SEGMENT_MODE_Arc;
                            TransitionMode = TRANSITION_MODE_Tangent;
                        }
                        break;
                    default: // unexpected mode
                        TransitionMode = TRANSITION_MODE_Free;
                        break;
                }
            }
            else {
                switch (TransitionMode) {
                    case TRANSITION_MODE_Tangent: // 4th mode
                        TransitionMode = TRANSITION_MODE_Perpendicular_L;
                        break;
                    case TRANSITION_MODE_Perpendicular_L: // 5th mode
                        TransitionMode = TRANSITION_MODE_Perpendicular_R;
                        break;
                    default: // 6th mode (Perpendicular_R) + unexpected mode
                        SegmentMode = SEGMENT_MODE_Line;
                        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
                            TransitionMode = TRANSITION_MODE_Tangent;
                        else
                            TransitionMode = TRANSITION_MODE_Free;
                        break;
                }
            }

            if (SegmentMode == SEGMENT_MODE_Line)
                EditCurve.resize(TransitionMode == TRANSITION_MODE_Free ? 2 : 3);
            else
                EditCurve.resize(32);
            mouseMove(onSketchPos); // trigger an update of EditCurve
        }
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        suppressTransition = false;
        if (Mode==STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Second){
            if (SegmentMode == SEGMENT_MODE_Line) {
                EditCurve[EditCurve.size()-1] = onSketchPos;
                if (TransitionMode == TRANSITION_MODE_Tangent) {
                    Base::Vector2d Tangent(dirVec.x,dirVec.y);
                    EditCurve[1].ProjectToLine(EditCurve[2] - EditCurve[0], Tangent);
                    if (EditCurve[1] * Tangent < 0) {
                        EditCurve[1] = EditCurve[2];
                        suppressTransition = true;
                    }
                    else
                        EditCurve[1] = EditCurve[0] + EditCurve[1];
                }
                else if (TransitionMode == TRANSITION_MODE_Perpendicular_L ||
                         TransitionMode == TRANSITION_MODE_Perpendicular_R) {
                    Base::Vector2d Perpendicular(-dirVec.y,dirVec.x);
                    EditCurve[1].ProjectToLine(EditCurve[2] - EditCurve[0], Perpendicular);
                    EditCurve[1] = EditCurve[0] + EditCurve[1];
                }

                drawEdit(EditCurve);

                float length = (EditCurve[1] - EditCurve[0]).Length();
                float angle = (EditCurve[1] - EditCurve[0]).GetAngle(Base::Vector2d(1.f,0.f));

                SbString text;
                text.sprintf(" (%.1f,%.1fdeg)", length, angle * 180 / M_PI);
                setPositionText(EditCurve[1], text);

                if (TransitionMode == TRANSITION_MODE_Free) {
                    if (seekAutoConstraint(sugConstr2, onSketchPos, onSketchPos - EditCurve[0])) {
                        renderSuggestConstraintsCursor(sugConstr2);
                        return;
                    }
                }
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) {

                if(QApplication::keyboardModifiers() == Qt::ControlModifier)
                    SnapMode = SNAP_MODE_45Degree;
                else
                    SnapMode = SNAP_MODE_Free;

                Base::Vector2d Tangent;
                if  (TransitionMode == TRANSITION_MODE_Tangent)
                    Tangent = Base::Vector2d(dirVec.x,dirVec.y);
                else if  (TransitionMode == TRANSITION_MODE_Perpendicular_L)
                    Tangent = Base::Vector2d(-dirVec.y,dirVec.x);
                else if  (TransitionMode == TRANSITION_MODE_Perpendicular_R)
                    Tangent = Base::Vector2d(dirVec.y,-dirVec.x);

                double theta = Tangent.GetAngle(onSketchPos - EditCurve[0]);

                arcRadius = (onSketchPos - EditCurve[0]).Length()/(2.0*sin(theta));

                // At this point we need a unit normal vector pointing towards
                // the center of the arc we are drawing. Derivation of the formula
                // used here can be found at http://people.richland.edu/james/lecture/m116/matrices/area.html
                double x1 = EditCurve[0].x;
                double y1 = EditCurve[0].y;
                double x2 = x1 + Tangent.x;
                double y2 = y1 + Tangent.y;
                double x3 = onSketchPos.x;
                double y3 = onSketchPos.y;
                if ((x2*y3-x3*y2)-(x1*y3-x3*y1)+(x1*y2-x2*y1) > 0)
                    arcRadius *= -1;
                if (boost::math::isnan(arcRadius) || boost::math::isinf(arcRadius))
                    arcRadius = 0.f;

                CenterPoint = EditCurve[0] + Base::Vector2d(arcRadius * Tangent.y, -arcRadius * Tangent.x);

                double rx = EditCurve[0].x - CenterPoint.x;
                double ry = EditCurve[0].y - CenterPoint.y;

                startAngle = atan2(ry,rx);

                double rxe = onSketchPos.x - CenterPoint.x;
                double rye = onSketchPos.y - CenterPoint.y;
                double arcAngle = atan2(-rxe*ry + rye*rx, rxe*rx + rye*ry);
                if (boost::math::isnan(arcAngle) || boost::math::isinf(arcAngle))
                    arcAngle = 0.f;
                if (arcRadius >= 0 && arcAngle > 0)
                    arcAngle -=  2*M_PI;
                if (arcRadius < 0 && arcAngle < 0)
                    arcAngle +=  2*M_PI;

                if (SnapMode == SNAP_MODE_45Degree)
                    arcAngle = round(arcAngle / (M_PI/4)) * M_PI/4;

                endAngle = startAngle + arcAngle;

                for (int i=1; i <= 29; i++) {
                    double angle = i*arcAngle/29.0;
                    double dx = rx * cos(angle) - ry * sin(angle);
                    double dy = rx * sin(angle) + ry * cos(angle);
                    EditCurve[i] = Base::Vector2d(CenterPoint.x + dx, CenterPoint.y + dy);
                }

                EditCurve[30] = CenterPoint;
                EditCurve[31] = EditCurve[0];

                drawEdit(EditCurve);

                SbString text;
                text.sprintf(" (%.1fR,%.1fdeg)", std::abs(arcRadius), arcAngle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f,0.f))) {
                    renderSuggestConstraintsCursor(sugConstr2);
                    return;
                }
            }
        }
        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_SEEK_First) {

            EditCurve[0] = onSketchPos; // this may be overwritten if previousCurve is found

            virtualsugConstr1 = sugConstr1; // store original autoconstraints.

            // here we check if there is a preselected point and
            // we set up a transition from the neighbouring segment.
            // (peviousCurve, previousPosId, dirVec, TransitionMode)
            for (unsigned int i=0; i < sugConstr1.size(); i++)
                if (sugConstr1[i].Type == Sketcher::Coincident) {
                    const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(sugConstr1[i].GeoId);
                    if ((geom->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                         geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) &&
                        (sugConstr1[i].PosId == Sketcher::PointPos::start ||
                         sugConstr1[i].PosId == Sketcher::PointPos::end)) {
                        previousCurve = sugConstr1[i].GeoId;
                        previousPosId = sugConstr1[i].PosId;
                        updateTransitionData(previousCurve,previousPosId); // -> dirVec, EditCurve[0]
                        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                            TransitionMode = TRANSITION_MODE_Tangent;
                            SnapMode = SNAP_MODE_Free;
                        }
                        sugConstr1.erase(sugConstr1.begin()+i); // actually we should clear the vector completely
                        break;
                    }
                }

            // remember our first point (even if we are doing a transition from a previous curve)
            firstCurve = getHighestCurveIndex() + 1;
            firstPosId = Sketcher::PointPos::start;

            if (SegmentMode == SEGMENT_MODE_Line)
                EditCurve.resize(TransitionMode == TRANSITION_MODE_Free ? 2 : 3);
            else if (SegmentMode == SEGMENT_MODE_Arc)
                EditCurve.resize(32);
            Mode = STATUS_SEEK_Second;
        }
        else if (Mode == STATUS_SEEK_Second) {
            // exit on clicking exactly at the same position (e.g. double click)
            if (onSketchPos == EditCurve[0]) {
                unsetCursor();
                resetPositionText();
                EditCurve.clear();
                drawEdit(EditCurve);

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

                if(continuousMode){
                    // This code enables the continuous creation mode.
                    Mode=STATUS_SEEK_First;
                    SegmentMode=SEGMENT_MODE_Line;
                    TransitionMode=TRANSITION_MODE_Free;
                    SnapMode = SNAP_MODE_Free;
                    suppressTransition=false;
                    firstCurve=-1;
                    previousCurve=-1;
                    firstPosId=Sketcher::PointPos::none;
                    previousPosId=Sketcher::PointPos::none;
                    EditCurve.clear();
                    drawEdit(EditCurve);
                    EditCurve.resize(2);
                    applyCursor();
                    /* this is ok not to call to purgeHandler
                    * in continuous creation mode because the
                    * handler is destroyed by the quit() method on pressing the
                    * right button of the mouse */
                    return true;
                }
                else{
                    sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
                    return true;
                }
            }

            Mode = STATUS_Do;

            if (getPreselectPoint() != -1 && firstPosId != Sketcher::PointPos::none) {
                int GeoId;
                Sketcher::PointPos PosId;
                sketchgui->getSketchObject()->getGeoVertexIndex(getPreselectPoint(),GeoId,PosId);
                if (sketchgui->getSketchObject()->arePointsCoincident(GeoId,PosId,firstCurve,firstPosId))
                    Mode = STATUS_Close;
            }
            else if (getPreselectCross() == 0 && firstPosId != Sketcher::PointPos::none) {
                // close line started at root point
                if (sketchgui->getSketchObject()->arePointsCoincident(-1,Sketcher::PointPos::start,firstCurve,firstPosId))
                    Mode = STATUS_Close;
            }
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        if (Mode == STATUS_Do || Mode == STATUS_Close) {
            bool addedGeometry = true;
            if (SegmentMode == SEGMENT_MODE_Line) {
                // issue the geometry
                try {
                    // open the transaction
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add line to sketch wire"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.LineSegment(App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%s)",
                        EditCurve[0].x,EditCurve[0].y,EditCurve[1].x,EditCurve[1].y,
                        geometryCreationMode==Construction?"True":"False");
                }
                catch (const Base::Exception& e) {
                    addedGeometry = false;
                    Base::Console().Error("Failed to add line: %s\n", e.what());
                    Gui::Command::abortCommand();
                }

                firstsegment=false;
            }
            else if (SegmentMode == SEGMENT_MODE_Arc) { // We're dealing with an Arc
                if (!boost::math::isnormal(arcRadius)) {
                    Mode = STATUS_SEEK_Second;
                    return true;
                }

                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add arc to sketch wire"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfCircle"
                        "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                        CenterPoint.x, CenterPoint.y, std::abs(arcRadius),
                        std::min(startAngle,endAngle), std::max(startAngle,endAngle),
                        geometryCreationMode==Construction?"True":"False");
                }
                catch (const Base::Exception& e) {
                    addedGeometry = false;
                    Base::Console().Error("Failed to add arc: %s\n", e.what());
                    Gui::Command::abortCommand();
                }

                firstsegment=false;
            }

            int lastCurve = getHighestCurveIndex();
            // issue the constraint
            if (addedGeometry && (previousPosId != Sketcher::PointPos::none)) {
                Sketcher::PointPos lastStartPosId = (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle) ?
                                                    Sketcher::PointPos::end : Sketcher::PointPos::start;
                Sketcher::PointPos lastEndPosId = (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle) ?
                                                  Sketcher::PointPos::start : Sketcher::PointPos::end;
                // in case of a tangency constraint, the coincident constraint is redundant
                std::string constrType = "Coincident";
                if (!suppressTransition && previousCurve != -1) {
                    if (TransitionMode == TRANSITION_MODE_Tangent)
                        constrType = "Tangent";
                    else if (TransitionMode == TRANSITION_MODE_Perpendicular_L ||
                             TransitionMode == TRANSITION_MODE_Perpendicular_R)
                        constrType = "Perpendicular";
                }
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('%s',%i,%i,%i,%i)) ",
                     constrType.c_str(), previousCurve, static_cast<int>(previousPosId), lastCurve, static_cast<int>(lastStartPosId));

                if(SnapMode == SNAP_MODE_45Degree && Mode != STATUS_Close) {
                    // -360, -315, -270, -225, -180, -135, -90, -45,  0, 45,  90, 135, 180, 225, 270, 315, 360
                    //  N/A,    a, perp,    a,  par,    a,perp,   a,N/A,  a,perp,   a, par,   a,perp,   a, N/A

                    // #3974: if in radians, the printf %f defaults to six decimals, which leads to loss of precision
                    double arcAngle = abs(round( (endAngle - startAngle) / (M_PI/4)) * 45); // in degrees

                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%i,App.Units.Quantity('%f deg'))) ",
                                          lastCurve, arcAngle);
                }
                if (Mode == STATUS_Close) {
                    // close the loop by constrain to the first curve point
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Coincident',%i,%i,%i,%i)) ",
                                          lastCurve,static_cast<int>(lastEndPosId),firstCurve,static_cast<int>(firstPosId));
                }
                Gui::Command::commitCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool avoidredundant = sketchgui->AvoidRedundant.getValue()  && sketchgui->Autoconstraints.getValue();

            if (Mode == STATUS_Close) {

                if(avoidredundant) {
                    if (SegmentMode == SEGMENT_MODE_Line) { // avoid redundant constraints.
                        if (sugConstr1.size() > 0)
                            removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),sugConstr1,sugConstr2);
                        else
                            removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),virtualsugConstr1,sugConstr2);
                    }
                }

                if (sugConstr2.size() > 0) {
                    // exclude any coincidence constraints
                    std::vector<AutoConstraint> sugConstr;
                    for (unsigned int i=0; i < sugConstr2.size(); i++) {
                        if (sugConstr2[i].Type != Sketcher::Coincident)
                            sugConstr.push_back(sugConstr2[i]);
                    }
                    createAutoConstraints(sugConstr, getHighestCurveIndex(), Sketcher::PointPos::end);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                unsetCursor();

                resetPositionText();
                EditCurve.clear();
                drawEdit(EditCurve);

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

                if(continuousMode){
                    // This code enables the continuous creation mode.
                    Mode=STATUS_SEEK_First;
                    SegmentMode=SEGMENT_MODE_Line;
                    TransitionMode=TRANSITION_MODE_Free;
                    SnapMode = SNAP_MODE_Free;
                    suppressTransition=false;
                    firstCurve=-1;
                    previousCurve=-1;
                    firstPosId=Sketcher::PointPos::none;
                    previousPosId=Sketcher::PointPos::none;
                    EditCurve.clear();
                    drawEdit(EditCurve);
                    EditCurve.resize(2);
                    applyCursor();
                    /* this is ok not to call to purgeHandler
                    * in continuous creation mode because the
                    * handler is destroyed by the quit() method on pressing the
                    * right button of the mouse */
                }
                else{
                    sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
                }
            }
            else {
                Gui::Command::commitCommand();

                // Add auto constraints
                if (sugConstr1.size() > 0) { // this is relevant only to the very first point
                    createAutoConstraints(sugConstr1, getHighestCurveIndex(), Sketcher::PointPos::start);
                    sugConstr1.clear();
                }


                if(avoidredundant) {
                    if (SegmentMode == SEGMENT_MODE_Line) { // avoid redundant constraints.
                        if (sugConstr1.size() > 0)
                            removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),sugConstr1,sugConstr2);
                        else
                            removeRedundantHorizontalVertical(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()),virtualsugConstr1,sugConstr2);
                    }
                }

                virtualsugConstr1 = sugConstr2; // these are the initial constraints for the next iteration.

                if (sugConstr2.size() > 0) {
                    createAutoConstraints(sugConstr2, getHighestCurveIndex(),
                                          (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle) ?
                                            Sketcher::PointPos::start : Sketcher::PointPos::end);
                    sugConstr2.clear();
                }

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                // remember the vertex for the next rounds constraint..
                previousCurve = getHighestCurveIndex();
                previousPosId = (SegmentMode == SEGMENT_MODE_Arc && startAngle > endAngle) ?
                                 Sketcher::PointPos::start : Sketcher::PointPos::end; // cw arcs are rendered in reverse

                // setup for the next line segment
                // calculate dirVec and EditCurve[0]
                updateTransitionData(previousCurve,previousPosId);

                applyCursor();
                Mode = STATUS_SEEK_Second;
                if (SegmentMode == SEGMENT_MODE_Arc) {
                    TransitionMode = TRANSITION_MODE_Tangent;
                    EditCurve.resize(3);
                    EditCurve[2] = EditCurve[0];
                }
                else {
                    TransitionMode = TRANSITION_MODE_Free;
                    EditCurve.resize(2);
                }
                SegmentMode = SEGMENT_MODE_Line;
                SnapMode = SNAP_MODE_Free;
                EditCurve[1] = EditCurve[0];
                mouseMove(onSketchPos); // trigger an update of EditCurve
            }
        }
        return true;
    }

    virtual void quit(void) override {
        // We must see if we need to create a B-spline before cancelling everything
        // and now just like any other Handler,

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

        bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

        if (firstsegment) {
            // user when right-clicking with no segment in really wants to exit
            DrawSketchHandler::quit();
        }
        else {

            if(!continuousMode){
                DrawSketchHandler::quit();
            }
            else {
                // This code disregards existing data and enables the continuous creation mode.
                Mode=STATUS_SEEK_First;
                SegmentMode=SEGMENT_MODE_Line;
                TransitionMode=TRANSITION_MODE_Free;
                SnapMode = SNAP_MODE_Free;
                suppressTransition=false;
                firstCurve=-1;
                previousCurve=-1;
                firstPosId=Sketcher::PointPos::none;
                previousPosId=Sketcher::PointPos::none;
                firstsegment=true;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(2);
                applyCursor();
            }
        }
    }

private:
    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_Lineset");
    }

protected:
    SELECT_MODE Mode;
    SEGMENT_MODE SegmentMode;
    TRANSITION_MODE TransitionMode;
    SNAP_MODE SnapMode;
    bool suppressTransition;

    std::vector<Base::Vector2d> EditCurve;
    int firstCurve;
    int previousCurve;
    Sketcher::PointPos firstPosId;
    Sketcher::PointPos previousPosId;
    // the latter stores those constraints that a first point would have been given in absence of the transition mechanism
    std::vector<AutoConstraint> sugConstr1, sugConstr2, virtualsugConstr1;

    Base::Vector2d CenterPoint;
    Base::Vector3d dirVec;
    double startAngle, endAngle, arcRadius;

    bool firstsegment;

    void updateTransitionData(int GeoId, Sketcher::PointPos PosId) {

        // Use updated startPoint/endPoint as autoconstraints can modify the position
        const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(GeoId);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geom);
            dirVec.Set(lineSeg->getEndPoint().x - lineSeg->getStartPoint().x,
                       lineSeg->getEndPoint().y - lineSeg->getStartPoint().y,
                       0.f);
            if (PosId == Sketcher::PointPos::start) {
                dirVec *= -1;
                EditCurve[0] = Base::Vector2d(lineSeg->getStartPoint().x, lineSeg->getStartPoint().y);
            }
            else
                EditCurve[0] = Base::Vector2d(lineSeg->getEndPoint().x, lineSeg->getEndPoint().y);
        }
        else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arcSeg = static_cast<const Part::GeomArcOfCircle *>(geom);
            if (PosId == Sketcher::PointPos::start) {
                EditCurve[0] = Base::Vector2d(arcSeg->getStartPoint(/*emulateCCW=*/true).x,arcSeg->getStartPoint(/*emulateCCW=*/true).y);
                dirVec = Base::Vector3d(0.f,0.f,-1.0) % (arcSeg->getStartPoint(/*emulateCCW=*/true)-arcSeg->getCenter());
            }
            else {
                EditCurve[0] = Base::Vector2d(arcSeg->getEndPoint(/*emulateCCW=*/true).x,arcSeg->getEndPoint(/*emulateCCW=*/true).y);
                dirVec = Base::Vector3d(0.f,0.f,1.0) % (arcSeg->getEndPoint(/*emulateCCW=*/true)-arcSeg->getCenter());
            }
        }
        dirVec.Normalize();
    }
};

DEF_STD_CMD_AU(CmdSketcherCreatePolyline)

CmdSketcherCreatePolyline::CmdSketcherCreatePolyline()
  : Command("Sketcher_CreatePolyline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create polyline");
    sToolTipText    = QT_TR_NOOP("Create a polyline in the sketch. 'M' Key cycles behaviour");
    sWhatsThis      = "Sketcher_CreatePolyline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePolyline";
    sAccel          = "G, M";
    eType           = ForEdit;
}

void CmdSketcherCreatePolyline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerLineSet() );
}

void CmdSketcherCreatePolyline::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePolyline_Constr"));
        break;
    }
}

bool CmdSketcherCreatePolyline::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


/* Circle ================================================================================*/
class DrawSketchHandlerCircle;

namespace SketcherGui::ConstructionMethods {

enum class CircleEllipseConstructionMethod {
    Center,
    ThreeRim,
    End // Must be the last one
};

}

using DrawSketchHandlerCircleBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerCircle,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<3, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerCircle : public DrawSketchHandlerCircleBase
{
    friend DrawSketchHandlerCircleBase;

public:
    DrawSketchHandlerCircle(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerCircleBase(constrMethod) {}
    virtual ~DrawSketchHandlerCircle() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            if (constructionMethod() == ConstructionMethod::Center) {
                centerPoint = onSketchPos;
            }
            else {
                firstPoint = onSketchPos;
            }

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (constructionMethod() == ConstructionMethod::ThreeRim) {
                centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                secondPoint = onSketchPos;
            }

            radius = (onSketchPos - centerPoint).Length();

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* circle = new Part::GeomCircle();
            circle->setRadius(radius);
            circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(circle);
            drawEdit(geometriesToAdd);

            SbString text;
            setPositionText(onSketchPos, text);
            if (constructionMethod() == ConstructionMethod::Center) {
                text.sprintf(" (%.1fR)", radius);

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, onSketchPos - centerPoint, AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
            else {
                double lineAngle = GetPointAngle(centerPoint, onSketchPos);
                // This lineAngle will report counter-clockwise from +X, not relatively
                text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)lineAngle * 180 / M_PI);

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            try
            {
                centerPoint = Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);
                radius = (onSketchPos - centerPoint).Length();

                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomCircle* circle = new Part::GeomCircle();
                circle->setRadius(radius);
                circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(circle);
                drawEdit(geometriesToAdd);

                double lineAngle = GetPointAngle(centerPoint, onSketchPos);
                // This lineAngle will report counter-clockwise from +X, not relatively
                SbString text;
                text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)lineAngle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[2]);
                    return;
                }
            }
            catch (Base::ValueError& e) {
                e.ReportException();
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        unsetCursor();
        resetPositionText();

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch circle"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Circle"
                "(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%s)",
                centerPoint.x, centerPoint.y,
                radius,
                geometryCreationMode == Construction ? "True" : "False");

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add circle: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {
        if (constructionMethod() == ConstructionMethod::Center) {
            // add auto constraints for the center point
            if (!sugConstraints[0].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstraints[0].clear();
            }

            // add suggested constraints for circumference
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[1].clear();
            }
        }
        else {
            // Auto Constraint first picked point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[0].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[1].clear();
            }

            // Auto Constraint third picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Circle";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Circle");
        else // constructionMethod == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointCircle");
    }

    //reimplement because circle is 2 steps while 3rims is 3 steps
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (state() == SelectMode::SeekSecond && constructionMethod() == ConstructionMethod::Center) {
            setState(SelectMode::End);
        }
        else {
            this->moveToNextMode();
        }
    }

private:
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double radius;
};

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::configureToolWidget() {

    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_circle", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_circle", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_circle", "Radius"));
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
    }
}

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->firstPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->firstPoint.y = value;
            break;
        case WParameter::Third:
            dHandler->secondPoint.x = value;
            break;
        case WParameter::Fourth:
            dHandler->secondPoint.y = value;
            break;
        }
    }
}

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Third)) {
                double radius = toolWidget->getParameter(WParameter::Third);
                onSketchPos.x = dHandler->centerPoint.x + radius;
                onSketchPos.y = dHandler->centerPoint.y;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    { //3 rims only
        if (toolWidget->isParameterSet(WParameter::Fifth))
            onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

        if (toolWidget->isParameterSet(WParameter::Sixth))
            onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third,dHandler->radius);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    { //3 rims only
        if (!toolWidget->isParameterSet(WParameter::Fifth))
            toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Sixth))
            toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
            else if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth) &&
                dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim) {

                handler->setState(SelectMode::SeekThird);

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth) ||
            toolWidget->isParameterSet(WParameter::Sixth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Fifth) &&
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerCircleBase::ToolWidgetManager::addConstraints() {
    if (dHandler->constructionMethod() == DrawSketchHandlerCircle::ConstructionMethod::Center) {
        int firstCurve = handler->getHighestCurveIndex();

        auto x0 = toolWidget->getParameter(WParameter::First);
        auto y0 = toolWidget->getParameter(WParameter::Second);

        auto x0set = toolWidget->isParameterSet(WParameter::First);
        auto y0set = toolWidget->isParameterSet(WParameter::Second);
        auto radiusSet = toolWidget->isParameterSet(WParameter::Third);

        using namespace Sketcher;

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }

        if (radiusSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve, dHandler->radius);
    }
    //No constraint possible for 3 rim circle.
}

DEF_STD_CMD_A(CmdSketcherCreateCircle)

CmdSketcherCreateCircle::CmdSketcherCreateCircle()
  : Command("Sketcher_CreateCircle")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create circle");
    sToolTipText    = QT_TR_NOOP("Create a circle in the sketch");
    sWhatsThis      = "Sketcher_CreateCircle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateCircle";
    sAccel          = "G, C";
    eType           = ForEdit;
}

void CmdSketcherCreateCircle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerCircle(ConstructionMethods::CircleEllipseConstructionMethod::Center));
}

bool CmdSketcherCreateCircle::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Ellipse ==============================================================================*/
class DrawSketchHandlerEllipse;

using DrawSketchHandlerEllipseBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerEllipse,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerEllipse : public DrawSketchHandlerEllipseBase
{
    friend DrawSketchHandlerEllipseBase;

public:
    DrawSketchHandlerEllipse(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerEllipseBase(constrMethod) {}
    virtual ~DrawSketchHandlerEllipse() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            if (constructionMethod() == ConstructionMethod::Center) {
                centerPoint = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            else {
                periapsis = onSketchPos;
                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }

        }
        break;
        case SelectMode::SeekSecond:
        {
            if (constructionMethod() == ConstructionMethod::ThreeRim) {
                apoapsis = onSketchPos;
                centerPoint = (apoapsis - periapsis) / 2 + periapsis;
            }
            else {
                periapsis = onSketchPos;
            }

            firstAxis = periapsis - centerPoint;
            firstRadius = firstAxis.Length();

            //for this step we just draw a circle.
            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* ellipse = new Part::GeomCircle();
            ellipse->setRadius(firstRadius);
            ellipse->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(ellipse);
            drawEdit(geometriesToAdd);

            SbString text;
            double angle = GetPointAngle(centerPoint, onSketchPos);
            text.sprintf(" (%.1fR,%.1fdeg)", (float)firstRadius, (float)angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            try
            {
                //recalculate in case widget modified something
                if (constructionMethod() == ConstructionMethod::ThreeRim) {
                    centerPoint = (apoapsis - periapsis) / 2 + periapsis;
                }
                firstAxis = periapsis - centerPoint;
                firstRadius = firstAxis.Length();

                //Find bPoint For that first we need the distance of onSketchPos to major axis.
                Base::Vector2d projectedPtn;
                projectedPtn.ProjectToLine(onSketchPos - centerPoint, firstAxis);
                projectedPtn = centerPoint + projectedPtn;
                secondAxis = onSketchPos - projectedPtn;
                secondRadius = secondAxis.Length();

                Base::Vector2d majorAxis = firstAxis;
                double majorRadius = firstRadius;
                double minorRadius = secondRadius;
                if (secondRadius > firstRadius) {
                    majorAxis = secondAxis;
                    majorRadius = secondRadius;
                    minorRadius = firstRadius;
                }

                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomEllipse* ellipse = new Part::GeomEllipse();
                ellipse->setMajorRadius(majorRadius);
                ellipse->setMinorRadius(minorRadius);
                ellipse->setMajorAxisDir(Base::Vector3d(majorAxis.x, majorAxis.y, 0.));
                ellipse->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(ellipse);
                drawEdit(geometriesToAdd);

                SbString text;
                text.sprintf(" (%.1fR,%.1fR)", (float)majorRadius, (float)minorRadius);
                setPositionText(onSketchPos, text);

                if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                    renderSuggestConstraintsCursor(sugConstraints[2]);
                    return;
                }
            }
            catch (Base::ValueError& e) {
                e.ReportException();
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        unsetCursor();
        resetPositionText();
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
        if (fabs(firstRadius - secondRadius) < Precision::Confusion()) {
            //don't make an ellipse with equal radius it won't work. We could create a circle instead?
            return;
        }
        try {
            ellipseGeoId = getHighestCurveIndex() + 1;
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch ellipse"));

            Base::Vector2d majorAxis = firstAxis;
            double majorRadius = firstRadius;
            double minorRadius = secondRadius;
            if (secondRadius > firstRadius) {
                majorAxis = secondAxis;
                majorRadius = secondRadius;
                minorRadius = firstRadius;
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomEllipse* ellipse = new Part::GeomEllipse();
            ellipse->setMajorRadius(majorRadius);
            ellipse->setMinorRadius(minorRadius);
            ellipse->setMajorAxisDir(Base::Vector3d(majorAxis.x, majorAxis.y, 0.));
            ellipse->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(ellipse);
            Obj->addGeometry(std::move(geometriesToAdd));

            Gui::cmdAppObjectArgs(Obj, "exposeInternalGeometry(%d)", ellipseGeoId);

            Gui::Command::commitCommand();

        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add ellipse: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {
        if (constructionMethod() == ConstructionMethod::Center) {
            // add auto constraints for the center point
            if (!sugConstraints[0].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], ellipseGeoId, Sketcher::PointPos::mid);
                sugConstraints[0].clear();
            }

            // add suggested constraints for circumference
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], ellipseGeoId, Sketcher::PointPos::none);
                sugConstraints[1].clear();
            }
        }
        else {
            // Auto Constraint first picked point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], ellipseGeoId, Sketcher::PointPos::none);
                sugConstraints[0].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], ellipseGeoId, Sketcher::PointPos::none);
                sugConstraints[1].clear();
            }

            // Auto Constraint third picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], ellipseGeoId, Sketcher::PointPos::none);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Ellipse";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Ellipse");
        else // constructionMethod == DrawSketchHandlerCircle::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointEllipse");
    }

private:
    Base::Vector2d centerPoint, periapsis, apoapsis, firstAxis, secondAxis;
    double firstRadius, secondRadius;
    int ellipseGeoId;

    void swapPoints(Base::Vector2d& p1, Base::Vector2d& p2) {
        Base::Vector2d p3 = p1;
        p1 = p2;
        p2 = p3;
    }
};

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::configureToolWidget() {

    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_ellipse", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_ellipse", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_ellipse", "First radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Angle to HAxis"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p3_ellipse", "Second radius"));
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        case WParameter::Third:
            //change angle?
            break;
        case WParameter::Fourth:
            dHandler->firstRadius = value;
            break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->periapsis.x = value;
            break;
        case WParameter::Second:
            dHandler->periapsis.y = value;
            break;
        case WParameter::Third:
            dHandler->apoapsis.x = value;
            break;
        case WParameter::Fourth:
            dHandler->apoapsis.y = value;
            break;
        }
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            double length = (onSketchPos - dHandler->centerPoint).Length();
            if (toolWidget->isParameterSet(WParameter::Third)) {
                dHandler->firstRadius = toolWidget->getParameter(WParameter::Third);
                if (length != 0.) {
                    onSketchPos.x = dHandler->centerPoint.x + (onSketchPos.x - dHandler->centerPoint.x) * dHandler->firstRadius / length;
                    onSketchPos.y = dHandler->centerPoint.y + (onSketchPos.y - dHandler->centerPoint.y) * dHandler->firstRadius / length;
                }
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                double angle = toolWidget->getParameter(WParameter::Fourth);
                onSketchPos.x = dHandler->centerPoint.x + cos(angle * M_PI / 180) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin(angle * M_PI / 180) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                dHandler->secondRadius = toolWidget->getParameter(WParameter::Fifth);
                onSketchPos = dHandler->centerPoint + dHandler->secondAxis * dHandler->secondRadius / dHandler->secondAxis.Length();
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth))
                onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

            if (toolWidget->isParameterSet(WParameter::Sixth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, dHandler->firstRadius);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->firstAxis.Angle());
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->secondRadius);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {

                handler->updateDataAndDrawToPosition(prevCursorPosition);

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth) ||
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->updateDataAndDrawToPosition(prevCursorPosition);

                if (toolWidget->isParameterSet(WParameter::Fifth) &&
                    toolWidget->isParameterSet(WParameter::Sixth)) {

                    handler->setState(SelectMode::End);
                    handler->finish();
                }
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerEllipseBase::ToolWidgetManager::addConstraints() {
    if (dHandler->constructionMethod() == DrawSketchHandlerEllipse::ConstructionMethod::Center) {
        int firstCurve = dHandler->ellipseGeoId;

        auto x0 = toolWidget->getParameter(WParameter::First);
        auto y0 = toolWidget->getParameter(WParameter::Second);
        auto angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;

        auto x0set = toolWidget->isParameterSet(WParameter::First);
        auto y0set = toolWidget->isParameterSet(WParameter::Second);
        auto firstRadiusSet = toolWidget->isParameterSet(WParameter::Third);
        auto angleSet = toolWidget->isParameterSet(WParameter::Fourth);
        auto secondRadiusSet = toolWidget->isParameterSet(WParameter::Fifth);

        using namespace Sketcher;

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }

        int firstLine = firstCurve + 1;
        int secondLine = firstCurve + 2;
        if (dHandler->secondRadius > dHandler->firstRadius)
            std::swap(firstLine, secondLine);


        //this require to show internal geometry.
        if (firstRadiusSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, firstLine, 1, dHandler->firstRadius);
        }
        //Todo: this makes the ellipse 'jump' because it's doing a 180 degree turn before applying asked angle. Probably because start and end points of line are not in the correct direction.
        if (angleSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                firstLine, angle);
        }

        if (secondRadiusSet) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, secondLine, 1, dHandler->secondRadius);
        }
    }
    //No constraint possible for 3 rim ellipse.
}


DEF_STD_CMD_A(CmdSketcherCreateEllipseByCenter)

CmdSketcherCreateEllipseByCenter::CmdSketcherCreateEllipseByCenter()
  : Command("Sketcher_CreateEllipseByCenter")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create ellipse by center");
    sToolTipText    = QT_TR_NOOP("Create an ellipse by center in the sketch");
    sWhatsThis      = "Sketcher_CreateEllipseByCenter";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Conics_Ellipse_Center";
    sAccel          = "G, E, E";
    eType           = ForEdit;
}

void CmdSketcherCreateEllipseByCenter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerEllipse(ConstructionMethods::CircleEllipseConstructionMethod::Center));
}

bool CmdSketcherCreateEllipseByCenter::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// Comp for circle and ellipse ============================================================

DEF_STD_CMD_ACLU(CmdSketcherCompCreateCircle)

CmdSketcherCompCreateCircle::CmdSketcherCompCreateCircle()
    : Command("Sketcher_CompCreateCircle")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create circle");
    sToolTipText = QT_TR_NOOP("Create a circle in the sketcher");
    sWhatsThis = "Sketcher_CompCreateCircle";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateCircle::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerCircle(ConstructionMethods::CircleEllipseConstructionMethod::Center));
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerEllipse(ConstructionMethods::CircleEllipseConstructionMethod::Center));
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateCircle::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* circle = pcAction->addAction(QString());
    circle->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
    QAction* ellipse = pcAction->addAction(QString());
    ellipse->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(circle->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateCircle::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateCircle_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateEllipse_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateCircle::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* circle = a[0];
    circle->setText(QApplication::translate("CmdSketcherCompCreateCircle", "Circle"));
    circle->setToolTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center or by rim points"));
    circle->setStatusTip(QApplication::translate("Sketcher_CreateCircle", "Create a circle by its center or by rim points"));
    QAction* ellipse = a[1];
    ellipse->setText(QApplication::translate("CmdSketcherCompCreateCircle", "Ellipse"));
    ellipse->setToolTip(QApplication::translate("Sketcher_CreateEllipse", "Create an ellipse by center or by rim points"));
    ellipse->setStatusTip(QApplication::translate("Sketcher_CreateEllipse", "Create an ellipse by center or by rim points"));
}

bool CmdSketcherCompCreateCircle::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


/* Arc of Circle tool  =================================================================*/
class DrawSketchHandlerArc;

using DrawSketchHandlerArcBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerArc,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::CircleEllipseConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerArc : public DrawSketchHandlerArcBase
{
    friend DrawSketchHandlerArcBase;
public:
    enum SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerArc(ConstructionMethod constrMethod = ConstructionMethod::Center) :
        DrawSketchHandlerArcBase(constrMethod)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0) {}

    virtual ~DrawSketchHandlerArc() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            if (constructionMethod() == ConstructionMethod::Center) {
                centerPoint = onSketchPos;
            }
            else {
                firstPoint = onSketchPos;
            }

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            if (constructionMethod() == ConstructionMethod::Center) {
                firstPoint = onSketchPos;
                double rx = firstPoint.x - centerPoint.x;
                double ry = firstPoint.y - centerPoint.y;
                startAngle = atan2(ry, rx);

                if (snapMode == SnapMode::Snap5Degree) {
                    startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                    firstPoint = centerPoint + radius * Base::Vector2d(cos(startAngle), sin(startAngle));
                }
            }
            else {
                centerPoint = (onSketchPos - firstPoint) / 2 + firstPoint;
                secondPoint = onSketchPos;
            }

            radius = (onSketchPos - centerPoint).Length();

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* circle = new Part::GeomCircle();
            circle->setRadius(radius);
            circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(circle);

            //add line to show the snap at 5 degree.
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x, centerPoint.y, 0.),
                Base::Vector3d(centerPoint.x + cos(startAngle) * 0.8 * radius, centerPoint.y + sin(startAngle) * 0.8 * radius, 0.));
            geometriesToAdd.push_back(line);

            drawEdit(geometriesToAdd);

            double angle = GetPointAngle(centerPoint, onSketchPos);
            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)angle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            try
            {
                double startAngleToDraw = startAngle;
                if (constructionMethod() == ConstructionMethod::Center) {
                    double angle1 = atan2(onSketchPos.y - centerPoint.y,
                        onSketchPos.x - centerPoint.x) - startAngle;
                    double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
                    arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

                    if (snapMode == SnapMode::Snap5Degree) {
                        arcAngle = round(arcAngle / (M_PI / 36)) * M_PI / 36;
                    }

                    if (arcAngle > 0)
                        endAngle = startAngle + arcAngle;
                    else {
                        endAngle = startAngle;
                        startAngleToDraw = startAngle + arcAngle;
                    }
                }
                else {
                    /*Centerline inverts when the arc flips sides.  Easily taken care of by replacing
                    centerline with a point.  It happens because the direction the curve is being drawn
                    reverses.*/
                    centerPoint = Part::Geom2dCircle::getCircleCenter(firstPoint, secondPoint, onSketchPos);
                    radius = (onSketchPos - centerPoint).Length();

                    double angle1 = GetPointAngle(centerPoint, firstPoint);
                    double angle2 = GetPointAngle(centerPoint, secondPoint);
                    double angle3 = GetPointAngle(centerPoint, onSketchPos);

                    // Always build arc counter-clockwise
                    // Point 3 is between Point 1 and 2
                    if (angle3 > min(angle1, angle2) && angle3 < max(angle1, angle2)) {
                        if (angle2 > angle1) {
                            arcPos1 = Sketcher::PointPos::start;
                            arcPos2 = Sketcher::PointPos::end;
                        }
                        else {
                            swapPoints(firstPoint, secondPoint);
                            arcPos1 = Sketcher::PointPos::end;
                            arcPos2 = Sketcher::PointPos::start;
                        }
                        startAngle = min(angle1, angle2);
                        endAngle = max(angle1, angle2);
                        arcAngle = endAngle - startAngle;
                    }
                    // Point 3 is not between Point 1 and 2
                    else {
                        if (angle2 > angle1) {
                            swapPoints(firstPoint, secondPoint);
                            arcPos1 = Sketcher::PointPos::end;
                            arcPos2 = Sketcher::PointPos::start;
                        }
                        else {
                            arcPos1 = Sketcher::PointPos::start;
                            arcPos2 = Sketcher::PointPos::end;
                        }
                        startAngle = max(angle1, angle2);
                        endAngle = min(angle1, angle2);
                        arcAngle = 2 * M_PI - (startAngle - endAngle);
                    }
                    startAngleToDraw = startAngle;
                }

                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomArcOfCircle* arc = new Part::GeomArcOfCircle();
                arc->setRadius(radius);
                arc->setRange(startAngleToDraw, endAngle, true);
                arc->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(arc);
                drawEdit(geometriesToAdd);

                SbString text;
                text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)arcAngle * 180 / M_PI);
                setPositionText(onSketchPos, text);

                if (constructionMethod() == ConstructionMethod::Center) {
                    if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
                else {
                    if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.f, 0.f), AutoConstraint::CURVE)) {
                        renderSuggestConstraintsCursor(sugConstraints[2]);
                        return;
                    }
                }
            }
            catch (Base::ValueError& e) {
                e.ReportException();
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        unsetCursor();
        resetPositionText();

        if (constructionMethod() == ConstructionMethod::Center) {
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
            }
        }

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfCircle"
                "(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),%f),%f,%f),%s)",
                centerPoint.x, centerPoint.y, radius,
                startAngle, endAngle,
                geometryCreationMode == Construction ? "True" : "False");

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add arc: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {
        if (constructionMethod() == ConstructionMethod::Center) {
            // Auto Constraint center point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstraints[0].clear();
            }

            // Auto Constraint first picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start);
                sugConstraints[2].clear();
            }
        }
        else {
            // Auto Constraint first picked point
            if (sugConstraints[0].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), arcPos1);
                sugConstraints[0].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), arcPos2);
                sugConstraints[1].clear();
            }

            // Auto Constraint third picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), Sketcher::PointPos::none);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Arc";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if (constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center)
            return QString::fromLatin1("Sketcher_Pointer_Create_Arc");
        else // constructionMethod == DrawSketchHandlerArc::ConstructionMethod::ThreeRim
            return QString::fromLatin1("Sketcher_Pointer_Create_3PointArc");

        return QStringLiteral("None");
    }

private:
    SnapMode snapMode;
    Base::Vector2d centerPoint, firstPoint, secondPoint;
    double radius, startAngle, endAngle, arcAngle;
    Sketcher::PointPos arcPos1, arcPos2;

    void swapPoints(Base::Vector2d& p1, Base::Vector2d& p2) {
        Base::Vector2d p3 = p1;
        p1 = p2;
        p2 = p3;
    }
};

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Center"), QStringLiteral("3 rim points")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_arc", "x of center"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_arc", "y of center"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_arc", "Radius"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_arc", "Start angle"));
        toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_arc", "Arc angle"));
        toolWidget->configureParameterUnit(WParameter::Fifth, Base::Unit::Angle);

        toolWidget->setNoticeVisible(true);
        toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_p3_notice", "Press Ctrl to snap angles at 5° steps."));
    }
    else {
        toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "x of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "y of 2nd point"));
        toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "x of 3rd point"));
        toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("ToolWidgetManager_p6", "y of 3rd point"));
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->centerPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->centerPoint.y = value;
            break;
        case WParameter::Third:
            dHandler->radius = value;
            break;
        case WParameter::Fourth:
            dHandler->startAngle = value * M_PI / 180;
            break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::ThreeRim)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->firstPoint.x = value;
            break;
        case WParameter::Second:
            dHandler->firstPoint.y = value;
            break;
        case WParameter::Third:
            dHandler->secondPoint.x = value;
            break;
        case WParameter::Fourth:
            dHandler->secondPoint.y = value;
            break;
        }
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            double length = (onSketchPos - dHandler->centerPoint).Length();
            if (toolWidget->isParameterSet(WParameter::Third)) {
                dHandler->radius = toolWidget->getParameter(WParameter::Third);
                if (length != 0.) {
                    onSketchPos = dHandler->centerPoint + (onSketchPos - dHandler->centerPoint) * dHandler->radius / length;
                }
            }
            if (toolWidget->isParameterSet(WParameter::Fourth)) {
                dHandler->startAngle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
                onSketchPos.x = dHandler->centerPoint.x + cos(dHandler->startAngle) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin(dHandler->startAngle) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Third))
                onSketchPos.x = toolWidget->getParameter(WParameter::Third);

            if (toolWidget->isParameterSet(WParameter::Fourth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Fourth);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (toolWidget->isParameterSet(WParameter::Fifth)) {
                dHandler->arcAngle = toolWidget->getParameter(WParameter::Fifth) * M_PI / 180;
                double length = (onSketchPos - dHandler->centerPoint).Length();
                onSketchPos.x = dHandler->centerPoint.x + cos((dHandler->startAngle + dHandler->arcAngle)) * length;
                onSketchPos.y = dHandler->centerPoint.y + sin((dHandler->startAngle + dHandler->arcAngle)) * length;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Fifth))
                onSketchPos.x = toolWidget->getParameter(WParameter::Fifth);

            if (toolWidget->isParameterSet(WParameter::Sixth))
                onSketchPos.y = toolWidget->getParameter(WParameter::Sixth);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, dHandler->radius);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->startAngle * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, onSketchPos.y);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, dHandler->arcAngle * 180 / M_PI, Base::Unit::Angle);
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Fifth))
                toolWidget->updateVisualValue(WParameter::Fifth, onSketchPos.x);

            if (!toolWidget->isParameterSet(WParameter::Sixth))
                toolWidget->updateVisualValue(WParameter::Sixth, onSketchPos.y);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);

            handler->updateDataAndDrawToPosition(prevCursorPosition);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);

            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth) &&
            dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);

            handler->setState(SelectMode::End);
            handler->finish();
        }

        if ((toolWidget->isParameterSet(WParameter::Fifth) ||
            toolWidget->isParameterSet(WParameter::Sixth)) &&
            dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::ThreeRim) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);


            if (toolWidget->isParameterSet(WParameter::Fifth) &&
                toolWidget->isParameterSet(WParameter::Sixth)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerArcBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();
    using namespace Sketcher;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    if (dHandler->constructionMethod() == DrawSketchHandlerArc::ConstructionMethod::Center) {
        auto radiusSet = toolWidget->isParameterSet(WParameter::Third);
        auto arcAngleSet = toolWidget->isParameterSet(WParameter::Fifth);


        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }

        if (radiusSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ", firstCurve, dHandler->radius);

        if (arcAngleSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ", firstCurve, dHandler->arcAngle);
    }
    else {
        auto x1 = toolWidget->getParameter(WParameter::Third);
        auto y1 = toolWidget->getParameter(WParameter::Fourth);

        auto x1set = toolWidget->isParameterSet(WParameter::Third);
        auto y1set = toolWidget->isParameterSet(WParameter::Fourth);

        if (x0set && y0set && x0 == 0. && y0 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::RtPnt,
                x0, handler->sketchgui->getObject());
        }
        else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos1), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
        }
        if (x1set && y1set && x1 == 0. && y1 == 0.) {
            ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::RtPnt,
                x1, handler->sketchgui->getObject());
        }
        else {
            if (x1set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::VAxis,
                    x1, handler->sketchgui->getObject());

            if (y1set)
                ConstraintToAttachment(GeoElementId(firstCurve, dHandler->arcPos2), GeoElementId::HAxis,
                    y1, handler->sketchgui->getObject());
        }
    }
}

DEF_STD_CMD_A(CmdSketcherCreateArc)

CmdSketcherCreateArc::CmdSketcherCreateArc()
    : Command("Sketcher_CreateArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc of circle");
    sToolTipText = QT_TR_NOOP("Create an arc by its center or by its end points");
    sWhatsThis = "Sketcher_CreateArc";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArc";
    sAccel = "G, A";
    eType = ForEdit;
}

void CmdSketcherCreateArc::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArc());
}

bool CmdSketcherCreateArc::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Arc of Ellipse tool  ===============================================================*/

class DrawSketchHandlerArcOfEllipse : public DrawSketchHandler
{
public:
    DrawSketchHandlerArcOfEllipse()
        : Mode(STATUS_SEEK_First), EditCurve(34)
        , rx(0), ry(0), startAngle(0), endAngle(0)
        , arcAngle(0), arcAngle_t(0) {}

    virtual ~DrawSketchHandlerArcOfEllipse() = default;

    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_SEEK_Second,     /**< enum value ----. */
        STATUS_SEEK_Third,      /**< enum value ----. */
        STATUS_SEEK_Fourth,     /**< enum value ----. */
        STATUS_Close
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f,0.f))) { // TODO: ellipse prio 1
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Second) {
            double rx0 = onSketchPos.x - EditCurve[0].x;
            double ry0 = onSketchPos.y - EditCurve[0].y;
            for (int i=0; i < 16; i++) {
                double angle = i*M_PI/16.0;
                double rx1 = rx0 * cos(angle) + ry0 * sin(angle);
                double ry1 = -rx0 * sin(angle) + ry0 * cos(angle);
                EditCurve[1+i] = Base::Vector2d(EditCurve[0].x + rx1, EditCurve[0].y + ry1);
                EditCurve[17+i] = Base::Vector2d(EditCurve[0].x - rx1, EditCurve[0].y - ry1);
            }
            EditCurve[33] = EditCurve[1];

            // Display radius for user
            float radius = (onSketchPos - EditCurve[0]).Length();

            SbString text;
            text.sprintf(" (%.1fR,%.1fR)", radius,radius);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, onSketchPos - centerPoint,
                                   AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Third) {
            // angle between the major axis of the ellipse and the X axis
            double a = (EditCurve[1]-EditCurve[0]).Length();
            double phi = atan2(EditCurve[1].y-EditCurve[0].y,EditCurve[1].x-EditCurve[0].x);

            // This is the angle at cursor point
            double angleatpoint = acos((onSketchPos.x-EditCurve[0].x+(onSketchPos.y-EditCurve[0].y)*tan(phi))/(a*(cos(phi)+tan(phi)*sin(phi))));
            double b=(onSketchPos.y-EditCurve[0].y-a*cos(angleatpoint)*sin(phi))/(sin(angleatpoint)*cos(phi));

            for (int i=1; i < 16; i++) {
                double angle = i*M_PI/16.0;
                double rx1 = a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi);
                double ry1 = a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi);
                EditCurve[1+i] = Base::Vector2d(EditCurve[0].x + rx1, EditCurve[0].y + ry1);
                EditCurve[17+i] = Base::Vector2d(EditCurve[0].x - rx1, EditCurve[0].y - ry1);
            }
            EditCurve[33] = EditCurve[1];
            EditCurve[17] = EditCurve[16];

            // Display radius for user
            SbString text;
            text.sprintf(" (%.1fR,%.1fR)", a, b);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Fourth) { // here we differ from ellipse creation
            // angle between the major axis of the ellipse and the X axis
            double a = (axisPoint-centerPoint).Length();
            double phi = atan2(axisPoint.y-centerPoint.y,axisPoint.x-centerPoint.x);

            // This is the angle at cursor point
            double angleatpoint = acos((startingPoint.x-centerPoint.x+(startingPoint.y-centerPoint.y)*tan(phi))/(a*(cos(phi)+tan(phi)*sin(phi))));
            double b=abs((startingPoint.y-centerPoint.y-a*cos(angleatpoint)*sin(phi))/(sin(angleatpoint)*cos(phi)));

            double rxs = startingPoint.x - centerPoint.x;
            double rys = startingPoint.y - centerPoint.y;
            startAngle = atan2(a*(rys*cos(phi)-rxs*sin(phi)), b*(rxs*cos(phi)+rys*sin(phi))); // eccentric anomaly angle

            double angle1 = atan2(a*((onSketchPos.y - centerPoint.y)*cos(phi)-(onSketchPos.x - centerPoint.x)*sin(phi)),
                                  b*((onSketchPos.x - centerPoint.x)*cos(phi)+(onSketchPos.y - centerPoint.y)*sin(phi)))- startAngle;

            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI ;
            arcAngle = abs(angle1-arcAngle) < abs(angle2-arcAngle) ? angle1 : angle2;

            for (int i=0; i < 34; i++) {
                double angle = startAngle+i*arcAngle/34.0;
                double rx1 = a * cos(angle) * cos(phi) - b * sin(angle) * sin(phi);
                double ry1 = a * cos(angle) * sin(phi) + b * sin(angle) * cos(phi);
                EditCurve[i] = Base::Vector2d(centerPoint.x + rx1, centerPoint.y + ry1);
            }
//             EditCurve[33] = EditCurve[1];
//             EditCurve[17] = EditCurve[16];

            // Display radii and angle for user
            SbString text;
            text.sprintf(" (%.1fR,%.1fR,%.1fdeg)", a, b, arcAngle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr4);
                return;
            }
        }



        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First){
            EditCurve[0] = onSketchPos;
            centerPoint = onSketchPos;
            Mode = STATUS_SEEK_Second;
        }
        else if(Mode==STATUS_SEEK_Second) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            Mode = STATUS_SEEK_Third;
        }
        else if(Mode==STATUS_SEEK_Third) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            arcAngle_t= 0.;
            Mode = STATUS_SEEK_Fourth;
        }
        else { // Fourth
            endPoint = onSketchPos;

            Mode = STATUS_Close;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode==STATUS_Close) {
            unsetCursor();
            resetPositionText();

            // angle between the major axis of the ellipse and the X axisEllipse
            double a = (axisPoint-centerPoint).Length();
            double phi = atan2(axisPoint.y-centerPoint.y,axisPoint.x-centerPoint.x);

            // This is the angle at cursor point
            double angleatpoint = acos((startingPoint.x-centerPoint.x+(startingPoint.y-centerPoint.y)*tan(phi))/(a*(cos(phi)+tan(phi)*sin(phi))));
            double b=abs((startingPoint.y-centerPoint.y-a*cos(angleatpoint)*sin(phi))/(sin(angleatpoint)*cos(phi)));

            double angle1 = atan2(a*((endPoint.y - centerPoint.y)*cos(phi)-(endPoint.x - centerPoint.x)*sin(phi)),
                                  b*((endPoint.x - centerPoint.x)*cos(phi)+(endPoint.y - centerPoint.y)*sin(phi)))- startAngle;

            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI ;
            arcAngle = abs(angle1-arcAngle) < abs(angle2-arcAngle) ? angle1 : angle2;

            bool isOriginalArcCCW=true;

            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
                isOriginalArcCCW=false;
            }

            Base::Vector2d majAxisDir,minAxisDir,minAxisPoint,majAxisPoint;
            // We always create a CCW ellipse, because we want our XY reference system to be in the +X +Y direction
            // Our normal will then always be in the +Z axis (local +Z axis of the sketcher)

            if(a>b)
            {
                // force second semidiameter to be perpendicular to first semidiamater
                majAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(-majAxisDir.y,majAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                minAxisPoint = centerPoint+perp;
                majAxisPoint = centerPoint+majAxisDir;
            }
            else {
                // force second semidiameter to be perpendicular to first semidiamater
                minAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(minAxisDir.y,-minAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                majAxisPoint = centerPoint+perp;
                minAxisPoint = centerPoint+minAxisDir;
                endAngle +=  M_PI/2;
                startAngle += M_PI/2;
                phi-=M_PI/2;
                double t=a; a=b; b=t;//swap a,b
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of ellipse"));

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfEllipse"
                    "(Part.Ellipse(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%f,%f),%s)",
                        majAxisPoint.x, majAxisPoint.y,
                        minAxisPoint.x, minAxisPoint.y,
                        centerPoint.x, centerPoint.y,
                        startAngle, endAngle,
                        geometryCreationMode==Construction?"True":"False");

                currentgeoid++;

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            // add auto constraints for the center point
            if (sugConstr1.size() > 0) {
                createAutoConstraints(sugConstr1, currentgeoid, Sketcher::PointPos::mid);
                sugConstr1.clear();
            }

            // add suggested constraints for arc
            if (sugConstr2.size() > 0) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::none);
                sugConstr2.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr3.size() > 0) {
                createAutoConstraints(sugConstr3, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::start:Sketcher::PointPos::end);
                sugConstr3.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr4.size() > 0) {
                createAutoConstraints(sugConstr4, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::end:Sketcher::PointPos::start);
                sugConstr4.clear();
            }

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);
            if(continuousMode){
                // This code enables the continuous creation mode.
                Mode=STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(34);
                applyCursor();
                /* this is ok not to call to purgeHandler
                * in continuous creation mode because the
                * handler is destroyed by the quit() method on pressing the
                * right button of the mouse */
            }
            else{
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
        }
        return true;
    }

private:
    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_ArcOfEllipse");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d centerPoint, axisPoint, startingPoint, endPoint;
    double rx, ry, startAngle, endAngle, arcAngle, arcAngle_t;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;
};

DEF_STD_CMD_A(CmdSketcherCreateArcOfEllipse)

CmdSketcherCreateArcOfEllipse::CmdSketcherCreateArcOfEllipse()
  : Command("Sketcher_CreateArcOfEllipse")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of ellipse");
    sToolTipText    = QT_TR_NOOP("Create an arc of ellipse in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfEllipse";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateElliptical_Arc";
    sAccel          = "G, E, A";
    eType           = ForEdit;
}

void CmdSketcherCreateArcOfEllipse::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfEllipse() );
}

bool CmdSketcherCreateArcOfEllipse::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

class DrawSketchHandlerArcOfHyperbola : public DrawSketchHandler
{
public:
    DrawSketchHandlerArcOfHyperbola()
      : Mode(STATUS_SEEK_First)
      , EditCurve(34)
      , arcAngle(0)
      , arcAngle_t(0) {}

    virtual ~DrawSketchHandlerArcOfHyperbola() = default;
    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_SEEK_Second,     /**< enum value ----. */
        STATUS_SEEK_Third,     /**< enum value ----. */
        STATUS_SEEK_Fourth,     /**< enum value ----. */
        STATUS_Close
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Second) {
            EditCurve[1]= onSketchPos;

            // Display radius for user
            float radius = (onSketchPos - centerPoint).Length();

            SbString text;
            text.sprintf(" (%.1fR,%.1fR)", radius,radius);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f,0.f),
                                   AutoConstraint::CURVE)) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Third) {
            // angle between the major axis of the hyperbola and the X axis
            double a = (axisPoint-centerPoint).Length();
            double phi = atan2(axisPoint.y-centerPoint.y,axisPoint.x-centerPoint.x);

            // This is the angle at cursor point
            double angleatpoint = acosh(((onSketchPos.x-centerPoint.x)*cos(phi)+(onSketchPos.y-centerPoint.y)*sin(phi))/a);
            double b=(onSketchPos.y-centerPoint.y-a*cosh(angleatpoint)*sin(phi))/(sinh(angleatpoint)*cos(phi));

            if(!boost::math::isnan(b)){
                for (int i=15; i >= -15; i--) {
                    // P(U) = O + MajRad*Cosh(U)*XDir + MinRad*Sinh(U)*YDir
                    //double angle = i*M_PI/16.0;
                    double angle=i*angleatpoint/15;
                    double rx = a * cosh(angle) * cos(phi) - b * sinh(angle) * sin(phi);
                    double ry = a * cosh(angle) * sin(phi) + b * sinh(angle) * cos(phi);
                    EditCurve[15+i] = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
                }

                // Display radius for user
                SbString text;
                text.sprintf(" (%.1fR,%.1fR)", a, b);
                setPositionText(onSketchPos, text);
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Fourth) {
            // angle between the major axis of the hyperbola and the X axis
            double a = (axisPoint-centerPoint).Length();
            double phi = atan2(axisPoint.y-centerPoint.y,axisPoint.x-centerPoint.x);

            // This is the angle at cursor point
            double angleatstartingpoint = acosh(((startingPoint.x-centerPoint.x)*cos(phi)+(startingPoint.y-centerPoint.y)*sin(phi))/a);
            double b=(startingPoint.y-centerPoint.y-a*cosh(angleatstartingpoint)*sin(phi))/(sinh(angleatstartingpoint)*cos(phi));

            double startAngle = angleatstartingpoint;

            //double angleatpoint = acosh(((onSketchPos.x-centerPoint.x)*cos(phi)+(onSketchPos.y-centerPoint.y)*sin(phi))/a);

            double angleatpoint = atanh( (((onSketchPos.y-centerPoint.y)*cos(phi)-(onSketchPos.x-centerPoint.x)*sin(phi))*a) /
                                         (((onSketchPos.x-centerPoint.x)*cos(phi)+(onSketchPos.y-centerPoint.y)*sin(phi))*b)  );

            /*double angle1 = angleatpoint - startAngle;

            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI ;
            arcAngle = abs(angle1-arcAngle) < abs(angle2-arcAngle) ? angle1 : angle2;*/

            arcAngle = angleatpoint - startAngle;

            //if(!boost::math::isnan(angle1) && !boost::math::isnan(angle2)){
            if (!boost::math::isnan(arcAngle)) {
                EditCurve.resize(33);
                for (int i=0; i < 33; i++) {
                    // P(U) = O + MajRad*Cosh(U)*XDir + MinRad*Sinh(U)*YDir
                    //double angle=i*angleatpoint/16;
                    double angle = startAngle+i*arcAngle/32.0;
                    double rx = a * cosh(angle) * cos(phi) - b * sinh(angle) * sin(phi);
                    double ry = a * cosh(angle) * sin(phi) + b * sinh(angle) * cos(phi);
                    EditCurve[i] = Base::Vector2d(centerPoint.x + rx, centerPoint.y + ry);
                }

                // Display radius for user
                SbString text;
                text.sprintf(" (%.1fR,%.1fR)", a, b);
                setPositionText(onSketchPos, text);
            }
            else {
                arcAngle=0.;
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr4);
                return;
            }
        }

        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First){
            EditCurve[0] = onSketchPos;
            centerPoint = onSketchPos;
            EditCurve.resize(2);
            Mode = STATUS_SEEK_Second;
        }
        else if(Mode==STATUS_SEEK_Second) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            EditCurve.resize(31);
            Mode = STATUS_SEEK_Third;
        }
        else if(Mode==STATUS_SEEK_Third) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            arcAngle_t= 0.;
            Mode = STATUS_SEEK_Fourth;
        }
        else { // Fourth
            endPoint = onSketchPos;

            Mode = STATUS_Close;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d /*onSketchPos*/) override
    {
        if (Mode==STATUS_Close) {
            unsetCursor();
            resetPositionText();


            // angle between the major axis of the hyperbola and the X axis
            double a = (axisPoint-centerPoint).Length();
            double phi = atan2(axisPoint.y-centerPoint.y,axisPoint.x-centerPoint.x);

            // This is the angle at cursor point
            double angleatstartingpoint = acosh(((startingPoint.x-centerPoint.x)*cos(phi)+(startingPoint.y-centerPoint.y)*sin(phi))/a);
            double b=(startingPoint.y-centerPoint.y-a*cosh(angleatstartingpoint)*sin(phi))/(sinh(angleatstartingpoint)*cos(phi));

            double startAngle = angleatstartingpoint;

            //double angleatpoint = acosh(((onSketchPos.x-centerPoint.x)*cos(phi)+(onSketchPos.y-centerPoint.y)*sin(phi))/a);

            double endAngle = atanh( (((endPoint.y-centerPoint.y)*cos(phi)-(endPoint.x-centerPoint.x)*sin(phi))*a) /
                                         (((endPoint.x-centerPoint.x)*cos(phi)+(endPoint.y-centerPoint.y)*sin(phi))*b)  );

            if (boost::math::isnan(startAngle) || boost::math::isnan(endAngle)) {
                sketchgui->purgeHandler();
                Base::Console().Error("Cannot create arc of hyperbola from invalid angles, try again!\n");
                return false;
            }


            bool isOriginalArcCCW=true;

            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
                isOriginalArcCCW=false;
            }

            Base::Vector2d majAxisDir,minAxisDir,minAxisPoint,majAxisPoint;
            // We always create a CCW hyperbola, because we want our XY reference system to be in the +X +Y direction
            // Our normal will then always be in the +Z axis (local +Z axis of the sketcher)

            if(a>b)
            {
                // force second semidiameter to be perpendicular to first semidiamater
                majAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(-majAxisDir.y,majAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                minAxisPoint = centerPoint+perp;
                majAxisPoint = centerPoint+majAxisDir;
            }
            else {
                // force second semidiameter to be perpendicular to first semidiamater
                minAxisDir = axisPoint - centerPoint;
                Base::Vector2d perp(minAxisDir.y,-minAxisDir.x);
                perp.Normalize();
                perp.Scale(abs(b));
                majAxisPoint = centerPoint+perp;
                minAxisPoint = centerPoint+minAxisDir;
                endAngle +=  M_PI/2;
                startAngle += M_PI/2;
            }

            int currentgeoid = getHighestCurveIndex();

            try {

                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of hyperbola"));

                //Add arc of hyperbola, point and constrain point as focus2. We add focus2 for it to balance
                //the intrinsic focus1, in order to balance out the intrinsic invisible focus1 when AOE is
                //dragged by its center
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfHyperbola"
                    "(Part.Hyperbola(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App.Vector(%f,%f,0)),%f,%f),%s)",
                    majAxisPoint.x, majAxisPoint.y,
                    minAxisPoint.x, minAxisPoint.y,
                    centerPoint.x, centerPoint.y,
                    startAngle, endAngle,
                    geometryCreationMode==Construction?"True":"False");

                currentgeoid++;

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            // add auto constraints for the center point
            if (sugConstr1.size() > 0) {
                createAutoConstraints(sugConstr1, currentgeoid, Sketcher::PointPos::mid);
                sugConstr1.clear();
            }

            // add suggested constraints for arc
            if (sugConstr2.size() > 0) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::none);
                sugConstr2.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr3.size() > 0) {
                createAutoConstraints(sugConstr3, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::start:Sketcher::PointPos::end);
                sugConstr3.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr4.size() > 0) {
                createAutoConstraints(sugConstr4, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::end:Sketcher::PointPos::start);
                sugConstr4.clear();
            }

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

            if(continuousMode){
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(34);
                applyCursor();
                /* It is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else{
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
        }
        return true;
    }

private:
    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_ArcOfHyperbola");
    }


protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d centerPoint, axisPoint, startingPoint, endPoint;
    double arcAngle, arcAngle_t;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;

};

DEF_STD_CMD_A(CmdSketcherCreateArcOfHyperbola)

CmdSketcherCreateArcOfHyperbola::CmdSketcherCreateArcOfHyperbola()
  : Command("Sketcher_CreateArcOfHyperbola")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of hyperbola");
    sToolTipText    = QT_TR_NOOP("Create an arc of hyperbola in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfHyperbola";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateHyperbolic_Arc";
    sAccel          = "G, H";
    eType           = ForEdit;
}

void CmdSketcherCreateArcOfHyperbola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfHyperbola() );
}

bool CmdSketcherCreateArcOfHyperbola::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

class DrawSketchHandlerArcOfParabola : public DrawSketchHandler
{
public:
    DrawSketchHandlerArcOfParabola()
        : Mode(STATUS_SEEK_First)
        , EditCurve(34)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0)
        , arcAngle_t(0) {}

    virtual ~DrawSketchHandlerArcOfParabola() = default;

    /// mode table
    enum SelectMode {
        STATUS_SEEK_First,      /**< enum value ----. */
        STATUS_SEEK_Second,     /**< enum value ----. */
        STATUS_SEEK_Third,      /**< enum value ----. */
        STATUS_SEEK_Fourth,     /**< enum value ----. */
        STATUS_Close
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First) {
            setPositionText(onSketchPos);
            if (seekAutoConstraint(sugConstr1, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr1);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Second) {
            EditCurve[1]= onSketchPos;

            // Display radius for user
            float radius = (onSketchPos - focusPoint).Length();

            SbString text;
            text.sprintf(" (F%.1f)", radius);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr2, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr2);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Third) {
            double focal = (axisPoint-focusPoint).Length();
            double phi = atan2(focusPoint.y-axisPoint.y,focusPoint.x-axisPoint.x);

            // P(U) = O + U*U/(4.*F)*XDir + U*YDir
            //
            // pnt = Base::Vector3d(pnt0.x + angle * angle / 4 / focal * cos(phi) - angle * sin(phi),
            //                      pnt0.y + angle * angle / 4 / focal * sin(phi) + angle * cos(phi),
            //                      0.f);

            // This is the angle at cursor point
            double u =
            ( cos(phi) * (onSketchPos.y - axisPoint.y) - (onSketchPos.x - axisPoint.x) * sin(phi));

            for (int i=15; i >= -15; i--) {
                double angle=i*u/15;
                double rx = angle * angle / 4 / focal * cos(phi) - angle * sin(phi);
                double ry = angle * angle / 4 / focal * sin(phi) + angle * cos(phi);
                EditCurve[15+i] = Base::Vector2d(axisPoint.x + rx, axisPoint.y + ry);
            }

            // Display radius for user
            SbString text;
            text.sprintf(" (F%.1f)", focal);
            setPositionText(onSketchPos, text);

            drawEdit(EditCurve);

            if (seekAutoConstraint(sugConstr3, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr3);
                return;
            }
        }
        else if (Mode==STATUS_SEEK_Fourth) {
            double focal = (axisPoint-focusPoint).Length();
            double phi = atan2(focusPoint.y-axisPoint.y,focusPoint.x-axisPoint.x);

            // P(U) = O + U*U/(4.*F)*XDir + U*YDir
            //
            // pnt = Base::Vector3d(pnt0.x + angle * angle / 4 / focal * cos(phi) - angle * sin(phi),
            //                      pnt0.y + angle * angle / 4 / focal * sin(phi) + angle * cos(phi),
            //                      0.f);

            // This is the angle at starting point
            double ustartpoint =
            ( cos(phi) * (startingPoint.y - axisPoint.y) - (startingPoint.x - axisPoint.x) * sin(phi));

            double startValue = ustartpoint;

            double u =
            ( cos(phi) * (onSketchPos.y - axisPoint.y) - (onSketchPos.x - axisPoint.x) * sin(phi));


            arcAngle = u - startValue;

            if (!boost::math::isnan(arcAngle)) {
                EditCurve.resize(33);
                for (std::size_t i=0; i < 33; i++) {
                    double angle = startValue+i*arcAngle/32.0;
                    double rx = angle * angle / 4 / focal * cos(phi) - angle * sin(phi);
                    double ry = angle * angle / 4 / focal * sin(phi) + angle * cos(phi);
                    EditCurve[i] = Base::Vector2d(axisPoint.x + rx, axisPoint.y + ry);
                }

                SbString text;
                text.sprintf(" (F%.1f)", focal);
                setPositionText(onSketchPos, text);
            }
            else {
                arcAngle=0.;
            }

            drawEdit(EditCurve);
            if (seekAutoConstraint(sugConstr4, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr4);
                return;
            }
        }

        applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        if (Mode==STATUS_SEEK_First){
            EditCurve[0] = onSketchPos;
            focusPoint = onSketchPos;
            EditCurve.resize(2);
            Mode = STATUS_SEEK_Second;
        }
        else if(Mode==STATUS_SEEK_Second) {
            EditCurve[1] = onSketchPos;
            axisPoint = onSketchPos;
            EditCurve.resize(31);
            Mode = STATUS_SEEK_Third;
        }
        else if(Mode==STATUS_SEEK_Third) {
            startingPoint = onSketchPos;
            arcAngle = 0.;
            arcAngle_t= 0.;
            Mode = STATUS_SEEK_Fourth;
        }
        else { // Fourth
            endPoint = onSketchPos;
            Mode = STATUS_Close;
        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d /*onSketchPos*/) override
    {
        if (Mode==STATUS_Close) {
            unsetCursor();
            resetPositionText();

            double phi = atan2(focusPoint.y-axisPoint.y,focusPoint.x-axisPoint.x);

            double ustartpoint =
            ( cos(phi) * (startingPoint.y - axisPoint.y) - (startingPoint.x - axisPoint.x) * sin(phi));

            double uendpoint =
            ( cos(phi) * (endPoint.y - axisPoint.y) - (endPoint.x - axisPoint.x) * sin(phi));

            double startAngle = ustartpoint;

            double endAngle = uendpoint;

            bool isOriginalArcCCW=true;

            if (arcAngle > 0) {
                endAngle = startAngle + arcAngle;
            }
            else {
                endAngle = startAngle;
                startAngle += arcAngle;
                isOriginalArcCCW=false;
            }

            int currentgeoid = getHighestCurveIndex();

            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch arc of Parabola"));

                //Add arc of parabola
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.ArcOfParabola"
                    "(Part.Parabola(App.Vector(%f,%f,0),App.Vector(%f,%f,0),App.Vector(0,0,1)),%f,%f),%s)",
                        focusPoint.x, focusPoint.y,
                        axisPoint.x, axisPoint.y,
                        startAngle, endAngle,
                        geometryCreationMode==Construction?"True":"False");

                currentgeoid++;

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            // add auto constraints for the focus point
            if (sugConstr1.size() > 0) {
                createAutoConstraints(sugConstr1, currentgeoid+1, Sketcher::PointPos::start);
                sugConstr1.clear();
            }

            // add suggested constraints for vertex point
            if (sugConstr2.size() > 0) {
                createAutoConstraints(sugConstr2, currentgeoid, Sketcher::PointPos::mid);
                sugConstr2.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr3.size() > 0) {
                createAutoConstraints(sugConstr3, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::start:Sketcher::PointPos::end);
                sugConstr3.clear();
            }

            // add suggested constraints for start of arc
            if (sugConstr4.size() > 0) {
                createAutoConstraints(sugConstr4, currentgeoid, isOriginalArcCCW?Sketcher::PointPos::end:Sketcher::PointPos::start);
                sugConstr4.clear();
            }

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);
            if (continuousMode) {
                // This code enables the continuous creation mode.
                Mode = STATUS_SEEK_First;
                EditCurve.clear();
                drawEdit(EditCurve);
                EditCurve.resize(34);
                applyCursor();
                /* It is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else {
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
        }
        return true;
    }

private:
    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_ArcOfParabola");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    Base::Vector2d focusPoint, axisPoint, startingPoint, endPoint;
    double startAngle, endAngle, arcAngle, arcAngle_t;
    std::vector<AutoConstraint> sugConstr1, sugConstr2, sugConstr3, sugConstr4;
};

DEF_STD_CMD_A(CmdSketcherCreateArcOfParabola)

CmdSketcherCreateArcOfParabola::CmdSketcherCreateArcOfParabola()
  : Command("Sketcher_CreateArcOfParabola")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create an arc of parabola");
    sToolTipText    = QT_TR_NOOP("Create an arc of parabola in the sketch");
    sWhatsThis      = "Sketcher_CreateArcOfParabola";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateParabolic_Arc";
    sAccel          = "G, J";
    eType           = ForEdit;
}

void CmdSketcherCreateArcOfParabola::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerArcOfParabola() );
}

bool CmdSketcherCreateArcOfParabola::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// Comp for arcs (circle, ellipse, hyperbola, parabola)===========================================

DEF_STD_CMD_ACLU(CmdSketcherCompCreateArc)

CmdSketcherCompCreateArc::CmdSketcherCompCreateArc()
    : Command("Sketcher_CompCreateArc")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc");
    sToolTipText = QT_TR_NOOP("Create an arc in the sketcher");
    sWhatsThis = "Sketcher_CompCreateArc";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompCreateArc::activated(int iMsg)
{
    if (iMsg == 0)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArc());
    else if (iMsg == 1)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfEllipse());
    else if (iMsg == 2)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfHyperbola());
    else if (iMsg == 3)
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcOfParabola());
    else
        return;

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateArc::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc = pcAction->addAction(QString());
    arc->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));

    QAction* arcofellipse = pcAction->addAction(QString());
    arcofellipse->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));

    QAction* arcofhyperbola = pcAction->addAction(QString());
    arcofhyperbola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));

    QAction* arcofparabola = pcAction->addAction(QString());
    arcofparabola->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateArc::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArc_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateElliptical_Arc_Constr"));
        a[2]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateHyperbolic_Arc_Constr"));
        a[3]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateParabolic_Arc_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateArc::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of Circle"));
    arc1->setToolTip(QApplication::translate("Sketcher_CreateArc", "Create an arc by its center or by its end points"));
    arc1->setStatusTip(QApplication::translate("Sketcher_CreateArc", "Create an arc by its center or by its end points"));
    QAction* arcofellipse = a[1];
    arcofellipse->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of ellipse"));
    arcofellipse->setToolTip(QApplication::translate("Sketcher_CreateArcOfEllipse", "Create an arc of ellipse by its center, major radius, and endpoints"));
    arcofellipse->setStatusTip(QApplication::translate("Sketcher_CreateArcOfEllipse", "Create an arc of ellipse by its center, major radius, and endpoints"));
    QAction* arcofhyperbola = a[2];
    arcofhyperbola->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of hyperbola"));
    arcofhyperbola->setToolTip(QApplication::translate("Sketcher_CreateArcOfHyperbola", "Create an arc of hyperbola by its center, major radius, and endpoints"));
    arcofhyperbola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfHyperbola", "Create an arc of hyperbola by its center, major radius, and endpoints"));
    QAction* arcofparabola = a[3];
    arcofparabola->setText(QApplication::translate("CmdSketcherCompCreateArc", "Arc of parabola"));
    arcofparabola->setToolTip(QApplication::translate("Sketcher_CreateArcOfParabola", "Create an arc of parabola by its focus, vertex, and endpoints"));
    arcofparabola->setStatusTip(QApplication::translate("Sketcher_CreateArcOfParabola", "Create an arc of parabola by its focus, vertex, and endpoints"));
}

bool CmdSketcherCompCreateArc::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// ======================================================================================

class DrawSketchHandlerBSpline: public DrawSketchHandler
{
public:
    DrawSketchHandlerBSpline(int constructionMethod)
      : Mode(STATUS_SEEK_FIRST_CONTROLPOINT)
      , MousePressMode(MOUSE_NOT_PRESSED)
      , ConstrMethod(constructionMethod)
      , SplineDegree(3)
      , IsClosed(false)
    {
        addSugConstraint();
        applyCursor();
    }

    virtual ~DrawSketchHandlerBSpline() = default;

    /// modes
    enum SELECT_MODE {
        STATUS_SEEK_FIRST_CONTROLPOINT,
        STATUS_SEEK_ADDITIONAL_CONTROLPOINTS,
        STATUS_CLOSE
    };

    // TODO: this kind of behavior will be useful in a superclass
    // when LMB is pressed it's a transitional state so some undos can't be done
    // (like delete last pole)
    enum MOUSE_PRESS_MODE {
        MOUSE_PRESSED,
        MOUSE_NOT_PRESSED
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        if (Mode==STATUS_SEEK_FIRST_CONTROLPOINT) {
            setPositionText(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr.back());
                return;
            }
        }
        else if (Mode==STATUS_SEEK_ADDITIONAL_CONTROLPOINTS) {

            drawControlPolygonToPosition(onSketchPos);

            drawCursorToPosition(onSketchPos);

            if (seekAutoConstraint(sugConstr.back(), onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(sugConstr.back());
                return;
            }
        }
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;

        MousePressMode = MOUSE_PRESSED;

        if (Mode == STATUS_SEEK_FIRST_CONTROLPOINT) {
            BSplinePoles.push_back(onSketchPos);

            Mode = STATUS_SEEK_ADDITIONAL_CONTROLPOINTS;

            // insert circle point for pole, defer internal alignment constraining.
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Pole circle"));

                //Add pole
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),10),True)",
                                      BSplinePoles.back().x, BSplinePoles.back().y);

                poleGeoIds.push_back(getHighestCurveIndex());

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Weight',%d,%f)) ",
                                      poleGeoIds.back(), 1.0 ); // First pole defaults to 1.0 weight
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return false;
            }

            //Gui::Command::commitCommand();

            //static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            // add auto constraints on pole
            if (sugConstr.back().size() > 0) {
                createAutoConstraints(sugConstr.back(), poleGeoIds.back(), Sketcher::PointPos::mid, false);
            }

            static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            addSugConstraint();

        }
        else if (Mode == STATUS_SEEK_ADDITIONAL_CONTROLPOINTS) {
            BSplinePoles.push_back(onSketchPos);

            // check if coincident with first pole
            for(auto & ac : sugConstr.back()) {
                if( ac.Type == Sketcher::Coincident && ac.GeoId == poleGeoIds[0] && ac.PosId == Sketcher::PointPos::mid ) {
                    IsClosed = true;
                }
            }

            if (IsClosed) {
                Mode = STATUS_CLOSE;

                if (ConstrMethod == 1) { // if periodic we do not need the last pole
                    BSplinePoles.pop_back();
                    sugConstr.pop_back();

                    return true;
                }
            }

            // insert circle point for pole, defer internal alignment constraining.
            try {

                //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add Pole circle"));

                //Add pole
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Circle(App.Vector(%f,%f,0),App.Vector(0,0,1),10),True)",
                                      BSplinePoles.back().x,BSplinePoles.back().y);

                poleGeoIds.push_back(getHighestCurveIndex());

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
                                      poleGeoIds[0], poleGeoIds.back());

            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return false;
            }

            //Gui::Command::commitCommand();

            //static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            // add auto constraints on pole
            if (sugConstr.back().size() > 0) {
                createAutoConstraints(sugConstr.back(), poleGeoIds.back(), Sketcher::PointPos::mid, false);
            }

            //static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

            if (!IsClosed) {
                addSugConstraint();
            }

        }
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        prevCursorPosition = onSketchPos;
        MousePressMode = MOUSE_NOT_PRESSED;

        return finishCommand(onSketchPos);
    }

    virtual void registerPressedKey(bool pressed, int key) override
    {
        if (SoKeyboardEvent::D == key && pressed) {
            SplineDegree = QInputDialog::getInt(
                Gui::getMainWindow(),
                QObject::tr("B-Spline Degree"),
                QObject::tr("Define B-Spline Degree, between 1 and %1:")
                .arg(QString::number(Geom_BSplineCurve::MaxDegree())),
                SplineDegree, 1, Geom_BSplineCurve::MaxDegree(), 1);
            // FIXME: Pressing Esc here also finishes the B-Spline creation.
            // The user may only want to exit the dialog.
        }
        // On pressing Backspace delete last pole
        else if (SoKeyboardEvent::BACKSPACE == key && pressed) {
            // when mouse is pressed we are in a transitional state so don't mess with it
            if (MOUSE_PRESSED == MousePressMode)
                return;

            // can only delete last pole if it exists
            if (STATUS_SEEK_FIRST_CONTROLPOINT == Mode ||
                STATUS_CLOSE == Mode)
                return;

            // if only first pole exists it's equivalent to canceling current spline
            if (poleGeoIds.size() == 1) {
                // this also exits b-spline creation if continuous mode is off
                this->quit();
                return;
            }

            // reverse the steps of press/release button
            try {
                // already ensured that CurrentConstraint == EditCurve.size() > 1
                const int delGeoId = poleGeoIds.back();
                const auto& constraints = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->Constraints.getValues();
                for (int i = constraints.size() - 1; i >= 0; --i) {
                    if (delGeoId == constraints[i]->First ||
                        delGeoId == constraints[i]->Second ||
                        delGeoId == constraints[i]->Third)
                        Gui::cmdAppObjectArgs(sketchgui->getObject(), "delConstraint(%d)", i);
                }

                // Remove pole
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", delGeoId);

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                poleGeoIds.pop_back();
                BSplinePoles.pop_back();

                // last entry is kept, as it corresponds to the current pole, but the one corresponding to the erased pole is removed
                sugConstr.erase(std::prev(std::prev(sugConstr.end())));


                // run this in the end to draw lines and position text
                drawControlPolygonToPosition(prevCursorPosition);
                drawCursorToPosition(prevCursorPosition);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                // some commands might have already deleted some constraints/geometries but not others
                Gui::Command::abortCommand();

                static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->solve();

                return;
            }
        }
        // TODO: On pressing, say, W, modify last pole's weight
        // TODO: On pressing, say, M, modify next knot's multiplicity

        return;
    }

    virtual void quit(void) override
    {
        // We must see if we need to create a B-spline before cancelling everything
        // and now just like any other Handler,

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");

        bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

        if (poleGeoIds.size() > 1) {
            // create B-spline from existing poles
            Mode=STATUS_CLOSE;
            finishCommand(Base::Vector2d(0.f,0.f));
        }
        else if(poleGeoIds.size() == 1) {
            // if we just have one point and we can not close anything, then cancel this creation but continue according to continuous mode
            //sketchgui->getDocument()->undo(1);

            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            if(!continuousMode){
                DrawSketchHandler::quit();
            }
            else {
                // This code disregards existing data and enables the continuous creation mode.
                resetHandlerState();
            }
        }
        else { // we have no data (CurrentConstraint == 0) so user when right-clicking really wants to exit
            DrawSketchHandler::quit();
        }
    }

private:
    void resetHandlerState()
    {
        Mode = STATUS_SEEK_FIRST_CONTROLPOINT;
        applyCursor();

        SplineDegree = 3;

        sugConstr.clear();
        poleGeoIds.clear();
        BSplinePoles.clear();

        eraseEditCurve();

        addSugConstraint();

        IsClosed = false;
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_BSpline");
    }

    void addSugConstraint() {
        std::vector<AutoConstraint> sugConstr1;
        sugConstr.push_back(std::move(sugConstr1));
    }

    void drawControlPolygonToPosition(Base::Vector2d position) {

        std::vector<Base::Vector2d> editcurve(BSplinePoles);
        editcurve.push_back(position);

        drawEdit(editcurve);
    }

    void drawCursorToPosition(Base::Vector2d position) {
        float length = (position - BSplinePoles.back()).Length();
        float angle = (position - BSplinePoles.back()).GetAngle(Base::Vector2d(1.f,0.f));

        SbString text;
        text.sprintf(" (%.1f,%.1fdeg)", length, (angle != -FLOAT_MAX) ? angle * 180 / M_PI : 0);
        setPositionText(position, text);
    }

    void eraseEditCurve() {
        drawEdit(std::vector<Base::Vector2d>());
    }

    bool finishCommand(Base::Vector2d position) {
        if (Mode==STATUS_CLOSE) {
            unsetCursor();
            resetPositionText();

            std::stringstream stream;

            for (auto & pole : BSplinePoles) {
                stream << "App.Vector(" << pole.x << "," << pole.y << "),";
            }

            std::string controlpoints = stream.str();

            // remove last comma and add brackets
            int index = controlpoints.rfind(',');
            controlpoints.resize(index);

            controlpoints.insert(0,1,'[');
            controlpoints.append(1,']');

            int currentgeoid = getHighestCurveIndex();

            try {
                //Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add B-spline curve"));

                /*Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.BSplineCurve"
                    "(%s,%s),"
                    "%s)",
                        controlpoints.c_str(),
                        ConstrMethod == 0 ?"False":"True",
                        geometryCreationMode==Construction?"True":"False"); */

                // {"poles", "mults", "knots", "periodic", "degree", "weights", "CheckRational", NULL};
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.BSplineCurve"
                                        "(%s,None,None,%s,%d,None,False),%s)",
                                        controlpoints.c_str(),
                                        ConstrMethod == 0 ?"False":"True",
                                        SplineDegree,
                                        geometryCreationMode==Construction?"True":"False");

                currentgeoid++;

                // autoconstraints were added to the circles of the poles, which is ok because they must go to the
                // right position, or the user will freak-out if they appear out of the autoconstrained position.
                // However, autoconstraints on the first and last pole, in normal non-periodic b-splines (with appropriate endpoint knot multiplicity)
                // as the ones created by this tool are intended for the b-spline endpoints, and not for the poles,
                // so here we retrieve any autoconstraint on those poles' center and mangle it to the endpoint.
                if (ConstrMethod == 0) {
                    for(auto & constr : static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->Constraints.getValues()) {
                        if(constr->First == poleGeoIds[0] && constr->FirstPos == Sketcher::PointPos::mid) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::start;
                        }
                        else if(constr->First == poleGeoIds.back() && constr->FirstPos == Sketcher::PointPos::mid) {
                            constr->First = currentgeoid;
                            constr->FirstPos = Sketcher::PointPos::end;
                        }
                    }
                }

                // Constraint pole circles to B-spline.
                std::stringstream cstream;

                cstream << "conList = []\n";

                for (size_t i = 0; i < poleGeoIds.size(); i++) {
                    cstream << "conList.append(Sketcher.Constraint('InternalAlignment:Sketcher::BSplineControlPoint'," << poleGeoIds[0] + i
                        << "," << static_cast<int>(Sketcher::PointPos::mid) << "," << currentgeoid << "," << i << "))\n";
                }

                cstream << Gui::Command::getObjectCmd(sketchgui->getObject()) << ".addConstraint(conList)\n";
                cstream << "del conList\n";

                Gui::Command::doCommand(Gui::Command::Doc, cstream.str().c_str());

                // for showing the knots on creation
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "exposeInternalGeometry(%d)", currentgeoid);
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();

                tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                return false;
            }

            Gui::Command::commitCommand();

            tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

            if(continuousMode){
                // This code enables the continuous creation mode.
                resetHandlerState();

                drawCursorToPosition(position);

                /* It is ok not to call to purgeHandler
                 * in continuous creation mode because the
                 * handler is destroyed by the quit() method on pressing the
                 * right button of the mouse */
            }
            else{
                sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
            }
        }
        else {
            drawCursorToPosition(position);
        }

        return true;
    }

protected:
    SELECT_MODE Mode;
    MOUSE_PRESS_MODE MousePressMode;

    // Stores position of the poles of the BSpline.
    std::vector<Base::Vector2d> BSplinePoles;

    // suggested autoconstraints for poles.
    // A new one must be added e.g. using addSugConstraint() before adding a new pole.
    std::vector<std::vector<AutoConstraint>> sugConstr;

    int ConstrMethod;
    int SplineDegree;
    bool IsClosed;
    std::vector<int> poleGeoIds;
    Base::Vector2d prevCursorPosition;
};

DEF_STD_CMD_A(CmdSketcherCreateBSpline)

CmdSketcherCreateBSpline::CmdSketcherCreateBSpline()
  : Command("Sketcher_CreateBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create B-spline");
    sToolTipText    = QT_TR_NOOP("Create a B-spline via control points in the sketch.");
    sWhatsThis      = "Sketcher_CreateBSpline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateBSpline";
    sAccel          = "G, B, B";
    eType           = ForEdit;
}

void CmdSketcherCreateBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(0) );
}

/*void CmdSketcherCreateBSpline::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr"));
        break;
    }
}*/

bool CmdSketcherCreateBSpline::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/// @brief Macro that declares a new sketcher command class 'CmdSketcherCreateBSpline'
DEF_STD_CMD_A(CmdSketcherCreatePeriodicBSpline)

/**
 * @brief ctor
 */
CmdSketcherCreatePeriodicBSpline::CmdSketcherCreatePeriodicBSpline()
: Command("Sketcher_CreatePeriodicBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create periodic B-spline");
    sToolTipText    = QT_TR_NOOP("Create a periodic B-spline via control points in the sketch.");
    sWhatsThis      = "Sketcher_CreatePeriodicBSpline";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Create_Periodic_BSpline";
    sAccel          = "G, B, P";
    eType           = ForEdit;
}

void CmdSketcherCreatePeriodicBSpline::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(),new DrawSketchHandlerBSpline(1) );
}

bool CmdSketcherCreatePeriodicBSpline::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


/// @brief Macro that declares a new sketcher command class 'CmdSketcherCompCreateBSpline'
DEF_STD_CMD_ACLU(CmdSketcherCompCreateBSpline)

/**
 * @brief ctor
 */
CmdSketcherCompCreateBSpline::CmdSketcherCompCreateBSpline()
: Command("Sketcher_CompCreateBSpline")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create a B-spline");
    sToolTipText    = QT_TR_NOOP("Create a B-spline in the sketch");
    sWhatsThis      = "Sketcher_CompCreateBSpline";
    sStatusTip      = sToolTipText;
    eType           = ForEdit;
}

/**
 * @brief Instantiates the B-spline handler when the B-spline command activated
 * @param int iMsg
 */
void CmdSketcherCompCreateBSpline::activated(int iMsg)
{
    if (iMsg == 0) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSpline(iMsg));
    } else if (iMsg == 1) {
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerBSpline(iMsg));
    } else {
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompCreateBSpline::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* bspline = pcAction->addAction(QString());
    bspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));

    QAction* periodicbspline = pcAction->addAction(QString());
    periodicbspline->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));

    _pcAction = pcAction;
    languageChange();

    // default
    pcAction->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateBSpline::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
        case Normal:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline"));
            getAction()->setIcon(a[index]->icon());
            break;
        case Construction:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateBSpline_Constr"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Create_Periodic_BSpline_Constr"));
            getAction()->setIcon(a[index]->icon());
            break;
    }
}

void CmdSketcherCompCreateBSpline::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* bspline = a[0];
    bspline->setText(QApplication::translate("Sketcher_CreateBSpline","B-spline by control points"));
    bspline->setToolTip(QApplication::translate("Sketcher_CreateBSpline","Create a B-spline by control points"));
    bspline->setStatusTip(QApplication::translate("Sketcher_CreateBSpline","Create a B-spline by control points"));
    QAction* periodicbspline = a[1];
    periodicbspline->setText(QApplication::translate("Sketcher_Create_Periodic_BSpline","Periodic B-spline by control points"));
    periodicbspline->setToolTip(QApplication::translate("Sketcher_Create_Periodic_BSpline","Create a periodic B-spline by control points"));
    periodicbspline->setStatusTip(QApplication::translate("Sketcher_Create_Periodic_BSpline","Create a periodic B-spline by control points"));
}

bool CmdSketcherCompCreateBSpline::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Create Point =======================================================*/

// DrawSketchHandlerPoint: An example of deriving from DrawSketchDefaultWidgetHandler with NVI for handler and specialisation for widgetmanager.
class DrawSketchHandlerPoint;

using DrawSketchHandlerPointBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerPoint,
    StateMachines::OneSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 1,
    /*WidgetParametersT =*/WidgetParameters<2>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerPoint : public DrawSketchHandlerPointBase
{
    friend DrawSketchHandlerPointBase;

public:

    DrawSketchHandlerPoint() = default;
    virtual ~DrawSketchHandlerPoint() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);

            editPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch point"));
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                editPoint.x, editPoint.y);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
                Base::Console().Error("Failed to add point: %s\n", e.what());
                Gui::Command::abortCommand();
            }
    }

    virtual void createAutoConstraints() override {

        if (!sugConstraints[0].empty()) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex(), Sketcher::PointPos::start);
            sugConstraints[0].clear();
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_Point";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_Point");
    }

private:
    Base::Vector2d editPoint;
};

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_point", "x of point"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_point", "y of point"));
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->editPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->editPoint.y = value;
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->updateDataAndDrawToPosition(prevCursorPosition); // draw curve to cursor with suggested constraints

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerPointBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex();

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
            if (x0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::VAxis,
                    x0, handler->sketchgui->getObject());

            if (y0set)
                ConstraintToAttachment(GeoElementId(firstCurve, PointPos::start), GeoElementId::HAxis,
                    y0, handler->sketchgui->getObject());
    }
}

DEF_STD_CMD_A(CmdSketcherCreatePoint)

CmdSketcherCreatePoint::CmdSketcherCreatePoint()
  : Command("Sketcher_CreatePoint")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Create point");
    sToolTipText    = QT_TR_NOOP("Create a point in the sketch");
    sWhatsThis      = "Sketcher_CreatePoint";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreatePoint";
    sAccel          = "G, Y";
    eType           = ForEdit;
}

void CmdSketcherCreatePoint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerPoint());
}

bool CmdSketcherCreatePoint::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// Fillet and Chamfer ===================================================================

namespace SketcherGui {
    class FilletSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        FilletSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr), object(obj)
        {}

        bool allow(App::Document * /*pDoc*/, App::DocumentObject *pObj, const char *sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if (element.substr(0,4) == "Edge") {
                int GeoId = std::atoi(element.substr(4,4000).c_str()) - 1;
                Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(object);
                const Part::Geometry *geom = Sketch->getGeometry(GeoId);
                if (geom->getTypeId().isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId()))
                    return true;
            }
            if (element.substr(0,6) == "Vertex") {
                int VtId = std::atoi(element.substr(6,4000).c_str()) - 1;
                Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(object);
                std::vector<int> GeoIdList;
                std::vector<Sketcher::PointPos> PosIdList;
                Sketch->getDirectlyCoincidentPoints(VtId, GeoIdList, PosIdList);
                if (GeoIdList.size() == 2 && GeoIdList[0] >= 0  && GeoIdList[1] >= 0) {
                    const Part::Geometry *geom1 = Sketch->getGeometry(GeoIdList[0]);
                    const Part::Geometry *geom2 = Sketch->getGeometry(GeoIdList[1]);
                    if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                        geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId())
                        return true;
                }
            }
            return  false;
        }
    };
}


class DrawSketchHandlerFillet;

namespace SketcherGui::ConstructionMethods {

enum class FilletChamferConstructionMethod {
        Fillet,
        Chamfer,
    End // Must be the last one
};

}

using DrawSketchHandlerFilletBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerFillet,
    StateMachines::TwoSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 0,
    /*WidgetParametersT =*/WidgetParameters<1, 2>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2, 2>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::FilletChamferConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerFillet : public DrawSketchHandlerFilletBase
{
    friend DrawSketchHandlerFilletBase;

public:

    DrawSketchHandlerFillet(ConstructionMethod constrMethod = ConstructionMethod::Fillet) :
        DrawSketchHandlerFilletBase(constrMethod),
        radius(-1),
        firstCurve(0),
        nofAngles(1),
        preservePoint(false) {}

    virtual ~DrawSketchHandlerFillet() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        Q_UNUSED(onSketchPos);
    }

    virtual void executeCommands() override {
        //all happen in onButtonPressed
    }

    virtual void createAutoConstraints() override {
        //none
    }

    virtual std::string getToolName() const override {
        return "DSH_Fillet";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Create_Fillet");
    }

    //Implement here ?
    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {

        bool construction = false;
        //Case 1 : User selected a point. In this case the fillet will be made at this point (if there are two lines intersecting)
        int VtId = getPreselectPoint();
        if (state() == SelectMode::SeekFirst && VtId != -1) {
            int GeoId;
            Sketcher::PointPos PosId = Sketcher::PointPos::none;
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, GeoId, PosId);
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                (PosId == Sketcher::PointPos::start || PosId == Sketcher::PointPos::end)) {

                std::vector<int> GeoIdList;
                std::vector<Sketcher::PointPos> PosIdList;
                sketchgui->getSketchObject()->getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
                if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
                    const Part::Geometry* geom1 = sketchgui->getSketchObject()->getGeometry(GeoIdList[0]);
                    const Part::Geometry* geom2 = sketchgui->getSketchObject()->getGeometry(GeoIdList[1]);
                    construction = Sketcher::GeometryFacade::getConstruction(geom1) && Sketcher::GeometryFacade::getConstruction(geom2);
                    if (radius < 0) { //if radius not -1 then it has been set by widget
                        // guess fillet radius
                        if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                            geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                            const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>(geom1);
                            const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>(geom2);
                            Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
                            Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();
                            if (PosIdList[0] == Sketcher::PointPos::end)
                                dir1 *= -1;
                            if (PosIdList[1] == Sketcher::PointPos::end)
                                dir2 *= -1;
                            double l1 = dir1.Length();
                            double l2 = dir2.Length();
                            double angle = dir1.GetAngle(dir2);
                            radius = (l1 < l2 ? l1 : l2) * 0.2 * sin(angle / 2);
                        }
                        else
                            radius = 0;
                    }
                }

                if (radius < 0)
                    return;

                firstCurveCreated = getHighestCurveIndex() + 1;
                // create fillet at point
                try {
                    //nofAngles add support for chamfer and poly-chamfer and inward-poly-chamfer and inward-fillet. 1 is normal fillet
                    //-1 is inward fillet, 2 and -2 are chamfer, 3 is a two edge chamfer, -3 is two edge inward chamfer and so on.

                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "fillet(%d,%d,%f,%s,%s,%d)", GeoId, static_cast<int>(PosId), radius, "True",
                        preservePoint ? "True" : "False", nofAngles);

                    if (construction) {
                        Gui::cmdAppObjectArgs(sketchgui->getObject(), "toggleConstruction(%d) ", firstCurveCreated);
                    }


                    Gui::Command::commitCommand();
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("Failed to create fillet: %s\n", e.what());
                    Gui::Command::abortCommand();
                }

                this->setState(SelectMode::End);
                this->finish();
            }
            return;
        }

        //Case 2 : User selected a curve. Then the fillet will be made between this curve and next selected curve
        int GeoId = getPreselectCurve();
        if (GeoId > -1) {
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId().isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId())) {
                if (state() == SelectMode::SeekFirst) {
                    firstCurve = GeoId;
                    firstPos = onSketchPos;
                    this->moveToNextMode();
                    // add the line to the selection
                    std::stringstream ss;
                    ss << "Edge" << firstCurve + 1;
                    Gui::Selection().addSelection(sketchgui->getSketchObject()->getDocument()->getName()
                        , sketchgui->getSketchObject()->getNameInDocument()
                        , ss.str().c_str()
                        , onSketchPos.x
                        , onSketchPos.y
                        , 0.f);
                }
                else if (state() == SelectMode::SeekSecond) {
                    int secondCurve = GeoId;
                    Base::Vector2d secondPos = onSketchPos;

                    Base::Vector3d refPnt1(firstPos.x, firstPos.y, 0.f);
                    Base::Vector3d refPnt2(secondPos.x, secondPos.y, 0.f);

                    const Part::Geometry* geom1 = sketchgui->getSketchObject()->getGeometry(firstCurve);

                    if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                        geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        // guess fillet radius
                        const Part::GeomLineSegment* lineSeg1 = static_cast<const Part::GeomLineSegment*>
                            (sketchgui->getSketchObject()->getGeometry(firstCurve));
                        const Part::GeomLineSegment* lineSeg2 = static_cast<const Part::GeomLineSegment*>
                            (sketchgui->getSketchObject()->getGeometry(secondCurve));

                        if (radius < 0) {
                            radius = Part::suggestFilletRadius(lineSeg1, lineSeg2, refPnt1, refPnt2);
                        }
                        if (radius < 0)
                            return;

                        construction = Sketcher::GeometryFacade::getConstruction(lineSeg1) && Sketcher::GeometryFacade::getConstruction(lineSeg2);
                    }
                    else { // other supported curves
                        if (radius < 0)
                            radius = 0;
                        const Part::Geometry* geo1 = static_cast<const Part::Geometry*>
                            (sketchgui->getSketchObject()->getGeometry(firstCurve));
                        const Part::Geometry* geo2 = static_cast<const Part::Geometry*>
                            (sketchgui->getSketchObject()->getGeometry(secondCurve));

                        construction = Sketcher::GeometryFacade::getConstruction(geo1) && Sketcher::GeometryFacade::getConstruction(geo2);
                    }


                    firstCurveCreated = getHighestCurveIndex() + 1;

                    // create fillet between lines
                    try {


                        Base::Console().Error("nofAngles: %d\n", nofAngles);
                        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                        Gui::cmdAppObjectArgs(sketchgui->getObject(), "fillet(%d,%d,App.Vector(%f,%f,0),App.Vector(%f,%f,0),%f,%s,%s,%d)",
                            firstCurve, secondCurve,
                            firstPos.x, firstPos.y,
                            secondPos.x, secondPos.y, radius,
                            "True", preservePoint ? "True" : "False", nofAngles);

                        //Set the fillet as construction if the selected lines were construction lines.
                        if (construction) {
                            Gui::cmdAppObjectArgs(sketchgui->getObject(), "toggleConstruction(%d) ", firstCurveCreated);
                        }

                        Gui::Command::commitCommand();
                    }
                    catch (const Base::CADKernelError& e) {
                        e.ReportException();
                        if (e.getTranslatable()) {
                            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("CAD Kernel Error"),
                                QObject::tr(e.getMessage().c_str()));
                        }
                        Gui::Selection().clearSelection();
                        Gui::Command::abortCommand();
                        this->setState(SelectMode::SeekFirst);
                    }
                    catch (const Base::ValueError& e) {
                        e.ReportException();
                        Gui::Selection().clearSelection();
                        Gui::Command::abortCommand();
                        this->setState(SelectMode::SeekFirst);
                    }

                    Gui::Selection().clearSelection();

                    this->setState(SelectMode::End);
                    this->finish();
                }
            }
        }

        if (VtId < 0 && GeoId < 0) // exit the fillet tool if the user clicked on empty space
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new FilletSelection(sketchgui->getObject()));
    }

    virtual void onReset() override {
        //For this tool we don't want to reinitialize the widget such that the user can make several identical fillet in a row.
        //toolWidgetManager.reset();
    }

private:
    double radius;
    int firstCurveCreated, firstCurve, nofAngles;
    bool preservePoint;
    Base::Vector2d firstPos;
};

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::configureToolWidget() {

    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Fillet"), QStringLiteral("Chamfer")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_fillet", "Radius"));
    if (dHandler->constructionMethod() == DrawSketchHandlerFillet::ConstructionMethod::Chamfer) {
        toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_fillet", "Number of corners"));
        toolWidget->setParameter(WParameter::Second, 2);
    }
    toolWidget->setCheckboxLabel(WCheckbox::FirstBox, QApplication::translate("TaskSketcherTool_c1_fillet", "Preserve corner and most constraints"));
    toolWidget->setCheckboxPrefEntry(WCheckbox::FirstBox, "PreserveFilletChamferCorner");
    toolWidget->setCheckboxLabel(WCheckbox::SecondBox, QApplication::translate("TaskSketcherTool_c2_fillet", "Inward"));
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    if (dHandler->constructionMethod() == DrawSketchHandlerFillet::ConstructionMethod::Fillet) {
        switch (parameterindex) {
        case WParameter::First:
            dHandler->radius = value;
            break;
        }
    }
    else { //if (constructionMethod == ConstructionMethod::Chamfer)
        switch (parameterindex) {
        case WParameter::First:
            dHandler->radius = value;
            break;
        case WParameter::Second:
            dHandler->nofAngles = max(2, abs(static_cast<int>(value)));
            break;
        }
    }
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::adaptDrawingToCheckboxChange(int checkboxindex, bool value) {

    switch (checkboxindex) {
    case WCheckbox::FirstBox:
        dHandler->preservePoint = value;
        break;
    case WCheckbox::SecondBox:
        if (value)
            dHandler->nofAngles = - abs(dHandler->nofAngles);
        else
            dHandler->nofAngles = abs(dHandler->nofAngles);
        break;
    }
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    Q_UNUSED(onSketchPos)
    //Do nothing
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    //do nothing
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::addConstraints() {

    auto radiusSet = toolWidget->isParameterSet(WParameter::First);

    if (radiusSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            dHandler->firstCurveCreated, dHandler->radius);
}

template <> void DrawSketchHandlerFilletBase::ToolWidgetManager::onHandlerModeChanged() {
    toolWidget->setParameterFocus(WParameter::First);
}

DEF_STD_CMD_AU(CmdSketcherCreateFillet)

CmdSketcherCreateFillet::CmdSketcherCreateFillet()
  : Command("Sketcher_CreateFillet")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Fillet - Chamfer");
    sToolTipText    = QT_TR_NOOP("Create a fillet or chamfer between two lines or at a coincident point");
    sWhatsThis      = "Sketcher_CreateFillet";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CreateFillet";
    sAccel          = "G, F";
    eType           = ForEdit;
}

void CmdSketcherCreateFillet::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerFillet());
}

void CmdSketcherCreateFillet::updateAction(int mode)
{
    switch (mode) {
    case Normal:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));
        break;
    case Construction:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet_Constr"));
        break;
    }
}

bool CmdSketcherCreateFillet::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// Trim edge =========================================================================

namespace SketcherGui {
    class TrimmingSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        TrimmingSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr), object(obj)
        {}

        bool allow(App::Document * /*pDoc*/, App::DocumentObject *pObj, const char *sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if (element.substr(0,4) == "Edge") {
                int GeoId = std::atoi(element.substr(4,4000).c_str()) - 1;
                Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(object);
                const Part::Geometry *geom = Sketch->getGeometry(GeoId);
                if (geom->getTypeId().isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId())   ||
                    geom->getTypeId() == Part::GeomCircle::getClassTypeId()                     ||
                    geom->getTypeId() == Part::GeomEllipse::getClassTypeId()                    ||
                    geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()
                ) {
                    // We do not trim internal geometry of complex geometries
                    if( Sketcher::GeometryFacade::isInternalType(geom, Sketcher::InternalType::None))
                        return true;
                }
            }
            return  false;
        }
    };
}

class DrawSketchHandlerTrimming: public DrawSketchHandler
{
public:
    DrawSketchHandlerTrimming() = default;
    virtual ~DrawSketchHandlerTrimming()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);

        int GeoId = getPreselectCurve();

        if (GeoId > -1) {
            auto sk = static_cast<Sketcher::SketchObject *>(sketchgui->getObject());
            int GeoId1, GeoId2;
            Base::Vector3d intersect1, intersect2;
            if(sk->seekTrimPoints(GeoId, Base::Vector3d(onSketchPos.x,onSketchPos.y,0),
                                  GeoId1, intersect1,
                                  GeoId2, intersect2)) {

                EditMarkers.resize(0);

                if(GeoId1 != Sketcher::GeoEnum::GeoUndef)
                    EditMarkers.emplace_back(intersect1.x, intersect1.y);
                else {
                    auto start = sk->getPoint(GeoId, Sketcher::PointPos::start);
                    EditMarkers.emplace_back(start.x, start.y);
                }

                if(GeoId2 != Sketcher::GeoEnum::GeoUndef)
                    EditMarkers.emplace_back(intersect2.x, intersect2.y);
                else {
                    auto end = sk->getPoint(GeoId, Sketcher::PointPos::end);
                    EditMarkers.emplace_back( end.x, end.y);
                }

                drawEditMarkers(EditMarkers, 2); // maker augmented by two sizes (see supported marker sizes)
            }
        }
        else {
            EditMarkers.resize(0);
            drawEditMarkers(EditMarkers, 2);
        }
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        int GeoId = getPreselectCurve();
        if (GeoId > -1) {
            const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId().isDerivedFrom(Part::GeomTrimmedCurve::getClassTypeId())   ||
                geom->getTypeId() == Part::GeomCircle::getClassTypeId()                     ||
                geom->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ) {
                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Trim edge"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "trim(%d,App.Vector(%f,%f,0))",
                              GeoId, onSketchPos.x, onSketchPos.y);
                    Gui::Command::commitCommand();
                    tryAutoRecompute(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("Failed to trim edge: %s\n", e.what());
                    Gui::Command::abortCommand();
                }
            }

            EditMarkers.resize(0);
            drawEditMarkers(EditMarkers);
        }
        else // exit the trimming tool if the user clicked on empty space
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider

        return true;
    }

private:
    virtual void activated() override
    {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new TrimmingSelection(sketchgui->getObject()));
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Trimming");
    }

private:
    std::vector<Base::Vector2d> EditMarkers;
};

DEF_STD_CMD_A(CmdSketcherTrimming)

CmdSketcherTrimming::CmdSketcherTrimming()
  : Command("Sketcher_Trimming")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Trim edge");
    sToolTipText    = QT_TR_NOOP("Trim an edge with respect to the picked position");
    sWhatsThis      = "Sketcher_Trimming";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Trimming";
    sAccel          = "G, T";
    eType           = ForEdit;
}

void CmdSketcherTrimming::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerTrimming());
}

bool CmdSketcherTrimming::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


// Extend edge ========================================================================

namespace SketcherGui {
    class ExtendSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        ExtendSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr)
            , object(obj)
            , disabled(false)
        {}

        bool allow(App::Document * /*pDoc*/, App::DocumentObject *pObj, const char *sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            if (disabled)
                return true;
            std::string element(sSubName);
            if (element.substr(0, 4) == "Edge") {
                int GeoId = std::atoi(element.substr(4, 4000).c_str()) - 1;
                Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(object);
                const Part::Geometry *geom = Sketch->getGeometry(GeoId);
                if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
                    geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
                    return true;
            }
            return false;
        }

        void setDisabled(bool isDisabled) {
            disabled = isDisabled;
        }
    protected:
        bool disabled;
    };
}

class DrawSketchHandlerExtend: public DrawSketchHandler
{
public:
    DrawSketchHandlerExtend()
        : Mode(STATUS_SEEK_First)
        , EditCurve(2)
        , BaseGeoId(-1)
        , ExtendFromStart(false)
        , SavedExtendFromStart(false)
        , Increment(0) {}

    virtual ~DrawSketchHandlerExtend()
    {
        Gui::Selection().rmvSelectionGate();
    }
    enum SelectMode {
        STATUS_SEEK_First,
        STATUS_SEEK_Second,
    };

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_SEEK_Second) {
            const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(BaseGeoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg = static_cast<const Part::GeomLineSegment *>(geom);
                // project point to the existing curve
                Base::Vector3d start3d = lineSeg->getStartPoint();
                Base::Vector3d end3d = lineSeg->getEndPoint();

                Base::Vector2d startPoint = Base::Vector2d(start3d.x, start3d.y);
                Base::Vector2d endPoint = Base::Vector2d(end3d.x, end3d.y);
                Base::Vector2d recenteredLine = endPoint - startPoint;
                Base::Vector2d recenteredPoint = onSketchPos - startPoint;
                Base::Vector2d projection;
                projection.ProjectToLine(recenteredPoint, recenteredLine);
                if (recenteredPoint.Length() < recenteredPoint.Distance(recenteredLine)) {
                    EditCurve[0] = startPoint + projection;
                    EditCurve[1] = endPoint;
                } else {
                    EditCurve[0] = startPoint;
                    EditCurve[1] = startPoint + projection;
                }
                /**
                 * If in-curve, the intuitive behavior is for the line to shrink an amount from
                 * the original click-point.
                 *
                 * If out-of-curve, the intuitive behavior is for the closest line endpoint to
                 * expand.
                 */
                bool inCurve = (projection.Length() < recenteredLine.Length()
                    && projection.GetAngle(recenteredLine) < 0.1); // Two possible values here, M_PI and 0, but 0.1 is to avoid floating point problems.
                if (inCurve) {
                    Increment = SavedExtendFromStart ? -1 * projection.Length() : projection.Length() - recenteredLine.Length();
                    ExtendFromStart = SavedExtendFromStart;
                } else {
                    ExtendFromStart = onSketchPos.Distance(startPoint) < onSketchPos.Distance(endPoint);
                    Increment = ExtendFromStart ? projection.Length() : projection.Length() - recenteredLine.Length();
                }
                drawEdit(EditCurve);

            } else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
                Base::Vector3d center = arc->getCenter();
                double radius = arc->getRadius();

                double start, end;
                arc->getRange(start, end, true);
                double arcAngle = end - start;

                Base::Vector2d angle = Base::Vector2d(onSketchPos.x - center.x, onSketchPos.y - center.y);
                Base::Vector2d startAngle = Base::Vector2d(cos(start), sin(start));
                Base::Vector2d endAngle = Base::Vector2d(cos(end), sin(end));

                Base::Vector2d arcHalf = Base::Vector2d(cos(start + arcAngle/ 2.0), sin(start+ arcAngle / 2.0));
                double angleToEndAngle = angle.GetAngle(endAngle);
                double angleToStartAngle = angle.GetAngle(startAngle);


                double modStartAngle = start;
                double modArcAngle = end - start;
                bool outOfArc = arcHalf.GetAngle(angle) * 2.0 > arcAngle;
                if (ExtendFromStart) {
                    bool isCCWFromStart = crossProduct(angle, startAngle) < 0;
                    if (outOfArc) {
                        if (isCCWFromStart) {
                            modStartAngle -= 2*M_PI - angleToStartAngle;
                            modArcAngle += 2*M_PI - angleToStartAngle;
                        } else {
                            modStartAngle -= angleToStartAngle;
                            modArcAngle += angleToStartAngle;
                        }
                    } else {
                        if (isCCWFromStart) {
                            modStartAngle += angleToStartAngle;
                            modArcAngle -= angleToStartAngle;
                        } else {
                            modStartAngle += 2*M_PI - angleToStartAngle;
                            modArcAngle -= 2*M_PI - angleToStartAngle;
                        }
                    }
                } else {
                    bool isCWFromEnd = crossProduct(angle, endAngle) >= 0;
                    if (outOfArc) {
                        if (isCWFromEnd) {
                            modArcAngle += 2*M_PI - angleToEndAngle;
                        } else {
                            modArcAngle += angleToEndAngle;
                        }
                    } else {
                        if (isCWFromEnd) {
                            modArcAngle -= angleToEndAngle;
                        } else {
                            modArcAngle -= 2*M_PI - angleToEndAngle;
                        }
                    }
                }
                Increment = modArcAngle - (end - start);
                for (int i = 0; i < 31; i++) {
                    double angle = modStartAngle + i * modArcAngle/30.0;
                    EditCurve[i] = Base::Vector2d(center.x + radius * cos(angle), center.y + radius * sin(angle));
                }
                drawEdit(EditCurve);
            }
            int curveId = getPreselectCurve();
            if (BaseGeoId != curveId && seekAutoConstraint(SugConstr, onSketchPos, Base::Vector2d(0.f,0.f))) {
                renderSuggestConstraintsCursor(SugConstr);
                return;
            }
        }
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Mode == STATUS_SEEK_First) {
            BaseGeoId = getPreselectCurve();
            if (BaseGeoId > -1) {
                const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(BaseGeoId);
                if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    const Part::GeomLineSegment *seg = static_cast<const Part::GeomLineSegment *>(geom);
                    Base::Vector3d start3d = seg->getStartPoint();
                    Base::Vector3d end3d = seg->getEndPoint();
                    Base::Vector2d start = Base::Vector2d(start3d.x, start3d.y);
                    Base::Vector2d end = Base::Vector2d(end3d.x, end3d.y);
                    SavedExtendFromStart = (onSketchPos.Distance(start) < onSketchPos.Distance(end));
                    ExtendFromStart = SavedExtendFromStart;
                    Mode = STATUS_SEEK_Second;
                } else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                    const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
                    double start, end;
                    arc->getRange(start, end, true);

                    Base::Vector3d center = arc->getCenter();
                    Base::Vector2d angle = Base::Vector2d(onSketchPos.x - center.x, onSketchPos.y - center.y);
                    double angleToStart = angle.GetAngle(Base::Vector2d(cos(start), sin(start)));
                    double angleToEnd = angle.GetAngle(Base::Vector2d(cos(end), sin(end)));
                    ExtendFromStart = (angleToStart < angleToEnd); // move start point if closer to angle than end point
                    EditCurve.resize(31);
                    Mode = STATUS_SEEK_Second;
                }
                filterGate->setDisabled(true);
            }
        } else if (Mode == STATUS_SEEK_Second) {
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Extend edge"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "extend(%d, %f, %d)\n", // GeoId, increment, PointPos
                    BaseGeoId, Increment, ExtendFromStart ? static_cast<int>(Sketcher::PointPos::start) : static_cast<int>(Sketcher::PointPos::end));
                Gui::Command::commitCommand();

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
                bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
                if(autoRecompute)
                    Gui::Command::updateActive();

                // constrain chosen point
                if (SugConstr.size() > 0) {
                    createAutoConstraints(SugConstr, BaseGeoId, (ExtendFromStart) ? Sketcher::PointPos::start : Sketcher::PointPos::end);
                    SugConstr.clear();
                }
                bool continuousMode = hGrp->GetBool("ContinuousCreationMode",true);

                if(continuousMode){
                    // This code enables the continuous creation mode.
                    Mode=STATUS_SEEK_First;
                    filterGate->setDisabled(false);
                    EditCurve.clear();
                    drawEdit(EditCurve);
                    EditCurve.resize(2);
                    applyCursor();
                    /* this is ok not to call to purgeHandler
                    * in continuous creation mode because the
                    * handler is destroyed by the quit() method on pressing the
                    * right button of the mouse */
                } else{
                    sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
                }
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("Failed to extend edge: %s\n", e.what());
                Gui::Command::abortCommand();
            }

        } else { // exit extension tool if user clicked on empty space
            BaseGeoId = -1;
            sketchgui->purgeHandler(); // no code after this line, Handler get deleted in ViewProvider
        }
        return true;
    }

private:
    virtual void activated() override
    {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        filterGate = new ExtendSelection(sketchgui->getObject());
        Gui::Selection().addSelectionGate(filterGate);
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Extension");
    }

protected:
    SelectMode Mode;
    std::vector<Base::Vector2d> EditCurve;
    int BaseGeoId;
    ExtendSelection* filterGate = nullptr;
    bool ExtendFromStart; // if true, extend from start, else extend from end (circle only)
    bool SavedExtendFromStart;
    double Increment;
    std::vector<AutoConstraint> SugConstr;

private:
    int crossProduct(Base::Vector2d &vec1, Base::Vector2d &vec2) {
        return vec1.x * vec2.y - vec1.y * vec2.x;
    }
};

DEF_STD_CMD_A(CmdSketcherExtend)

//TODO: fix the translations for this
CmdSketcherExtend::CmdSketcherExtend()
  : Command("Sketcher_Extend")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Extend edge");
    sToolTipText    = QT_TR_NOOP("Extend an edge with respect to the picked position");
    sWhatsThis      = "Sketcher_Extend";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Extend";
    sAccel          = "G, Q";
    eType           = ForEdit;
}

void CmdSketcherExtend::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExtend());
}

bool CmdSketcherExtend::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}


// Split edge ==========================================================================

namespace SketcherGui {
    class SplittingSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        SplittingSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr), object(obj)
        {}

        bool allow(App::Document * /*pDoc*/, App::DocumentObject *pObj, const char *sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if (element.substr(0,4) == "Edge") {
                int GeoId = std::atoi(element.substr(4,4000).c_str()) - 1;
                Sketcher::SketchObject *Sketch = static_cast<Sketcher::SketchObject*>(object);
                const Part::Geometry *geom = Sketch->getGeometry(GeoId);
                if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                    || geom->getTypeId() == Part::GeomCircle::getClassTypeId()
                    || geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                    return true;
                }
            }
            return  false;
        }
    };
}

class DrawSketchHandlerSplitting: public DrawSketchHandler
{
public:
    DrawSketchHandlerSplitting() = default;
    virtual ~DrawSketchHandlerSplitting()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        int GeoId = getPreselectCurve();
        if (GeoId >= 0) {
            const Part::Geometry *geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                || geom->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Split edge"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "split(%d,App.Vector(%f,%f,0))",
                              GeoId, onSketchPos.x, onSketchPos.y);
                    Gui::Command::commitCommand();
                    tryAutoRecompute(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("Failed to split edge: %s\n", e.what());
                    Gui::Command::abortCommand();
                }
            }
        }
        else {
            sketchgui->purgeHandler();
        }

        return true;
    }

private:
    virtual void activated() override
    {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new SplittingSelection(sketchgui->getObject()));
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Splitting");
    }
};

DEF_STD_CMD_A(CmdSketcherSplit)

//TODO: fix the translations for this
CmdSketcherSplit::CmdSketcherSplit()
  : Command("Sketcher_Split")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Split edge");
    sToolTipText    = QT_TR_NOOP("Splits an edge into two while preserving constraints");
    sWhatsThis      = "Sketcher_Split";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_Split";
    sAccel          = "G, Z";
    eType           = ForEdit;
}

void CmdSketcherSplit::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSplitting());
}

bool CmdSketcherSplit::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Create Insert =====================================================*/
class DrawSketchHandlerInsert;

namespace SketcherGui::ConstructionMethods {

enum class InsertConstructionMethod {
    Box,
    Arc,
    End // Must be the last one
};

}

using DrawSketchHandlerInsertBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerInsert,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 2,
    /*WidgetParametersT =*/WidgetParameters<3, 3>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::InsertConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerInsert : public DrawSketchHandlerInsertBase
{
    friend DrawSketchHandlerInsertBase;
public:
    DrawSketchHandlerInsert(int geoI, ConstructionMethod constrMethod = ConstructionMethod::Box) :
        DrawSketchHandlerInsertBase(constrMethod),
        reverseArc(false),
        geoId(geoI) {}

    virtual ~DrawSketchHandlerInsert() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
        }
        break;
        case SelectMode::SeekSecond:
        {
            Base::Vector2d projectedPoint;
            projectedPoint.ProjectToLine(onSketchPos - startPoint, dirVec);
            projectedPoint = startPoint + projectedPoint;
            p1 = projectedPoint;
            startLength = (p1 - startPoint).Length();
            if (startLength > lineLength * 0.75) {
                boxLength = (lineLength - startLength) * 0.8;
            }
            else {
                boxLength = lineLength / 5;
            }

            SbString text;
            text.sprintf(" (%.1fL)", startLength);
            setPositionText(onSketchPos, text);

            if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                p2 = onSketchPos;

                p3 = p2 + boxLength * dirVec;
                p4.ProjectToLine(p3 - startPoint, dirVec);
                p4 = startPoint + p4;

                drawEdit(createBoxGeometries());

                if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[0]);
                    return;
                }
            }
            else {
                p2 = p1 + boxLength * dirVec;
                radius = boxLength / 2;
                centerPoint = p1 + radius * dirVec;

                startAngle = GetPointAngle(centerPoint, p1);
                endAngle = GetPointAngle(centerPoint, p2);

                //check if we need to reverse arc
                Base::Vector2d midArcPoint;
                midArcPoint.x = centerPoint.x + cos((startAngle + endAngle) / 2) * radius;
                midArcPoint.y = centerPoint.y + sin((startAngle + endAngle) / 2) * radius;
                int signOfMidPoint = getPointSideOfVector(midArcPoint, dirVec, startPoint);
                int signOfCurPos = getPointSideOfVector(onSketchPos, dirVec, startPoint);
                if ((signOfMidPoint != signOfCurPos && signOfMidPoint == 1) || (signOfMidPoint == signOfCurPos && signOfMidPoint == -1))
                    reverseArc = true;
                else
                    reverseArc = false;

                if (reverseArc)
                    std::swap(startAngle, endAngle);

                drawEdit(createArcGeometries());
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            Base::Vector2d projectedPtn;
            projectedPtn.ProjectToLine(onSketchPos - startPoint, dirVec);
            projectedPtn = startPoint + projectedPtn;

            if ((projectedPtn - startPoint).Length() > startLength) {
                if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                    p3 = onSketchPos;
                    p4 = projectedPtn;
                    boxLength = (projectedPtn - p1).Length();
                    Base::Vector2d Perpendicular(-dirVec.y, dirVec.x);
                    p2.ProjectToLine(onSketchPos - p1, Perpendicular);
                    p2 = p1 + p2;

                    drawEdit(createBoxGeometries());
                }
                else {
                    boxLength = (projectedPtn - p1).Length() * 2;
                    centerPoint = onSketchPos;

                    p2 = p1 + boxLength * dirVec;
                    radius = (p1 - centerPoint).Length();

                    startAngle = GetPointAngle(centerPoint, p1);
                    endAngle = GetPointAngle(centerPoint, p2);

                    if (reverseArc)
                        std::swap(startAngle, endAngle);

                    drawEdit(createArcGeometries());
                }

                if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                    renderSuggestConstraintsCursor(sugConstraints[1]);
                    return;
                }
            }

            SbString text;
            text.sprintf(" (%.1fL)", boxLength);
            setPositionText(onSketchPos, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add sketch insert"));

            if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
                sketchgui->getSketchObject()->addGeometry(std::move(createBoxGeometries()));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,1))\n"
                    "conList.append(Sketcher.Constraint('Parallel',%d,%d))\n"
                    "conList.append(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,2))\n"
                    "conList.append(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,1))\n"
                    "conList.append(Sketcher.Constraint('Tangent',%d,%d))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve, firstCurve + 1, // coincident1
                    firstCurve + 1, firstCurve + 2, // coincident2
                    firstCurve + 2, firstCurve + 3, // coincident3
                    firstCurve + 3, firstCurve + 4, // coincident4
                    firstCurve + 2, firstCurve, // Parallel
                    firstCurve, firstCurve + 1, firstCurve, // Perpendicular1
                    firstCurve + 3, firstCurve + 4, firstCurve + 4, // Perpendicular2
                    firstCurve, firstCurve + 4, // tangent
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 1, firstCurve, 1);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 2, firstCurve + 4, 2);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 0, firstCurve, 0);
            }
            else {
                sketchgui->getSketchObject()->addGeometry(std::move(createArcGeometries()));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,2,%i,%i))\n"
                    "conList.append(Sketcher.Constraint('Coincident',%i,1,%i,%i))\n"
                    "conList.append(Sketcher.Constraint('Tangent',%d,%d))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve,     firstCurve + 1, reverseArc ? 2 : 1, // coincident1
                    firstCurve + 2, firstCurve + 1, reverseArc ? 1 : 2, // coincident2
                    firstCurve, firstCurve + 2, // colinear
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 1, firstCurve, 1);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 2, firstCurve + 2, 2);
                Gui::cmdAppObjectArgs(sketchgui->getObject(), "transferConstraints(%d,%d,%d,%d)",
                    geoId, 0, firstCurve, 0);
            }

            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometry(%d)", geoId);
            firstCurve--;

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add insert: %s\n", e.what());
            Gui::Command::abortCommand();
        }
    }

    virtual void createAutoConstraints() override {

        if (constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            // add auto constraints for the insert segment start
            if (!sugConstraints[0].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[0], firstCurve + 2, Sketcher::PointPos::start);
                sugConstraints[0].clear();
            }

            // add auto constraints for the insert segment end
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 2, Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }
        }
        else {
            // add auto constraints for the insert segment end
            if (!sugConstraints[1].empty()) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], firstCurve + 1, Sketcher::PointPos::mid);
                sugConstraints[1].clear();
            }
        }

    }

    virtual std::string getToolName() const override {
        return "DSH_Insert";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Insert");
    }

    virtual void activated() override
    {
        DrawSketchDefaultHandler::activated();
        setLineGeo(geoId);
    }

    virtual void onButtonPressed(Base::Vector2d onSketchPos) override {
        if (state() == SelectMode::SeekFirst) {
            geoId = getPreselectCurve();
            setLineGeo(geoId);
        }
        else {
            DrawSketchDefaultHandler::onButtonPressed(onSketchPos);
        }
    }

private:
    bool reverseArc;
    int geoId, firstCurve;
    Base::Vector2d startPoint, endPoint, p1, p2, p3, p4, dirVec, centerPoint;
    double lineLength, boxLength, startLength, radius, startAngle, endAngle;

    void setLineGeo(int geoId) {
        if (geoId >= 0) {
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(geoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                //Hide GeoId line?

                const Part::GeomLineSegment* lineGeo = static_cast<const Part::GeomLineSegment*>(geom);
                startPoint.x = lineGeo->getStartPoint().x;
                startPoint.y = lineGeo->getStartPoint().y;
                endPoint.x = lineGeo->getEndPoint().x;
                endPoint.y = lineGeo->getEndPoint().y;
                lineLength = (endPoint - startPoint).Length();
                dirVec = (endPoint - startPoint) / lineLength;

                setState(SelectMode::SeekSecond);
            }
        }
    }

    std::vector<Part::Geometry*> createBoxGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line3 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line4 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line5 = new Part::GeomLineSegment();

        line1->setPoints(Base::Vector3d(startPoint.x, startPoint.y, 0.), Base::Vector3d(p1.x, p1.y, 0.));
        line2->setPoints(Base::Vector3d(p1.x, p1.y, 0.), Base::Vector3d(p2.x, p2.y, 0.));
        line3->setPoints(Base::Vector3d(p2.x, p2.y, 0.), Base::Vector3d(p3.x, p3.y, 0.));
        line4->setPoints(Base::Vector3d(p3.x, p3.y, 0.), Base::Vector3d(p4.x, p4.y, 0.));
        line5->setPoints(Base::Vector3d(p4.x, p4.y, 0.), Base::Vector3d(endPoint.x, endPoint.y, 0.));

        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line3, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line4, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line5, geometryCreationMode);

        geometriesToAdd.push_back(line1);
        geometriesToAdd.push_back(line2);
        geometriesToAdd.push_back(line3);
        geometriesToAdd.push_back(line4);
        geometriesToAdd.push_back(line5);

        return geometriesToAdd;
    }

    std::vector<Part::Geometry*> createArcGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();

        arc1->setRadius(radius);
        arc1->setRange(startAngle, endAngle, true);
        arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

        line1->setPoints(Base::Vector3d(startPoint.x, startPoint.y, 0.), Base::Vector3d(p1.x, p1.y, 0.));
        line2->setPoints(Base::Vector3d(p2.x, p2.y, 0.), Base::Vector3d(endPoint.x, endPoint.y, 0.));

        Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);

        geometriesToAdd.push_back(line1);
        geometriesToAdd.push_back(arc1);
        geometriesToAdd.push_back(line2);

        return geometriesToAdd;
    }

    int getPointSideOfVector(Base::Vector2d pointToCheck, Base::Vector2d separatingVector, Base::Vector2d pointOnVector) {
        Base::Vector2d secondPointOnVec = pointOnVector + separatingVector;
        double d = (pointToCheck.x - pointOnVector.x) * (secondPointOnVec.y - pointOnVector.y)
            - (pointToCheck.y - pointOnVector.y) * (secondPointOnVec.x - pointOnVector.x);
        if (abs(d) < Precision::Confusion()) {
            return 0;
        }
        else if (d < 0) {
            return -1;
        }
        else {
            return 1;
        }
    }
};

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Box"), QStringLiteral("Arc")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterEnabled(WParameter::First, false);
    toolWidget->setParameterEnabled(WParameter::Second, false);
    toolWidget->setParameterEnabled(WParameter::Third, false);
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_insert", "Start distance"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_insert", "Insert length"));
    if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box)
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_insert", "Insert depth"));
    else
        toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_insert", "Distance of center to line"));
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->startLength = value;
        break;
    case WParameter::Second:
        dHandler->boxLength = value;
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {
            dHandler->startLength = toolWidget->getParameter(WParameter::First);

            Base::Vector2d projectedPtn;
            projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
            projectedPtn = dHandler->startPoint + projectedPtn;

            onSketchPos = (dHandler->startPoint + dHandler->dirVec * dHandler->startLength) + (onSketchPos - projectedPtn);
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            if (toolWidget->isParameterSet(WParameter::Second)) {
                dHandler->boxLength = toolWidget->getParameter(WParameter::Second);
                dHandler->p4 = dHandler->p1 + dHandler->dirVec * dHandler->boxLength;

                Base::Vector2d projectedPtn;
                projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                projectedPtn = dHandler->startPoint + projectedPtn;

                onSketchPos = dHandler->p4 + (onSketchPos - projectedPtn);
            }

            if (toolWidget->isParameterSet(WParameter::Third)) {
                double depth = toolWidget->getParameter(WParameter::Third);
                dHandler->p4.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                dHandler->p4 = dHandler->startPoint + dHandler->p4;

                if (!dHandler->reverseArc)
                    onSketchPos = dHandler->p4 + (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
                else
                    onSketchPos = dHandler->p4 - (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
            }
        }
        else {
            if (toolWidget->isParameterSet(WParameter::Second)) {
                dHandler->boxLength = toolWidget->getParameter(WParameter::Second);
                Base::Vector2d centerProjOnLine = dHandler->p1 + dHandler->dirVec * dHandler->boxLength / 2;

                Base::Vector2d projectedPtn;
                projectedPtn.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                projectedPtn = dHandler->startPoint + projectedPtn;

                onSketchPos = centerProjOnLine + (onSketchPos - projectedPtn);
            }

            if (toolWidget->isParameterSet(WParameter::Third)) {
                double depth = toolWidget->getParameter(WParameter::Third);
                dHandler->p4.ProjectToLine(onSketchPos - dHandler->startPoint, dHandler->dirVec);
                dHandler->p4 = dHandler->startPoint + dHandler->p4;

                onSketchPos = dHandler->p4 + (onSketchPos - dHandler->p4) / (onSketchPos - dHandler->p4).Length() * depth;
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    Q_UNUSED(onSketchPos)
    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, dHandler->startLength);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, dHandler->boxLength);

        if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
            if (!toolWidget->isParameterSet(WParameter::Third))
                toolWidget->updateVisualValue(WParameter::Third, (dHandler->p1 - dHandler->p2).Length());
        }
        else {
            if (!toolWidget->isParameterSet(WParameter::Third)) {
                Base::Vector2d centerProjOnLine = dHandler->p1 + dHandler->dirVec * dHandler->boxLength / 2;
                if(abs(dHandler->startAngle - dHandler->endAngle) > M_PI)
                    toolWidget->updateVisualValue(WParameter::Third, (dHandler->centerPoint - centerProjOnLine).Length());
                else
                    toolWidget->updateVisualValue(WParameter::Third, -(dHandler->centerPoint - centerProjOnLine).Length());
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::First)) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);


            handler->setState(SelectMode::SeekThird);

        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if ((toolWidget->isParameterSet(WParameter::Second) ||
            toolWidget->isParameterSet(WParameter::Third))) {

            doEnforceWidgetParameters(prevCursorPosition);
            handler->updateDataAndDrawToPosition(prevCursorPosition);


            if (toolWidget->isParameterSet(WParameter::Second) &&
                toolWidget->isParameterSet(WParameter::Third)) {

                handler->setState(SelectMode::End);
                handler->finish();
            }
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::addConstraints() {

    auto depth = toolWidget->getParameter(WParameter::Third);

    auto startLengthSet = toolWidget->isParameterSet(WParameter::First);
    auto insertLengthSet = toolWidget->isParameterSet(WParameter::Second);
    auto depthSet = toolWidget->isParameterSet(WParameter::Third);


    if (startLengthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            dHandler->firstCurve, 1, dHandler->firstCurve, 2, dHandler->startLength);


    if (dHandler->constructionMethod() == DrawSketchHandlerInsert::ConstructionMethod::Box) {
        if (insertLengthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve +2 , 1, dHandler->firstCurve + 2, 2, dHandler->boxLength);

        if (depthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve + 1, 1, dHandler->firstCurve + 1, 2, depth);

    }
    else {
        if (insertLengthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve, 2, dHandler->firstCurve + 2, 1, dHandler->boxLength);

        if (depthSet)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                dHandler->firstCurve + 1, 3, dHandler->firstCurve, 0, depth);
    }
}

template <> void DrawSketchHandlerInsertBase::ToolWidgetManager::onHandlerModeChanged() {
    switch (handler->state()) {
    case SelectMode::SeekSecond:
        toolWidget->setParameterEnabled(WParameter::First, true);
        toolWidget->setParameterEnabled(WParameter::Second, true);
        toolWidget->setParameterEnabled(WParameter::Third, true);
        toolWidget->setParameterFocus(WParameter::First);
        break;
    case SelectMode::SeekThird:
        toolWidget->setParameterFocus(WParameter::Second);
        break;
    default:
        break;
    }
}


DEF_STD_CMD_A(CmdSketcherInsert)

CmdSketcherInsert::CmdSketcherInsert()
    : Command("Sketcher_Insert")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Insert geometry on line");
    sToolTipText = QT_TR_NOOP("Insert a geometry in a line while preserving constraints.");
    sWhatsThis = "Sketcher_Insert";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_Insert";
    sAccel = "G, I";
    eType = ForEdit;
}

void CmdSketcherInsert::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //The following code let us catch if user selected an edge before launching the tool.
    int geoId = Sketcher::GeoEnum::GeoUndef;
    std::vector<Gui::SelectionObject> selection;
    selection = Gui::Command::getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() == 1) {
        // get the needed lists and objects
        const std::vector<std::string>& SubNames = selection[0].getSubNames();
        if (SubNames.size() == 1) {
            if (SubNames[0].size() > 4 && SubNames[0].substr(0, 4) == "Edge") {
                geoId = std::atoi(SubNames[0].substr(4, 4000).c_str()) - 1;
            }
        }
    }
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerInsert(geoId));
}

bool CmdSketcherInsert::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Modify edge comp ====================================================================*/

DEF_STD_CMD_ACL(CmdSketcherCompModifyEdge)

CmdSketcherCompModifyEdge::CmdSketcherCompModifyEdge()
    : Command("Sketcher_CompModifyEdge")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Modify an edge");
    sToolTipText = QT_TR_NOOP("Trim, extend, split... and edge");
    sWhatsThis = "Sketcher_CompModifyEdge";
    sStatusTip = sToolTipText;
    sAccel = "G, T";
    eType = ForEdit;
}

void CmdSketcherCompModifyEdge::activated(int iMsg)
{
    switch (iMsg) {
    case 0:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerTrimming()); break;
    case 1:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExtend()); break;
    case 2:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSplitting()); break;
    case 3:
    {
        //The following code let us catch if user selected an edge before launching the tool.
        int geoId = Sketcher::GeoEnum::GeoUndef;
        std::vector<Gui::SelectionObject> selection;
        selection = Gui::Command::getSelection().getSelectionEx(0, Sketcher::SketchObject::getClassTypeId());

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() == 1) {
            // get the needed lists and objects
            const std::vector<std::string>& SubNames = selection[0].getSubNames();
            if (SubNames.size() == 1) {
                if (SubNames[0].size() > 4 && SubNames[0].substr(0, 4) == "Edge") {
                    geoId = std::atoi(SubNames[0].substr(4, 4000).c_str()) - 1;
                }
            }
        }
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerInsert(geoId)); break;
    }
    break;
    default:
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompModifyEdge::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* trim = pcAction->addAction(QString());
    trim->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Trimming"));
    QAction* extend = pcAction->addAction(QString());
    extend->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Extend"));
    QAction* split = pcAction->addAction(QString());
    split->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Split"));
    QAction* insert = pcAction->addAction(QString());
    insert->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_Insert"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(trim->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompModifyEdge::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* trim = a[0];
    trim->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Trim edge"));
    trim->setToolTip(QApplication::translate("Sketcher_Trimming", "Trim an edge with respect to the picked position"));
    trim->setStatusTip(QApplication::translate("Sketcher_Trimming", "Trim an edge with respect to the picked position"));
    QAction* extend = a[1];
    extend->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Extend edge"));
    extend->setToolTip(QApplication::translate("Sketcher_Extend", "Extend an edge with respect to the picked position"));
    extend->setStatusTip(QApplication::translate("Sketcher_Extend", "Extend an edge with respect to the picked position"));
    QAction* split = a[2];
    split->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Split edge"));
    split->setToolTip(QApplication::translate("Sketcher_Split", "Splits an edge into two while preserving constraintst"));
    split->setStatusTip(QApplication::translate("Sketcher_Split", "Splits an edge into two while preserving constraints"));
    QAction* insert = a[3];
    insert->setText(QApplication::translate("CmdSketcherCompModifyEdge", "Insert geometry on line"));
    split->setToolTip(QApplication::translate("Sketcher_Insert", "Insert a geometry in a line while preserving constraints"));
    split->setStatusTip(QApplication::translate("Sketcher_Insert", "Insert a geometry in a line while preserving constraints"));
}

bool CmdSketcherCompModifyEdge::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* External Geometries ==================================================================*/

namespace SketcherGui {
    class ExternalSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        ExternalSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr), object(obj)
        {}

        bool allow(App::Document *pDoc, App::DocumentObject *pObj, const char *sSubName)
        {
            Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(object);

            this->notAllowedReason = "";
            Sketcher::SketchObject::eReasonList msg;
            if (!sketch->isExternalAllowed(pDoc, pObj, &msg)){
                switch(msg){
                case Sketcher::SketchObject::rlCircularReference:
                    this->notAllowedReason = QT_TR_NOOP("Linking this will cause circular dependency.");
                    break;
                case Sketcher::SketchObject::rlOtherDoc:
                    this->notAllowedReason = QT_TR_NOOP("This object is in another document.");
                    break;
                case Sketcher::SketchObject::rlOtherBody:
                    this->notAllowedReason = QT_TR_NOOP("This object belongs to another body, can't link.");
                    break;
                case Sketcher::SketchObject::rlOtherPart:
                    this->notAllowedReason = QT_TR_NOOP("This object belongs to another part, can't link.");
                    break;
                default:
                    break;
                }
                return false;
            }

            // Note: its better to search the support of the sketch in case the sketch support is a base plane
            //Part::BodyBase* body = Part::BodyBase::findBodyOf(sketch);
            //if ( body && body->hasFeature ( pObj ) && body->isAfter ( pObj, sketch ) ) {
                // Don't allow selection after the sketch in the same body
                // NOTE: allowness of features in other bodies is handled by SketchObject::isExternalAllowed()
                // TODO may be this should be in SketchObject::isExternalAllowed() (2015-08-07, Fat-Zer)
                //return false;
            //}

            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if ((element.size() > 4 && element.substr(0,4) == "Edge") ||
                (element.size() > 6 && element.substr(0,6) == "Vertex") ||
                (element.size() > 4 && element.substr(0,4) == "Face")) {
                return true;
            }
            if (pObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ||
                pObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()))
                return true;
            return  false;
        }
    };
}

class DrawSketchHandlerExternal: public DrawSketchHandler
{
public:
    DrawSketchHandlerExternal() = default;
    virtual ~DrawSketchHandlerExternal()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Gui::Selection().getPreselection().pObjectName)
            applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        /* this is ok not to call to purgeHandler
        * in continuous creation mode because the
        * handler is destroyed by the quit() method on pressing the
        * right button of the mouse */
        return true;
    }

    virtual bool onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            App::DocumentObject* obj = sketchgui->getObject()->getDocument()->getObject(msg.pObjectName);
            if (obj == nullptr)
                throw Base::ValueError("Sketcher: External geometry: Invalid object in selection");
            std::string subName(msg.pSubName);
            if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ||
                obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()) ||
                (subName.size() > 4 && subName.substr(0,4) == "Edge") ||
                (subName.size() > 6 && subName.substr(0,6) == "Vertex") ||
                (subName.size() > 4 && subName.substr(0,4) == "Face")) {
                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add external geometry"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "addExternal(\"%s\",\"%s\")",
                              msg.pObjectName, msg.pSubName);
                    Gui::Command::commitCommand();

                    // adding external geometry does not require a solve() per se (the DoF is the same),
                    // however a solve is required to update the amount of solver geometry, because we only
                    // redraw a changed Sketch if the solver geometry amount is the same as the SkethObject
                    // geometry amount (as this avoids other issues).
                    // This solver is a very low cost one anyway (there is actually nothing to solve).
                    tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                    Gui::Selection().clearSelection();
                /* this is ok not to call to purgeHandler
                * in continuous creation mode because the
                * handler is destroyed by the quit() method on pressing the
                * right button of the mouse */
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("Failed to add external geometry: %s\n", e.what());
                    Gui::Selection().clearSelection();
                    Gui::Command::abortCommand();
                }
                return true;
            }
        }
        return false;
    }

private:
    virtual void activated() override
    {
        setAxisPickStyle(false);
        Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer *viewer;
        viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

        SoNode* root = viewer->getSceneGraph();
        static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);

        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new ExternalSelection(sketchgui->getObject()));
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_External");
    }

    virtual void deactivated() override
    {
        Q_UNUSED(sketchgui);
        setAxisPickStyle(true);
    }
};

DEF_STD_CMD_A(CmdSketcherExternal)

CmdSketcherExternal::CmdSketcherExternal()
  : Command("Sketcher_External")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("External geometry");
    sToolTipText    = QT_TR_NOOP("Create an edge linked to an external geometry");
    sWhatsThis      = "Sketcher_External";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_External";
    sAccel          = "G, X";
    eType           = ForEdit;
}

void CmdSketcherExternal::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerExternal());
}

bool CmdSketcherExternal::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// ======================================================================================

namespace SketcherGui {
    class CarbonCopySelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        CarbonCopySelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate((Gui::SelectionFilter*)nullptr), object(obj)
        {}

        bool allow(App::Document *pDoc, App::DocumentObject *pObj, const char *sSubName)
        {
            Q_UNUSED(sSubName);

            Sketcher::SketchObject *sketch = static_cast<Sketcher::SketchObject*>(object);
            sketch->setAllowOtherBody(QApplication::keyboardModifiers() == Qt::ControlModifier || QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier));
            sketch->setAllowUnaligned(QApplication::keyboardModifiers() == (Qt::ControlModifier | Qt::AltModifier));

            this->notAllowedReason = "";
            Sketcher::SketchObject::eReasonList msg;
            // Reusing code: All good reasons not to allow a carbon copy
            bool xinv = false, yinv = false;
            if (!sketch->isCarbonCopyAllowed(pDoc, pObj, xinv, yinv, &msg)){
                switch(msg){
                    case Sketcher::SketchObject::rlCircularReference:
                        this->notAllowedReason = QT_TR_NOOP("Carbon copy would cause a circular dependency.");
                        break;
                    case Sketcher::SketchObject::rlOtherDoc:
                        this->notAllowedReason = QT_TR_NOOP("This object is in another document.");
                        break;
                    case Sketcher::SketchObject::rlOtherBody:
                        this->notAllowedReason = QT_TR_NOOP("This object belongs to another body. Hold Ctrl to allow cross-references.");
                        break;
                    case Sketcher::SketchObject::rlOtherBodyWithLinks:
                        this->notAllowedReason = QT_TR_NOOP("This object belongs to another body and it contains external geometry. Cross-reference not allowed.");
                        break;
                    case Sketcher::SketchObject::rlOtherPart:
                        this->notAllowedReason = QT_TR_NOOP("This object belongs to another part.");
                        break;
                    case Sketcher::SketchObject::rlNonParallel:
                        this->notAllowedReason = QT_TR_NOOP("The selected sketch is not parallel to this sketch. Hold Ctrl+Alt to allow non-parallel sketches.");
                        break;
                    case Sketcher::SketchObject::rlAxesMisaligned:
                        this->notAllowedReason = QT_TR_NOOP("The XY axes of the selected sketch do not have the same direction as this sketch. Hold Ctrl+Alt to disregard it.");
                        break;
                    case Sketcher::SketchObject::rlOriginsMisaligned:
                        this->notAllowedReason = QT_TR_NOOP("The origin of the selected sketch is not aligned with the origin of this sketch. Hold Ctrl+Alt to disregard it.");
                        break;
                    default:
                        break;
                }
                return false;
            }
            // Carbon copy only works on sketches that are not disallowed (e.g. would produce a circular reference)
            return  true;
        }
    };
}


class DrawSketchHandlerCarbonCopy: public DrawSketchHandler
{
public:
    DrawSketchHandlerCarbonCopy() = default;
    virtual ~DrawSketchHandlerCarbonCopy()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        if (Gui::Selection().getPreselection().pObjectName)
            applyCursor();
    }

    virtual bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        /* this is ok not to call to purgeHandler
            * in continuous creation mode because the
            * handler is destroyed by the quit() method on pressing the
            * right button of the mouse */
        return true;
    }

    virtual bool onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            App::DocumentObject* obj = sketchgui->getObject()->getDocument()->getObject(msg.pObjectName);
            if (obj == nullptr)
                throw Base::ValueError("Sketcher: Carbon Copy: Invalid object in selection");

            if (obj->getTypeId() == Sketcher::SketchObject::getClassTypeId()) {

                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add carbon copy"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(), "carbonCopy(\"%s\",%s)",
                                            msg.pObjectName, geometryCreationMode==Construction?"True":"False");

                    Gui::Command::commitCommand();

                    tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));

                    Gui::Selection().clearSelection();
                    /* this is ok not to call to purgeHandler
                        * in continuous creation mode because the
                        * handler is destroyed by the quit() method on pressing the
                        * right button of the mouse */
                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("Failed to add carbon copy: %s\n", e.what());
                    Gui::Command::abortCommand();
                }
                return true;
                }
        }
        return false;
    }

private:
    virtual void activated() override
    {
        setAxisPickStyle(false);
        Gui::MDIView *mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer *viewer;
        viewer = static_cast<Gui::View3DInventor *>(mdi)->getViewer();

        SoNode* root = viewer->getSceneGraph();
        static_cast<Gui::SoFCUnifiedSelection*>(root)->selectionRole.setValue(true);

        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new CarbonCopySelection(sketchgui->getObject()));
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_CarbonCopy");
    }

    virtual void deactivated() override
    {
        Q_UNUSED(sketchgui);
        setAxisPickStyle(true);
    }
};

DEF_STD_CMD_AU(CmdSketcherCarbonCopy)

CmdSketcherCarbonCopy::CmdSketcherCarbonCopy()
: Command("Sketcher_CarbonCopy")
{
    sAppModule      = "Sketcher";
    sGroup          = "Sketcher";
    sMenuText       = QT_TR_NOOP("Carbon copy");
    sToolTipText    = QT_TR_NOOP("Copies the geometry of another sketch");
    sWhatsThis      = "Sketcher_CarbonCopy";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_CarbonCopy";
    sAccel          = "G, W";
    eType           = ForEdit;
}

void CmdSketcherCarbonCopy::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerCarbonCopy());
}

bool CmdSketcherCarbonCopy::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

void CmdSketcherCarbonCopy::updateAction(int mode)
{
    switch (mode) {
        case Normal:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CarbonCopy"));
            break;
        case Construction:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CarbonCopy_Constr"));
            break;
    }
}


/* Create Slot =========================================================*/

class DrawSketchHandlerSlot;

using DrawSketchHandlerSlotBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerSlot,
    StateMachines::ThreeSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<5>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

class DrawSketchHandlerSlot : public DrawSketchHandlerSlotBase
{
    friend DrawSketchHandlerSlotBase;
public:

    enum class SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerSlot() :
        radius(1)
        , angleIsSet(false), lengthIsSet(false)
        ,isHorizontal(false), isVertical(false) {}

    virtual ~DrawSketchHandlerSlot() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            startPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            secondPoint = onSketchPos;

            angle = GetPointAngle(startPoint, secondPoint);
            length = (secondPoint - startPoint).Length();
            radius = length / 5; //radius chosen at 1/5 of length

            if (!angleIsSet && snapMode == SnapMode::Snap5Degree) {
                angle = round(angle / (M_PI / 36)) * M_PI / 36;
                secondPoint = startPoint + length * Base::Vector2d(cos(angle), sin(angle));

                if (std::fmod(angle, M_PI) < Precision::Confusion())
                    isHorizontal = true;
                else if (std::fmod(angle, M_PI / 2) < Precision::Confusion())
                    isVertical = true;
            }

            drawEdit(createSlotGeometries());

            SbString text;
            text.sprintf(" (%.1fL)", length);
            setPositionText(onSketchPos, text);

            if ((isHorizontal || isVertical) && seekAutoConstraint(sugConstraints[1], onSketchPos, secondPoint - startPoint, AutoConstraint::VERTEX_NO_TANGENCY)) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
            else if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            /*To follow the cursor, r should adapt depending on the position of the cursor. If cursor is 'between' the center points,
            then its distance to that line and not distance to the second center.
            A is "between" B and C if angle ∠ABC and angle ∠ACB are both less than or equal to ninety degrees.
            An angle ∠ABC is greater than ninety degrees iff AB^2 + BC^2 < AC^2.*/

            double L1 = (onSketchPos - startPoint).Length();//distance between first center and onSketchPos
            double L2 = (onSketchPos - secondPoint).Length(); //distance between second center and onSketchPos

            if ((L1 * L1 + length * length > L2 * L2) && (L2 * L2 + length * length > L1 * L1)) {
                //distance of onSketchPos to the line StartPos-SecondPos
                radius = (abs((secondPoint.y - startPoint.y) * onSketchPos.x - (secondPoint.x - startPoint.x) * onSketchPos.y + secondPoint.x * startPoint.y - secondPoint.y * startPoint.x)) / length;
            }
            else {
                radius = min(L1, L2);
            }

            drawEdit(createSlotGeometries());

            SbString text;
            text.sprintf(" (%.1fR)", radius);
            setPositionText(onSketchPos, text);

            //Todo: we could add another auto constraint but we would need to know to which geometry to add it depending on mouse position.
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        int firstCurve = getHighestCurveIndex() + 1;

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));

            sketchgui->getSketchObject()->addGeometry(std::move(createSlotGeometries()));

            Gui::Command::doCommand(Gui::Command::Doc,
                "conList = []\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 1))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 1))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 2, %i, 2))\n"
                "conList.append(Sketcher.Constraint('Tangent', %i, 1, %i, 2))\n"
                "conList.append(Sketcher.Constraint('Equal', %i, %i))\n"
                "%s.addConstraint(conList)\n"
                "del conList\n",
                firstCurve, firstCurve + 2,     // tangent1
                firstCurve, firstCurve + 3,     // tangent2
                firstCurve + 1, firstCurve + 2, // tangent3
                firstCurve + 1, firstCurve + 3, // tangent4
                firstCurve, firstCurve + 1,     // equal constraint
                Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add slot: %s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
        angleIsSet = false;
        lengthIsSet = false;
    }

    virtual void createAutoConstraints() override {
        // add auto constraints at the center of the first arc
        if (sugConstraints[0].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex() - 3, Sketcher::PointPos::mid);
            sugConstraints[0].clear();
        }

        // add auto constraints at the center of the second arc
        if (sugConstraints[1].size() > 0) {
            if (isHorizontal || isVertical)
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex(), Sketcher::PointPos::none);
            else
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 2, Sketcher::PointPos::mid);
            sugConstraints[1].clear();
        }
        isHorizontal = false;
        isVertical = false;
    }

    virtual std::string getToolName() const override {
        return "DSH_Slot";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_Slot");
    }

private:
    SnapMode snapMode;
    Base::Vector2d startPoint, secondPoint;
    double radius, length, angle;
    bool angleIsSet, lengthIsSet, isHorizontal, isVertical;

    std::vector<Part::Geometry*> createSlotGeometries() {
        std::vector<Part::Geometry*> geometriesToAdd;

        Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
        arc1->setRadius(radius);
        arc1->setRange(M_PI / 2 + angle, 1.5 * M_PI + angle, true);
        arc1->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
        Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
        geometriesToAdd.push_back(arc1);

        Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
        arc2->setRadius(radius);
        arc2->setRange(1.5 * M_PI + angle, M_PI / 2 + angle, true);
        arc2->setCenter(Base::Vector3d(secondPoint.x, secondPoint.y, 0.));
        Sketcher::GeometryFacade::setConstruction(arc2, geometryCreationMode);
        geometriesToAdd.push_back(arc2);

        Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
        line1->setPoints(arc1->getStartPoint(), arc2->getEndPoint());
        Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
        geometriesToAdd.push_back(line1);

        Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
        line2->setPoints(arc1->getEndPoint(), arc2->getStartPoint());
        Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
        geometriesToAdd.push_back(line2);

        return geometriesToAdd;
    }
};

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::configureToolWidget() {
    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("ToolWidgetManager_p1", "x of 1st point"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("ToolWidgetManager_p2", "y of 1st point"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("ToolWidgetManager_p3", "Length"));
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("ToolWidgetManager_p4", "Angle to HAxis"));
    toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("ToolWidgetManager_p5", "Radius"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_p3_notice", "Press Ctrl to snap angle at 5° steps."));
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {

    switch (parameterindex) {
    case WParameter::First:
        dHandler->startPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->startPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->length = value;
        dHandler->lengthIsSet = true;
        break;
    case WParameter::Fourth:
        dHandler->angle = value * M_PI / 180;
        dHandler->angleIsSet = true;
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        dHandler->length = (onSketchPos - dHandler->startPoint).Length();

        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->length = toolWidget->getParameter(WParameter::Third);
            Base::Vector2d v = onSketchPos - dHandler->startPoint;
            onSketchPos = dHandler->startPoint + v * dHandler->length / v.Length();
        }

        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            dHandler->angle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
            onSketchPos.x = dHandler->startPoint.x + cos(dHandler->angle) * dHandler->length;
            onSketchPos.y = dHandler->startPoint.y + sin(dHandler->angle) * dHandler->length;
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {
            dHandler->radius = toolWidget->getParameter(WParameter::Fifth);
            onSketchPos.x = dHandler->secondPoint.x + cos(dHandler->angle) * dHandler->radius;
            onSketchPos.y = dHandler->secondPoint.y + sin(dHandler->angle) * dHandler->radius;
        }

    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->length);

        if (!toolWidget->isParameterSet(WParameter::Fourth))
                toolWidget->updateVisualValue(WParameter::Fourth, dHandler->angle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Fifth))
            toolWidget->updateVisualValue(WParameter::Fifth, dHandler->radius);
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::doChangeDrawSketchHandlerMode() {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First) &&
            toolWidget->isParameterSet(WParameter::Second)) {

            handler->setState(SelectMode::SeekSecond);
        }
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (toolWidget->isParameterSet(WParameter::Third) ||
            toolWidget->isParameterSet(WParameter::Fourth)) {

            if (toolWidget->isParameterSet(WParameter::Third) &&
                toolWidget->isParameterSet(WParameter::Fourth)) {

                handler->setState(SelectMode::SeekThird);
            }
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {

            handler->setState(SelectMode::End);
            handler->finish();
        }
    }
    break;
    default:
        break;
    }

}

template <> void DrawSketchHandlerSlotBase::ToolWidgetManager::addConstraints() {

    int firstCurve = handler->getHighestCurveIndex() - 3;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);
    auto lengthSet = toolWidget->isParameterSet(WParameter::Third);
    auto angleSet = toolWidget->isParameterSet(WParameter::Fourth);
    auto radiusSet = toolWidget->isParameterSet(WParameter::Fifth);

    using namespace Sketcher;

    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                y0, handler->sketchgui->getObject());
    }

    if (lengthSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
            firstCurve, 3, firstCurve + 1, 3, dHandler->length);

    if (angleSet) {
        if (fabs(dHandler->angle - M_PI) < Precision::Confusion() || fabs(dHandler->angle + M_PI) < Precision::Confusion() || fabs(dHandler->angle) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Horizontal',%d)) ", firstCurve + 2);
        }
        else if (fabs(dHandler->angle - M_PI/2) < Precision::Confusion() || fabs(dHandler->angle + M_PI / 2) < Precision::Confusion()) {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Vertical',%d)) ", firstCurve + 2);
        }
        else {
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%d,%f)) ",
                Sketcher::GeoEnum::HAxis, firstCurve + 2, dHandler->angle);
        }
    }

    if (radiusSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
            firstCurve, dHandler->radius);
}

DEF_STD_CMD_A(CmdSketcherCreateSlot)

CmdSketcherCreateSlot::CmdSketcherCreateSlot()
    : Command("Sketcher_CreateSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create slot");
    sToolTipText = QT_TR_NOOP("Create a slot in the sketch");
    sWhatsThis = "Sketcher_CreateSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateSlot";
    sAccel = "G, S";
    eType = ForEdit;
}

void CmdSketcherCreateSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSlot());
}

bool CmdSketcherCreateSlot::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Create Arc Slot =========================================================*/
class DrawSketchHandlerArcSlot;

namespace SketcherGui::ConstructionMethods {

enum class ArcSlotConstructionMethod {
    ArcSlot,
    RectangleSlot,
    End // Must be the last one
};

}

using DrawSketchHandlerArcSlotBase = DrawSketchDefaultWidgetHandler<  DrawSketchHandlerArcSlot,
    StateMachines::FourSeekEnd,
    /*PEditCurveSize =*/ 0,
    /*PAutoConstraintSize =*/ 3,
    /*WidgetParametersT =*/WidgetParameters<6, 6>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<0, 0>,
    /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,
    ConstructionMethods::ArcSlotConstructionMethod,
    /*bool PFirstComboboxIsConstructionMethod =*/ true>;

class DrawSketchHandlerArcSlot : public DrawSketchHandlerArcSlotBase
{
    friend DrawSketchHandlerArcSlotBase;

public:
    enum class SnapMode {
        Free,
        Snap5Degree
    };

    DrawSketchHandlerArcSlot(ConstructionMethod constrMethod = ConstructionMethod::ArcSlot) :
        DrawSketchHandlerArcSlotBase(constrMethod)
        , startAngle(0)
        , endAngle(0)
        , arcAngle(0) {}

    virtual ~DrawSketchHandlerArcSlot() = default;

private:
    virtual void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override {
        if (QApplication::keyboardModifiers() == Qt::ControlModifier)
            snapMode = SnapMode::Snap5Degree;
        else
            snapMode = SnapMode::Free;

        switch (state()) {
        case SelectMode::SeekFirst:
        {
            drawPositionAtCursor(onSketchPos);
            centerPoint = onSketchPos;

            if (seekAutoConstraint(sugConstraints[0], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[0]);
                return;
            }
        }
        break;
        case SelectMode::SeekSecond:
        {
            startPoint = onSketchPos;

            startAngle = GetPointAngle(centerPoint, startPoint);
            radius = (startPoint - centerPoint).Length();

            if (snapMode == SnapMode::Snap5Degree) {
                startAngle = round(startAngle / (M_PI / 36)) * M_PI / 36;
                startPoint = centerPoint + radius * Base::Vector2d(cos(startAngle), sin(startAngle));
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomCircle* circle = new Part::GeomCircle();
            circle->setRadius(radius);
            circle->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
            geometriesToAdd.push_back(circle);

            //add line to show the snap at 5 degree.
            Part::GeomLineSegment* line = new Part::GeomLineSegment();
            line->setPoints(Base::Vector3d(centerPoint.x , centerPoint.y, 0.),
                Base::Vector3d(centerPoint.x + cos(startAngle) * 0.8 * radius, centerPoint.y + sin(startAngle) * 0.8 * radius, 0.));
            geometriesToAdd.push_back(line);

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)startAngle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[1], onSketchPos, Base::Vector2d(0.f, 0.f))) {
                renderSuggestConstraintsCursor(sugConstraints[1]);
                return;
            }
        }
        break;
        case SelectMode::SeekThird:
        {
            endPoint = centerPoint + (onSketchPos - centerPoint) / (onSketchPos - centerPoint).Length() * radius;
            r = radius / 10; //Auto radius to 1/10 of the arc radius

            double startAngleToDraw = startAngle;
            double angle1 = atan2(onSketchPos.y - centerPoint.y,
                onSketchPos.x - centerPoint.x) - startAngle;
            double angle2 = angle1 + (angle1 < 0. ? 2 : -2) * M_PI;
            arcAngle = abs(angle1 - arcAngle) < abs(angle2 - arcAngle) ? angle1 : angle2;

            if (snapMode == SnapMode::Snap5Degree) {
                arcAngle = round(arcAngle / (M_PI / 36)) * M_PI / 36;
                endPoint = centerPoint + radius * Base::Vector2d(cos(startAngle + arcAngle), sin(startAngle + arcAngle));
            }

            bool angleReversed = false;
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngleToDraw = startAngle + arcAngle;
                angleReversed = true;
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
            arc1->setRange(startAngleToDraw, endAngle, true);
            arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                arc1->setRadius(radius - r);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRadius(radius + r);
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                geometriesToAdd.push_back(arc2);

                Part::GeomArcOfCircle* arc3 = new Part::GeomArcOfCircle();
                arc3->setRadius(r);
                arc3->setRange(M_PI + startAngleToDraw, 2 * M_PI + startAngleToDraw, true);
                if (angleReversed)
                    arc3->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                else
                    arc3->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                geometriesToAdd.push_back(arc3);

                Part::GeomArcOfCircle* arc4 = new Part::GeomArcOfCircle();
                arc4->setRadius(r);
                arc4->setRange(endAngle, M_PI + endAngle, true);
                if (angleReversed)
                    arc4->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                else
                    arc4->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                geometriesToAdd.push_back(arc4);
            }
            else {
                arc1->setRadius(radius);
                geometriesToAdd.push_back(arc1);
            }

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR,%.1fdeg)", (float)radius, (float)arcAngle * 180 / M_PI);
            setPositionText(onSketchPos, text);

            if (seekAutoConstraint(sugConstraints[2], onSketchPos, Base::Vector2d(0.0, 0.0))) {
                renderSuggestConstraintsCursor(sugConstraints[2]);
                return;
            }
        }
        break;
        case SelectMode::SeekFourth:
        {
            double startAngleToDraw = startAngle;
            bool angleReversed = false;
            if (arcAngle > 0)
                endAngle = startAngle + arcAngle;
            else {
                endAngle = startAngle;
                startAngleToDraw = startAngle + arcAngle;
                angleReversed = true;
            }

            std::vector<Part::Geometry*> geometriesToAdd;
            Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
            arc1->setRange(startAngleToDraw, endAngle, true);
            arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));

            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                r = min(radius * 0.999, fabs(radius - (onSketchPos - centerPoint).Length()));
                arc1->setRadius(radius - r);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(radius + r);
                geometriesToAdd.push_back(arc2);

                Part::GeomArcOfCircle* arc3 = new Part::GeomArcOfCircle();
                arc3->setRange(M_PI + startAngleToDraw, 2 * M_PI + startAngleToDraw, true);
                if (angleReversed)
                    arc3->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                else
                    arc3->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                arc3->setRadius(r);
                geometriesToAdd.push_back(arc3);

                Part::GeomArcOfCircle* arc4 = new Part::GeomArcOfCircle();
                arc4->setRange(endAngle, M_PI + endAngle, true);
                if (angleReversed)
                    arc4->setCenter(Base::Vector3d(startPoint.x, startPoint.y, 0.));
                else
                    arc4->setCenter(Base::Vector3d(endPoint.x, endPoint.y, 0.));
                arc4->setRadius(r);
                geometriesToAdd.push_back(arc4);
            }
            else {
                r = max(0.001, (onSketchPos - centerPoint).Length());
                arc1->setRadius(radius);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngleToDraw, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(r);
                geometriesToAdd.push_back(arc2);

                Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
                line1->setPoints(arc1->getStartPoint(), arc2->getStartPoint());
                geometriesToAdd.push_back(line1);

                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                line2->setPoints(arc1->getEndPoint(), arc2->getEndPoint());
                geometriesToAdd.push_back(line2);
            }

            drawEdit(geometriesToAdd);

            SbString text;
            text.sprintf(" (%.1fR)", (float)r);
            setPositionText(onSketchPos, text);
        }
        break;
        default:
            break;
        }
    }

    virtual void executeCommands() override {
        int firstCurve = getHighestCurveIndex() + 1;

        if (arcAngle > 0)
            endAngle = startAngle + arcAngle;
        else {
            endAngle = startAngle;
            startAngle = startAngle + arcAngle;
        }

        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Add slot"));
            if (constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                Gui::Command::doCommand(Gui::Command::Doc,
                    "geoList = []\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f, 0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "geoList.append(Part.ArcOfCircle(Part.Circle(App.Vector(%f, %f ,0), App.Vector(0, 0, 1), %f), %f, %f))\n"
                    "%s.addGeometry(geoList, %s)\n"
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 3, %i, 3))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "conList.append(Sketcher.Constraint('Tangent', %i, %i, %i, %i))\n"
                    "%s.addConstraint(conList)\n"
                    "del geoList, conList\n",
                    centerPoint.x, centerPoint.y,         // center of the arc1
                    radius - r,                           // radius arc1
                    startAngle, endAngle,             // start and end angle of arc1
                    centerPoint.x, centerPoint.y,         // center of the arc2
                    radius + r,                           // radius arc2
                    startAngle, endAngle,             // start and end angle of arc2
                    startPoint.x, startPoint.y,           // center of the arc3
                    r,                                // radius arc3
                    (arcAngle > 0) ? startAngle + M_PI : endAngle,
                    (arcAngle > 0) ? startAngle + 2 * M_PI : endAngle + M_PI,    // start and end angle of arc3
                    endPoint.x, endPoint.y,               // center of the arc4
                    r,                                // radius arc4
                    (arcAngle > 0) ? endAngle : startAngle + M_PI,
                    (arcAngle > 0) ? endAngle + M_PI : startAngle + 2 * M_PI,        // start and end angle of arc4
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str(), // the sketch
                    geometryCreationMode == Construction ? "True" : "False", // geometry as construction or not
                    firstCurve, firstCurve + 1,      // coicident1: mid of the two arcs
                    firstCurve, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent1
                    firstCurve + 3, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent1
                    firstCurve, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent2
                    firstCurve + 2, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent2
                    firstCurve + 1, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent3
                    firstCurve + 3, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent3
                    firstCurve + 1, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end,     //tangent4
                    firstCurve + 2, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start,     //tangent4
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch
            }
            else {
                std::vector<Part::Geometry*> geometriesToAdd;
                Part::GeomArcOfCircle* arc1 = new Part::GeomArcOfCircle();
                arc1->setRange(startAngle, endAngle, true);
                arc1->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc1->setRadius(radius);
                Sketcher::GeometryFacade::setConstruction(arc1, geometryCreationMode);
                geometriesToAdd.push_back(arc1);

                Part::GeomArcOfCircle* arc2 = new Part::GeomArcOfCircle();
                arc2->setRange(startAngle, endAngle, true);
                arc2->setCenter(Base::Vector3d(centerPoint.x, centerPoint.y, 0.));
                arc2->setRadius(r);
                Sketcher::GeometryFacade::setConstruction(arc2, geometryCreationMode);
                geometriesToAdd.push_back(arc2);

                Part::GeomLineSegment* line1 = new Part::GeomLineSegment();
                line1->setPoints(arc1->getStartPoint(), arc2->getStartPoint());
                Sketcher::GeometryFacade::setConstruction(line1, geometryCreationMode);
                geometriesToAdd.push_back(line1);

                Part::GeomLineSegment* line2 = new Part::GeomLineSegment();
                line2->setPoints(arc1->getEndPoint(), arc2->getEndPoint());
                Sketcher::GeometryFacade::setConstruction(line2, geometryCreationMode);
                geometriesToAdd.push_back(line2);

                sketchgui->getSketchObject()->addGeometry(std::move(geometriesToAdd));

                Gui::Command::doCommand(Gui::Command::Doc,
                    "conList = []\n"
                    "conList.append(Sketcher.Constraint('Perpendicular', %i, 0, %i, 0))\n"
                    "conList.append(Sketcher.Constraint('Perpendicular', %i, 0, %i, 0))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 3, %i, 3))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 1, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 2, %i, 1))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 1, %i, 2))\n"
                    "conList.append(Sketcher.Constraint('Coincident', %i, 2, %i, 2))\n"
                    "%s.addConstraint(conList)\n"
                    "del conList\n",
                    firstCurve, firstCurve + 2,     // perpendicular1
                    firstCurve, firstCurve + 3,     // perpendicular2
                    firstCurve, firstCurve + 1,      // coicident1: mid of the two arcs
                    firstCurve, firstCurve + 2,     // coicident2
                    firstCurve, firstCurve + 3,     // coicident3
                    firstCurve + 1, firstCurve + 2, // coicident4
                    firstCurve + 1, firstCurve + 3, // coicident5
                    Gui::Command::getObjectCmd(sketchgui->getObject()).c_str()); // the sketch

            }
            Gui::Command::commitCommand();

        }
        catch (const Base::Exception& e) {
            Base::Console().Error("Failed to add slot: %s\n", e.what());
            Gui::Command::abortCommand();

            tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
        }
    }

    virtual void createAutoConstraints() override {
        // Auto Constraint center point
        if (sugConstraints[0].size() > 0) {
            DrawSketchHandler::createAutoConstraints(sugConstraints[0], getHighestCurveIndex() - 3, Sketcher::PointPos::mid);
            sugConstraints[0].clear();
        }

        if (constructionMethod() == ConstructionMethod::ArcSlot) {
            // Auto Constraint first picked point
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 1, Sketcher::PointPos::mid);
                sugConstraints[1].clear();
            }

            // Auto Constraint second picked point
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex(), Sketcher::PointPos::mid);
                sugConstraints[2].clear();
            }
        }
        else {
            // Auto Constraint start point of first arc
            if (sugConstraints[1].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[1], getHighestCurveIndex() - 3, (arcAngle > 0) ? Sketcher::PointPos::start : Sketcher::PointPos::end);
                sugConstraints[1].clear();
            }

            // Auto Constraint end point of first arc
            if (sugConstraints[2].size() > 0) {
                DrawSketchHandler::createAutoConstraints(sugConstraints[2], getHighestCurveIndex() - 3, (arcAngle > 0) ? Sketcher::PointPos::end : Sketcher::PointPos::start);
                sugConstraints[2].clear();
            }
        }
    }

    virtual std::string getToolName() const override {
        return "DSH_ArcSlot";
    }

    virtual QString getCrosshairCursorSVGName() const override {
        if(constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
            if (geometryCreationMode)
                return QString::fromLatin1("Sketcher_CreateArcSlot_Constr");
            else
                return QString::fromLatin1("Sketcher_CreateArcSlot");
        }
        else { // constructionMethod == DrawSketchHandlerArcSlot::ConstructionMethod::RectangleSlot
            if (geometryCreationMode)
                return QString::fromLatin1("Sketcher_CreateRectangleSlot_Constr");
            else
                return QString::fromLatin1("Sketcher_CreateRectangleSlot");
        }

        return QStringLiteral("None");
    }

private:
    SnapMode snapMode;
    Base::Vector2d centerPoint, startPoint, endPoint;
    double startAngle, endAngle, arcAngle, r, radius;
};

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::configureToolWidget() {
    if(!init) { // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Arc ends"), QStringLiteral("Flat ends")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);
    }

    toolWidget->setParameterLabel(WParameter::First, QApplication::translate("TaskSketcherTool_p1_arcSlot", "x of center"));
    toolWidget->setParameterLabel(WParameter::Second, QApplication::translate("TaskSketcherTool_p2_arcSlot", "y of center"));
    toolWidget->setParameterLabel(WParameter::Third, QApplication::translate("TaskSketcherTool_p3_arcSlot", "Radius"));
    toolWidget->setParameterLabel(WParameter::Fourth, QApplication::translate("TaskSketcherTool_p4_arcSlot", "Start angle"));
    toolWidget->configureParameterUnit(WParameter::Fourth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Fifth, QApplication::translate("TaskSketcherTool_p5_arcSlot", "Arc angle"));
    toolWidget->configureParameterUnit(WParameter::Fifth, Base::Unit::Angle);
    toolWidget->setParameterLabel(WParameter::Sixth, QApplication::translate("TaskSketcherTool_p6_arcSlot", "Slot width"));

    toolWidget->setNoticeVisible(true);
    toolWidget->setNoticeText(QApplication::translate("TaskSketcherTool_notice_arcSlot", "Press Ctrl to snap angle at 5° steps."));
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::adaptDrawingToParameterChange(int parameterindex, double value) {
    switch (parameterindex) {
    case WParameter::First:
        dHandler->centerPoint.x = value;
        break;
    case WParameter::Second:
        dHandler->centerPoint.y = value;
        break;
    case WParameter::Third:
        dHandler->radius = value;
        break;
    case WParameter::Fourth:
        dHandler->startAngle = value * M_PI / 180;
        break;
    case WParameter::Fifth:
        dHandler->arcAngle = value * M_PI / 180;
        break;
    case WParameter::Sixth:
        if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            dHandler->r = value/2;
        else
            dHandler->r = value;
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::doEnforceWidgetParameters(Base::Vector2d& onSketchPos) {

    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (toolWidget->isParameterSet(WParameter::First))
            onSketchPos.x = toolWidget->getParameter(WParameter::First);

        if (toolWidget->isParameterSet(WParameter::Second))
            onSketchPos.y = toolWidget->getParameter(WParameter::Second);
    }
    break;
    case SelectMode::SeekSecond:
    {
        double length = (onSketchPos - dHandler->centerPoint).Length();
        if (toolWidget->isParameterSet(WParameter::Third)) {
            dHandler->radius = toolWidget->getParameter(WParameter::Third);
            if (length != 0.) {
                onSketchPos = dHandler->centerPoint + (onSketchPos - dHandler->centerPoint) * dHandler->radius / length;
            }
        }
        if (toolWidget->isParameterSet(WParameter::Fourth)) {
            dHandler->startAngle = toolWidget->getParameter(WParameter::Fourth) * M_PI / 180;
            onSketchPos.x = dHandler->centerPoint.x + cos(dHandler->startAngle) * length;
            onSketchPos.y = dHandler->centerPoint.y + sin(dHandler->startAngle) * length;
        }
    }
    break;
    case SelectMode::SeekThird:
    {
        if (toolWidget->isParameterSet(WParameter::Fifth)) {
            dHandler->arcAngle = toolWidget->getParameter(WParameter::Fifth) * M_PI / 180;
            double length = (onSketchPos - dHandler->centerPoint).Length();
            onSketchPos.x = dHandler->centerPoint.x + cos((dHandler->startAngle + dHandler->arcAngle)) * length;
            onSketchPos.y = dHandler->centerPoint.y + sin((dHandler->startAngle + dHandler->arcAngle)) * length;
        }
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (toolWidget->isParameterSet(WParameter::Sixth)) {
            if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot) {
                dHandler->r = toolWidget->getParameter(WParameter::Sixth) / 2;
            }
            else {
                dHandler->r = toolWidget->getParameter(WParameter::Sixth);
            }
            onSketchPos = dHandler->centerPoint + Base::Vector2d(dHandler->radius + dHandler->r, 0.);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::adaptWidgetParameters(Base::Vector2d onSketchPos) {
    switch (handler->state()) {
    case SelectMode::SeekFirst:
    {
        if (!toolWidget->isParameterSet(WParameter::First))
            toolWidget->updateVisualValue(WParameter::First, onSketchPos.x);

        if (!toolWidget->isParameterSet(WParameter::Second))
            toolWidget->updateVisualValue(WParameter::Second, onSketchPos.y);
    }
    break;
    case SelectMode::SeekSecond:
    {
        if (!toolWidget->isParameterSet(WParameter::Third))
            toolWidget->updateVisualValue(WParameter::Third, dHandler->radius);

        if (!toolWidget->isParameterSet(WParameter::Fourth))
            toolWidget->updateVisualValue(WParameter::Fourth, dHandler->startAngle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekThird:
    {
        if (!toolWidget->isParameterSet(WParameter::Fifth))
            toolWidget->updateVisualValue(WParameter::Fifth, dHandler->arcAngle * 180 / M_PI, Base::Unit::Angle);
    }
    break;
    case SelectMode::SeekFourth:
    {
        if (!toolWidget->isParameterSet(WParameter::Sixth)) {
            if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->r);
            else
                toolWidget->updateVisualValue(WParameter::Sixth, dHandler->r - dHandler->radius);
        }
    }
    break;
    default:
        break;
    }
}

template <> void DrawSketchHandlerArcSlotBase::ToolWidgetManager::addConstraints() {
    int firstCurve = handler->getHighestCurveIndex() - 3;
    using namespace Sketcher;

    auto x0 = toolWidget->getParameter(WParameter::First);
    auto y0 = toolWidget->getParameter(WParameter::Second);

    auto x0set = toolWidget->isParameterSet(WParameter::First);
    auto y0set = toolWidget->isParameterSet(WParameter::Second);

    auto radiusSet = toolWidget->isParameterSet(WParameter::Third);
    auto arcAngleSet = toolWidget->isParameterSet(WParameter::Fifth);
    auto slotRadiusSet = toolWidget->isParameterSet(WParameter::Sixth);


    if (x0set && y0set && x0 == 0. && y0 == 0.) {
        ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::RtPnt,
            x0, handler->sketchgui->getObject());
    }
    else {
        if (x0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::VAxis,
                x0, handler->sketchgui->getObject());

        if (y0set)
            ConstraintToAttachment(GeoElementId(firstCurve, PointPos::mid), GeoElementId::HAxis,
                y0, handler->sketchgui->getObject());
    }

    if (radiusSet) {
        if (dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                firstCurve, 3, firstCurve + 2, 3, dHandler->radius);
        else
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve, dHandler->radius);
    }

    if (arcAngleSet)
        Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Angle',%d,%f)) ", firstCurve, fabs(dHandler->arcAngle));

    if (slotRadiusSet) {
        if(dHandler->constructionMethod() == DrawSketchHandlerArcSlot::ConstructionMethod::ArcSlot)
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                firstCurve + 2, dHandler->r);
        else
            Gui::cmdAppObjectArgs(handler->sketchgui->getObject(), "addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                firstCurve + 2, fabs(dHandler->radius - dHandler->r));
    }
}

DEF_STD_CMD_A(CmdSketcherCreateArcSlot)

CmdSketcherCreateArcSlot::CmdSketcherCreateArcSlot()
    : Command("Sketcher_CreateArcSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create arc slot");
    sToolTipText = QT_TR_NOOP("Create an arc slot in the sketch");
    sWhatsThis = "Sketcher_CreateArcSlot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_CreateArcSlot";
    sAccel = "G, S, 2";
    eType = ForEdit;
}

void CmdSketcherCreateArcSlot::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcSlot());
}

bool CmdSketcherCreateArcSlot::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/* Slot comp ============================================================*/

DEF_STD_CMD_ACLU(CmdSketcherCompCreateSlot)

CmdSketcherCompCreateSlot::CmdSketcherCompCreateSlot()
    : Command("Sketcher_CompCreateSlot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Create slot");
    sToolTipText = QT_TR_NOOP("Create a slot in the sketcher");
    sWhatsThis = "Sketcher_CompCreateSlot";
    sStatusTip = sToolTipText;
    sAccel = "G, S, S";
    eType = ForEdit;
}

void CmdSketcherCompCreateSlot::activated(int iMsg)
{
    switch (iMsg) {
    case 0:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerSlot()); break;
    case 1:
        ActivateHandler(getActiveGuiDocument(), new DrawSketchHandlerArcSlot()); break;
    default:
        return;
    }

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompCreateSlot::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* slot = pcAction->addAction(QString());
    slot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot"));
    QAction* arcSlot = pcAction->addAction(QString());
    arcSlot->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(slot->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompCreateSlot::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
    case Normal:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot"));
        getAction()->setIcon(a[index]->icon());
        break;
    case Construction:
        a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateSlot_Constr"));
        a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_CreateArcSlot_Constr"));
        getAction()->setIcon(a[index]->icon());
        break;
    }
}

void CmdSketcherCompCreateSlot::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* twoPointsSlot = a[0];
    twoPointsSlot->setText(QApplication::translate("CmdSketcherCompCreateSlot", "Slot"));
    twoPointsSlot->setToolTip(QApplication::translate("Sketcher_CreateTriangle", "Create a slot by its two center points and radius point"));
    twoPointsSlot->setStatusTip(QApplication::translate("Sketcher_CreateTriangle", "Create a slot by its two center points and radius point"));
    QAction* arcSlot = a[1];
    arcSlot->setText(QApplication::translate("CmdSketcherCompCreateSlot", "Arc slot"));
    arcSlot->setToolTip(QApplication::translate("Sketcher_CreateSquare", "Create a slot by its arc center first"));
    arcSlot->setStatusTip(QApplication::translate("Sketcher_CreateSquare", "Create a slot by its arc center first"));
}

bool CmdSketcherCompCreateSlot::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

/*=========================================================================*/

void CreateSketcherCommandsCreateGeo(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherCreatePoint());
    rcCmdMgr.addCommand(new CmdSketcherCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateArc());
    rcCmdMgr.addCommand(new CmdSketcherCreateCircle());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateCircle());
    rcCmdMgr.addCommand(new CmdSketcherCreateEllipseByCenter());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfEllipse());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfHyperbola());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcOfParabola());
    rcCmdMgr.addCommand(new CmdSketcherCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreatePeriodicBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateBSpline());
    rcCmdMgr.addCommand(new CmdSketcherCreateLine());
    rcCmdMgr.addCommand(new CmdSketcherCreatePolyline());
    rcCmdMgr.addCommand(new CmdSketcherCreateRectangle());
    rcCmdMgr.addCommand(new CmdSketcherCreatePolygon());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateRectangles());
    rcCmdMgr.addCommand(new CmdSketcherCompCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateArcSlot());
    rcCmdMgr.addCommand(new CmdSketcherCreateFillet());
    //rcCmdMgr.addCommand(new CmdSketcherCreateText());
    //rcCmdMgr.addCommand(new CmdSketcherCreateDraftLine());
    rcCmdMgr.addCommand(new CmdSketcherCompModifyEdge());
    rcCmdMgr.addCommand(new CmdSketcherTrimming());
    rcCmdMgr.addCommand(new CmdSketcherExtend());
    rcCmdMgr.addCommand(new CmdSketcherSplit());
    rcCmdMgr.addCommand(new CmdSketcherInsert());
    rcCmdMgr.addCommand(new CmdSketcherExternal());
    rcCmdMgr.addCommand(new CmdSketcherCarbonCopy());
}
