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


#ifndef SKETCHERGUI_DrawSketchHandlerExternal_H
#define SKETCHERGUI_DrawSketchHandlerExternal_H


#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp

class ExternalSelection : public Gui::SelectionFilterGate
{
    App::DocumentObject* object;
public:
    ExternalSelection(App::DocumentObject* obj)
        : Gui::SelectionFilterGate(nullPointer()), object(obj)
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

class DrawSketchHandlerExternal: public DrawSketchHandler
{
public:
    std::vector<int> attaching;
    bool defining;
    bool restorePickedList = false;

    DrawSketchHandlerExternal(bool defining=false)
        :attaching(0),defining(defining)
    {
    }

    DrawSketchHandlerExternal(std::vector<int> &&geoIds)
        :attaching(std::move(geoIds)),defining(false)
    {
    }

    virtual ~DrawSketchHandlerExternal()
    {
        Gui::Selection().rmvSelectionGate();
        if (restorePickedList)
            Gui::Selection().enablePickedList(false);
    }

    virtual bool allowExternalPick() const
    {
        return true;
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
            App::DocumentObject* obj = msg.Object.getObject();
            if (obj == nullptr)
                throw Base::ValueError("Sketcher: External geometry: Invalid object in selection");

            std::string subName = msg.Object.getOldElementName();
            if (obj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ||
                obj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId()) ||
                (subName.size() > 4 && subName.substr(0,4) == "Edge") ||
                (subName.size() > 6 && subName.substr(0,6) == "Vertex") ||
                (subName.size() > 4 && subName.substr(0,4) == "Face")) {
                try {
                    if(attaching.size()) {
                        Gui::Command::openCommand(
                                QT_TRANSLATE_NOOP("Command", "Attach external geometry"));
                        std::ostringstream ss;
                        ss << '[';
                        for(int geoId : attaching)
                            ss << geoId << ',';
                        ss << ']';
                        Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                "attachExternal(%s, %s)",
                                ss.str(), msg.Object.getSubObjectPython());
                    } else {
                        Gui::Command::openCommand(
                                QT_TRANSLATE_NOOP("Command", "Add external geometry"));
                        Gui::cmdAppObjectArgs(sketchgui->getObject(),
                                "addExternal(%s, %s)",
                                msg.Object.getSubObjectPython(), defining?"True":"False");
                    }

                    Gui::Selection().clearSelection();
                    if(attaching.size())
                        sketchgui->purgeHandler();

                    // adding external geometry does not require a solve() per se (the DoF is the same),
                    // however a solve is required to update the amount of solver geometry, because we only
                    // redraw a changed Sketch if the solver geometry amount is the same as the SkethObject
                    // geometry amount (as this avoids other issues).
                    // This solver is a very low cost one anyway (there is actually nothing to solve).
                    try {
                        tryAutoRecomputeIfNotSolve(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
                    } catch (Base::Exception &e) {
                        e.ReportException();
                    }
                    Gui::Command::commitCommand();
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
        if (!Gui::Selection().needPickedList()) {
            restorePickedList = true;
            Gui::Selection().enablePickedList(true);
        }
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

    virtual std::map<unsigned long, unsigned long> getCursorColorMap() const override
    {
        unsigned long color = 0;
        std::map<unsigned long, unsigned long> colorMapping;
        if (defining)
            color = 0xffccee;
        else if (attaching.size())
            color = 0x00ff00;
        if (color)
            colorMapping[0xd60000] = color;
        return colorMapping;
    }

    virtual QString getCrosshairCursorSVGName() const override {
        return QString::fromLatin1("Sketcher_Pointer_External");
    }

    virtual void deactivated() override
    {
        if (restorePickedList) {
            restorePickedList = false;
            Gui::Selection().enablePickedList(false);
        }
        setAxisPickStyle(true);
    }
};

} // namespace SketcherGui

#endif // SKETCHERGUI_DrawSketchHandlerExternal_H

