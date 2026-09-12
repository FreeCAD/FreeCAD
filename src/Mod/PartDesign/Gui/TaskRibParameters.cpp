// SPDX-License-Identifier: LGPL-2.1-or-later

#include "TaskRibParameters.h"
#include "ReferenceSelection.h"
#include "ViewProviderRib.h"
#include "ui_TaskRibParameters.h"

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
#include <Gui/Inventor/Draggers/SoRotationDragger.h>
#include <Base/Converter.h>
#include <Gui/Utilities.h>
#include <Mod/Part/App/Tools.h>
#include <Precision.hxx>
#include <gp_Ax3.hxx>
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
        : NoDependentsSelection(rib)
        , document(rib->getDocument())
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
{
    auto container = new QWidget(this);
    ui->setupUi(container);
    groupLayout()->addWidget(container);

    auto rib = getObject<PartDesign::Rib>();
    ui->ribClearProfile->setIcon(Gui::BitmapFactory().iconFromTheme("edit-delete"));
    refreshProfile();
    refreshEnums();
    ui->ribReversed->setChecked(rib->Reversed.getValue());

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
    ui->ribLength->bind(App::ObjectIdentifier::parse(rib, "Distance"));

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

    updateVisibility();

    // Connect after initialization so opening the dialog does not alter the feature.
    connect(ui->ribSelectProfile, &QToolButton::toggled, this, &TaskRibParameters::selectProfile);
    connect(ui->ribClearProfile, &QToolButton::clicked, this, [this]() {
        finishSelection();
        getObject<PartDesign::Rib>()->Profile.setValue(nullptr);
        refreshProfile();
        updateRib();
    });
    const auto connectEnum = [this](QComboBox* combo, App::PropertyEnumeration& property) {
        connect(
            combo,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            [this, combo, &property](int index) {
                if (index < 0) {
                    return;
                }
                // Item data holds the untranslated model value, not the visible caption.
                property.setValue(combo->itemData(index).toString().toUtf8().constData());
                updateVisibility();
                updateRib();
            }
        );
    };
    connectEnum(ui->ribExtension, rib->ExtendType);
    connectEnum(ui->ribPlacement, rib->PlacementType);
    connectEnum(ui->ribExtent, rib->ExtentType);
    connectEnum(ui->ribDraftReference, rib->DraftReference);
    connect(ui->ribReversed, &QCheckBox::toggled, this, [this](bool reversed) {
        getObject<PartDesign::Rib>()->Reversed.setValue(reversed);
        updateRib();
    });
    connect(
        ui->ribThickness,
        qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this,
        [this](double value) {
            getObject<PartDesign::Rib>()->Thickness.setValue(value);
            updateRib();
        }
    );
    connect(
        ui->ribDraftAngle,
        qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this,
        [this](double value) {
            getObject<PartDesign::Rib>()->DraftAngle.setValue(value);
            updateRib();
        }
    );
    connect(
        ui->ribFilletRadius,
        qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this,
        [this](double value) {
            getObject<PartDesign::Rib>()->FilletRadius.setValue(value);
            updateRib();
        }
    );
    connect(
        ui->ribLength,
        qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
        this,
        [this](double value) {
            getObject<PartDesign::Rib>()->Distance.setValue(value);
            updateRib();
        }
    );
    setupGizmos();
}

TaskRibParameters::~TaskRibParameters()
{
    finishSelection();
}

void TaskRibParameters::refreshEnums()
{
    const auto caption = [](const std::string& value) {
        if (value == "Off") {
            return tr("Off");
        }
        if (value == "C1") {
            return tr("C1");
        }
        if (value == "C2") {
            return tr("C2");
        }
        if (value == "Side A") {
            return tr("Side A");
        }
        if (value == "Side B") {
            return tr("Side B");
        }
        if (value == "Centered") {
            return tr("Centered");
        }
        if (value == "Free end") {
            return tr("Free End");
        }
        if (value == "Root") {
            return tr("Root");
        }
        if (value == "Shape") {
            return tr("Shape");
        }
        if (value == "Distance") {
            return tr("Distance");
        }
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
        populate(ui->ribDraftReference, rib->DraftReference);
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
        ui->ribProfile->setPlaceholderText(
            pickingProfile ? tr("Selecting…") : tr("Select a sketch or edges")
        );
        ui->ribClearProfile->setEnabled(rib->Profile.getValue() != nullptr);
    }
}

void TaskRibParameters::updateVisibility()
{
    const bool distance = getObject<PartDesign::Rib>()->ExtentType.isValue("Distance");
    ui->ribLengthLabel->setVisible(distance);
    ui->ribLength->setVisible(distance);
}

void TaskRibParameters::selectProfile(bool enabled)
{
    if (!enabled) {
        finishSelection();
        return;
    }
    auto rib = getObject<PartDesign::Rib>();
    pickingProfile = true;
    if (gizmoContainer) {
        gizmoContainer->visible = false;
    }
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
    setGizmoPositions();
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
        return;  // A Profile link can hold multiple edges, but only from one object.
    }

    // Resolve an external object once per picking session. Copy the whole source
    // so Ctrl-added edges can all refer to the same copy, rather than separate copies.
    App::DocumentObject* object = selectedSource == raw->getNameInDocument()
        ? rib->getDocument()->getObject(resolvedSource.c_str())
        : nullptr;
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
    updateRib();
}

void TaskRibParameters::apply()
{
    finishSelection();
    TaskSketchBasedParameters::apply();
    auto rib = getObject<PartDesign::Rib>();
    ui->ribThickness->apply();
    if (!ui->ribThickness->hasExpression()) {
        FCMD_OBJ_CMD(rib, "Thickness = " << ui->ribThickness->value().getValue());
    }
    ui->ribDraftAngle->apply();
    if (!ui->ribDraftAngle->hasExpression()) {
        FCMD_OBJ_CMD(rib, "DraftAngle = " << ui->ribDraftAngle->value().getValue());
    }
    ui->ribFilletRadius->apply();
    if (!ui->ribFilletRadius->hasExpression()) {
        FCMD_OBJ_CMD(rib, "FilletRadius = " << ui->ribFilletRadius->value().getValue());
    }
    FCMD_OBJ_CMD(rib, "ExtendType = " << rib->ExtendType.getValue());
    FCMD_OBJ_CMD(rib, "PlacementType = " << rib->PlacementType.getValue());
    FCMD_OBJ_CMD(rib, "ExtentType = " << rib->ExtentType.getValue());
    FCMD_OBJ_CMD(rib, "DraftReference = " << rib->DraftReference.getValue());
    FCMD_OBJ_CMD(rib, "Reversed = " << (rib->Reversed.getValue() ? "True" : "False"));
    ui->ribLength->apply();
    if (!ui->ribLength->hasExpression()) {
        FCMD_OBJ_CMD(rib, "Distance = " << ui->ribLength->value().getValue());
    }
    const auto profile = rib->Profile.getValue();
    const std::string reference = profile ? "(" + Gui::Command::getObjectCmd(profile) + ", "
            + buildLinkSubPythonStr(profile, rib->Profile.getSubValues()) + ")"
                                          : "None";
    FCMD_OBJ_CMD(rib, "Profile = " << reference);
}

void TaskRibParameters::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this->findChild<QWidget*>(QStringLiteral("ribParametersPanel")));
        setHeaderText(tr("Rib Parameters"));
        refreshEnums();
        refreshProfile();
    }
    TaskSketchBasedParameters::changeEvent(event);
}


void TaskRibParameters::updateRib()
{
    recomputeFeature();
    // A failed preview must not remove the controls needed to correct its inputs.
    setGizmoPositions();
}

void TaskRibParameters::setupGizmos()
{
    if (!Gui::GizmoContainer::isEnabled()) {
        return;
    }
    thicknessGizmo = new Gui::LinearGizmo(ui->ribThickness);
    lengthGizmo = new Gui::LinearGizmo(ui->ribLength);
    lengthGizmo->setClickCallback([this] {
        ui->ribReversed->setChecked(!ui->ribReversed->isChecked());
    });
    draftGizmo = new Gui::RotationGizmo(ui->ribDraftAngle);
    gizmoContainer = Gui::GizmoContainer::create(
        {thicknessGizmo, lengthGizmo, draftGizmo},
        getViewObject<ViewProviderRib>()
    );
    setGizmoPositions();
    showDraggerHints();
}

void TaskRibParameters::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }
    gizmoContainer->visible = false;
    auto rib = getObject<PartDesign::Rib>();
    if (!rib || !rib->Profile.getValue() || pickingProfile) {
        return;
    }
    try {
        const auto profile = rib->getRibProfileWire();
        Base::Vector3d centerValue;
        if (!profile.getCenterOfGravity(centerValue)) {
            return;
        }
        gp_Pnt center(centerValue.x, centerValue.y, centerValue.z);
        const auto plane = rib->getRibProfilePlane();
        const gp_Vec normal(plane.Axis().Direction());
        const auto direction = rib->Direction.getValue();
        gp_Vec travel(direction.x, direction.y, direction.z);
        if (travel.Magnitude() <= Precision::Confusion()) {
            return;
        }
        travel.Normalize();
        if (rib->Reversed.getValue()) {
            travel.Reverse();
        }
        gp_Vec pull = -travel;
        if (rib->UseCustomPullDirection.getValue()) {
            const auto value = rib->PullDirection.getValue();
            pull = gp_Vec(value.x, value.y, value.z);
        }
        // Properties are Rib-local; the scene graph and selected profile are world-space.
        travel.Transform(rib->getLocation().Transformation());
        pull.Transform(rib->getLocation().Transformation());
        if (pull.Magnitude() <= Precision::Confusion()) {
            return;
        }
        pull.Normalize();

        auto anchor = center;
        const auto addition = rib->AddSubShape.getShape();
        if (!addition.isNull()) {
            auto material = addition.moved(rib->getLocation());
            gp_Trsf frame;
            frame.SetTransformation(gp_Ax3(center, gp_Dir(pull)));
            const auto bounds = material.moved(TopLoc_Location(frame)).getBoundBoxOptimal();
            anchor.Translate(pull * (rib->DraftReference.isValue("Root") ? bounds.MinZ : bounds.MaxZ));
        }
        const gp_Vec width = rib->PlacementType.isValue("Side B") ? -normal : normal;
        const double fraction = rib->PlacementType.isValue("Centered") ? .5 : 1.;
        thicknessGizmo->setMultFactor(fraction);
        thicknessGizmo->Gizmo::setDraggerPlacement(
            Base::convertTo<Base::Vector3d>(anchor),
            Base::convertTo<Base::Vector3d>(width)
        );
        lengthGizmo->Gizmo::setDraggerPlacement(
            Base::convertTo<Base::Vector3d>(center),
            Base::convertTo<Base::Vector3d>(travel)
        );
        lengthGizmo->setVisibility(rib->ExtentType.isValue("Distance"));

        gp_Vec rotationAxis = width.Crossed(pull);
        if (rotationAxis.Magnitude() > Precision::Confusion()) {
            rotationAxis.Normalize();
            const auto at = anchor.Translated(width * (fraction * rib->Thickness.getValue()));
            draftGizmo->Gizmo::setDraggerPlacement(
                Base::convertTo<Base::Vector3d>(at),
                Base::convertTo<Base::Vector3d>(pull)
            );
            draftGizmo->getDraggerContainer()->setArcNormalDirection(
                Base::convertTo<SbVec3f>(rotationAxis)
            );
            draftGizmo->automaticOrientation = false;
        }
        draftGizmo->setVisibility(rotationAxis.Magnitude() > Precision::Confusion());
        gizmoContainer->visible = true;
        gizmoContainer->calculateScaleAndOrientation();
    }
    catch (const Base::Exception&) {
    }
    catch (const Standard_Failure&) {
    }
    catch (const std::exception&) {
    }
}

TaskDlgRibParameters::TaskDlgRibParameters(ViewProviderRib* view)
    : TaskDlgSketchBasedParameters(view)
    , parameters(new TaskRibParameters(view))
{
    Content.push_back(parameters);
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
