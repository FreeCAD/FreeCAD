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
# include <QMessageBox>
#endif

#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Gui/DlgEditFileIncludeProptertyExternal.h>

#include <Mod/Part/App/Geometry.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ViewProviderSketch.h"
#include "EditDatumDialog.h"

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

    // Ask for the value of the distance immediately
    EditDatumDialog *editDatumDialog = new EditDatumDialog(sketch, ConStr.size() - 1);
    editDatumDialog->exec(false);
    delete editDatumDialog;

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
        GeoId = std::atoi(name.substr(4,4000).c_str());
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
        GeoId = -3 - std::atoi(name.substr(12,4000).c_str());
    else if (name.size() > 6 && name.substr(0,6) == "Vertex") {
        int VtId = std::atoi(name.substr(6,4000).c_str());
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
    else if (PosId == Sketcher::mid &&
             (geo->getTypeId() == Part::GeomCircle::getClassTypeId() ||
              geo->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()))
        return true;
    else
        return false;
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
    sWhatsThis      = sToolTipText;
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
            int GeoId=std::atoi(it->substr(4,4000).c_str());

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
    sWhatsThis      = sToolTipText;
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
            int index=std::atoi(it->substr(4,4000).c_str());

            const Part::Geometry *geo = Obj->getGeometry(index);
            if (geo->getTypeId() != Part::GeomLineSegment::getClassTypeId()) {
                QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                                     QObject::tr("The selected edge is not a line segment"));
                return;
            }

            // check if the edge has already a Horizontal or Vertical constraint
            for (std::vector< Sketcher::Constraint * >::const_iterator it= vals.begin();
                 it != vals.end(); ++it) {
                if ((*it)->Type == Sketcher::Horizontal && (*it)->First == index) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Impossible constraint"),
                        QObject::tr("The selected edge has already a horizontal constraint!"));
                    return;
                }
                if ((*it)->Type == Sketcher::Vertical && (*it)->First == index) {
                    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Double constraint"),
                        QObject::tr("The selected edge has already a vertical constraint!"));
                    return;
                }
            }
            ids.push_back(index);
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
    sWhatsThis      = sToolTipText;
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
    sWhatsThis      = sToolTipText;
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
    openCommand("add coincident constraint");
    for (std::size_t i=1; i<SubNames.size(); i++) {
        getIdsFromName(SubNames[i], Obj, GeoId2, PosId2);
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Coincident',%d,%d,%d,%d)) ",
            selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2);
    }

    // finish the transaction and update
    commitCommand();
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Length";
    sAccel          = "D";
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
        commitCommand();

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
            commitCommand();

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
            commitCommand();

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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_PointOnObject";
    sAccel          = "O";
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
            geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId() ) {

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
    sWhatsThis      = sToolTipText;
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
        commitCommand();

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
            commitCommand();

            finishDistanceConstraint(this, Obj);

            return;
        }
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed x-coordinate

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a fixed x-cootdinate constraint on an external geometry!")
                            : QObject::tr("Cannot add a fixed x-cootdinate constraint on the root point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActX = pnt.x;

        openCommand("add fixed x-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceX',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActX);
        commitCommand();

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
    sWhatsThis      = sToolTipText;
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
        commitCommand();

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
            commitCommand();

            finishDistanceConstraint(this, Obj);

            return;
        }
    }
    else if (isVertex(GeoId1,PosId1) && GeoId2 == Constraint::GeoUndef) { // point on fixed y-coordinate

        if (GeoId1 < 0) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                GeoId1 < -2 ? QObject::tr("Cannot add a fixed y-cootdinate constraint on an external geometry!")
                            : QObject::tr("Cannot add a fixed y-cootdinate constraint on the root point!"));
            return;
        }

        Base::Vector3d pnt = Obj->getPoint(GeoId1,PosId1);
        double ActY = pnt.y;

        openCommand("add fixed y-coordinate constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('DistanceY',%d,%d,%f)) ",
            selection[0].getFeatName(),GeoId1,PosId1,ActY);
        commitCommand();

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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Parallel";
    sAccel          = "P";
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
    sToolTipText    = QT_TR_NOOP("Create a Perpendicular constraint between two lines");
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Perpendicular";
    sAccel          = "N";
    eType           = ForEdit;
}

void CmdSketcherConstrainPerpendicular::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two entities from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly two entities from the sketch."));
        return;
    }

    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;

    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // perpendicularity at common point

        if (isSimpleVertex(Obj, GeoId1, PosId1) ||
            isSimpleVertex(Obj, GeoId2, PosId2)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a perpendicularity constraint at an unconnected point!"));
            return;
        }

        const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
        if ((PosId1 != Sketcher::start && PosId1 != Sketcher::end) ||
            (PosId2 != Sketcher::start && PosId2 != Sketcher::end) ||
            (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
             geo1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId()) ||
            (geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
             geo2->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())) {

            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("The selected points should be end points of arcs and lines."));
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
             (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2))) { // connecting point
        if (isVertex(GeoId2,PosId2)) {
            std::swap(GeoId1,GeoId2);
            std::swap(PosId1,PosId2);
        }

        if (isSimpleVertex(Obj, GeoId1, PosId1)) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Cannot add a perpendicularity constraint at an unconnected point!"));
            return;
        }

        const Part::Geometry *geo1 = Obj->getGeometry(GeoId1);
        const Part::Geometry *geo2 = Obj->getGeometry(GeoId2);
        if ((PosId1 != Sketcher::start && PosId1 != Sketcher::end) ||
            (geo1->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
             geo1->getTypeId() != Part::GeomArcOfCircle::getClassTypeId())) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("The selected point should be an end point of an arc or line."));
            return;
        }
        else if (geo2->getTypeId() != Part::GeomLineSegment::getClassTypeId() &&
                 geo2->getTypeId() != Part::GeomArcOfCircle::getClassTypeId() &&
                 geo2->getTypeId() != Part::GeomCircle::getClassTypeId()) {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("The selected edge should be an arc, line or circle."));
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

        openCommand("add perpendicular constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Perpendicular',%d,%d)) ",
            selection[0].getFeatName(),GeoId1,GeoId2);
        commitCommand();
        updateActive();
        getSelection().clearSelection();
        return;
    }
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly two entities from the sketch."));
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_Tangent";
    sAccel          = "T";
    eType           = ForEdit;
}

void CmdSketcherConstrainTangent::activated(int iMsg)
{
    // get the selection
    std::vector<Gui::SelectionObject> selection = getSelection().getSelectionEx();

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select two entities from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 2) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly two entities from the sketch."));
        return;
    }

    int GeoId1, GeoId2;
    Sketcher::PointPos PosId1, PosId2;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;

    if (isVertex(GeoId1,PosId1) && isVertex(GeoId2,PosId2)) { // tangency at common point

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
             (isEdge(GeoId1,PosId1) && isVertex(GeoId2,PosId2))) { // tangency point
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

        openCommand("add tangent constraint");
        Gui::Command::doCommand(
            Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Tangent',%d,%d)) ",
            selection[0].getFeatName(),GeoId1,GeoId2);
        commitCommand();
        updateActive();
        getSelection().clearSelection();
        return;
    }
    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly two entities from the sketch."));
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
    sWhatsThis      = sToolTipText;
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
            QObject::tr("Select exactly one arc or circle from the sketch."));
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string> &SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = dynamic_cast<Sketcher::SketchObject*>(selection[0].getObject());

    if (SubNames.size() != 1) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
            QObject::tr("Select exactly one arc or circle from the sketch."));
        return;
    }

    if (SubNames[0].size() > 4 && SubNames[0].substr(0,4) == "Edge") {
        int GeoId = std::atoi(SubNames[0].substr(4,4000).c_str());

        const Part::Geometry *geom = Obj->getGeometry(GeoId);
        if (geom->getTypeId() == Part::GeomArcOfCircle::getClassTypeId()) {
            const Part::GeomArcOfCircle *arc = dynamic_cast<const Part::GeomArcOfCircle *>(geom);
            double ActRadius = arc->getRadius();

            openCommand("add radius constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                selection[0].getFeatName(),GeoId,ActRadius);
            commitCommand();

            finishDistanceConstraint(this, Obj);
            return;
        }
        else if (geom->getTypeId() == Part::GeomCircle::getClassTypeId()) {
            const Part::GeomCircle *circle = dynamic_cast<const Part::GeomCircle *>(geom);
            double ActRadius = circle->getRadius();

            openCommand("add radius constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Radius',%d,%f)) ",
                selection[0].getFeatName(),GeoId,ActRadius);
            commitCommand();

            finishDistanceConstraint(this, Obj);
            return;
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one arc or circle from the sketch."));
    return;
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
    sWhatsThis      = sToolTipText;
    sStatusTip      = sToolTipText;
    sPixmap         = "Constraint_InternalAngle";
    sAccel          = "A";
    eType           = ForEdit;
}

void CmdSketcherConstrainAngle::activated(int iMsg)
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
            QObject::tr("Select one or two lines from the sketch."));
        return;
    }


    int GeoId1, GeoId2=Constraint::GeoUndef;
    Sketcher::PointPos PosId1, PosId2=Sketcher::none;
    getIdsFromName(SubNames[0], Obj, GeoId1, PosId1);
    if (SubNames.size() == 2)
        getIdsFromName(SubNames[1], Obj, GeoId2, PosId2);

    if (checkBothExternal(GeoId1, GeoId2))
        return;
    else if (isVertex(GeoId1,PosId1) && isEdge(GeoId2,PosId2)) {
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
            double length = 1e10;
            for (int i=0; i <= 1; i++)
                for (int j=0; j <= 1; j++) {
                    double tmp = ((j?p2a:p2b)-(i?p1a:p1b)).Length();
                    if (tmp < length) {
                        length = tmp;
                        PosId1 = i ? Sketcher::start : Sketcher::end;
                        PosId2 = j ? Sketcher::start : Sketcher::end;
                    }
                }

            Base::Vector3d dir1 = ((PosId1 == Sketcher::start) ? 1. : -1.) *
                                  (lineSeg1->getEndPoint()-lineSeg1->getStartPoint());
            Base::Vector3d dir2 = ((PosId2 == Sketcher::start) ? 1. : -1.) *
                                  (lineSeg2->getEndPoint()-lineSeg2->getStartPoint());

            double ActAngle = atan2(-dir1.y*dir2.x+dir1.x*dir2.y,
                                    dir1.x*dir2.x+dir1.y*dir2.y);
            if (ActAngle < 0) {
                ActAngle *= -1;
                std::swap(GeoId1,GeoId2);
                std::swap(PosId1,PosId2);
            }

            openCommand("add angle constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%d,%d,%d,%f)) ",
                selection[0].getFeatName(),GeoId1,PosId1,GeoId2,PosId2,ActAngle);
            commitCommand();

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

            openCommand("add angle constraint");
            Gui::Command::doCommand(
                Doc,"App.ActiveDocument.%s.addConstraint(Sketcher.Constraint('Angle',%d,%f)) ",
                selection[0].getFeatName(),GeoId1,ActAngle);
            commitCommand();

            finishDistanceConstraint(this, Obj);
            return;
        }
    }

    QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
        QObject::tr("Select exactly one or two lines from the sketch."));
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
    sWhatsThis      = sToolTipText;
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
            QObject::tr("Select atleast two lines from the sketch."));
        return;
    }

    std::vector<int> ids;
    bool lineSel = false, arcSel = false, circSel = false, hasAlreadyExternal = false;

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
        else {
            QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Wrong selection"),
                QObject::tr("Select two or more edges of similar type"));
            return;
        }

        ids.push_back(GeoId);
    }

    if (lineSel && (arcSel || circSel)) {
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
    sToolTipText    = QT_TR_NOOP("Create an symmetry constraint between two points with respect to a line");
    sWhatsThis      = sToolTipText;
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
}
