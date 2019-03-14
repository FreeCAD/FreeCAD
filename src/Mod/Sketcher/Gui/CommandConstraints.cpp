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
# include <QPainter>
#endif

#include <Base/Tools.h>
#include <Base/Tools2D.h>
#include <App/Application.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>
#include <Gui/Action.h>
#include <Gui/BitmapFactory.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>
#include <Mod/Sketcher/App/Sketch.h>

#include "ViewProviderSketch.h"
#include "DrawSketchHandler.h"
#include "ui_InsertDatum.h"
#include "EditDatumDialog.h"
#include "CommandConstraints.h"

using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

/***** Creation Mode ************/
namespace SketcherGui
{
    enum ConstraintCreationMode {
        Driving,
        Reference
    };
}

ConstraintCreationMode constraintCreationMode = Driving;

void ActivateHandler(Gui::Document *doc, DrawSketchHandler *handler);

bool isCreateGeoActive(Gui::Document *doc);

bool isCreateConstraintActive(Gui::Document *doc)
{
    if (doc) {
        // checks if a Sketch View provider is in Edit and is in no special mode
        if (doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            if (static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit())
                ->getSketchMode() == ViewProviderSketch::STATUS_NONE) {
                if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0)
                    return true;
            }
        }
    }
    return false;
}

void openEditDatumDialog(Sketcher::SketchObject* sketch, int ConstrNbr)
{
    const std::vector<Sketcher::Constraint *> &Constraints = sketch->Constraints.getValues();
    Sketcher::Constraint* Constr = Constraints[ConstrNbr];

    // Return if constraint doesn't have editable value
    if (Constr->isDimensional()) {

        QDialog dlg(Gui::getMainWindow());
        Ui::InsertDatum ui_ins_datum;
        ui_ins_datum.setupUi(&dlg);

        double datum = Constr->getValue();
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
        else if (Constr->Type == Sketcher::Diameter) {
            dlg.setWindowTitle(EditDatumDialog::tr("Insert diameter"));
            init_val.setUnit(Base::Unit::Length);
            ui_ins_datum.label->setText(EditDatumDialog::tr("Diameter:"));
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
        init_val.setValue(datum);

        ui_ins_datum.labelEdit->setValue(init_val);
        ui_ins_datum.labelEdit->selectNumber();
        ui_ins_datum.labelEdit->bind(sketch->Constraints.createPath(ConstrNbr));
        ui_ins_datum.name->setText(Base::Tools::fromStdString(Constr->Name));

        if (dlg.exec()) {
            Base::Quantity newQuant = ui_ins_datum.labelEdit->value();
            if (newQuant.isQuantity() || (Constr->Type == Sketcher::SnellsLaw && newQuant.isDimensionless())) {
                // save the value for the history
                ui_ins_datum.labelEdit->pushToHistory();

                double newDatum = newQuant.getValue();

                try {
                    if (ui_ins_datum.labelEdit->hasExpression())
                        ui_ins_datum.labelEdit->apply();
                    else
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                                sketch->getNameInDocument(),
                                                ConstrNbr, newDatum, (const char*)newQuant.getUnit().getString().toUtf8());

                    QString constraintName = ui_ins_datum.name->text().trimmed();
                    if (Base::Tools::toStdString(constraintName) != sketch->Constraints[ConstrNbr]->Name) {
                        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(constraintName.toUtf8().constData());
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.renameConstraint(%d, u'%s')",
                                                sketch->getNameInDocument(),
                                                ConstrNbr, escapedstr.c_str());
                    }
                    Gui::Command::commitCommand();

                    if (sketch->noRecomputes && sketch->ExpressionEngine.depsAreTouched()) {
                        sketch->ExpressionEngine.execute();
                        sketch->solve();
                    }

                    tryAutoRecompute(sketch);
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    Gui::Command::abortCommand();

                    tryAutoRecomputeIfNotSolve(sketch);
                }
            }
        }
        else {
            // command canceled
            Gui::Command::abortCommand();

            tryAutoRecomputeIfNotSolve(sketch); // we have to update the solver after this aborted addition.
        }
    }
}

// Utility method to avoid repeating the same code over and over again
void finishDistanceConstraint(Gui::Command* cmd, Sketcher::SketchObject* sketch, bool isDriven=true)
{
    // Get the latest constraint
    const std::vector<Sketcher::Constraint *> &ConStr = sketch->Constraints.getValues();
    Sketcher::Constraint *constr = ConStr[ConStr.size() -1];

    // Guess some reasonable distance for placing the datum text
    Gui::Document *doc = cmd->getActiveGuiDocument();
    float sf = 1.f;
    if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
        SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
        sf = vp->getScaleFactor();

        constr->LabelDistance = 2. * sf;
        vp->draw(false,false); // Redraw
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);

    // Ask for the value of the distance immediately
    if (show && isDriven) {
        openEditDatumDialog(sketch, ConStr.size() - 1);
    }
    else {
        // no dialog was shown so commit the command
        cmd->commitCommand();
    }

    tryAutoRecompute(sketch);
    cmd->getSelection().clearSelection();
}

void showNoConstraintBetweenExternal()
{
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                         QObject::tr("Cannot add a constraint between two external geometries!"));
}

void showNoConstraintBetweenFixedGeometry()
{
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                         QObject::tr("Cannot add a constraint between two fixed geometries! Fixed geometries involve external geometry, blocked geometry or special points as B-spline knot points."));
}

bool SketcherGui::checkBothExternal(int GeoId1, int GeoId2)
{
    if (GeoId1 == Constraint::GeoUndef || GeoId2 == Constraint::GeoUndef)
        return false;
    else
        return (GeoId1 < 0 && GeoId2 < 0);
}

bool SketcherGui::checkBothExternalOrConstructionPoints(const Sketcher::SketchObject* Obj,int GeoId1, int GeoId2)
{
    if (GeoId1 == Constraint::GeoUndef || GeoId2 == Constraint::GeoUndef)
        return false;
    else
        return (GeoId1 < 0 && GeoId2 < 0) || (isConstructionPoint(Obj,GeoId1) && isConstructionPoint(Obj,GeoId2)) ||
        (GeoId1 < 0 && isConstructionPoint(Obj,GeoId2)) || (GeoId2 < 0 && isConstructionPoint(Obj,GeoId1));
}

bool SketcherGui::isPointOrSegmentFixed(const Sketcher::SketchObject* Obj, int GeoId)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId == Constraint::GeoUndef)
        return false;
    else
        return checkConstraint(vals, Sketcher::Block, GeoId, Sketcher::none) || GeoId <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId);
}

bool SketcherGui::areBothPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId1 == Constraint::GeoUndef || GeoId2 == Constraint::GeoUndef)
        return false;
    else
        return ((checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::none) || GeoId1 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId1)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::none) || GeoId2 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId2)));
}

bool SketcherGui::areAllPointsOrSegmentsFixed(const Sketcher::SketchObject* Obj, int GeoId1, int GeoId2, int GeoId3)
{
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    if (GeoId1 == Constraint::GeoUndef || GeoId2 == Constraint::GeoUndef || GeoId3 == Constraint::GeoUndef)
        return false;
    else
        return ((checkConstraint(vals, Sketcher::Block, GeoId1, Sketcher::none) || GeoId1 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId1)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId2, Sketcher::none) || GeoId2 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId2)) &&
                (checkConstraint(vals, Sketcher::Block, GeoId3, Sketcher::none) || GeoId3 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId3)));
}

void SketcherGui::getIdsFromName(const std::string &name, const Sketcher::SketchObject* Obj,
                                 int &GeoId, PointPos &PosId)
{
    GeoId = Constraint::GeoUndef;
    PosId = Sketcher::none;

    if (name.size() > 4 && name.substr(0,4) == "Edge") {
        GeoId = std::atoi(name.substr(4,4000).c_str()) - 1;
    }
    else if (name.size() == 9 && name.substr(0,9) == "RootPoint") {
        GeoId = Sketcher::GeoEnum::RtPnt;
        PosId = Sketcher::start;
    }
    else if (name.size() == 6 && name.substr(0,6) == "H_Axis")
        GeoId = Sketcher::GeoEnum::HAxis;
    else if (name.size() == 6 && name.substr(0,6) == "V_Axis")
        GeoId = Sketcher::GeoEnum::VAxis;
    else if (name.size() > 12 && name.substr(0,12) == "ExternalEdge")
        GeoId = Sketcher::GeoEnum::RefExt + 1 - std::atoi(name.substr(12,4000).c_str());
    else if (name.size() > 6 && name.substr(0,6) == "Vertex") {
        int VtId = std::atoi(name.substr(6,4000).c_str()) - 1;
        Obj->getGeoVertexIndex(VtId,GeoId,PosId);
    }
}

bool inline SketcherGui::isVertex(int GeoId, PointPos PosId)
{
    return (GeoId != Constraint::GeoUndef && PosId != Sketcher::none);
}

bool inline SketcherGui::isEdge(int GeoId, PointPos PosId)
{
    return (GeoId != Constraint::GeoUndef && PosId == Sketcher::none);
}

bool SketcherGui::isSimpleVertex(const Sketcher::SketchObject* Obj, int GeoId, PointPos PosId)
{
    if (PosId == Sketcher::start && (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis))
        return true;
    const Part::Geometry *geo = Obj->getGeometry(GeoId);
    if (geo->getTypeId() == Part::GeomPoint::getClassTypeId())
        return true;
    else if (PosId == Sketcher::mid)
        return true;
    else
        return false;
}

bool SketcherGui::isConstructionPoint(const Sketcher::SketchObject* Obj, int GeoId)
{
    const Part::Geometry * geo = Obj->getGeometry(GeoId);
    return (geo && geo->getTypeId() == Part::GeomPoint::getClassTypeId() && geo->Construction == true);
}

bool SketcherGui::IsPointAlreadyOnCurve(int GeoIdCurve, int GeoIdPoint, Sketcher::PointPos PosIdPoint, Sketcher::SketchObject* Obj)
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
void SketcherGui::makeTangentToEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                             const Part::GeomEllipse *ellipse,
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
)
{

    Base::Vector3d center=ellipse->getCenter();
    double majord=ellipse->getMajorRadius();
    double minord=ellipse->getMinorRadius();
    double phi=atan2(ellipse->getMajorAxisDir().y, ellipse->getMajorAxisDir().x);

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
    double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomaly by the polar

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

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an arc of ellipse
/// geom2 => any of an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfEllipseviaNewPoint(Sketcher::SketchObject* Obj,
                                             const Part::GeomArcOfEllipse *aoe,
                                             const Part::Geometry *geom2,
                                             int geoId1,
                                             int geoId2
)
{

    Base::Vector3d center=aoe->getCenter();
    double majord=aoe->getMajorRadius();
    double minord=aoe->getMinorRadius();
    double phi=atan2(aoe->getMajorAxisDir().y, aoe->getMajorAxisDir().x);

    Base::Vector3d center2;

    if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomCircle *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfCircle *>(geom2))->getCenter();

    Base::Vector3d direction=center2-center;
    double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomaly by the polar

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

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an arc of hyperbola
/// geom2 => any of an arc of hyperbola, an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfHyperbolaviaNewPoint(Sketcher::SketchObject* Obj,
                                                       const Part::GeomArcOfHyperbola *aoh,
                                                       const Part::Geometry *geom2,
                                                       int geoId1,
                                                       int geoId2
)
{

    Base::Vector3d center=aoh->getCenter();
    double majord=aoh->getMajorRadius();
    double minord=aoh->getMinorRadius();
    Base::Vector3d dirmaj = aoh->getMajorAxisDir();
    double phi=atan2(dirmaj.y, dirmaj.x);
    double df = sqrt(majord*majord+minord*minord);
    Base::Vector3d focus = center+df*dirmaj; // positive focus

    Base::Vector3d center2;

    if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
        const Part::GeomArcOfHyperbola *aoh2 = static_cast<const Part::GeomArcOfHyperbola *>(geom2);
        Base::Vector3d dirmaj2 = aoh2->getMajorAxisDir();
        double majord2 = aoh2->getMajorRadius();
        double minord2 = aoh2->getMinorRadius();
        double df2 = sqrt(majord2*majord2+minord2*minord2);
        center2 = aoh2->getCenter()+df2*dirmaj2; // positive focus
    }
    else if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomCircle *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfCircle *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *l2 = static_cast<const Part::GeomLineSegment *>(geom2);
        center2= (l2->getStartPoint() + l2->getEndPoint())/2;
    }

    Base::Vector3d direction=center2-focus;
    double tapprox=atan2(direction.y,direction.x)-phi;

    Base::Vector3d PoH = Base::Vector3d(center.x+majord*cosh(tapprox)*cos(phi)-minord*sinh(tapprox)*sin(phi),
                                        center.y+majord*cosh(tapprox)*sin(phi)+minord*sinh(tapprox)*cos(phi), 0);

    try {
        // Add a point
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                Obj->getNameInDocument(), PoH.x,PoH.y);
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

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();

    tryAutoRecompute(Obj);
}

/// Makes a simple tangency constraint using extra point + tangent via point
/// geom1 => an arc of parabola
/// geom2 => any of an arc of parabola, an arc of hyperbola an arc of ellipse, a circle, or an arc (of circle)
/// NOTE: A command must be opened before calling this function, which this function
/// commits or aborts as appropriate. The reason is for compatibility reasons with
/// other code e.g. "Autoconstraints" in DrawSketchHandler.cpp
void SketcherGui::makeTangentToArcOfParabolaviaNewPoint(Sketcher::SketchObject* Obj,
                                                       const Part::GeomArcOfParabola *aop,
                                                       const Part::Geometry *geom2,
                                                       int geoId1,
                                                       int geoId2
)
{

    Base::Vector3d focus = aop->getFocus();

    Base::Vector3d center2;

    if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfParabola *>(geom2))->getFocus();
    else if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId()){
        const Part::GeomArcOfHyperbola *aoh2 = static_cast<const Part::GeomArcOfHyperbola *>(geom2);
        Base::Vector3d dirmaj2 = aoh2->getMajorAxisDir();
        double majord2 = aoh2->getMajorRadius();
        double minord2 = aoh2->getMinorRadius();
        double df2 = sqrt(majord2*majord2+minord2*minord2);
        center2 = aoh2->getCenter()+df2*dirmaj2; // positive focus
    }
    else if( geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomEllipse::getClassTypeId())
        center2= (static_cast<const Part::GeomEllipse *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomCircle *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
        center2= (static_cast<const Part::GeomArcOfCircle *>(geom2))->getCenter();
    else if( geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
        const Part::GeomLineSegment *l2 = static_cast<const Part::GeomLineSegment *>(geom2);
        center2= (l2->getStartPoint() + l2->getEndPoint())/2;
    }

    Base::Vector3d direction = center2-focus;

    Base::Vector3d PoP = focus + direction / 2;

    try {
        // Add a point
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                                Obj->getNameInDocument(), PoP.x,PoP.y);
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

        tryAutoRecompute(Obj);
        return;
    }

    Gui::Command::commitCommand();
    tryAutoRecompute(Obj);
}

std::string SketcherGui::getStrippedPythonExceptionString(const Base::Exception& e)
{
    std::string msg = e.what();

    if( msg.length() > 26 && msg.substr(0,26) == "FreeCAD exception thrown (") {
        return msg.substr(26, msg.length()-27);
    }
    else
        return msg;
}

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj, bool &autoremoveredundants)
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
    bool autoRecompute = hGrp->GetBool("AutoRecompute",false);
    bool autoRemoveRedundants = hGrp->GetBool("AutoRemoveRedundants",false);

    // We need to make sure the solver has right redundancy information before trying to remove the redundants.
    // for example if a non-driving constraint has been added.
    if(autoRemoveRedundants && autoRecompute)
        obj->solve();

    if(autoRemoveRedundants)
        obj->autoRemoveRedundants();

    if (autoRecompute)
        Gui::Command::updateActive();

    autoremoveredundants = autoRemoveRedundants;

    return autoRecompute;
}

bool SketcherGui::tryAutoRecompute(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    return tryAutoRecompute(obj,autoremoveredundants);
}

void SketcherGui::tryAutoRecomputeIfNotSolve(Sketcher::SketchObject* obj)
{
    bool autoremoveredundants;

    if(!tryAutoRecompute(obj,autoremoveredundants)) {
        obj->solve();

        if(autoremoveredundants) {
            obj->autoRemoveRedundants();
        }
    }
}

bool SketcherGui::checkConstraint(const std::vector< Sketcher::Constraint * > &vals, ConstraintType type, int geoid, PointPos pos)
{
    for (std::vector< Sketcher::Constraint * >::const_iterator itc= vals.begin(); itc != vals.end(); ++itc) {
        if ((*itc)->Type == type && (*itc)->First == geoid && (*itc)->FirstPos == pos){
            return true;
        }
    }

    return false;
}


void SketcherGui::doEndpointTangency(Sketcher::SketchObject* Obj, Gui::SelectionObject &selection,
                                     int GeoId1, int GeoId2, PointPos PosId1, PointPos PosId2){
    // This code supports simple B-spline endpoint tangency to any other geometric curve
    const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
    const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

    if (geom1 && geom2 &&
       (geom1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
        geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId())){

        if(geom1->getTypeId() != Part::GeomBSplineCurve::getClassTypeId()) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }
        // GeoId1 is the B-spline now
        } // end of code supports simple B-spline endpoint tangency

        Gui::Command::doCommand(
            Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d,%d)) ",
                                selection.getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
}


namespace SketcherGui {

struct SelIdPair{
    int GeoId;
    Sketcher::PointPos PosId;
};

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

        SketchObj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
        SketchSubNames = selection[0].getSubNames();
    } else if(selection.size() == 2) {
        if(selection[0].getObject()->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())){
            SketchObj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
            // check if the none sketch object is the support of the sketch
            if(selection[1].getObject() != SketchObj->Support.getValue()){
                ErrorMsg = QObject::tr("Only sketch and its support is allowed to select");
                return-1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[1].getObject()->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()));
            SketchSubNames  = selection[0].getSubNames();
            SupportSubNames = selection[1].getSubNames();

        } else if (selection[1].getObject()->getTypeId().isDerivedFrom(Sketcher::SketchObject::getClassTypeId())) {
            SketchObj = static_cast<Sketcher::SketchObject*>(selection[1].getObject());
            // check if the none sketch object is the support of the sketch
            if(selection[0].getObject() != SketchObj->Support.getValue()){
                ErrorMsg = QObject::tr("Only sketch and its support is allowed to select");
                return -1;
            }
            // assume always a Part::Feature derived object as support
            assert(selection[0].getObject()->getTypeId().isDerivedFrom(Part::Feature::getClassTypeId()));
            SketchSubNames  = selection[1].getSubNames();
            SupportSubNames = selection[0].getSubNames();

        } else {
            ErrorMsg = QObject::tr("One of the selected has to be on the sketch");
            return -1;
        }
    }

    return Items.size();
}

} // namespace SketcherGui

/* Constrain commands =======================================================*/

namespace SketcherGui {
    /**
     * @brief The SelType enum
     * Types of sketch elements that can be (pre)selected. The root/origin and the
     * axes are put up separately so that they can be specifically disallowed, for
     * example, when in lock, horizontal, or vertical constraint modes.
     */
    enum SelType {
        SelUnknown = 0,
        SelVertex = 1,
        SelVertexOrRoot = 64,
        SelRoot = 2,
        SelEdge = 4,
        SelEdgeOrAxis = 128,
        SelHAxis = 8,
        SelVAxis = 16,
        SelExternalEdge = 32
    };

    /**
     * @brief The GenericConstraintSelection class
     * SelectionFilterGate with changeable filters. In a constraint creation mode
     * like point on object, if the first selection object can be a point, the next
     * has to be a curve for the constraint to make sense. Thus filters are
     * changeable so that same filter can be kept on while in one mode.
     */
    class GenericConstraintSelection : public Gui::SelectionFilterGate
    {
        App::DocumentObject* object;
    public:
        GenericConstraintSelection(App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)0)
            , object(obj), allowedSelTypes(0)
        {}

        bool allow(App::Document *, App::DocumentObject *pObj, const char *sSubName)
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            if (    (allowedSelTypes & (SelRoot | SelVertexOrRoot) && element.substr(0,9) == "RootPoint") ||
                    (allowedSelTypes & (SelVertex  | SelVertexOrRoot) && element.substr(0,6) == "Vertex") ||
                    (allowedSelTypes & (SelEdge | SelEdgeOrAxis) && element.substr(0,4) == "Edge") ||
                    (allowedSelTypes & (SelHAxis | SelEdgeOrAxis) && element.substr(0,6) == "H_Axis") ||
                    (allowedSelTypes & (SelVAxis | SelEdgeOrAxis) && element.substr(0,6) == "V_Axis") ||
                    (allowedSelTypes & SelExternalEdge && element.substr(0,12) == "ExternalEdge"))
                return true;

            return false;
        }

        void setAllowedSelTypes(unsigned int types) {
            if (types < 256) allowedSelTypes = types;
        }

    protected:
        int allowedSelTypes;
    };
}

/**
 * @brief The CmdSketcherConstraint class
 * Superclass for all sketcher constraints to ease generation of constraint
 * creation modes.
 */
class CmdSketcherConstraint : public Gui::Command
{
    friend class DrawSketchHandlerGenConstraint;
public:
    CmdSketcherConstraint(const char* name)
            : Command(name) {}

    virtual ~CmdSketcherConstraint(){}

    virtual const char* className() const
    { return "CmdSketcherConstraint"; }

protected:
    /**
     * @brief allowedSelSequences
     * Each element is a vector representing sequence of selections allowable.
     * DrawSketchHandlerGenConstraint will use these to filter elements and
     * generate sequences to be passed to applyConstraint().
     * Whenever any sequence is completed, applyConstraint() is called, so it's
     * best to keep them prefix-free.
     * Be mindful that when SelVertex and SelRoot are given preference over
     * SelVertexOrRoot, and similar for edges/axes. Thus if a vertex is selected
     * when SelVertex and SelVertexOrRoot are both applicable, only sequences with
     * SelVertex will be continue.
     *
     * TODO: Introduce structs to allow keeping first selection
     */
    std::vector<std::vector<SketcherGui::SelType> > allowedSelSequences;

    const char** constraintCursor = 0;

    virtual void applyConstraint(std::vector<SelIdPair> &, int) {}
    virtual void activated(int /*iMsg*/);
    virtual bool isActive(void)
    { return isCreateGeoActive(getActiveGuiDocument()); }
};

/* XPM */
static const char *cursor_genericconstraint[]={
"32 32 3 1",
"  c None",
". c #FFFFFF",
"+ c #FF0000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",
"                                ",};

class DrawSketchHandlerGenConstraint: public DrawSketchHandler
{
public:
    DrawSketchHandlerGenConstraint(const char* cursor[], CmdSketcherConstraint *_cmd)
        : constraintCursor(cursor), cmd(_cmd), seqIndex(0) {}
    virtual ~DrawSketchHandlerGenConstraint()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void activated(ViewProviderSketch *)
    {
        selFilterGate = new GenericConstraintSelection(sketchgui->getObject());

        resetOngoingSequences();

        selSeq.clear();

        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(selFilterGate);

        // Constrain icon size in px
        int iconSize = 16;
        QPixmap cursorPixmap(cursor_genericconstraint),
                icon = Gui::BitmapFactory().pixmap(cmd->sPixmap).scaledToWidth(iconSize);
        QPainter cursorPainter;
        cursorPainter.begin(&cursorPixmap);
        cursorPainter.drawPixmap(16, 16, icon);
        cursorPainter.end();
        setCursor(cursorPixmap, 7, 7);
    }

    virtual void mouseMove(Base::Vector2d /*onSketchPos*/) {}

    virtual bool pressButton(Base::Vector2d /*onSketchPos*/)
    {
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos)
    {
        SelIdPair selIdPair;
        selIdPair.GeoId = Constraint::GeoUndef;
        selIdPair.PosId = Sketcher::none;
        std::stringstream ss;
        SelType newSelType = SelUnknown;

        //For each SelType allowed, check if button is released there and assign it to selIdPair
        int VtId = sketchgui->getPreselectPoint();
        int CrvId = sketchgui->getPreselectCurve();
        int CrsId = sketchgui->getPreselectCross();
        if (allowedSelTypes & (SelRoot | SelVertexOrRoot) && CrsId == 0) {
            selIdPair.GeoId = Sketcher::GeoEnum::RtPnt;
            selIdPair.PosId = Sketcher::start;
            newSelType = (allowedSelTypes & SelRoot) ? SelRoot : SelVertexOrRoot;
            ss << "RootPoint";
        }
        else if (allowedSelTypes & (SelVertex | SelVertexOrRoot) && VtId >= 0) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId,
                                                            selIdPair.GeoId,
                                                            selIdPair.PosId);
            newSelType = (allowedSelTypes & SelVertex) ? SelVertex : SelVertexOrRoot;
            ss << "Vertex" << VtId + 1;
        }
        else if (allowedSelTypes & (SelEdge | SelEdgeOrAxis) && CrvId >= 0) {
            selIdPair.GeoId = CrvId;
            newSelType = (allowedSelTypes & SelEdge) ? SelEdge : SelEdgeOrAxis;
            ss << "Edge" << CrvId + 1;
        }
        else if (allowedSelTypes & (SelHAxis | SelEdgeOrAxis) && CrsId == 1) {
            selIdPair.GeoId = Sketcher::GeoEnum::HAxis;
            newSelType = (allowedSelTypes & SelHAxis) ? SelHAxis : SelEdgeOrAxis;
            ss << "H_Axis";
        }
        else if (allowedSelTypes & (SelVAxis | SelEdgeOrAxis) && CrsId == 2) {
            selIdPair.GeoId = Sketcher::GeoEnum::VAxis;
            newSelType = (allowedSelTypes & SelVAxis) ? SelVAxis : SelEdgeOrAxis;
            ss << "V_Axis";
        }
        else if (allowedSelTypes & SelExternalEdge && CrvId <= Sketcher::GeoEnum::RefExt) {
            //TODO: Figure out how this works
            selIdPair.GeoId = CrvId;
            newSelType = SelExternalEdge;
            ss << "ExternalEdge" << Sketcher::GeoEnum::RefExt + 1 - CrvId;
        }

        if (selIdPair.GeoId == Constraint::GeoUndef) {
            // If mouse is released on "blank" space, start over
            selSeq.clear();
            resetOngoingSequences();
            Gui::Selection().clearSelection();
        }
        else {
            // If mouse is released on something allowed, select it and move forward
            selSeq.push_back(selIdPair);
            Gui::Selection().addSelection(sketchgui->getSketchObject()->getDocument()->getName(),
                                          sketchgui->getSketchObject()->getNameInDocument(),
                                          ss.str().c_str(),
                                          onSketchPos.x,
                                          onSketchPos.y,
                                          0.f);
            _tempOnSequences.clear();
            allowedSelTypes = 0;
            for (std::set<int>::iterator token = ongoingSequences.begin();
                 token != ongoingSequences.end(); ++token) {
                if ((cmd->allowedSelSequences).at(*token).at(seqIndex) == newSelType) {
                    if (seqIndex == (cmd->allowedSelSequences).at(*token).size()-1) {
                        // One of the sequences is completed. Pass to cmd->applyConstraint
                        cmd->applyConstraint(selSeq, *token); // replace arg 2 by ongoingToken

                        selSeq.clear();
                        resetOngoingSequences();

                        return true;
                    }
                    _tempOnSequences.insert(*token);
                    allowedSelTypes = allowedSelTypes | (cmd->allowedSelSequences).at(*token).at(seqIndex+1);
                }
            }

            // Progress to next seqIndex
            std::swap(_tempOnSequences, ongoingSequences);
            seqIndex++;
            selFilterGate->setAllowedSelTypes(allowedSelTypes);
        }

        return true;
    }

protected:
    const char** constraintCursor;
    CmdSketcherConstraint* cmd;

    GenericConstraintSelection* selFilterGate = nullptr;

    std::vector<SelIdPair> selSeq;
    unsigned int allowedSelTypes = 0;

    /// indices of currently ongoing sequences in cmd->allowedSequences
    std::set<int> ongoingSequences, _tempOnSequences;
    /// Index within the selection sequences active
    unsigned int seqIndex;

    void resetOngoingSequences() {
        ongoingSequences.clear();
        for (unsigned int i = 0; i < cmd->allowedSelSequences.size(); i++) {
            ongoingSequences.insert(i);
        }
        seqIndex = 0;

        // Estimate allowed selections from the first types in allowedSelTypes
        allowedSelTypes = 0;
        for (std::vector< std::vector< SelType > >::const_iterator it = cmd->allowedSelSequences.begin();
             it != cmd->allowedSelSequences.end(); ++it) {
            allowedSelTypes = allowedSelTypes | (*it).at(seqIndex);
        }
        selFilterGate->setAllowedSelTypes(allowedSelTypes);

        Gui::Selection().clearSelection();
    }
};

void CmdSketcherConstraint::activated(int /*iMsg*/)
{
    ActivateHandler(getActiveGuiDocument(),
            new DrawSketchHandlerGenConstraint(constraintCursor, this));
    getSelection().clearSelection();
}

// ============================================================================

/* XPM */
static const char *cursor_createhoriconstraint[]={
"32 32 3 1",
"+ c white",
"# c red",
". c None",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"+++++...+++++...................",
"................................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+......#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............############.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................"};

class CmdSketcherConstrainHorizontal : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainHorizontal();
    virtual ~CmdSketcherConstrainHorizontal(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainHorizontal"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);

};

CmdSketcherConstrainHorizontal::CmdSketcherConstrainHorizontal()
    :CmdSketcherConstraint("Sketcher_ConstrainHorizontal")
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

    allowedSelSequences = {{SelEdge}};
    constraintCursor = cursor_createhoriconstraint;
}

void CmdSketcherConstrainHorizontal::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select an edge from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    std::vector<int> edgegeoids;
    std::vector<int> pointgeoids;
    std::vector<Sketcher::PointPos> pointpos;

    int fixedpoints = 0;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName((*it), Obj, GeoId, PosId);


        if (isEdge(GeoId,PosId)) {// it is an edge
            const Part::Geometry *geo = Obj->getGeometry(GeoId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge already has a Horizontal/Vertical/Block constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none){
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                        QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a vertical constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }
            edgegeoids.push_back(GeoId);
        }
        else if(isVertex(GeoId,PosId)) {
            // can be a point, a construction point, an external point or root

            if(isPointOrSegmentFixed(Obj, GeoId))
                fixedpoints++;

            pointgeoids.push_back(GeoId);
            pointpos.push_back(PosId);
        }
    }

    if (edgegeoids.empty() && pointgeoids.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("The selected item(s) can't accept a horizontal constraint!"));
        return;
    }

    // if there is at least one edge selected, ignore the point alignment functionality
    if (!edgegeoids.empty()) {
        // undo command open
        openCommand("add horizontal constraint");
        for (std::vector<int>::iterator it=edgegeoids.begin(); it != edgegeoids.end(); it++) {
            // issue the actual commands to create the constraint
            doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Horizontal',%d)) "
                    ,selection[0].getFeatName(),*it);
        }
    }
    else if (fixedpoints <= 1) { // pointgeoids
        // undo command open
        openCommand("add horizontal alignment");
        std::vector<int>::iterator it;
        std::vector<Sketcher::PointPos>::iterator itp;
        for (it=pointgeoids.begin(), itp=pointpos.begin(); it != std::prev(pointgeoids.end()) && itp != std::prev(pointpos.end()); it++,itp++) {
            // issue the actual commands to create the constraint
            doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Horizontal',%d,%d,%d,%d)) "
            ,selection[0].getFeatName(),*it,*itp,*std::next(it),*std::next(itp));
        }
    }
    else { // vertex mode, fixedpoints > 1
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("There are more than one fixed point selected. Select a maximum of one fixed point!"));
        return;
    }
    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainHorizontal::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
    case 0: // {Edge}
        // create the constraint
        SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

        int CrvId = selSeq.front().GeoId;
        if (CrvId != -1) {
            const Part::Geometry *geo = Obj->getGeometry(CrvId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge already has a Horizontal/Vertical/Block constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == CrvId){
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                        QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == CrvId) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a vertical constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == CrvId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }

            // undo command open
            Gui::Command::openCommand("add horizontal constraint");
            // issue the actual commands to create the constraint
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Horizontal',%d)) ",
                      sketchgui->getObject()->getNameInDocument(),CrvId);
            // finish the transaction and update
            Gui::Command::commitCommand();

            tryAutoRecompute(Obj);
        }

        break;
    }
}

// ================================================================================

static const char *cursor_createvertconstraint[]={
"32 32 3 1",
"+ c white",
"# c red",
". c None",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"+++++...+++++...................",
"................................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+......#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
".............#..........#.......",
"..............#........#........",
"...............#......#.........",
"................#....#..........",
".................#..#...........",
"..................##............",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................"};

class CmdSketcherConstrainVertical : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainVertical();
    virtual ~CmdSketcherConstrainVertical(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainVertical"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);

};

CmdSketcherConstrainVertical::CmdSketcherConstrainVertical()
    :CmdSketcherConstraint("Sketcher_ConstrainVertical")
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

    allowedSelSequences = {{SelEdge}};
    constraintCursor = cursor_createvertconstraint;
}

void CmdSketcherConstrainVertical::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select an edge from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    std::vector<int> edgegeoids;
    std::vector<int> pointgeoids;
    std::vector<Sketcher::PointPos> pointpos;

    int fixedpoints = 0;
    // go through the selected subelements
    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {
        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName((*it), Obj, GeoId, PosId);


        if (isEdge(GeoId,PosId)) {// it is an edge
            const Part::Geometry *geo = Obj->getGeometry(GeoId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge already has a Horizontal/Vertical/Block constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none){
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                                         QObject::tr("The selected edge already has a vertical constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == GeoId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }
            edgegeoids.push_back(GeoId);
        }
        else if(isVertex(GeoId,PosId)) {
            // can be a point, a construction point, an external point, root or a blocked geometry
            if(isPointOrSegmentFixed(Obj, GeoId))
                fixedpoints++;

            pointgeoids.push_back(GeoId);
            pointpos.push_back(PosId);
        }
    }

    if (edgegeoids.empty() && pointgeoids.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("The selected item(s) can't accept a vertical constraint!"));
        return;
    }

    // if there is at least one edge selected, ignore the point alignment functionality
    if (!edgegeoids.empty()) {
        // undo command open
        openCommand("add vertical constraint");
        for (std::vector<int>::iterator it=edgegeoids.begin(); it != edgegeoids.end(); it++) {
            // issue the actual commands to create the constraint
            doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Vertical',%d)) "
            ,selection[0].getFeatName(),*it);
        }
    }
    else if (fixedpoints <= 1) { // vertex mode, maximum one fixed point
        // undo command open
        openCommand("add vertical alignment");
        std::vector<int>::iterator it;
        std::vector<Sketcher::PointPos>::iterator itp;
        for (it=pointgeoids.begin(), itp=pointpos.begin(); it != std::prev(pointgeoids.end()) && itp != std::prev(pointpos.end()); it++,itp++) {
            // issue the actual commands to create the constraint
            doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Vertical',%d,%d,%d,%d)) "
            ,selection[0].getFeatName(),*it,*itp,*std::next(it),*std::next(itp));
        }
    }
    else { // vertex mode, fixedpoints > 1
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                             QObject::tr("There are more than one fixed points selected. Select a maximum of one fixed point!"));
        return;
    }

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainVertical::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
    case 0: // {Edge}
        // create the constraint
        SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

        int CrvId = selSeq.front().GeoId;
        if (CrvId != -1) {
            const Part::Geometry *geo = Obj->getGeometry(CrvId);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge already has a Horizontal or Vertical constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == CrvId){
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == CrvId) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                                         QObject::tr("The selected edge already has a vertical constraint!"));
                    return;
                }
                // check if the edge already has a Block constraint
                if ((*it)->Type == Sketcher::Block && (*it)->First == CrvId && (*it)->FirstPos == Sketcher::none) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                         QObject::tr("The selected edge already has a Block constraint!"));
                    return;
                }
            }

            // undo command open
            Gui::Command::openCommand("add vertical constraint");
            // issue the actual commands to create the constraint
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Vertical',%d)) ",
                      sketchgui->getObject()->getNameInDocument(),CrvId);
            // finish the transaction and update
            Gui::Command::commitCommand();
            tryAutoRecompute(Obj);
        }

        break;
    }
}

// ======================================================================================

/* XPM */
static const char *cursor_createlock[]={
"32 32 3 1",
"+ c white",
"# c red",
". c None",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"+++++...+++++...................",
"................................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+..........###............",
"................######..........",
"...............##....##.........",
"..............##......##........",
"..............##......##........",
".............############.......",
".............############.......",
".............############.......",
".............############.......",
".............############.......",
".............############.......",
".............############.......",
".............############.......",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................"};

class CmdSketcherConstrainLock : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainLock();
    virtual ~CmdSketcherConstrainLock(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainLock"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainLock::CmdSketcherConstrainLock()
    :CmdSketcherConstraint("Sketcher_ConstrainLock")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain lock");
    sToolTipText    = QT_TR_NOOP("Create a lock constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainLock";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ConstrainLock";
    eType           = ForEdit;

    allowedSelSequences = {{SelVertex}};
    constraintCursor = cursor_createlock;
}

void CmdSketcherConstrainLock::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select vertices from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    std::vector<int> GeoId;
    std::vector<Sketcher::PointPos> PosId;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        int GeoIdt;
        Sketcher::PointPos PosIdt;
        getIdsFromName((*it), Obj, GeoIdt, PosIdt);
        GeoId.push_back(GeoIdt);
        PosId.push_back(PosIdt);

        if ((it != std::prev(SubNames.end()) && (isEdge(GeoIdt,PosIdt) || (GeoIdt < 0 && GeoIdt >= Sketcher::GeoEnum::VAxis))) ||
            (it == std::prev(SubNames.end()) && isEdge(GeoIdt,PosIdt)) ) {
            if(selection.size() == 1) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Select one vertex from the sketch other than the origin."));
            }
            else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Select only vertices from the sketch. The last selected vertex may be the origin."));
            }
            // clear the selection (convenience)
            getSelection().clearSelection();
            return;
        }
    }

    int lastconstraintindex = Obj->Constraints.getSize()-1;

    if( GeoId.size() == 1 ) { // absolute mode
        // check if the edge already has a Block constraint
        bool edgeisblocked = false;

        if ( isPointOrSegmentFixed(Obj, GeoId[0])) {
            edgeisblocked = true;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId[0],PosId[0]);

        // undo command open
        openCommand("add fixed constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
                                selection[0].getFeatName(),GeoId[0],PosId[0],pnt.x);
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
                                selection[0].getFeatName(),GeoId[0],PosId[0],pnt.y);

        lastconstraintindex+=2;

        if (edgeisblocked || GeoId[0] <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId[0]) || constraintCreationMode==Reference) {
            // it is a constraint on a external line, make it non-driving

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                    selection[0].getFeatName(),lastconstraintindex-2,"False");

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                    selection[0].getFeatName(),lastconstraintindex,"False");
        }
    }
    else {
        std::vector<int>::const_iterator itg;
        std::vector<Sketcher::PointPos>::const_iterator itp;

        Base::Vector3d pntr = Obj->getPoint(GeoId.back(),PosId.back());

        // check if the edge already has a Block constraint
        bool refpointfixed = false;

        if ( isPointOrSegmentFixed(Obj, GeoId.back()))
            refpointfixed = true;

        for (itg = GeoId.begin(), itp = PosId.begin(); itg != std::prev(GeoId.end()) && itp != std::prev(PosId.end()); ++itp, ++itg) {
            bool pointfixed = false;

            if ( isPointOrSegmentFixed(Obj, *itg))
                pointfixed = true;

            Base::Vector3d pnt = Obj->getPoint(*itg,*itp);

            // undo command open
            openCommand("add relative lock constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
                                    selection[0].getFeatName(),*itg,*itp,GeoId.back(),PosId.back(),pntr.x-pnt.x);

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                                    selection[0].getFeatName(),*itg,*itp,GeoId.back(),PosId.back(),pntr.y-pnt.y);
            lastconstraintindex+=2;

            if ( (refpointfixed && pointfixed) || constraintCreationMode==Reference) {
                // it is a constraint on a external line, make it non-driving

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                        selection[0].getFeatName(),lastconstraintindex-1,"False");

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                        selection[0].getFeatName(),lastconstraintindex,"False");
            }
        }
    }

    // finish the transaction and update
    commitCommand();
    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainLock::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
    case 0: // {Vertex}
        // Create the constraints
        SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        // check if the edge already has a Block constraint
        bool pointfixed = false;

        if ( isPointOrSegmentFixed(Obj, selSeq.front().GeoId))
            pointfixed = true;

        Base::Vector3d pnt = Obj->getPoint(selSeq.front().GeoId, selSeq.front().PosId);

        // undo command open
        Gui::Command::openCommand("add fixed constraint");
        Gui::Command::doCommand(
            Gui::Command::Doc, "App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX', %d, %d, %f)) ",
            sketchgui->getObject()->getNameInDocument(), selSeq.front().GeoId, selSeq.front().PosId, pnt.x);
        Gui::Command::doCommand(
            Gui::Command::Doc, "App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY', %d, %d, %f)) ",
            sketchgui->getObject()->getNameInDocument(), selSeq.front().GeoId, selSeq.front().PosId, pnt.y);

        if (pointfixed || constraintCreationMode==Reference) {
            // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.setDriving(%i, %s)",
            sketchgui->getObject()->getNameInDocument(), ConStr.size()-2, "False");

            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.%s.setDriving(%i, %s)",
            sketchgui->getObject()->getNameInDocument(), ConStr.size()-1, "False");
        }

        // finish the transaction and update
        Gui::Command::commitCommand();

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool autoRecompute = hGrp->GetBool("AutoRecompute", false);

        if(autoRecompute)
            Gui::Command::updateActive();
        break;
    }
}

void CmdSketcherConstrainLock::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_ConstrainLock_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_ConstrainLock"));
        break;
    }
}

// ======================================================================================

/* XPM */
static const char *cursor_createblock[]={
    "32 32 3 1",
    "+ c white",
    "# c red",
    ". c None",
    "......+.........................",
    "......+.........................",
    "......+.........................",
    "......+.........................",
    "......+.........................",
    "................................",
    "+++++...+++++...................",
    "................................",
    "......+.........................",
    "......+.........................",
    "......+.........................",
    "......+.........................",
    "......+..........###............",
    "....................##..........",
    ".....................##.........",
    "......................##........",
    "......................##........",
    ".............############.......",
    ".............###########........",
    ".............##########.........",
    ".............########...........",
    ".............#######............",
    ".............########...........",
    ".............##########.........",
    ".............###########........",
    ".............############.......",
    "......................##........",
    "......................##........",
    ".....................##.........",
    "....................##..........",
    ".................###............",
    "................................"};

class CmdSketcherConstrainBlock : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainBlock();
    virtual ~CmdSketcherConstrainBlock(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainBlock"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainBlock::CmdSketcherConstrainBlock()
:CmdSketcherConstraint("Sketcher_ConstrainBlock")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain Block");
    sToolTipText    = QT_TR_NOOP("Create a Block constraint on the selected item");
    sWhatsThis      = "Sketcher_ConstrainBlock";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ConstrainBlock";
    eType           = ForEdit;

    allowedSelSequences = {{SelEdge}};
    constraintCursor = cursor_createblock;
}

void CmdSketcherConstrainBlock::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                            new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                    QObject::tr("Select vertices from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // Check that the solver does not report redundant/conflicting constraints
    if(Obj->getLastSolverStatus()!=GCS::Success || Obj->getLastHasConflicts() || Obj->getLastHasRedundancies()) {
      QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong solver status"),
                           QObject::tr("A Block constraint cannot be added if the sketch is unsolved or there are redundant and/or conflicting constraints."));
      return;
    }

    std::vector<int> GeoId;
    const std::vector< Sketcher::Constraint * > &vals = Obj->Constraints.getValues();

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        int GeoIdt;
        Sketcher::PointPos PosIdt;
        getIdsFromName((*it), Obj, GeoIdt, PosIdt);

        if ( isVertex(GeoIdt,PosIdt) || GeoIdt < 0 ) {
            if(selection.size() == 1) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                        QObject::tr("Select one edge from the sketch."));
            }
            else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                        QObject::tr("Select only edges from the sketch."));
            }
            // clear the selection
            getSelection().clearSelection();
            return;
        }

        // check if the edge already has a Block constraint
        if ( checkConstraint(vals, Sketcher::Block, GeoIdt, Sketcher::none)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                                 QObject::tr("The selected edge already has a Block constraint!"));
            return;
        }

        GeoId.push_back(GeoIdt);
    }

    for (std::vector<int>::iterator itg = GeoId.begin(); itg != GeoId.end(); ++itg) {
        // undo command open
        openCommand("add block constraint");

        try {

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Block',%d)) ",
                selection[0].getFeatName(),(*itg));

        } catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Error"),
                                 QString::fromLatin1(e.what()));

            Gui::Command::abortCommand();

            tryAutoRecompute(Obj);
            return;
        }

        commitCommand();
        tryAutoRecompute(Obj);
    }

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainBlock::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
        case 0: // {Edge}
        {
            // Create the constraints
            SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());

            // check if the edge already has a Block constraint
            const std::vector< Sketcher::Constraint * > &vals = static_cast<Sketcher::SketchObject *>(sketchgui->getObject())->Constraints.getValues();

            if ( checkConstraint(vals, Sketcher::Block, selSeq.front().GeoId, Sketcher::none)) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                                     QObject::tr("The selected edge already has a Block constraint!"));
                return;
            }

            // undo command open
            openCommand("add block constraint");

            try {

                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Block',%d)) ",
                                        sketchgui->getObject()->getNameInDocument(),selSeq.front().GeoId);

            } catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                QMessageBox::warning(Gui::getMainWindow(),
                                     QObject::tr("Error"),
                                     QString::fromLatin1(e.what()));

                Gui::Command::abortCommand();

                tryAutoRecompute(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
                return;
            }

            commitCommand();
            tryAutoRecompute(static_cast<Sketcher::SketchObject *>(sketchgui->getObject()));
        }
        break;
    default:
        break;
    }
}


// ======================================================================================

/* XPM */
static const char *cursor_createcoincident[]={
"32 32 3 1",
"+ c white",
"# c red",
". c None",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"+++++...+++++...................",
"................................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"......+.........................",
"................................",
"................................",
"................................",
".................####...........",
"................######..........",
"...............########.........",
"...............########.........",
"...............########.........",
"...............########.........",
"................######..........",
".................####...........",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................",
"................................"};

class DrawSketchHandlerCoincident: public DrawSketchHandler
{
public:
    DrawSketchHandlerCoincident()
    {
        GeoId1 = GeoId2 = Constraint::GeoUndef;
        PosId1 = PosId2 = Sketcher::none;
    }
    virtual ~DrawSketchHandlerCoincident()
    {
        Gui::Selection().rmvSelectionGate();
    }

    virtual void activated(ViewProviderSketch *)
    {
        Gui::Selection().rmvSelectionGate();
        GenericConstraintSelection* selFilterGate = new GenericConstraintSelection(sketchgui->getObject());
        selFilterGate->setAllowedSelTypes(SelVertex|SelRoot);
        Gui::Selection().addSelectionGate(selFilterGate);
        setCursor(QPixmap(cursor_createcoincident), 7, 7);
    }

    virtual void mouseMove(Base::Vector2d onSketchPos) {Q_UNUSED(onSketchPos);}

    virtual bool pressButton(Base::Vector2d onSketchPos)
    {
        Q_UNUSED(onSketchPos);
        return true;
    }

    virtual bool releaseButton(Base::Vector2d onSketchPos)
    {
        int VtId = sketchgui->getPreselectPoint();
        int CrsId = sketchgui->getPreselectCross();
        std::stringstream ss;
        int GeoId_temp;
        Sketcher::PointPos PosId_temp;

        if (VtId != -1) {
            sketchgui->getSketchObject()->getGeoVertexIndex(VtId,GeoId_temp,PosId_temp);
            ss << "Vertex" << VtId + 1;
        }
        else if (CrsId == 0){
            GeoId_temp = Sketcher::GeoEnum::RtPnt;
            PosId_temp = Sketcher::start;
            ss << "RootPoint";
        }
        else {
            GeoId1 = GeoId2 = Constraint::GeoUndef;
            PosId1 = PosId2 = Sketcher::none;
            Gui::Selection().clearSelection();

            return true;
        }


        if (GeoId1 == Constraint::GeoUndef) {
            GeoId1 = GeoId_temp;
            PosId1 = PosId_temp;
            Gui::Selection().addSelection(sketchgui->getSketchObject()->getDocument()->getName(),
                                          sketchgui->getSketchObject()->getNameInDocument(),
                                          ss.str().c_str(),
                                          onSketchPos.x,
                                          onSketchPos.y,
                                          0.f);
        }
        else {
            GeoId2 = GeoId_temp;
            PosId2 = PosId_temp;

            // Apply the constraint
            Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(sketchgui->getObject());

            // undo command open
            Gui::Command::openCommand("add coincident constraint");

            // check if this coincidence is already enforced (even indirectly)
            bool constraintExists = Obj->arePointsCoincident(GeoId1,PosId1,GeoId2,PosId2);
            if (!constraintExists && (GeoId1 != GeoId2)) {
                Gui::Command::doCommand(
                    Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
                    sketchgui->getObject()->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2);
                Gui::Command::commitCommand();
            }
            else {
                Gui::Command::abortCommand();
            }
        }

        return true;
    }
protected:
    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
};

class CmdSketcherConstrainCoincident : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainCoincident();
    virtual ~CmdSketcherConstrainCoincident(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainCoincident"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainCoincident::CmdSketcherConstrainCoincident()
    :CmdSketcherConstraint("Sketcher_ConstrainCoincident")
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

    allowedSelSequences = {{SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex}};
    constraintCursor = cursor_createcoincident;
}

void CmdSketcherConstrainCoincident::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two or more points from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

        // check if the edge already has a Block constraint
        if ( areBothPointsOrSegmentsFixed(Obj,GeoId1,GeoId2) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }

        // check if this coincidence is already enforced (even indirectly)
        bool constraintExists=Obj->arePointsCoincident(GeoId1,PosId1,GeoId2,PosId2);

        // check for a preexisting edge-to-edge tangency
        const std::vector< Constraint * > &cvals = Obj->Constraints.getValues();

        int j=0;
        for (std::vector<Constraint *>::const_iterator it = cvals.begin(); it != cvals.end(); ++it,++j) {
            if( (*it)->Type == Sketcher::Tangent &&
                (*it)->FirstPos == Sketcher::none && (*it)->SecondPos == Sketcher::none &&
                (*it)->Third == Constraint::GeoUndef &&
                (((*it)->First == GeoId1 && (*it)->Second == GeoId2) ||
                ((*it)->Second == GeoId1 && (*it)->First == GeoId2)) ) {

                Gui::Command::openCommand("swap edge tangency with ptp tangency");

                if(constraintExists) {
                    // try to remove any pre-existing direct coincident constraints
                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.delConstraintOnPoint(%i,%i)",
                                            selection[0].getFeatName(), GeoId1, PosId1);
                }

                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.delConstraint(%i)"
                                        ,selection[0].getFeatName(), j);

                doEndpointTangency(Obj, selection[0], GeoId1, GeoId2, PosId1, PosId2);

                commitCommand();
                tryAutoRecomputeIfNotSolve(Obj);

                ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

                if(hGrp->GetBool("NotifyConstraintSubstitutions", true)) {
                    QMessageBox::information(Gui::getMainWindow(), QObject::tr("Constraint Substitution"),
                                            QObject::tr("Endpoint to endpoint tangency was applied instead."));
                }

                getSelection().clearSelection();
                return;
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

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainCoincident::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
    case 0: // {SelVertex, SelVertexOrRoot}
    case 1: // {SelRoot, SelVertex}
        // create the constraint
        SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        int GeoId1 = selSeq.at(0).GeoId, GeoId2 = selSeq.at(1).GeoId;
        Sketcher::PointPos PosId1 = selSeq.at(0).PosId, PosId2 = selSeq.at(1).PosId;

        // check if the edge already has a Block constraint
        if ( areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }

        // undo command open
        Gui::Command::openCommand("add coincident constraint");

        // check if this coincidence is already enforced (even indirectly)
        bool constraintExists = Obj->arePointsCoincident(GeoId1, PosId1, GeoId2, PosId2);
        if (!constraintExists && (GeoId1 != GeoId2)) {
            Gui::Command::doCommand(
                Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident', %d, %d, %d, %d)) ",
                sketchgui->getObject()->getNameInDocument(), GeoId1, PosId1, GeoId2, PosId2);
            Gui::Command::commitCommand();
        }
        else {
            Gui::Command::abortCommand();
        }

        break;
    }
}

// ======================================================================================

class CmdSketcherConstrainDistance : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistance();
    virtual ~CmdSketcherConstrainDistance(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainDistance"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainDistance::CmdSketcherConstrainDistance()
    :CmdSketcherConstraint("Sketcher_ConstrainDistance")
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

    allowedSelSequences = {{SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex},
                           {SelEdge}, {SelExternalEdge},
                           {SelVertex, SelEdgeOrAxis}, {SelRoot, SelEdge},
                           {SelVertex, SelExternalEdge}, {SelRoot, SelExternalEdge}};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainDistance::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                new DrawSketchHandlerGenConstraint(constraintCursor, this));

            getSelection().clearSelection();
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select vertexes from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

    bool arebothpointsorsegmentsfixed=areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2);

    if (isVertex(GeoId1,PosId1) && (GeoId2 == Sketcher::GeoEnum::VAxis || GeoId2 == Sketcher::GeoEnum::HAxis)) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if ((isVertex(GeoId1,PosId1) || GeoId1 == Sketcher::GeoEnum::VAxis || GeoId1 == Sketcher::GeoEnum::HAxis) &&
        isVertex(GeoId2,PosId2)) { // point to point distance

        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);

        if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::none) {
            PosId1 = Sketcher::start;

            openCommand("add distance from horizontal axis constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,pnt2.y);
        }
        else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::none) {
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

        if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);
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
            lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            Base::Vector3d pnt1 = lineSeg->getStartPoint();
            Base::Vector3d pnt2 = lineSeg->getEndPoint();
            Base::Vector3d d = pnt2-pnt1;
            double ActDist = std::abs(-pnt.x*d.y+pnt.y*d.x+pnt1.x*pnt2.y-pnt2.x*pnt1.y) / d.Length();

            openCommand("add point to line Distance constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,ActDist);

            if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                selection[0].getFeatName(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);

            return;
        }
    }
    else if (isEdge(GeoId1,PosId1)) { // line length
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Cannot add a length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed=isPointOrSegmentFixed(Obj,GeoId1);

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Length();

            openCommand("add length constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                selection[0].getFeatName(),GeoId1,ActLength);

            if (arebothpointsorsegmentsfixed || GeoId1 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId1) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                selection[0].getFeatName(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);

            return;
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or one point and one line or two points from the sketch."));
    return;
}

void CmdSketcherConstrainDistance::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none;

    bool arebothpointsorsegmentsfixed=areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2);

    switch (seqIndex) {
    case 0: // {SelVertex, SelVertexOrRoot}
    case 1: // {SelRoot, SelVertex}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId; PosId2 = selSeq.at(1).PosId;

        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);

        if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::none) {
            PosId1 = Sketcher::start;

            openCommand("add distance from horizontal axis constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,pnt2.y);
        }
        else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::none) {
            PosId1 = Sketcher::start;

            openCommand("add distance from vertical axis constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,pnt2.x);
        }
        else {
            Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);

            openCommand("add point to point distance constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,(pnt2-pnt1).Length());
        }

        if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            Obj->getNameInDocument(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    }
    case 2: // {SelEdge}
    case 3: // {SelExternalEdge}
    {
        GeoId1 = GeoId2 = selSeq.at(0).GeoId;
        PosId1 = Sketcher::start; PosId2 = Sketcher::end;

        arebothpointsorsegmentsfixed=isPointOrSegmentFixed(Obj,GeoId1);

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            double ActLength = (lineSeg->getEndPoint()-lineSeg->getStartPoint()).Length();

            openCommand("add length constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,ActLength);

            if (arebothpointsorsegmentsfixed || GeoId1 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId1) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                Obj->getNameInDocument(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("This constraint does not make sense for non-linear curves"));
        }

        return;
    }
    case 4: // {SelVertex, SelEdgeOrAxis}
    case 5: // {SelRoot, SelEdge}
    case 6: // {SelVertex, SelExternalEdge}
    case 7: // {SelRoot, SelExternalEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId; PosId2 = selSeq.at(1).PosId;

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        const Part::Geometry *geom = Obj->getGeometry(GeoId2);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg;
            lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
            Base::Vector3d pnt1 = lineSeg->getStartPoint();
            Base::Vector3d pnt2 = lineSeg->getEndPoint();
            Base::Vector3d d = pnt2-pnt1;
            double ActDist = std::abs(-pnt.x*d.y+pnt.y*d.x+pnt1.x*pnt2.y-pnt2.x*pnt1.y) / d.Length();

            openCommand("add point to line Distance constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Distance',%d,%d,%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,ActDist);

            if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                Obj->getNameInDocument(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);
        }

        return;
    }
    default:
        break;
    }
}

void CmdSketcherConstrainDistance::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Length_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Length"));
        break;
    }
}

// ======================================================================================

/* XPM */
static const char * cursor_createpointonobj[] = {
"32 32 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #FF0000",
"      .                         ",
"      .                     ++++",
"      .                   ++++++",
"      .                +++++++  ",
"      .              ++++++     ",
"                   ++++++       ",
".....   .....     +++++         ",
"                +++++           ",
"      .    +++ ++++             ",
"      .  +++++++++              ",
"      .  ++++++++               ",
"      . +++++++++               ",
"      . +++++++++               ",
"        +++++++++               ",
"         +++++++                ",
"        ++++++++                ",
"       +++++++                  ",
"       +++                      ",
"      +++                       ",
"     +++                        ",
"     +++                        ",
"    +++                         ",
"    +++                         ",
"   +++                          ",
"   +++                          ",
"   ++                           ",
"  +++                           ",
"  ++                            ",
" +++                            ",
" +++                            ",
" ++                             ",
" ++                             "};

class CmdSketcherConstrainPointOnObject : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainPointOnObject();
    virtual ~CmdSketcherConstrainPointOnObject(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainPointOnObject"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

//DEF_STD_CMD_A(CmdSketcherConstrainPointOnObject);

CmdSketcherConstrainPointOnObject::CmdSketcherConstrainPointOnObject()
    :CmdSketcherConstraint("Sketcher_ConstrainPointOnObject")
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

    allowedSelSequences = {{SelVertex, SelEdgeOrAxis}, {SelRoot, SelEdge},
                           {SelVertex, SelExternalEdge},
                           {SelEdge, SelVertexOrRoot}, {SelEdgeOrAxis, SelVertex},
                           {SelExternalEdge, SelVertex}};
    constraintCursor = cursor_createpointonobj;

}

void CmdSketcherConstrainPointOnObject::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    //count curves and points
    std::vector<SelIdPair> points;
    std::vector<SelIdPair> curves;
    for (std::size_t i = 0  ;  i < SubNames.size()  ;  i++){
        SelIdPair id;
        getIdsFromName(SubNames[i], Obj, id.GeoId, id.PosId);
        if (isEdge(id.GeoId, id.PosId))
            curves.push_back(id);
        if (isVertex(id.GeoId, id.PosId))
            points.push_back(id);
    }

    if ((points.size() == 1 && curves.size() >= 1) ||
        (points.size() >= 1 && curves.size() == 1)) {

        openCommand("add point on object constraint");
        int cnt = 0;
        for (std::size_t iPnt = 0;  iPnt < points.size();  iPnt++) {
            for (std::size_t iCrv = 0;  iCrv < curves.size();  iCrv++) {
                if (areBothPointsOrSegmentsFixed(Obj, points[iPnt].GeoId, curves[iCrv].GeoId)){
                    showNoConstraintBetweenFixedGeometry();
                    continue;
                }
                if (points[iPnt].GeoId == curves[iCrv].GeoId)
                    continue; //constraining a point of an element onto the element is a bad idea...

                const Part::Geometry *geom = Obj->getGeometry(curves[iCrv].GeoId);

                if( geom && geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ){
                    // unsupported until normal to B-spline at any point implemented.
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                         QObject::tr("Point on B-spline edge currently unsupported."));
                    continue;
                }

                cnt++;
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    selection[0].getFeatName(),points[iPnt].GeoId, points[iPnt].PosId, curves[iCrv].GeoId);
            }
        }
        if (cnt) {
            commitCommand();
            getSelection().clearSelection();
        } else {
            abortCommand();
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("None of the selected points were constrained onto the respective curves, either "
                            "because they are parts of the same element, or because they are both external geometry."));
        }
        return;
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select either one point and several curves, or one curve and several points. "
                    "You have selected %1 curves and %2 points.").arg(curves.size()).arg(points.size()));
    return;
}

void CmdSketcherConstrainPointOnObject::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    int GeoIdVt, GeoIdCrv;
    Sketcher::PointPos PosIdVt;

    switch (seqIndex) {
    case 0: // {SelVertex, SelEdgeOrAxis}
    case 1: // {SelRoot, SelEdge}
    case 2: // {SelVertex, SelExternalEdge}
        GeoIdVt = selSeq.at(0).GeoId; GeoIdCrv = selSeq.at(1).GeoId;
        PosIdVt = selSeq.at(0).PosId;

        break;
    case 3: // {SelEdge, SelVertexOrRoot}
    case 4: // {SelEdgeOrAxis, SelVertex}
    case 5: // {SelExternalEdge, SelVertex}
        GeoIdVt = selSeq.at(1).GeoId; GeoIdCrv = selSeq.at(0).GeoId;
        PosIdVt = selSeq.at(1).PosId;

        break;
    default:
        return;
    }

    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    openCommand("add point on object constraint");
    bool allOK = true;

    if (areBothPointsOrSegmentsFixed(Obj, GeoIdVt, GeoIdCrv)){
        showNoConstraintBetweenFixedGeometry();
        allOK = false;
    }
    if (GeoIdVt == GeoIdCrv)
        allOK = false; //constraining a point of an element onto the element is a bad idea...

    const Part::Geometry *geom = Obj->getGeometry(GeoIdCrv);

    if( geom && geom->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ){
        // unsupported until normal to B-spline at any point implemented.
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Point on B-spline edge currently unsupported."));
        abortCommand();

        return;
    }

    if (allOK) {
        Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                sketchgui->getObject()->getNameInDocument(), GeoIdVt, PosIdVt, GeoIdCrv);

        commitCommand();
        tryAutoRecompute(Obj);
    }
    else {
        abortCommand();
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("None of the selected points were constrained onto the respective curves, either "
                        "because they are parts of the same element, or because they are both external geometry."));
    }
    return;
}

// ======================================================================================

class CmdSketcherConstrainDistanceX : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistanceX();
    virtual ~CmdSketcherConstrainDistanceX(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainDistanceX"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainDistanceX::CmdSketcherConstrainDistanceX()
    :CmdSketcherConstraint("Sketcher_ConstrainDistanceX")
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

    allowedSelSequences = {{SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex},
                           {SelEdge}, {SelExternalEdge}}; // Can't do single vertex because its a prefix for 2 vertices
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainDistanceX::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        }
        else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

    bool arebothpointsorsegmentsfixed=areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2);

    if (GeoId2 == Sketcher::GeoEnum::HAxis || GeoId2 == Sketcher::GeoEnum::VAxis) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::none) // reject horizontal axis from selection
        GeoId1 = Constraint::GeoUndef;
    else if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::none) {
        GeoId1 = Sketcher::GeoEnum::HAxis;
        PosId1 = Sketcher::start;
    }

    if (isEdge(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef)  { // horizontal length of a line
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                            QObject::tr("Cannot add a horizontal length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj,GeoId1);

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            //convert to as if two endpoints of the line have been selected
            PosId1 = Sketcher::start;
            GeoId2 = GeoId1;
            PosId2 = Sketcher::end;
        }
    }
    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // point to point horizontal distance

        Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
        double ActLength = pnt2.x-pnt1.x;

        //negative sign avoidance: swap the points to make value positive
        if (ActLength < -Precision::Confusion()) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
            std::swap(pnt1, pnt2);
            ActLength = -ActLength;
        }

        openCommand("add point to point horizontal distance constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

        if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed x-coordinate

        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add a fixed x-coordinate constraint on the origin point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActX = pnt.x;

        arebothpointsorsegmentsfixed=isPointOrSegmentFixed(Obj,GeoId1);

        openCommand("add fixed x-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActX);


        if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

void CmdSketcherConstrainDistanceX::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none;

    switch (seqIndex) {
    case 0: // {SelVertex, SelVertexOrRoot}
    case 1: // {SelRoot, SelVertex}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId; PosId2 = selSeq.at(1).PosId;
        break;
    }
    case 2: // {SelEdge}
    case 4: // {SelExternalEdge}
    {
        GeoId1 = GeoId2 = selSeq.at(0).GeoId;
        PosId1 = Sketcher::start; PosId2 = Sketcher::end;

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("This constraint only makes sense on a line segment or a pair of points"));
            return;
        }

        break;
    }
    default:
        break;
    }

    Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
    Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
    double ActLength = pnt2.x-pnt1.x;

    //negative sign avoidance: swap the points to make value positive
    if (ActLength < -Precision::Confusion()) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
        std::swap(pnt1, pnt2);
        ActLength = -ActLength;
    }

    openCommand("add point to point horizontal distance constraint");
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%d,%d,%f)) ",
        Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

    if (areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
        Obj->getNameInDocument(),ConStr.size()-1,"False");
        finishDistanceConstraint(this, Obj, false);
    }
    else
        finishDistanceConstraint(this, Obj, true);
}

void CmdSketcherConstrainDistanceX::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_HorizontalDistance"));
        break;
    }
}


// ======================================================================================

class CmdSketcherConstrainDistanceY : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDistanceY();
    virtual ~CmdSketcherConstrainDistanceY(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainDistanceY"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainDistanceY::CmdSketcherConstrainDistanceY()
    :CmdSketcherConstraint("Sketcher_ConstrainDistanceY")
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

    allowedSelSequences = {{SelVertex, SelVertexOrRoot}, {SelRoot, SelVertex},
                           {SelEdge}, {SelExternalEdge}}; // Can't do single vertex because its a prefix for 2 vertices
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainDistanceY::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

    bool arebothpointsorsegmentsfixed=areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2);

    if (GeoId2 == Sketcher::GeoEnum::HAxis || GeoId2 == Sketcher::GeoEnum::VAxis) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
    }

    if (GeoId1 == Sketcher::GeoEnum::VAxis && PosId1 == Sketcher::none) // reject vertical axis from selection
        GeoId1 = Constraint::GeoUndef;
    else if (GeoId1 == Sketcher::GeoEnum::HAxis && PosId1 == Sketcher::none)
        PosId1 = Sketcher::start;

    if (isEdge(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef)  { // vertical length of a line
        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                        QObject::tr("Cannot add a vertical length constraint on an axis!"));
            return;
        }

        arebothpointsorsegmentsfixed = isPointOrSegmentFixed(Obj,GeoId1);

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            //convert to as if two endpoints of the line have been selected
            PosId1 = Sketcher::start;
            GeoId2 = GeoId1;
            PosId2 = Sketcher::end;
        }
    }

    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // point to point vertical distance

        Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
        Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
        double ActLength = pnt2.y-pnt1.y;

        //negative sign avoidance: swap the points to make value positive
        if (ActLength < -Precision::Confusion()) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
            std::swap(pnt1, pnt2);
            ActLength = -ActLength;
        }

        openCommand("add point to point vertical distance constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

        if (arebothpointsorsegmentsfixed || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed y-coordinate

        if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a fixed y-coordinate constraint on the origin point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActY = pnt.y;

        arebothpointsorsegmentsfixed=isPointOrSegmentFixed(Obj,GeoId1);

        openCommand("add fixed y-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActY);

        if (GeoId1 <= Sketcher::GeoEnum::RefExt || isConstructionPoint(Obj,GeoId1) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one line or up to two points from the sketch."));
    return;
}

void CmdSketcherConstrainDistanceY::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none;

    switch (seqIndex) {
    case 0: // {SelVertex, SelVertexOrRoot}
    case 1: // {SelRoot, SelVertex}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId; PosId2 = selSeq.at(1).PosId;
        break;
    }
    case 2: // {SelEdge}
    case 3: // {SelExternalEdge}
    {
        GeoId1 = GeoId2 = selSeq.at(0).GeoId;
        PosId1 = Sketcher::start; PosId2 = Sketcher::end;

        const Part::Geometry *geom = Obj->getGeometry(GeoId1);
        if (geom->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("This constraint only makes sense on a line segment or a pair of points"));
            return;
        }

        break;
    }
    default:
        break;
    }

    Base::Vector3d pnt1 = Obj->getPoint(GeoId1,PosId1);
    Base::Vector3d pnt2 = Obj->getPoint(GeoId2,PosId2);
    double ActLength = pnt2.y-pnt1.y;

    //negative sign avoidance: swap the points to make value positive
    if (ActLength < -Precision::Confusion()) {
        std::swap(GeoId1,GeoId2);
        std::swap(PosId1,PosId2);
        std::swap(pnt1, pnt2);
        ActLength = -ActLength;
    }

    openCommand("add point to point vertical distance constraint");
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%d,%d,%f)) ",
        Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,ActLength);

    if (areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
        Obj->getNameInDocument(),ConStr.size()-1,"False");
        finishDistanceConstraint(this, Obj,false);
    }
    else
        finishDistanceConstraint(this, Obj,true);
}

void CmdSketcherConstrainDistanceY::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_VerticalDistance"));
        break;
    }
}

//=================================================================================

/* XPM */
static const char *cursor_createparallel[]={
"32 32 3 1",
"  c None",
". c #FFFFFF",
"+ c #FF0000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .                         ",
"      .                         ",
"      .         +         +     ",
"      .        ++        ++     ",
"      .        +         +      ",
"              ++        ++      ",
"              +         +       ",
"             ++        ++       ",
"             +         +        ",
"            ++        ++        ",
"            +         +         ",
"           ++        ++         ",
"           +         +          ",
"          ++        ++          ",
"          +         +           ",
"         ++        ++           ",
"         +         +            ",
"        ++        ++            ",
"        +         +             ",
"       ++        ++             ",
"       +         +              ",
"                                ",
"                                ",
"                                "};

class CmdSketcherConstrainParallel : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainParallel();
    virtual ~CmdSketcherConstrainParallel(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainParallel"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainParallel::CmdSketcherConstrainParallel()
    :CmdSketcherConstraint("Sketcher_ConstrainParallel")
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

    // TODO: Also needed: ExternalEdges
    allowedSelSequences = {{SelEdge, SelEdgeOrAxis}, {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge}, {SelExternalEdge, SelEdge}};
    constraintCursor = cursor_createparallel;
}

void CmdSketcherConstrainParallel::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two or more lines from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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
        else if (isPointOrSegmentFixed(Obj,GeoId)) {
            if (hasAlreadyExternal) {
                showNoConstraintBetweenFixedGeometry();
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

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainParallel::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    switch (seqIndex) {
    case 0: // {SelEdge, SelEdgeOrAxis}
    case 1: // {SelEdgeOrAxis, SelEdge}
    case 2: // {SelEdge, SelExternalEdge}
    case 3: // {SelExternalEdge, SelEdge}
        // create the constraint
        SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
        Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

        int GeoId1 = selSeq.at(0).GeoId, GeoId2 = selSeq.at(1).GeoId;

        // Check that the curves are line segments
        if (    Obj->getGeometry(GeoId1)->getTypeId() != Part::GeomLineSegment::getClassTypeId() ||
                Obj->getGeometry(GeoId2)->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                QObject::tr("The selected edge is not a valid line"));
            return;
        }

        if( areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2)) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }

        // undo command open
        openCommand("add parallel constraint");
        Gui::Command::doCommand(
            Doc, "App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Parallel',%d,%d)) ",
            sketchgui->getObject()->getNameInDocument(), GeoId1, GeoId2);
        // finish the transaction and update
        commitCommand();
        tryAutoRecompute(Obj);
    }
}

// ======================================================================================

/* XPM */
static const char *cursor_createperpconstraint[] = {
"32 32 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #FF0000",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .                         ",
"                                ",
".....   .....                   ",
"                                ",
"      .                         ",
"      .                         ",
"      .                         ",
"      .         ++              ",
"      .         ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"                ++              ",
"       ++++++++++++++++++++     ",
"       ++++++++++++++++++++     ",
"                                ",
"                                "};

class CmdSketcherConstrainPerpendicular : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainPerpendicular();
    virtual ~CmdSketcherConstrainPerpendicular(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainPerpendicular"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainPerpendicular::CmdSketcherConstrainPerpendicular()
    :CmdSketcherConstraint("Sketcher_ConstrainPerpendicular")
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

    // TODO: there are two more combos: endpoint then curve and endpoint then endpoint
    allowedSelSequences = {{SelEdge, SelEdgeOrAxis}, {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge}, {SelExternalEdge, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
                           {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelExternalEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelEdge}};
;
    constraintCursor = cursor_createperpconstraint;
}

void CmdSketcherConstrainPerpendicular::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString strBasicHelp =
            QObject::tr(
             "There is a number of ways this constraint can be applied.\n\n"
             "Accepted combinations: two curves; an endpoint and a curve; two endpoints; two curves and a point.",
             /*disambig.:*/ "perpendicular constraint");
    QString strError;

    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            strError = QObject::tr("Select some geometry from the sketch.", "perpendicular constraint");
            if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 strError+strBasicHelp);
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (!Obj || (SubNames.size() != 2 && SubNames.size() != 3)) {
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

    if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)) { //checkBothExternal displays error message
        showNoConstraintBetweenFixedGeometry();
        return;
    }

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

                tryAutoRecompute(Obj);
                return;
            }

            commitCommand();
            tryAutoRecompute(Obj);

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

            // This code supports simple B-spline endpoint perp to any other geometric curve
            const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

            if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomBSplineCurve::getClassTypeId()) {
                    std::swap(GeoId1,GeoId2);
                    std::swap(PosId1,PosId2);
                }
                // GeoId1 is the B-spline now
            } // end of code supports simple B-spline endpoint tangency

            openCommand("add perpendicular constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
            commitCommand();
            tryAutoRecompute(Obj);

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

            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

            if( geom2 && geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ){
                // unsupported until normal to B-spline at any point implemented.
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Perpendicular to B-spline edge currently unsupported."));
                return;
            }

            openCommand("add perpendicularity constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) { // simple perpendicularity between GeoId1 and GeoId2

            const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
            if (!geo1 || !geo2) {
                return;
            }

            if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("One of the selected edges should be a line."));
                return;
            }

            if (geo1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
                geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){

                // unsupported until tangent to B-spline at any point implemented.
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Perpendicular to B-spline edge currently unsupported."));
                return;
            }

            if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId())
                std::swap(GeoId1,GeoId2);

            // GeoId2 is the line
            geo1 = Obj->getGeometry(GeoId1);
            geo2 = Obj->getGeometry(GeoId2);

            if (geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
                geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId()) {

                Base::Vector3d center;
                Base::Vector3d majdir;
                Base::Vector3d focus;
                double majord = 0;
                double minord = 0;
                double phi = 0;

                if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ){
                    const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo1);

                    center=ellipse->getCenter();
                    majord=ellipse->getMajorRadius();
                    minord=ellipse->getMinorRadius();
                    majdir=ellipse->getMajorAxisDir();
                    phi=atan2(majdir.y, majdir.x);
                }
                else if( geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ){
                    const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo1);

                    center=aoe->getCenter();
                    majord=aoe->getMajorRadius();
                    minord=aoe->getMinorRadius();
                    majdir=aoe->getMajorAxisDir();
                    phi=atan2(majdir.y, majdir.x);
                }
                else if( geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){
                    const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo1);

                    center=aoh->getCenter();
                    majord=aoh->getMajorRadius();
                    minord=aoh->getMinorRadius();
                    majdir=aoh->getMajorAxisDir();
                    phi=atan2(majdir.y, majdir.x);
                }
                else if( geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ){
                    const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo1);

                    center=aop->getCenter();
                    focus=aop->getFocus();
                }

                const Part::GeomLineSegment *line = static_cast<const Part::GeomLineSegment *>(geo2);

                Base::Vector3d point1=line->getStartPoint();
                Base::Vector3d PoO;

                if( geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ) {
                    double df=sqrt(majord*majord+minord*minord);
                    Base::Vector3d direction=point1-(center+majdir*df); // towards the focus
                    double tapprox=atan2(direction.y,direction.x)-phi;

                    PoO = Base::Vector3d(center.x+majord*cosh(tapprox)*cos(phi)-minord*sinh(tapprox)*sin(phi),
                                         center.y+majord*cosh(tapprox)*sin(phi)+minord*sinh(tapprox)*cos(phi), 0);
                }
                else if( geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                    Base::Vector3d direction=point1-focus; // towards the focus

                    PoO = point1 + direction / 2;
                }
                else {
                    Base::Vector3d direction=point1-center;
                    double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomaly by the polar

                    PoO = Base::Vector3d(center.x+majord*cos(tapprox)*cos(phi)-minord*sin(tapprox)*sin(phi),
                                                        center.y+majord*cos(tapprox)*sin(phi)+minord*sin(tapprox)*cos(phi), 0);
                }
                openCommand("add perpendicular constraint");

                try {
                    // Add a point
                    Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                        Obj->getNameInDocument(), PoO.x,PoO.y);
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

                    tryAutoRecompute(Obj);
                    return;
                }

                commitCommand();
                tryAutoRecompute(Obj);

                getSelection().clearSelection();
                return;

            }

            openCommand("add perpendicular constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d)) ",
                selection[0].getFeatName(),GeoId1,GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
    }

    if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        strError+strBasicHelp);
    return;
}

void CmdSketcherConstrainPerpendicular::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef, GeoId3 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none, PosId3 = Sketcher::none;

    // check if the edge already has a Block constraint
    if ( areBothPointsOrSegmentsFixed(Obj,GeoId1,GeoId2) ) {
        showNoConstraintBetweenFixedGeometry();
        return;
    }

    switch (seqIndex) {
    case 0: // {SelEdge, SelEdgeOrAxis}
    case 1: // {SelEdgeOrAxis, SelEdge}
    case 2: // {SelEdge, SelExternalEdge}
    case 3: // {SelExternalEdge, SelEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;

        const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
        if (!geo1 || !geo2) {
            return;
        }

        if (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
            geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("One of the selected edges should be a line."));
            return;
        }

        if (geo1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
            geo2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()){

            // unsupported until tangent to B-spline at any point implemented.
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Perpendicular to B-spline edge currently unsupported."));
            return;
        }

        if (geo1->getTypeId() == Part::GeomLineSegment::getClassTypeId())
            std::swap(GeoId1,GeoId2);

        // GeoId2 is the line
        geo1 = Obj->getGeometry(GeoId1);
        geo2 = Obj->getGeometry(GeoId2);

        if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ||
            geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
            geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
            geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {

            Base::Vector3d center;
            Base::Vector3d majdir;
            Base::Vector3d focus;
            double majord = 0;
            double minord = 0;
            double phi = 0;

            if( geo1->getTypeId() == Part::GeomEllipse::getClassTypeId() ){
                const Part::GeomEllipse *ellipse = static_cast<const Part::GeomEllipse *>(geo1);

                center=ellipse->getCenter();
                majord=ellipse->getMajorRadius();
                minord=ellipse->getMinorRadius();
                majdir=ellipse->getMajorAxisDir();
                phi=atan2(majdir.y, majdir.x);
            }
            else if( geo1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ){
                const Part::GeomArcOfEllipse *aoe = static_cast<const Part::GeomArcOfEllipse *>(geo1);

                center=aoe->getCenter();
                majord=aoe->getMajorRadius();
                minord=aoe->getMinorRadius();
                majdir=aoe->getMajorAxisDir();
                phi=atan2(majdir.y, majdir.x);
            }
            else if( geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ){
                const Part::GeomArcOfHyperbola *aoh = static_cast<const Part::GeomArcOfHyperbola *>(geo1);

                center=aoh->getCenter();
                majord=aoh->getMajorRadius();
                minord=aoh->getMinorRadius();
                majdir=aoh->getMajorAxisDir();
                phi=atan2(majdir.y, majdir.x);
            }
            else if( geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ){
                const Part::GeomArcOfParabola *aop = static_cast<const Part::GeomArcOfParabola *>(geo1);

                center=aop->getCenter();
                focus=aop->getFocus();
            }

            const Part::GeomLineSegment *line = static_cast<const Part::GeomLineSegment *>(geo2);

            Base::Vector3d point1=line->getStartPoint();
            Base::Vector3d PoO;

            if( geo1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ) {
                double df=sqrt(majord*majord+minord*minord);
                Base::Vector3d direction=point1-(center+majdir*df); // towards the focus
                double tapprox=atan2(direction.y,direction.x)-phi;

                PoO = Base::Vector3d(center.x+majord*cosh(tapprox)*cos(phi)-minord*sinh(tapprox)*sin(phi),
                                     center.y+majord*cosh(tapprox)*sin(phi)+minord*sinh(tapprox)*cos(phi), 0);
            }
            else if( geo1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                Base::Vector3d direction=point1-focus; // towards the focus

                PoO = point1 + direction / 2;
            }
            else {
                Base::Vector3d direction=point1-center;
                double tapprox=atan2(direction.y,direction.x)-phi; // we approximate the eccentric anomaly by the polar

                PoO = Base::Vector3d(center.x+majord*cos(tapprox)*cos(phi)-minord*sin(tapprox)*sin(phi),
                                                    center.y+majord*cos(tapprox)*sin(phi)+minord*sin(tapprox)*cos(phi), 0);
            }
            openCommand("add perpendicular constraint");

            try {
                // Add a point
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.addGeometry(Part.Point(App.Vector(%f,%f,0)))",
                    Obj->getNameInDocument(), PoO.x,PoO.y);
                int GeoIdPoint = Obj->getHighestCurveIndex();

                // Point on first object (ellipse, arc of ellipse)
                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,GeoId1);
                // Point on second object
                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoIdPoint,Sketcher::start,GeoId2);

                // add constraint: Perpendicular-via-point
                Gui::Command::doCommand(
                    Gui::Command::Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d))",
                    Obj->getNameInDocument(), GeoId1, GeoId2 ,GeoIdPoint, Sketcher::start);

                commitCommand();
            }
            catch (const Base::Exception& e) {
                Base::Console().Error("%s\n", e.what());
                Gui::Command::abortCommand();
            }


            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;

        }

        openCommand("add perpendicular constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d)) ",
            Obj->getNameInDocument(),GeoId1,GeoId2);
        commitCommand();

        tryAutoRecompute(Obj);
        return;
    }
    case 4: // {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
    case 5: // {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
    case 6: // {SelVertexOrRoot, SelEdge, SelExternalEdge}
    case 7: // {SelVertexOrRoot, SelExternalEdge, SelEdge}
    {
        //let's sink the point to be GeoId3.
        GeoId1 = selSeq.at(1).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(0).GeoId;
        PosId3 = selSeq.at(0).PosId;

        break;
    }
    case 8: // {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
    case 9: // {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
    case 10: // {SelEdge, SelVertexOrRoot, SelExternalEdge}
    case 11: // {SelExternalEdge, SelVertexOrRoot, SelEdge}
    {
        //let's sink the point to be GeoId3.
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId3 = selSeq.at(1).PosId;

        break;
    }
    default:
        return;
    }

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

        openCommand("add perpendicular constraint");

        try{
            //add missing point-on-object constraints
            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
            };

            if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId2);
            };

            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
            };

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PerpendicularViaPoint',%d,%d,%d,%d)) ",
                Obj->getNameInDocument(),GeoId1,GeoId2,GeoId3,PosId3);
        } catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Error"),
                                 QString::fromLatin1(e.what()));
            Gui::Command::abortCommand();

            tryAutoRecompute(Obj);
            return;
        }

        commitCommand();
        tryAutoRecompute(Obj);

        getSelection().clearSelection();

        return;

    };
}

// ======================================================================================

class CmdSketcherConstrainTangent : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainTangent();
    virtual ~CmdSketcherConstrainTangent(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainTangent"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainTangent::CmdSketcherConstrainTangent()
    :CmdSketcherConstraint("Sketcher_ConstrainTangent")
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

    allowedSelSequences = {{SelEdge, SelEdgeOrAxis}, {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge}, {SelExternalEdge, SelEdge},/* Two Curves */
                           {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
                           {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelExternalEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelEdge}, /* Two Curves and a Point */
                           {SelVertexOrRoot, SelVertex} /*Two Endpoints*/ /*No Place for One Endpoint and One Curve*/};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainTangent::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    QString strBasicHelp =
            QObject::tr(
             "There are a number of ways this constraint can be applied.\n\n"
             "Accepted combinations: two curves; an endpoint and a curve; two endpoints; two curves and a point.",
             /*disambig.:*/ "tangent constraint");

    QString strError;
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            strError = QObject::tr("Select some geometry from the sketch.", "tangent constraint");
            if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 strError+strBasicHelp);
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

    if (areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2)){ //checkBothExternal displays error message
        showNoConstraintBetweenFixedGeometry();
        return;
    }
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
                tryAutoRecompute(Obj);
                return;
            }

            commitCommand();
            tryAutoRecompute(Obj);

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
            doEndpointTangency(Obj, selection[0], GeoId1, GeoId2, PosId1, PosId2);
            commitCommand();
            tryAutoRecompute(Obj);

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

            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

            if( geom2 && geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ){
                // unsupported until tangent to B-spline at any point implemented.
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Tangency to B-spline edge currently unsupported."));
                return;
            }

            openCommand("add tangent constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }
        else if (isEdge(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) { // simple tangency between GeoId1 and GeoId2

            const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

            if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() )){

                // unsupported until tangent to B-spline at any point implemented.
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Tangency to B-spline edge currently unsupported."));
                return;
            }

            // check if there is a coincidence constraint on GeoId1, GeoId2
            const std::vector< Constraint * > &cvals = Obj->Constraints.getValues();

            for (std::vector<Constraint *>::const_iterator it = cvals.begin(); it != cvals.end(); ++it) {
                if( (*it)->Type == Sketcher::Coincident &&
                    (((*it)->First == GeoId1 && (*it)->Second == GeoId2) ||
                    ((*it)->Second == GeoId1 && (*it)->First == GeoId2)) ) {

                    // save values because 'doEndpointTangency' changes the
                    // constraint property and thus invalidates this iterator
                    int first = (*it)->First;
                    int firstpos = static_cast<int>((*it)->FirstPos);

                    Gui::Command::openCommand("swap coincident+tangency with ptp tangency");

                    doEndpointTangency(Obj, selection[0], (*it)->First, (*it)->Second, (*it)->FirstPos, (*it)->SecondPos);

                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.delConstraintOnPoint(%i,%i)",
                                            selection[0].getFeatName(), first, firstpos);

                    commitCommand();
                    tryAutoRecomputeIfNotSolve(Obj);

                    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher/General");

                    if(hGrp->GetBool("NotifyConstraintSubstitutions", true)) {
                        QMessageBox::information(Gui::getMainWindow(), QObject::tr("Constraint Substitution"),
                                         QObject::tr("Endpoint to endpoint tangency was applied. The coincident constraint was deleted."));
                    }
                    getSelection().clearSelection();
                    return;
                }
            }

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
                    makeTangentToEllipseviaNewPoint(Obj,static_cast<const Part::GeomEllipse *>(geom1), geom2,
                                                    GeoId1, GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ) {
                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfHyperbolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfHyperbola *>(geom2),
                                                           geom1, GeoId2, GeoId1);
                    getSelection().clearSelection();
                    return;
                }
                else if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfParabolaviaNewPoint(Obj,static_cast<const Part::GeomArcOfParabola *>(geom2),
                                                          geom1, GeoId2, GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomArcOfEllipse::getClassTypeId())
                    std::swap(GeoId1,GeoId2);

                // GeoId1 is the arc of ellipse
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {

                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfEllipseviaNewPoint(Obj,
                                                         static_cast<const Part::GeomArcOfEllipse *>(geom1), geom2, GeoId1, GeoId2);

                    getSelection().clearSelection();
                    return;
                }
                else if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfParabolaviaNewPoint(Obj,
                                                          static_cast<const Part::GeomArcOfParabola *>(geom2),
                                                          geom1, GeoId2, GeoId1);
                    getSelection().clearSelection();
                    return;
                }
            }
            else if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                  geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomArcOfHyperbola::getClassTypeId())
                    std::swap(GeoId1,GeoId2);

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {

                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfHyperbolaviaNewPoint(Obj,
                                                           static_cast<const Part::GeomArcOfHyperbola *>(geom1),
                                                           geom2, GeoId1, GeoId2);
                    getSelection().clearSelection();
                    return;
                }
                else if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfParabolaviaNewPoint(Obj,
                                                          static_cast<const Part::GeomArcOfParabola *>(geom2),
                                                          geom1, GeoId2, GeoId1);
                    getSelection().clearSelection();
                    return;
                }

            }
            else if( geom1 && geom2 &&
                ( geom1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                  geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() )){

                if(geom1->getTypeId() != Part::GeomArcOfParabola::getClassTypeId())
                    std::swap(GeoId1,GeoId2);

                // GeoId1 is the arc of hyperbola
                geom1 = Obj->getGeometry(GeoId1);
                geom2 = Obj->getGeometry(GeoId2);

                if (geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                    geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {

                    Gui::Command::openCommand("add tangent constraint point");
                    makeTangentToArcOfParabolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfParabola *>(geom1),
                                                          geom2, GeoId1, GeoId2);
                    getSelection().clearSelection();
                    return;
                }
            }

            openCommand("add tangent constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d)) ",
                selection[0].getFeatName(),GeoId1,GeoId2);
            commitCommand();
            tryAutoRecompute(Obj);

            getSelection().clearSelection();
            return;
        }

    }

    if (!strError.isEmpty()) strError.append(QString::fromLatin1("\n\n"));
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        strError+strBasicHelp);
    return;
}

void CmdSketcherConstrainTangent::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
    QString strError;

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef, GeoId3 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none, PosId3 = Sketcher::none;

    // check if the edge already has a Block constraint
    if ( areBothPointsOrSegmentsFixed(Obj,GeoId1,GeoId2) ) {
      showNoConstraintBetweenFixedGeometry();
      return;
    }

    switch (seqIndex) {
    case 0: // {SelEdge, SelEdgeOrAxis}
    case 1: // {SelEdgeOrAxis, SelEdge}
    case 2: // {SelEdge, SelExternalEdge}
    case 3: // {SelExternalEdge, SelEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;

        const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

        if( geom1 && geom2 &&
            ( geom1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
            geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() )){

            // unsupported until tangent to B-spline at any point implemented.
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Tangency to B-spline edge currently unsupported."));
            return;
        }


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
                makeTangentToEllipseviaNewPoint(Obj, static_cast<const Part::GeomEllipse *>(geom1),
                                                geom2, GeoId1, GeoId2);
                getSelection().clearSelection();
                return;
            }
            else if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ) {
                Gui::Command::openCommand("add tangent constraint point");
                makeTangentToArcOfHyperbolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfHyperbola *>(geom2),
                                                       geom1, GeoId2, GeoId1);
                getSelection().clearSelection();
                return;
            }
            else if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                Gui::Command::openCommand("add tangent constraint point");
                makeTangentToArcOfParabolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfParabola *>(geom2),
                                                      geom1, GeoId2, GeoId1);
                getSelection().clearSelection();
                return;
            }
        }
        else if( geom1 && geom2 &&
            ( geom1->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
              geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() )){

            if(geom1->getTypeId() != Part::GeomArcOfHyperbola::getClassTypeId())
                std::swap(GeoId1,GeoId2);

            // GeoId1 is the arc of hyperbola
            geom1 = Obj->getGeometry(GeoId1);
            geom2 = Obj->getGeometry(GeoId2);

            if( geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {

                Gui::Command::openCommand("add tangent constraint point");
                makeTangentToArcOfHyperbolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfHyperbola *>(geom1),
                                                       geom2, GeoId1, GeoId2);
                getSelection().clearSelection();
                return;
            }
            else if( geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ) {
                Gui::Command::openCommand("add tangent constraint point");
                makeTangentToArcOfParabolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfParabola *>(geom2),
                                                      geom1, GeoId2, GeoId1);
                getSelection().clearSelection();
                return;
            }

        }
        else if( geom1 && geom2 &&
            ( geom1->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
              geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() )){

            if(geom1->getTypeId() != Part::GeomArcOfParabola::getClassTypeId())
                std::swap(GeoId1,GeoId2);

            // GeoId1 is the arc of hyperbola
            geom1 = Obj->getGeometry(GeoId1);
            geom2 = Obj->getGeometry(GeoId2);

            if (geom2->getTypeId() == Part::GeomArcOfParabola::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomCircle::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ||
                geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId() ) {

                Gui::Command::openCommand("add tangent constraint point");
                makeTangentToArcOfParabolaviaNewPoint(Obj, static_cast<const Part::GeomArcOfParabola *>(geom1),
                                                      geom2, GeoId1, GeoId2);
                getSelection().clearSelection();
                return;
            }
        }

        openCommand("add tangent constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d)) ",
            Obj->getNameInDocument(),GeoId1,GeoId2);
        commitCommand();
        tryAutoRecompute(Obj);

        return;
    }
    case 4: // {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
    case 5: // {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
    case 6: // {SelVertexOrRoot, SelEdge, SelExternalEdge}
    case 7: // {SelVertexOrRoot, SelExternalEdge, SelEdge}
    {
        //let's sink the point to be GeoId3.
        GeoId1 = selSeq.at(1).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(0).GeoId;
        PosId3 = selSeq.at(0).PosId;

        break;
    }
    case 8: // {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
    case 9: // {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
    case 10: // {SelEdge, SelVertexOrRoot, SelExternalEdge}
    case 11: // {SelExternalEdge, SelVertexOrRoot, SelEdge}
    {
        //let's sink the point to be GeoId3.
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId3 = selSeq.at(1).PosId;

        break;
    }
    case 12: // {SelVertexOrRoot, SelVertex}
    {
        // Different notation than the previous places
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId; PosId2 = selSeq.at(1).PosId;

        if (isSimpleVertex(Obj, GeoId1, PosId1) ||
            isSimpleVertex(Obj, GeoId2, PosId2)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a tangency constraint at an unconnected point!"));
            return;
        }

        // This code supports simple B-spline endpoint tangency to any other geometric curve
        const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);

        if( geom1 && geom2 &&
            ( geom1->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ||
            geom2->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() )){

            if(geom1->getTypeId() != Part::GeomBSplineCurve::getClassTypeId()) {
                std::swap(GeoId1,GeoId2);
                std::swap(PosId1,PosId2);
            }
            // GeoId1 is the B-spline now
        } // end of code supports simple B-spline endpoint tangency

        openCommand("add tangent constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d,%d,%d)) ",
            Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2);
        commitCommand();
        tryAutoRecompute(Obj);

        getSelection().clearSelection();
        return;
    }
    default:
        return;
    }

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {

        openCommand("add tangent constraint");

        try{
            //add missing point-on-object constraints
            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
            };

            if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId2);
            };

            if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                    Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
            };

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('TangentViaPoint',%d,%d,%d,%d)) ",
                Obj->getNameInDocument(), GeoId1,GeoId2,GeoId3,PosId3);
        } catch (const Base::Exception& e) {
            Base::Console().Error("%s\n", e.what());
            QMessageBox::warning(Gui::getMainWindow(),
                                 QObject::tr("Error"),
                                 QString::fromLatin1(e.what()));
            Gui::Command::abortCommand();

            tryAutoRecompute(Obj);
            return;
        }

        commitCommand();
        tryAutoRecompute(Obj);

        getSelection().clearSelection();

        return;
    };
}

// ======================================================================================

class CmdSketcherConstrainRadius : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainRadius();
    virtual ~CmdSketcherConstrainRadius(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainRadius"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainRadius::CmdSketcherConstrainRadius()
    :CmdSketcherConstraint("Sketcher_ConstrainRadius")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain radius");
    sToolTipText    = QT_TR_NOOP("Fix the radius of a circle or an arc");
    sWhatsThis      = "Sketcher_ConstrainRadius";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Radius";
    sAccel          = "";
    eType           = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelExternalEdge}};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainRadius::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector< std::pair<int, double> > geoIdRadiusMap;
    std::vector< std::pair<int, double> > externalGeoIdRadiusMap;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        bool issegmentfixed = false;
        int GeoId;

        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            issegmentfixed = isPointOrSegmentFixed(Obj,GeoId);
        }
        else if (it->size() > 4 && it->substr(0,12) == "ExternalEdge") {
            GeoId = -std::atoi(it->substr(12,4000).c_str()) - 2;
            issegmentfixed = true;
        }
        else
            continue;

        const Part::Geometry *geom = Obj->getGeometry(GeoId);

        if (geom && geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
            double radius = arc->getRadius();

            if(issegmentfixed) {
                externalGeoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
            else {
                geoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
        }
        else if (geom && geom->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geom);
            double radius = circle->getRadius();

            if(issegmentfixed) {
                externalGeoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
            else {
                geoIdRadiusMap.push_back(std::make_pair(GeoId, radius));
            }
        }
    }

    if (geoIdRadiusMap.empty() && externalGeoIdRadiusMap.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select one or more arcs or circles from the sketch."));
    }

    bool commitNeeded=false;
    bool updateNeeded=false;
    bool commandopened=false;

    if(!externalGeoIdRadiusMap.empty()) {
        // Create the non-driving radius constraints now
        openCommand("Add radius constraint");
        commandopened=true;
        unsigned int constrSize = 0;

        for (std::vector< std::pair<int, double> >::iterator it = externalGeoIdRadiusMap.begin(); it != externalGeoIdRadiusMap.end(); ++it) {
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                selection[0].getFeatName(),it->first,it->second);

            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            constrSize=ConStr.size();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),constrSize-1,"False");
        }

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

        std::size_t indexConstr = constrSize - externalGeoIdRadiusMap.size();

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            for (std::size_t i=0; i<externalGeoIdRadiusMap.size();i++) {
                Sketcher::Constraint *constr = ConStr[indexConstr + i];
                constr->LabelDistance = 2. * sf;
            }
            vp->draw(false,false); // Redraw
        }

        commitNeeded=true;
        updateNeeded=true;
    }

    if(!geoIdRadiusMap.empty())
    {
        bool constrainEqual = false;
        if (geoIdRadiusMap.size() > 1 && constraintCreationMode==Driving) {
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

            if(!commandopened)
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
            if(!commandopened)
                openCommand("Add radius constraint");
            for (std::vector< std::pair<int, double> >::iterator it = geoIdRadiusMap.begin(); it != geoIdRadiusMap.end(); ++it) {
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                    selection[0].getFeatName(),it->first,it->second);

                    if(constraintCreationMode==Reference) {

                        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                        selection[0].getFeatName(),ConStr.size()-1,"False");

                    }

            }
        }

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();
        std::size_t indexConstr = ConStr.size() - geoIdRadiusMap.size();

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            for (std::size_t i=0; i<geoIdRadiusMap.size();i++) {
                Sketcher::Constraint *constr = ConStr[indexConstr + i];
                constr->LabelDistance = 2. * sf;
            }
            vp->draw(false,false); // Redraw
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
        // Ask for the value of the radius immediately
        if (show && constraintCreationMode==Driving) {
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
            if (constrainEqual || geoIdRadiusMap.size() == 1)
                ui_Datum.labelEdit->bind(Obj->Constraints.createPath(indexConstr));
            else
                ui_Datum.name->setDisabled(true);

            if (dlg.exec() == QDialog::Accepted) {
                Base::Quantity newQuant = ui_Datum.labelEdit->value();
                double newRadius = newQuant.getValue();

                try {
                    if (constrainEqual || geoIdRadiusMap.size() == 1) {
                        doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                    Obj->getNameInDocument(),
                                    indexConstr, newRadius, (const char*)newQuant.getUnit().getString().toUtf8());

                        QString constraintName = ui_Datum.name->text().trimmed();
                        if (Base::Tools::toStdString(constraintName) != Obj->Constraints[indexConstr]->Name) {
                            std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(constraintName.toUtf8().constData());
                            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.renameConstraint(%d, u'%s')",
                                                    Obj->getNameInDocument(),
                                                    indexConstr, escapedstr.c_str());
                        }
                    }
                    else {
                        for (std::size_t i=0; i<geoIdRadiusMap.size();i++) {
                            doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                        Obj->getNameInDocument(),
                                        indexConstr+i, newRadius, (const char*)newQuant.getUnit().getString().toUtf8());
                        }
                    }

                    commitCommand();

                    if (Obj->noRecomputes && Obj->ExpressionEngine.depsAreTouched()) {
                        Obj->ExpressionEngine.execute();
                        Obj->solve();
                    }

                    tryAutoRecompute(Obj);

                    commitNeeded=false;
                    updateNeeded=false;
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    abortCommand();

                    tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
                }
            }
            else {
                // command canceled
                abortCommand();

                updateNeeded=true;
            }
        }
        else {
            // now dialog was shown so commit the command
            commitCommand();
            commitNeeded=false;
        }
        //updateActive();
        getSelection().clearSelection();
    }

    if (commitNeeded)
        commitCommand();

    if(updateNeeded) {
        tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
    }
}

void CmdSketcherConstrainRadius::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId = selSeq.at(0).GeoId;
    double radius = 0.0;

    bool commitNeeded=false;
    bool updateNeeded=false;

    switch (seqIndex) {
    case 0: // {SelEdge}
    case 1: // {SelExternalEdge}
    {
        const Part::Geometry *geom = Obj->getGeometry(GeoId);
        if (geom && geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
            radius = arc->getRadius();
        }
        else if (geom && geom->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geom);
            radius = circle->getRadius();
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Constraint only applies to arcs or circles."));
            return;
        }

        // Create the radius constraint now
        openCommand("Add radius constraint");
        Gui::Command::doCommand(Doc, "App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                                Obj->getNameInDocument(), GeoId, radius);

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

        int indexConstr = ConStr.size() - 1;
        bool fixed = isPointOrSegmentFixed(Obj,GeoId);
        if(fixed || constraintCreationMode==Reference) {
            Gui::Command::doCommand(Doc, "App.ActiveDocument.%s.setDriving(%i,%s)",
                                    Obj->getNameInDocument(), ConStr.size()-1, "False");
        }

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            Sketcher::Constraint *constr = ConStr[ConStr.size()-1];
            constr->LabelDistance = 2. * sf;
            vp->draw(); // Redraw
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
        // Ask for the value of the radius immediately
        if (show && constraintCreationMode==Driving && !fixed) {
            QDialog dlg(Gui::getMainWindow());
            Ui::InsertDatum ui_Datum;
            ui_Datum.setupUi(&dlg);
            dlg.setWindowTitle(EditDatumDialog::tr("Change radius"));
            ui_Datum.label->setText(EditDatumDialog::tr("Radius:"));
            Base::Quantity init_val;
            init_val.setUnit(Base::Unit::Length);
            init_val.setValue(radius);

            ui_Datum.labelEdit->setValue(init_val);
            ui_Datum.labelEdit->selectNumber();
            ui_Datum.labelEdit->bind(Obj->Constraints.createPath(indexConstr));

            if (dlg.exec() == QDialog::Accepted) {
                Base::Quantity newQuant = ui_Datum.labelEdit->value();
                double newRadius = newQuant.getValue();

                try {
                    doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                Obj->getNameInDocument(),
                                indexConstr, newRadius, (const char*)newQuant.getUnit().getString().toUtf8());

                    QString constraintName = ui_Datum.name->text().trimmed();
                    if (Base::Tools::toStdString(constraintName) != Obj->Constraints[indexConstr]->Name) {
                        std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(constraintName.toUtf8().constData());
                        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.renameConstraint(%d, u'%s')",
                                                Obj->getNameInDocument(),
                                                indexConstr, escapedstr.c_str());
                    }

                    commitCommand();

                    if (Obj->noRecomputes && Obj->ExpressionEngine.depsAreTouched()) {
                        Obj->ExpressionEngine.execute();
                        Obj->solve();
                    }

                    tryAutoRecompute(Obj);

                    commitNeeded=false;
                    updateNeeded=false;
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    abortCommand();

                    tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
                }
            }
            else {
                // command canceled
                abortCommand();

                updateNeeded=true;
            }
        }
        else {
            // now dialog was shown so commit the command
            commitCommand();
            commitNeeded=false;
        }
        //updateActive();
        getSelection().clearSelection();

        if(commitNeeded)
            commitCommand();

        if(updateNeeded) {
            tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
        }
    }
    }
}

void CmdSketcherConstrainRadius::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
        break;
    }
}

// ======================================================================================

class CmdSketcherConstrainDiameter : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainDiameter();
    virtual ~CmdSketcherConstrainDiameter(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainDiameter"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainDiameter::CmdSketcherConstrainDiameter()
:CmdSketcherConstraint("Sketcher_ConstrainDiameter")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain diameter");
    sToolTipText    = QT_TR_NOOP("Fix the diameter of a circle or an arc");
    sWhatsThis      = "Sketcher_ConstrainDiameter";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Diameter";
    sAccel          = "";
    eType           = ForEdit;

    allowedSelSequences = {{SelEdge}, {SelExternalEdge}};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainDiameter::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                            new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select one or more arcs or circles from the sketch."));
        return;
    }

    // check for which selected geometry the constraint can be applied
    std::vector< std::pair<int, double> > geoIdDiameterMap;
    std::vector< std::pair<int, double> > externalGeoIdDiameterMap;

    for (std::vector<std::string>::const_iterator it = SubNames.begin(); it != SubNames.end(); ++it) {
        bool issegmentfixed = false;
        int GeoId;

        if (it->size() > 4 && it->substr(0,4) == "Edge") {
            GeoId = std::atoi(it->substr(4,4000).c_str()) - 1;
            issegmentfixed = isPointOrSegmentFixed(Obj,GeoId);
        }
        else if (it->size() > 4 && it->substr(0,12) == "ExternalEdge") {
            GeoId = -std::atoi(it->substr(12,4000).c_str()) - 2;
            issegmentfixed = true;
        }
        else
            continue;

        const Part::Geometry *geom = Obj->getGeometry(GeoId);

        if (geom && geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
            double radius = arc->getRadius();

            if(issegmentfixed) {
                externalGeoIdDiameterMap.push_back(std::make_pair(GeoId, 2*radius));
            }
            else {
                geoIdDiameterMap.push_back(std::make_pair(GeoId, 2*radius));
            }
        }
        else if (geom && geom->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geom);
            double radius = circle->getRadius();

            if(issegmentfixed) {
                externalGeoIdDiameterMap.push_back(std::make_pair(GeoId, 2*radius));
            }
            else {
                geoIdDiameterMap.push_back(std::make_pair(GeoId, 2*radius));
            }
        }
    }

    if (geoIdDiameterMap.empty() && externalGeoIdDiameterMap.empty()) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                             QObject::tr("Select one or more arcs or circles from the sketch."));
    }

    bool commitNeeded=false;
    bool updateNeeded=false;
    bool commandopened=false;

    if(!externalGeoIdDiameterMap.empty()) {
        // Create the non-driving radius constraints now
        openCommand("Add diameter constraint");
        commandopened=true;
        unsigned int constrSize = 0;

        for (std::vector< std::pair<int, double> >::iterator it = externalGeoIdDiameterMap.begin(); it != externalGeoIdDiameterMap.end(); ++it) {
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                                    selection[0].getFeatName(),it->first,it->second);

            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            constrSize=ConStr.size();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                    selection[0].getFeatName(),constrSize-1,"False");
        }

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

        std::size_t indexConstr = constrSize - externalGeoIdDiameterMap.size();

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            for (std::size_t i=0; i<externalGeoIdDiameterMap.size();i++) {
                Sketcher::Constraint *constr = ConStr[indexConstr + i];
                constr->LabelDistance = 2. * sf;
            }
            vp->draw(false,false); // Redraw
        }

        commitNeeded=true;
        updateNeeded=true;
    }

    if(!geoIdDiameterMap.empty())
    {
        bool constrainEqual = false;
        if (geoIdDiameterMap.size() > 1 && constraintCreationMode==Driving) {
            int ret = QMessageBox::question(Gui::getMainWindow(), QObject::tr("Constrain equal"),
                                            QObject::tr("Do you want to share the same diameter for all selected elements?"),
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
            int refGeoId = geoIdDiameterMap.front().first;
            double diameter = geoIdDiameterMap.front().second;

            if(!commandopened)
                openCommand("Add diameter constraint");

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                                    selection[0].getFeatName(),refGeoId,diameter);

            // Add the equality constraints
            for (std::vector< std::pair<int, double> >::iterator it = geoIdDiameterMap.begin()+1; it != geoIdDiameterMap.end(); ++it) {
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
                                        selection[0].getFeatName(),refGeoId,it->first);
            }
        }
        else {
            // Create the diameter constraints now
            if(!commandopened)
                openCommand("Add diameter constraint");
            for (std::vector< std::pair<int, double> >::iterator it = geoIdDiameterMap.begin(); it != geoIdDiameterMap.end(); ++it) {
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                                        selection[0].getFeatName(),it->first,it->second);

                if(constraintCreationMode==Reference) {

                    const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                                            selection[0].getFeatName(),ConStr.size()-1,"False");

                }

            }
        }

        const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();
        std::size_t indexConstr = ConStr.size() - geoIdDiameterMap.size();

        // Guess some reasonable distance for placing the datum text
        Gui::Document *doc = getActiveGuiDocument();
        float sf = 1.f;
        if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            sf = vp->getScaleFactor();

            for (std::size_t i=0; i<geoIdDiameterMap.size();i++) {
                Sketcher::Constraint *constr = ConStr[indexConstr + i];
                constr->LabelDistance = 2. * sf;
            }
            vp->draw(false,false); // Redraw
        }

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
        // Ask for the value of the diameter immediately
        if (show && constraintCreationMode==Driving) {
            QDialog dlg(Gui::getMainWindow());
            Ui::InsertDatum ui_Datum;
            ui_Datum.setupUi(&dlg);
            dlg.setWindowTitle(EditDatumDialog::tr("Change diameter"));
            ui_Datum.label->setText(EditDatumDialog::tr("Diameter:"));
            Base::Quantity init_val;
            init_val.setUnit(Base::Unit::Length);
            init_val.setValue(geoIdDiameterMap.front().second);

            ui_Datum.labelEdit->setValue(init_val);
            ui_Datum.labelEdit->selectNumber();
            if (constrainEqual || geoIdDiameterMap.size() == 1)
                ui_Datum.labelEdit->bind(Obj->Constraints.createPath(indexConstr));
            else
                ui_Datum.name->setDisabled(true);

            if (dlg.exec() == QDialog::Accepted) {
                Base::Quantity newQuant = ui_Datum.labelEdit->value();
                double newDiameter = newQuant.getValue();

                try {
                    if (constrainEqual || geoIdDiameterMap.size() == 1) {
                        doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                  Obj->getNameInDocument(),
                                  indexConstr, newDiameter, (const char*)newQuant.getUnit().getString().toUtf8());

                        QString constraintName = ui_Datum.name->text().trimmed();
                        if (Base::Tools::toStdString(constraintName) != Obj->Constraints[indexConstr]->Name) {
                            std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(constraintName.toUtf8().constData());
                            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.renameConstraint(%d, u'%s')",
                                                    Obj->getNameInDocument(),
                                                    indexConstr, escapedstr.c_str());
                        }
                    }
                    else {
                        for (std::size_t i=0; i<geoIdDiameterMap.size();i++) {
                            doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                      Obj->getNameInDocument(),
                                      indexConstr+i, newDiameter, (const char*)newQuant.getUnit().getString().toUtf8());
                        }
                    }

                    commitCommand();

                    if (Obj->noRecomputes && Obj->ExpressionEngine.depsAreTouched()) {
                        Obj->ExpressionEngine.execute();
                        Obj->solve();
                    }

                    tryAutoRecompute(Obj);

                    commitNeeded=false;
                    updateNeeded=false;
                }
                catch (const Base::Exception& e) {
                    QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                    abortCommand();

                    tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
                }
            }
            else {
                // command canceled
                abortCommand();

                updateNeeded=true;
            }
        }
        else {
            // now dialog was shown so commit the command
            commitCommand();
            commitNeeded=false;
        }
        //updateActive();
        getSelection().clearSelection();
    }

    if (commitNeeded)
        commitCommand();

    if(updateNeeded) {
        tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
    }
}

void CmdSketcherConstrainDiameter::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId = selSeq.at(0).GeoId;
    double diameter = 0.0;

    bool commitNeeded=false;
    bool updateNeeded=false;

    switch (seqIndex) {
        case 0: // {SelEdge}
        case 1: // {SelExternalEdge}
        {
            const Part::Geometry *geom = Obj->getGeometry(GeoId);
            if (geom && geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc = static_cast<const Part::GeomArcOfCircle *>(geom);
                diameter = 2*arc->getRadius();
            }
            else if (geom && geom->getTypeId() == Part::GeomCircle::getClassTypeId()) {
                const Part::GeomCircle *circle = static_cast<const Part::GeomCircle *>(geom);
                diameter = 2*circle->getRadius();
            }
            else {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Constraint only applies to arcs or circles."));
                return;
            }

            // Create the diameter constraint now
            openCommand("Add diameter constraint");
            Gui::Command::doCommand(Doc, "App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Diameter',%d,%f)) ",
                                    Obj->getNameInDocument(), GeoId, diameter);

            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            int indexConstr = ConStr.size() - 1;
            bool fixed = isPointOrSegmentFixed(Obj,GeoId);
            if(fixed || constraintCreationMode==Reference) {
                Gui::Command::doCommand(Doc, "App.ActiveDocument.%s.setDriving(%i,%s)",
                                        Obj->getNameInDocument(), ConStr.size()-1, "False");
            }

            // Guess some reasonable distance for placing the datum text
            Gui::Document *doc = getActiveGuiDocument();
            float sf = 1.f;
            if (doc && doc->getInEdit() && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
                SketcherGui::ViewProviderSketch *vp = static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
                sf = vp->getScaleFactor();

                Sketcher::Constraint *constr = ConStr[ConStr.size()-1];
                constr->LabelDistance = 2. * sf;
                vp->draw(); // Redraw
            }

            ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
            bool show = hGrp->GetBool("ShowDialogOnDistanceConstraint", true);
            // Ask for the value of the diameter immediately
            if (show && constraintCreationMode==Driving && !fixed) {
                QDialog dlg(Gui::getMainWindow());
                Ui::InsertDatum ui_Datum;
                ui_Datum.setupUi(&dlg);
                dlg.setWindowTitle(EditDatumDialog::tr("Change diameter"));
                ui_Datum.label->setText(EditDatumDialog::tr("Diameter:"));
                Base::Quantity init_val;
                init_val.setUnit(Base::Unit::Length);
                init_val.setValue(diameter);

                ui_Datum.labelEdit->setValue(init_val);
                ui_Datum.labelEdit->selectNumber();
                ui_Datum.labelEdit->bind(Obj->Constraints.createPath(indexConstr));

                if (dlg.exec() == QDialog::Accepted) {
                    Base::Quantity newQuant = ui_Datum.labelEdit->value();
                    double newDiameter = newQuant.getValue();

                    try {
                        doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDatum(%i,App.Units.Quantity('%f %s'))",
                                  Obj->getNameInDocument(),
                                  indexConstr, newDiameter, (const char*)newQuant.getUnit().getString().toUtf8());

                        QString constraintName = ui_Datum.name->text().trimmed();
                        if (Base::Tools::toStdString(constraintName) != Obj->Constraints[indexConstr]->Name) {
                            std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(constraintName.toUtf8().constData());
                            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.renameConstraint(%d, u'%s')",
                                                    Obj->getNameInDocument(),
                                                    indexConstr, escapedstr.c_str());
                        }

                        commitCommand();

                        if (Obj->noRecomputes && Obj->ExpressionEngine.depsAreTouched()) {
                            Obj->ExpressionEngine.execute();
                            Obj->solve();
                        }

                        tryAutoRecompute(Obj);

                        commitNeeded=false;
                        updateNeeded=false;
                    }
                    catch (const Base::Exception& e) {
                        QMessageBox::critical(qApp->activeWindow(), QObject::tr("Dimensional constraint"), QString::fromUtf8(e.what()));
                        abortCommand();

                        tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
                    }
                }
                else {
                    // command canceled
                    abortCommand();

                    updateNeeded=true;
                }
            }
            else {
                // now dialog was shown so commit the command
                commitCommand();
                commitNeeded=false;
            }
            //updateActive();
            getSelection().clearSelection();

            if(commitNeeded)
                commitCommand();

            if(updateNeeded) {
                tryAutoRecomputeIfNotSolve(Obj); // we have to update the solver after this aborted addition.
            }
        }
    }
}

void CmdSketcherConstrainDiameter::updateAction(int mode)
{
    switch (mode) {
        case Reference:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            break;
        case Driving:
            if (getAction())
                getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            break;
    }
}

// ======================================================================================

DEF_STD_CMD_ACLU(CmdSketcherCompConstrainRadDia);

CmdSketcherCompConstrainRadDia::CmdSketcherCompConstrainRadDia()
: Command("Sketcher_CompConstrainRadDia")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain arc or circle");
    sToolTipText    = QT_TR_NOOP("Constrain an arc or a circle");
    sWhatsThis      = "Sketcher_CompCreateCircle";
    sStatusTip      = sToolTipText;
    sAccel          = "SHIFT+R";
    eType           = ForEdit;
}

void CmdSketcherCompConstrainRadDia::activated(int iMsg)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    if (iMsg==0) {
        rcCmdMgr.runCommandByName("Sketcher_ConstrainRadius");
    }
    else if (iMsg==1) {
        rcCmdMgr.runCommandByName("Sketcher_ConstrainDiameter");
    }
    else
        return;

    // Since the default icon is reset when enabing/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action * CmdSketcherCompConstrainRadDia::createAction(void)
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* arc1 = pcAction->addAction(QString());
    arc1->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
    QAction* arc2 = pcAction->addAction(QString());
    arc2->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(arc1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    pcAction->setShortcut(QString::fromLatin1(sAccel));

    return pcAction;
}

void CmdSketcherCompConstrainRadDia::updateAction(int mode)
{
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(getAction());
    if (!pcAction)
        return;

    QList<QAction*> a = pcAction->actions();
    int index = pcAction->property("defaultAction").toInt();
    switch (mode) {
        case Reference:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius_Driven"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter_Driven"));
            getAction()->setIcon(a[index]->icon());
            break;
        case Driving:
            a[0]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Radius"));
            a[1]->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_Diameter"));
            getAction()->setIcon(a[index]->icon());
            break;
    }
}

void CmdSketcherCompConstrainRadDia::languageChange()
{
    Command::languageChange();

    if (!_pcAction)
        return;
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* arc1 = a[0];
    arc1->setText(QApplication::translate("CmdSketcherCompConstrainRadDia", "Constrain radius"));
    arc1->setToolTip(QApplication::translate("Sketcher_ConstrainRadius", "Fix the radius of a circle or an arc"));
    arc1->setStatusTip(QApplication::translate("Sketcher_ConstrainRadius", "Fix the radius of a circle or an arc"));
    QAction* arc2 = a[1];
    arc2->setText(QApplication::translate("CmdSketcherCompConstrainRadDia", "Constrain diameter"));
    arc2->setToolTip(QApplication::translate("Sketcher_ConstrainDiameter", "Fix the diameter of a circle or an arc"));
    arc2->setStatusTip(QApplication::translate("Sketcher_ConstrainDiameter", "Fix the diameter of a circle or an arc"));
}

bool CmdSketcherCompConstrainRadDia::isActive(void)
{
    return isCreateGeoActive(getActiveGuiDocument());
}

// ======================================================================================

class CmdSketcherConstrainAngle : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainAngle();
    virtual ~CmdSketcherConstrainAngle(){}
    virtual void updateAction(int mode);
    virtual const char* className() const
    { return "CmdSketcherConstrainAngle"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainAngle::CmdSketcherConstrainAngle()
    :CmdSketcherConstraint("Sketcher_ConstrainAngle")
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

    allowedSelSequences = {{SelEdge, SelEdgeOrAxis}, {SelEdgeOrAxis, SelEdge},
                           {SelEdge, SelExternalEdge}, {SelExternalEdge, SelEdge},
                           {SelExternalEdge, SelExternalEdge},
                           {SelEdge, SelVertexOrRoot, SelEdgeOrAxis},
                           {SelEdgeOrAxis, SelVertexOrRoot, SelEdge},
                           {SelEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelEdge},
                           {SelExternalEdge, SelVertexOrRoot, SelExternalEdge},
                           {SelVertexOrRoot, SelEdge, SelEdgeOrAxis},
                           {SelVertexOrRoot, SelEdgeOrAxis, SelEdge},
                           {SelVertexOrRoot, SelEdge, SelExternalEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelEdge},
                           {SelVertexOrRoot, SelExternalEdge, SelExternalEdge}};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainAngle::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    //TODO: comprehensive messages, like in CmdSketcherConstrainTangent
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                    new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            // TODO: Get the exact message from git history and put it here
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select the right things from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

        bool bothexternal=areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

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

            //negative constraint value avoidance
            if (ActAngle < -Precision::Angular()){
                std::swap(GeoId1, GeoId2);
                std::swap(PosId1, PosId2);
                ActAngle = -ActAngle;
            }

            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('AngleViaPoint',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,GeoId2,GeoId3,PosId3,ActAngle);

            if (bothexternal || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                selection[0].getFeatName(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);

            return;
        };

    } else if (SubNames.size() < 3) {

        bool bothexternal = areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2);

        if (isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }

        if (isEdge(GeoId2,PosId2)) { // line to line angle

            const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
            const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);
            if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
                geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment*>(geom1);
                const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment*>(geom2);

                // find the two closest line ends
                Sketcher::PointPos PosId1 = Sketcher::none;
                Sketcher::PointPos PosId2 = Sketcher::none;
                Base::Vector3d p1[2], p2[2];
                p1[0] = lineSeg1->getStartPoint();
                p1[1] = lineSeg1->getEndPoint();
                p2[0] = lineSeg2->getStartPoint();
                p2[1] = lineSeg2->getEndPoint();

                // Get the intersection point in 2d of the two lines if possible
                Base::Line2d line1(Base::Vector2d(p1[0].x, p1[0].y), Base::Vector2d(p1[1].x, p1[1].y));
                Base::Line2d line2(Base::Vector2d(p2[0].x, p2[0].y), Base::Vector2d(p2[1].x, p2[1].y));
                Base::Vector2d s;
                if (line1.Intersect(line2, s)) {
                    // get the end points of the line segments that are closest to the intersection point
                    Base::Vector3d s3d(s.x, s.y, p1[0].z);
                    if (Base::DistanceP2(s3d, p1[0]) < Base::DistanceP2(s3d, p1[1]))
                        PosId1 = Sketcher::start;
                    else
                        PosId1 = Sketcher::end;
                    if (Base::DistanceP2(s3d, p2[0]) < Base::DistanceP2(s3d, p2[1]))
                        PosId2 = Sketcher::start;
                    else
                        PosId2 = Sketcher::end;
                }
                else {
                    // if all points are collinear
                    double length = DBL_MAX;
                    for (int i=0; i <= 1; i++) {
                        for (int j=0; j <= 1; j++) {
                            double tmp = Base::DistanceP2(p2[j], p1[i]);
                            if (tmp < length) {
                                length = tmp;
                                PosId1 = i ? Sketcher::end : Sketcher::start;
                                PosId2 = j ? Sketcher::end : Sketcher::start;
                            }
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
                    Base::Vector3d dist = (p1[0] - p2[0]) % dir1;
                    if (dist.Sqr() > Precision::Intersection()) {
                        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Parallel lines"),
                            QObject::tr("An angle constraint cannot be set for two parallel lines."));
                        return;
                    }
                }

                double ActAngle = atan2(dir1.x*dir2.y-dir1.y*dir2.x,
                                        dir1.y*dir2.y+dir1.x*dir2.x);
                if (ActAngle < 0) {
                    ActAngle *= -1;
                    std::swap(GeoId1,GeoId2);
                    std::swap(PosId1,PosId2);
                }

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActAngle);

                if (bothexternal || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                    selection[0].getFeatName(),ConStr.size()-1,"False");
                    finishDistanceConstraint(this, Obj,false);
                }
                else
                    finishDistanceConstraint(this, Obj,true);

                return;
            }
        } else if (isEdge(GeoId1,PosId1)) { // line angle
            if (GeoId1 < 0 && GeoId1 >= Sketcher::GeoEnum::VAxis) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                    QObject::tr("Cannot add an angle constraint on an axis!"));
                return;
            }

            const Part::Geometry *geom = Obj->getGeometry(GeoId1);
            if (geom->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
                const Part::GeomLineSegment *lineSeg;
                lineSeg = static_cast<const Part::GeomLineSegment*>(geom);
                Base::Vector3d dir = lineSeg->getEndPoint()-lineSeg->getStartPoint();
                double ActAngle = atan2(dir.y,dir.x);

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,ActAngle);

                if (GeoId1 <= Sketcher::GeoEnum::RefExt || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                    selection[0].getFeatName(),ConStr.size()-1,"False");
                    finishDistanceConstraint(this, Obj,false);
                }
                else
                    finishDistanceConstraint(this, Obj,true);

                return;
            }
            else if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
                const Part::GeomArcOfCircle *arc;
                arc = static_cast<const Part::GeomArcOfCircle*>(geom);
                double startangle, endangle;
                arc->getRange(startangle, endangle, /*EmulateCCWXY=*/true);
                double angle = endangle - startangle;

                openCommand("Add angle constraint");
                Gui::Command::doCommand(
                    Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                    selection[0].getFeatName(),GeoId1,angle);

                if (GeoId1 <= Sketcher::GeoEnum::RefExt || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                    const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                    Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                    selection[0].getFeatName(),ConStr.size()-1,"False");
                    finishDistanceConstraint(this, Obj,false);
                }
                else
                    finishDistanceConstraint(this, Obj,true);

                return;
            }
        }
    };

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select one or two lines from the sketch. Or select two edges and a point."));
    return;
}

void CmdSketcherConstrainAngle::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef, GeoId3 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none, PosId3 = Sketcher::none;

    switch (seqIndex) {
    case 0: // {SelEdge, SelEdgeOrAxis}
    case 1: // {SelEdgeOrAxis, SelEdge}
    case 2: // {SelEdge, SelExternalEdge}
    case 3: // {SelExternalEdge, SelEdge}
    case 4: // {SelExternalEdge, SelExternalEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;

        const Part::Geometry *geom1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geom2 = Obj->getGeometry(GeoId2);
        if (geom1->getTypeId() == Part::GeomLineSegment::getClassTypeId() &&
            geom2->getTypeId() == Part::GeomLineSegment::getClassTypeId()) {
            const Part::GeomLineSegment *lineSeg1 = static_cast<const Part::GeomLineSegment*>(geom1);
            const Part::GeomLineSegment *lineSeg2 = static_cast<const Part::GeomLineSegment*>(geom2);

            // find the two closest line ends
            Sketcher::PointPos PosId1 = Sketcher::none;
            Sketcher::PointPos PosId2 = Sketcher::none;
            Base::Vector3d p1[2], p2[2];
            p1[0] = lineSeg1->getStartPoint();
            p1[1] = lineSeg1->getEndPoint();
            p2[0] = lineSeg2->getStartPoint();
            p2[1] = lineSeg2->getEndPoint();

            // Get the intersection point in 2d of the two lines if possible
            Base::Line2d line1(Base::Vector2d(p1[0].x, p1[0].y), Base::Vector2d(p1[1].x, p1[1].y));
            Base::Line2d line2(Base::Vector2d(p2[0].x, p2[0].y), Base::Vector2d(p2[1].x, p2[1].y));
            Base::Vector2d s;
            if (line1.Intersect(line2, s)) {
                // get the end points of the line segments that are closest to the intersection point
                Base::Vector3d s3d(s.x, s.y, p1[0].z);
                if (Base::DistanceP2(s3d, p1[0]) < Base::DistanceP2(s3d, p1[1]))
                    PosId1 = Sketcher::start;
                else
                    PosId1 = Sketcher::end;
                if (Base::DistanceP2(s3d, p2[0]) < Base::DistanceP2(s3d, p2[1]))
                    PosId2 = Sketcher::start;
                else
                    PosId2 = Sketcher::end;
            }
            else {
                // if all points are collinear
                double length = DBL_MAX;
                for (int i=0; i <= 1; i++) {
                    for (int j=0; j <= 1; j++) {
                        double tmp = Base::DistanceP2(p2[j], p1[i]);
                        if (tmp < length) {
                            length = tmp;
                            PosId1 = i ? Sketcher::end : Sketcher::start;
                            PosId2 = j ? Sketcher::end : Sketcher::start;
                        }
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
                Base::Vector3d dist = (p1[0] - p2[0]) % dir1;
                if (dist.Sqr() > Precision::Intersection()) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Parallel lines"),
                        QObject::tr("An angle constraint cannot be set for two parallel lines."));
                    return;
                }
            }

            double ActAngle = atan2(dir1.x*dir2.y-dir1.y*dir2.x,
                                    dir1.y*dir2.y+dir1.x*dir2.x);
            if (ActAngle < 0) {
                ActAngle *= -1;
                std::swap(GeoId1,GeoId2);
                std::swap(PosId1,PosId2);
            }

            openCommand("Add angle constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,ActAngle);

            if (areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2) || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
                const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

                Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
                Obj->getNameInDocument(),ConStr.size()-1,"False");
                finishDistanceConstraint(this, Obj,false);
            }
            else
                finishDistanceConstraint(this, Obj,true);

            return;
        }
        return;
    }
    case 5: // {SelEdge, SelVertexOrRoot, SelEdgeOrAxis}
    case 6: // {SelEdgeOrAxis, SelVertexOrRoot, SelEdge}
    case 7: // {SelEdge, SelVertexOrRoot, SelExternalEdge}
    case 8: // {SelExternalEdge, SelVertexOrRoot, SelEdge}
    case 9: // {SelExternalEdge, SelVertexOrRoot, SelExternalEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId3 = selSeq.at(1).PosId;
        break;
    }
    case 10: // {SelVertexOrRoot, SelEdge, SelEdgeOrAxis}
    case 11: // {SelVertexOrRoot, SelEdgeOrAxis, SelEdge}
    case 12: // {SelVertexOrRoot, SelEdge, SelExternalEdge}
    case 13: // {SelVertexOrRoot, SelExternalEdge, SelEdge}
    case 14: // {SelVertexOrRoot, SelExternalEdge, SelExternalEdge}
    {
        GeoId1 = selSeq.at(1).GeoId; GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(0).GeoId;
        PosId3 = selSeq.at(0).PosId;
        break;
    }
    }


    bool bothexternal=areBothPointsOrSegmentsFixed(Obj,GeoId1, GeoId2);

    if (isEdge(GeoId1, PosId1) && isEdge(GeoId2, PosId2) && isVertex(GeoId3, PosId3)) {
        double ActAngle = 0.0;

        openCommand("Add angle constraint");

        //add missing point-on-object constraints
        if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
        };
        if(! IsPointAlreadyOnCurve(GeoId2, GeoId3, PosId3, Obj)){
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                Obj->getNameInDocument(),GeoId3,PosId3,GeoId2);
        };
        if(! IsPointAlreadyOnCurve(GeoId1, GeoId3, PosId3, Obj)){//FIXME: it's a good idea to add a check if the sketch is solved
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('PointOnObject',%d,%d,%d)) ",
                Obj->getNameInDocument(),GeoId3,PosId3,GeoId1);
        };

        //assuming point-on-curves have been solved, calculate the angle.
        //DeepSOIC: this may be slow, but I wanted to reuse the conversion from Geometry to GCS shapes that is done in Sketch
        Base::Vector3d p = Obj->getPoint(GeoId3, PosId3 );
        ActAngle = Obj->calculateAngleViaPoint(GeoId1,GeoId2,p.x,p.y);

        //negative constraint value avoidance
        if (ActAngle < -Precision::Angular()){
            std::swap(GeoId1, GeoId2);
            std::swap(PosId1, PosId2);
            ActAngle = -ActAngle;
        }

        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('AngleViaPoint',%d,%d,%d,%d,%f)) ",
            Obj->getNameInDocument(),GeoId1,GeoId2,GeoId3,PosId3,ActAngle);

        if (bothexternal || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            Obj->getNameInDocument(),ConStr.size()-1,"False");
            finishDistanceConstraint(this, Obj,false);
        }
        else
            finishDistanceConstraint(this, Obj,true);

        return;
    };

}

void CmdSketcherConstrainAngle::updateAction(int mode)
{
    switch (mode) {
    case Reference:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle_Driven"));
        break;
    case Driving:
        if (getAction())
            getAction()->setIcon(Gui::BitmapFactory().iconFromTheme("Constraint_InternalAngle"));
        break;
    }
}

// ======================================================================================

class CmdSketcherConstrainEqual : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainEqual();
    virtual ~CmdSketcherConstrainEqual(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainEqual"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainEqual::CmdSketcherConstrainEqual()
    :CmdSketcherConstraint("Sketcher_ConstrainEqual")
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

    allowedSelSequences = {{SelEdge, SelEdge}, {SelEdge, SelExternalEdge},
                           {SelExternalEdge, SelEdge}}; // Only option for equal constraint
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainEqual::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                            new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two edges from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // go through the selected subelements

    if (SubNames.size() < 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool lineSel = false, arcSel = false, circSel = false, ellipsSel = false, arcEllipsSel=false, hasAlreadyExternal = false;
    bool hyperbSel = false, parabSel=false;

    for (std::vector<std::string>::const_iterator it=SubNames.begin(); it != SubNames.end(); ++it) {

        int GeoId;
        Sketcher::PointPos PosId;
        getIdsFromName(*it, Obj, GeoId, PosId);

        if (!isEdge(GeoId,PosId)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two or more compatible edges"));
            return;
        }
        else if (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Sketch axes cannot be used in equality constraints"));
                return;
        }
        else if (isPointOrSegmentFixed(Obj,GeoId)) {

            if (hasAlreadyExternal) {
                showNoConstraintBetweenFixedGeometry();
                return;
            }
            else {
                hasAlreadyExternal = true;
            }
        }

        const Part::Geometry *geo = Obj->getGeometry(GeoId);

        if(geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
            // unsupported as they are generally hereogeneus shapes
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Equality for B-spline edge currently unsupported."));
            return;
        }

        if (geo->getTypeId() == Part::GeomLineSegment::getClassTypeId())
            lineSel = true;
        else if (geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId())
            arcSel = true;
        else if (geo->getTypeId() == Part::GeomCircle::getClassTypeId())
            circSel = true;
        else if (geo->getTypeId() == Part::GeomEllipse::getClassTypeId())
            ellipsSel = true;
        else if (geo->getTypeId() == Part::GeomArcOfEllipse::getClassTypeId())
            arcEllipsSel = true;
        else if (geo->getTypeId() == Part::GeomArcOfHyperbola::getClassTypeId())
            hyperbSel = true;
        else if (geo->getTypeId() == Part::GeomArcOfParabola::getClassTypeId())
            parabSel = true;
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two or more edges of similar type"));
            return;
        }

        ids.push_back(GeoId);
    }

    // Check for heterogeneous groups in selection
    if ( (lineSel && ((arcSel || circSel) || (ellipsSel || arcEllipsSel) || hyperbSel || parabSel) ) ||
         ((arcSel || circSel) && ((ellipsSel || arcEllipsSel) || hyperbSel || parabSel)) ||
         ((ellipsSel || arcEllipsSel) && (hyperbSel || parabSel)) ||
         (hyperbSel && parabSel) ) {

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
    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
}

void CmdSketcherConstrainEqual::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
    QString strError;

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef;

    switch (seqIndex) {
    case 0: // {SelEdge, SelEdge}
    case 1: // {SelEdge, SelExternalEdge}
    case 2: // {SelExternalEdge, SelEdge}
    {
        GeoId1 = selSeq.at(0).GeoId; GeoId2 = selSeq.at(1).GeoId;

        // check if the edge already has a Block constraint
        if ( areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }

        // undo command open
        openCommand("add equality constraint");
        Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Equal',%d,%d)) ",
            Obj->getNameInDocument(), GeoId1, GeoId2);
        // finish the transaction and update
        commitCommand();
        tryAutoRecompute(Obj);

        return;
    }
    default:
        break;
    }
}

// ======================================================================================

class CmdSketcherConstrainSymmetric : public CmdSketcherConstraint
{
public:
    CmdSketcherConstrainSymmetric();
    virtual ~CmdSketcherConstrainSymmetric(){}
    virtual const char* className() const
    { return "CmdSketcherConstrainSymmetric"; }
protected:
    virtual void activated(int iMsg);
    virtual void applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex);
};

CmdSketcherConstrainSymmetric::CmdSketcherConstrainSymmetric()
    :CmdSketcherConstraint("Sketcher_ConstrainSymmetric")
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

    allowedSelSequences = {{SelEdge, SelVertexOrRoot},
                           {SelExternalEdge, SelVertex},
                           {SelVertex, SelEdge, SelVertexOrRoot},
                           {SelRoot, SelEdge, SelVertex},
                           {SelVertex, SelExternalEdge, SelVertexOrRoot},
                           {SelRoot, SelExternalEdge, SelVertex},
                           {SelVertex, SelEdgeOrAxis, SelVertex},
                           {SelVertex, SelVertexOrRoot, SelVertex},
                           {SelVertex, SelVertex, SelVertexOrRoot},
                           {SelVertexOrRoot, SelVertex, SelVertex}};
    constraintCursor = cursor_genericconstraint;
}

void CmdSketcherConstrainSymmetric::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool constraintMode = hGrp->GetBool("ContinuousConstraintMode", true);

        if (constraintMode) {
            ActivateHandler(getActiveGuiDocument(),
                            new DrawSketchHandlerGenConstraint(constraintCursor, this));
            getSelection().clearSelection();
        } else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                                             "or a line and a symmetry point from the sketch."));
        }
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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
        if ( areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }
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
                tryAutoRecompute(Obj);

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

    if ( areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3) ) {
      showNoConstraintBetweenFixedGeometry();
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
                tryAutoRecompute(Obj);

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
            tryAutoRecompute(Obj);

            // clear the selection (convenience)
            getSelection().clearSelection();
            return;
        }
    }
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                    "or a line and a symmetry point from the sketch."));
}

void CmdSketcherConstrainSymmetric::applyConstraint(std::vector<SelIdPair> &selSeq, int seqIndex)
{
    SketcherGui::ViewProviderSketch* sketchgui = static_cast<SketcherGui::ViewProviderSketch*>(getActiveGuiDocument()->getInEdit());
    Sketcher::SketchObject* Obj = sketchgui->getSketchObject();
    QString strError;

    int GeoId1 = Constraint::GeoUndef, GeoId2 = Constraint::GeoUndef, GeoId3 = Constraint::GeoUndef;
    Sketcher::PointPos PosId1 = Sketcher::none, PosId2 = Sketcher::none, PosId3 = Sketcher::none;

    switch (seqIndex) {
    case 0: // {SelEdge, SelVertexOrRoot}
    case 1: // {SelExternalEdge, SelVertex}
    {
        GeoId1 = GeoId2 = selSeq.at(0).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId1 = Sketcher::start; PosId2 = Sketcher::end; PosId3 = selSeq.at(1).PosId;

        if (GeoId1 == GeoId3) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a symmetry constraint between a line and its end points!"));
            return;
        }

        if ( areBothPointsOrSegmentsFixed(Obj, GeoId1, GeoId2) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }
        break;
    }
    case 2: // {SelVertex, SelEdge, SelVertexOrRoot}
    case 3: // {SelRoot, SelEdge, SelVertex}
    case 4: // {SelVertex, SelExternalEdge, SelVertexOrRoot}
    case 5: // {SelRoot, SelExternalEdge, SelVertex}
    case 6: // {SelVertex, SelEdgeOrAxis, SelVertex}
    {
        GeoId1 = selSeq.at(0).GeoId;  GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId;  PosId2 = selSeq.at(2).PosId;

        if ( areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }
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
                Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,GeoId3);

            // finish the transaction and update
            commitCommand();
            tryAutoRecompute(Obj);
        }
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two points and a symmetry line, two points and a symmetry point "
                            "or a line and a symmetry point from the sketch."));
        }

        return;
    }
    case 7: // {SelVertex, SelVertexOrRoot, SelVertex}
    case 8: // {SelVertex, SelVertex, SelVertexOrRoot}
    case 9: // {SelVertexOrRoot, SelVertex, SelVertex}
    {
        GeoId1 = selSeq.at(0).GeoId;  GeoId2 = selSeq.at(2).GeoId; GeoId3 = selSeq.at(1).GeoId;
        PosId1 = selSeq.at(0).PosId;  PosId2 = selSeq.at(2).PosId; PosId3 = selSeq.at(1).PosId;

        if ( areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3) ) {
            showNoConstraintBetweenFixedGeometry();
            return;
        }
        break;
    }
    default:
        break;
    }

    // undo command open
    openCommand("add symmetric constraint");
    Gui::Command::doCommand(
        Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Symmetric',%d,%d,%d,%d,%d,%d)) ",
        Obj->getNameInDocument(),GeoId1,PosId1,GeoId2,PosId2,GeoId3,PosId3);

    // finish the transaction and update
    commitCommand();

    tryAutoRecompute(Obj);

    // clear the selection (convenience)
    getSelection().clearSelection();
    return;
}

// ======================================================================================

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
    Q_UNUSED(iMsg);
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

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
            strError = QObject::tr("Selected objects are not just geometry from one sketch.", dmbg);
            throw Base::ValueError("");
        }

        // get the needed lists and objects
        Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());
        const std::vector<std::string> &SubNames = selection[0].getSubNames();

        if (SubNames.size() != 3) {
            strError = QObject::tr("Number of selected objects is not 3 (is %1).", dmbg).arg(SubNames.size());
            throw Base::ValueError("");
        }

        int GeoId1, GeoId2, GeoId3;
        Sketcher::PointPos PosId1, PosId2, PosId3;
        getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);
        getIdsFromName(SubNames[2], Obj, GeoId3, PosId3);

        //sink the edge to be the last item
        if (isEdge(GeoId1,PosId1) ) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }
        if (isEdge(GeoId2,PosId2) ) {
            std::swap(GeoId2,GeoId3);
            std::swap(PosId2,PosId3);
        }

        //a bunch of validity checks
        if (areAllPointsOrSegmentsFixed(Obj, GeoId1, GeoId2, GeoId3) ) {
            strError = QObject::tr("Cannot create constraint with external geometry only!!", dmbg);
            throw Base::ValueError("");
        }

        if (!(isVertex(GeoId1,PosId1) && !isSimpleVertex(Obj, GeoId1, PosId1) &&
              isVertex(GeoId2,PosId2) && !isSimpleVertex(Obj, GeoId2, PosId2) &&
              isEdge(GeoId3,PosId3)   )) {
            strError = QObject::tr("Incompatible geometry is selected!", dmbg);
            throw Base::ValueError("");
        };

        const Part::Geometry *geo = Obj->getGeometry(GeoId3);

        if( geo && geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId() ){
            // unsupported until normal to B-spline at any point implemented.
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                 QObject::tr("SnellsLaw on B-spline edge currently unsupported."));
            return;
        }

        double n2divn1=0;

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
        // Unable to bind, because the constraint does not yet exist

        if (dlg.exec() != QDialog::Accepted) return;
        ui_Datum.labelEdit->pushToHistory();

        Base::Quantity newQuant = ui_Datum.labelEdit->value();
        n2divn1 = newQuant.getValue();

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

        /*if (allexternal || constraintCreationMode==Reference) { // it is a constraint on a external line, make it non-driving
            const std::vector<Sketcher::Constraint *> &ConStr = Obj->Constraints.getValues();

            Gui::Command::doCommand(Doc,"App.ActiveDocument.%s.setDriving(%i,%s)",
            selection[0].getFeatName(),ConStr.size()-1,"False");
        }*/

        commitCommand();
        tryAutoRecompute(Obj);

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

// ======================================================================================

DEF_STD_CMD_A(CmdSketcherConstrainInternalAlignment);

CmdSketcherConstrainInternalAlignment::CmdSketcherConstrainInternalAlignment()
    :Command("Sketcher_ConstrainInternalAlignment")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Constrain InternalAlignment");
    sToolTipText    = QT_TR_NOOP("Constrains an element to be aligned with the internal geometry of another element");
    sWhatsThis      = "Sketcher_ConstrainInternalAlignment";
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_InternalAlignment";
    sAccel          = "Ctrl+A";
    eType           = ForEdit;
}

void CmdSketcherConstrainInternalAlignment::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select at least one ellipse and one edge from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

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

        if (isPointOrSegmentFixed(Obj,GeoId)) {
            if (GeoId == Sketcher::GeoEnum::HAxis || GeoId == Sketcher::GeoEnum::VAxis) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                                     QObject::tr("Sketch axes cannot be used in internal alignment constraint"));
                return;
            }
            else if (hasAlreadyExternal) {
                showNoConstraintBetweenExternal();
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
                QObject::tr("You cannot internally constrain an ellipse on other ellipse. Select only one ellipse."));
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
                    default:
                        break;
                }
            }
        }

        if(major && minor && focus1 && focus2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Nothing to constrain"),
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

            tryAutoRecompute(Obj);

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
                QObject::tr("You cannot internally constrain an arc of ellipse on another arc of ellipse. Select only one arc of ellipse."));
            return;
        }

        if(ellipseids.size()>0){
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("You cannot internally constrain an ellipse on an arc of ellipse. Select only one ellipse or arc of ellipse."));
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
                    default:
                        break;
                }
            }
        }

        if(major && minor && focus1 && focus2) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Nothing to constrain"),
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

            tryAutoRecompute(Obj);

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

// ======================================================================================
/*** Creation Mode / Toggle to or from Reference ***/
DEF_STD_CMD_A(CmdSketcherToggleDrivingConstraint);

CmdSketcherToggleDrivingConstraint::CmdSketcherToggleDrivingConstraint()
    :Command("Sketcher_ToggleDrivingConstraint")
{
    sAppModule      = "Sketcher";
    sGroup          = QT_TR_NOOP("Sketcher");
    sMenuText       = QT_TR_NOOP("Toggle reference/driving constraint");
    sToolTipText    = QT_TR_NOOP("Toggles the toolbar or selected constraints to/from reference mode");
    sWhatsThis      = "Sketcher_ToggleDrivingConstraint";
    sStatusTip      = sToolTipText;
    sPixmap         = "Sketcher_ToggleConstraint";
    sAccel          = "";
    eType           = ForEdit;

    // list of toggle driving constraint commands
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainLock");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistance");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistanceX");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDistanceY");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainRadius");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainDiameter");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainAngle");
    rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_CompConstrainRadDia");
  //rcCmdMgr.addCommandMode("ToggleDrivingConstraint", "Sketcher_ConstrainSnellsLaw");
}

void CmdSketcherToggleDrivingConstraint::activated(int iMsg)
{
    Q_UNUSED(iMsg);
    bool modeChange=true;

    std::vector<Gui::SelectionObject> selection;

    if (Gui::Selection().countObjectsOfType(Sketcher::SketchObject::getClassTypeId()) > 0){
        // Now we check whether we have a constraint selected or not.

        // get the selection
        selection = getSelection().getSelectionEx();

        // only one sketch with its subelements are allowed to be selected
        if (selection.size() != 1 || !selection[0].isObjectTypeOf(Sketcher::SketchObject::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select constraint(s) from the sketch."));
            return;
        }

        // get the needed lists and objects
        const std::vector<std::string> &SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select constraint(s) from the sketch."));
            return;
        }

        for (std::vector<std::string>::const_iterator it=SubNames.begin();it!=SubNames.end();++it){
            // see if we have constraints, if we do it is not a mode change, but a toggle.
            if (it->size() > 10 && it->substr(0,10) == "Constraint")
                modeChange=false;
        }
    }

    if (modeChange) {
        // Here starts the code for mode change
        Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

        if (constraintCreationMode == Driving) {
            constraintCreationMode = Reference;
        }
        else {
            constraintCreationMode = Driving;
        }

        rcCmdMgr.updateCommands("ToggleDrivingConstraint", static_cast<int>(constraintCreationMode));
    }
    else // toggle the selected constraint(s)
    {
        Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

        // get the needed lists and objects
        const std::vector<std::string> &SubNames = selection[0].getSubNames();
        if (SubNames.empty()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select constraint(s) from the sketch."));
            return;
        }

        // undo command open
        openCommand("Toggle driving from/to non-driving");

        int successful=SubNames.size();
        // go through the selected subelements
        for (std::vector<std::string>::const_iterator it=SubNames.begin();it!=SubNames.end();++it){
            // only handle constraints
            if (it->size() > 10 && it->substr(0,10) == "Constraint") {
                int ConstrId = Sketcher::PropertyConstraintList::getIndexFromConstraintName(*it);
                try {
                    // issue the actual commands to toggle
                    doCommand(Doc,"App.ActiveDocument.%s.toggleDriving(%d) ",selection[0].getFeatName(),ConstrId);
                }
                catch(const Base::Exception&) {
                    successful--;
                }
            }
        }

        if (successful > 0)
            commitCommand();
        else
            abortCommand();

        tryAutoRecompute(Obj);

        // clear the selection (convenience)
        getSelection().clearSelection();
    }
}

bool CmdSketcherToggleDrivingConstraint::isActive(void)
{
    return isCreateGeoActive( getActiveGuiDocument() );
}

void CreateSketcherCommandsConstraints(void)
{
    Gui::CommandManager &rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherConstrainHorizontal());
    rcCmdMgr.addCommand(new CmdSketcherConstrainVertical());
    rcCmdMgr.addCommand(new CmdSketcherConstrainLock());
    rcCmdMgr.addCommand(new CmdSketcherConstrainBlock());
    rcCmdMgr.addCommand(new CmdSketcherConstrainCoincident());
    rcCmdMgr.addCommand(new CmdSketcherConstrainParallel());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPerpendicular());
    rcCmdMgr.addCommand(new CmdSketcherConstrainTangent());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistance());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceX());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDistanceY());
    rcCmdMgr.addCommand(new CmdSketcherConstrainRadius());
    rcCmdMgr.addCommand(new CmdSketcherConstrainDiameter());
    rcCmdMgr.addCommand(new CmdSketcherCompConstrainRadDia());
    rcCmdMgr.addCommand(new CmdSketcherConstrainAngle());
    rcCmdMgr.addCommand(new CmdSketcherConstrainEqual());
    rcCmdMgr.addCommand(new CmdSketcherConstrainPointOnObject());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSymmetric());
    rcCmdMgr.addCommand(new CmdSketcherConstrainSnellsLaw());
    rcCmdMgr.addCommand(new CmdSketcherConstrainInternalAlignment());
    rcCmdMgr.addCommand(new CmdSketcherToggleDrivingConstraint());
}
