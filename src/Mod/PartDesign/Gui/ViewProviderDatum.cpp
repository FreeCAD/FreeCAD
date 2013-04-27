/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender <jrheinlaender@users.sourceforge.net>        *
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


#include "PreCompiled.h"

#ifndef _PreComp_
# include <QMessageBox>
# include <QAction>
# include <QMenu>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
#endif

#include "ViewProviderDatum.h"
#include "TaskDatumParameters.h"
#include "Workbench.h"
#include <Mod/PartDesign/App/DatumFeature.h>
#include <Gui/Control.h>
#include <Gui/Command.h>

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatum,Gui::ViewProviderGeometryObject)

ViewProviderDatum::ViewProviderDatum()
{    
    pShapeSep = new SoSeparator();
    pShapeSep->ref();
}

ViewProviderDatum::~ViewProviderDatum()
{
    pShapeSep->unref();
}

void ViewProviderDatum::attach(App::DocumentObject *obj)
{
    ViewProviderDocumentObject::attach(obj);

    PartDesign::Datum* pcDatum = static_cast<PartDesign::Datum*>(getObject());
    if (pcDatum->getTypeId() == PartDesign::Plane::getClassTypeId())
        datumType = QObject::tr("Plane");
    else if (pcDatum->getTypeId() == PartDesign::Line::getClassTypeId())
        datumType = QObject::tr("Line");
    else if (pcDatum->getTypeId() == PartDesign::Point::getClassTypeId())
        datumType = QObject::tr("Point");

    SoSeparator* sep = new SoSeparator();
    SoShapeHints* hints = new SoShapeHints();
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
    SoBaseColor* color = new SoBaseColor();
    color->rgb.setValue(0.9, 0.9, 0.2);
    sep->addChild(hints);
    sep->addChild(color);
    sep->addChild(pShapeSep);
    addDisplayMaskMode(sep, "Base");
}

std::vector<std::string> ViewProviderDatum::getDisplayModes(void) const
{
    // add modes
    std::vector<std::string> StrList;
    StrList.push_back("Base");
    return StrList;
}

void ViewProviderDatum::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderDocumentObject::setDisplayMode(ModeName);
}

void ViewProviderDatum::onChanged(const App::Property* prop)
{
    /*if (prop == &Shape) {
        updateData(prop);
    }
    else {*/
        ViewProviderDocumentObject::onChanged(prop);
    //}
}

void ViewProviderDatum::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit datum ") + datumType, receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
    Gui::ViewProviderGeometryObject::setupContextMenu(menu, receiver, member);
}

bool ViewProviderDatum::setEdit(int ModNum)
{
    if (!ViewProvider::setEdit(ModNum))
        return false;

    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this datum feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumParameters *datumDlg = qobject_cast<TaskDlgDatumParameters *>(dlg);
        if (datumDlg && datumDlg->getDatumView() != this)
            datumDlg = 0; // another datum feature left open its task panel
        if (dlg && !datumDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes)
                Gui::Control().closeDialog();
            else
                return false;
        }

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        // always change to PartDesign WB, remember where we come from
        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (datumDlg)
            Gui::Control().showDialog(datumDlg);
        else
            Gui::Control().showDialog(new TaskDlgDatumParameters(this));

        return true;
    }
    else {
        return ViewProvider::setEdit(ModNum);
    }
}

void ViewProviderDatum::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderGeometryObject::unsetEdit(ModNum);
    }
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPoint,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPoint::ViewProviderDatumPoint()
{
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;
    points->numPoints = 1;
    pShapeSep->addChild(points);
}

ViewProviderDatumPoint::~ViewProviderDatumPoint()
{

}

void ViewProviderDatumPoint::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Point* pcDatum = static_cast<PartDesign::Point*>(this->getObject());

    if (strcmp(prop->getName(),"_Point") == 0) {
        Base::Vector3f p = pcDatum->_Point.getValue();
        SoMFVec3f v;
        v.set1Value(0, p.x, p.y, p.z);
        SoVertexProperty* vprop = new SoVertexProperty();
        vprop->vertex = v;
        SoMarkerSet* points = static_cast<SoMarkerSet*>(pShapeSep->getChild(0));
        points->vertexProperty = vprop;
    }

    ViewProviderDatum::updateData(prop);
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumLine,PartDesignGui::ViewProviderDatum)

ViewProviderDatumLine::ViewProviderDatumLine()
{
    /*
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;
    points->numPoints = 1;
    pShapeSep->addChild(points);
    */
}

ViewProviderDatumLine::~ViewProviderDatumLine()
{

}

void ViewProviderDatumLine::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Line* pcDatum = static_cast<PartDesign::Line*>(this->getObject());

    /*
    if (strcmp(prop->getName(),"_Point") == 0) {
        Base::Vector3f p = pcDatum->_Point.getValue();
        SoMFVec3f v;
        v.set1Value(0, p.x, p.y, p.z);
        SoVertexProperty* vprop = new SoVertexProperty();
        vprop->vertex = v;
        SoMarkerSet* points = static_cast<SoMarkerSet*>(pShapeSep->getChild(0));
        points->vertexProperty = vprop;
    }*/

    ViewProviderDatum::updateData(prop);
}

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatumPlane,PartDesignGui::ViewProviderDatum)

ViewProviderDatumPlane::ViewProviderDatumPlane()
{
    /*
    SoMarkerSet* points = new SoMarkerSet();
    points->markerIndex = SoMarkerSet::DIAMOND_FILLED_9_9;
    points->numPoints = 1;
    pShapeSep->addChild(points);
    */
}

ViewProviderDatumPlane::~ViewProviderDatumPlane()
{

}

void ViewProviderDatumPlane::updateData(const App::Property* prop)
{
    // Gets called whenever a property of the attached object changes
    PartDesign::Plane* pcDatum = static_cast<PartDesign::Plane*>(this->getObject());
/*
    if (strcmp(prop->getName(),"_Point") == 0) {
        Base::Vector3f p = pcDatum->_Point.getValue();
        SoMFVec3f v;
        v.set1Value(0, p.x, p.y, p.z);
        SoVertexProperty* vprop = new SoVertexProperty();
        vprop->vertex = v;
        SoMarkerSet* points = static_cast<SoMarkerSet*>(pShapeSep->getChild(0));
        points->vertexProperty = vprop;
    }*/

    ViewProviderDatum::updateData(prop);
}


