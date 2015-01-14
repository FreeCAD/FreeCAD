/***************************************************************************
 *   Copyright (c) 2010 Jürgen Riegel (juergen.riegel@web.de)              *
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
# include <cfloat>
# include <QMessageBox>
# include <Precision.hxx>
#endif

#include <Base/Tools.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Sketch.h>

#include "ViewProviderSketch.h"
#include "ui_InsertDatum.h"
#include "EditDatumDialog.h"
#include "CommandConstraints.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

bool isCreateConstraintActive(Gui::Document *doc)
{
    if (doc)
        // checks if a Sketch Viewprovider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId()))
            if (dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode() == ViewProviderSketch::STATUS_NONE)
                if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0)
                    return true;
    return false;
}

void openEditDatumDialog(Sketcher::SketchObject* sketch, int ConstrNbr)
{
    const std::vector<Sketcher::Constraint *> &Constraints = sketch->Constraints.getValues();
    Sketcher::Constraint* Constr = Constraints[ConstrNbr];

    // Return if constraint doesn't have editable value
    if (Constr->Type == Sketcher::Distance ||
        Constr->Type == Sketcher::DistanceX || 
        Constr->Type == Sketcher::DistanceY ||
        Constr->Type == Sketcher::Radius || 
        Constr->Type == Sketcher::Angle ||
        Constr->Type == Sketcher::SnellsLaw) {

        QDialog dlg(Gui::getMainWindow());
        Ui::InsertDatum ui_ins_datum;
        ui_ins_datum.setupUi(&dlg);

        double datum = Constr->Value;
        Base::Quantity init_val;

        if (Constr->Type == Sketcher::Angle) {
            datum = Base::toDegrees<double>(datum);
            dlg.setWindowTitle(EditDatumDialog::tr("Insert angle"));
            init_val.setUnit(Base::Unit::Angle);
            ui_ins_datum.label->setText(EditDatumDialog::tr("Angle:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherAngle"));
        }
        else if (Constr->Type == Sketcher::Radius) {
            dlg.setWindowTitle(EditDatumDialog::tr("Insert radius"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum.label->setText(EditDatumDialog::tr("Radius:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }
        else if (Constr->Type == Sketcher::SnellsLaw) {
            dlg.setWindowTitle(EditDatumDialog::tr("Refractive index ratio", "Constraint_SnellsLaw"));
            ui_ins_datum.label->setText(EditDatumDialog::tr("Ratio n2/n1:", "Constraint_SnellsLaw"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherRefrIndexRatio"));
        }
        else {
            dlg.setWindowTitle(EditDatumDialog::tr("Insert length"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum.label->setText(EditDatumDialog::tr("Length:"));
            ui_ins_datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherLength"));
        }

        // e.g. an angle or a distance X or Y applied on a line or two vertexes
        if (Constr->Type == Sketcher::Angle ||
            ((Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY) &&
             (Constr->FirstPos == Sketcher::none || Constr->Second != Sketcher::Constraint::GeoUndef)))
            // hide negative sign
            init_val.setValue(std::abs(datum));

        else // show negative sign
            init_val.setValue(datum);

        ui_ins_datum.labelEdit->setValue(init_val);
        ui_ins_datum.labelEdit->selectNumber();

        if (dlg.exec()) {
            Base::Quantity newQuant = ui_ins_datum.labelEdit->value();
            if (newQuant.isQuantity() || (Constr->Type == Sketcher::SnellsLaw && newQuant.isDimensionless())) {
                // save the value for the history 
                ui_ins_datum.labelEdit->pushToHistory();

                double newDatum = newQuant.getValue();
                if (Constr->Type == Sketcher::Angle ||
                    ((Constr->Type == Sketcher::DistanceX || Constr->Type == Sketcher::DistanceY) &&
                     Constr->FirstPos == Sketcher::none || Constr->Second != Sketcher::Constraint::GeoUndef)) {
                    // Permit negative values to flip the sign of the constraint
                    if (newDatum >= 0) // keep the old sign
                        newDatum = ((datum >= 0) ? 1 : -1) * std::abs(newDatum);
                    else // flip sign
                        newDatum = ((datum >= 0) ? -1 : 1) * std::abs(newDatum);
                }

                try {
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                sketch->getNameInDocument(),
                                ConstrNbr, newDatum, (const char*)newQuant.getUnit().getString().toUtf8());
                    Gui::Command::commitCommand();
                    Gui::Command::updateActive();
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    Gui::Command::abortCommand();
                }
            }
        }
        else {
            // command canceled
            Gui::Command::abortCommand();
        }
    }
}

// Utility method to avoid repeating the same code over and over again
void finishDistanceConstraint(Gui::Command* cmd, Sketcher::SketchObject* sketch)
{
    // Get the latest constraint
    const std::vector<Sketcher::Constraint *> &ConStr = sketch->Constraints.getValues();
    Sketcher::Constraint *constr = ConStr[ConStr.size() -1];

    // Guess some reasonable distance for placing the datum text
    Gui::Document *doc = cmd->getActiveGuiDocument();
    float sf = 1.f;
    if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
        SketcherGui::ViewProviderSketch *vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        sf = vp->getScaleFactor();

        constr->LabelDistance = 2. * sf;
        vp->draw(); // Redraw
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
    bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);

    // Ask for the value of the distance immediately
    if (show) {
        openEditDatumDialog(sketch, ConStr.size() - 1);
    }
    else {
        // now dialog was shown so commit the command
        cmd->commitCommand();
    }

    //updateActive();
    cmd->getSelection().clearSelection();
}

bool checkBothExternal(int GeoId1, int GeoId2)
{
    if (GeoId1 == Constraint::GeoUndef || GeoId2 == Constraint::GeoUndef)
        return false;
    else if (GeoId1 < 0 && GeoId2 < 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Cannot add a constraint between two external geometries!"));
        return true;
    }
    else
        return false;
}

void getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj,
                    int &GeoId, PointPos &PosId)
{
    GeoId = Constraint::GeoUndef;
    PosId = Sketcher::none;

    if (name.size() > 4 && name.substr(0,4) == "Edge") {
        GeoId = std::atoi(name.substr(4,4000).c_str()) - 1;
    }
    else if (name.size() == 9 && name.substr(0,9) == "RootPoint") {
        GeoId = -1;
        PosId = Sketcher::start;
    }
    else if (name.size() == 6 && name.substr(0,6) == "H_Axis")
        GeoId = -1;
    else if (name.size() == 6 && name.substr(0,6) == "V_Axis")
        GeoId = -2;
    else if (name.size() > 12 && name.substr(0,12) == "ExternalEdge")
        GeoId = -2 - std::atoi(name.substr(12,4000).c_str());
    else if (name.size() > 6 && name.substr(0,6) == "Vertex") {
        int VtId = std::atoi(name.substr(6,4000).c_str()) - 1;
        Obj->getGeoVertexIndex(VtId,GeoId,PosId);
    }
}

bool inline isVertex(int GeoId, PointPos PosId)
{
    return (GeoId != Constraint::GeoUndef && PosId != Sketcher::none);
}

bool inline isEdge(int GeoId, PointPos PosId)
{
    return (GeoId != Constraint::GeoUndef && PosId == Sketcher::none);
}

bool isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, PointPos PosId)
{
    if (PosId == Sketcher::start && (GeoId == -1 || GeoId == -2))
        return true;
    const Part::Geometry *geo = Obj->getGeometry(GeoId);
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId())
        return true;
    else if (PosId == Sketcher::mid)
        return true;
    else
        return false;
}

bool IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj)
{
    //This func is a "smartness" behind three-element tangent-, perp.- and angle-via-point.
    //We want to find out, if the point supplied by user is already on
    // both of the curves. If not, necessary point-on-object constraints
    // are to be added automatically.
    //Simple geometric test seems to be the best, because a point can be
    // constrained to a curve in a number of ways (e.g. it is an endpoint of an
    // arc, or is coincident to endpoint of an arc, or it is an endpoint of an
    // ellipse's majopr diameter line). Testing all those possibilities is way
    // too much trouble, IMO(DeepSOIC).
    Base::Vector3d p = Obj->getPoint(GeoIdPoint, PosIdPoint);
    return Obj->isPointOnCurve(GeoIdCurve, p.x, p.y);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an ellipse
/// geom2 => any of an ellipse, an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToEllipseviaNewPoint(const Sketcher::SketchObject* Obj,
                                             const Part::Geometry *geom1, 
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            )
{
    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geom1);
    
    Base::Vector3d center=ellipse->getCenter();
    double majord=ellipse->getMajorRadius();
    double minord=ellipse->getMinorRadius();
    double phi=ellipse->getAngleXU();

    Base::Vector3d center2;
    
    if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() )
        center2= (static_cast<const Part::GeomEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomCircle *>(geom2))->getCenter();               
    else if( geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfCircle *>(geom2))->getCenter();

    Base::Vector3d direction=center2-center;
    double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomally by the polar

    Base::Vector3d PoE = Base::Vector3d(center.x+majord*cos(tapprox)*cos(phi)-minord*sin(tapprox)*sin(phi),
                                        center.y+majord*cos(tapprox)*sin(phi)+minord*sin(tapprox)*cos(phi), 0);
    
    try {                    
        // Add a point
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
            Obj->getNameInDocument(), PoE.x,PoE.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();

        // Point on first object
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
            Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,geoId1); // constrain major axis
        // Point on second object
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
            Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,geoId2); // constrain major axis
        // tangent via point
        Gui::Command::doCommand(
            Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
            Obj->getNameInDocument(), geoId1, geoId2 ,GeoIdPoint, Sketcher::start);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        Gui::Command::abortCommand();
        Gui::Command::updateActive();
        return;
    }

    Gui::Command::commitCommand();
    Gui::Command::updateActive();
}
/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an arc of ellipse
/// geom2 => any of an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfEllipseviaNewPoint(const Sketcher::SketchObject* Obj,
                                             const Part::Geometry *geom1, 
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
                                            )
{
    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geom1);
    
    Base::Vector3d center=aoe->getCenter();
    double majord=aoe->getMajorRadius();
    double minord=aoe->getMinorRadius();
    double phi=aoe->getAngleXU();

    Base::Vector3d center2;
    
    if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomCircle *>(geom2))->getCenter();               
    else if( geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfCircle *>(geom2))->getCenter();

    Base::Vector3d direction=center2-center;
    double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomally by the polar

    Base::Vector3d PoE = Base::Vector3d(center.x+majord*cos(tapprox)*cos(phi)-minord*sin(tapprox)*sin(phi),
                                        center.y+majord*cos(tapprox)*sin(phi)+minord*sin(tapprox)*cos(phi), 0);
    
    try {
        // Add a point
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
            Obj->getNameInDocument(), PoE.x,PoE.y);
        int GeoIdPoint = Obj->getHighestCurveIndex();
                    
        // Point on first object
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
            Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,geoId1); // constrain major axis
        // Point on second object
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
            Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,geoId2); // constrain major axis
        // tangent via point
        Gui::Command::doCommand(
            Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d))",
            Obj->getNameInDocument(), geoId1, geoId2 ,GeoIdPoint, Sketcher::start);
    }
    catch (const Base::Exception& e) {
        Base::Console().Error("%s\n", e.what());
        Gui::Command::abortCommand();
        Gui::Command::updateActive();
        return;
    }

    Gui::Command::commitCommand();
    Gui::Command::updateActive();  
}

namespace SketcherGui {

struct SketchSelection{
    enum GeoType {
        Point,
        Line,
        Circle,
        Arc
    };
    int setUp(void);
    struct SketchSelectionItem {
        GeoType type;
        int GeoId;
        bool Extern;
    };
    std::list<SketchSelectionItem> Items;
    QString ErrorMsg;
};

int SketchSelection::setUp(void)
{
    std::vector<Gui::SelectionObject> selection = Gui::Selection().getSelectionEx();

    Sketcher::SketchObject *SketchObj=0;
    Part::Feature          *SupportObj=0;
    std::vector<std::string> SketchSubNames;
    std::vector<std::string> SupportSubNames;

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() == 1) {
        // if one selectetd, only sketch allowed. should be done by activity of command
        if(!selection[0].getObject()->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId()))
        {
            ErrorMsg = QObject::tr("Only sketch and its support is allowed to select");
            return -1;
        }

        SketchObj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
        SketchSubNames = selection[0].getSubNames();
    } else if(selection.size() == 2) {
        if(selection[0].getObject()->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())){
            SketchObj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
            // check if the none sketch object is the support of the sketch
            if(selection[1].getObject() != SketchObj->Support.getValue()){
                ErrorMsg = QObject::tr("Only sketch and its support is allowed to select");
                return-1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[1].getObject()->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()));
            SupportObj = dynamic_cast<Part::Feature*>(selection[1].getObject());
            SketchSubNames  = selection[0].getSubNames();
            SupportSubNames = selection[1].getSubNames();

        } else if (selection[1].getObject()->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
            SketchObj = dynamic_cast<Sketcher::SketchObject*>(selection[1].getObject());
            // check if the none sketch object is the support of the sketch
            if(selection[0].getObject() != SketchObj->Support.getValue()){
                ErrorMsg = QObject::tr("Only sketch and its support is allowed to select");
                return -1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[0].getObject()->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()));
            SupportObj = dynamic_cast<Part::Feature*>(selection[0].getObject());
            SketchSubNames  = selection[1].getSubNames();
            SupportSubNames = selection[0].getSubNames();

        } else {
            ErrorMsg = QObject::tr("One of the selected has to be on the sketch");
            return -1;
        }
    }

    // colect Sketch geos
    for ( std::vector<std::string>::const_iterator it= SketchSubNames.begin();it!=SketchSubNames.end();++it){


    }


    return Items.size();
}

} // namespace SketcherGui



/* Constrain commands =======================================================*/
DEF_STD_CMD_A(CmdSketcherConstrainHorizontal);

CmdSketcherConstrainHorizontal::CmdSketcherConstrainHorizontal()
    :Command("Sketcher_ConstrainHorizontal")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain horizontally");
    sToolTipText    = QT_TR_NOOP("Create a horizontal constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainHorizontal";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Horizontal";
    sAccel          = "H";
    eType           = ForEdit;
}

void CmdSketcherConstrainHorizontal::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    std::vector<int> ids;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            const Part::Geometry *geo = Obj->getGeometry(GeoId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge has already a Horizontal or Vertical constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId){
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                        QObject::tr("The selected edge has already a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge has already a vertical constraint!"));
                    return;
                }
            }
            ids.push_back(GeoId);
        }
    }

    if (ids.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("The selected item(s) can't accept a horizontal constraint!"));
        return;
    }

    // undo command open
    openCommand("add horizontal constraint");
    for (std::vector<int>::iterator it=ids.begin(); it != ids.end(); it++) {
        // issue the actual commands to create the constraint
        doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Horizontal',%d)) "
                 ,selection[0].getFeatName(),*it);
    }
    // finish the transaction and update
    commitCommand();
    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainHorizontal::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainVertical);

CmdSketcherConstrainVertical::CmdSketcherConstrainVertical()
    :Command("Sketcher_ConstrainVertical")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain vertically");
    sToolTipText    = QT_TR_NOOP("Create a vertical constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainVertical";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Vertical";
    sAccel          = "V";
    eType           = ForEdit;
}

void CmdSketcherConstrainVertical::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select an edge from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

     std::vector<int> ids;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin();it!=SubNames.end();++it) {
        // only handle edges
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;

            const Part::Geometry *geo = Obj->getGeometry(GeoId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge has already a Horizontal or Vertical constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge has already a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                        QObject::tr("The selected edge has already a vertical constraint!"));
                    return;
                }
            }
            ids.push_back(GeoId);
        }
    }

    if (ids.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("The selected item(s) can't accept a vertical constraint!"));
        return;
    }

    // undo command open
    openCommand("add vertical constraint");
    for (std::vector<int>::iterator it=ids.begin(); it != ids.end(); it++) {
        // issue the actual command to create the constraint
        doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Vertical',%d))"
                 ,selection[0].getFeatName(),*it);
    }
    // finish the transaction and update
    commitCommand();
    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainVertical::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainLock);

CmdSketcherConstrainLock::CmdSketcherConstrainLock()
    :Command("Sketcher_ConstrainLock")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain lock");
    sToolTipText    = QT_TR_NOOP("Create a lock constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainLock";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ConstrainLock";
    eType           = ForEdit;
}

void CmdSketcherConstrainLock::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select entities from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one entity from the sketch."));
        return;
    }

    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[0], Obj, GeoId, PosId);

    if (isEdge(GeoId,PosId) || GeoId < 0) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one vertex from the sketch."));
        return;
    }

    Base::Vector3d pnt = Obj->getPoint(GeoId,PosId);

    // undo command open
    openCommand("add fixed constraint");
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
        selection[0].getFeatName(),GeoId,PosId,pnt.x);
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
        selection[0].getFeatName(),GeoId,PosId,pnt.y);

    // finish the transaction and update
    commitCommand();
    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainLock::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainCoincident);

CmdSketcherConstrainCoincident::CmdSketcherConstrainCoincident()
    :Command("Sketcher_ConstrainCoincident")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain coincident");
    sToolTipText    = QT_TR_NOOP("Create a coincident constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainCoincident";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_PointOnPoint";
    sAccel          = "C";
    eType           = ForEdit;
}

void CmdSketcherConstrainCoincident::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select vertexes from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two or more vertexes from the sketch."));
        return;
    }

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);
        if (isEdge(GeoId,PosId)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two or more vertexes from the sketch."));
            return;
        }
    }

    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);

    // undo command open
    bool constraintsAdded = false;
    openCommand("add coincident constraint");
    for (std::size_t i=1; i<SubNames.size(); i++) {
        getIdsFromName(SubNames[i], Obj, GeoId2, PosId2);

        // check if any of the coincident constraints exist
        bool constraintExists=false;

        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin(); it != vals.end(); ++it) {
            if ((*it)->Type == Sketcher::Coincident && (
               ((*it)->First == GeoId1 && (*it)->FirstPos == PosId1 &&
                (*it)->Second == GeoId2 && (*it)->SecondPos == PosId2  ) ||
               ((*it)->First == GeoId2 && (*it)->FirstPos == PosId2 &&
                (*it)->Second == GeoId1 && (*it)->SecondPos == PosId1  ) ) ) {
                constraintExists=true;
                break;
            }
        }

        if (!constraintExists) {
            constraintsAdded = true;
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
        }
    }

    // finish or abort the transaction and update
    if (constraintsAdded)
        commitCommand();
    else
        abortCommand();

    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainCoincident::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainDistance);

CmdSketcherConstrainDistance::CmdSketcherConstrainDistance()
    :Command("Sketcher_ConstrainDistance")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain distance");
    sToolTipText    = QT_TR_NOOP("Fix a length of a line or the distance between a line and a vertex");
    sWhatsThis      = "Sketcher_ConstrainDistance";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Length";
    sAccel          = "SHIFT+D";
    eType           = ForEdit;
}

void CmdSketcherConstrainDistance::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select vertexes from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 1 || SubNames.size() > 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one line or one point and one line or two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2=Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;
    else if (isVertex(GeoId1,PosId1) && (GeoId2 == -2 || GeoId2 == -1)) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if ((isVertex(GeoId1,PosId1) || GeoId1 == -2 || GeoId1 == -1) &&
        isVertex(GeoId2,PosId2)) { // point to point distance

        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);

        if (GeoId1 == -1 && PosId1 == Sketcher::none) {
            PosId1 = Sketcher::start;
            openCommand("add distance from horizontal axis constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,pnt2.y);
        }
        else if (GeoId1 == -2 && PosId1 == Sketcher::none) {
            PosId1 = Sketcher::start;
            openCommand("add distance from vertical axis constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,pnt2.x);
        }
        else {
            Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);

            openCommand("add point to point distance constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,(pnt2-pnt1).Length());
        }

        finishDistanceConstraint(this, Obj);
        return;
    }
    else if ((isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) ||
             (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2)))  { // point to line distance
        if (isVertex(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }
        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        const Part::Geometry *geom = Obj->getGeometry(GeoId2);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geom);
            Base::Vector3d pnt1 = lineSeg->getStartPoint();
            Base::Vector3d pnt2 = lineSeg->getEndPoint();
            Base::Vector3d d = pnt2-pnt1;
            double ActDist = std::abs(-pnt.x*d.y+pnt.y*d.x+pnt1.x*pnt2.y-pnt2.x*pnt1.y) / d.Length();

            openCommand("add point to line Distance constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,ActDist);

            finishDistanceConstraint(this, Obj);
            return;
        }
    }
    else if (isEdge(GeoId1,PosId1)) { // line length
        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 GeoId1 < 2 ? QObject::tr("Cannot add a length constraint on an external geometry!")
                                            : QObject::tr("Cannot add a length constraint on an axis!"));
            return;
        }

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Length();

            openCommand("add length constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                selection[0].getFeatName(),GeoId1,ActLength);

            finishDistanceConstraint(this, Obj);
            return;
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or one point and one line or two points from the sketch."));
    return;
}

bool CmdSketcherConstrainDistance::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainPointOnObject);

CmdSketcherConstrainPointOnObject::CmdSketcherConstrainPointOnObject()
    :Command("Sketcher_ConstrainPointOnObject")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain point onto object");
    sToolTipText    = QT_TR_NOOP("Fix a point onto an object");
    sWhatsThis      = "Sketcher_ConstrainPointOnObject";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_PointOnObject";
    sAccel          = "SHIFT+O";
    eType           = ForEdit;
}

void CmdSketcherConstrainPointOnObject::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select vertexes from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 1 || SubNames.size() > 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one point and one object from the sketch."));
        return;
    }

    int GeoId1, GeoId2=Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;

    if ((isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) ||
        (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2))) {
        if (isVertex(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }

        const Part::Geometry *geom = Obj->getGeometry(GeoId2);

        // Currently only accepts line segments and circles
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId() ||
            geom->getTypeId() == Part::GeomCircle::getClassTypeId() || 
            geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
            geom->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
            geom->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ) {

            openCommand("add point on object constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2);
            commitCommand();
            //updateActive();
            getSelection().clearSelection();
            return;
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one point and one object from the sketch."));
    return;
}

bool CmdSketcherConstrainPointOnObject::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}

DEF_STD_CMD_A(CmdSketcherConstrainDistanceX);

CmdSketcherConstrainDistanceX::CmdSketcherConstrainDistanceX()
    :Command("Sketcher_ConstrainDistanceX")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain horizontal distance");
    sToolTipText    = QT_TR_NOOP("Fix the horizontal distance between two points or line ends");
    sWhatsThis      = "Sketcher_ConstrainDistanceX";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_HorizontalDistance";
    sAccel          = "SHIFT+H";
    eType           = ForEdit;
}

void CmdSketcherConstrainDistanceX::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select vertexes from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 1 || SubNames.size() > 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one line or up to two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2=Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;
    else if (GeoId2 == -1 || GeoId2 == -2) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if (GeoId1 == -1 && PosId1 == Sketcher::none) // reject horizontal axis from selection
        GeoId1 = Constraint::GeoUndef;
    else if (GeoId1 == -2 && PosId1 == Sketcher::none) {
        GeoId1 = -1;
        PosId1 = Sketcher::start;
    }

    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // point to point horizontal distance

        Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
        double ActLength = pnt2.x-pnt1.x;

        openCommand("add point to point horizontal distance constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

        finishDistanceConstraint(this, Obj);
        return;
    }
    else if (isEdge(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef)  { // horizontal length of a line

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a horizontal length constraint on an external geometry!")
                            : QObject::tr("Cannot add a horizontal length constraint on an axis!"));
            return;
        }

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = lineSeg->getEndPoint().x-lineSeg->getStartPoint().x;

            openCommand("add horizontal length constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%f)) ",
                selection[0].getFeatName(),GeoId1,ActLength);

            finishDistanceConstraint(this, Obj);
            return;
        }
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed x-coordinate

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a fixed x-coordinate constraint on an external geometry!")
                            : QObject::tr("Cannot add a fixed x-coordinate constraint on the root point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActX = pnt.x;

        openCommand("add fixed x-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActX);

        finishDistanceConstraint(this, Obj);
        return;
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

bool CmdSketcherConstrainDistanceX::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainDistanceY);

CmdSketcherConstrainDistanceY::CmdSketcherConstrainDistanceY()
    :Command("Sketcher_ConstrainDistanceY")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain vertical distance");
    sToolTipText    = QT_TR_NOOP("Fix the vertical distance between two points or line ends");
    sWhatsThis      = "Sketcher_ConstrainDistanceY";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_VerticalDistance";
    sAccel          = "SHIFT+V";
    eType           = ForEdit;
}

void CmdSketcherConstrainDistanceY::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select vertexes from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 1 || SubNames.size() > 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one line or up to two points from the sketch."));
        return;
    }

    int GeoId1, GeoId2=Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;
    else if (GeoId2 == -1 || GeoId2 == -2) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if (GeoId1 == -2 && PosId1 == Sketcher::none) // reject vertical axis from selection
        GeoId1 = Constraint::GeoUndef;
    else if (GeoId1 == -1 && PosId1 == Sketcher::none)
        PosId1 = Sketcher::start;

    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // point to point vertical distance

        Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
        double ActLength = pnt2.y-pnt1.y;

        openCommand("add point to point vertical distance constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

        finishDistanceConstraint(this, Obj);
        return;
    }
    else if (isEdge(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef)  { // vertical length of a line

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a vertical length constraint on an external geometry!")
                            : QObject::tr("Cannot add a vertical length constraint on an axis!"));
            return;
        }

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = lineSeg->getEndPoint().y-lineSeg->getStartPoint().y;

            openCommand("add vertical length constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%f)) ",
                selection[0].getFeatName(),GeoId1,ActLength);

            finishDistanceConstraint(this, Obj);
            return;
        }
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed y-coordinate

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a fixed y-coordinate constraint on an external geometry!")
                            : QObject::tr("Cannot add a fixed y-coordinate constraint on the root point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActY = pnt.y;

        openCommand("add fixed y-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActY);

        finishDistanceConstraint(this, Obj);
        return;
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

bool CmdSketcherConstrainDistanceY::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainParallel);

CmdSketcherConstrainParallel::CmdSketcherConstrainParallel()
    :Command("Sketcher_ConstrainParallel")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain parallel");
    sToolTipText    = QT_TR_NOOP("Create a parallel constraint between two lines");
    sWhatsThis      = "Sketcher_ConstrainParallel";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Parallel";
    sAccel          = "SHIFT+P";
    eType           = ForEdit;
}

void CmdSketcherConstrainParallel::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two or more lines from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements

    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool hasAlreadyExternal=false;
    for (std::vector<std::string>::const_iterator it=SubNames.begin();it!=SubNames.end();++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);

        if (!isEdge(GeoId,PosId)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select a valid line"));
            return;
        }
        else if (GeoId < 0) {
            if (hasAlreadyExternal) {
                checkBothExternal(-1,-2); // just for printing the error message
                return;
            }
            else
                hasAlreadyExternal = true;
        }

        // Check that the curve is a line segment
        const Part::Geometry *geo = Obj->getGeometry(GeoId);
        if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                QObject::tr("The selected edge is not a valid line"));
            return;
        }
        ids.push_back(GeoId);
    }

    // undo command open
    openCommand("add parallel constraint");
    for (int i=0; i < int(ids.size()-1); i++) {
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Parallel',%d,%d)) ",
            selection[0].getFeatName(),ids[i],ids[i+1]);
    }
    // finish the transaction and update
    commitCommand();
    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainParallel::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainPerpendicular);

CmdSketcherConstrainPerpendicular::CmdSketcherConstrainPerpendicular()
    :Command("Sketcher_ConstrainPerpendicular")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain perpendicular");
    sToolTipText    = QT_TR_NOOP("Create a perpendicular constraint between two lines");
    sWhatsThis      = "Sketcher_ConstrainPerpendicular";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Perpendicular";
    sAccel          = "N";
    eType           = ForEdit;
}

void CmdSketcherConstrainPerpendicular::activated(int iMsg)
{
    QString strBasicHelp =
            QObject::tr(
             "There is a number of ways this constraint can be applied.\n\n"
             "Accepted combinations: two curves; an endpoint and a curve; two endpoints; two curves and a point.",
             /*disambig.:*/ "perpendicular constraint");
    QString strError;

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        strError = QObject::tr("Select some geometry from the sketch.", "perpendicular constraint");
        if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            strError+strBasicHelp);
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 2 && SubNames.size() != 3) {
        strError = QObject::tr("Wrong number of selected objects!","perpendicular constraint");
        if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            strError+strBasicHelp);
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2)) //checkBothExternal displays error message
        return;

    if (SubNames.size() == 3) { //perpendicular via point
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);
        //let's sink the point to be GeoId3. We want to keep the order the two curves have been selected in.
        if ( isVertex(GeoId1, PosId1) ){
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        };
        if ( isVertex(GeoId2, PosId2) ){
            std::swap(GeoId2,GeoId3);
            std::swap(PosId2,PosId3);
        };

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

            openCommand("add perpendicular constraint");

            try{
                //add missing point-on-object constraints
                if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
                };

                if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId2);
                };

                if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
                };

                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId1,GeoId2,GeoId3,PosId3);
            } catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                QMessageBox::warning(Gui::getMainWindow(),
                                     QObject::tr("Error"),
                                     QString::fromLatin1(e.what()));
                Gui::Command::abortCommand();
                Gui::Command::updateActive();
                return;
            }

            commitCommand();
            updateActive();
            getSelection().clearSelection();

            return;

        };
        strError = QObject::tr("With 3 objects, there must be 2 curves and 1 point.", "tangent constraint");

    } else if (SubNames.size() == 2) {

        if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { //endpoint-to-endpoint perpendicularity

            if (isSimpleVertex(Obj, GeoId1, PosId1) ||
                isSimpleVertex(Obj, GeoId2, PosId2)) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a perpendicularity constraint at an unconnected point!"));
                return;
            }

            openCommand("add perpendicular constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }
        else if ((isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) ||
                 (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2))) { // endpoint-to-curve
            if (isVertex(GeoId2,PosId2)) {
                std::swap(GeoId1,GeoId2);
                std::swap(PosId1,PosId2);
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1)) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a perpendicularity constraint at an unconnected point!"));
                return;
            }

            openCommand("add perpendicularity constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) { // simple perpendicularity between GeoId1 and GeoId2

            const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
            if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("One of the selected edges should be a line."));
                return;
            }

            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId())
                std::swap(GeoId1,GeoId2);

            // GeoId2 is the line
            geo1 = Obj->getGeometry(GeoId1);
            geo2 = Obj->getGeometry(GeoId2);

            if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ) {

                Base::Vector3d center;
                double majord;
                double minord;
                double phi;

                if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ){
                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo1);

                    center=ellipse->getCenter();
                    majord=ellipse->getMajorRadius();
                    minord=ellipse->getMinorRadius();
                    phi=ellipse->getAngleXU();
                } else
                  if( geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ){
                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo1);

                    center=aoe->getCenter();
                    majord=aoe->getMajorRadius();
                    minord=aoe->getMinorRadius();
                    phi=aoe->getAngleXU();
                }

                const Part::GeomLineSegment *line = static_cast<const Part::GeomLineSegment *>(geo2);

                Base::Vector3d point1=line->getStartPoint();
                Base::Vector3d point2=line->getEndPoint();

                Base::Vector3d direction=point1-center;
                double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomally by the polar

                Base::Vector3d PoE = Base::Vector3d(center.x+majord*cos(tapprox)*cos(phi)-minord*sin(tapprox)*sin(phi),
                                                    center.y+majord*cos(tapprox)*sin(phi)+minord*sin(tapprox)*cos(phi), 0);

                openCommand("add perpendicular constraint");

                try {
                    // Add a point
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                        Obj->getNameInDocument(), PoE.x,PoE.y);
                    int GeoIdPoint = Obj->getHighestCurveIndex();

                    // Point on first object (ellipse, arc of ellipse)
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoIdPoint,Sketcher::start,GeoId1);
                    // Point on second object
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoIdPoint,Sketcher::start,GeoId2);

                    // add constraint: Perpendicular-via-point
                    Gui::Command::doCommand(
                        Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                        Obj->getNameInDocument(), GeoId1, GeoId2 ,GeoIdPoint, Sketcher::start);

                }
                catch (const Base::Exception& e) {
                    Base::Console().Error("%s\n", e.what());
                    Gui::Command::abortCommand();
                    Gui::Command::updateActive();
                    return;
                }

                commitCommand();
                updateActive();
                getSelection().clearSelection();
                return;

            }

            openCommand("add perpendicular constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d)) ",
                selection[0].getFeatName(),GeoId1,GeoId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }
    }

    if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        strError+strBasicHelp);
    return;
}

bool CmdSketcherConstrainPerpendicular::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainTangent);

CmdSketcherConstrainTangent::CmdSketcherConstrainTangent()
    :Command("Sketcher_ConstrainTangent")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain tangent");
    sToolTipText    = QT_TR_NOOP("Create a tangent constraint between two entities");
    sWhatsThis      = "Sketcher_ConstrainTangent";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Tangent";
    sAccel          = "T";
    eType           = ForEdit;
}

void CmdSketcherConstrainTangent::activated(int iMsg)
{
    QString strBasicHelp =
            QObject::tr(
             "There is a number of ways this constraint can be applied.\n\n"
             "Accepted combinations: two curves; an endpoint and a curve; two endpoints; two curves and a point.",
             /*disambig.:*/ "tangent constraint");

    QString strError;
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        strError = QObject::tr("Select some geometry from the sketch.", "tangent constraint");
        if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            strError+strBasicHelp);
        return;

    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 2 && SubNames.size() != 3){
        strError = QObject::tr("Wrong number of selected objects!","tangent constraint");
        if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            strError+strBasicHelp);
        return;

    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2)) //checkBothExternal displays error message
        return;
    if (SubNames.size() == 3) { //tangent via point
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);
        //let's sink the point to be GeoId3. We want to keep the order the two curves have been selected in.
        if ( isVertex(GeoId1, PosId1) ){
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        };
        if ( isVertex(GeoId2, PosId2) ){
            std::swap(GeoId2,GeoId3);
            std::swap(PosId2,PosId3);
        };

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

            openCommand("add tangent constraint");

            try{
                //add missing point-on-object constraints
                if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
                };

                if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId2);
                };

                if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
                    Gui::Command::doCommand(
                        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                        selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
                };

                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId1,GeoId2,GeoId3,PosId3);
            } catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                QMessageBox::warning(Gui::getMainWindow(),
                                     QObject::tr("Error"),
                                     QString::fromLatin1(e.what()));
                Gui::Command::abortCommand();
                Gui::Command::updateActive();
                return;
            }

            commitCommand();
            updateActive();
            getSelection().clearSelection();

            return;

        };
        strError = QObject::tr("With 3 objects, there must be 2 curves and 1 point.", "tangent constraint");

    } else if (SubNames.size() == 2) {

        if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // endpoint-to-endpoint tangency

            if (isSimpleVertex(Obj, GeoId1, PosId1) ||
                isSimpleVertex(Obj, GeoId2, PosId2)) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
                return;
            }

            openCommand("add tangent constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }
        else if ((isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) ||
                 (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2))) { // endpoint-to-curve tangency
            if (isVertex(GeoId2,PosId2)) {
                std::swap(GeoId1,GeoId2);
                std::swap(PosId1,PosId2);
            }

            if (isSimpleVertex(Obj, GeoId1, PosId1)) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
                return;
            }

            openCommand("add tangent constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) { // simple tangency between GeoId1 and GeoId2

            const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

            if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                  geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomEllipse::getClassTypeId())
                    std::swap(GeoId1,GeoId2);

                // GeoId1 is the ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {

                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToEllipseviaNewPoint(Obj,geom1,geom2,GeoId1,GeoId2);
                    getSelection().clearSelection();
                    return;
                }
            }

            if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                  geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId())
                    std::swap(GeoId1,GeoId2);

                // GeoId1 is the arc of ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {

                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfEllipseviaNewPoint(Obj,geom1,geom2,GeoId1,GeoId2);
                    getSelection().clearSelection();
                    return;
                }

            }

            openCommand("add tangent constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d)) ",
                selection[0].getFeatName(),GeoId1,GeoId2);
            commitCommand();
            updateActive();
            getSelection().clearSelection();
            return;
        }

    }

    if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        strError+strBasicHelp);
    return;
}

bool CmdSketcherConstrainTangent::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainRadius);

CmdSketcherConstrainRadius::CmdSketcherConstrainRadius()
    :Command("Sketcher_ConstrainRadius")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain radius");
    sToolTipText    = QT_TR_NOOP("Fix the radius of a circle or an arc");
    sWhatsThis      = "Sketcher_ConstrainRadius";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Radius";
    sAccel          = "R";
    eType           = ForEdit;
}

void CmdSketcherConstrainRadius::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector< std::pair<int, double> > geoIdRadiusMap;
    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            int GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            const Part::Geometry *geom = Obj->getGeometry(GeoId);
            if (geom && geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geom);
                double radius = arc->getRadius();
                geoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
            else if (geom && geom->getTypeId() == Part::GeomCircle::getClassTypeId()) { 
                const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geom);
                double radius = circle->getRadius();
                geoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
        }
    }

    if (geoIdRadiusMap.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
    }
    else {
        bool constrainEqual = false;
        if (geoIdRadiusMap.size() > 1) {
            int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Constrain equal"),
                QObject::tr("Do you want to share the same radius for all selected elements?"),
                QMessageBox::Yes, QMessageBox::No, QMessageBox::Cancel);
            // use an equality constraint
            if (ret == QMessageBox::Yes) {
                constrainEqual = true;
            }
            else if (ret == QMessageBox::Cancel) {
                // do nothing
                return;
            }
        }

        if (constrainEqual) {
            // Create the one radius constraint now
            int refGeoId = geoIdRadiusMap.front().first;
            double radius = geoIdRadiusMap.front().second;
            openCommand("Add radius constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                selection[0].getFeatName(),refGeoId,radius);

            // Add the equality constraints
            for (std::vector< std::pair<int, double> >::iterator it = geoIdRadiusMap.begin()+1; it != geoIdRadiusMap.end(); ++it) {
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
                    selection[0].getFeatName(),refGeoId,it->first);
            }
        }
        else {
            // Create the radius constraints now
            openCommand("Add radius constraint");
            for (std::vector< std::pair<int, double> >::iterator it = geoIdRadiusMap.begin(); it != geoIdRadiusMap.end(); ++it) {
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    selection[0].getFeatName(),it->first,it->second);
            }
        }

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();
        std::size_t indexConstr = ConStr.size() - geoIdRadiusMap.size();

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = dynamic_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            for (std::size_t i=0; i<geoIdRadiusMap.size();i++) {
                Sketcher::Constraint *constr = ConStr[indexConstr + i];
                constr->LabelDistance = 2. * sf;
            }
            vp->draw(); // Redraw
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");
        bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
        // Ask for the value of the radius immediately
        if (show) {
            QDialog dlg(Gui::getMainWindow());
            Ui::InsertDatum ui_Datum;
            ui_Datum.setupUi(&dlg);
            dlg.setWindowTitle(EditDatumDialog::tr("Change radius"));
            ui_Datum.label->setText(EditDatumDialog::tr("Radius:"));
            Base::Quantity init_val;
            init_val.setUnit(Base::Unit::Length);
            init_val.setValue(geoIdRadiusMap.front().second);

            ui_Datum.labelEdit->setValue(init_val);
            ui_Datum.labelEdit->selectNumber();

            if (dlg.exec() == QDialog::Accepted) {
                Base::Quantity newQuant = ui_Datum.labelEdit->value();
                double newRadius = newQuant.getValue();

                try {
                    if (constrainEqual) {
                        doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                    Obj->getNameInDocument(),
                                    indexConstr, newRadius, (const char*)newQuant.getUnit().getString().toUtf8());
                    }
                    else {
                        for (std::size_t i=0; i<geoIdRadiusMap.size();i++) {
                            doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                        Obj->getNameInDocument(),
                                        indexConstr+i, newRadius, (const char*)newQuant.getUnit().getString().toUtf8());
                        }
                    }
                    commitCommand();
                    updateActive();
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    abortCommand();
                }
            }
            else {
                // command canceled
                abortCommand();
            }
        }
        else {
            // now dialog was shown so commit the command
            commitCommand();
        }

        //updateActive();
        getSelection().clearSelection();
    }
}

bool CmdSketcherConstrainRadius::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}

DEF_STD_CMD_A(CmdSketcherConstrainAngle);

CmdSketcherConstrainAngle::CmdSketcherConstrainAngle()
    :Command("Sketcher_ConstrainAngle")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain angle");
    sToolTipText    = QT_TR_NOOP("Fix the angle of a line or the angle between two lines");
    sWhatsThis      = "Sketcher_ConstrainAngle";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_InternalAngle";
    sAccel          = "A";
    eType           = ForEdit;
}


void CmdSketcherConstrainAngle::activated(int iMsg)
{
    //TODO: comprehensive messages, like in CmdSketcherConstrainTangent
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select only entities from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() < 1 || SubNames.size() > 3) {
        //goto ExitWithMessage;
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or two lines from the sketch. Or select two edges and a point."));

    }


    int GeoId1, GeoId2=Constraint::GeoUndef, GeoId3 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none, PosId3 = Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() > 1)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
    if (SubNames.size() > 2)
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

    if (SubNames.size() == 3){//standalone implementation of angle-via-point

        //let's sink the point to be GeoId3. We want to keep the order the two curves have been selected in.
        if ( isVertex(GeoId1, PosId1) ){
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        };
        if ( isVertex(GeoId2, PosId2) ){
            std::swap(GeoId2,GeoId3);
            std::swap(PosId2,PosId3);
        };

        if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {
            double ActAngle = 0.0;

            openCommand("Add angle constraint");

            //add missing point-on-object constraints
            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
            };
            if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId3,PosId3,GeoId2);
            };
            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId3,PosId3,GeoId1);
            };

            //assuming point-on-curves have been solved, calculate the angle.
            //DeepSOIC: this may be slow, but I wanted to reuse the conversion from Geometry to GCS shapes that is done in Sketch
            Base::Vector3d p = Obj->getPoint(GeoId3, PosId3 );
            ActAngle = Obj->calculateAngleViaPoint(GeoId1,GeoId2,p.x,p.y);

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('AngleViaPoint',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,GeoId2,GeoId3,PosId3,ActAngle);

            finishDistanceConstraint(this, Obj);
            return;
        };

    } else if (SubNames.size() < 3) {

        if (checkBothExternal(GeoId1, GeoId2))
            return;
        if (isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }

        if (isEdge(GeoId2,PosId2)) { // line to line angle

            const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);
            if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg1 = dynamic_cast<const Part::GeomLineSegment*>(geom1);
                const Part::GeomLineSegment *lineSeg2 = dynamic_cast<const Part::GeomLineSegment*>(geom2);

                // find the two closest line ends
                Sketcher::PointPos PosId1,PosId2;
                Base::Vector3d p1a = lineSeg1->getStartPoint();
                Base::Vector3d p1b = lineSeg1->getEndPoint();
                Base::Vector3d p2a = lineSeg2->getStartPoint();
                Base::Vector3d p2b = lineSeg2->getEndPoint();
                double length = DBL_MAX;
                for (int i=0; i <= 1; i++) {
                    for (int j=0; j <= 1; j++) {
                        double tmp = ((j?p2a:p2b)-(i?p1a:p1b)).Length();
                        if (tmp < length) {
                            length = tmp;
                            PosId1 = i ? Sketcher::start : Sketcher::end;
                            PosId2 = j ? Sketcher::start : Sketcher::end;
                        }
                    }
                }

                Base::Vector3d dir1 = ((PosId1 == Sketcher::start) ? 1. : -1.) *
                                      (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
                Base::Vector3d dir2 = ((PosId2 == Sketcher::start) ? 1. : -1.) *
                                      (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());

                // check if the two lines are parallel, in this case an angle is not possible
                Base::Vector3d dir3 = dir1 % dir2;
                if (dir3.Length() < Precision::Intersection()) {
                    Base::Vector3d dist = (p1a - p2a) % dir1;
                    if (dist.Sqr() > Precision::Intersection()) {
                        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Parallel lines"),
                            QObject::tr("An angle constraint cannot be set for two parallel lines."));
                        return;
                    }
                }

                double ActAngle = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                        dir1.x*dir2.x+dir1.y*dir2.y);
                if (ActAngle < 0) {
                    ActAngle *= -1;
                    std::swap(GeoId1,GeoId2);
                    std::swap(PosId1,PosId2);
                }

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActAngle);

                finishDistanceConstraint(this, Obj);
                return;
            }
        } else if (isEdge(GeoId1,PosId1)) { // line angle
            if (GeoId1 < 0) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    GeoId1 < -2 ? QObject::tr("Cannot add an angle constraint on an external geometry!")
                                : QObject::tr("Cannot add an angle constraint on an axis!"));
                return;
            }

            const Part::Geometry *geom = Obj->getGeometry(GeoId1);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg;
                lineSeg = dynamic_cast<const Part::GeomLineSegment*>(geom);
                Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                double ActAngle = atan2(dir.y,dir.x);

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,ActAngle);

                finishDistanceConstraint(this, Obj);
                return;
            }
            else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc;
                arc = dynamic_cast<const Part::GeomArcOfCircle*>(geom);
                double startangle, endangle;
                arc->getRange(startangle, endangle);
                double angle = endangle - startangle;

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,angle);

                finishDistanceConstraint(this, Obj);
                return;
            }
        }
    };

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select one or two lines from the sketch. Or select two edges and a point."));
    return;
}

bool CmdSketcherConstrainAngle::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}

DEF_STD_CMD_A(CmdSketcherConstrainEqual);

CmdSketcherConstrainEqual::CmdSketcherConstrainEqual()
    :Command("Sketcher_ConstrainEqual")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain equal");
    sToolTipText    = QT_TR_NOOP("Create an equality constraint between two lines or between circles and arcs");
    sWhatsThis      = "Sketcher_ConstrainEqual";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_EqualLength";
    sAccel          = "E";
    eType           = ForEdit;
}

void CmdSketcherConstrainEqual::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two edges from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements

    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool lineSel = false, arcSel = false, circSel = false, ellipsSel = false, arcEllipsSel=false, hasAlreadyExternal = false;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);

        if (!isEdge(GeoId,PosId)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two or more compatible edges"));
            return;
        }
        else if (GeoId < 0) {
            if (GeoId == -1 || GeoId == -2) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Sketch axes cannot be used in equality constraints"));
                return;
            }
            else if (hasAlreadyExternal) {
                checkBothExternal(-1,-2); // just for printing the error message
                return;
            }
            else
                hasAlreadyExternal = true;
        }

        const Part::Geometry *geo = Obj->getGeometry(GeoId);
        if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId())
            lineSel = true;
        else if (geo->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())
            arcSel = true;
        else if (geo->getTypeId() != Part::GeomCircle::getClassTypeId()) 
            circSel = true;
        else if (geo->getTypeId() != Part::GeomEllipse::getClassTypeId()) 
            ellipsSel = true;
        else if (geo->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId()) 
            arcEllipsSel = true;
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two or more edges of similar type"));
            return;
        }

        ids.push_back(GeoId);
    }

    if (lineSel && (arcSel || circSel) && (ellipsSel || arcEllipsSel)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two or more edges of similar type"));
        return;
    }

    // undo command open
    openCommand("add equality constraint");
    for (int i=0; i < int(ids.size()-1); i++) {
        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
            selection[0].getFeatName(),ids[i],ids[i+1]);
    }
    // finish the transaction and update
    commitCommand();
    updateActive();

    // clear the selection (convenience)
    getSelection().clearSelection();
}

bool CmdSketcherConstrainEqual::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainSymmetric);

CmdSketcherConstrainSymmetric::CmdSketcherConstrainSymmetric()
    :Command("Sketcher_ConstrainSymmetric")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain symmetrical");
    sToolTipText    = QT_TR_NOOP("Create a symmetry constraint between two points with respect to a line or a third point");
    sWhatsThis      = "Sketcher_ConstrainSymmetric";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Symmetric";
    sAccel          = "S";
    eType           = ForEdit;
}

void CmdSketcherConstrainSymmetric::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                        "or a line and a symmetry point from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();

    if (SubNames.size() != 3 && SubNames.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                        "or a line and a symmetry point from the sketch."));
        return;
    }

    int GeoId1, GeoId2, GeoId3;
    Sketcher::PointPos PosId1, PosId2, PosId3;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (SubNames.size() == 2) {
        checkBothExternal(GeoId1, GeoId2);
        if (isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }
        if (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) {
            const Part::Geometry *geom = Obj->getGeometry(GeoId1);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                if (GeoId1 == GeoId2) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a symmetry constraint between a line and its end points!"));
                    return;
                }

                // undo command open
                openCommand("add symmetric constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId1,Sketcher::start,GeoId1,Sketcher::end,GeoId2,PosId2);

                // finish the transaction and update
                commitCommand();
                updateActive();

                // clear the selection (convenience)
                getSelection().clearSelection();
                return;
            }
        }

        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                        "or a line and a symmetry point from the sketch."));
        return;
    }

    getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

    if (isEdge(GeoId1,PosId1) && isVertex(GeoId3,PosId3)) {
        std::swap(GeoId1,GeoId3);
        std::swap(PosId1,PosId3);
    }
    else if (isEdge(GeoId2,PosId2) && isVertex(GeoId3,PosId3)) {
        std::swap(GeoId2,GeoId3);
        std::swap(PosId2,PosId3);
    }

    if ((GeoId1 < 0 && GeoId2 < 0 && GeoId3 < 0)) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Cannot add a constraint between external geometries!"));
        return;
    }

    if (isVertex(GeoId1,PosId1) &&
        isVertex(GeoId2,PosId2)) {

        if (isEdge(GeoId3,PosId3)) {
            const Part::Geometry *geom = Obj->getGeometry(GeoId3);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                if (GeoId1 == GeoId2 && GeoId2 == GeoId3) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a symmetry constraint between a line and its end points!"));
                    return;
                }

                // undo command open
                openCommand("add symmetric constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d)) ",
                    selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,GeoId3);

                // finish the transaction and update
                commitCommand();
                updateActive();

                // clear the selection (convenience)
                getSelection().clearSelection();
                return;
            }
        }
        else if (isVertex(GeoId3,PosId3)) {
            // undo command open
            openCommand("add symmetric constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,GeoId3,PosId3);

            // finish the transaction and update
            commitCommand();
            updateActive();

            // clear the selection (convenience)
            getSelection().clearSelection();
            return;
        }
    }
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                    "or a line and a symmetry point from the sketch."));
}

bool CmdSketcherConstrainSymmetric::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}

DEF_STD_CMD_A(CmdSketcherConstrainSnellsLaw);

CmdSketcherConstrainSnellsLaw::CmdSketcherConstrainSnellsLaw()
    :Command("Sketcher_ConstrainSnellsLaw")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain refraction (Snell's law')");
    sToolTipText    = QT_TR_NOOP("Create a refraction law (Snell's law) constraint between two endpoints of rays and an edge as an interface.");
    sWhatsThis      = "Sketcher_ConstrainSnellsLaw";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_SnellsLaw";
    sAccel          = "";
    eType           = ForEdit;
}

void CmdSketcherConstrainSnellsLaw::activated(int iMsg)
{
    QString strHelp = QObject::tr("Select two endpoints of lines to act as rays, and"
                                  " an edge representing a boundary. The first"
                                  " selected point corresponds to index n1, second"
                                  " - to n2, and datum value sets the ratio n2/n1.",
                                  "Constraint_SnellsLaw");
    QString strError;

    const char dmbg[] = "Constraint_SnellsLaw";

    try{
        // get the selection
        std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();
        Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1) {
            strError = QObject::tr("Selected objects are not just geometry from one sketch.", dmbg);
            throw(Base::Exception(""));
        }

        // get the needed lists and objects
        const std::vector<std::string> &SubNames = selection[0].getSubNames();

        if (SubNames.size() != 3) {
            strError = QObject::tr("Number of selected objects is not 3 (is %1).", dmbg).arg(SubNames.size());
            throw(Base::Exception(""));
        }

        int GeoId1, GeoId2, GeoId3;
        Sketcher::PointPos PosId1, PosId2, PosId3;
        getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

        //sink the egde to be the last item
        if (isEdge(GeoId1,PosId1) ) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }
        if (isEdge(GeoId2,PosId2) ) {
            std::swap(GeoId2,GeoId3);
            std::swap(PosId2,PosId3);
        }

        //a bunch of validity checks
        if ((GeoId1 < 0 && GeoId2 < 0 && GeoId3 < 0)) {
            strError = QObject::tr("Cannot add a constraint between external geometries!", dmbg);
            throw(Base::Exception(""));
        }

        if (!(isVertex(GeoId1,PosId1) && !isSimpleVertex(Obj, GeoId1, PosId1) &&
              isVertex(GeoId2,PosId2) && !isSimpleVertex(Obj, GeoId2, PosId2) &&
              isEdge(GeoId3,PosId3)   )) {
            strError = QObject::tr("Incompatible geometry is selected!", dmbg);
            throw(Base::Exception(""));
        };

        //the essence.
        //Unlike other constraints, we'll ask for a value immediately.
        QDialog dlg(Gui::getMainWindow());
        Ui::InsertDatum ui_Datum;
        ui_Datum.setupUi(&dlg);
        dlg.setWindowTitle(EditDatumDialog::tr("Refractive index ratio", dmbg));
        ui_Datum.label->setText(EditDatumDialog::tr("Ratio n2/n1:", dmbg));
        Base::Quantity init_val;
        init_val.setUnit(Base::Unit());
        init_val.setValue(0.0);

        ui_Datum.labelEdit->setValue(init_val);
        ui_Datum.labelEdit->setParamGrpPath(QByteArray("User parameter:BaseApp/History/SketcherRefrIndexRatio"));
        ui_Datum.labelEdit->setToLastUsedValue();
        ui_Datum.labelEdit->selectNumber();

        if (dlg.exec() != QDialog::Accepted) return;
        ui_Datum.labelEdit->pushToHistory();

        Base::Quantity newQuant = ui_Datum.labelEdit->value();
        double n2divn1 = newQuant.getValue();

        //add constraint
        openCommand("add Snell's law constraint");

        if (! IsPointAlreadyOnCurve(GeoId2,GeoId1,PosId1,Obj))
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);

        if (! IsPointAlreadyOnCurve(GeoId3,GeoId1,PosId1,Obj))
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId3);

        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('SnellsLaw',%d,%d,%d,%d,%d,%.12f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,GeoId3,n2divn1);

        commitCommand();
        updateActive();

        // clear the selection (convenience)
        getSelection().clearSelection();
    } catch (Base::Exception &e) {
        if (strError.isEmpty()) strError = QString::fromLatin1(e.what());
        if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Error"), strError + strHelp);
    }
}

bool CmdSketcherConstrainSnellsLaw::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}


DEF_STD_CMD_A(CmdSketcherConstrainInternalAlignment);

CmdSketcherConstrainInternalAlignment::CmdSketcherConstrainInternalAlignment()
    :Command("Sketcher_ConstrainInternalAlignment")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain InternalAlignment");
    sToolTipText    = QT_TR_NOOP("Constraint an element to be aligned with the internal geometry of another element");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_InternalAlignment";
    sAccel          = "Ctrl+A";
    eType           = ForEdit;
}

void CmdSketcherConstrainInternalAlignment::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least one ellipse and one edge from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements
    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least one ellipse and one edge from the sketch."));
        return;
    }

    std::vector<int> pointids;
    std::vector<int> lineids;
    std::vector<int> ellipseids;
    std::vector<int> arcsofellipseids;
    
    bool hasAlreadyExternal = false;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);
        
        if (GeoId < 0) {
            if (GeoId == -1 || GeoId == -2) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Sketch axes cannot be used in internal alignment constraint"));
                return;
            }
            else if (hasAlreadyExternal) {
                checkBothExternal(-1,-2); // just for printing the error message
                return;
            }
            else
                hasAlreadyExternal = true;
        }

        const Part::Geometry *geo = Obj->getGeometry(GeoId);
        
        if (geo->getTypeId() == Part::GeomPoint::getClassTypeId())
            pointids.push_back(GeoId);
        else if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId())
            lineids.push_back(GeoId);
        else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId())
            ellipseids.push_back(GeoId);
        else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
            arcsofellipseids.push_back(GeoId);
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two or more compatible edges"));
            return;
        }
    }
    
    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[SubNames.size()-1], Obj, GeoId, PosId); // last selected element
    
    const Part::Geometry *geo = Obj->getGeometry(GeoId);
       
    // Currently it is only supported for ellipses
    if(geo->getTypeId() == Part::GeomEllipse::getClassTypeId()) {
    
        // Priority list
        // EllipseMajorDiameter    = 1,
        // EllipseMinorDiameter    = 2,
        // EllipseFocus1           = 3,
        // EllipseFocus2           = 4
        
        if(ellipseids.size()>1){
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("You can not internally constraint an ellipse on other ellipse. Select only one ellipse."));
            return;        
        }
                
        if (pointids.size()>2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Maximum 2 points are supported."));
            return;
        }
        
        if (lineids.size()>2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Maximum 2 lines are supported."));
            return;
        }
        
        // look for which internal constraints are already applied
        bool major=false;
        bool minor=false;
        bool focus1=false;
        bool focus2=false;
        bool extra_elements=false;
        
        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
        
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->Second == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        major=true;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        minor=true;
                        break;
                    case Sketcher::EllipseFocus1: 
                        focus1=true;
                        break;
                    case Sketcher::EllipseFocus2: 
                        focus2=true;
                        break;
                }
            }
        }
        
        if(major && minor && focus1 && focus2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Nothing to constraint"),
            QObject::tr("Currently all internal geometry of the ellipse is already exposed."));            
            return;
        }
        
        if((!(focus1 && focus2) && pointids.size()>=1) || // if some element is missing and we are adding an element of that type
           (!(major && minor) && lineids.size()>=1) ){
        
            openCommand("add internal alignment constraint");
            
            if(pointids.size()>=1)
            {
                if(!focus1) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus1',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[0],Sketcher::start,ellipseids[0]);
                }
                else if(!focus2) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus2',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[0],Sketcher::start,ellipseids[0]);  
                    focus2=true;
                }
                else
                    extra_elements=true;
            }
            
            if(pointids.size()==2)
            {
                if(!focus2) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus2',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[1],Sketcher::start,ellipseids[0]);  
                }
                else
                    extra_elements=true;
            }
            
            if(lineids.size()>=1)
            {
                if(!major) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMajorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),lineids[0],ellipseids[0]);
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[0]));
                    
                    if(!geo->Construction) 
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[0]);
                    
                }
                else if(!minor) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMinorDiameter',%d,%d)) ",
                            selection[0].getFeatName(),lineids[0],ellipseids[0]);      
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[0]));
                    
                    if(!geo->Construction)
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[0]);
                    
                    minor=true;
                }
                else
                    extra_elements=true;
            }
            if(lineids.size()==2)
            {
                if(!minor){
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMinorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),lineids[1],ellipseids[0]);
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[1]));
                    
                    if(!geo->Construction)
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[1]);
                }
                else
                    extra_elements=true;
            }

            // finish the transaction and update
            commitCommand();
            updateActive();
            
            if(extra_elements){
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Extra elements"),
                QObject::tr("More elements than possible for the given ellipse were provided. These were ignored."));
            }

            // clear the selection (convenience)
            getSelection().clearSelection();
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Extra elements"),
            QObject::tr("More elements than possible for the given ellipse were provided. These were ignored."));
        }
    }
    else if(geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId()) {
    
        // Priority list
        // EllipseMajorDiameter    = 1,
        // EllipseMinorDiameter    = 2,
        // EllipseFocus1           = 3,
        // EllipseFocus2           = 4
        
        if(arcsofellipseids.size()>1){
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("You can not internally constraint an arc of ellipse on other arc of ellipse. Select only one arc of ellipse."));
            return;        
        }
        
        if(ellipseids.size()>0){
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("You can not internally constraint an ellipse on an arc of ellipse. Select only one ellipse or arc of ellipse."));
            return;        
        }
                
        if (pointids.size()>2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Maximum 2 points are supported."));
            return;
        }
        
        if (lineids.size()>2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Maximum 2 lines are supported."));
            return;
        }
        
        // look for which internal constraints are already applied
        bool major=false;
        bool minor=false;
        bool focus1=false;
        bool focus2=false;
        bool extra_elements=false;
        
        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();
        
        for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                it != vals.end(); ++it) {
            if((*it)->Type == Sketcher::InternalAlignment && (*it)->First == GeoId)
            {
                switch((*it)->AlignmentType){
                    case Sketcher::EllipseMajorDiameter:
                        major=true;
                        break;
                    case Sketcher::EllipseMinorDiameter:
                        minor=true;
                        break;
                    case Sketcher::EllipseFocus1: 
                        focus1=true;
                        break;
                    case Sketcher::EllipseFocus2: 
                        focus2=true;
                        break;
                }
            }
        }
        
        if(major && minor && focus1 && focus2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Nothing to constraint"),
            QObject::tr("Currently all internal geometry of the arc of ellipse is already exposed."));            
            return;
        }
        
        if((!(focus1 && focus2) && pointids.size()>=1) || // if some element is missing and we are adding an element of that type
           (!(major && minor) && lineids.size()>=1) ){
        
            openCommand("add internal alignment constraint");
            
            if(pointids.size()>=1)
            {
                if(!focus1) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus1',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[0],Sketcher::start,arcsofellipseids[0]);
                }
                else if(!focus2) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus2',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[0],Sketcher::start,arcsofellipseids[0]);  
                    focus2=true;
                }
                else
                    extra_elements=true;
            }
            
            if(pointids.size()==2)
            {
                if(!focus2) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseFocus2',%d,%d,%d)) ",
                        selection[0].getFeatName(),pointids[1],Sketcher::start,arcsofellipseids[0]);  
                }
                else
                    extra_elements=true;
            }
            
            if(lineids.size()>=1)
            {
                if(!major) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMajorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),lineids[0],arcsofellipseids[0]);
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[0]));
                    
                    if(!geo->Construction) 
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[0]);
                    
                }
                else if(!minor) {
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMinorDiameter',%d,%d)) ",
                            selection[0].getFeatName(),lineids[0],arcsofellipseids[0]);      
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[0]));
                    
                    if(!geo->Construction)
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[0]);
                    
                    minor=true;
                }
                else
                    extra_elements=true;
            }
            if(lineids.size()==2)
            {
                if(!minor){
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('InternalAlignment:EllipseMinorDiameter',%d,%d)) ",
                        selection[0].getFeatName(),lineids[1],arcsofellipseids[0]);
                    
                    const Part::GeomLineSegment *geo = static_cast<const Part::GeomLineSegment *>(Obj->getGeometry(lineids[1]));
                    
                    if(!geo->Construction)
                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.toggleConstruction(%d) ",selection[0].getFeatName(),lineids[1]);
                }
                else
                    extra_elements=true;
            }

            // finish the transaction and update
            commitCommand();
            updateActive();
            
            if(extra_elements){
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Extra elements"),
                QObject::tr("More elements than possible for the given ellipse were provided. These were ignored."));
            }

            // clear the selection (convenience)
            getSelection().clearSelection();
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Extra elements"),
            QObject::tr("More elements than possible for the given arc of ellipse were provided. These were ignored."));
        }
    }
    else {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Currently internal geometry is only supported for ellipse or arc of ellipse. The last selected element must be an ellipse or an arc of ellipse."));
    }
}

bool CmdSketcherConstrainInternalAlignment::isActive(void)
{
    return isCreateConstraintActive( getActiveGuiDocument() );
}

void CreateSketcherCommandsConstraints(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherConstrainHorizontal());
    rcCmdMgr.addCommand(new CmdSketcherConstrainVertical());
    rcCmdMgr.addCommand(new CmdSketcherConstrainLock());
    rcCmdMgr.addCommand(new CmdSketcherConstrainCoincident());
    rcCmdMgr.addCommand(new CmdSketcherConstrainParallel());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPerpendicular());
    rcCmdMgr.addCommand(new CmdSketcherConstrainTangent());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistance());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceX());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceY());
    rcCmdMgr.addCommand(new CmdSketcherConstrainRadius());
    rcCmdMgr.addCommand(new CmdSketcherConstrainAngle());
    rcCmdMgr.addCommand(new CmdSketcherConstrainEqual());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPointOnObject());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSymmetric());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSnellsLaw());
    rcCmdMgr.addCommand(new CmdSketcherConstrainInternalAlignment());

}
