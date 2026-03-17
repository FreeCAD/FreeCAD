// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2026 Morten Vajhøj                                                     *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#include "TaskMassProperties.h"
#include "Mod/Measure/App/MassPropertiesResult.h"
#include "Mod/Measure/App/MassPropertiesObject.h"
#include "ui_TaskMassProperties.h"

#include <QtCore/QScopedValueRollback>
#include <QKeyEvent>
#include <QTimer>

#include <QtWidgets>
#include <unordered_set>
#include <sstream>
#include <iomanip>
#include <tuple>
#include <vector>

#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Control.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProviderDocumentObject.h>

#include <Base/Console.h>
#include <Base/Exception.h>
#include <Base/Matrix.h>
#include <Base/Parameter.h>
#include <Base/Placement.h>
#include <Base/Precision.h>
#include <Base/Quantity.h>
#include <Base/Rotation.h>
#include <Base/Type.h>
#include <Base/UnitsApi.h>
#include <Base/Vector3D.h>


#include <App/DocumentObject.h>
#include <App/Document.h>
#include <App/Datums.h>
#include <App/Origin.h>
#include <App/GroupExtension.h>
#include <App/Link.h>
#include <App/DocumentObjectGroup.h>
#include <App/DocumentObserver.h>
#include <App/GeoFeature.h>
#include <App/PropertyGeo.h>
#include <App/PropertyStandard.h>
#include <App/PropertyUnits.h>

#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Body.h>

#include <TopoDS_Compound.hxx>
#include <TopoDS_Shape.hxx>
#include <BRep_Builder.hxx>

using namespace MassPropertiesGui;

namespace MassPropertiesGui {

class TaskMassPropertiesWidget : public QWidget
{
public:
    TaskMassPropertiesWidget()
        : coordinateSystemGroup(this)
    {
        ui.setupUi(this);

        coordinateSystemGroup.addButton(ui.centerOfGravityRadioButton);
        coordinateSystemGroup.addButton(ui.customRadioButton);

        shortcutQuit = new QShortcut(this);
        shortcutQuit->setKey(QKeySequence(QStringLiteral("ESC")));
        shortcutQuit->setContext(Qt::ApplicationShortcut);
    }

    QWidget* takePage(QWidget* page)
    {
        ui.mainLayout->removeWidget(page);
        page->setParent(nullptr);
        return page;
    }

    Ui::TaskMassProperties ui;
    QButtonGroup coordinateSystemGroup;
    QShortcut* shortcutQuit = nullptr;
};

} // namespace MassPropertiesGui

MassPropertiesData currentInfo;
std::string currentMode = "Center of gravity";
App::DocumentObject* currentDatum = nullptr;
std::vector<std::tuple<std::string, std::string, std::string>> savedSelection;

static int getPreferredUnitsSchemaIndex()
{
    auto params = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Units"
    );
    const bool ignoreProjectSchema = params->GetBool("IgnoreProjectSchema", false);
    int schemaIndex = params->GetInt("UserSchema", 0);

    if (!ignoreProjectSchema) {
        if (App::Document* doc = App::GetApplication().getActiveDocument()) {
            schemaIndex = doc->UnitSystem.getValue();
        }
    }

    const int schemaCount = static_cast<int>(Base::UnitsApi::count());
    if (schemaIndex < 0 || schemaIndex >= schemaCount) {
        schemaIndex = 0;
    }

    return schemaIndex;
}

static int getUnitsComboIndex(int schemaIndex)
{
    std::string lengthUnit = "mm";

    auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(schemaIndex));
    if (schema) {
        lengthUnit = schema->getBasicLengthUnit();
    }

    if (lengthUnit == "m") {
        return 1;
    }
    if (lengthUnit == "in") {
        return 2;
    }
    if (lengthUnit == "ft") {
        return 3;
    }
    return 0;
}

static int findUnitsSchemaIndex(const char* schemaName, int fallbackSchemaIndex)
{
    const auto names = Base::UnitsApi::getNames();
    for (std::size_t index = 0; index < names.size(); ++index) {
        if (names[index] == schemaName) {
            return static_cast<int>(index);
        }
    }

    return fallbackSchemaIndex;
}

static int getUnitsSchemaIndex(int comboIndex, int preferredSchemaIndex)
{
    switch (comboIndex) {
        case 1:
            return findUnitsSchemaIndex("MKS", preferredSchemaIndex);
        case 2:
            return findUnitsSchemaIndex("Imperial", preferredSchemaIndex);
        case 3:
            return findUnitsSchemaIndex("ImperialCivil", preferredSchemaIndex);
        default:
            return findUnitsSchemaIndex("Internal", preferredSchemaIndex);
    }
}

TaskMassProperties::TaskMassProperties()
    : Gui::SelectionObserver(true)
    , panel(new TaskMassPropertiesWidget)
    , selectingCustomCoordSystem(false)
{
    currentInfo = MassPropertiesData {};
    currentMode = "Center of gravity";
    currentDatum = nullptr;
    hasCurrentDatumPlacement = false;

    qApp->installEventFilter(this);

    if (auto* app = Gui::Application::Instance) {
        if (auto* stdDeleteCommand = app->commandManager().getCommandByName("Std_Delete")) {
            if ((deleteAction = stdDeleteCommand->getAction())) {
                deleteActivated = deleteAction->isEnabled();
                deleteAction->setEnabled(false);
            }
        }
    }

    connect(panel->shortcutQuit, &QShortcut::activated, this, &TaskMassProperties::escape);
    connect(panel->ui.centerOfGravityRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            onCoordinateSystemChanged("Center of gravity");
        }
    });
    connect(panel->ui.customRadioButton, &QRadioButton::toggled, this, [this](bool checked) {
        if (checked) {
            onCoordinateSystemChanged("Custom");
        }
    });
    connect(panel->ui.selectCustomButton, &QPushButton::pressed, this, &TaskMassProperties::onSelectCustomCoordinateSystem);
    connect(panel->ui.cogDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCogDatumButtonPressed);
    connect(panel->ui.covDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCovDatumButtonPressed);
    connect(panel->ui.inertiaLcsButton, &QPushButton::pressed, this, &TaskMassProperties::onLcsButtonPressed);

    const int preferredSchemaIndex = getPreferredUnitsSchemaIndex();

    panel->ui.unitsComboBox->setCurrentIndex(getUnitsComboIndex(preferredSchemaIndex));
    unitsSchemaIndex = getUnitsSchemaIndex(panel->ui.unitsComboBox->currentIndex(), preferredSchemaIndex);

    connect(panel->ui.unitsComboBox, &QComboBox::currentIndexChanged, this, [=](int index) {
        unitsSchemaIndex = getUnitsSchemaIndex(index, preferredSchemaIndex);
        update(Gui::SelectionChanges());
    });

    auto addTaskBox = [this](const char* icon, const QString& title, QWidget* page) {
        auto* box = new Gui::TaskView::TaskBox(Gui::BitmapFactory().pixmap(icon), title, true, nullptr);
        auto* layout = box->groupLayout();
        layout->setContentsMargins(10, 10, 10, 10);
        layout->setSpacing(8);
        layout->addWidget(page);
        Content.emplace_back(box);
    };

    addTaskBox("MassPropertiesIcon", tr("Parameters"), panel->takePage(panel->ui.parametersPage));
    addTaskBox("MassPropertiesIcon", tr("Physical Properties"), panel->takePage(panel->ui.physicalPropertiesPage));
    addTaskBox("Std_Point", tr("Center of Gravity"), panel->takePage(panel->ui.centerOfGravityPage));
    addTaskBox("Std_Point", tr("Center of Volume"), panel->takePage(panel->ui.centerOfVolumePage));
    addTaskBox("Std_CoordinateSystem", tr("Inertia"), panel->takePage(panel->ui.inertiaPage));

    updateInertiaVisibility();
    update(Gui::SelectionChanges());
}

TaskMassProperties::~TaskMassProperties()
{
    qApp->removeEventFilter(this);
    if (deleteAction) {
        deleteAction->setEnabled(deleteActivated);
    }
    delete panel;
}

bool TaskMassProperties::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    if (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Delete && panel->ui.objectList->hasFocus()) {
            QList<QListWidgetItem*> selectedItems = panel->ui.objectList->selectedItems();

            std::vector<QString> toRemove;
            for (auto* item : selectedItems) {
                toRemove.push_back(item->data(Qt::UserRole).toString());
            }

            if (toRemove.size() == panel->ui.objectList->count()) {
                Gui::Selection().clearSelection();
            }

            for (const auto& userData : toRemove) {
                QStringList parts = userData.split(QLatin1Char('|'));
                if (parts.size() == 3) {
                    QByteArray docName = parts[0].toLatin1();
                    QByteArray objName = parts[1].toLatin1();
                    QByteArray subName = parts[2].toLatin1();
                    Gui::Selection().rmvSelection(
                        docName.isEmpty() ? nullptr : docName.constData(),
                        objName.isEmpty() ? nullptr : objName.constData(),
                        subName.isEmpty() ? nullptr : subName.constData()
                    );
                }
            }
            event->accept();
            return true;
        }
    }

    return Gui::TaskView::TaskDialog::eventFilter(watched, event);
}

void TaskMassProperties::modifyStandardButtons(QDialogButtonBox* box)
{
    QPushButton* closeButton = box->button(QDialogButtonBox::Abort);
    closeButton->setText(tr("Close"));

    QPushButton* saveButton = box->button(QDialogButtonBox::Apply);
    saveButton->setText(tr("Save"));
    QObject::connect(saveButton, &QPushButton::released, this, &TaskMassProperties::saveResult);

    QPushButton* resetButton = box->button(QDialogButtonBox::Reset);
    resetButton->setText(tr("Reset"));
    QObject::connect(resetButton, &QPushButton::released, [this]() {
        Gui::Selection().clearSelection();
        removeTemporaryObjects();
        clearUiFields();
        panel->ui.objectList->clear();
    });
}

void TaskMassProperties::invoke()
{
}

bool TaskMassProperties::accept()
{
    return true;
}

bool TaskMassProperties::reject()
{
    removeTemporaryObjects();
    Gui::Control().closeDialog();
    return true;
}

void TaskMassProperties::escape()
{
    if (Gui::Selection().getSelection().empty()) {
        this->reject();
        return;
    }

    Gui::Selection().clearSelection();
    this->removeTemporaryObjects();
    this->clearUiFields();
    panel->ui.objectList->clear();

    selectingCustomCoordSystem = false;
    currentDatum = nullptr;
    hasCurrentDatumPlacement = false;
    panel->ui.customEdit->clear();

    currentInfo = MassPropertiesData {};
}

void TaskMassProperties::removeTemporaryObjects()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }

    if (!doc->getObject("Center_of_Gravity")
        && !doc->getObject("Center_of_Volume")
        && !doc->getObject("Principal_Axes_LCS")) {
        return;
    }

    doc->openTransaction("Remove temporary datum objects");

    if (doc->getObject("Center_of_Gravity")) {
        doc->removeObject("Center_of_Gravity");
    }
    if (doc->getObject("Center_of_Volume")) {
        doc->removeObject("Center_of_Volume");
    }
    if (doc->getObject("Principal_Axes_LCS")) {
        doc->removeObject("Principal_Axes_LCS");
    }

    doc->commitTransaction();
}


void TaskMassProperties::clearUiFields()
{
    panel->ui.volumeEdit->clear();
    panel->ui.massEdit->clear();
    panel->ui.densityEdit->clear();
    panel->ui.surfaceAreaEdit->clear();

    panel->ui.cogXText->clear();
    panel->ui.cogYText->clear();
    panel->ui.cogZText->clear();

    panel->ui.covXText->clear();
    panel->ui.covYText->clear();
    panel->ui.covZText->clear();

    panel->ui.inertiaJoxText->clear();
    panel->ui.inertiaJxyText->clear();
    panel->ui.inertiaJzxText->clear();
    panel->ui.inertiaJoyText->clear();
    panel->ui.inertiaJzyText->clear();
    panel->ui.inertiaJozText->clear();

    panel->ui.inertiaJxText->clear();
    panel->ui.inertiaJyText->clear();
    panel->ui.inertiaJzText->clear();

    panel->ui.axisInertiaText->clear();
}

void TaskMassProperties::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (isUpdating) return;

    if (msg.Type != Gui::SelectionChanges::AddSelection
        && msg.Type != Gui::SelectionChanges::RmvSelection
        && msg.Type != Gui::SelectionChanges::SetSelection
        && msg.Type != Gui::SelectionChanges::ClrSelection) {

        return;
    }

    update(msg);
}

void TaskMassProperties::update(const Gui::SelectionChanges& msg)
{
    try {
        tryupdate();
    }
    catch (const Base::Exception& e) {
        Base::Console().error("Mass Properties update failed: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().error("Mass Properties update failed: %s\n", e.what());
    }
}


void TaskMassProperties::tryupdate()
{
    if (isUpdating) return;

    QScopedValueRollback<bool> updatingGuard(isUpdating, true);

    auto guiSelection = Gui::Selection().getSelection(nullptr, Gui::ResolveMode::NoResolve);
    
    if (guiSelection.empty()) {
        clearUiFields();
        objectsToMeasure.clear();
        panel->ui.objectList->clear();
        removeTemporaryObjects();
        return;
    }

    if (!selectingCustomCoordSystem) {
        bool hasElementSelection = false;
        bool hasInvisibleSelection = false;
        std::vector<std::tuple<App::DocumentObject*, std::string, bool>> selectedObjects;
        selectedObjects.reserve(guiSelection.size());

        for (const auto& sel : guiSelection) {
            if (!sel.pObject) {
                continue;
            }

            App::DocumentObject* pickedObject = sel.pObject;
            if (sel.pResolvedObject && sel.pResolvedObject != sel.pObject) {
                pickedObject = sel.pResolvedObject;
            }
            if (sel.SubName && sel.SubName[0]) {
                App::SubObjectT sub(sel.pObject, sel.SubName);
                if (auto* leaf = sub.getSubObject()) {
                    pickedObject = leaf;
                }
            }

            bool isVisible = true;
            Gui::Document* guiDoc = Gui::Application::Instance->getDocument(pickedObject->getDocument());
            if (guiDoc) {
                auto* viewProvider = dynamic_cast<Gui::ViewProviderDocumentObject*>(guiDoc->getViewProvider(pickedObject));
                if (viewProvider && !viewProvider->Visibility.getValue()) {
                    isVisible = false;
                }
            }

            if (!isVisible) {
                hasInvisibleSelection = true;
            }

            if (sel.SubName && sel.SubName[0]) {
                App::SubObjectT sub(sel.pObject, sel.SubName);
                std::string subNoElement = sub.getSubNameNoElement();

                bool canPickShape = !subNoElement.empty() || !sel.pResolvedObject || sel.pResolvedObject == sel.pObject;

                if (canPickShape && subNoElement != sel.SubName) {
                    hasElementSelection = true;
                }
                if (canPickShape) {
                    selectedObjects.emplace_back(sel.pObject, subNoElement, isVisible);
                }
                else {
                    selectedObjects.emplace_back(sel.pObject, sel.SubName, isVisible);
                }
            }
            else {
                selectedObjects.emplace_back(sel.pObject, std::string(), isVisible);
            }
        }

        if (hasElementSelection || hasInvisibleSelection) {
            std::unordered_set<std::string> seen;
            isUpdating = true;
            Gui::Selection().clearSelection();

            for (const auto& selected : selectedObjects) {
                bool isVisible = std::get<2>(selected);
                if (!isVisible) {
                    continue;
                }

                App::DocumentObject* obj = std::get<0>(selected);
                if (!obj || !obj->getDocument()) {
                    continue;
                }

                const std::string& subName = std::get<1>(selected);
                std::string key = obj->getDocument()->getName();
                key += '|' + obj->getNameInDocument() + '|' + subName;

                if (!seen.insert(key).second) {
                    continue;
                }

                if (subName.empty()) {
                    Gui::Selection().addSelection(obj->getDocument()->getName(), obj->getNameInDocument());
                }
                else {
                    Gui::Selection().addSelection(obj->getDocument()->getName(), obj->getNameInDocument(), subName.c_str());
                }
            }

            isUpdating = false;
            tryupdate();

            return;
        }
    }

    objectsToMeasure.clear();
    App::DocumentObject const* referenceDatum = nullptr;
    
    panel->ui.objectList->clear();
    
    auto coordLabel = [](App::DocumentObject* obj) {
        if (auto* datum = dynamic_cast<App::DatumElement*>(obj)) {
            if (auto* lcs = datum->getLCS()) {
                return lcs->getFullLabel();
            }
        }
        if (auto* lcs = dynamic_cast<App::LocalCoordinateSystem*>(obj)) {
            return lcs->getFullLabel();
        }
        if (auto* origin = dynamic_cast<App::Origin*>(obj)) {
            return origin->getFullLabel();
        }

        return obj->getFullLabel();
    };


    auto isReferenceObject = [](App::DocumentObject* obj) {
        if (!obj) {
            return false;
        }
        auto datum = dynamic_cast<App::DatumElement*>(obj);
        if (datum && datum->getLCS()) {
            return true;
        }
        if (dynamic_cast<App::LocalCoordinateSystem*>(obj)) {
            return true;
        }
        if (dynamic_cast<App::Origin*>(obj)) {
            return true;
        }
        if (obj->getTypeId().getName() == std::string("PartDesign::CoordinateSystem")) {
            return true;
        }

        return obj->isDerivedFrom<App::Line>();
    };

    auto getPlacementFromObject = [](App::DocumentObject* obj) {
        if (!obj) {
            return Base::Placement();
        }
        if (auto* prop = dynamic_cast<App::PropertyPlacement*>(obj->getPropertyByName("Placement"))) {
            return prop->getValue();
        }
        return Base::Placement();
    };

    auto getGlobalPlacement = [&](App::DocumentObject* root, const char* subname, App::DocumentObject* resolvedObject = nullptr) {
        if (!root) {
            return Base::Placement();
        }

        if (!subname || !subname[0]) {
            if (resolvedObject && resolvedObject != root) {
                return App::GeoFeature::getGlobalPlacement(resolvedObject);
            }
            return App::GeoFeature::getGlobalPlacement(root);
        }

        App::SubObjectT sub(root, subname);
        std::string subNoElement = sub.getSubNameNoElement();
        
        if (subNoElement.empty()) {
            if (resolvedObject && resolvedObject != root) {
                return App::GeoFeature::getGlobalPlacement(resolvedObject);
            }
            return App::GeoFeature::getGlobalPlacement(root);
        }

        App::DocumentObject* target = sub.getSubObject();
        if (!target) {
            target = resolvedObject;
        }
        if (!target) {
            target = root->getSubObject(subNoElement.c_str());
        }

        if (!target) {
            return App::GeoFeature::getGlobalPlacement(root);
        }

        return App::GeoFeature::getGlobalPlacement(target, root, subNoElement);
    };

    auto addObject = [&](App::DocumentObject* obj, const char* elementName, const Base::Placement& placement, std::unordered_set<std::string>& objectKeys) {
        if (!obj) {
            return false;
        }

        App::DocumentObject* object = nullptr;
        Part::ShapeOptions options = Part::ShapeOption::ResolveLink;

        if (elementName && elementName[0]) {
            options |= Part::ShapeOption::NeedSubElement;
        }

        TopoDS_Shape shape = Part::Feature::getShape(obj, options, elementName, nullptr, &object);

        if (shape.IsNull()) {
            return false;
        }

        TopAbs_ShapeEnum shapeType = shape.ShapeType();
        if (shapeType != TopAbs_SOLID && shapeType != TopAbs_COMPSOLID && 
            shapeType != TopAbs_SHELL && shapeType != TopAbs_FACE && shapeType != TopAbs_COMPOUND) {
            return false;
        }

        App::DocumentObject* materialObj = object ? object : obj;

        std::ostringstream key;
        key << std::fixed << std::setprecision(9) << materialObj->getDocument()->getName() << '|' << materialObj->getNameInDocument() << '|';

        Base::Matrix4D matrix = placement.toMatrix();

        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                key << matrix[r][c] << ';';
            }
        }
        std::string objectKey = key.str();
        if (!objectKeys.insert(objectKey).second) {
            return true;
        }

        objectsToMeasure.push_back({materialObj, shape, placement});
        return true;
    };

    std::unordered_set<std::string> objectKeys;
    std::unordered_set<App::DocumentObject*> visited;

    auto collectBodies = [&](auto&& self, App::DocumentObject* obj, const Base::Placement& parentPlacement) -> void {
        if (!obj) {
            return;
        }

        App::DocumentObject* resolved;
        if (obj->isLink()) {
            resolved = obj->getLinkedObject(true);
        }
        else {
            resolved = obj;
        }

        if (!resolved) {
            return;
        }
        if (!visited.insert(obj).second) {
            return;
        }

        Base::Placement currentPlacement = parentPlacement * getPlacementFromObject(obj);
        if (resolved != obj) {
            currentPlacement = currentPlacement * getPlacementFromObject(resolved);
        }

        if (auto* body = dynamic_cast<PartDesign::Body*>(resolved)) {
            if (auto* tip = body->Tip.getValue()) {
                Base::Placement tipPlacement = currentPlacement * getPlacementFromObject(tip);
                addObject(tip, nullptr, tipPlacement, objectKeys);
            }
            return;
        }

        if (auto* group = resolved->getExtensionByType<App::GroupExtension>(true)) {
            for (auto* child : group->getObjects()) {
                self(self, child, currentPlacement);
            }
            return;
        }

        if (addObject(resolved, nullptr, currentPlacement, objectKeys)) {
            return;
        }
    };

    hasCurrentDatumPlacement = false;

    if (selectingCustomCoordSystem) {

        for (const auto& selObj : guiSelection) {
            App::DocumentObject* coordSystem = selObj.pObject;

            if (selObj.pResolvedObject && selObj.pResolvedObject != selObj.pObject) {
                coordSystem = selObj.pResolvedObject;
            }

            if (selObj.SubName && selObj.SubName[0]) {
                App::SubObjectT sub(selObj.pObject, selObj.SubName);
                
                if (auto* leaf = sub.getSubObject()) {
                    coordSystem = leaf;
                }
            }

            if (isReferenceObject(coordSystem)) {
                panel->ui.customEdit->setText(QString::fromStdString(coordLabel(coordSystem)));
                currentDatum = coordSystem;
                currentDatumPlacement = getGlobalPlacement(selObj.pObject, selObj.SubName, selObj.pResolvedObject);
                hasCurrentDatumPlacement = true;
                selectingCustomCoordSystem = false;

                isUpdating = true;
                Gui::Selection().clearSelection();
                for (const auto& sel : savedSelection) {
                    if (std::get<2>(sel).empty()) {
                        Gui::Selection().addSelection(std::get<0>(sel).c_str(), std::get<1>(sel).c_str());
                    } else {
                        Gui::Selection().addSelection(std::get<0>(sel).c_str(), std::get<1>(sel).c_str(), std::get<2>(sel).c_str());
                    }
                }
                savedSelection.clear();
                isUpdating = false;
                tryupdate();
                return;
            }
        }
    }
    
    for (const auto& selObj : guiSelection) {
        if (!selObj.pObject) {
            continue;
        }

        App::DocumentObject* displayObject = selObj.pObject;
        if (selObj.pResolvedObject && selObj.pResolvedObject != selObj.pObject) {
            displayObject = selObj.pResolvedObject;
        }

        if (selObj.SubName && selObj.SubName[0]) {
            App::SubObjectT sub(selObj.pObject, selObj.SubName);
            if (auto* leaf = sub.getSubObject()) {
                displayObject = leaf;
            }
        }

        bool shouldAddToList = false;
        if (!isReferenceObject(displayObject)) {
            if (displayObject->isDerivedFrom(Base::Type::fromName("Sketcher::SketchObject"))) {
                continue;
            }

            Gui::Document* guiDoc = Gui::Application::Instance->getDocument(displayObject->getDocument());
            if (!guiDoc) {
                continue;
            }

            auto* viewProvider = dynamic_cast<Gui::ViewProviderDocumentObject*>(guiDoc->getViewProvider(displayObject));
            if (!viewProvider || !viewProvider->Visibility.getValue()) {
                continue;
            }

            shouldAddToList = true;
        }

        App::DocumentObject* coordSystem = selObj.pObject;
        if (selObj.pResolvedObject && selObj.pResolvedObject != selObj.pObject) {
            coordSystem = selObj.pResolvedObject;
        }

        if (selObj.SubName && selObj.SubName[0]) {
            App::SubObjectT sub(selObj.pObject, selObj.SubName);
            if (auto* leaf = sub.getSubObject()) {
                coordSystem = leaf;
            }
        }

        if (isReferenceObject(coordSystem)) {
            if (currentMode == "Custom" && !selectingCustomCoordSystem) {
                currentDatum = coordSystem;
                currentDatumPlacement = getGlobalPlacement(selObj.pObject, selObj.SubName, selObj.pResolvedObject);
                hasCurrentDatumPlacement = true;
                panel->ui.customEdit->setText(QString::fromStdString(coordLabel(coordSystem)));
                referenceDatum = currentDatum;
                break;
            }
            continue;
        }

        App::DocumentObject* leaf = nullptr;
        if (selObj.SubName && selObj.SubName[0]) {
            App::SubObjectT sub(selObj.pObject, selObj.SubName);
            if (selObj.pResolvedObject && selObj.pResolvedObject != selObj.pObject) {
                leaf = selObj.pResolvedObject;
            }
            if (!leaf) {
                leaf = sub.getSubObject();
            }
        }
        if (!leaf) {
            leaf = selObj.pObject;
        }

        Base::Placement rootPlacement = getGlobalPlacement(selObj.pObject, selObj.SubName, selObj.pResolvedObject);
        Base::Placement parentPlacement = rootPlacement * getPlacementFromObject(leaf).inverse();
        visited.clear();
        size_t objectsBefore = objectsToMeasure.size();
        collectBodies(collectBodies, leaf, parentPlacement);
        
        if (shouldAddToList && objectsToMeasure.size() > objectsBefore) {
            auto* item = new QListWidgetItem(QString::fromStdString(displayObject->getFullLabel()));
            QString docName;
            if (auto* doc = selObj.pObject->getDocument()) {
                docName = QString::fromLatin1(doc->getName());
            }
            QString objName = QString::fromLatin1(selObj.pObject->getNameInDocument());
            QString subName = selObj.SubName ? QString::fromLatin1(selObj.SubName) : QString();
            item->setData(Qt::UserRole, docName + QLatin1String("|") + objName + QLatin1String("|") + subName);
            panel->ui.objectList->addItem(item);
        }
    }

    if (currentMode == "Custom") {
        referenceDatum = currentDatum;
    }
    else {
        panel->ui.customEdit->clear();
    }

    if (currentMode == "Custom" && !referenceDatum) {
        this->clearUiFields();
        this->removeTemporaryObjects();
        return;
    }

    if (panel->ui.objectList->count() == 0) {
        this->clearUiFields();
        this->removeTemporaryObjects();
        return;
    }

    updateInertiaVisibility();
    
    MassPropertiesData info = CalculateMassProperties(
        objectsToMeasure,
        currentMode,
        referenceDatum,
        hasCurrentDatumPlacement ? &currentDatumPlacement : nullptr
    );

    if (info.volume == 0.0 && info.mass == 0.0 && info.surfaceArea == 0.0) {
        this->clearUiFields();
        this->removeTemporaryObjects();
        objectsToMeasure.clear();
        panel->ui.objectList->clear();
        return;
    }

    currentInfo = info;

    if (currentMode == "Custom" && referenceDatum) {
        auto applyOriginOffset = [&](const Base::Vector3d& originPos) {
            info.cogX -= originPos.x;
            info.cogY -= originPos.y;
            info.cogZ -= originPos.z;

            info.covX -= originPos.x;
            info.covY -= originPos.y;
            info.covZ -= originPos.z;
        };


        if (!referenceDatum->isDerivedFrom<App::Line>()) {
            if (hasCurrentDatumPlacement) {
                applyOriginOffset(currentDatumPlacement.getPosition());
            }
            else if (auto datum = dynamic_cast<const App::DatumElement*>(referenceDatum)) {
                if (datum->getLCS()) {
                    applyOriginOffset(datum->getBasePoint());
                }
            }
            else if (auto lcs = dynamic_cast<const App::LocalCoordinateSystem*>(referenceDatum)) {
                applyOriginOffset(lcs->Placement.getValue().getPosition());
            }
            else if (auto origin = dynamic_cast<const App::Origin*>(referenceDatum)) {
                applyOriginOffset(origin->Placement.getValue().getPosition());
            }
        }
    }

    const int decimals = Base::UnitsApi::getDecimals();
    const int denominator = Base::UnitsApi::getDenominator();

    auto setText = [&](QLineEdit* edit, double value, const Base::Unit& unit, const QString& suffix = QString()) {
        if (value < Base::Precision::Confusion() && value > -Base::Precision::Confusion()) {
            value = 0.0;
        }
        Base::Quantity q {value, unit};
        Base::QuantityFormat format(Base::QuantityFormat::Fixed, decimals);
        format.setDenominator(denominator);
        q.setFormat(format);

        std::string text;
        auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(unitsSchemaIndex));
        if (schema) {
            text = schema->translate(q);
        }
        else {
            text = Base::UnitsApi::schemaTranslate(q);
        }
        edit->setText(QString::fromUtf8(text.c_str()) + suffix);
        edit->setCursorPosition(0);
    };

    const QString densitySuffix = objectsToMeasure.size() > 1 ? tr(" (Avg)") : QString();

    setText(panel->ui.volumeEdit, info.volume, Base::Unit::Volume);
    setText(panel->ui.massEdit, info.mass, Base::Unit::Mass);
    setText(panel->ui.surfaceAreaEdit, info.surfaceArea, Base::Unit::Area);
    setText(panel->ui.densityEdit, info.density, Base::Unit::Density, densitySuffix);

    setText(panel->ui.cogXText, info.cogX, Base::Unit::Length);
    setText(panel->ui.cogYText, info.cogY, Base::Unit::Length);
    setText(panel->ui.cogZText, info.cogZ, Base::Unit::Length);
    setText(panel->ui.covXText, info.covX, Base::Unit::Length);
    setText(panel->ui.covYText, info.covY, Base::Unit::Length);
    setText(panel->ui.covZText, info.covZ, Base::Unit::Length);

    setText(panel->ui.inertiaJoxText, info.inertiaJox, Base::Unit::Inertia);
    setText(panel->ui.inertiaJoyText, info.inertiaJoy, Base::Unit::Inertia);
    setText(panel->ui.inertiaJozText, info.inertiaJoz, Base::Unit::Inertia);
    setText(panel->ui.inertiaJxyText, info.inertiaJxy, Base::Unit::Inertia);
    setText(panel->ui.inertiaJzxText, info.inertiaJzx, Base::Unit::Inertia);
    setText(panel->ui.inertiaJzyText, info.inertiaJzy, Base::Unit::Inertia);

    setText(panel->ui.inertiaJxText, info.inertiaJx, Base::Unit::Inertia);
    setText(panel->ui.inertiaJyText, info.inertiaJy, Base::Unit::Inertia);
    setText(panel->ui.inertiaJzText, info.inertiaJz, Base::Unit::Inertia);
    setText(panel->ui.axisInertiaText, info.axisInertia, Base::Unit::Inertia);

    const auto infoSnapshot = currentInfo;
    QTimer::singleShot(0, this, [this, infoSnapshot]() {
        currentInfo = infoSnapshot;
        createDatum(currentInfo.cogX, currentInfo.cogY, currentInfo.cogZ, "Center_of_Gravity");
        createDatum(currentInfo.covX, currentInfo.covY, currentInfo.covZ, "Center_of_Volume");
        createLCS("Principal_Axes_LCS");
    });
}

void TaskMassProperties::updateInertiaVisibility()
{
    const bool hasAxisSelection = currentMode == "Custom" && currentDatum && currentDatum->isDerivedFrom<App::Line>();

    panel->ui.inertiaMatrixWidget->setVisible(!hasAxisSelection);
    panel->ui.inertiaDiagWidget->setVisible(!hasAxisSelection);
    panel->ui.inertiaLcsWidget->setVisible(!hasAxisSelection);
    panel->ui.axisInertiaWidget->setVisible(hasAxisSelection);
    panel->ui.inertiaSeparator->setVisible(!hasAxisSelection);
    panel->ui.inertiaDiagSpacer1->changeSize(hasAxisSelection ? 0 : 8, 20);
    panel->ui.inertiaDiagSpacer2->changeSize(hasAxisSelection ? 0 : 8, 20);
    panel->ui.inertiaDiagLayout->invalidate();
    panel->ui.inertiaMatrixLabel->setVisible(!hasAxisSelection);
    panel->ui.inertiaPrincipalLabel->setVisible(!hasAxisSelection);
}

void TaskMassProperties::createDatum(double x, double y, double z, const std::string& name, bool removeExisting)
{
    if (isUpdating && removeExisting) return;

    try {
        App::Document* doc = App::GetApplication().getActiveDocument();
        doc->openTransaction("Create Datum Point");
        
        App::DocumentObject* datum = doc->getObject(name.c_str());
        
        if (removeExisting && datum) {
            doc->removeObject(name.c_str());
        }

        datum = doc->addObject("Part::DatumPoint", name.c_str());
        
        App::Property* baseProp = datum->getPropertyByName("Placement");
        App::PropertyPlacement* prop = dynamic_cast<App::PropertyPlacement*>(baseProp);
        Base::Placement plm;
        plm.setPosition(Base::Vector3d(x, y, z));
        prop->setValue(plm);

        doc->commitTransaction();
        doc->recompute();
    } 
    catch (const Base::Exception& e) {
        Base::Console().error("Datum Creation failed: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().error("Datum Creation failed: %s", e.what());
    }
}

void TaskMassProperties::createLCS(std::string name, bool removeExisting)
{
    if (isUpdating && removeExisting) return;

    try {
        App::Document* doc = App::GetApplication().getActiveDocument();
        doc->openTransaction("Create LCS");

        App::DocumentObject* LCS = doc->getObject(name.c_str());

        if (removeExisting && LCS) {
            doc->removeObject(name.c_str());
        }
        LCS = doc->addObject("Part::LocalCoordinateSystem", name.c_str());
        
        App::Property* baseProp = LCS->getPropertyByName("Placement");
        App::PropertyPlacement* prop = dynamic_cast<App::PropertyPlacement*>(baseProp);
        Base::Placement plm;
        plm.setPosition(Base::Vector3d(
            currentInfo.cogX,
            currentInfo.cogY,
            currentInfo.cogZ
        ));

        Base::Matrix4D mat;
        mat.setToUnity();
        
        mat[0][0] = currentInfo.principalAxisX.x;
        mat[1][0] = currentInfo.principalAxisX.y;
        mat[2][0] = currentInfo.principalAxisX.z;
        
        mat[0][1] = currentInfo.principalAxisY.x;
        mat[1][1] = currentInfo.principalAxisY.y;
        mat[2][1] = currentInfo.principalAxisY.z;
        
        mat[0][2] = currentInfo.principalAxisZ.x;
        mat[1][2] = currentInfo.principalAxisZ.y;
        mat[2][2] = currentInfo.principalAxisZ.z;

        plm.setRotation(mat);

        prop->setValue(plm);

        LCS->Visibility.setValue(true);
        if (auto* lcsObj = dynamic_cast<App::LocalCoordinateSystem*>(LCS)) {
            for (auto* plane : lcsObj->planes()) {
                if (plane) {
                    plane->Visibility.setValue(false);
                }
            }
        }

        doc->commitTransaction();
        doc->recompute();
    } 
    catch (const Base::Exception& e) {
        Base::Console().error("LCS Creation failed: %s\n", e.what());
    }
    catch (const std::exception& e) {
        Base::Console().error("LCS Creation failed: %s", e.what());
    }
}

void TaskMassProperties::onCogDatumButtonPressed()
{
    createDatum(currentInfo.cogX, currentInfo.cogY, currentInfo.cogZ, "Center_of_Gravity", false);
}

void TaskMassProperties::onCovDatumButtonPressed()
{
    createDatum(currentInfo.covX, currentInfo.covY, currentInfo.covZ, "Center_of_Volume", false);
}

void TaskMassProperties::onLcsButtonPressed()
{
    createLCS("Principal_Axes_LCS", false);
}

void TaskMassProperties::onSelectCustomCoordinateSystem()
{
    selectingCustomCoordSystem = true;
    savedSelection.clear();

    auto guiSelection = Gui::Selection().getSelection(nullptr, Gui::ResolveMode::NoResolve);
    for (const auto& sel : guiSelection) {
        if (!sel.pObject || !sel.pObject->getDocument()) continue;
        std::string docName = sel.pObject->getDocument()->getName();
        std::string objName = sel.pObject->getNameInDocument();
        std::string subName = (sel.SubName && sel.SubName[0]) ? sel.SubName : "";
        savedSelection.emplace_back(docName, objName, subName);
    }
}

void TaskMassProperties::onCoordinateSystemChanged(std::string coordSystem)
{
    currentMode = coordSystem;
    if (currentMode != "Custom") {
        selectingCustomCoordSystem = false;
        currentDatum = nullptr; 
        hasCurrentDatumPlacement = false;
        panel->ui.customEdit->clear();
    }
    if (Gui::Selection().getSelection().empty()) {
        clearUiFields();
        panel->ui.objectList->clear();
        removeTemporaryObjects();
        return;
    }

    updateInertiaVisibility();
    tryupdate();
}

void TaskMassProperties::saveResult()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    
    if (!doc || panel->ui.objectList->count() == 0 || (currentMode == "Custom" && !currentDatum)) {
        return;
    }

    doc->openTransaction("Add Mass Properties");

    MassProperties::Result::init();
    
    auto group = dynamic_cast<App::DocumentObjectGroup*>(doc->getObject("Measurements"));
    
    if (!group || !group->isValid()) {
        group = doc->addObject<App::DocumentObjectGroup>("Measurements");
    }

    auto* obj = doc->addObject("MassProperties::Result", "MassProperties");
    if (!obj) {
        doc->abortTransaction();
        return;
    }

    obj->Visibility.setValue(true);

    auto setQuantity = [&](const char* name, double value, const Base::Unit& unit) {
        if (value < Base::Precision::Confusion() && value > -Base::Precision::Confusion()) {
            value = 0.0;
        }
        auto* prop = dynamic_cast<App::PropertyString*>(
            obj->addDynamicProperty("App::PropertyString", name, "MassProperties")
        );
        if (prop) {
            Base::Quantity q(value, unit);

            std::string text;
            auto schema = Base::UnitsApi::createSchema(static_cast<std::size_t>(unitsSchemaIndex));
            if (schema) {
                text = schema->translate(q);
            }
            else {
                text = Base::UnitsApi::schemaTranslate(q);
            }

            prop->setValue(text.c_str());
            prop->setReadOnly(true);
        }
    };

    auto setVector = [&](const char* name, Base::Vector3d& value) {
        for (int i = 0; i < 3; ++i) {
            if (value[i] < Base::Precision::Confusion() && value[i] > -Base::Precision::Confusion()) {
                value[i] = 0.0;
            }
        }
        auto* prop = dynamic_cast<App::PropertyVector*>(
            obj->addDynamicProperty("App::PropertyVector", name, "MassProperties")
        );
        if (prop) {
            prop->setValue(value);
            prop->setReadOnly(true);
        }
    };

    auto setString = [&](const char* name, const std::string& value) {
        auto* prop = dynamic_cast<App::PropertyString*>(
            obj->addDynamicProperty("App::PropertyString", name, "MassProperties")
        );
        if (prop) {
            prop->setValue(value);
            prop->setReadOnly(true);
        }
    };

    setString("Mode", currentMode);

    setQuantity("Volume", currentInfo.volume, Base::Unit::Volume);
    setQuantity("Mass", currentInfo.mass, Base::Unit::Mass);
    setQuantity("Density", currentInfo.density, Base::Unit::Density);
    setQuantity("SurfaceArea", currentInfo.surfaceArea, Base::Unit::Area);

    setQuantity("CenterOfGravityX", currentInfo.cogX, Base::Unit::Length);
    setQuantity("CenterOfGravityY", currentInfo.cogY, Base::Unit::Length);
    setQuantity("CenterOfGravityZ", currentInfo.cogZ, Base::Unit::Length);
    setQuantity("CenterOfVolumeX", currentInfo.covX, Base::Unit::Length);
    setQuantity("CenterOfVolumeY", currentInfo.covY, Base::Unit::Length);
    setQuantity("CenterOfVolumeZ", currentInfo.covZ, Base::Unit::Length);

    if (currentInfo.axisInertia != 0.0) {
        setQuantity("AxisInertia", currentInfo.axisInertia, Base::Unit::Inertia);
    }
    else {
        setQuantity("InertiaJox", currentInfo.inertiaJox, Base::Unit::Inertia);
        setQuantity("InertiaJoy", currentInfo.inertiaJoy, Base::Unit::Inertia);
        setQuantity("InertiaJoz", currentInfo.inertiaJoz, Base::Unit::Inertia);
        setQuantity("InertiaJxy", currentInfo.inertiaJxy, Base::Unit::Inertia);
        setQuantity("InertiaJzx", currentInfo.inertiaJzx, Base::Unit::Inertia);
        setQuantity("InertiaJzy", currentInfo.inertiaJzy, Base::Unit::Inertia);
        setQuantity("InertiaJx", currentInfo.inertiaJx, Base::Unit::Inertia);
        setQuantity("InertiaJy", currentInfo.inertiaJy, Base::Unit::Inertia);
        setQuantity("InertiaJz", currentInfo.inertiaJz, Base::Unit::Inertia);
    }
    
    

    setVector("PrincipalAxisX", currentInfo.principalAxisX);
    setVector("PrincipalAxisY", currentInfo.principalAxisY);
    setVector("PrincipalAxisZ", currentInfo.principalAxisZ);

    if (group) {
        group->addObject(obj);
        group->purgeTouched();
    }

    if (auto* guiDoc = Gui::Application::Instance->activeDocument()) {
        if (auto* view = dynamic_cast<Gui::ViewProviderDocumentObject*>(
                guiDoc->getViewProvider(obj))) {
            view->setShowable(true);
            view->show();
        }
    }

    doc->commitTransaction();
}