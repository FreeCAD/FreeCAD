// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TaskRibParameters.h"
#include "ReferenceSelection.h"
#include "ViewProviderRib.h"
#include "ui_TaskRibParameters.h"
#include "ui_TaskRibAdvancedParameters.h"

#include <algorithm>
#include <QApplication>
#include <QComboBox>
#include <QEvent>
#include <QSignalBlocker>
#include <App/Document.h>
#include <App/ObjectIdentifier.h>
#include <Base/UnitsApi.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/PrefWidgets.h>
#include <Mod/Part/App/Part2DObject.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/FeatureRib.h>

namespace PartDesignGui
{
namespace
{
// Keep the picker limited to sketch-like objects or edges, excluding dependency cycles.
class RibProfileSelection: public NoDependentsSelection
{
public:
    explicit RibProfileSelection(const PartDesign::Rib* rib)
        : NoDependentsSelection(rib), document(rib->getDocument())
    {}

    bool allow(App::Document* doc, App::DocumentObject* object, const char* subname) override
    {
        if (doc != document || !object || !NoDependentsSelection::allow(doc, object, subname)) {
            return false;
        }
        const std::string name = subname ? subname : "";
        return name.empty() ? object->isDerivedFrom<Part::Part2DObject>()
                            : object->isDerivedFrom<Part::Feature>() && name.starts_with("Edge");
    }

private:
    const App::Document* document;
};
}  // namespace

TaskRibParameters::TaskRibParameters(ViewProviderRib* view)
    : TaskSketchBasedParameters(view, nullptr, "PartDesign_Rib", tr("Rib Parameters"))
    , ui(std::make_unique<Ui_TaskRibParameters>())
    , advancedUi(std::make_unique<Ui_TaskRibAdvancedParameters>())
    , advanced(new Gui::TaskView::TaskBox(tr("Advanced Rib Parameters")))
{
    auto container = new QWidget(this);
    ui->setupUi(container);
    groupLayout()->addWidget(container);
    advanced->setObjectName(QStringLiteral("ribAdvancedParameters"));
    auto advancedContainer = new QWidget(advanced);
    advancedUi->setupUi(advancedContainer);
    advanced->groupLayout()->addWidget(advancedContainer);

    auto rib = getObject<PartDesign::Rib>();
    ui->ribClearProfile->setIcon(Gui::BitmapFactory().iconFromTheme("edit-delete"));
    refreshProfile();
    refreshEnums();

    ui->ribThickness->setMinimum(0.0);
    ui->ribThickness->setMaximum(1e9);
    ui->ribThickness->setValue(rib->Thickness.getQuantityValue());
    ui->ribThickness->bind(rib->Thickness);

    // Distance is currently a PropertyFloat in the model (millimetres), not a
    // PropertyLength. Display user units but write the numeric internal-unit value.
    ui->ribLength->setUnit(Base::Unit::Length);
    ui->ribLength->setMinimum(0.0);
    ui->ribLength->setMaximum(1e9);
    ui->ribLength->setValue(rib->Distance.getValue());


    ui->ribDraftAngle->setUnit(Base::Unit::Angle);
    ui->ribDraftAngle->setMinimum(-89.0);
    ui->ribDraftAngle->setMaximum(89.0);
    ui->ribDraftAngle->setValue(rib->DraftAngle.getQuantityValue());
    ui->ribDraftAngle->bind(rib->DraftAngle);

    // FilletRadius is a PropertyFloat in millimetres, not a PropertyLength.
    ui->ribFilletRadius->setUnit(Base::Unit::Length);
    ui->ribFilletRadius->setMinimum(0.0);
    ui->ribFilletRadius->setMaximum(1e9);
    ui->ribFilletRadius->setValue(rib->FilletRadius.getValue());
    ui->ribFilletRadius->bind(App::ObjectIdentifier::parse(rib, "FilletRadius"));

    advancedUi->ribUseCustomPullDirection->setChecked(rib->UseCustomPullDirection.getValue());
    for (auto field : {advancedUi->ribPullDirectionX, advancedUi->ribPullDirectionY,
                       advancedUi->ribPullDirectionZ}) {
        field->setMinimum(-1e9);
        field->setMaximum(1e9);
    }
    const auto pullDirection = rib->PullDirection.getValue();
    advancedUi->ribPullDirectionX->setValue(pullDirection.x);
    advancedUi->ribPullDirectionY->setValue(pullDirection.y);
    advancedUi->ribPullDirectionZ->setValue(pullDirection.z);
    advancedUi->ribPullDirectionX->bind(App::ObjectIdentifier::parse(rib, "PullDirection.x"));
    advancedUi->ribPullDirectionY->bind(App::ObjectIdentifier::parse(rib, "PullDirection.y"));
    advancedUi->ribPullDirectionZ->bind(App::ObjectIdentifier::parse(rib, "PullDirection.z"));
    for (auto field : {advancedUi->ribPullDirectionX, advancedUi->ribPullDirectionY,
                       advancedUi->ribPullDirectionZ}) {
        connect(field, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
                this, &TaskRibParameters::updatePullDirection);
    }

    for (auto field : {advancedUi->ribDirectionX, advancedUi->ribDirectionY, advancedUi->ribDirectionZ}) {
        field->setMinimum(-1e9);
        field->setMaximum(1e9);
    }
    const auto direction = rib->Direction.getValue();
    advancedUi->ribDirectionX->setValue(direction.x);
    advancedUi->ribDirectionY->setValue(direction.y);
    advancedUi->ribDirectionZ->setValue(direction.z);
    advancedUi->ribDirectionX->bind(App::ObjectIdentifier::parse(rib, "Direction.x"));
    advancedUi->ribDirectionY->bind(App::ObjectIdentifier::parse(rib, "Direction.y"));
    advancedUi->ribDirectionZ->bind(App::ObjectIdentifier::parse(rib, "Direction.z"));
    for (auto field : {advancedUi->ribDirectionX, advancedUi->ribDirectionY, advancedUi->ribDirectionZ}) {
        connect(field, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
                this, &TaskRibParameters::updateDirection);
    }
    updateVisibility();

    // Connect after initialization so opening the dialog does not alter the feature.
    connect(ui->ribSelectProfile, &QToolButton::toggled, this, &TaskRibParameters::selectProfile);
    connect(ui->ribClearProfile, &QToolButton::clicked, this, [this]() {
        finishSelection();
        getObject<PartDesign::Rib>()->Profile.setValue(nullptr);
        refreshProfile();
        recomputeFeature();
    });
    const auto connectEnum = [this](QComboBox* combo, App::PropertyEnumeration& property) {
        connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this,
                [this, combo, &property](int index) {
            if (index < 0) {
                return;
            }
            // Item data holds the untranslated model value, not the visible caption.
            property.setValue(combo->itemData(index).toString().toUtf8().constData());
            updateVisibility();
            recomputeFeature();
        });
    };
    connectEnum(ui->ribExtension, rib->ExtendType);
    connectEnum(ui->ribPlacement, rib->PlacementType);
    connectEnum(ui->ribExtent, rib->ExtentType);
    connect(ui->ribThickness, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, [this](double value) {
        getObject<PartDesign::Rib>()->Thickness.setValue(value);
        recomputeFeature();
    });
    connect(ui->ribDraftAngle, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, [this](double value) {
        getObject<PartDesign::Rib>()->DraftAngle.setValue(value);
        recomputeFeature();
    });
    connect(ui->ribFilletRadius, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, [this](double value) {
        getObject<PartDesign::Rib>()->FilletRadius.setValue(value);
        recomputeFeature();
    });
    connect(advancedUi->ribUseCustomPullDirection, &QCheckBox::toggled,
            this, [this](bool checked) {
        getObject<PartDesign::Rib>()->UseCustomPullDirection.setValue(checked);
        updateVisibility();
        recomputeFeature();
    });
    connect(ui->ribLength, qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this, [this](double value) {
        getObject<PartDesign::Rib>()->Distance.setValue(value);
        recomputeFeature();
    });
}

TaskRibParameters::~TaskRibParameters()
{
    finishSelection();
}

QWidget* TaskRibParameters::advancedPanel() const
{
    return advanced;
}

void TaskRibParameters::refreshEnums()
{
    const auto caption = [](const std::string& value) {
        if (value == "Off") return tr("Off");
        if (value == "C1") return tr("C1");
        if (value == "C2") return tr("C2");
        if (value == "Side A") return tr("Side A");
        if (value == "Side B") return tr("Side B");
        if (value == "Centered") return tr("Centered");
        if (value == "Shape") return tr("Shape");
        if (value == "Distance") return tr("Distance");
        return QString::fromStdString(value);
    };
    const auto populate = [&caption](QComboBox* combo, const App::PropertyEnumeration& property) {
        const QSignalBlocker blocker(combo);
        combo->clear();
        for (const auto& value : property.getEnumVector()) {
            combo->addItem(caption(value), QString::fromStdString(value));
        }
        combo->setCurrentIndex(combo->findData(QString::fromUtf8(property.getValueAsString())));
    };
    if (auto rib = getObject<PartDesign::Rib>()) {
        populate(ui->ribExtension, rib->ExtendType);
        populate(ui->ribPlacement, rib->PlacementType);
        populate(ui->ribExtent, rib->ExtentType);
    }
}

void TaskRibParameters::refreshProfile()
{
    if (auto rib = getObject<PartDesign::Rib>()) {
        QString text;
        if (auto profile = rib->Profile.getValue()) {
            text = QString::fromUtf8(profile->Label.getValue());
            QStringList edges;
            for (const auto& name : rib->Profile.getSubValues()) {
                if (!name.empty()) {
                    edges.push_back(QString::fromStdString(name));
                }
            }
            if (!edges.empty()) {
                text += QStringLiteral(": ") + edges.join(QStringLiteral(", "));
            }
        }
        ui->ribProfile->setText(text);
        ui->ribProfile->setPlaceholderText(pickingProfile ? tr("Selecting…") : tr("Select a sketch or edges"));
        ui->ribClearProfile->setEnabled(rib->Profile.getValue() != nullptr);
    }
}

void TaskRibParameters::updateVisibility()
{
    const bool distance = getObject<PartDesign::Rib>()->ExtentType.isValue("Distance");
    ui->ribLengthLabel->setVisible(distance);
    ui->ribLength->setVisible(distance);
    const bool custom = advancedUi->ribUseCustomPullDirection->isChecked();
    for (auto field : {advancedUi->ribPullDirectionX, advancedUi->ribPullDirectionY,
                       advancedUi->ribPullDirectionZ}) {
        field->setEnabled(custom);
    }
    for (auto label : {advancedUi->ribPullDirectionXLabel, advancedUi->ribPullDirectionYLabel,
                       advancedUi->ribPullDirectionZLabel}) {
        label->setEnabled(custom);
    }
}

void TaskRibParameters::updateDirection()
{
    getObject<PartDesign::Rib>()->Direction.setValue(
        advancedUi->ribDirectionX->value().getValue(),
        advancedUi->ribDirectionY->value().getValue(),
        advancedUi->ribDirectionZ->value().getValue()
    );
    recomputeFeature();
}

void TaskRibParameters::updatePullDirection()
{
    getObject<PartDesign::Rib>()->PullDirection.setValue(
        advancedUi->ribPullDirectionX->value().getValue(),
        advancedUi->ribPullDirectionY->value().getValue(),
        advancedUi->ribPullDirectionZ->value().getValue()
    );
    recomputeFeature();
}

void TaskRibParameters::selectProfile(bool enabled)
{
    if (!enabled) {
        finishSelection();
        return;
    }
    auto rib = getObject<PartDesign::Rib>();
    pickingProfile = true;
    selectedSource.clear();
    resolvedSource.clear();
    onSelectReference(AllowSelection::EDGE | AllowSelection::WHOLE);
    Gui::Selection().rmvSelectionGate();
    Gui::Selection().addSelectionGate(new RibProfileSelection(rib));
    if (auto profile = rib->Profile.getValue()) {
        shownProfile = profile->getNameInDocument();
        const auto view = getGuiDocument()->getViewProvider(profile);
        profileWasVisible = view && view->isVisible();
        getGuiDocument()->setShow(shownProfile.c_str());
    }
    refreshProfile();
}

void TaskRibParameters::finishSelection()
{
    if (!pickingProfile) {
        return;
    }
    if (getGuiDocument() && getAppDocument() && !shownProfile.empty()
        && getAppDocument()->getObject(shownProfile.c_str()) && !profileWasVisible) {
        getGuiDocument()->setHide(shownProfile.c_str());
    }
    shownProfile.clear();
    exitSelectionMode();
    pickingProfile = false;
    const QSignalBlocker blocker(ui->ribSelectProfile);
    ui->ribSelectProfile->setChecked(false);
    refreshProfile();
}

void TaskRibParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (!pickingProfile || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }
    auto rib = getObject<PartDesign::Rib>();
    if (!rib || std::string(msg.pDocName) != rib->getDocument()->getName()) {
        return;
    }
    auto raw = rib->getDocument()->getObject(msg.pObjectName);
    RibProfileSelection gate(rib);
    if (!gate.allow(rib->getDocument(), raw, msg.pSubName)) {
        return;
    }
    const bool add = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
    if (add && !selectedSource.empty() && selectedSource != raw->getNameInDocument()) {
        return; // A Profile link can hold multiple edges, but only from one object.
    }

    // Resolve an external object once per picking session. Copy the whole source
    // so Ctrl-added edges can all refer to the same copy, rather than separate copies.
    App::DocumentObject* object = selectedSource == raw->getNameInDocument()
        ? rib->getDocument()->getObject(resolvedSource.c_str()) : nullptr;
    if (!object) {
        auto wholeObjectSelection = msg;
        wholeObjectSelection.pSubName = "";
        std::vector<std::string> resolvedNames;
        if (!getReferencedSelection(rib, wholeObjectSelection, object, resolvedNames) || !object) {
            return;
        }
        selectedSource = raw->getNameInDocument();
        resolvedSource = object->getNameInDocument();
    }
    // The generic reference helper rewrites subnames for copies; retain our actual
    // edge selection (or the empty name for a whole sketch) on the resolved object.
    std::vector<std::string> names {msg.pSubName};
    if (add && object == rib->Profile.getValue() && !names.front().empty()) {
        auto existing = rib->Profile.getSubValues();
        existing.erase(std::remove(existing.begin(), existing.end(), ""), existing.end());
        if (std::find(existing.begin(), existing.end(), names.front()) == existing.end()) {
            existing.push_back(names.front());
        }
        names = existing;
    }
    rib->Profile.setValue(object, names);
    Gui::Selection().clearSelection();
    if (!add || names.front().empty()) {
        finishSelection();
    }
    refreshProfile();
    recomputeFeature();
}

void TaskRibParameters::apply()
{
    finishSelection();
    TaskSketchBasedParameters::apply();
    auto rib = getObject<PartDesign::Rib>();
    ui->ribThickness->apply();
    FCMD_OBJ_CMD(rib, "Thickness = " << ui->ribThickness->value().getValue());
    advancedUi->ribDirectionX->apply();
    advancedUi->ribDirectionY->apply();
    advancedUi->ribDirectionZ->apply();
    ui->ribDraftAngle->apply();
    FCMD_OBJ_CMD(rib, "DraftAngle = " << ui->ribDraftAngle->value().getValue());
    ui->ribFilletRadius->apply();
    if (!ui->ribFilletRadius->hasExpression()) {
        FCMD_OBJ_CMD(rib, "FilletRadius = " << ui->ribFilletRadius->value().getValue());
    }
    advancedUi->ribPullDirectionX->apply();
    advancedUi->ribPullDirectionY->apply();
    advancedUi->ribPullDirectionZ->apply();
    FCMD_OBJ_CMD(rib, "UseCustomPullDirection = "
                 << (advancedUi->ribUseCustomPullDirection->isChecked() ? "True" : "False"));
    const auto pullDirection = rib->PullDirection.getValue();
    FCMD_OBJ_CMD(rib, "PullDirection = (" << pullDirection.x << ", " << pullDirection.y
                 << ", " << pullDirection.z << ")");
    FCMD_OBJ_CMD(rib, "ExtendType = " << rib->ExtendType.getValue());
    FCMD_OBJ_CMD(rib, "PlacementType = " << rib->PlacementType.getValue());
    FCMD_OBJ_CMD(rib, "ExtentType = " << rib->ExtentType.getValue());
    FCMD_OBJ_CMD(rib, "Distance = " << ui->ribLength->value().getValue());
    const auto direction = rib->Direction.getValue();
    FCMD_OBJ_CMD(rib, "Direction = (" << direction.x << ", " << direction.y << ", " << direction.z << ")");
    const auto profile = rib->Profile.getValue();
    const std::string reference = profile
        ? "(" + Gui::Command::getObjectCmd(profile) + ", "
            + buildLinkSubPythonStr(profile, rib->Profile.getSubValues()) + ")"
        : "None";
    FCMD_OBJ_CMD(rib, "Profile = " << reference);
}

void TaskRibParameters::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this->findChild<QWidget*>(QStringLiteral("ribParametersPanel")));
        advancedUi->retranslateUi(advanced->findChild<QWidget*>(QStringLiteral("ribAdvancedPanel")));
        setHeaderText(tr("Rib Parameters"));
        advanced->setHeaderText(tr("Advanced Rib Parameters"));
        refreshEnums();
        refreshProfile();
    }
    TaskSketchBasedParameters::changeEvent(event);
}

TaskDlgRibParameters::TaskDlgRibParameters(ViewProviderRib* view)
    : TaskDlgSketchBasedParameters(view)
    , parameters(new TaskRibParameters(view))
{
    Content.push_back(parameters);
    Content.push_back(parameters->advancedPanel());
    Content.push_back(preview);
}

bool TaskDlgRibParameters::accept()
{
    parameters->finishSelection();
    return TaskDlgSketchBasedParameters::accept();
}

bool TaskDlgRibParameters::reject()
{
    parameters->finishSelection();
    return TaskDlgSketchBasedParameters::reject();
}

}  // namespace PartDesignGui
