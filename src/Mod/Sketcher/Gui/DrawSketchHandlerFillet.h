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


#ifndef SKETCHERGUI_DrawSketchHandlerFillet_H
#define SKETCHERGUI_DrawSketchHandlerFillet_H

#include "DrawSketchDefaultWidgetHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp


class FilletSelection : public Gui::SelectionFilterGate
{
    App::DocumentObject* object;
public:
    FilletSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer()), object(obj)
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

class DrawSketchHandlerFillet;

namespace ConstructionMethods {

    enum class FilletChamferConstructionMethod {
        Fillet,
        Chamfer,
        End // Must be the last one
    };

    } // ConstructionMethods

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
            dHandler->nofAngles = std::max(2, abs(static_cast<int>(value)));
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

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerFillet_H

