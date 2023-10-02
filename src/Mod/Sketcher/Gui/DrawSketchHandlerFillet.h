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

#include "GeometryCreationMode.h"


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
            if (geom->getTypeId().isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId())) {
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
                if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                    && geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                    return true;
                }
            }
        }
        return false;
    }
};


class DrawSketchHandlerFillet: public DrawSketchHandler
{
public:
    enum FilletType
    {
        SimpleFillet,
        ConstraintPreservingFillet
    };

    explicit DrawSketchHandlerFillet(FilletType filletType)
        : filletType(filletType)
        , Mode(STATUS_SEEK_First)
        , firstCurve(0)
    {}
    ~DrawSketchHandlerFillet() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    enum SelectMode
    {
        STATUS_SEEK_First,
        STATUS_SEEK_Second
    };

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        bool construction = false;
        int VtId = getPreselectPoint();
        if (Mode == STATUS_SEEK_First && VtId != -1) {
            int GeoId;
            Sketcher::PointPos PosId = Sketcher::PointPos::none;
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId, GeoId, PosId);
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                && (PosId == Sketcher::PointPos::start || PosId == Sketcher::PointPos::end)) {

                // guess fillet radius
                double radius = -1;
                std::vector<int> GeoIdList;
                std::vector<Sketcher::PointPos> PosIdList;
                sketchgui->getSketchObject()->getDirectlyCoincidentPoints(GeoId,
                                                                          PosId,
                                                                          GeoIdList,
                                                                          PosIdList);
                if (GeoIdList.size() == 2 && GeoIdList[0] >= 0 && GeoIdList[1] >= 0) {
                    const Part::Geometry* geom1 =
                        sketchgui->getSketchObject()->getGeometry(GeoIdList[0]);
                    const Part::Geometry* geom2 =
                        sketchgui->getSketchObject()->getGeometry(GeoIdList[1]);
                    construction = Sketcher::GeometryFacade::getConstruction(geom1)
                        && Sketcher::GeometryFacade::getConstruction(geom2);
                    if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                        && geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        const Part::GeomLineSegment* lineSeg1 =
                            static_cast<const Part::GeomLineSegment*>(geom1);
                        const Part::GeomLineSegment* lineSeg2 =
                            static_cast<const Part::GeomLineSegment*>(geom2);
                        Base::Vector3d dir1 = lineSeg1->getEndPoint() - lineSeg1->getStartPoint();
                        Base::Vector3d dir2 = lineSeg2->getEndPoint() - lineSeg2->getStartPoint();
                        if (PosIdList[0] == Sketcher::PointPos::end) {
                            dir1 *= -1;
                        }
                        if (PosIdList[1] == Sketcher::PointPos::end) {
                            dir2 *= -1;
                        }
                        double l1 = dir1.Length();
                        double l2 = dir2.Length();
                        double angle = dir1.GetAngle(dir2);
                        radius = (l1 < l2 ? l1 : l2) * 0.2 * sin(angle / 2);
                    }
                }
                if (radius < 0) {
                    return false;
                }

                int currentgeoid = getHighestCurveIndex();
                // create fillet at point
                try {
                    bool pointFillet = (filletType == 1);
                    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                    Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                          "fillet(%d,%d,%f,%s,%s)",
                                          GeoId,
                                          static_cast<int>(PosId),
                                          radius,
                                          "True",
                                          pointFillet ? "True" : "False");

                    if (construction) {
                        Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                              "toggleConstruction(%d) ",
                                              currentgeoid + 1);
                    }

                    Gui::Command::commitCommand();
                }
                catch (const Base::Exception& e) {
                    Gui::NotifyUserError(
                        sketchgui->getObject(),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to create fillet"),
                        e.what());
                    Gui::Command::abortCommand();
                }

                tryAutoRecomputeIfNotSolve(
                    static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            return true;
        }

        int GeoId = getPreselectCurve();
        if (GeoId > -1) {
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(GeoId);
            if (geom->getTypeId().isDerivedFrom(Part::GeomBoundedCurve::getClassTypeId())) {
                if (Mode == STATUS_SEEK_First) {
                    firstCurve = GeoId;
                    firstPos = onSketchPos;
                    Mode = STATUS_SEEK_Second;
                    // add the line to the selection
                    std::stringstream ss;
                    ss << "Edge" << firstCurve + 1;
                    Gui::Selection().addSelection(
                        sketchgui->getSketchObject()->getDocument()->getName(),
                        sketchgui->getSketchObject()->getNameInDocument(),
                        ss.str().c_str(),
                        onSketchPos.x,
                        onSketchPos.y,
                        0.f);
                }
                else if (Mode == STATUS_SEEK_Second) {
                    int secondCurve = GeoId;
                    Base::Vector2d secondPos = onSketchPos;

                    Base::Vector3d refPnt1(firstPos.x, firstPos.y, 0.f);
                    Base::Vector3d refPnt2(secondPos.x, secondPos.y, 0.f);

                    const Part::Geometry* geom1 =
                        sketchgui->getSketchObject()->getGeometry(firstCurve);

                    double radius = 0;

                    if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                        && geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                        // guess fillet radius
                        const Part::GeomLineSegment* lineSeg1 =
                            static_cast<const Part::GeomLineSegment*>(
                                sketchgui->getSketchObject()->getGeometry(firstCurve));
                        const Part::GeomLineSegment* lineSeg2 =
                            static_cast<const Part::GeomLineSegment*>(
                                sketchgui->getSketchObject()->getGeometry(secondCurve));

                        radius = Part::suggestFilletRadius(lineSeg1, lineSeg2, refPnt1, refPnt2);
                        if (radius < 0) {
                            return false;
                        }

                        construction = Sketcher::GeometryFacade::getConstruction(lineSeg1)
                            && Sketcher::GeometryFacade::getConstruction(lineSeg2);
                    }
                    else {  // other supported curves
                        const Part::Geometry* geo1 = static_cast<const Part::Geometry*>(
                            sketchgui->getSketchObject()->getGeometry(firstCurve));
                        const Part::Geometry* geo2 = static_cast<const Part::Geometry*>(
                            sketchgui->getSketchObject()->getGeometry(secondCurve));

                        construction = Sketcher::GeometryFacade::getConstruction(geo1)
                            && Sketcher::GeometryFacade::getConstruction(geo2);
                    }


                    int currentgeoid = getHighestCurveIndex();

                    // create fillet between lines
                    try {
                        bool pointFillet = (filletType == 1);
                        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Create fillet"));
                        Gui::cmdAppObjectArgs(
                            sketchgui->getObject(),
                            "fillet(%d,%d,App.Vector(%f,%f,0),App.Vector(%f,%f,0),%f,%s,%s)",
                            firstCurve,
                            secondCurve,
                            firstPos.x,
                            firstPos.y,
                            secondPos.x,
                            secondPos.y,
                            radius,
                            "True",
                            pointFillet ? "True" : "False");
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
                        Mode = STATUS_SEEK_First;
                    }
                    catch (const Base::ValueError& e) {
                        Gui::TranslatedUserError(sketchgui,
                                                 QObject::tr("Value Error"),
                                                 QObject::tr(e.getMessage().c_str()));
                        Gui::Selection().clearSelection();
                        Gui::Command::abortCommand();
                        Mode = STATUS_SEEK_First;
                    }

                    tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));

                    if (construction) {
                        Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                              "toggleConstruction(%d) ",
                                              currentgeoid + 1);
                    }


                    Gui::Selection().clearSelection();
                    Mode = STATUS_SEEK_First;
                }
            }
        }

        if (VtId < 0 && GeoId < 0) {  // exit the fillet tool if the user clicked on empty space
            sketchgui
                ->purgeHandler();  // no code after this line, Handler get deleted in ViewProvider
        }

        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new FilletSelection(sketchgui->getObject()));
        return QString::fromLatin1("Sketcher_Pointer_Create_Fillet");
    }

protected:
    int filletType;
    SelectMode Mode;
    int firstCurve;
    Base::Vector2d firstPos;
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerFillet_H
