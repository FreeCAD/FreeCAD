/***************************************************************************
 *   Copyright (c) 2010 Juergen Riegel <FreeCAD@juergen-riegel.net>        *
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


#ifndef PARTGUI_ViewProviderPad_H
#define PARTGUI_ViewProviderPad_H

#include "ViewProviderExtrude.h"
#include <Gui/ViewProviderPart.h>
#include <Gui/EditableDatumLabel.h>

namespace Gui {
class SoTransformDragger;
}

namespace PartDesignGui {

class TaskDlgPadParameters;

class PartDesignGuiExport ViewProviderPad : public ViewProviderExtrude
{
    PROPERTY_HEADER_WITH_OVERRIDE(PartDesignGui::ViewProviderPad);

public:
    /// constructor
    ViewProviderPad();
    /// destructor
    ~ViewProviderPad() override;

    void setupContextMenu(QMenu*, QObject*, const char*) override;

protected:
    /// Returns a newly created TaskDlgPadParameters
    TaskDlgFeatureParameters *getEditDialog() override;

public:
    bool setEdit(int ModNum) override;
    void unsetEdit(int ModNum) override;
    void setEditViewer(Gui::View3DInventorViewer*, int ModNum) override;

    void updatePosition(double padLength);
    Base::Placement getDraggerPlacement();
    double getPadLengthFromDragger();
    void hideUnWantedAxes();
    void setDraggerLabel();

    void setDraggerPosFromUI(double value);

    void createOVP(double padLength);
    void updateOVPPosition();
    Gui::View3DInventorViewer* getViewer();

private:
    Gui::CoinPtr<Gui::SoTransformDragger> dragger;
    Gui::QuantitySpinBox *ovp = nullptr;
    TaskDlgPadParameters *dialog = nullptr;
    SoNodeSensor *cameraSensor;

    static void dragStartCallback(void *data, SoDragger *d);
    static void dragFinishCallback(void *data, SoDragger *d);
    static void dragMotionCallback(void *data, SoDragger *d);
};


} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderPad_H
