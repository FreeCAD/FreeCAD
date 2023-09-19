/***************************************************************************
 *   Copyright (c) 2017 Abdullah Tahiri <abdullah.tahiri.yo@gmail.com>     *
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
#include <Inventor/SbString.h>
#include <QApplication>
#include <cfloat>
#endif

#include <App/Application.h>
#include <Base/Console.h>
#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/Notifications.h>
#include <Gui/Selection.h>
#include <Gui/SelectionObject.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "DrawSketchHandler.h"
#include "Utils.h"
#include "ViewProviderSketch.h"


using namespace std;
using namespace SketcherGui;
using namespace Sketcher;

void ActivateBSplineHandler(Gui::Document* doc, DrawSketchHandler* handler)
{
    std::unique_ptr<DrawSketchHandler> ptr(handler);
    if (doc) {
        if (doc->getInEdit()
            && doc->getInEdit()->isDerivedFrom(SketcherGui::ViewProviderSketch::getClassTypeId())) {
            SketcherGui::ViewProviderSketch* vp =
                static_cast<SketcherGui::ViewProviderSketch*>(doc->getInEdit());
            vp->purgeHandler();
            vp->activateHandler(ptr.release());
        }
    }
}

/// For a knot given by (GeoId, PosId) finds the B-Spline and the knot's
/// index within it (by OCC numbering).
/// Returns true if the entities are found, false otherwise.
/// If returns false, `splineGeoId` and `knotIndexOCC` have garbage values.
bool findBSplineAndKnotIndex(Sketcher::SketchObject* Obj,
                             int knotGeoId,
                             Sketcher::PointPos knotPosId,
                             int& splineGeoId,
                             int& knotIndexOCC)
{
    for (auto const constraint : Obj->Constraints.getValues()) {
        if (constraint->Type == Sketcher::InternalAlignment && constraint->First == knotGeoId
            && constraint->AlignmentType == Sketcher::BSplineKnotPoint) {
            splineGeoId = constraint->Second;
            knotIndexOCC = constraint->InternalAlignmentIndex + 1;
            return true;  // we have already found our knot.
        }
    }

    // TODO: what to do if multiple splines have the same first/last point?
    const Part::Geometry* geo = Obj->getGeometry(knotGeoId);
    if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        splineGeoId = knotGeoId;
        switch (knotPosId) {
            case Sketcher::PointPos::start:
                knotIndexOCC = 1;
                return true;
            case Sketcher::PointPos::end:
                knotIndexOCC = static_cast<const Part::GeomBSplineCurve*>(geo)->countKnots();
                return true;
            default:
                // If we reach here something went wrong.
                // isBsplineKnotOrEndPoint (that we expect is run before) will
                // only accept spline knotGeoID if knotPosId is start or end.
                return false;
        }
    }

    return false;
}

// Convert to NURBS
DEF_STD_CMD_A(CmdSketcherConvertToNURBS)

CmdSketcherConvertToNURBS::CmdSketcherConvertToNURBS()
    : Command("Sketcher_BSplineConvertToNURBS")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Convert geometry to B-spline");
    sToolTipText = QT_TR_NOOP("Converts the selected geometry to a B-spline");
    sWhatsThis = "Sketcher_BSplineConvertToNURBS";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineApproximate";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherConvertToNURBS::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand(QT_TRANSLATE_NOOP("Command", "Convert to NURBS"));

    std::vector<int> GeoIdList;

    for (const auto& subName : SubNames) {
        // only handle edges
        if (subName.size() > 4 && subName.substr(0, 4) == "Edge") {
            int GeoId = std::atoi(subName.substr(4, 4000).c_str()) - 1;
            GeoIdList.push_back(GeoId);
        }
        else if (subName.size() > 12 && subName.substr(0, 12) == "ExternalEdge") {
            int GeoId = -(std::atoi(subName.substr(12, 4000).c_str()) + 2);
            GeoIdList.push_back(GeoId);
        }
    }

    // for creating the poles and knots
    for (auto GeoId : GeoIdList) {
        Gui::cmdAppObjectArgs(selection[0].getObject(), "convertToNURBS(%d) ", GeoId);
    }
    for (auto GeoId : GeoIdList) {
        Gui::cmdAppObjectArgs(selection[0].getObject(), "exposeInternalGeometry(%d)", GeoId);
    }

    if (GeoIdList.empty()) {
        abortCommand();

        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("None of the selected elements is an edge."));
    }
    else {
        commitCommand();
    }
    tryAutoRecomputeIfNotSolve(Obj);
}

bool CmdSketcherConvertToNURBS::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}

// Increase degree of the spline
DEF_STD_CMD_A(CmdSketcherIncreaseDegree)

CmdSketcherIncreaseDegree::CmdSketcherIncreaseDegree()
    : Command("Sketcher_BSplineIncreaseDegree")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Increase B-spline degree");
    sToolTipText = QT_TR_NOOP("Increases the degree of the B-spline");
    sWhatsThis = "Sketcher_BSplineIncreaseDegree";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineIncreaseDegree";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherIncreaseDegree::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand(QT_TRANSLATE_NOOP("Command", "Increase spline degree"));

    bool ignored = false;

    for (size_t i = 0; i < SubNames.size(); i++) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0, 4) == "Edge") {
            int GeoId = std::atoi(SubNames[i].substr(4, 4000).c_str()) - 1;
            const Part::Geometry* geo = Obj->getGeometry(GeoId);

            if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "increaseBSplineDegree(%d) ",
                                      GeoId);
                // add new control points
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "exposeInternalGeometry(%d)",
                                      GeoId);
            }
            else {
                ignored = true;
            }
        }
    }

    if (ignored) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("At least one of the selected "
                                               "objects was not a B-Spline and was ignored."));
    }

    commitCommand();
    tryAutoRecomputeIfNotSolve(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherIncreaseDegree::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}


// Decrease degree of the spline
DEF_STD_CMD_A(CmdSketcherDecreaseDegree)

CmdSketcherDecreaseDegree::CmdSketcherDecreaseDegree()
    : Command("Sketcher_BSplineDecreaseDegree")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Decrease B-spline degree");
    sToolTipText = QT_TR_NOOP("Decreases the degree of the B-spline");
    sWhatsThis = "Sketcher_BSplineDecreaseDegree";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineDecreaseDegree";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherDecreaseDegree::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    getSelection().clearSelection();

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand(QT_TRANSLATE_NOOP("Command", "Decrease spline degree"));

    bool ignored = false;

    for (size_t i = 0; i < SubNames.size(); i++) {
        // only handle edges
        if (SubNames[i].size() > 4 && SubNames[i].substr(0, 4) == "Edge") {
            int GeoId = std::atoi(SubNames[i].substr(4, 4000).c_str()) - 1;
            const Part::Geometry* geo = Obj->getGeometry(GeoId);

            if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "decreaseBSplineDegree(%d) ",
                                      GeoId);
                // add new control points
                // Currently exposeInternalGeometry is called from within decreaseBSplineDegree
                // because the old spline is deleted and a new one is added so that the GeoId is
                // invalid afterwards
                // Gui::cmdAppObjectArgs(selection[0].getObject(), "exposeInternalGeometry(%d)",
                // GeoId);
                break;  // cannot handle more than spline because the GeoIds will be invalidated
                        // after the first change
            }
            else {
                ignored = true;
            }
        }
    }

    if (ignored) {
        Gui::TranslatedUserWarning(Obj,
                                   QObject::tr("Wrong selection"),
                                   QObject::tr("At least one of the selected "
                                               "objects was not a B-Spline and was ignored."));
    }

    commitCommand();
    tryAutoRecomputeIfNotSolve(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherDecreaseDegree::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}


DEF_STD_CMD_A(CmdSketcherIncreaseKnotMultiplicity)

CmdSketcherIncreaseKnotMultiplicity::CmdSketcherIncreaseKnotMultiplicity()
    : Command("Sketcher_BSplineIncreaseKnotMultiplicity")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Increase knot multiplicity");
    sToolTipText = QT_TR_NOOP("Increases the multiplicity of the selected knot of a B-spline");
    sWhatsThis = "Sketcher_BSplineIncreaseKnotMultiplicity";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineIncreaseKnotMultiplicity";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherIncreaseKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();

    if (SubNames.size() > 1) {
        // Check that only one object is selected,
        // as we need only one object to get the new GeoId after multiplicity change
        Gui::TranslatedUserWarning(
            getActiveGuiDocument()->getDocument(),
            QObject::tr("Wrong selection"),
            QObject::tr(
                "The selection comprises more than one item. Please select just one knot."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand(QT_TRANSLATE_NOOP("Command", "Increase knot multiplicity"));

    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[0], Obj, GeoId, PosId);

    int splineGeoId;
    int knotIndexOCC;

    bool applied = false;
    bool notaknot = !(isBsplineKnotOrEndPoint(Obj, GeoId, PosId)
                      && findBSplineAndKnotIndex(Obj, GeoId, PosId, splineGeoId, knotIndexOCC));
    boost::uuids::uuid bsplinetag;

    if (!notaknot) {
        bsplinetag = Obj->getGeometry(splineGeoId)->getTag();

        try {
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "modifyBSplineKnotMultiplicity(%d, %d, %d) ",
                                  splineGeoId,
                                  knotIndexOCC,
                                  1);
            applied = true;

            // Warning: GeoId list might have changed
            // as the consequence of deleting pole circles and
            // particularly B-spline GeoID might have changed.
        }
        catch (const Base::CADKernelError& e) {
            e.ReportException();
            if (e.getTranslatable()) {
                Gui::TranslatedUserError(Obj,
                                         QObject::tr("CAD Kernel Error"),
                                         QObject::tr(e.getMessage().c_str()));
            }
            getSelection().clearSelection();
        }
        catch (const Base::Exception& e) {
            e.ReportException();
            if (e.getTranslatable()) {
                Gui::TranslatedUserError(Obj,
                                         QObject::tr("Input Error"),
                                         QObject::tr(e.getMessage().c_str()));
            }
            getSelection().clearSelection();
        }
    }

    if (notaknot) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("None of the selected elements is a knot of a B-spline"));
    }

    if (applied) {
        // find new geoid for B-spline as GeoId might have changed
        const std::vector<Part::Geometry*>& gvals = Obj->getInternalGeometry();

        int ngeoid = 0;
        bool ngfound = false;

        for (std::vector<Part::Geometry*>::const_iterator geo = gvals.begin(); geo != gvals.end();
             geo++, ngeoid++) {
            if ((*geo) && (*geo)->getTag() == bsplinetag) {
                ngfound = true;
                break;
            }
        }

        if (ngfound) {
            try {
                // add internalalignment for new pole
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "exposeInternalGeometry(%d)",
                                      ngeoid);
            }
            catch (const Base::Exception& e) {
                Gui::NotifyUserError(Obj,
                                     QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                                     e.what());
                getSelection().clearSelection();
            }
        }
    }

    if (!applied) {
        abortCommand();
    }
    else {
        commitCommand();
    }

    tryAutoRecomputeIfNotSolve(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherIncreaseKnotMultiplicity::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}

DEF_STD_CMD_A(CmdSketcherDecreaseKnotMultiplicity)

CmdSketcherDecreaseKnotMultiplicity::CmdSketcherDecreaseKnotMultiplicity()
    : Command("Sketcher_BSplineDecreaseKnotMultiplicity")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Decrease knot multiplicity");
    sToolTipText = QT_TR_NOOP("Decreases the multiplicity of the selected knot of a B-spline");
    sWhatsThis = "Sketcher_BSplineDecreaseKnotMultiplicity";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineDecreaseKnotMultiplicity";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherDecreaseKnotMultiplicity::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();

    if (SubNames.size() > 1) {
        // Check that only one object is selected,
        // as we need only one object to get the new GeoId after multiplicity change
        Gui::TranslatedUserWarning(
            getActiveGuiDocument()->getDocument(),
            QObject::tr("Wrong selection"),
            QObject::tr(
                "The selection comprises more than one item. Please select just one knot."));
        return;
    }

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    openCommand(QT_TRANSLATE_NOOP("Command", "Decrease knot multiplicity"));

    int GeoId;
    Sketcher::PointPos PosId;
    getIdsFromName(SubNames[0], Obj, GeoId, PosId);

    int splineGeoId;
    int knotIndexOCC;

    bool applied = false;
    bool notaknot = !(isBsplineKnotOrEndPoint(Obj, GeoId, PosId)
                      && findBSplineAndKnotIndex(Obj, GeoId, PosId, splineGeoId, knotIndexOCC));
    boost::uuids::uuid bsplinetag;

    if (!notaknot) {
        bsplinetag = Obj->getGeometry(splineGeoId)->getTag();

        try {
            Gui::cmdAppObjectArgs(selection[0].getObject(),
                                  "modifyBSplineKnotMultiplicity(%d, %d, %d) ",
                                  splineGeoId,
                                  knotIndexOCC,
                                  -1);
            applied = true;

            // Warning: GeoId list might have changed as the consequence of deleting pole circles
            // and particularly B-spline GeoID might have changed.
        }
        catch (const Base::Exception& e) {
            Gui::TranslatedUserError(Obj,
                                     QObject::tr("Error"),
                                     QObject::tr(getStrippedPythonExceptionString(e).c_str()));

            getSelection().clearSelection();
        }
    }

    if (notaknot) {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("None of the selected elements is a knot of a B-spline"));
    }

    if (applied) {
        // find new geoid for B-spline as GeoId might have changed
        const std::vector<Part::Geometry*>& gvals = Obj->getInternalGeometry();

        int ngeoid = 0;
        bool ngfound = false;

        for (std::vector<Part::Geometry*>::const_iterator geo = gvals.begin(); geo != gvals.end();
             geo++, ngeoid++) {
            if ((*geo) && (*geo)->getTag() == bsplinetag) {
                ngfound = true;
                break;
            }
        }

        if (ngfound) {
            try {
                // add internalalignment for new pole
                Gui::cmdAppObjectArgs(selection[0].getObject(),
                                      "exposeInternalGeometry(%d)",
                                      ngeoid);
            }
            catch (const Base::Exception& e) {
                Gui::NotifyUserError(Obj,
                                     QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                                     e.what());
                getSelection().clearSelection();
            }
        }
    }

    if (!applied) {
        abortCommand();
    }
    else {
        commitCommand();
    }

    tryAutoRecomputeIfNotSolve(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherDecreaseKnotMultiplicity::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}


// Composite drop down for knot increase/decrease
DEF_STD_CMD_ACLU(CmdSketcherCompModifyKnotMultiplicity)

CmdSketcherCompModifyKnotMultiplicity::CmdSketcherCompModifyKnotMultiplicity()
    : Command("Sketcher_CompModifyKnotMultiplicity")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Modify knot multiplicity");
    sToolTipText = QT_TR_NOOP("Modifies the multiplicity of the selected knot of a B-spline");
    sWhatsThis = "Sketcher_CompModifyKnotMultiplicity";
    sStatusTip = sToolTipText;
    eType = ForEdit;
}

void CmdSketcherCompModifyKnotMultiplicity::activated(int iMsg)
{

    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();
    Gui::Command* cmd;

    if (iMsg == 0) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineIncreaseKnotMultiplicity");
    }
    else if (iMsg == 1) {
        cmd = rcCmdMgr.getCommandByName("Sketcher_BSplineDecreaseKnotMultiplicity");
    }
    else {
        return;
    }

    cmd->invoke(0);

    // Since the default icon is reset when enabling/disabling the command we have
    // to explicitly set the icon of the used command.
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    assert(iMsg < a.size());
    pcAction->setIcon(a[iMsg]->icon());
}

Gui::Action* CmdSketcherCompModifyKnotMultiplicity::createAction()
{
    Gui::ActionGroup* pcAction = new Gui::ActionGroup(this, Gui::getMainWindow());
    pcAction->setDropDownMenu(true);
    applyCommandData(this->className(), pcAction);

    QAction* c1 = pcAction->addAction(QString());
    c1->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineIncreaseKnotMultiplicity"));
    QAction* c2 = pcAction->addAction(QString());
    c2->setIcon(Gui::BitmapFactory().iconFromTheme("Sketcher_BSplineDecreaseKnotMultiplicity"));

    _pcAction = pcAction;
    languageChange();

    pcAction->setIcon(c1->icon());
    int defaultId = 0;
    pcAction->setProperty("defaultAction", QVariant(defaultId));

    return pcAction;
}

void CmdSketcherCompModifyKnotMultiplicity::languageChange()
{
    Command::languageChange();

    if (!_pcAction) {
        return;
    }
    Gui::ActionGroup* pcAction = qobject_cast<Gui::ActionGroup*>(_pcAction);
    QList<QAction*> a = pcAction->actions();

    QAction* c1 = a[0];
    c1->setText(QApplication::translate("CmdSketcherCompModifyKnotMultiplicity",
                                        "Increase knot multiplicity"));
    c1->setToolTip(
        QApplication::translate("Sketcher_BSplineIncreaseKnotMultiplicity",
                                "Increases the multiplicity of the selected knot of a B-spline"));
    c1->setStatusTip(
        QApplication::translate("Sketcher_BSplineIncreaseKnotMultiplicity",
                                "Increases the multiplicity of the selected knot of a B-spline"));
    QAction* c2 = a[1];
    c2->setText(QApplication::translate("CmdSketcherCompModifyKnotMultiplicity",
                                        "Decrease knot multiplicity"));
    c2->setToolTip(
        QApplication::translate("Sketcher_BSplineDecreaseKnotMultiplicity",
                                "Decreases the multiplicity of the selected knot of a B-spline"));
    c2->setStatusTip(
        QApplication::translate("Sketcher_BSplineDecreaseKnotMultiplicity",
                                "Decreases the multiplicity of the selected knot of a B-spline"));
}

void CmdSketcherCompModifyKnotMultiplicity::updateAction(int /*mode*/)
{}

bool CmdSketcherCompModifyKnotMultiplicity::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), false);
}

class DrawSketchHandlerBSplineInsertKnot: public DrawSketchHandler
{
public:
    DrawSketchHandlerBSplineInsertKnot(Sketcher::SketchObject* _Obj, int _GeoId)
        : Obj(_Obj)
        , GeoId(_GeoId)
        , EditMarkers(1)
    {
        auto bsp = static_cast<const Part::GeomBSplineCurve*>(Obj->getGeometry(GeoId));
        guessParam = bsp->getFirstParameter();
    }

    ~DrawSketchHandlerBSplineInsertKnot() override
    {}

    void mouseMove(Base::Vector2d onSketchPos) override
    {
        auto bsp = static_cast<const Part::GeomBSplineCurve*>(Obj->getGeometry(GeoId));

        // get closest parameter using OCC
        // TODO: This is called every time we move the cursor. Can get overwhelming.
        Base::Vector3d onSketchPos3d(onSketchPos.x, onSketchPos.y, 0.0);
        SbString text;
        text.sprintf(" %.3f", guessParam);
        // FIXME: Sometimes the "closest" point is on the other end of the B-Spline.
        // Find when it happens and fix it?
        bsp->closestParameter(onSketchPos3d, guessParam);

        Base::Vector3d pointOnCurve3d = bsp->value(guessParam);

        // TODO: Also draw a point at our position instead of just text
        Base::Vector2d pointOnCurve(pointOnCurve3d.x, pointOnCurve3d.y);
        setPositionText(pointOnCurve, text);

        EditMarkers[0] = pointOnCurve;
        drawEditMarkers(EditMarkers);

        applyCursor();
    }

    bool pressButton(Base::Vector2d /*onSketchPos*/) override
    {
        // just here to consume the button press
        return true;
    }

    bool releaseButton(Base::Vector2d onSketchPos) override
    {
        Q_UNUSED(onSketchPos);

        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Insert knot"));

        bool applied = false;
        boost::uuids::uuid bsplinetag = Obj->getGeometry(GeoId)->getTag();

        try {
            Gui::cmdAppObjectArgs(Obj, "insertBSplineKnot(%d, %lf, %d) ", GeoId, guessParam, 1);
            applied = true;

            // Warning: GeoId list might have changed
            // as the consequence of deleting pole circles and
            // particularly B-spline GeoID might have changed.
        }
        catch (const Base::CADKernelError& e) {
            e.ReportException();
            if (e.getTranslatable()) {
                Gui::TranslatedUserError(Obj,
                                         QObject::tr("CAD Kernel Error"),
                                         QObject::tr(e.getMessage().c_str()));
            }
        }
        catch (const Base::Exception& e) {
            e.ReportException();
            if (e.getTranslatable()) {
                Gui::TranslatedUserError(Obj,
                                         QObject::tr("Input Error"),
                                         QObject::tr(e.getMessage().c_str()));
            }
        }

        int newGeoId = 0;
        bool newGeoIdFound = false;

        if (applied) {
            // find new geoid for B-spline as GeoId might have changed
            const std::vector<Part::Geometry*>& gvals = Obj->getInternalGeometry();

            for (std::vector<Part::Geometry*>::const_iterator geo = gvals.begin();
                 geo != gvals.end();
                 geo++, newGeoId++) {
                if ((*geo) && (*geo)->getTag() == bsplinetag) {
                    newGeoIdFound = true;
                    break;
                }
            }

            if (newGeoIdFound) {
                try {
                    // add internalalignment for new pole
                    Gui::cmdAppObjectArgs(Obj, "exposeInternalGeometry(%d)", newGeoId);
                }
                catch (const Base::Exception& e) {
                    Gui::NotifyUserError(Obj,
                                         QT_TRANSLATE_NOOP("Notifications", "Invalid Constraint"),
                                         e.what());
                }
            }
        }

        if (applied) {
            Gui::Command::commitCommand();
        }
        else {
            Gui::Command::abortCommand();
        }

        tryAutoRecomputeIfNotSolve(Obj);

        ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
            "User parameter:BaseApp/Preferences/Mod/Sketcher");
        bool continuousMode = hGrp->GetBool("ContinuousCreationMode", true);
        if (continuousMode && newGeoIdFound) {
            // This code enables the continuous creation mode.

            // The new entities created changed the B-Spline's GeoId
            GeoId = newGeoId;

            applyCursor();
            /* It is ok not to call to purgeHandler
             * in continuous creation mode because the
             * handler is destroyed by the quit() method on pressing the
             * right button of the mouse */
        }
        else {
            sketchgui
                ->purgeHandler();  // no code after this line, Handler get deleted in ViewProvider
        }

        return true;
    }

private:
    QString getCrosshairCursorSVGName() const override
    {
        return QString::fromLatin1("Sketcher_Pointer_InsertKnot");
    }

protected:
    Sketcher::SketchObject* Obj;
    int GeoId;
    double guessParam;
    std::vector<Base::Vector2d> EditMarkers;
};

DEF_STD_CMD_A(CmdSketcherInsertKnot)

CmdSketcherInsertKnot::CmdSketcherInsertKnot()
    : Command("Sketcher_BSplineInsertKnot")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Insert knot");
    sToolTipText = QT_TR_NOOP("Inserts knot at given parameter. If a knot already exists at that "
                              "parameter, it's multiplicity is increased by one.");
    sWhatsThis = "Sketcher_BSplineInsertKnot";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_BSplineInsertKnot";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherInsertKnot::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // TODO: let user click on a curve after pressing command.
    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();
    if (SubNames.empty()) {
        // Check that only one object is selected,
        // as we need only one object to get the new GeoId after multiplicity change
        Gui::TranslatedUserWarning(getActiveGuiDocument()->getDocument(),
                                   QObject::tr("Selection is empty"),
                                   QObject::tr("Nothing is selected. Please select a b-spline."));

        return;
    }
    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    // TODO: Ensure GeoId is for the BSpline and not for it's internal geometry
    int GeoId = std::atoi(SubNames[0].substr(4, 4000).c_str()) - 1;
    const Part::Geometry* geo = Obj->getGeometry(GeoId);

    if (geo->getTypeId() == Part::GeomBSplineCurve::getClassTypeId()) {
        ActivateBSplineHandler(getActiveGuiDocument(),
                               new DrawSketchHandlerBSplineInsertKnot(Obj, GeoId));
    }
    else {
        Gui::TranslatedUserWarning(
            Obj,
            QObject::tr("Wrong selection"),
            QObject::tr("Please select a b-spline curve to insert a knot (not a knot on it). "
                        "If the curve is not a b-spline, please convert it into one first."));
    }

    getSelection().clearSelection();
}

bool CmdSketcherInsertKnot::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}

DEF_STD_CMD_A(CmdSketcherJoinCurves)

CmdSketcherJoinCurves::CmdSketcherJoinCurves()
    : Command("Sketcher_JoinCurves")
{
    sAppModule = "Sketcher";
    sGroup = "Sketcher";
    sMenuText = QT_TR_NOOP("Join curves");
    sToolTipText = QT_TR_NOOP("Join two curves at selected end points");
    sWhatsThis = "Sketcher_JoinCurves";
    sStatusTip = sToolTipText;
    sPixmap = "Sketcher_JoinCurves";
    sAccel = "";
    eType = ForEdit;
}

void CmdSketcherJoinCurves::activated(int iMsg)
{
    Q_UNUSED(iMsg);

    // get the selection
    std::vector<Gui::SelectionObject> selection;
    selection = getSelection().getSelectionEx(nullptr, Sketcher::SketchObject::getClassTypeId());

    // only one sketch with its subelements are allowed to be selected
    if (selection.size() != 1) {
        return;
    }

    // get the needed lists and objects
    const std::vector<std::string>& SubNames = selection[0].getSubNames();

    int GeoIds[2];
    Sketcher::PointPos PosIds[2];

    Sketcher::SketchObject* Obj = static_cast<Sketcher::SketchObject*>(selection[0].getObject());

    switch (SubNames.size()) {
        case 0: {
            // Nothing is selected
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Selection is empty"),
                QObject::tr("Nothing is selected. Please select end points of curves."));
            return;
        }
        case 1: {
            std::vector<int> GeoIdList;
            std::vector<Sketcher::PointPos> PosIdList;

            int selGeoId;
            Sketcher::PointPos selPosId;

            getIdsFromName(SubNames[0], Obj, selGeoId, selPosId);

            Obj->getDirectlyCoincidentPoints(selGeoId, selPosId, GeoIdList, PosIdList);

            // Find the right pair of coincident points
            size_t j = 0;
            for (size_t i = 0; i < GeoIdList.size(); ++i) {
                if (Sketcher::PointPos::start == PosIdList[i]
                    || Sketcher::PointPos::end == PosIdList[i]) {
                    if (j < 2) {
                        GeoIds[j] = GeoIdList[i];
                        PosIds[j] = PosIdList[i];
                        ++j;
                    }
                    else {
                        Gui::TranslatedUserWarning(
                            Obj,
                            QObject::tr("Too many curves on point"),
                            QObject::tr("Exactly two curves should end at the selected point to be "
                                        "able to join them."));

                        return;
                    }
                }
            }
            if (j < 2) {
                Gui::TranslatedUserWarning(Obj,
                                           QObject::tr("Too few curves on point"),
                                           QObject::tr("Exactly two curves should end at the "
                                                       "selected point to be able to join them."));

                return;
            }

            break;
        }
        case 2: {
            getIdsFromName(SubNames[0], Obj, GeoIds[0], PosIds[0]);
            getIdsFromName(SubNames[1], Obj, GeoIds[1], PosIds[1]);
            break;
        }
        default: {
            Gui::TranslatedUserWarning(
                Obj,
                QObject::tr("Wrong selection"),
                QObject::tr("Two end points, or coincident point should be selected."));

            return;
        }
    }

    Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Join Curves"));
    bool applied = false;

    try {
        Gui::cmdAppObjectArgs(selection[0].getObject(),
                              "join(%d, %d, %d, %d) ",
                              GeoIds[0],
                              static_cast<int>(PosIds[0]),
                              GeoIds[1],
                              static_cast<int>(PosIds[1]));
        applied = true;

        // Warning: GeoId list will have changed
    }
    catch (const Base::Exception& e) {
        Gui::TranslatedUserError(Obj,
                                 QObject::tr("Error"),
                                 QObject::tr(getStrippedPythonExceptionString(e).c_str()));

        getSelection().clearSelection();
    }

    if (applied) {
        Gui::Command::commitCommand();
    }
    else {
        Gui::Command::abortCommand();
    }

    tryAutoRecomputeIfNotSolve(Obj);
    getSelection().clearSelection();
}

bool CmdSketcherJoinCurves::isActive()
{
    return isSketcherBSplineActive(getActiveGuiDocument(), true);
}

void CreateSketcherCommandsBSpline()
{
    Gui::CommandManager& rcCmdMgr = Gui::Application::Instance->commandManager();

    rcCmdMgr.addCommand(new CmdSketcherConvertToNURBS());
    rcCmdMgr.addCommand(new CmdSketcherIncreaseDegree());
    rcCmdMgr.addCommand(new CmdSketcherDecreaseDegree());
    rcCmdMgr.addCommand(new CmdSketcherIncreaseKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherDecreaseKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherCompModifyKnotMultiplicity());
    rcCmdMgr.addCommand(new CmdSketcherInsertKnot());
    rcCmdMgr.addCommand(new CmdSketcherJoinCurves());
}
