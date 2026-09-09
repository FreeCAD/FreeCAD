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

}  // namespace PartDesignGui
