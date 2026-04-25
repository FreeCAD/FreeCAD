// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once

#include <QApplication>
#include <Base/Tools.h>

#include <Gui/Notifications.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "GeometryCreationMode.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include "SnapManager.h"


namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class TrimmingSelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit TrimmingSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}

    bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
    {
        if (pObj != this->object) {
            return false;
        }
        if (Base::Tools::isNullOrEmpty(sSubName)) {
            return false;
        }
        std::string element(sSubName);
        if (element.substr(0, 4) == "Edge") {
            int GeoId = std::atoi(element.substr(4, 4000).c_str()) - 1;
            Sketcher::SketchObject* Sketch = static_cast<Sketcher::SketchObject*>(object);
            const Part::Geometry* geom = Sketch->getGeometry(GeoId);
            if (geom->isDerivedFrom<Part::GeomTrimmedCurve>() || geom->is<Part::GeomCircle>()
                || geom->is<Part::GeomEllipse>() || geom->is<Part::GeomBSplineCurve>()) {
                // We do not trim internal geometry of complex geometries
                if (Sketcher::GeometryFacade::isInternalType(geom, Sketcher::InternalType::None)) {
                    return true;
                }
            }
        }
        return false;
    }
};

class DrawSketchHandlerTrimming: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerTrimming)

public:
    DrawSketchHandlerTrimming() = default;
    ~DrawSketchHandlerTrimming() override
    {
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        Base::Vector2d onSketchPos = snapHandle.compute();
        if (mousePressed) {
            executeCommands(onSketchPos);
            return;
        }

        int GeoId = getPreselectCurve();

        if (GeoId < 0) {
            EditMarkers.resize(0);
            drawEditMarkers(EditMarkers, 2);
        }

        auto sk = sketchgui->getObject<Sketcher::SketchObject>();
        int GeoId1, GeoId2;
        Base::Vector3d intersect1, intersect2;
        if (!sk->seekTrimPoints(
                GeoId,
                Base::Vector3d(onSketchPos.x, onSketchPos.y, 0),
                GeoId1,
                intersect1,
                GeoId2,
                intersect2
            )) {
            return;
        }

        EditMarkers.resize(0);

        if (GeoId1 != Sketcher::GeoEnum::GeoUndef) {
            EditMarkers.emplace_back(intersect1.x, intersect1.y);
        }
        else {
            auto start = sk->getPoint(GeoId, Sketcher::PointPos::start);
            EditMarkers.emplace_back(start.x, start.y);
        }

        if (GeoId2 != Sketcher::GeoEnum::GeoUndef) {
            EditMarkers.emplace_back(intersect2.x, intersect2.y);
        }
        else {
            auto end = sk->getPoint(GeoId, Sketcher::PointPos::end);
            EditMarkers.emplace_back(end.x, end.y);
        }

        // maker augmented by two sizes (see supported marker sizes)
        drawEditMarkers(EditMarkers, 2);
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        mousePressed = true;

        EditMarkers.resize(0);
        drawEditMarkers(EditMarkers);

        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        mousePressed = false;

        executeCommands(onSketchPos);

        return true;
    }

    void executeCommands(Base::Vector2d onSketchPos)
    {
        int GeoId = getPreselectCurve();
        if (GeoId < 0) {
            return;
        }

        // FIXME: Attempt to avoid double trimming. This messes up the cursor.
        // Possibly `mouseMove` gets triggered after first trim, but before preselection,
        // resulting in another edge being deleted.
        Gui::Selection().rmvPreselect();

        const Part::Geometry* geo = sketchgui->getSketchObject()->getGeometry(GeoId);
        if (geo->isDerivedFrom<Part::GeomTrimmedCurve>() || geo->is<Part::GeomCircle>()
            || geo->is<Part::GeomEllipse>() || geo->is<Part::GeomBSplineCurve>()) {
            try {
                Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Trim edge"));
                Gui::cmdAppObjectArgs(
                    sketchgui->getObject(),
                    "trim(%d,App.Vector(%f,%f,0))",
                    GeoId,
                    onSketchPos.x,
                    onSketchPos.y
                );
                Gui::Command::commitCommand();
                tryAutoRecompute(sketchgui->getObject<Sketcher::SketchObject>());
            }
            catch (const Base::Exception&) {
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Failed to trim edge")
                );

                Gui::Command::abortCommand();
            }
        }
    }

private:
    void activated() override
    {
        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new TrimmingSelection(sketchgui->getObject()));
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Trimming");
    }

private:
    std::vector<Base::Vector2d> EditMarkers;
    bool mousePressed = false;

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        return {
            {tr("%1 pick edge to trim", "Sketcher Trimming: hint"),
             {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
};

}  // namespace SketcherGui
