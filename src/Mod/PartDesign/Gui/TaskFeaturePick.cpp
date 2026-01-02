/******************************************************************************
 *   Copyright (c) 2012 Jan Rheinländer                                       *
 *                                      <jrheinlaender@users.sourceforge.net> *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/


#include <QListIterator>
#include <QListWidgetItem>
#include <QTimer>


#include <ranges>

#include <fmt/format.h>

#include <App/Document.h>
#include <App/Origin.h>
#include <App/Datums.h>
#include <App/Part.h>
#include <Base/Console.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Control.h>
#include <Gui/ViewProviderCoordinateSystem.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/ShapeBinder.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/FeaturePrimitive.h>
#include <Mod/Sketcher/App/SketchObject.h>

#include "ui_TaskFeaturePick.h"
#include "TaskFeaturePick.h"
#include "Utils.h"

#include <Gui/ViewParams.h>


using namespace PartDesignGui;
using namespace Attacher;

// TODO Do ve should snap here to App:Part or GeoFeatureGroup/DocumentObjectGroup ? (2015-09-04,
// Fat-Zer)
const QString TaskFeaturePick::getFeatureStatusString(const featureStatus st)
{
    switch (st) {
        case validFeature:
            return tr("Valid");
        case invalidShape:
            return tr("Invalid shape");
        case noWire:
            return tr("No wire in sketch");
        case isUsed:
            return tr("Sketch already used by other feature");
        case otherBody:
            return tr("Belongs to another body");
        case otherPart:
            return tr("Belongs to another part");
        case notInBody:
            return tr("Doesn't belong to any body");
        case basePlane:
            return tr("Base plane");
        case afterTip:
            return tr("Feature is located after the tip of the body");
    }

    return QString();
}

TaskFeaturePick::TaskFeaturePick(
    std::vector<App::DocumentObject*>& objects,
    const std::vector<featureStatus>& status,
    bool singleFeatureSelect,
    QWidget* parent
)
    : TaskBox(Gui::BitmapFactory().pixmap("edit-select-all"), tr("Select Attachment"), true, parent)
    , ui(new Ui_TaskFeaturePick)
    , doSelection(false)
{

    proxy = new QWidget(this);
    ui->setupUi(proxy);

    // clang-format off
    connect(ui->checkUsed, &QCheckBox::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->checkOtherBody, &QCheckBox::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->checkOtherPart, &QCheckBox::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->radioIndependent, &QRadioButton::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->radioDependent, &QRadioButton::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->radioXRef, &QRadioButton::toggled, this, &TaskFeaturePick::onUpdate);
    connect(ui->listWidget, &QListWidget::itemSelectionChanged, this, &TaskFeaturePick::onItemSelectionChanged);
    connect(ui->listWidget, &QListWidget::itemDoubleClicked, this, &TaskFeaturePick::onDoubleClick);
    // clang-format on


    if (!singleFeatureSelect) {
        ui->listWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    }

    // NOTE: generally there shouldn't be more then one origin
    std::map<App::Origin*, Gui::DatumElements> originVisStatus;

    auto statusIt = status.cbegin();
    auto objIt = objects.begin();
    assert(status.size() == objects.size());

    bool attached = false;
    for (; statusIt != status.end(); ++statusIt, ++objIt) {
        QListWidgetItem* item = new QListWidgetItem(QStringLiteral("%1 (%2)").arg(
            QString::fromUtf8((*objIt)->Label.getValue()),
            getFeatureStatusString(*statusIt)
        ));
        item->setData(Qt::UserRole, QString::fromLatin1((*objIt)->getNameInDocument()));
        ui->listWidget->addItem(item);

        App::Document* pDoc = (*objIt)->getDocument();
        documentName = pDoc->getName();
        if (!attached) {
            attached = true;
            attachDocument(Gui::Application::Instance->getDocument(pDoc));
        }

        // check if we need to set any origin in temporary visibility mode
        auto* datum = dynamic_cast<App::DatumElement*>(*objIt);
        if ((*statusIt == validFeature || *statusIt == basePlane) && datum) {
            auto* origin = dynamic_cast<App::Origin*>(datum->getLCS());
            if (origin) {
                if ((*objIt)->isDerivedFrom<App::Plane>()) {
                    originVisStatus[origin].setFlag(Gui::DatumElement::Planes, true);
                }
                else if ((*objIt)->isDerivedFrom<App::Line>()) {
                    originVisStatus[origin].setFlag(Gui::DatumElement::Axes, true);
                }
            }
        }
    }

    // Setup the origin's temporary visibility
    for (const auto& originPair : originVisStatus) {
        const auto& origin = originPair.first;

        auto* vpo = static_cast<Gui::ViewProviderCoordinateSystem*>(
            Gui::Application::Instance->getViewProvider(origin)
        );
        if (vpo) {
            vpo->setTemporaryVisibility(originVisStatus[origin]);
            vpo->setTemporaryScale(Gui::ViewParams::instance()->getDatumTemporaryScaleFactor());
            vpo->setPlaneLabelVisibility(true);
            origins.push_back(vpo);
        }
    }

    // TODO may be update origin API to show only some objects (2015-08-31, Fat-Zer)

    groupLayout()->addWidget(proxy);
    statuses = status;
    updateList();
}

TaskFeaturePick::~TaskFeaturePick()
{
    for (Gui::ViewProviderCoordinateSystem* vpo : origins) {
        vpo->resetTemporaryVisibility();
        vpo->resetTemporarySize();
        vpo->setPlaneLabelVisibility(false);
    }
}

void TaskFeaturePick::updateList()
{
    int index = 0;

    for (auto status : statuses) {
        QListWidgetItem* item = ui->listWidget->item(index);

        switch (status) {
            case validFeature:
                item->setHidden(false);
                break;
            case invalidShape:
                item->setHidden(true);
                break;
            case isUsed:
                item->setHidden(!ui->checkUsed->isChecked());
                break;
            case noWire:
                item->setHidden(true);
                break;
            case otherBody:
                item->setHidden(!ui->checkOtherBody->isChecked());
                break;
            case otherPart:
                item->setHidden(!ui->checkOtherPart->isChecked());
                break;
            case notInBody:
                item->setHidden(!ui->checkOtherPart->isChecked());
                break;
            case basePlane:
                item->setHidden(false);
                break;
            case afterTip:
                item->setHidden(true);
                break;
        }

        index++;
    }
}

void TaskFeaturePick::onUpdate(bool)
{
    bool enable = false;
    if (ui->checkOtherBody->isChecked() || ui->checkOtherPart->isChecked()) {
        enable = true;
    }

    ui->radioDependent->setEnabled(enable);
    ui->radioIndependent->setEnabled(enable);
    ui->radioXRef->setEnabled(enable);

    updateList();
}

std::vector<App::DocumentObject*> TaskFeaturePick::getFeatures()
{
    features.clear();
    QListIterator<QListWidgetItem*> i(ui->listWidget->selectedItems());
    while (i.hasNext()) {

        auto item = i.next();
        if (item->isHidden()) {
            continue;
        }

        QString t = item->data(Qt::UserRole).toString();
        features.push_back(t);
    }

    std::vector<App::DocumentObject*> result;

    for (const auto& feature : features) {
        result.push_back(
            App::GetApplication().getDocument(documentName.c_str())->getObject(feature.toLatin1().data())
        );
    }

    return result;
}

std::vector<App::DocumentObject*> TaskFeaturePick::buildFeatures()
{
    int index = 0;
    std::vector<App::DocumentObject*> result;
    try {
        auto activeBody = PartDesignGui::getBody(false);
        if (!activeBody) {
            return result;
        }

        auto activePart = PartDesignGui::getPartFor(activeBody, false);

        for (auto status : statuses) {
            QListWidgetItem* item = ui->listWidget->item(index);

            if (item->isSelected() && !item->isHidden()) {
                QString t = item->data(Qt::UserRole).toString();
                auto obj = App::GetApplication()
                               .getDocument(documentName.c_str())
                               ->getObject(t.toLatin1().data());

                // build the dependent copy or reference if wanted by the user
                if (status == otherBody || status == otherPart || status == notInBody) {
                    if (!ui->radioXRef->isChecked()) {
                        auto copy = makeCopy(obj, "", ui->radioIndependent->isChecked());

                        if (status == otherBody) {
                            activeBody->addObject(copy);
                        }
                        else if (status == otherPart) {
                            auto oBody = PartDesignGui::getBodyFor(obj, false);
                            if (!oBody) {
                                activePart->addObject(copy);
                            }
                            else {
                                activeBody->addObject(copy);
                            }
                        }
                        else if (status == notInBody) {
                            activeBody->addObject(copy);
                            // doesn't supposed to get here anything but sketch but to be on the
                            // safe side better to check
                            if (copy->isDerivedFrom<Sketcher::SketchObject>()) {
                                Sketcher::SketchObject* sketch
                                    = static_cast<Sketcher::SketchObject*>(copy);
                                PartDesignGui::fixSketchSupport(sketch);
                            }
                        }
                        result.push_back(copy);
                    }
                    else {
                        result.push_back(obj);
                    }
                }
                else {
                    result.push_back(obj);
                }
            }

            index++;
        }
    }
    catch (const Base::Exception& e) {
        e.reportException();
    }
    catch (Py::Exception& e) {
        // reported by code analyzers
        e.clear();
        Base::Console().warning("Unexpected PyCXX exception\n");
    }
    catch (const boost::exception&) {
        // reported by code analyzers
        Base::Console().warning("Unexpected boost exception\n");
    }

    return result;
}

App::DocumentObject* TaskFeaturePick::makeCopy(App::DocumentObject* obj, std::string sub, bool independent)
{

    App::DocumentObject* copy = nullptr;
    // Check for null to avoid segfault
    if (!obj) {
        return copy;
    }
    if (independent
        && (obj->isDerivedFrom<Sketcher::SketchObject>()
            || obj->isDerivedFrom<PartDesign::FeaturePrimitive>())) {

        // we do know that the created instance is a document object, as obj is one. But we do not
        // know which exact type
        auto* doc = App::GetApplication().getActiveDocument();
        const auto name = fmt::format("Copy{}", obj->getNameInDocument());
        copy = doc->addObject(obj->getTypeId().getName(), name.c_str());

        // copy over all properties
        std::vector<App::Property*> props;
        std::vector<App::Property*> cprops;
        obj->getPropertyList(props);
        copy->getPropertyList(cprops);

        auto it = cprops.begin();
        for (App::Property* prop : props) {

            // independent copies don't have links and are not attached
            if (independent
                && (prop->isDerivedFrom<App::PropertyLink>()
                    || prop->isDerivedFrom<App::PropertyLinkList>()
                    || prop->isDerivedFrom<App::PropertyLinkSub>()
                    || prop->isDerivedFrom<App::PropertyLinkSubList>()
                    || (prop->getGroup() && strcmp(prop->getGroup(), "Attachment") == 0))) {

                ++it;
                continue;
            }

            App::Property* cprop = *it++;

            if (prop->getName() && strcmp(prop->getName(), "Label") == 0) {
                static_cast<App::PropertyString*>(cprop)->setValue(name.c_str());
                continue;
            }

            cprop->Paste(*prop);

            // we are a independent copy, therefore no external geometry was copied. WE therefore
            // can delete all constraints
            if (auto* sketchObj = freecad_cast<Sketcher::SketchObject*>(obj)) {
                sketchObj->delConstraintsToExternal();
            }
        }
    }
    else {

        const std::string name = (!independent ? std::string("Reference") : std::string("Copy"))
            + obj->getNameInDocument();
        const std::string entity = sub;

        Part::PropertyPartShape* shapeProp = nullptr;

        // TODO Replace it with commands (2015-09-11, Fat-Zer)
        if (obj->isDerivedFrom<Part::Datum>()) {
            auto* doc = App::GetApplication().getActiveDocument();
            copy = doc->addObject<Part::Datum>(name.c_str());

            // we need to reference the individual datums and make again datums. This is important
            // as datum adjust their size dependent on the part size, hence simply copying the shape
            // is not enough
            long int mode = mmDeactivated;
            Part::Datum* datumCopy = static_cast<Part::Datum*>(copy);

            if (obj->is<PartDesign::Point>()) {
                mode = mm0Vertex;
            }
            else if (obj->is<PartDesign::Line>()) {
                mode = mm1TwoPoints;
            }
            else if (obj->is<PartDesign::Plane>()) {
                mode = mmFlatFace;
            }
            else {
                return copy;
            }

            // TODO Recheck this. This looks strange in case of independent copy (2015-10-31,
            // Fat-Zer)
            if (!independent) {
                datumCopy->AttachmentSupport.setValue(obj, entity.c_str());
                datumCopy->MapMode.setValue(mode);
            }
            else if (!entity.empty()) {
                datumCopy->Shape.setValue(
                    static_cast<Part::Datum*>(obj)->Shape.getShape().getSubShape(entity.c_str())
                );
            }
            else {
                datumCopy->Shape.setValue(static_cast<Part::Datum*>(obj)->Shape.getValue());
            }
        }
        else if (obj->is<PartDesign::ShapeBinder>() || obj->isDerivedFrom<Part::Feature>()) {

            auto* doc = App::GetApplication().getActiveDocument();
            auto* shapeBinderObj = doc->addObject<PartDesign::ShapeBinder>(name.c_str());
            if (!independent) {
                shapeBinderObj->Support.setValue(obj, entity.c_str());
            }
            else {
                shapeProp = &shapeBinderObj->Shape;
            }
            copy = shapeBinderObj;
        }
        else if (obj->isDerivedFrom<App::Plane>() || obj->isDerivedFrom<App::Line>()) {

            auto* doc = App::GetApplication().getActiveDocument();
            auto* shapeBinderObj = doc->addObject<PartDesign::ShapeBinder>(name.c_str());
            if (!independent) {
                shapeBinderObj->Support.setValue(obj, entity.c_str());
            }
            else {
                std::vector<std::string> subvalues;
                subvalues.push_back(entity);
                Part::TopoShape shape
                    = PartDesign::ShapeBinder::buildShapeFromReferences(shapeBinderObj, subvalues);
                shapeBinderObj->Shape.setValue(shape);
            }
            copy = shapeBinderObj;
        }

        if (independent && shapeProp) {
            auto* featureObj = static_cast<Part::Feature*>(obj);
            shapeProp->setValue(
                entity.empty() ? featureObj->Shape.getValue()
                               : featureObj->Shape.getShape().getSubShape(entity.c_str())
            );
        }
    }

    return copy;
}

bool TaskFeaturePick::isSingleSelectionEnabled() const
{
    ParameterGrp::handle hGrp = App::GetApplication()
                                    .GetUserParameter()
                                    .GetGroup("BaseApp")
                                    ->GetGroup("Preferences")
                                    ->GetGroup("Selection");
    return hGrp->GetBool("singleClickFeatureSelect", true);
}

void TaskFeaturePick::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (doSelection) {
        return;
    }
    doSelection = true;
    ui->listWidget->clearSelection();
    for (Gui::SelectionSingleton::SelObj obj : Gui::Selection().getSelection()) {
        for (int row = 0; row < ui->listWidget->count(); row++) {
            QListWidgetItem* item = ui->listWidget->item(row);
            QString t = item->data(Qt::UserRole).toString();
            if (t.compare(QString::fromLatin1(obj.FeatName)) == 0) {
                item->setSelected(true);

                if (msg.Type == Gui::SelectionChanges::AddSelection) {
                    if (isSingleSelectionEnabled()) {
                        QMetaObject::invokeMethod(
                            qobject_cast<Gui::ControlSingleton*>(&Gui::Control()),
                            "accept",
                            Qt::QueuedConnection
                        );
                    }
                }
            }
        }
    }
    doSelection = false;
}

void TaskFeaturePick::onItemSelectionChanged()
{
    if (doSelection) {
        return;
    }
    doSelection = true;
    ui->listWidget->blockSignals(true);
    Gui::Selection().clearSelection();
    for (int row = 0; row < ui->listWidget->count(); row++) {
        QListWidgetItem* item = ui->listWidget->item(row);
        QString t = item->data(Qt::UserRole).toString();
        if (item->isSelected()) {
            Gui::Selection().addSelection(documentName.c_str(), t.toLatin1());
        }
    }
    ui->listWidget->blockSignals(false);
    doSelection = false;
}

void TaskFeaturePick::onDoubleClick(QListWidgetItem* item)
{
    if (doSelection) {
        return;
    }
    doSelection = true;
    QString t = item->data(Qt::UserRole).toString();
    Gui::Selection().addSelection(documentName.c_str(), t.toLatin1());
    doSelection = false;

    QMetaObject::invokeMethod(
        qobject_cast<Gui::ControlSingleton*>(&Gui::Control()),
        "accept",
        Qt::QueuedConnection
    );
}

void TaskFeaturePick::slotDeletedObject(const Gui::ViewProviderDocumentObject& Obj)
{
    if (const auto it = std::ranges::find(origins, &Obj); it != origins.end()) {
        origins.erase(it);
    }
}

void TaskFeaturePick::slotUndoDocument(const Gui::Document&)
{
    if (origins.empty()) {
        QTimer::singleShot(100, &Gui::Control(), &Gui::ControlSingleton::closeDialog);
    }
}

void TaskFeaturePick::slotDeleteDocument(const Gui::Document&)
{
    origins.clear();
    QTimer::singleShot(100, &Gui::Control(), &Gui::ControlSingleton::closeDialog);
}

void TaskFeaturePick::showExternal(bool val)
{
    ui->checkOtherBody->setChecked(val);
    ui->checkOtherPart->setChecked(val);
    updateList();
}


//**************************************************************************
//**************************************************************************
// TaskDialog
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

TaskDlgFeaturePick::TaskDlgFeaturePick(
    std::vector<App::DocumentObject*>& objects,
    const std::vector<TaskFeaturePick::featureStatus>& status,
    std::function<bool(std::vector<App::DocumentObject*>)> afunc,
    std::function<void(std::vector<App::DocumentObject*>)> wfunc,
    bool singleFeatureSelect,
    std::function<void(void)> abortfunc /* = NULL */
)
    : TaskDialog()
    , accepted(false)
{
    pick = new TaskFeaturePick(objects, status, singleFeatureSelect);
    Content.push_back(pick);

    acceptFunction = afunc;
    workFunction = wfunc;
    abortFunction = abortfunc;
}

TaskDlgFeaturePick::~TaskDlgFeaturePick()
{
    // do the work now as before in accept() the dialog is still open, hence the work
    // function could not open another dialog
    if (accepted) {
        try {
            workFunction(pick->buildFeatures());
        }
        catch (...) {
        }
    }
    else if (abortFunction) {

        // Get rid of the TaskFeaturePick before the TaskDialog dtor does. The
        // TaskFeaturePick holds pointers to things (ie any implicitly created
        // Body objects) that might be modified/removed by abortFunction.
        for (auto it : Content) {
            delete it;
        }
        Content.clear();

        try {
            abortFunction();
        }
        catch (...) {
        }
    }
}

//==== calls from the TaskView ===============================================================


void TaskDlgFeaturePick::open()
{}

void TaskDlgFeaturePick::clicked(int)
{}

bool TaskDlgFeaturePick::accept()
{
    accepted = acceptFunction(pick->getFeatures());
    return accepted;
}

bool TaskDlgFeaturePick::reject()
{
    accepted = false;
    return true;
}

void TaskDlgFeaturePick::showExternal(bool val)
{
    pick->showExternal(val);
}


#include "moc_TaskFeaturePick.cpp"
