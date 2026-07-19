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

#include <cstring>
#include <string>
#include <App/Datums.h>
#include <App/IndexedName.h>
#include <Base/Tools.h>
#include <Mod/Part/App/DatumFeature.h>

#include <Gui/Notifications.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/MDIView.h>

#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"
#include <Mod/Part/App/Datums.h>
#include "SnapManager.h"


namespace SketcherGui
{

class ExternalSelection: public Gui::SelectionFilterGate
{
    App::DocumentObject* object;

public:
    explicit ExternalSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer())
        , object(obj)
    {}

    bool allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName) override
    {
        Sketcher::SketchObject* sketch = static_cast<Sketcher::SketchObject*>(object);

        this->notAllowedReason = "";
        Sketcher::SketchObject::eReasonList msg;
        if (!sketch->isExternalAllowed(pDoc, pObj, &msg)) {
            switch (msg) {
                case Sketcher::SketchObject::rlCircularReference:
                    this->notAllowedReason = QT_TR_NOOP("Linking this will cause circular dependency.");
                    break;
                case Sketcher::SketchObject::rlOtherDoc:
                    this->notAllowedReason = QT_TR_NOOP("This object is in another document.");
                    break;
                case Sketcher::SketchObject::rlOtherBody:
                    this->notAllowedReason = QT_TR_NOOP(
                        "This object belongs to another body, can't link."
                    );
                    break;
                case Sketcher::SketchObject::rlOtherPart:
                    this->notAllowedReason = QT_TR_NOOP(
                        "This object belongs to another part, can't link."
                    );
                    break;
                default:
                    break;
            }
            return false;
        }

        // Note: its better to search the support of the sketch in case the sketch support is a base
        // plane
        // Part::BodyBase* body = Part::BodyBase::findBodyOf(sketch);
        // if ( body && body->hasFeature ( pObj ) && body->isAfter ( pObj, sketch ) ) {
        // Don't allow selection after the sketch in the same body
        // NOTE: allowness of features in other bodies is handled by
        // SketchObject::isExternalAllowed()
        // TODO may be this should be in SketchObject::isExternalAllowed() (2015-08-07, Fat-Zer)
        // return false;
        //}

        if (pObj->isDerivedFrom<Part::DatumLine>() || pObj->isDerivedFrom<Part::DatumPoint>()
            || pObj->isDerivedFrom<App::Line>() || pObj->isDerivedFrom<App::Point>()) {
            return true;
        }

        if (pObj->isDerivedFrom<App::Plane>() || pObj->isDerivedFrom<Part::Datum>()) {
            return true;
        }

        if (Base::Tools::isNullOrEmpty(sSubName)) {
            return false;
        }

        const Data::IndexedName element(sSubName);
        if (element && element.getIndex() > 0
            && (std::strcmp(element.getType(), "Edge") == 0
                || std::strcmp(element.getType(), "Vertex") == 0
                || std::strcmp(element.getType(), "Face") == 0)) {
            return true;
        }

        return false;
    }
};

class DrawSketchHandlerExternal: public DrawSketchHandler
{
    Q_DECLARE_TR_FUNCTIONS(SketcherGui::DrawSketchHandlerExternal)

public:
    DrawSketchHandlerExternal(bool alwaysReference, bool intersection)
        : alwaysReference {alwaysReference}
        , intersection {intersection}
    {}
    ~DrawSketchHandlerExternal() override
    {
        resumeLazyExternalLayer();
        Gui::Selection().rmvSelectionGate();
    }

    void mouseMove(SnapManager::SnapHandle snapHandle) override
    {
        Q_UNUSED(snapHandle);
        if (Gui::Selection().getPreselection().pObjectName) {
            applyCursor();
        }
    }

    bool pressButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);
        /* this is ok not to call to purgeHandler
         * in continuous creation mode because the
         * handler is destroyed by the quit() method on pressing the
         * right button of the mouse */
        return true;
    }

    bool onSelectionChanged(const Gui::SelectionChanges& msg) override
    {
        if (msg.Type == Gui::SelectionChanges::AddSelection) {
            App::DocumentObject* obj = sketchgui->getObject()->getDocument()->getObject(
                msg.pObjectName
            );
            if (!obj) {
                throw Base::ValueError("Sketcher: External geometry: Invalid object in selection");
            }
            const Data::IndexedName subName(msg.pSubName);
            const bool hasValidIndex = subName && subName.getIndex() > 0;
            const bool isExternalEdge = hasValidIndex && std::strcmp(subName.getType(), "Edge") == 0;
            const bool isExternalVertex = hasValidIndex
                && std::strcmp(subName.getType(), "Vertex") == 0;
            const bool isExternalFace = hasValidIndex && std::strcmp(subName.getType(), "Face") == 0;

            if (obj->isDerivedFrom<App::Plane>() || obj->isDerivedFrom<Part::Datum>()
                || obj->isDerivedFrom<Part::DatumLine>() || obj->isDerivedFrom<Part::DatumPoint>()
                || obj->isDerivedFrom<App::Line>() || obj->isDerivedFrom<App::Point>()
                || isExternalEdge || isExternalVertex || isExternalFace) {
                try {
                    openCommand(QT_TRANSLATE_NOOP("Command", "Add external geometry"));
                    Gui::cmdAppObjectArgs(
                        sketchgui->getObject(),
                        "addExternal(\"%s\",\"%s\", %s, %s)",
                        msg.pObjectName,
                        msg.pSubName,
                        alwaysReference || isConstructionMode() ? "False" : "True",
                        intersection ? "True" : "False"
                    );

                    commitCommand();

                    // adding external geometry does not require a solve() per se (the DoF is the
                    // same), however a solve is required to update the amount of solver geometry,
                    // because we only redraw a changed Sketch if the solver geometry amount is the
                    // same as the SkethObject geometry amount (as this avoids other issues). This
                    // solver is a very low cost one anyway (there is actually nothing to solve).
                    tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());

                    Gui::Selection().clearSelection();
                    /* this is ok not to call to purgeHandler
                     * in continuous creation mode because the
                     * handler is destroyed by the quit() method on pressing the
                     * right button of the mouse */
                }
                catch (const Base::Exception&) {
                    Gui::NotifyError(
                        sketchgui,
                        QT_TRANSLATE_NOOP("Notifications", "Error"),
                        QT_TRANSLATE_NOOP("Notifications", "Failed to add external geometry")
                    );
                    Gui::Selection().clearSelection();
                    abortCommand();
                }
                return true;
            }
        }
        updateHint();

        return false;
    }

private:
    void activated() override
    {
        if (materializeSelectedLazyExternalReferences()) {
            sketchgui->purgeHandler();
            return;
        }

        suspendLazyExternalLayer();

        setAxisPickStyle(false);
        Gui::MDIView* mdi = Gui::Application::Instance->activeDocument()->getActiveView();
        Gui::View3DInventorViewer* viewer;
        viewer = static_cast<Gui::View3DInventor*>(mdi)->getViewer();
        viewer->setSelectionEnabled(true);

        Gui::Selection().clearSelection();
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new ExternalSelection(sketchgui->getObject()));
    }

    QString getCrosshairCursorSVGName() const override
    {
        if (intersection) {
            return QStringLiteral("Sketcher_Pointer_External_Intersection");
        }

        return QStringLiteral("Sketcher_Pointer_External");
    }

    void deactivated() override
    {
        resumeLazyExternalLayer();
        setAxisPickStyle(true);
    }

    bool allowLazyExternalPreselection() const override
    {
        return false;
    }

    bool materializeSelectedLazyExternalReferences()
    {
        if (intersection || !sketchgui) {
            return false;
        }

        const auto lazyExternalIds = getSelectedLazyExternalReferenceIds();
        if (lazyExternalIds.empty()) {
            return false;
        }

        try {
            openCommand(QT_TRANSLATE_NOOP("Command", "Add external geometry"));

            bool materializationFailed = false;
            for (int lazyExternalId : lazyExternalIds) {
                std::string sourceObjectName;
                std::string sourceSubName;
                bool sourceIntersection = false;
                bool sourceVertex = false;
                if (!getLazyExternalSourceReference(
                        lazyExternalId,
                        sourceObjectName,
                        sourceSubName,
                        sourceIntersection,
                        sourceVertex
                    )) {
                    materializationFailed = true;
                    break;
                }

                const int geoId = materializeLazyExternalSourceReference(
                    sourceObjectName,
                    sourceSubName,
                    sourceIntersection,
                    !(alwaysReference || isConstructionMode())
                );
                if (geoId == Sketcher::GeoEnum::GeoUndef) {
                    materializationFailed = true;
                    break;
                }
            }

            if (materializationFailed) {
                abortCommand();
                clearSelectedLazyExternalReferences();
                Gui::Selection().clearSelection();
                Gui::NotifyError(
                    sketchgui,
                    QT_TRANSLATE_NOOP("Notifications", "Error"),
                    QT_TRANSLATE_NOOP("Notifications", "Failed to add external geometry")
                );
                return true;
            }

            commitCommand();
            tryAutoRecomputeIfNotSolve(sketchgui->getObject<Sketcher::SketchObject>());
            clearSelectedLazyExternalReferences();
            Gui::Selection().clearSelection();
        }
        catch (const Base::Exception&) {
            Gui::NotifyError(
                sketchgui,
                QT_TRANSLATE_NOOP("Notifications", "Error"),
                QT_TRANSLATE_NOOP("Notifications", "Failed to add external geometry")
            );
            clearSelectedLazyExternalReferences();
            Gui::Selection().clearSelection();
            abortCommand();
        }

        return true;
    }

    void suspendLazyExternalLayer()
    {
        if (!lazyLayerSuspended && sketchgui) {
            suspendLazyExternalGeometryLayer();
            lazyLayerSuspended = true;
        }
    }

    void resumeLazyExternalLayer()
    {
        if (lazyLayerSuspended && sketchgui) {
            resumeLazyExternalGeometryLayer();
            lazyLayerSuspended = false;
        }
    }

    bool alwaysReference;
    bool intersection;
    bool lazyLayerSuspended = false;

public:
    std::list<Gui::InputHint> getToolHints() const override
    {
        return {
            {tr("%1 pick external geometry", "Sketcher External: hint"),
             {Gui::InputHint::UserInput::MouseLeft}},
        };
    }
};

}  // namespace SketcherGui
