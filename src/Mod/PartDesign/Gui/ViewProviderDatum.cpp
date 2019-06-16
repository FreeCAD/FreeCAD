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
# include <QApplication>
# include <QMessageBox>
# include <QAction>
# include <QMenu>
# include <Inventor/actions/SoGetBoundingBoxAction.h>
# include <Inventor/nodes/SoSeparator.h>
# include <Inventor/nodes/SoPickStyle.h>
# include <Inventor/nodes/SoShapeHints.h>
# include <Inventor/nodes/SoMaterial.h>
# include <Inventor/nodes/SoBaseColor.h>
# include <Inventor/nodes/SoTransparencyType.h>
# include <Inventor/nodes/SoDrawStyle.h>
# include <Inventor/nodes/SoMarkerSet.h>
# include <Inventor/nodes/SoVertexProperty.h>
# include <Inventor/nodes/SoLineSet.h>
# include <Inventor/nodes/SoFaceSet.h>
# include <Inventor/details/SoLineDetail.h>
# include <Inventor/details/SoFaceDetail.h>
# include <Inventor/details/SoPointDetail.h>
# include <TopoDS_Vertex.hxx>
# include <TopoDS.hxx>
# include <BRep_Tool.hxx>
# include <gp_Pnt.hxx>
# include <Precision.hxx>
# include <Geom_Plane.hxx>
# include <Geom_Line.hxx>
# include <GeomAPI_IntCS.hxx>
#endif

#include <App/DocumentObjectGroup.h>
#include <App/GeoFeatureGroupExtension.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>
#include <Gui/MDIView.h>
#include <Gui/ViewProviderOrigin.h>
#include <Gui/View3DInventor.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/BitmapFactory.h>

#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumCS.h>

#include "TaskDatumParameters.h"
#include "ViewProviderBody.h"
#include "Utils.h"

#include "ViewProviderDatum.h"

using namespace PartDesignGui;

PROPERTY_SOURCE(PartDesignGui::ViewProviderDatum,Gui::ViewProviderGeometryObject)

// static data
const double ViewProviderDatum::defaultSize = Gui::ViewProviderOrigin::defaultSize ();

ViewProviderDatum::ViewProviderDatum()
{
    pShapeSep = new SoSeparator();
    pShapeSep->ref();
    pPickStyle = new SoPickStyle();
    pPickStyle->ref();

    DisplayMode.setStatus(App::Property::Hidden, true);

    // set default color for datums (golden yellow with 60% transparency)
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath (
            "User parameter:BaseApp/Preferences/Mod/PartDesign");
    unsigned long shcol = hGrp->GetUnsigned ( "DefaultDatumColor", 0xFFD70099 );

    App::Color col ( (uint32_t) shcol );
    ShapeColor.setValue ( col );

    Transparency.setValue (col.a * 100);

    oldWb = "";
    oldTip = NULL;
}

ViewProviderDatum::~ViewProviderDatum()
{
    pShapeSep->unref();
    pPickStyle->unref();
}

void ViewProviderDatum::attach(App::DocumentObject *obj)
{
    ViewProviderGeometryObject::attach ( obj );

    // TODO remove this field (2015-09-08, Fat-Zer)
    App::DocumentObject* o = getObject();
    if (o->getTypeId() == PartDesign::Plane::getClassTypeId()) {
        datumType = QString::fromLatin1("Plane");
        datumText = QObject::tr("Plane");
    }
    else if (o->getTypeId() == PartDesign::Line::getClassTypeId()) {
        datumType = QString::fromLatin1("Line");
        datumText = QObject::tr("Line");
    }
    else if (o->getTypeId() == PartDesign::Point::getClassTypeId()) {
        datumType = QString::fromLatin1("Point");
        datumText = QObject::tr("Point");
    }
    else if (o->getTypeId() == PartDesign::CoordinateSystem::getClassTypeId()) {
        datumType = QString::fromLatin1("CoordinateSystem");
        datumText = QObject::tr("Coordinate System");
    }

    SoShapeHints* hints = new SoShapeHints();
    hints->shapeType.setValue(SoShapeHints::UNKNOWN_SHAPE_TYPE);
    hints->vertexOrdering.setValue(SoShapeHints::COUNTERCLOCKWISE);
    SoDrawStyle* fstyle = new SoDrawStyle();
    fstyle->style = SoDrawStyle::FILLED;
    fstyle->lineWidth = 3;
    fstyle->pointSize = 5;
    pPickStyle->style = SoPickStyle::SHAPE;
    SoMaterialBinding* matBinding = new SoMaterialBinding;
    matBinding->value = SoMaterialBinding::OVERALL;

    SoSeparator* sep = new SoSeparator();
    sep->addChild(hints);
    sep->addChild(fstyle);
    sep->addChild(pPickStyle);
    sep->addChild(matBinding);
    sep->addChild(pcShapeMaterial);
    sep->addChild(pShapeSep);

    addDisplayMaskMode(sep, "Base");
}

bool ViewProviderDatum::onDelete(const std::vector<std::string> &)
{
    // TODO: Ask user what to do about dependent objects, e.g. Sketches that have this feature as their support
    // 1. Delete
    // 2. Suppress
    // 3. Re-route

    return true;
}

std::vector<std::string> ViewProviderDatum::getDisplayModes(void) const
{
    return { "Base" };
}

void ViewProviderDatum::setDisplayMode(const char* ModeName)
{
    if (strcmp(ModeName, "Base") == 0)
        setDisplayMaskMode("Base");
    ViewProviderGeometryObject::setDisplayMode(ModeName);
}

std::string ViewProviderDatum::getElement(const SoDetail* detail) const
{
    if (detail) {
        int element = 1;

        if (detail->getTypeId() == SoLineDetail::getClassTypeId()) {
            const SoLineDetail* line_detail = static_cast<const SoLineDetail*>(detail);
            element = line_detail->getLineIndex();
        } else if (detail->getTypeId() == SoFaceDetail::getClassTypeId()) {
            const SoFaceDetail* face_detail = static_cast<const SoFaceDetail*>(detail);
            element = face_detail->getFaceIndex();
        } else if (detail->getTypeId() == SoPointDetail::getClassTypeId()) {
            const SoPointDetail* point_detail = static_cast<const SoPointDetail*>(detail);
            element = point_detail->getCoordinateIndex();
        }

        if (element == 0)
            return datumType.toStdString();
    }

    return std::string("");
}

SoDetail* ViewProviderDatum::getDetail(const char* subelement) const
{
    QString subelem = QString::fromLatin1(subelement);

    if (subelem == QObject::tr("Line")) {
         SoLineDetail* detail = new SoLineDetail();
         detail->setPartIndex(0);
         return detail;
    } else if (subelem == QObject::tr("Plane")) {
        SoFaceDetail* detail = new SoFaceDetail();
        detail->setPartIndex(0);
        return detail;
   } else if (subelem == QObject::tr("Point")) {
        SoPointDetail* detail = new SoPointDetail();
        detail->setCoordinateIndex(0);
        return detail;
   }

    return NULL;
}

bool ViewProviderDatum::isSelectable(void) const
{
    return true;
}

void ViewProviderDatum::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit datum"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderDatum::setEdit(int ModNum)
{
    if (!ViewProvider::setEdit(ModNum))
        return false;
    // TODO Share this code with Features view providers somehow (2015-09-08, Fat-Zer)
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this datum feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumParameters *datumDlg = qobject_cast<TaskDlgDatumParameters *>(dlg);
        if (datumDlg && datumDlg->getViewProvider() != this)
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

bool ViewProviderDatum::doubleClicked(void)
{
    std::string Msg("Edit ");
    Msg += this->pcObject->Label.getValue();
    Gui::Command::openCommand(Msg.c_str());

    Part::Datum* pcDatum = static_cast<Part::Datum*>(getObject());
    PartDesign::Body* activeBody = getActiveView()->getActiveObject<PartDesign::Body*>(PDBODYKEY);
    auto datumBody = PartDesignGui::getBodyFor(pcDatum, false);

    if (datumBody != NULL) {
        if (datumBody != activeBody) {
            Gui::Command::doCommand(Gui::Command::Gui,
                "Gui.getDocument('%s').ActiveView.setActiveObject('%s', App.getDocument('%s').getObject('%s'))",
                datumBody->getDocument()->getName(),
                PDBODYKEY,
                datumBody->getDocument()->getName(),
                datumBody->getNameInDocument());
        }
    }
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.activeDocument().setEdit('%s',0)",this->pcObject->getNameInDocument());
    return true;
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

void ViewProviderDatum::updateExtents () {
    setExtents ( getRelevantBoundBox () );
}

void ViewProviderDatum::setExtents (const SbBox3f &bbox) {
    const SbVec3f & min = bbox.getMin ();
    const SbVec3f & max = bbox.getMax ();
    setExtents ( Base::BoundBox3d ( min.getValue()[0], min.getValue()[1], min.getValue()[2],
                                    max.getValue()[0], max.getValue()[1], max.getValue()[2] ) );
}

SbBox3f ViewProviderDatum::getRelevantBoundBox () const {
    std::vector<App::DocumentObject *> objs;

    // Probe body first
    PartDesign::Body* body = PartDesign::Body::findBodyOf ( this->getObject() );
    if (body) {
        objs = body->getFullModel ();
    } else {
        // Probe if we belongs to some group
        App::DocumentObject* group =  App::DocumentObjectGroup::getGroupOfObject ( this->getObject () );

        if(group) {
            auto* ext = group->getExtensionByType<App::GroupExtension>();
            if(ext)
                objs = ext->getObjects ();
        } else {
            // Fallback to whole document
            objs = this->getObject ()->getDocument ()->getObjects ();
        }
    }

    Gui::View3DInventor* view = dynamic_cast<Gui::View3DInventor*>(this->getActiveView());
    if(view){
       Gui::View3DInventorViewer* viewer = view->getViewer();
       SoGetBoundingBoxAction bboxAction(viewer->getSoRenderManager()->getViewportRegion());
       SbBox3f bbox = getRelevantBoundBox (bboxAction, objs);

       if ( bbox.getVolume () < Precision::Confusion() ) {
           bbox.extendBy ( defaultBoundBox () );
       }
       return bbox;
    } else {
       return defaultBoundBox();
    }
}

SbBox3f ViewProviderDatum::getRelevantBoundBox (
        SoGetBoundingBoxAction &bboxAction, const std::vector <App::DocumentObject *> &objs )
{
    SbBox3f bbox = defaultBoundBox();

    // Adds the bbox of given feature to the output
    for (auto obj :objs) {
        ViewProvider *vp = Gui::Application::Instance->getViewProvider(obj);
        if (!vp) { continue; }
        if (!vp->isVisible ()) { continue; }

        if (obj->isDerivedFrom (Part::Datum::getClassTypeId() ) ) {
            // Treat datums only as their basepoint
            // I hope it's ok to take FreeCAD's point here
            Base::Vector3d basePoint = static_cast<Part::Datum *> ( obj )->getBasePoint ();
            bbox.extendBy (SbVec3f(basePoint.x, basePoint.y, basePoint.z ));
        } else {
            bboxAction.apply ( vp->getRoot () );
            SbBox3f obj_bbox =  bboxAction.getBoundingBox ();

            if ( obj_bbox.getVolume () < Precision::Infinite () ) {
                bbox.extendBy ( obj_bbox );
            }
        }
    }

    // TODO: shrink bbox when all other elements are too small
    return bbox;
}

SbBox3f ViewProviderDatum::defaultBoundBox () {
    return SbBox3f ( -defaultSize, -defaultSize, -defaultSize,
            defaultSize, defaultSize, defaultSize );
}

bool ViewProviderDatum::isPickable() {

    return bool(pPickStyle->style.getValue() == SoPickStyle::SHAPE);
}

void ViewProviderDatum::setPickable(bool val) {

    if(val)
        pPickStyle->style = SoPickStyle::SHAPE;
    else
        pPickStyle->style = SoPickStyle::UNPICKABLE;
}

QIcon ViewProviderDatum::mergeOverlayIcons (const QIcon & orig) const
{
    QIcon mergedicon = orig;

    if (getObject()->hasExtension(Part::AttachExtension::getExtensionClassTypeId())) {

        bool attached = false;

        try{
            attached = getObject()->getExtensionByType<Part::AttachExtension>()->positionBySupport();
        }
        catch (...) { // We are just trying to get an icon, if no placement can be calculated, set unattached.
            // set unattached
        }

        if(!attached) {
            QPixmap px;

            static const char * const feature_detached_xpm[]={
                "9 9 3 1",
                ". c None",
                "# c #cc00cc",
                "a c #ffffff",
                "...###...",
                ".##aaa##.",
                "##aaaaa##",
                "##aaaaa##",
                ".##aaa##.",
                "...###...",
                "...###...",
                "....#....",
                "....#....",
                "....#...."};

                px = QPixmap(feature_detached_xpm);

                mergedicon = mergePixmap(mergedicon, px, Gui::BitmapFactoryInst::BottomLeft);
        }
    }

    return mergedicon;
}
