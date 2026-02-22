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
#include "UnitHelper.h"
#include "Mod/MassProperties/App/MassPropertiesResult.h"
#include "Mod/MassProperties/App/MassPropertiesObject.h"

#include <QtCore/QScopedValueRollback>
#include <QKeyEvent>
#include <QTimer>

#include <QtWidgets>
#include <unordered_set>
#include <sstream>
#include <iomanip>

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
#include <Base/Placement.h>
#include <Base/Precision.h>
#include <Base/Quantity.h>
#include <Base/Rotation.h>
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

MassPropertiesData currentInfo;
std::string currentMode = "Center of gravity";
App::DocumentObject* currentDatum = nullptr;

TaskMassProperties::TaskMassProperties()
    : Gui::SelectionObserver(true)
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

    auto physicalProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("PropertiesIcon"),
        tr("Physical Properties"),
        true,
        nullptr
    );
    
    auto shortcutQuit = new QShortcut(physicalProperties);
    shortcutQuit->setKey(QKeySequence(QStringLiteral("ESC")));
    shortcutQuit->setContext(Qt::ApplicationShortcut);
    connect(shortcutQuit, &QShortcut::activated, this, &TaskMassProperties::escape);

    QBoxLayout* physicalLayout = physicalProperties->groupLayout();
    physicalLayout->setContentsMargins(10, 10, 10, 10);
    physicalLayout->setSpacing(8);

    QLabel* objectsLabel = new QLabel(tr("Objects to measure"));
    objectsLabel->setWordWrap(true);
    physicalLayout->addWidget(objectsLabel);

    objectList = new QListWidget();
    objectList->setMaximumHeight(50);
    physicalLayout->addWidget(objectList);

    QLabel* referenceLabel = new QLabel(tr("Reference"));
    physicalLayout->addWidget(referenceLabel);
    
    QRadioButton *centerOfGravityRadioButton = new QRadioButton(tr("Center of gravity"));
    centerOfGravityRadioButton->setChecked(true);
    QObject::connect(centerOfGravityRadioButton, &QRadioButton::toggled, [=](bool checked){
        if (checked) {
            onCoordinateSystemChanged("Center of gravity");
        }
    });

    QRadioButton *customRadioButton = new QRadioButton(tr("Custom"));
    QObject::connect(customRadioButton, &QRadioButton::toggled, [=](bool checked){
        if (checked) {
            onCoordinateSystemChanged("Custom");
        }
    });
    QButtonGroup *coordinateSystemGroup = new QButtonGroup(physicalProperties);
    coordinateSystemGroup->addButton(centerOfGravityRadioButton);
    coordinateSystemGroup->addButton(customRadioButton);

    customEdit = new QLineEdit();
    customEdit->setReadOnly(true);
    QPushButton* selectButton = new QPushButton(tr("Select…"));
    QObject::connect(selectButton, &QPushButton::pressed, this, &TaskMassProperties::onSelectCustomCoordinateSystem);
    selectButton->setEnabled(true);

    QHBoxLayout* customControls = new QHBoxLayout();
    customControls->setContentsMargins(0, 0, 0, 0);
    customControls->setSpacing(6);
    customControls->addWidget(customRadioButton);
    customControls->addWidget(customEdit, 1);
    customControls->addWidget(selectButton);

    physicalLayout->addWidget(centerOfGravityRadioButton);
    physicalLayout->addLayout(customControls);

    unitsComboBox = new QComboBox(physicalProperties);
    unitsComboBox->addItem(QString::fromUtf8("mm, kg, kg·mm²"));
    unitsComboBox->addItem(QString::fromUtf8("m, kg, kg·m²"));
    unitsComboBox->addItem(QString::fromUtf8("in, lb, lb·in²"));
    unitsComboBox->addItem(QString::fromUtf8("ft, lb, lb·ft²"));

    physicalLayout->addWidget(unitsComboBox);

    const int preferredSchemaIndex = UnitHelper::getPreferred();
    
    unitsComboBox->setCurrentIndex(UnitHelper::getComboIndex(preferredSchemaIndex));
    unitsSchemaIndex = UnitHelper::getSchemaIndex(unitsComboBox->currentIndex(), preferredSchemaIndex);

    QObject::connect(unitsComboBox, &QComboBox::currentIndexChanged, [=](int index) {
        unitsSchemaIndex = UnitHelper::getSchemaIndex(index, preferredSchemaIndex);
        update(Gui::SelectionChanges());
    });

    Content.emplace_back(physicalProperties);

    auto basicProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("PropertiesIcon"),
        tr("Basic Properties"),
        true,
        nullptr
    );

    QBoxLayout* basicLayout = basicProperties->groupLayout();
    basicLayout->setContentsMargins(10, 10, 10, 10);
    basicLayout->setSpacing(8);

    QLabel* volumeLabel = new QLabel(tr("Volume"));
    volumeEdit = new QLineEdit();
    volumeEdit->setReadOnly(true);
    QLabel* massLabel = new QLabel(tr("Mass"));
    massEdit = new QLineEdit();
    massEdit->setReadOnly(true);
    QLabel* densityLabel = new QLabel(tr("Density"));
    densityEdit = new QLineEdit();
    densityEdit->setReadOnly(true);
    QLabel* surfaceAreaLabel = new QLabel(tr("Surface area"));
    surfaceAreaEdit = new QLineEdit();
    surfaceAreaEdit->setReadOnly(true);

    QFormLayout* basicForm = new QFormLayout();
    basicForm->setContentsMargins(0, 0, 0, 0);
    basicForm->setSpacing(6);
    basicForm->addRow(volumeLabel, volumeEdit);
    basicForm->addRow(massLabel, massEdit);
    basicForm->addRow(densityLabel, densityEdit);
    basicForm->addRow(surfaceAreaLabel, surfaceAreaEdit);

    basicLayout->addLayout(basicForm);

    Content.emplace_back(basicProperties);
    
    auto centerOfGravity = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Std_Point"),
        tr("Center of Gravity"),
        true,
        nullptr
    );

    QBoxLayout* cogLayout = centerOfGravity->groupLayout();
    cogLayout->setContentsMargins(10, 10, 10, 10);
    cogLayout->setSpacing(8);

    QHBoxLayout* cogFields = new QHBoxLayout();
    cogFields->setContentsMargins(0, 0, 0, 0);

    QLabel* cogXLabel = new QLabel(QStringLiteral("X"));
    cogXText = new QLineEdit();
    cogXText->setReadOnly(true);
    QLabel* cogYLabel = new QLabel(QStringLiteral("Y"));
    cogYText = new QLineEdit();
    cogYText->setReadOnly(true);
    QLabel* cogZLabel = new QLabel(QStringLiteral("Z"));
    cogZText = new QLineEdit();
    cogZText->setReadOnly(true);

    cogFields->addWidget(cogXLabel);
    cogFields->addWidget(cogXText);
    cogFields->addSpacing(8);
    cogFields->addWidget(cogYLabel);
    cogFields->addWidget(cogYText);
    cogFields->addSpacing(8);
    cogFields->addWidget(cogZLabel);
    cogFields->addWidget(cogZText);

    cogLayout->addLayout(cogFields);

    QPushButton* cogDatumButton = new QPushButton(tr("Create Datum Point"));
    QObject::connect(cogDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCogDatumButtonPressed);
    QHBoxLayout* cogButtonLayout = new QHBoxLayout();
    cogButtonLayout->addStretch();
    cogButtonLayout->addWidget(cogDatumButton);
    cogButtonLayout->addStretch();
    cogLayout->addLayout(cogButtonLayout);
    Content.emplace_back(centerOfGravity);

    auto centerOfVolume = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Std_Point"),
        tr("Center of Volume"),
        true,
        nullptr
    );

    QBoxLayout* covLayout = centerOfVolume->groupLayout();
    covLayout->setContentsMargins(10, 10, 10, 10);
    covLayout->setSpacing(8);

    QHBoxLayout* covFields = new QHBoxLayout();
    covFields->setContentsMargins(0, 0, 0, 0);

    QLabel* covXLabel = new QLabel(QStringLiteral("X"));
    covXText = new QLineEdit();
    covXText->setReadOnly(true);
    QLabel* covYLabel = new QLabel(QStringLiteral("Y"));
    covYText = new QLineEdit();
    covYText->setReadOnly(true);
    QLabel* covZLabel = new QLabel(QStringLiteral("Z"));
    covZText = new QLineEdit();
    covZText->setReadOnly(true);

    covFields->addWidget(covXLabel);
    covFields->addWidget(covXText);
    covFields->addSpacing(8);
    covFields->addWidget(covYLabel);
    covFields->addWidget(covYText);
    covFields->addSpacing(8);
    covFields->addWidget(covZLabel);
    covFields->addWidget(covZText);

    covLayout->addLayout(covFields);

    QPushButton* covDatumButton = new QPushButton(tr("Create Datum Point"));
    QObject::connect(covDatumButton, &QPushButton::pressed, this, &TaskMassProperties::onCovDatumButtonPressed);
    QHBoxLayout* covButtonLayout = new QHBoxLayout();
    covButtonLayout->addStretch();
    covButtonLayout->addWidget(covDatumButton);
    covButtonLayout->addStretch();
    covLayout->addLayout(covButtonLayout);

    Content.emplace_back(centerOfVolume);

    auto inertiaProperties = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Std_CoordinateSystem"),
        tr("Inertia"),
        true,
        nullptr
    );

    QBoxLayout* inertiaLayout = inertiaProperties->groupLayout();
    inertiaLayout->setContentsMargins(10, 10, 10, 10);
    inertiaLayout->setSpacing(8);

    inertiaMatrixWidget = new QWidget();
    QLabel* inertiaMatrixLabel = new QLabel(tr("Inertia Matrix"));
    inertiaLayout->addWidget(inertiaMatrixLabel);
    QGridLayout* inertiaGrid = new QGridLayout(inertiaMatrixWidget);
    inertiaGrid->setContentsMargins(0, 0, 0, 0);
    inertiaGrid->setHorizontalSpacing(6);
    inertiaGrid->setVerticalSpacing(6);

    QLabel* inertiaJoxLabel = new QLabel(QStringLiteral("Jox"));
    inertiaJoxText = new QLineEdit(); 
    inertiaJoxText->setReadOnly(true);
    QLabel* inertiaJxyLabel = new QLabel(QStringLiteral("Jxy"));
    inertiaJxyText = new QLineEdit(); 
    inertiaJxyText->setReadOnly(true);
    QLabel* inertiaJzxLabel = new QLabel(QStringLiteral("Jzx"));
    inertiaJzxText = new QLineEdit(); 
    inertiaJzxText->setReadOnly(true);

    QLabel* inertiaJoyLabel = new QLabel(QStringLiteral("Joy"));
    inertiaJoyText = new QLineEdit(); 
    inertiaJoyText->setReadOnly(true);
    QLabel* inertiaJzyLabel = new QLabel(QStringLiteral("Jzy"));
    inertiaJzyText = new QLineEdit(); 
    inertiaJzyText->setReadOnly(true);

    QLabel* inertiaJozLabel = new QLabel(QStringLiteral("Joz"));
    inertiaJozText = new QLineEdit(); 
    inertiaJozText->setReadOnly(true);

    inertiaGrid->addWidget(inertiaJoxLabel, 0, 0);
    inertiaGrid->addWidget(inertiaJoxText, 0, 1);
    inertiaGrid->addWidget(inertiaJxyLabel, 0, 2);
    inertiaGrid->addWidget(inertiaJxyText, 0, 3);
    inertiaGrid->addWidget(inertiaJzxLabel, 0, 4);
    inertiaGrid->addWidget(inertiaJzxText, 0, 5);

    inertiaGrid->addWidget(inertiaJoyLabel, 1, 2);
    inertiaGrid->addWidget(inertiaJoyText, 1, 3);
    inertiaGrid->addWidget(inertiaJzyLabel, 1, 4);
    inertiaGrid->addWidget(inertiaJzyText, 1, 5);

    inertiaGrid->addWidget(inertiaJozLabel, 2, 4);
    inertiaGrid->addWidget(inertiaJozText, 2, 5);

    inertiaLayout->addWidget(inertiaMatrixWidget);

    QFrame* inertiaSeparator = new QFrame(inertiaProperties);
    inertiaSeparator->setFrameShape(QFrame::HLine);
    inertiaSeparator->setFrameShadow(QFrame::Sunken);
    inertiaLayout->addWidget(inertiaSeparator);

    QLabel* inertiaPrincipalLabel = new QLabel(tr("Principal Moments of Inertia"));
    inertiaLayout->addWidget(inertiaPrincipalLabel);
    
    inertiaDiagWidget = new QWidget();
    QHBoxLayout* inertiaDiag = new QHBoxLayout(inertiaDiagWidget);
    inertiaDiag->setContentsMargins(0, 0, 0, 0);
    inertiaDiag->setSpacing(6);
    QLabel* inertiaJxLabel = new QLabel(QStringLiteral("Jx"));
    inertiaJxText = new QLineEdit();
    inertiaJxText->setReadOnly(true);
    QLabel* inertiaJyLabel = new QLabel(QStringLiteral("Jy"));
    inertiaJyText = new QLineEdit(); 
    inertiaJyText->setReadOnly(true);
    QLabel* inertiaJzLabel = new QLabel(QStringLiteral("Jz"));
    inertiaJzText = new QLineEdit(); 
    inertiaJzText->setReadOnly(true);

    inertiaDiag->addWidget(inertiaJxLabel);
    inertiaDiag->addWidget(inertiaJxText);
    inertiaDiag->addSpacing(8);
    inertiaDiag->addWidget(inertiaJyLabel);
    inertiaDiag->addWidget(inertiaJyText);
    inertiaDiag->addSpacing(8);
    inertiaDiag->addWidget(inertiaJzLabel);
    inertiaDiag->addWidget(inertiaJzText);

    inertiaLayout->addWidget(inertiaDiagWidget);

    inertiaLcsWidget = new QWidget();
    QPushButton* inertiaLCSButton = new QPushButton(tr("Create Principal Axes LCS"));
    QObject::connect(inertiaLCSButton, &QPushButton::pressed, this, &TaskMassProperties::onLcsButtonPressed);
    QHBoxLayout* inertiaLCSButtonLayout = new QHBoxLayout(inertiaLcsWidget);
    inertiaLCSButtonLayout->addStretch();
    inertiaLCSButtonLayout->addWidget(inertiaLCSButton);
    inertiaLCSButtonLayout->addStretch();
    inertiaLayout->addWidget(inertiaLcsWidget);

    axisInertiaWidget = new QWidget();
    QHBoxLayout* axisDiag = new QHBoxLayout(axisInertiaWidget);

    QLabel* axisLabel = new QLabel(tr("Inertia around axis"));
    axisLabel->setWordWrap(true);
    axisDiag->addWidget(axisLabel);

    axisInertiaText = new QLineEdit();
    axisInertiaText->setReadOnly(true);

    axisDiag->addWidget(axisInertiaText);

    inertiaLayout->addWidget(axisInertiaWidget);

    Content.emplace_back(inertiaProperties);

    updateInertiaVisibility();
    update(Gui::SelectionChanges());
}

TaskMassProperties::~TaskMassProperties()
{
    qApp->removeEventFilter(this);
    if (deleteAction) {
        deleteAction->setEnabled(deleteActivated);
    }
}

bool TaskMassProperties::eventFilter(QObject* watched, QEvent* event)
{
    Q_UNUSED(watched);

    if (event && (event->type() == QEvent::ShortcutOverride || event->type() == QEvent::KeyPress)) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent && keyEvent->key() == Qt::Key_Delete) {
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
        objectList->clear();
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
    objectList->clear();

    selectingCustomCoordSystem = false;
    currentDatum = nullptr;
    hasCurrentDatumPlacement = false;
    customEdit->clear();

    currentInfo = MassPropertiesData {};
}

void TaskMassProperties::removeTemporaryObjects()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    if (!doc) {
        return;
    }

    const bool hasObjectsToRemove = doc->getObject("Center_of_Gravity") 
        || doc->getObject("Center_of_Volume") 
        || doc->getObject("Principal_Axes_LCS");

    if (!hasObjectsToRemove) {
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
    volumeEdit->clear();
    massEdit->clear();
    densityEdit->clear();
    surfaceAreaEdit->clear();

    cogXText->clear();
    cogYText->clear();
    cogZText->clear();

    covXText->clear();
    covYText->clear();
    covZText->clear();

    inertiaJoxText->clear();
    inertiaJxyText->clear();
    inertiaJzxText->clear();
    inertiaJoyText->clear();
    inertiaJzyText->clear();
    inertiaJozText->clear();

    inertiaJxText->clear();
    inertiaJyText->clear();
    inertiaJzText->clear();

    axisInertiaText->clear();
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
        Base::Console().error("Mass Properties update failed: %s", e.what());
    }
}


void TaskMassProperties::tryupdate()
{
    if (isUpdating) return;

    QScopedValueRollback<bool> updatingGuard(isUpdating, true);

    auto guiSelection = Gui::Selection().getSelection(nullptr, Gui::ResolveMode::NoResolve);
    
    if (guiSelection.empty()) {
        if (currentMode == "Custom") {
            clearUiFields();
        }
        return;
    }

    if (!selectingCustomCoordSystem) {
        bool hasElementSelection = false;
        std::vector<std::pair<App::DocumentObject*, std::string>> selectedObjects;
        selectedObjects.reserve(guiSelection.size());

        for (const auto& sel : guiSelection) {
            if (!sel.pObject) {
                continue;
            }
            if (sel.SubName && sel.SubName[0]) {
                App::SubObjectT sub(sel.pObject, sel.SubName);
                std::string subNoElement = sub.getSubNameNoElement();

                bool canPickShape = !subNoElement.empty() || !sel.pResolvedObject || sel.pResolvedObject == sel.pObject;

                if (canPickShape && subNoElement != sel.SubName) {
                    hasElementSelection = true;
                }
                if (canPickShape) {
                    selectedObjects.emplace_back(sel.pObject, subNoElement);
                }
                else {
                    selectedObjects.emplace_back(sel.pObject, sel.SubName);
                }
            }
            else {
                selectedObjects.emplace_back(sel.pObject, std::string());
            }
        }

        if (hasElementSelection) {
            std::unordered_set<std::string> seen;
            isUpdating = true;
            Gui::Selection().clearSelection();

            for (const auto& selected : selectedObjects) {
                App::DocumentObject* obj = selected.first;
                if (!obj || !obj->getDocument()) {
                    continue;
                }

                const std::string& subName = selected.second;
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

    std::vector<MassPropertiesInput> objectsToMeasure;
    App::DocumentObject const* referenceDatum = nullptr;
    
    objectList->clear();
    
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
        App::DocumentObject* materialObj = object ? object : obj;

        std::ostringstream keyBuilder;
        keyBuilder << std::fixed << std::setprecision(9) << materialObj->getDocument()->getName() << '|' << materialObj->getNameInDocument() << '|';

        Base::Matrix4D matrix = placement.toMatrix();

        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                keyBuilder << matrix[r][c] << ';';
            }
        }
        std::string objectKey = keyBuilder.str();
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
                customEdit->setText(QString::fromStdString(coordLabel(coordSystem)));
                currentDatum = coordSystem;
                currentDatumPlacement = getGlobalPlacement(selObj.pObject, selObj.SubName, selObj.pResolvedObject);
                hasCurrentDatumPlacement = true;
                selectingCustomCoordSystem = false;
                break;
            }
        }
    }
    
    for (const auto& selObj : guiSelection) {
        if (!selObj.pObject) {
            continue;
        }
        
        if (!isReferenceObject(selObj.pObject)) {
            objectList->addItem(QString::fromStdString(selObj.pObject->getFullLabel()));
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
                customEdit->setText(QString::fromStdString(coordLabel(coordSystem)));
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
        collectBodies(collectBodies, leaf, parentPlacement);
    }

    if (currentMode == "Custom") {
        referenceDatum = currentDatum;
    }
    else {
        customEdit->clear();
    }

    if (currentMode == "Custom" && !referenceDatum) {
        this->clearUiFields();
        return;
    }

    updateInertiaVisibility();
    
    MassPropertiesData info = CalculateMassProperties(
        objectsToMeasure,
        currentMode,
        referenceDatum,
        hasCurrentDatumPlacement ? &currentDatumPlacement : nullptr
    );

    currentInfo = info;

    if (currentMode == "Center of gravity") {
        info.cogX = 0.0;
        info.cogY = 0.0;
        info.cogZ = 0.0;

        info.covX -= currentInfo.cogX;
        info.covY -= currentInfo.cogY;
        info.covZ -= currentInfo.cogZ;
    }
    if (currentMode == "Custom" && !referenceDatum) {
        info.cogX = 0.0;
        info.cogY = 0.0;
        info.cogZ = 0.0;

        info.covX = 0.0;
        info.covY = 0.0;
        info.covZ = 0.0;
    }
    else if (currentMode == "Custom" && referenceDatum) {
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

        const std::string text = UnitHelper::translate(q, unitsSchemaIndex);
        edit->setText(QString::fromUtf8(text.c_str()) + suffix);
        edit->setCursorPosition(0);
    };

    const QString densitySuffix = objectsToMeasure.size() > 1 ? tr(" (Avg)") : QString();

    setText(volumeEdit, info.volume, Base::Unit::Volume);
    setText(massEdit, info.mass, Base::Unit::Mass);
    setText(surfaceAreaEdit, info.surfaceArea, Base::Unit::Area);
    setText(densityEdit, info.density, Base::Unit::Density, densitySuffix);

    setText(cogXText, info.cogX, Base::Unit::Length);
    setText(cogYText, info.cogY, Base::Unit::Length);
    setText(cogZText, info.cogZ, Base::Unit::Length);
    setText(covXText, info.covX, Base::Unit::Length);
    setText(covYText, info.covY, Base::Unit::Length);
    setText(covZText, info.covZ, Base::Unit::Length);

    setText(inertiaJoxText, info.inertiaJox, Base::Unit::Inertia);
    setText(inertiaJoyText, info.inertiaJoy, Base::Unit::Inertia);
    setText(inertiaJozText, info.inertiaJoz, Base::Unit::Inertia);
    setText(inertiaJxyText, info.inertiaJxy, Base::Unit::Inertia);
    setText(inertiaJzxText, info.inertiaJzx, Base::Unit::Inertia);
    setText(inertiaJzyText, info.inertiaJzy, Base::Unit::Inertia);

    setText(inertiaJxText, info.inertiaJx, Base::Unit::Inertia);
    setText(inertiaJyText, info.inertiaJy, Base::Unit::Inertia);
    setText(inertiaJzText, info.inertiaJz, Base::Unit::Inertia);
    setText(axisInertiaText, info.axisInertia, Base::Unit::Inertia);

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

    inertiaMatrixWidget->setVisible(!hasAxisSelection);
    inertiaDiagWidget->setVisible(!hasAxisSelection);
    inertiaLcsWidget->setVisible(!hasAxisSelection);
    axisInertiaWidget->setVisible(hasAxisSelection);
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
}

void TaskMassProperties::onCoordinateSystemChanged(std::string coordSystem)
{
    currentMode = coordSystem;
    if (currentMode != "Custom") {
        selectingCustomCoordSystem = false;
        currentDatum = nullptr;
        hasCurrentDatumPlacement = false;
        customEdit->clear();
    }
    updateInertiaVisibility();
    tryupdate();
}

void TaskMassProperties::saveResult()
{
    App::Document* doc = App::GetApplication().getActiveDocument();
    
    if (!doc || objectList->count() == 0) {
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
        auto* prop = dynamic_cast<App::PropertyQuantity*>(
            obj->addDynamicProperty("App::PropertyQuantity", name, "MassProperties")
        );
        if (prop) {
            prop->setUnit(unit);
            prop->setValue(value);
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
        }
    };

    auto setString = [&](const char* name, const std::string& value) {
        auto* prop = dynamic_cast<App::PropertyString*>(
            obj->addDynamicProperty("App::PropertyString", name, "MassProperties")
        );
        if (prop) {
            prop->setValue(value);
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