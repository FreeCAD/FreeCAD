// SPDX-License-Identifier: LGPL-2.1-or-later
#include "TaskRibParameters.h"
#include "ui_TaskRibParameters.h"
#include "ui_TaskRibAdvancedParameters.h"
#include "ReferenceSelection.h"
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QApplication>
#include <QLineEdit>
#include <QSignalBlocker>
#include <QToolButton>
#include <Gui/PrefWidgets.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Utilities.h>
#include <App/ObjectIdentifier.h>
#include <App/Document.h>
#include <Base/Converter.h>
#include <BRepGProp.hxx>
#include <Mod/PartDesign/App/ThinExtrusion.h>
#include <BRepAdaptor_Curve.hxx>
#include <BRep_Tool.hxx>
#include <GProp_GProps.hxx>
#include <TopoDS.hxx>
#include <Base/BoundBox.h>
#include <Gui/Inventor/Draggers/SoRotationDragger.h>
#include <Mod/PartDesign/App/FeatureRib.h>
#include <Mod/Part/App/Tools.h>

namespace PartDesignGui
{
namespace
{
// Operation-aware filtering also covers origin points, linked datums and whole
// single-axis sketches that the generic edge/face filter cannot classify.
class RibDirectionSelection: public NoDependentsSelection
{
    const PartDesign::Rib* rib;
    bool toward;

public:
    RibDirectionSelection(const PartDesign::Rib* rib, bool toward)
        : NoDependentsSelection(rib)
        , rib(rib)
        , toward(toward)
    {}

    bool allow(App::Document* doc, App::DocumentObject* object, const char* sub) override
    {
        if (doc != rib->getDocument() || !object || !NoDependentsSelection::allow(doc, object, sub)) {
            return false;
        }

        try {
            rib->referenceDirection(object, {sub ? sub : ""}, toward);
            return true;
        }
        catch (const Base::Exception&) {
            return false;
        }
        catch (const Standard_Failure&) {
            return false;
        }
    }
};
}  // namespace

TaskRibParameters::TaskRibParameters(ViewProviderPad* view)
    : TaskSketchBasedParameters(view, nullptr, "PartDesign_Rib", tr("Rib Parameters"))
    , ui(std::make_unique<Ui_TaskRibParameters>())
    , advancedUi(std::make_unique<Ui_TaskRibAdvancedParameters>())
{
    auto rib = getObject<PartDesign::Rib>();
    auto container = new QWidget(this);
    ui->setupUi(container);
    groupLayout()->addWidget(container);

    advanced = new Gui::TaskView::TaskBox(tr("Advanced Rib Parameters"));
    advanced->setObjectName(QStringLiteral("ribAdvancedParameters"));
    auto advancedContainer = new QWidget(advanced);
    advancedUi->setupUi(advancedContainer);
    advanced->groupLayout()->addWidget(advancedContainer);

    // Designer owns the layout; these bindings keep selection and model updates together.
    auto picker =
        [this](QToolButton* button, QToolButton* reset, Pick mode, std::function<void()> clear) {
            reset->setIcon(Gui::BitmapFactory().iconFromTheme("edit-delete"));
            connect(button, &QToolButton::clicked, this, [this, mode]() { select(mode); });

            connect(reset, &QToolButton::clicked, this, [this, clear]() {
                finishSelection();
                clear();
                refresh();
                updateFeature();
            });
        };

    picker(ui->ribSelectProfile, ui->ribClearProfile, Pick::Profile, [rib]() {
        rib->Profile.setValue(nullptr);
    });

    ui->ribExtension->setCurrentIndex(rib->Extension.getValue());
    connect(ui->ribExtension, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, rib](int value) {
        rib->Extension.setValue(value);
        updateFeature();
    });

    auto quantity = [this](
                        Gui::PrefQuantitySpinBox* field,
                        App::PropertyQuantity& property,
                        bool signedValue = false
                    ) {
        field->setMinimum(signedValue ? -1e9 : 0);
        field->setMaximum(1e9);
        field->setValue(property.getQuantityValue());
        field->bind(property);
        quantities.push_back(field);

        connect(
            field,
            qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this,
            [this, &property](double value) {
                property.setValue(value);
                updateFeature();
            }
        );
    };

    quantity(ui->ribThickness, rib->ThinThickness);

    ui->ribPlacement->setCurrentIndex(rib->ThinSide.getValue());
    quantity(ui->ribThickness2, rib->ThinThickness2);
    connect(ui->ribPlacement, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, rib](int value) {
        rib->ThinSide.setValue(value);
        updateVisibility();
        updateFeature();
    });

    // Store stable feature enum values independently of translated captions.
    ui->ribExtent->setItemData(0, QStringLiteral("UpToShape"));
    ui->ribExtent->setItemData(1, QStringLiteral("Length"));
    const auto savedType = QString::fromLatin1(rib->Type.getValueAsString());
    ui->ribExtent->setCurrentIndex(ui->ribExtent->findData(savedType));
    quantity(ui->ribLength, rib->Length);
    ui->ribReversed->setChecked(rib->Reversed.getValue());
    picker(ui->ribSelectTarget, ui->ribClearTarget, Pick::Target, [rib]() {
        rib->UpToShape.setValues({}, {});
    });

    connect(ui->ribExtent, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, rib]() {
        finishSelection();
        rib->Type.setValue(ui->ribExtent->currentData().toString().toLatin1().constData());
        // Shape in this compact panel means contact with the selected body.
        if (ui->ribExtent->currentData() == QStringLiteral("UpToShape")) {
            rib->Offset.setValue(0);
        }
        updateVisibility();
        updateFeature();
    });
    connect(ui->ribReversed, &QCheckBox::toggled, this, [this, rib](bool value) {
        finishSelection();
        rib->Reversed.setValue(value);
        updateVisibility();
        updateFeature();
    });

    quantity(ui->ribDraftAngle, rib->TaperAngle, true);
    ui->ribDraftAngle->setMinimum(rib->TaperAngle.getMinimum());
    ui->ribDraftAngle->setMaximum(rib->TaperAngle.getMaximum());
    ui->ribDraftAngle->setSingleStep(rib->TaperAngle.getStepSize());
    quantity(ui->ribFilletRadius, rib->RootFilletRadius);

    advancedUi->ribDirectionMode->setCurrentIndex(
        rib->UseCustomVector.getValue()       ? 3
            : rib->TowardReference.getValue() ? 1
            : rib->ReferenceAxis.getValue()   ? 2
            : rib->AutoDirection.getValue()   ? 0
                                              : 4
    );
    picker(advancedUi->ribSelectDirection, advancedUi->ribClearDirection, Pick::Direction, [this, rib]() {
        rib->ReferenceAxis.setValue(nullptr);
        rib->TowardReference.setValue(false);
        rib->UseCustomVector.setValue(false);
        rib->AutoDirection.setValue(true);
        QSignalBlocker blocker(advancedUi->ribDirectionMode);
        advancedUi->ribDirectionMode->setCurrentIndex(0);
    });

    direction = {advancedUi->ribDirectionX, advancedUi->ribDirectionY, advancedUi->ribDirectionZ};
    const auto vector = advancedUi->ribDirectionMode->currentIndex() == 4
        ? rib->FillDirection.getValue()
        : rib->Direction.getValue();
    const double values[] = {vector.x, vector.y, vector.z};
    for (int i = 0; i < 3; ++i) {
        direction[i]->setMinimum(-1e9);
        direction[i]->setMaximum(1e9);
        direction[i]->setValue(values[i]);

        connect(
            direction[i],
            qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this,
            [this, rib]() {
                const Base::Vector3d value(
                    direction[0]->value().getValue(),
                    direction[1]->value().getValue(),
                    direction[2]->value().getValue()
                );
                if (advancedUi->ribDirectionMode->currentIndex() == 4) {
                    rib->FillDirection.setValue(value);
                }
                else {
                    rib->Direction.setValue(value);
                }
                updateFeature();
            }
        );
    }

    connect(
        advancedUi->ribDirectionMode,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this, rib](int mode) {
            finishSelection();
            rib->AutoDirection.setValue(mode == 0);
            rib->TowardReference.setValue(mode == 1);
            rib->UseCustomVector.setValue(mode == 3);
            if (mode == 0 || mode == 3 || mode == 4) {
                rib->ReferenceAxis.setValue(nullptr);
            }
            const auto vector = mode == 4 ? rib->FillDirection.getValue() : rib->Direction.getValue();
            const double values[] = {vector.x, vector.y, vector.z};
            for (int i = 0; i < 3; ++i) {
                QSignalBlocker blocker(direction[i]);
                direction[i]->setValue(values[i]);
            }
            refresh();
            updateFeature();
        }
    );

    advancedUi->ribPullMode->setCurrentIndex(rib->DraftPullMode.getValue());
    picker(
        advancedUi->ribSelectPullDirection,
        advancedUi->ribClearPullDirection,
        Pick::Pull,
        [this, rib]() {
            rib->DraftPullDirection.setValue(nullptr);
            rib->DraftPullMode.setValue(0L);
            QSignalBlocker blocker(advancedUi->ribPullMode);
            advancedUi->ribPullMode->setCurrentIndex(0);
        }
    );

    pullVector = {advancedUi->ribPullX, advancedUi->ribPullY, advancedUi->ribPullZ};
    const auto pull = rib->DraftPullVector.getValue();
    const double pullValues[] = {pull.x, pull.y, pull.z};
    for (int i = 0; i < 3; ++i) {
        pullVector[i]->setMinimum(-1e9);
        pullVector[i]->setMaximum(1e9);
        pullVector[i]->setValue(pullValues[i]);

        connect(
            pullVector[i],
            qOverload<double>(&Gui::PrefQuantitySpinBox::valueChanged),
            this,
            [this, rib]() {
                rib->DraftPullVector.setValue(
                    Base::Vector3d(
                        pullVector[0]->value().getValue(),
                        pullVector[1]->value().getValue(),
                        pullVector[2]->value().getValue()
                    )
                );
                updateFeature();
            }
        );
    }

    connect(
        advancedUi->ribPullMode,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        [this, rib](int mode) {
            finishSelection();
            rib->DraftPullMode.setValue(mode);
            if (mode == 0) {
                rib->DraftPullDirection.setValue(nullptr);
            }
            refresh();
            updateFeature();
        }
    );

    advancedUi->ribFlipPullDirection->setChecked(rib->FlipPullDirection.getValue());
    connect(advancedUi->ribFlipPullDirection, &QCheckBox::toggled, this, [this, rib](bool value) {
        rib->FlipPullDirection.setValue(value);
        updateFeature();
    });

    refresh();
    advanced->hideGroupBox();
    setupGizmos();
}

TaskRibParameters::~TaskRibParameters()
{
    finishSelection();
}

QWidget* TaskRibParameters::advancedPanel() const
{
    return advanced;
}

void TaskRibParameters::changeEvent(QEvent* event)
{
    TaskBox::changeEvent(event);
    if (event->type() == QEvent::LanguageChange) {
        setHeaderText(tr("Rib Parameters"));
        advanced->setHeaderText(tr("Advanced Rib Parameters"));
        ui->retranslateUi(ui->form->parentWidget());
        advancedUi->retranslateUi(advancedUi->advancedForm->parentWidget());
    }
}

void TaskRibParameters::updateVisibility()
{
    auto show = [](QFormLayout* layout, QWidget* field, bool visible) {
        field->setVisible(visible);
        if (auto label = layout->labelForField(field)) {
            label->setVisible(visible);
        }
    };

    show(ui->form, ui->ribLength, ui->ribExtent->currentData() == QStringLiteral("Length"));
    show(ui->form, ui->ribReversed, ui->ribExtent->currentData() == QStringLiteral("Length"));
    show(ui->form, ui->ribThickness2, ui->ribPlacement->currentIndex() == 3);

    // Keep a saved explicit target discoverable even before the user reverses.
    show(
        ui->form,
        ui->ribTargetRow,
        ui->ribExtent->currentData() == QStringLiteral("UpToShape")
            && (ui->ribReversed->isChecked()
                || !getObject<PartDesign::Rib>()->UpToShape.getValues().empty())
    );
    show(
        advancedUi->advancedForm,
        advancedUi->ribReferenceRow,
        advancedUi->ribDirectionMode->currentIndex() == 1
            || advancedUi->ribDirectionMode->currentIndex() == 2
    );
    for (auto field : direction) {
        show(advancedUi->advancedForm, field, advancedUi->ribDirectionMode->currentIndex() >= 3);
    }
    show(
        advancedUi->advancedForm,
        advancedUi->ribPullRow,
        advancedUi->ribPullMode->currentIndex() == 2 || advancedUi->ribPullMode->currentIndex() == 3
    );
    for (auto field : pullVector) {
        show(advancedUi->advancedForm, field, advancedUi->ribPullMode->currentIndex() >= 4);
    }
}

void TaskRibParameters::refresh()
{
    auto rib = getObject<PartDesign::Rib>();
    auto showReference = [](QLineEdit* edit,
                            const App::PropertyLinkSub& property,
                            const QString& empty) {
        edit->setText(
            property.getValue() ? getRefStr(property.getValue(), property.getSubValues()) : QString()
        );
        edit->setPlaceholderText(empty);
        edit->setToolTip(edit->text());
    };

    showReference(ui->ribProfile, rib->Profile, tr("No Profile Selected"));
    showReference(advancedUi->ribDirectionReference, rib->ReferenceAxis, tr("No Reference Selected"));
    showReference(advancedUi->ribPullReference, rib->DraftPullDirection, tr("No Reference Selected"));

    QStringList targets;
    const auto& objects = rib->UpToShape.getValues();
    const auto& subs = rib->UpToShape.getSubValues();
    for (size_t i = 0; i < objects.size(); ++i) {
        targets << getRefStr(objects[i], {subs[i]});
    }

    ui->ribTarget->setText(targets.join(QStringLiteral("; ")));
    ui->ribTarget->setPlaceholderText(tr("Previous Shape"));
    ui->ribTarget->setToolTip(ui->ribTarget->text());
    updateVisibility();
}

void TaskRibParameters::finishSelection()
{
    if (picking == Pick::None) {
        return;
    }

    auto rib = getObject<PartDesign::Rib>();
    if (picking == Pick::Profile && rib && rib->Profile.getValue() && !profileVisible) {
        getGuiDocument()->setHide(rib->Profile.getValue()->getNameInDocument());
    }
    exitSelectionMode();
    picking = Pick::None;
    for (auto button :
         {ui->ribSelectProfile,
          ui->ribSelectTarget,
          advancedUi->ribSelectDirection,
          advancedUi->ribSelectPullDirection}) {
        QSignalBlocker blocker(button);
        button->setChecked(false);
    }
    refresh();
    setGizmoPositions();
}

void TaskRibParameters::select(Pick mode)
{
    const auto old = picking;
    finishSelection();
    if (old == mode) {
        return;
    }

    picking = mode;
    auto flags = mode == Pick::Target ? AllowSelection::FACE | AllowSelection::WHOLE
        : mode == Pick::Profile ? AllowSelection::EDGE | AllowSelection::FACE | AllowSelection::WHOLE
                                : AllowSelectionFlags(AllowSelection::EDGE);
    onSelectReference(flags);

    auto rib = getObject<PartDesign::Rib>();
    if (mode == Pick::Direction || mode == Pick::Pull) {
        Gui::Selection().rmvSelectionGate();
        Gui::Selection().addSelectionGate(new RibDirectionSelection(
            rib,
            mode == Pick::Direction ? advancedUi->ribDirectionMode->currentIndex() == 1
                                    : advancedUi->ribPullMode->currentIndex() == 2
        ));
    }

    if (mode == Pick::Profile && rib->Profile.getValue()) {
        auto view = getGuiDocument()->getViewProvider(rib->Profile.getValue());
        profileVisible = view && view->isVisible();
        getGuiDocument()->setShow(rib->Profile.getValue()->getNameInDocument());
    }

    (mode == Pick::Profile         ? ui->ribSelectProfile
         : mode == Pick::Target    ? ui->ribSelectTarget
         : mode == Pick::Direction ? advancedUi->ribSelectDirection
                                   : advancedUi->ribSelectPullDirection)
        ->setChecked(true);

    auto edit = mode == Pick::Profile ? ui->ribProfile
        : mode == Pick::Target        ? ui->ribTarget
        : mode == Pick::Direction     ? advancedUi->ribDirectionReference
                                      : advancedUi->ribPullReference;
    edit->clear();
    edit->setPlaceholderText(tr("Selecting…"));
    if (gizmoContainer) {
        gizmoContainer->visible = false;
    }
}

void TaskRibParameters::onSelectionChanged(const Gui::SelectionChanges& msg)
{
    if (picking == Pick::None || msg.Type != Gui::SelectionChanges::AddSelection) {
        return;
    }

    auto rib = getObject<PartDesign::Rib>();
    App::DocumentObject* object = nullptr;
    std::vector<std::string> names;
    NoDependentsSelection gate(rib);
    auto raw = rib->getDocument()->getObject(msg.pObjectName);
    if (!raw || !gate.allow(rib->getDocument(), raw, msg.pSubName)
        || !getReferencedSelection(rib, msg, object, names) || !object || names.empty()) {
        return;
    }

    if (picking == Pick::Profile) {
        if (!names.front().empty() && names.front().rfind("Edge", 0) != 0) {
            return;
        }
        const bool add = QApplication::keyboardModifiers().testFlag(Qt::ControlModifier);
        if (add && object == rib->Profile.getValue() && !names.front().empty()) {
            auto old = rib->Profile.getSubValues();
            old.erase(std::remove(old.begin(), old.end(), ""), old.end());
            if (std::find(old.begin(), old.end(), names.front()) == old.end()) {
                old.push_back(names.front());
            }
            names = old;
        }
        rib->Profile.setValue(object, names);
        Gui::Selection().clearSelection();
        if (!add || names.front().empty()) {
            finishSelection();
        }
    }
    else {
        if (picking == Pick::Target) {
            rib->UpToShape.setValues({object}, names);
        }
        else if (picking == Pick::Direction) {
            rib->ReferenceAxis.setValue(object, names);
        }
        else {
            rib->DraftPullDirection.setValue(object, names);
        }
        finishSelection();
    }
    refresh();
    updateFeature();
}

void TaskRibParameters::apply()
{
    finishSelection();

    auto rib = getObject<PartDesign::Rib>();
    for (auto field : quantities) {
        field->apply();
    }

    FCMD_OBJ_CMD(rib, "Extension = " << ui->ribExtension->currentIndex());
    FCMD_OBJ_CMD(rib, "ThinSide = " << ui->ribPlacement->currentIndex());
    FCMD_OBJ_CMD(rib, "Type = '" << rib->Type.getValueAsString() << "'");
    FCMD_OBJ_CMD(rib, "Reversed = " << (rib->Reversed.getValue() ? "True" : "False"));
    FCMD_OBJ_CMD(rib, "AutoDirection = " << (rib->AutoDirection.getValue() ? "True" : "False"));
    FCMD_OBJ_CMD(rib, "TowardReference = " << (rib->TowardReference.getValue() ? "True" : "False"));
    FCMD_OBJ_CMD(rib, "UseCustomVector = " << (rib->UseCustomVector.getValue() ? "True" : "False"));
    FCMD_OBJ_CMD(rib, "FlipPullDirection = " << (rib->FlipPullDirection.getValue() ? "True" : "False"));
    FCMD_OBJ_CMD(rib, "DraftPullMode = " << rib->DraftPullMode.getValue());

    const auto pull = rib->DraftPullVector.getValue();
    FCMD_OBJ_CMD(rib, "DraftPullVector = (" << pull.x << "," << pull.y << "," << pull.z << ")");

    auto link = [](const App::PropertyLinkSub& property) {
        return property.getValue() ? "(" + Gui::Command::getObjectCmd(property.getValue()) + ", "
                + buildLinkSubPythonStr(property.getValue(), property.getSubValues()) + ")"
                                   : "None";
    };
    FCMD_OBJ_CMD(rib, "Profile = " << link(rib->Profile));
    FCMD_OBJ_CMD(rib, "ReferenceAxis = " << link(rib->ReferenceAxis));
    FCMD_OBJ_CMD(rib, "DraftPullDirection = " << link(rib->DraftPullDirection));
    FCMD_OBJ_CMD(
        rib,
        "UpToShape = "
            << buildLinkSubListPythonStr(rib->UpToShape.getValues(), rib->UpToShape.getSubValues())
    );

    const auto value = rib->UseCustomVector.getValue() ? rib->Direction.getValue()
                                                       : rib->FillDirection.getValue();
    FCMD_OBJ_CMD(
        rib,
        (rib->UseCustomVector.getValue() ? "Direction" : "FillDirection")
            << " = (" << value.x << "," << value.y << "," << value.z << ")"
    );
}

void TaskRibParameters::updateFeature()
{
    recomputeFeature();
    setGizmoPositions();
}

void TaskRibParameters::setupGizmos()
{
    if (!Gui::GizmoContainer::isEnabled()) {
        return;
    }

    thicknessGizmo = new Gui::LinearGizmo(ui->ribThickness);
    thickness2Gizmo = new Gui::LinearGizmo(ui->ribThickness2);
    lengthGizmo = new Gui::LinearGizmo(ui->ribLength);
    lengthGizmo->setClickCallback([this]() {
        if (ui->ribExtent->currentData() == QStringLiteral("Length")) {
            ui->ribReversed->setChecked(!ui->ribReversed->isChecked());
        }
    });

    draftGizmo = new Gui::RotationGizmo(ui->ribDraftAngle);
    gizmoContainer = Gui::GizmoContainer::create(
        {thicknessGizmo, thickness2Gizmo, lengthGizmo, draftGizmo},
        getViewObject<ViewProviderPad>()
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
    if (!rib || rib->isError() || !rib->Profile.getValue() || picking != Pick::None) {
        return;
    }

    try {
        const auto source = rib->getInputProfile();
        const auto edges = source.getSubTopoShapes(TopAbs_EDGE);
        if (edges.empty()) {
            return;
        }

        GProp_GProps properties;
        BRepGProp::LinearProperties(source.getShape(), properties);
        auto center = properties.CentreOfMass();
        auto dir = Base::convertTo<gp_Dir>(rib->Direction.getValue());
        const auto growth = rib->Reversed.getValue() ? dir.Reversed() : dir;
        center.Translate(gp_Vec(growth) * rib->getStartOffset());

        // Side placement follows the first source edge's orientation, just as
        // the surface-extrusion engine does, rather than a world axis.
        BRepAdaptor_Curve curve(TopoDS::Edge(edges.front().getShape()));
        gp_Pnt at;
        gp_Vec tangent;
        curve.D1((curve.FirstParameter() + curve.LastParameter()) / 2, at, tangent);
        auto width = tangent.Crossed(gp_Vec(dir));
        if (width.Magnitude() <= Precision::Confusion()) {
            return;
        }
        width.Normalize();

        auto material = rib->AddSubShape.getShape();
        material.move(rib->getLocation());
        gp_Trsf toGrowth;
        toGrowth.SetTransformation(gp_Ax3(center, growth));
        const auto measured = Part::TopoShape().makeElementTransform(material, toGrowth);
        const auto bounds = measured.getBoundBoxOptimal();

        const bool rootAtStart = PartDesign::thinRootAtStart(source, rib->getBaseTopoShape(true));
        const bool holdTop = rib->ThinDraftReference.getValue() == 1;
        const double neutral = holdTop == rootAtStart ? bounds.MaxZ : bounds.MinZ;
        const auto anchor = center.Translated(gp_Vec(growth) * neutral);

        const auto side = rib->ThinSide.getValue();
        const auto widthDirection = side == 1 ? -width : width;
        const double widthFactor = side == 2 ? .5 : 1.;
        thicknessGizmo->setMultFactor(widthFactor);
        thicknessGizmo->Gizmo::setDraggerPlacement(
            Base::convertTo<Base::Vector3d>(anchor),
            Base::convertTo<Base::Vector3d>(widthDirection)
        );
        thickness2Gizmo->Gizmo::setDraggerPlacement(
            Base::convertTo<Base::Vector3d>(anchor),
            Base::convertTo<Base::Vector3d>(-width)
        );
        thickness2Gizmo->setVisibility(side == 3);

        lengthGizmo->Gizmo::setDraggerPlacement(
            Base::convertTo<Base::Vector3d>(center),
            Base::convertTo<Base::Vector3d>(growth)
        );
        lengthGizmo->setMultFactor(rib->SideType.getValue() == 2 ? .5 : 1.);
        lengthGizmo->setVisibility(ui->ribExtent->currentData() == QStringLiteral("Length"));

        auto pull = rootAtStart ? growth : growth.Reversed();
        if (const auto explicitPull = rib->getDraftPullVector()) {
            pull = Base::convertTo<gp_Dir>(*explicitPull);
        }
        if (rib->FlipPullDirection.getValue()) {
            pull.Reverse();
        }

        auto rotationAxis = widthDirection.Crossed(gp_Vec(pull));
        draftGizmo->setVisibility(rotationAxis.Magnitude() > Precision::Confusion());
        if (rotationAxis.Magnitude() > Precision::Confusion()) {
            rotationAxis.Normalize();
            const auto draftAnchor = anchor.Translated(
                widthDirection * rib->ThinThickness.getValue() * widthFactor
            );
            draftGizmo->Gizmo::setDraggerPlacement(
                Base::convertTo<Base::Vector3d>(draftAnchor),
                Base::convertTo<Base::Vector3d>(pull)
            );
            draftGizmo->getDraggerContainer()->setArcNormalDirection(
                Base::convertTo<SbVec3f>(rotationAxis)
            );
            draftGizmo->automaticOrientation = false;
        }

        gizmoContainer->visible = true;
        gizmoContainer->calculateScaleAndOrientation();
    }
    catch (const Base::Exception&) {
    }
    catch (const Standard_Failure&) {
    }
}

TaskDlgRibParameters::TaskDlgRibParameters(ViewProviderPad* view)
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
