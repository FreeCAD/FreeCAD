// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2012 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <limits>
#include <QMessageBox>

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Base/Tools.h>
#include <Gui/Application.h>
#include <Gui/AsyncTaskRecompute.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Control.h>
#include <Gui/Document.h>
#include <Gui/Selection/Selection.h>
#include <Gui/Selection/SelectionFilter.h>
#include <Gui/Selection/SelectionObject.h>
#include <Gui/TaskView/TaskView.h>
#include <Gui/ViewProvider.h>
#include <Gui/Inventor/Draggers/Gizmo.h>
#include <Mod/Part/App/GizmoHelper.h>
#include <Mod/Part/App/PartFeatures.h>

#include "TaskThickness.h"
#include "ViewProvider.h"
#include "ui_TaskOffset.h"


using namespace PartGui;

class ThicknessWidget::Private
{
public:
    Ui_TaskOffset ui {};
    QString text;
    std::string selection;
    Part::Thickness* thickness {nullptr};
    std::unique_ptr<Gui::AsyncTaskRecompute> recompute;
    bool acceptRequested {false};
    bool rejectRequested {false};

    class FaceSelection: public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;

    public:
        explicit FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate(nullPointer())
            , object(obj)
        {}
        bool allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* sSubName) override
        {
            if (pObj != this->object) {
                return false;
            }
            if (Base::Tools::isNullOrEmpty(sSubName)) {
                return false;
            }
            std::string element(sSubName);
            return element.substr(0, 4) == "Face";
        }
    };
};

/* TRANSLATOR PartGui::ThicknessWidget */

ThicknessWidget::ThicknessWidget(Part::Thickness* thickness, QWidget* parent)
    : d(new Private())
{
    Q_UNUSED(parent);
    Gui::Command::runCommand(Gui::Command::App, "from FreeCAD import Base");
    Gui::Command::runCommand(Gui::Command::App, "import Part");

    d->thickness = thickness;
    d->recompute = std::make_unique<Gui::AsyncTaskRecompute>(this);
    d->recompute->setRunningChanged([this](bool running) { setEnabled(!running); });
    d->ui.setupUi(this);
    setupConnections();

    d->ui.labelOffset->setText(tr("Thickness"));
    d->ui.fillOffset->hide();

    QSignalBlocker blockOffset(d->ui.spinOffset);
    d->ui.spinOffset->setRange(-std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
    d->ui.spinOffset->setSingleStep(0.1);
    d->ui.spinOffset->setValue(d->thickness->Value.getValue());

    int mode = d->thickness->Mode.getValue();
    d->ui.modeType->setCurrentIndex(mode);

    int join = d->thickness->Join.getValue();
    d->ui.joinType->setCurrentIndex(join);

    QSignalBlocker blockIntSct(d->ui.intersection);
    bool intsct = d->thickness->Intersection.getValue();
    d->ui.intersection->setChecked(intsct);

    QSignalBlocker blockSelfInt(d->ui.selfIntersection);
    bool selfint = d->thickness->SelfIntersection.getValue();
    d->ui.selfIntersection->setChecked(selfint);

    d->ui.spinOffset->bind(d->thickness->Value);

    // The interactive gizmos are implemented for this operation just as a proof
    // of concept. And so, it is kept disabled until the other operations of the
    // Part workbench are covered.
    // setupGizmos();
}

ThicknessWidget::~ThicknessWidget()
{
    delete d;
    Gui::Selection().rmvSelectionGate();
}

void ThicknessWidget::setupConnections()
{
    // clang-format off
    connect(d->ui.spinOffset, qOverload<double>(&Gui::QuantitySpinBox::valueChanged),
            this, &ThicknessWidget::onSpinOffsetValueChanged);
    connect(d->ui.modeType, qOverload<int>(&QComboBox::activated),
            this, &ThicknessWidget::onModeTypeActivated);
    connect(d->ui.joinType, qOverload<int>(&QComboBox::activated),
            this, &ThicknessWidget::onJoinTypeActivated);
    connect(d->ui.intersection, &QCheckBox::toggled,
            this, &ThicknessWidget::onIntersectionToggled);
    connect(d->ui.selfIntersection, &QCheckBox::toggled,
            this, &ThicknessWidget::onSelfIntersectionToggled);
    connect(d->ui.facesButton, &QPushButton::toggled,
            this, &ThicknessWidget::onFacesButtonToggled);
    connect(d->ui.updateView, &QCheckBox::toggled,
            this, &ThicknessWidget::onUpdateViewToggled);
    // clang-format on
}

Part::Thickness* ThicknessWidget::getObject() const
{
    return d->thickness;
}

void ThicknessWidget::schedulePreview()
{
    if (!d->ui.updateView->isChecked()) {
        d->recompute->cancel();
        return;
    }

    auto request = App::RecomputeRequest::fromDocumentObject(*d->thickness, true);
    d->recompute->schedule(std::move(request), 150, [this](App::RecomputeResult&) {
        completeDeferredAction();
    });
}

void ThicknessWidget::applyUiState()
{
    if (!d->selection.empty()) {
        Gui::cmdAppObjectArgs(d->thickness, "Faces = %s", d->selection.c_str());
    }
    Gui::cmdAppObjectArgs(d->thickness, "Value = %f", d->ui.spinOffset->value().getValue());
    Gui::cmdAppObjectArgs(d->thickness, "Mode = %d", d->ui.modeType->currentIndex());
    Gui::cmdAppObjectArgs(d->thickness, "Join = %d", d->ui.joinType->currentIndex());
    Gui::cmdAppObjectArgs(
        d->thickness,
        "Intersection = %s",
        d->ui.intersection->isChecked() ? "True" : "False"
    );
    Gui::cmdAppObjectArgs(
        d->thickness,
        "SelfIntersection = %s",
        d->ui.selfIntersection->isChecked() ? "True" : "False"
    );
}

void ThicknessWidget::completeDeferredAction()
{
    if (d->rejectRequested) {
        if (d->recompute->isSettling()) {
            return;
        }

        d->rejectRequested = false;
        Gui::Control().reject(d->thickness->getDocument());
        return;
    }

    if (!d->acceptRequested || d->recompute->isSettling()) {
        return;
    }

    if (!d->recompute->hasCurrentSuccessfulPreview() || !d->thickness->isValid()) {
        d->acceptRequested = false;
        return;
    }

    d->acceptRequested = false;
    Gui::Control().accept(d->thickness->getDocument());
}

void ThicknessWidget::onSpinOffsetValueChanged(double val)
{
    d->thickness->Value.setValue(val);
    schedulePreview();
}

void ThicknessWidget::onModeTypeActivated(int val)
{
    d->thickness->Mode.setValue(val);
    schedulePreview();
}

void ThicknessWidget::onJoinTypeActivated(int val)
{
    d->thickness->Join.setValue((long)val);
    schedulePreview();
}

void ThicknessWidget::onIntersectionToggled(bool on)
{
    d->thickness->Intersection.setValue(on);
    schedulePreview();
}

void ThicknessWidget::onSelfIntersectionToggled(bool on)
{
    d->thickness->SelfIntersection.setValue(on);
    schedulePreview();
}

void ThicknessWidget::onFacesButtonToggled(bool on)
{
    if (on) {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c) {
            it->setEnabled(false);
        }
        d->ui.facesButton->setEnabled(true);
        d->ui.labelFaces->setText(tr("Select faces of the source object and press 'Done'"));
        d->ui.labelFaces->setEnabled(true);
        d->text = d->ui.facesButton->text();
        d->ui.facesButton->setText(tr("Done"));

        Gui::Application::Instance->showViewProvider(d->thickness->Faces.getValue());
        Gui::Application::Instance->hideViewProvider(d->thickness);
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new Private::FaceSelection(d->thickness->Faces.getValue()));

        if (gizmoContainer) {
            gizmoContainer->visible = false;
        }
    }
    else {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c) {
            it->setEnabled(true);
        }
        d->ui.facesButton->setText(d->text);
        d->ui.labelFaces->clear();

        d->selection = Gui::Command::getPythonTuple(
            d->thickness->Faces.getValue()->getNameInDocument(),
            d->thickness->Faces.getSubValues()
        );
        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx();
        for (auto& it : sel) {
            if (it.getObject() == d->thickness->Faces.getValue()) {
                d->thickness->Faces.setValue(it.getObject(), it.getSubNames());
                d->selection = it.getAsPropertyLinkSubString();
                break;
            }
        }

        Gui::Selection().rmvSelectionGate();
        Gui::Application::Instance->showViewProvider(d->thickness);
        Gui::Application::Instance->hideViewProvider(d->thickness->Faces.getValue());
        schedulePreview();

        if (gizmoContainer) {
            gizmoContainer->visible = true;
            setGizmoPositions();
        }
    }
}

void ThicknessWidget::onUpdateViewToggled(bool on)
{
    if (on) {
        schedulePreview();
    }
    else {
        d->recompute->cancel();
    }
}

bool ThicknessWidget::accept()
{
    if (d->ui.facesButton->isChecked()) {
        return false;
    }

    try {
        if (d->recompute->isSettling()) {
            d->acceptRequested = true;
            return false;
        }

        applyUiState();

        if (d->recompute->isPending()) {
            d->acceptRequested = true;
            d->recompute->flushPending();
            return false;
        }

        if (!d->recompute->hasCurrentSuccessfulPreview() || !d->thickness->isValid()) {
            d->acceptRequested = true;
            auto request = App::RecomputeRequest::fromDocument(*d->thickness->getDocument());
            d->recompute->schedule(std::move(request), 0, [this](App::RecomputeResult&) {
                completeDeferredAction();
            });
            return false;
        }

        // The expensive Thickness feature has already been computed by the
        // accepted preview. Clear its touched state and settle only its
        // dependents so accepting does not execute Thickness a second time.
        d->thickness->purgeTouched();
        if (!d->thickness->isValid()) {
            throw Base::CADKernelError(d->thickness->getStatusString());
        }
        for (auto* obj : d->thickness->getInList()) {
            obj->touch();
        }
        d->thickness->getDocument()->recompute();

        Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
        d->thickness->getDocument()->commitTransaction();  // Opened in
                                                           // ViewProviderDocumentObject::startDefaultEditMode()
    }
    catch (const Base::Exception& e) {
        d->thickness->getDocument()->abortTransaction();  // ViewProviderDocumentObject::startDefaultEditMode()
        QMessageBox::warning(
            this,
            tr("Input error"),
            QCoreApplication::translate("Exception", e.what())
        );
        return false;
    }

    return true;
}

bool ThicknessWidget::reject()
{
    if (d->ui.facesButton->isChecked()) {
        return false;
    }

    if (d->recompute->isSettling()) {
        d->rejectRequested = true;
        d->recompute->cancel();
        return false;
    }

    if (d->recompute->isPending()) {
        d->recompute->cancel();
    }

    // save this and check if the object is still there after the
    // transaction is aborted
    std::string objname = d->thickness->getNameInDocument();
    App::DocumentObject* source = d->thickness->Faces.getValue();

    // roll back the done things
    d->thickness->getDocument()->abortTransaction();  // ViewProviderDocumentObject::startDefaultEditMode()
    Gui::Command::doCommand(Gui::Command::Gui, "Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();

    // Thickness object was deleted
    if (source && !source->getDocument()->getObject(objname.c_str())) {
        Gui::Application::Instance->getViewProvider(source)->show();
    }

    return true;
}

void ThicknessWidget::changeEvent(QEvent* e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->ui.labelOffset->setText(tr("Thickness"));
    }
}

void ThicknessWidget::setupGizmos()
{
    if (!Gui::GizmoContainer::isEnabled()) {
        return;
    }

    linearGizmo = new Gui::LinearGizmo(d->ui.spinOffset);

    auto vp = Base::freecad_cast<ViewProviderPart*>(
        Gui::Application::Instance->getViewProvider(d->thickness)
    );
    if (!vp) {
        delete linearGizmo;
        return;
    }
    gizmoContainer = Gui::GizmoContainer::create({linearGizmo}, vp);

    setGizmoPositions();
}

void ThicknessWidget::setGizmoPositions()
{
    if (!gizmoContainer) {
        return;
    }

    Part::Thickness* thickness = getObject();
    auto base = Part::Thickness::getTopoShape(
        thickness->Faces.getValue(),
        Part::ShapeOption::ResolveLink | Part::ShapeOption::Transform
    );
    auto faces = thickness->Faces.getSubValues(true);

    if (faces.size() == 0) {
        gizmoContainer->visible = false;
    }

    Part::TopoShape face = base.getSubTopoShape(faces[0].c_str());
    if (face.getShape().ShapeType() == TopAbs_FACE) {
        auto edges = getAdjacentEdgesFromFace(face);
        assert(edges.size() != 0 && "A face without any edges? Please file a bug report");
        DraggerPlacementProps props = getDraggerPlacementFromEdgeAndFace(edges[0], face);

        // The part thickness operation by default goes creates towards outside
        // so -props.dir is taken
        linearGizmo->Gizmo::setDraggerPlacement(props.position, -props.dir);

        gizmoContainer->visible = true;

        return;
    }
}


/* TRANSLATOR PartGui::TaskThickness */

TaskThickness::TaskThickness(Part::Thickness* offset)
{
    widget = new ThicknessWidget(offset);
    widget->setWindowTitle(ThicknessWidget::tr("Thickness"));
    addTaskBox(Gui::BitmapFactory().pixmap("Part_Thickness"), widget);
}

Part::Thickness* TaskThickness::getObject() const
{
    return widget->getObject();
}

void TaskThickness::open()
{}

void TaskThickness::clicked(int)
{}

bool TaskThickness::accept()
{
    return widget->accept();
}

bool TaskThickness::reject()
{
    return widget->reject();
}

#include "moc_TaskThickness.cpp"
