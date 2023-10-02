/**************************************************************************
*   Copyright (c) 2022 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
# include <TopoDS.hxx>
# include <TopoDS_Face.hxx>
# include <boost/signals2.hpp>
# include <map>
# include <string>
# include <vector>
# include <QMessageBox>
#endif

#include "SketchWorkflow.h"
#include "DlgActiveBody.h"
#include "TaskFeaturePick.h"
#include "Utils.h"
#include "ViewProviderBody.h"
#include "WorkflowManager.h"
#include "ui_DlgReference.h"
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/Part/App/Attacher.h>
#include <Mod/Part/App/TopoShape.h>

#include <App/Document.h>
#include <App/Origin.h>
#include <App/OriginFeature.h>
#include <App/Part.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/SelectionFilter.h>

using namespace PartDesignGui;

namespace {
struct RejectException
{

};

struct WrongSelectionException
{

};

struct WrongSupportException
{

};

struct SupportNotPlanarException
{

};

struct MissingPlanesException
{

};

class SupportFaceValidator
{
public:
    explicit SupportFaceValidator(Gui::SelectionObject faceSelection)
        : faceSelection(faceSelection)
    {
    }

    void handleSelectedBody(PartDesign::Body* activeBody)
    {
        App::DocumentObject* object = faceSelection.getObject();
        std::vector<std::string> elements = faceSelection.getSubNames();

        // In case the selected face belongs to the body then it means its
        // Display Mode Body is set to Tip. But the body face is not allowed
        // to be used as support because otherwise it would cause a cyclic
        // dependency. So, instead we use the tip object as reference.
        // https://forum.freecad.org/viewtopic.php?f=3&t=37448
        if (object == activeBody) {
            App::DocumentObject* tip = activeBody->Tip.getValue();
            if (tip && tip->isDerivedFrom(Part::Feature::getClassTypeId()) && elements.size() == 1) {
                Gui::SelectionChanges msg;
                msg.pDocName = faceSelection.getDocName();
                msg.pObjectName = tip->getNameInDocument();
                msg.pSubName = elements[0].c_str();
                msg.pTypeName = tip->getTypeId().getName();

                faceSelection = Gui::SelectionObject{msg};

                // automatically switch to 'Through' mode
                setThroughModeOfBody(activeBody);
            }
        }
    }

    void throwIfInvalid()
    {
        App::DocumentObject* object = faceSelection.getObject();
        std::vector<std::string> elements = faceSelection.getSubNames();

        Part::Feature* partobject = dynamic_cast<Part::Feature*>(object);
        if (!partobject) {
            throw WrongSelectionException();
        }

        if (elements.size() != 1) {
            throw WrongSelectionException();
        }

        // get the selected sub shape (a Face)
        const Part::TopoShape &shape = partobject->Shape.getValue();
        Part::TopoShape subshape(shape.getSubShape(elements[0].c_str()));
        if (subshape.isNull()) {
            throw WrongSupportException();
        }

        if (!subshape.isPlanar()) {
            throw SupportNotPlanarException();
        }
    }

    std::string getSupport() const
    {
        return faceSelection.getAsPropertyLinkSubString();
    }

    App::DocumentObject* getObject() const
    {
        return faceSelection.getObject();
    }

private:
    void setThroughModeOfBody(PartDesign::Body* activeBody)
    {
        // automatically switch to 'Through' mode
        PartDesignGui::ViewProviderBody* vpBody = dynamic_cast<PartDesignGui::ViewProviderBody*>
                (Gui::Application::Instance->getViewProvider(activeBody));
        if (vpBody) {
            vpBody->DisplayModeBody.setValue("Through");
        }
    }

private:
    mutable Gui::SelectionObject faceSelection;
};

class SupportPlaneValidator
{
public:
    explicit SupportPlaneValidator(Gui::SelectionObject faceSelection)
        : faceSelection(faceSelection)
    {
    }

    std::string getSupport() const
    {
        return Gui::Command::getObjectCmd(getObject(), "(",",'')");
    }

    App::DocumentObject* getObject() const
    {
        return faceSelection.getObject();
    }

private:
    mutable Gui::SelectionObject faceSelection;
};

class SketchPreselection
{
public:
    SketchPreselection(Gui::Document* guidocument, PartDesign::Body* activeBody,
                       std::tuple<Gui::SelectionFilter, Gui::SelectionFilter> filter)
        : guidocument(guidocument)
        , activeBody(activeBody)
        , faceFilter(std::get<0>(filter))
        , planeFilter(std::get<1>(filter))
    {
    }

    bool matches()
    {
        return faceFilter.match() || planeFilter.match();
    }

    std::string getSupport() const
    {
        return supportString;
    }

    void createSupport()
    {
        createBodyOrThrow();

        // get the selected object
        App::DocumentObject* selectedObject{};

        if (faceFilter.match()) {
            Gui::SelectionObject faceSelObject = faceFilter.Result[0][0];
            SupportFaceValidator validator{faceSelObject};
            validator.handleSelectedBody(activeBody);
            validator.throwIfInvalid();

            selectedObject = validator.getObject();
            supportString = validator.getSupport();
        }
        else {
            SupportPlaneValidator validator(planeFilter.Result[0][0]);
            selectedObject = validator.getObject();
            supportString = validator.getSupport();
        }

        handleIfSupportOutOfBody(selectedObject);
    }

    void createSketchOnSupport(const std::string& supportString)
    {
        // create Sketch on Face or Plane
        App::Document* appdocument = guidocument->getDocument();
        std::string FeatName = appdocument->getUniqueObjectName("Sketch");

        guidocument->openCommand(QT_TRANSLATE_NOOP("Command", "Create a Sketch on Face"));
        FCMD_OBJ_CMD(activeBody, "newObject('Sketcher::SketchObject','" << FeatName << "')");
        auto Feat = appdocument->getObject(FeatName.c_str());
        FCMD_OBJ_CMD(Feat, "Support = " << supportString);
        FCMD_OBJ_CMD(Feat, "MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
        Gui::Command::updateActive();
        PartDesignGui::setEdit(Feat, activeBody);
    }

private:
    void createBodyOrThrow()
    {
        if (!activeBody) {
            activeBody = PartDesignGui::getBody( /* messageIfNot = */ true );
            if (activeBody) {
                tryAddNewBodyToActivePart();
            }
            else {
                throw RejectException();
            }
        }
    }

    void tryAddNewBodyToActivePart()
    {
        App::Part *activePart = PartDesignGui::getActivePart();
        if (activePart) {
            activePart->addObject(activeBody);
        }
    }

    void handleIfSupportOutOfBody(App::DocumentObject* selectedObject)
    {
        if (!activeBody->hasObject(selectedObject)) {
            if ( !selectedObject->isDerivedFrom ( App::Plane::getClassTypeId() ) )  {
                // TODO check here if the plane associated with right part/body (2015-09-01, Fat-Zer)

                //check the prerequisites for the selected objects
                //the user has to decide which option we should take if external references are used
                // TODO share this with UnifiedDatumCommand() (2015-10-20, Fat-Zer)
                QDialog dia(Gui::getMainWindow());
                PartDesignGui::Ui_DlgReference dlg;
                dlg.setupUi(&dia);
                dia.setModal(true);
                int result = dia.exec();
                if (result == QDialog::Rejected) {
                    throw RejectException();
                }

                if (!dlg.radioXRef->isChecked()) {
                    guidocument->openCommand(QT_TRANSLATE_NOOP("Command", "Make copy"));
                    auto copy = makeCopy(selectedObject, dlg.radioIndependent->isChecked());
                    supportString = supportFromCopy(copy);
                    guidocument->commitCommand();
                }
            }
        }
    }

    App::DocumentObject* makeCopy(App::DocumentObject* selectedObject, bool independent)
    {
        std::string sub;
        if (faceFilter.match())
            sub = faceFilter.Result[0][0].getSubNames()[0];
        auto copy = PartDesignGui::TaskFeaturePick::makeCopy(selectedObject, sub, independent);

        addToBodyOrPart(copy);

        return copy;
    }

    std::string supportFromCopy(App::DocumentObject* copy)
    {
        std::string supportString;
        if (planeFilter.match()) {
            supportString = Gui::Command::getObjectCmd(copy,"(",",'')");
        }
        else {
            //it is ensured that only a single face is selected, hence it must always be Face1 of the shapebinder
            supportString = Gui::Command::getObjectCmd(copy,"(",",'Face1')");
        }
        return supportString;
    }

    void addToBodyOrPart(App::DocumentObject* object)
    {
        auto activePart = PartDesignGui::getPartFor(activeBody, false);
        if (activeBody) {
            activeBody->addObject(object);
        }
        else if (activePart) {
            activePart->addObject(object);
        }
    }

private:
    Gui::Document* guidocument;
    PartDesign::Body* activeBody;
    Gui::SelectionFilter faceFilter;
    Gui::SelectionFilter planeFilter;
    std::string supportString;
};

class PlaneFinder
{
public:
    PlaneFinder(App::Document* appdocument, PartDesign::Body* activeBody)
        : appdocument(appdocument)
        , activeBody(activeBody)
    {

    }

    std::vector<App::DocumentObject*> getPlanes() const
    {
        return planes;
    }

    std::vector<PartDesignGui::TaskFeaturePick::featureStatus> getStatus() const
    {
        return status;
    }

    unsigned countValidPlanes() const
    {
        return validPlaneCount;
    }

    void findBasePlanes()
    {
        try {
            tryFindBasePlanes();
        }
        catch (const Base::Exception &ex) {
            Base::Console().Error ("%s\n", ex.what() );
        }
    }

    void findDatumPlanes()
    {
        App::GeoFeatureGroupExtension *geoGroup = getGroupExtensionOfBody();
        auto datumPlanes( appdocument->getObjectsOfType(PartDesign::Plane::getClassTypeId()) );
        for (auto plane : datumPlanes) {
            planes.push_back ( plane );
            // Check whether this plane belongs to the active body
            if ( activeBody->hasObject(plane) ) {
                if ( !activeBody->isAfterInsertPoint ( plane ) ) {
                    validPlaneCount++;
                    status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
                } else {
                    status.push_back(PartDesignGui::TaskFeaturePick::afterTip);
                }
            } else {
                PartDesign::Body *planeBody = PartDesign::Body::findBodyOf (plane);
                if ( planeBody ) {
                    if ( ( geoGroup && geoGroup->hasObject ( planeBody, true ) ) ||
                           !App::GeoFeatureGroupExtension::getGroupOfObject (planeBody) ) {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherBody );
                    } else {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherPart );
                    }
                } else {
                    if ( ( geoGroup && geoGroup->hasObject ( plane, true ) ) ||
                           !App::GeoFeatureGroupExtension::getGroupOfObject ( plane ) ) {
                        status.push_back ( PartDesignGui::TaskFeaturePick::otherPart );
                    } else {
                        status.push_back ( PartDesignGui::TaskFeaturePick::notInBody );
                    }
                }
            }
        }
    }

    void findShapeBinderPlanes()
    {

        // Collect also shape binders consisting of a single planar face
        auto shapeBinders( appdocument->getObjectsOfType(PartDesign::ShapeBinder::getClassTypeId()) );
        auto binders( appdocument->getObjectsOfType(PartDesign::SubShapeBinder::getClassTypeId()) );
        shapeBinders.insert(shapeBinders.end(),binders.begin(),binders.end());
        for (auto binder : shapeBinders) {
            // Check whether this plane belongs to the active body
            if (activeBody->hasObject(binder)) {
                Part::TopoShape shape = static_cast<Part::Feature*>(binder)->Shape.getShape();
                if (shape.isPlanar()) {
                    if (!activeBody->isAfterInsertPoint (binder)) {
                        validPlaneCount++;
                        planes.push_back(binder);
                        status.push_back(PartDesignGui::TaskFeaturePick::validFeature);
                    }
                }
            }
        }
    }

private:
    void tryFindBasePlanes()
    {
        auto* origin = activeBody->getOrigin();
        for (auto plane : origin->planes()) {
            planes.push_back (plane);
            status.push_back(PartDesignGui::TaskFeaturePick::basePlane);
            validPlaneCount++;
        }
    }

    App::GeoFeatureGroupExtension* getGroupExtensionOfBody() const
    {
        App::GeoFeatureGroupExtension *geoGroup{nullptr};
        if (activeBody) {
            auto group( App::GeoFeatureGroupExtension::getGroupOfObject(activeBody) );
            if (group) {
                geoGroup = group->getExtensionByType<App::GeoFeatureGroupExtension>();
            }
        }

        return geoGroup;
    }

private:
    App::Document* appdocument;
    PartDesign::Body* activeBody;
    unsigned validPlaneCount = 0;
    std::vector<App::DocumentObject*> planes;
    std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status;
};

class SketchRequestSelection
{
public:
    SketchRequestSelection(Gui::Document* guidocument, PartDesign::Body* activeBody)
        : guidocument(guidocument)
        , activeBody(activeBody)
    {
    }

    void findSupport()
    {
        try {
            // Start command early, so undo will undo any Body creation
            guidocument->openCommand(QT_TRANSLATE_NOOP("Command", "Create a new Sketch"));
            tryFindSupport();
        }
        catch (const RejectException&) {
            guidocument->abortCommand();
            throw;
        }
        catch (const MissingPlanesException&) {
            guidocument->abortCommand();
            throw;
        }
    }

private:
    void tryFindSupport()
    {
        createBodyOrThrow();
        findAndSelectPlane();
    }

    void createBodyOrThrow()
    {
        if (!activeBody) {
            App::Document* appdocument = guidocument->getDocument();
            activeBody = PartDesignGui::makeBody(appdocument);
            if (activeBody) {
                tryAddNewBodyToActivePart();
            }
            else {
                throw RejectException();
            }
        }
    }

    void tryAddNewBodyToActivePart()
    {
        App::Part *activePart = PartDesignGui::getActivePart();
        if (activePart) {
            activePart->addObject(activeBody);
        }
    }

    void findAndSelectPlane()
    {
        App::Document* appdocument = guidocument->getDocument();
        PlaneFinder planeFinder{appdocument, activeBody};
        planeFinder.findBasePlanes();
        planeFinder.findDatumPlanes();
        planeFinder.findShapeBinderPlanes();

        std::vector<App::DocumentObject*> planes = planeFinder.getPlanes();
        std::vector<PartDesignGui::TaskFeaturePick::featureStatus> status = planeFinder.getStatus();
        unsigned validPlaneCount = planeFinder.countValidPlanes();

        //
        // Lambda definitions
        //
        App::Document* documentOfBody = appdocument;
        PartDesign::Body* partDesignBody = activeBody;

        // Determines if user made a valid selection in dialog
        auto acceptFunction = [](const std::vector<App::DocumentObject*>& features) -> bool {
            return !features.empty();
        };

        // Called by dialog when user hits "OK" and accepter returns true
        auto processFunction = [documentOfBody, partDesignBody](const std::vector<App::DocumentObject*>& features) {
            SketchRequestSelection::createSketch(documentOfBody, partDesignBody, features);
        };

        // Called by dialog for "Cancel", or "OK" if accepter returns false
        std::string docname = documentOfBody->getName();
        auto rejectFunction = [docname]() {
            Gui::Document* document = Gui::Application::Instance->getDocument(docname.c_str());
            if (document)
                document->abortCommand();
        };

        //
        // End of lambda definitions
        //

        if (validPlaneCount == 0) {
            throw MissingPlanesException();
        }
        else if (validPlaneCount == 1) {
            processFunction(planes);
        }
        else if (validPlaneCount > 1) {
            checkForShownDialog();
            Gui::Selection().clearSelection();

            // Show dialog and let user pick plane
            Gui::Control().showDialog(new PartDesignGui::TaskDlgFeaturePick(planes, status, acceptFunction,
                                                                            processFunction, true, rejectFunction));
        }
    }

    void checkForShownDialog()
    {
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        PartDesignGui::TaskDlgFeaturePick *pickDlg = qobject_cast<PartDesignGui::TaskDlgFeaturePick *>(dlg);
        if (dlg && !pickDlg) {
            QMessageBox msgBox;
            msgBox.setText(QObject::tr("A dialog is already open in the task panel"));
            msgBox.setInformativeText(QObject::tr("Do you want to close this dialog?"));
            msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
            msgBox.setDefaultButton(QMessageBox::Yes);
            int ret = msgBox.exec();
            if (ret == QMessageBox::Yes) {
                Gui::Control().closeDialog();
            }
            else {
                throw RejectException();
            }
        }

        if (dlg) {
            Gui::Control().closeDialog();
        }
    }

    static void createSketch(App::Document* documentOfBody, PartDesign::Body* partDesignBody,
                             const std::vector<App::DocumentObject*>& features)
    {
        // may happen when the user switched to an empty document while the
        // dialog is open
        if (features.empty())
            return;
        App::Plane* plane = static_cast<App::Plane*>(features.front());
        std::string FeatName = documentOfBody->getUniqueObjectName("Sketch");
        std::string supportString = Gui::Command::getObjectCmd(plane,"(",",[''])");

        FCMD_OBJ_CMD(partDesignBody,"newObject('Sketcher::SketchObject','" << FeatName << "')");
        auto Feat = partDesignBody->getDocument()->getObject(FeatName.c_str());
        FCMD_OBJ_CMD(Feat,"Support = " << supportString);
        FCMD_OBJ_CMD(Feat,"MapMode = '" << Attacher::AttachEngine::getModeName(Attacher::mmFlatFace)<<"'");
        Gui::Command::updateActive(); // Make sure the Support's Placement property is updated
        PartDesignGui::setEdit(Feat, partDesignBody);
    }

private:
    Gui::Document* guidocument;
    PartDesign::Body* activeBody;
};

}

SketchWorkflow::SketchWorkflow(Gui::Document* document)
    : guidocument(document)
{
    appdocument = guidocument->getDocument();
}

void SketchWorkflow::createSketch()
{
    try {
        tryCreateSketch();
    }
    catch (const RejectException&) {

    }
    catch (const WrongSelectionException&) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("Several sub-elements selected"),
            QObject::tr("You have to select a single face as support for a sketch!"));
    }
    catch (const WrongSupportException&) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No support face selected"),
            QObject::tr("You have to select a face as support for a sketch!"));
    }
    catch (const SupportNotPlanarException&) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No planar support"),
            QObject::tr("You need a planar face as support for a sketch!"));
    }
    catch (const MissingPlanesException&) {
        QMessageBox::warning(Gui::getMainWindow(), QObject::tr("No valid planes in this document"),
            QObject::tr("Please create a plane first or select a face to sketch on"));
    }
}

void SketchWorkflow::tryCreateSketch()
{
    if (PartDesignGui::assureModernWorkflow(appdocument)) {
        createSketchWithModernWorkflow();
    }
    // No PartDesign feature without Body past FreeCAD 0.13
    else if (PartDesignGui::isLegacyWorkflow(appdocument)) {
        createSketchWithLegacyWorkflow();
    }
}

std::tuple<bool, PartDesign::Body*> SketchWorkflow::shouldCreateBody()
{
    auto shouldMakeBody{false};

    // We need either an active Body, or for there to be no Body
    // objects (in which case, just make one) to make a new sketch.
    PartDesign::Body* pdBody = PartDesignGui::getBody(/* messageIfNot = */ false);
    if (!pdBody) {
        if (appdocument->countObjectsOfType(PartDesign::Body::getClassTypeId()) == 0) {
            shouldMakeBody = true;
        }
        else {
            PartDesignGui::DlgActiveBody dia(Gui::getMainWindow(), appdocument);
            if (dia.exec() == QDialog::Accepted) {
                pdBody = dia.getActiveBody();
            }
        }
    }

    return std::make_tuple(shouldMakeBody, pdBody);
}

bool SketchWorkflow::shouldAbort(bool shouldMakeBody) const
{
    return !shouldMakeBody && !activeBody;
}

std::tuple<Gui::SelectionFilter, Gui::SelectionFilter> SketchWorkflow::getFaceAndPlaneFilter() const
{
    // Hint:
    // The behaviour of this command has changed with respect to a selected sketch:
    // It doesn't try any more to edit a selected sketch but always tries to create
    // a new sketch.
    // See https://forum.freecad.org/viewtopic.php?f=3&t=44070

    Gui::SelectionFilter FaceFilter  ("SELECT Part::Feature SUBELEMENT Face COUNT 1");
    Gui::SelectionFilter PlaneFilter ("SELECT App::Plane COUNT 1");
    Gui::SelectionFilter PlaneFilter2("SELECT PartDesign::Plane COUNT 1");

    if (PlaneFilter2.match()) {
        PlaneFilter = PlaneFilter2;
    }
    return std::make_tuple(FaceFilter, PlaneFilter);
}

void SketchWorkflow::createSketchWithModernWorkflow()
{
    auto result = shouldCreateBody();
    auto shouldMakeBody = std::get<0>(result);
    activeBody = std::get<1>(result);
    if (shouldAbort(shouldMakeBody)) {
        return;
    }

    auto faceOrPlaneFilter = getFaceAndPlaneFilter();
    SketchPreselection sketchOnFace{guidocument, activeBody, faceOrPlaneFilter};

    if (sketchOnFace.matches()) {
        // create Sketch on Face or Plane
        sketchOnFace.createSupport();
        sketchOnFace.createSketchOnSupport(sketchOnFace.getSupport());
    }
    else {
        SketchRequestSelection requestSelection{guidocument, activeBody};
        requestSelection.findSupport();
    }
}

void SketchWorkflow::createSketchWithLegacyWorkflow()
{
    Gui::CommandManager& cmdMgr = Gui::Application::Instance->commandManager();
    cmdMgr.runCommandByName("Sketcher_NewSketch");
}
