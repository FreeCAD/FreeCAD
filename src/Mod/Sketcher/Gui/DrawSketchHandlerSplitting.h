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


#include "GeometryCreationMode.h"
#include "Utils.h"

namespace SketcherGui {

extern GeometryCreationMode geometryCreationMode; // defined in CommandCreateGeo.cpp


class SplittingSelection : public Gui::SelectionFilterGate
{
    App::DocumentObject* object;
public:
    SplittingSelection(App::DocumentObject* obj)
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
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()
                || geom->getTypeId() == Part::GeomCircle::getClassTypeId()
                || geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                return true;
            }
        }
        return  false;
    }
};


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

} // namespace SketcherGui


#endif // SKETCHERGUI_DrawSketchHandlerSplitting_H

