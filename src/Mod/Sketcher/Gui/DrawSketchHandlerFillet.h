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

#include <Gui/Notifications.h>
#include <Gui/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"

using namespace Sketcher;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class FilletSelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit FilletSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}

    bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
    {
        if (pObj != this->object) {
            return false;
        }
        if (!sSubName || sSubName[0] == '\0') {
            return false;
        }
        std::string element(sSubName);
        if (element.substr(0, 4) == "Edge") {
            int GeoId = std::atoi(element.substr(4, 4000).c_str()) - 1;
            Sketcher::SketchObject* Sketch = static_cast<Sketcher::SketchObject*>(object);
            const Part::Geometry* geom = Sketch->getGeometry(GeoId);
            if (geom->isDerivedFrom<Part::GeomBoundedCurve>()) {
                return true;
            }
        }
        if (element.substr(0, 6) == "Vertex") {
            int VtId = std::atoi(element.substr(6, 4000).c_str()) - 1;
            Sketcher::SketchObject* Sketch = static_cast<Sketcher::SketchObject*>(object);
            std::vector<int> GeoIdList;
            std::vector<Sketcher::PointPos> PosIdList;
            Sketch->getDirectlyCoincidentPoints(VtId, GeoIdList, PosIdList);
            if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
                const Part::Geometry* geom1 = Sketch->getGeometry(GeoIdList[0]);
                const Part::Geometry* geom2 = Sketch->getGeometry(GeoIdList[1]);
                if (geom1->is<Part::GeomLineSegment>() && geom2->is<Part::GeomLineSegment>()) {
                    return true;
                }
            }
        }
        return false;
    }
};


class DrawSketchHandlerFillet;

namespace ConstructionMethods
{

enum class FilletConstructionMethod
{
    Fillet,
    Chamfer,
    End  // Must be the last one
};

}

using DSHFilletController =
    DrawSketchDefaultWidgetController<DrawSketchHandlerFillet,
                                      StateMachines::TwoSeekEnd,
                                      /*PAutoConstraintSize =*/0,
                                      /*OnViewParametersT =*/OnViewParameters<0, 0>,  // NOLINT
                                      /*WidgetParametersT =*/WidgetParameters<0, 0>,  // NOLINT
                                      /*WidgetCheckboxesT =*/WidgetCheckboxes<1, 1>,  // NOLINT
                                      /*WidgetComboboxesT =*/WidgetComboboxes<1, 1>,  // NOLINT
                                      ConstructionMethods::FilletConstructionMethod,
                                      /*bool PFirstComboboxIsConstructionMethod =*/true>;

using DSHFilletControllerBase = DSHFilletController::ControllerBase;

using DrawSketchHandlerFilletBase = DrawSketchControllableHandler<DSHFilletController>;

class DrawSketchHandlerFillet: public DrawSketchHandlerFilletBase
{
    friend DSHFilletController;
    friend DSHFilletControllerBase;

public:
    explicit DrawSketchHandlerFillet(ConstructionMethod constrMethod = ConstructionMethod::Fillet)
        : DrawSketchHandlerFilletBase(constrMethod)
        , preserveCorner(true)
        , vtId(-1)
        , geoId1(GeoEnum::GeoUndef)
        , geoId2(GeoEnum::GeoUndef)
    {}
    ~DrawSketchHandlerFillet() override
    {
        Gui::Selection().rmvSelectionGate();
    }

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        switch (state()) {
            case SelectMode::SeekFirst: {
                vtId = getPreselectPoint();
                geoId1 = getPreselectCurve();
                firstPos = onSketchPos;
            } break;
            case SelectMode::SeekSecond: {
                geoId2 = getPreselectCurve();
                secondPos = onSketchPos;
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        SketchObject* obj = sketchgui->getSketchObject();

        bool construction = false;
        bool isChamfer = constructionMethod() == ConstructionMethod::Chamfer;

        if (vtId != -1) {
            int GeoId;
            PointPos PosId = PointPos::none;
            obj->getGeoVertexIndex(vtId, GeoId, PosId);
            const Part::Geometry* geom = obj->getGeometry(GeoId);
            if (isLineSegment(*geom) && (PosId == PointPos::start || PosId == PointPos::end)) {

                // guess fillet radius
                double radius = -1;
                std::vector<int> GeoIdList;
                std::vector<Sketcher::PointPos> PosIdList;
                obj->getDirectlyCoincidentPoints(GeoId, PosId, GeoIdList, PosIdList);
                if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
                    const Part::Geometry* geo1 = obj->getGeometry(GeoIdList[0]);
                    const Part::Geometry* geo2 = obj->getGeometry(GeoIdList[1]);

                    construction = GeometryFacade::getConstruction(geo1)
                        && GeometryFacade::getConstruction(geo2);

                    if (isLineSegment(*geo1) && isLineSegment(*geo2)) {
                        auto* line1 = static_cast<const Part::GeomLineSegment*>(geo1);
                        auto* line2 = static_cast<const Part::GeomLineSegment*>(geo2);
                        Base::Vector3d dir1 = line1->getEndPoint() - line1->getStartPoint();
                        Base::Vector3d dir2 = line2->getEndPoint() - line2->getStartPoint();
                        if (PosIdList[0] == PointPos::end) {
                            dir1 *= -1;
                        }
                        if (PosIdList[1] == PointPos::end) {
                            dir2 *= -1;
                        }
                        double l1 = dir1.Length();
                        double l2 = dir2.Length();
                        double angle = dir1.GetAngle(dir2);
                        radius = (l1 < l2 ? l1 : l2) * 0.2 * sin(angle / 2);
                    }
                }
                if (radius < 0) {
                    return;
                }

                int filletGeoId = getHighestCurveIndex() + (isChamfer ? 2 : 1);
                // create fillet at point
                try {
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                    Gui::cmdAppObjectArgs(obj,
                                          "fillet(%d,%d,%f,%s,%s,%s)",
                                          GeoId,
                                          static_cast<int>(PosId),
                                          radius,
                                          "True",
                                          preserveCorner ? "True" : "False",
                                          isChamfer ? "True" : "False");

                    if (construction) {
                        Gui::cmdAppObjectArgs(obj, "toggleConstruction(%d) ", filletGeoId);
                    }

                    Gui::Command::commitCommand();
                }
                catch (const Base::Exception& e) {
                    Gui::NotifyUserError(
                        obj,
                        QT_TRANSLATE_NOOP("Notifications", "Failed to create fillet"),
                        e.what());
                    Gui::Command::abortCommand();
                }

                tryAutoRecomputeIfNotSolve(obj);
            }
        }

        else {
            Base::Vector3d refPnt1(firstPos.x, firstPos.y, 0.f);
            Base::Vector3d refPnt2(secondPos.x, secondPos.y, 0.f);

            const Part::Geometry* geo1 = obj->getGeometry(geoId1);
            const Part::Geometry* geo2 = obj->getGeometry(geoId2);

            construction =
                GeometryFacade::getConstruction(geo1) && GeometryFacade::getConstruction(geo2);

            double radius = 0;

            if (isLineSegment(*geo1) && isLineSegment(*geo2)) {
                // guess fillet radius
                auto* line1 = static_cast<const Part::GeomLineSegment*>(geo1);
                auto* line2 = static_cast<const Part::GeomLineSegment*>(geo2);

                radius = Part::suggestFilletRadius(line1, line2, refPnt1, refPnt2);
                if (radius < 0) {
                    return;
                }
            }

            int filletGeoId = getHighestCurveIndex() + (isChamfer ? 2 : 1);

            // create fillet between lines
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                Gui::cmdAppObjectArgs(
                    obj,
                    "fillet(%d,%d,App.Vector(%f,%f,0),App.Vector(%f,%f,0),%f,%s,%s,%s)",
                    geoId1,
                    geoId2,
                    firstPos.x,
                    firstPos.y,
                    secondPos.x,
                    secondPos.y,
                    radius,
                    "True",
                    preserveCorner ? "True" : "False",
                    isChamfer ? "True" : "False");
                Gui::Command::commitCommand();
            }
            catch (const Base::CADKernelError& e) {
                if (e.getTranslatable()) {
                    Gui::TranslatedUserError(sketchgui,
                                             QObject::tr("CAD Kernel Error"),
                                             QObject::tr(e.getMessage().c_str()));
                }
                Gui::Selection().clearSelection();
                Gui::Command::abortCommand();
            }
            catch (const Base::ValueError& e) {
                Gui::TranslatedUserError(sketchgui,
                                         QObject::tr("Value Error"),
                                         QObject::tr(e.getMessage().c_str()));
                Gui::Selection().clearSelection();
                Gui::Command::abortCommand();
            }

            tryAutoRecompute(obj);

            if (construction) {
                Gui::cmdAppObjectArgs(obj, "toggleConstruction(%d) ", filletGeoId);
            }


            Gui::Selection().clearSelection();
        }
    }

    std::string getToolName() const override
    {
        return "DSH_Fillet";
    }

    QString getCrosshairCursorSVGName() const override
    {
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new FilletSelection(sketchgui->getObject()));

        if (constructionMethod() == DrawSketchHandlerFillet::ConstructionMethod::Fillet) {
            if (preserveCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Create_PointFillet");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Create_Fillet");
            }
        }
        else {
            if (preserveCorner) {
                return QString::fromLatin1("Sketcher_Pointer_Create_PointChamfer");
            }
            else {
                return QString::fromLatin1("Sketcher_Pointer_Create_Chamfer");
            }
        }
    }

    std::unique_ptr<QWidget> createWidget() const override
    {
        return std::make_unique<SketcherToolDefaultWidget>();
    }

    bool isWidgetVisible() const override
    {
        return true;
    };

    QPixmap getToolIcon() const override
    {
        return Gui::BitmapFactory().pixmap("Sketcher_CreateFillet");
    }

    QString getToolWidgetText() const override
    {
        return QString(QObject::tr("Fillet/Chamfer parameters"));
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekFirst) {
            if (vtId != -1) {
                return true;
            }
            if (geoId1 >= 0) {
                const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId1);
                if (geo->isDerivedFrom<Part::GeomBoundedCurve>()) {
                    return true;
                }
            }
        }

        if (state() == SelectMode::SeekSecond) {
            if (geoId2 >= 0) {
                const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(geoId2);
                if (geo->isDerivedFrom<Part::GeomBoundedCurve>()) {
                    return true;
                }
            }
        }

        return false;
    }

    void onButtonPressed(Base::Vector2d onSketchPos) override
    {
        this->updateDataAndDrawToPosition(onSketchPos);
        if (canGoToNextMode()) {
            if (state() == SelectMode::SeekFirst) {
                if (vtId != -1) {
                    setState(SelectMode::End);
                }
                else {
                    // add the edge to the selection
                    std::stringstream ss;
                    ss << "Edge" << geoId1 + 1;
                    Gui::Selection().addSelection(
                        sketchgui->getSketchObject()->getDocument()->getName(),
                        sketchgui->getSketchObject()->getNameInDocument(),
                        ss.str().c_str(),
                        onSketchPos.x,
                        onSketchPos.y,
                        0.f);
                    moveToNextMode();
                }
            }
            else {
                moveToNextMode();
            }
        }
    }


private:
    bool preserveCorner;
    int vtId, geoId1, geoId2;
    Base::Vector2d firstPos, secondPos;
};

template<>
void DSHFilletController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        QStringList names = {QStringLiteral("Fillet"), QStringLiteral("Chamfer")};
        toolWidget->setComboboxElements(WCombobox::FirstCombo, names);

        toolWidget->setComboboxItemIcon(
            WCombobox::FirstCombo,
            0,
            Gui::BitmapFactory().iconFromTheme("Sketcher_CreateFillet"));
        toolWidget->setComboboxItemIcon(
            WCombobox::FirstCombo,
            1,
            Gui::BitmapFactory().iconFromTheme("Sketcher_CreateChamfer"));

        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_fillet", "Preserve corner (U)"));
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_fillet",
                                    "Preserves intersection point and most constraints"));

        toolWidget->setCheckboxIcon(
            WCheckbox::FirstBox,
            Gui::BitmapFactory().iconFromTheme("Sketcher_CreatePointFillet"));
    }
    syncCheckboxToHandler(WCheckbox::FirstBox, handler->preserveCorner);
}

template<>
void DSHFilletController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    Q_UNUSED(checkboxindex);

    switch (checkboxindex) {
        case WCheckbox::FirstBox:
            handler->preserveCorner = value;
            break;
    }

    handler->updateCursor();
}

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerFillet_H
