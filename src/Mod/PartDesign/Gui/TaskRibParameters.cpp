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

}  // namespace PartDesignGui
