// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2022 Boyer Pierre-Louis <pierrelouis.boyer@gmail.com>   *
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

#include <Gui/BitmapFactory.h>
#include <Gui/Notifications.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>

#include <Mod/Sketcher/App/GeometryFacade.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchDefaultWidgetController.h"
#include "DrawSketchControllableHandler.h"

#include "GeometryCreationMode.h"
#include "Utils.h"

using namespace Sketcher;

namespace SketcherGui
{

extern GeometryCreationMode geometryCreationMode;  // defined in CommandCreateGeo.cpp

class DrawSketchHandlerSymmetry;

using DSHSymmetryController = DrawSketchDefaultWidgetController<
    DrawSketchHandlerSymmetry,
    StateMachines::OneSeekEnd,
    /*PAutoConstraintSize =*/0,
    /*OnViewParametersT =*/OnViewParameters<0>,
    /*WidgetParametersT =*/WidgetParameters<0>,
    /*WidgetCheckboxesT =*/WidgetCheckboxes<2>,
    /*WidgetComboboxesT =*/WidgetComboboxes<0>>;

using DSHSymmetryControllerBase = DSHSymmetryController::ControllerBase;

using DrawSketchHandlerSymmetryBase = DrawSketchControllableHandler<DSHSymmetryController>;

class DrawSketchHandlerSymmetry: public DrawSketchHandlerSymmetryBase
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerSymmetry)

    friend DSHSymmetryController;
    friend DSHSymmetryControllerBase;

public:
    explicit DrawSketchHandlerSymmetry(std::vector<int> listOfGeoIds)
        : listOfGeoIds(listOfGeoIds)
        , refGeoId(Sketcher::GeoEnum::GeoUndef)
        , refPosId(Sketcher::PointPos::none)
        , deleteOriginal(false)
        , createSymConstraints(false)
    {}

    DrawSketchHandlerSymmetry(const DrawSketchHandlerSymmetry&) = delete;
    DrawSketchHandlerSymmetry(DrawSketchHandlerSymmetry&&) = delete;
    DrawSketchHandlerSymmetry& operator=(const DrawSketchHandlerSymmetry&) = delete;
    DrawSketchHandlerSymmetry& operator=(DrawSketchHandlerSymmetry&&) = delete;

    ~DrawSketchHandlerSymmetry() override = default;

private:
    void updateDataAndDrawToPosition(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        switch (state()) {
            case SelectMode::SeekFirst: {
                int VtId = getPreselectPoint();
                int CrvId = getPreselectCurve();
                int CrsId = getPreselectCross();
                Sketcher::SketchObject* obj = sketchgui->getSketchObject();

                if (VtId >= 0) {  // Vertex
                    SketchObject* Obj = sketchgui->getSketchObject();
                    Obj->getGeoVertexIndex(VtId, refGeoId, refPosId);
                }
                else if (CrsId == 0) {  // RootPoint
                    refGeoId = Sketcher::GeoEnum::RtPnt;
                    refPosId = Sketcher::PointPos::start;
                }
                else if (CrsId == 1) {  // H_Axis
                    refGeoId = Sketcher::GeoEnum::HAxis;
                    refPosId = Sketcher::PointPos::none;
                }
                else if (CrsId == 2) {  // V_Axis
                    refGeoId = Sketcher::GeoEnum::VAxis;
                    refPosId = Sketcher::PointPos::none;
                }
                else if ((CrvId >= 0 || CrvId <= Sketcher::GeoEnum::RefExt)
                         && isLineSegment(*obj->getGeometry(CrvId))) {  // Curves
                    refGeoId = CrvId;
                    refPosId = Sketcher::PointPos::none;
                }
                else {
                    refGeoId = Sketcher::GeoEnum::GeoUndef;
                    refPosId = Sketcher::PointPos::none;
                }


                CreateAndDrawShapeGeometry();
            } break;
            default:
                break;
        }
    }

    void executeCommands() override
    {
        try {
            Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Symmetry geometries"));

            SketchObject* Obj = sketchgui->getSketchObject();
            createSymConstraints = !deleteOriginal && createSymConstraints;
            Obj->addSymmetric(listOfGeoIds, refGeoId, refPosId, createSymConstraints);

            if (deleteOriginal) {
                deleteOriginalGeos();
            }
            tryAutoRecomputeIfNotSolve(Obj);

            Gui::Command::commitCommand();
        }
        catch (const Base::Exception& e) {
            e.reportException();
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to create symmetry")
            );

            Gui::Command::abortCommand();
            THROWM(
                Base::RuntimeError,
                QT_TRANSLATE_NOOP(
                    "Notifications",
                    "Tool execution aborted"
                ) "\n"
            )  // This prevents constraints from being
               // applied on non existing geometry
        }
    }

    void createAutoConstraints() override
    {
        // none
    }

    std::string getToolName() const override
    {
        return "DSH_Symmetry";
    }

    QString getCrosshairCursorSVGName() const override
    {
        return QStringLiteral("Sketcher_Pointer_Create_Symmetry");
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
        return Gui::BitmapFactory().pixmap("Sketcher_Symmetry");
    }

    QString getToolWidgetText() const override
    {
        return QString(tr("Symmetry Parameters"));
    }

    void activated() override
    {
        DrawSketchDefaultHandler::activated();
        continuousMode = false;
    }

    bool canGoToNextMode() override
    {
        if (state() == SelectMode::SeekFirst && refGeoId == Sketcher::GeoEnum::GeoUndef) {
            // Prevent validation if no reference selected.
            return false;
        }
        return true;
    }

private:
    std::vector<int> listOfGeoIds;
    int refGeoId;
    Sketcher::PointPos refPosId;
    bool deleteOriginal, createSymConstraints;

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        using enum Gui::InputHint::UserInput;

        return {
            {tr("%1 pick axis, edge, or point", "Sketcher Symmetry: hint"), {MouseLeft}},
        };
    }

    void deleteOriginalGeos()
    {
        std::stringstream stream;
        for (size_t j = 0; j < listOfGeoIds.size() - 1; j++) {
            stream << listOfGeoIds[j] << ",";
        }
        stream << listOfGeoIds[listOfGeoIds.size() - 1];
        try {
            Gui::cmdAppObjectArgs(sketchgui->getObject(), "delGeometries([%s])", stream.str().c_str());
        }
        catch (const Base::Exception& e) {
            Base::Console().error("%s\n", e.what());
        }
    }

    void createShape(bool onlyeditoutline) override
    {
        SketchObject* Obj = sketchgui->getSketchObject();

        ShapeGeometry.clear();

        if (refGeoId == Sketcher::GeoEnum::GeoUndef) {
            return;
        }

        if (onlyeditoutline) {
            std::map<int, int> dummy1;
            std::map<int, bool> dummy2;
            std::vector<Part::Geometry*> symGeos
                = Obj->getSymmetric(listOfGeoIds, dummy1, dummy2, refGeoId, refPosId);

            for (auto* geo : symGeos) {
                ShapeGeometry.emplace_back(geo);
            }
        }
    }
};

template<>
void DSHSymmetryController::configureToolWidget()
{
    if (!init) {  // Code to be executed only upon initialisation
        toolWidget->setCheckboxLabel(
            WCheckbox::FirstBox,
            QApplication::translate("TaskSketcherTool_c1_symmetry", "Delete original geometries (U)")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::FirstBox,
            QApplication::translate(
                "TaskSketcherTool_c1_symmetry",
                "Removes the original geometry and keeps only the mirrored result."

            )
        );
        toolWidget->setCheckboxLabel(
            WCheckbox::SecondBox,
            QApplication::translate("TaskSketcherTool_c2_symmetry", "Create symmetry constraints (J)")
        );
        toolWidget->setCheckboxToolTip(
            WCheckbox::SecondBox,
            QApplication::translate(
                "TaskSketcherTool_c2_symmetry",
                "Create symmetry constraints between the original and mirrored geometries"
            )
        );
    }
}

template<>
void DSHSymmetryController::adaptDrawingToCheckboxChange(int checkboxindex, bool value)
{
    switch (checkboxindex) {
        case WCheckbox::FirstBox: {
            handler->deleteOriginal = value;
            if (value && toolWidget->getCheckboxChecked(WCheckbox::SecondBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::SecondBox, false);
            }
        } break;
        case WCheckbox::SecondBox: {
            handler->createSymConstraints = value;
            if (value && toolWidget->getCheckboxChecked(WCheckbox::FirstBox)) {
                toolWidget->setCheckboxChecked(WCheckbox::FirstBox, false);
            }
        } break;
    }
}


}  // namespace SketcherGui
