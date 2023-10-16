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

#ifndef SKETCHERGUI_DrawSketchHandlerSplitting_H
#define SKETCHERGUI_DrawSketchHandlerSplitting_H

#include <Gui/Notifications.h>

#include "GeometryCreationMode.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp


class SplittingSelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit SplittingSelection(App::DocumentObject* obj)
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
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                || geom->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geom->getTypeId() == Part::GeomEllipse::getClassTypeId()
                || geom->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())
                || geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                return true;
            }
        }
        else if (element.substr(0, 6) == "Vertex") {
            int VertId = std::atoi(element.substr(6, 4000).c_str()) - 1;
            int GeoId = Sketcher::GeoEnum::GeoUndef;
            Sketcher::PointPos PosId = Sketcher::PointPos::none;
            Sketcher::SketchObject* Sketch = static_cast<Sketcher::SketchObject*>(object);
            Sketch->getGeoVertexIndex(VertId, GeoId, PosId);
            if (isBsplineKnot(Sketch, GeoId)) {
                return true;
            }
        }
        return false;
    }
};


class DrawSketchHandlerSplitting: public DrawSketchHandler
{
public:
    DrawSketchHandlerSplitting() = default;
    ~DrawSketchHandlerSplitting() override
    {
        Gui::Selection().rmvSelectionGate();
    }

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
        int GeoId = Sketcher::GeoEnum::GeoUndef;

        int curveGeoId = getPreselectCurve();
        if (curveGeoId >= 0) {
            const Part::Geometry* geom = sketchgui->getSketchObject()->getGeometry(curveGeoId);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                || geom->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geom->getTypeId() == Part::GeomEllipse::getClassTypeId()
                || geom->isDerivedFrom(Part::GeomArcOfConic::getClassTypeId())
                || geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                GeoId = curveGeoId;
            }
        }
        else {
            // No curve of interest is pre-selected. Try pre-selected point.
            int pointGeoId = getPreselectPoint();

            if (pointGeoId >= 0) {
                // TODO: This has to be a knot. Find the spline.

                const auto& constraints = getSketchObject()->Constraints.getValues();
                const auto& conIt =
                    std::find_if(constraints.begin(), constraints.end(), [pointGeoId](auto constr) {
                        return (constr->Type == Sketcher::InternalAlignment
                                && constr->AlignmentType == Sketcher::BSplineKnotPoint
                                && constr->First == pointGeoId);
                    });

                if (conIt != constraints.end()) {
                    GeoId = (*conIt)->Second;
                }
            }
        }

        if (GeoId >= 0) {
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Split edge"));
                Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                      "split(%d,App.Vector(%f,%f,0))",
                                      GeoId,
                                      onSketchPos.x,
                                      onSketchPos.y);
                Gui::Command::commitCommand();
                tryAutoRecompute(static_cast<Sketcher::SketchObject*>(sketchgui->getObject()));
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(sketchgui,
                                 QT_TRANSLATE_NOOP("Notifications", "Error"),
                                 QT_TRANSLATE_NOOP("Notifications", "Failed to add edge"));

                Gui::Command::abortCommand();
            }
        }
        else {
            sketchgui->purgeHandler();
        }

        return true;
    }

private:
    void activated() override
    {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new SplittingSelection(sketchgui->getObject()));
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_Splitting");
    }
};

}  // namespace SketcherGui


#endif  // SKETCHERGUI_DrawSketchHandlerSplitting_H
