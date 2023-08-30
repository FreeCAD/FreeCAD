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

#include "PreCompiled.h"
#ifndef _PreComp_
# include <QMessageBox>
#endif

#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/CommandT.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/SelectionObject.h>
#include <Gui/ViewProvider.h>
#include <Mod/Part/App/PartFeatures.h>

#include "TaskThickness.h"
#include "ui_TaskOffset.h"


using namespace PartGui;

class ThicknessWidget::Private
{
public:
    Ui_TaskOffset ui{};
    QString text;
    std::string selection;
    Part::Thickness* thickness{nullptr};

    class FaceSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        explicit FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate(nullPointer()), object(obj)
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName) override
        {
            if (pObj != this->object)
                return false;
            if (!sSubName || sSubName[0] == '\0')
                return false;
            std::string element(sSubName);
            return element.substr(0,4) == "Face";
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
    d->ui.setupUi(this);
    setupConnections();

    d->ui.labelOffset->setText(tr("Thickness"));
    d->ui.fillOffset->hide();

    QSignalBlocker blockOffset(d->ui.spinOffset);
    d->ui.spinOffset->setRange(-INT_MAX, INT_MAX);
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
}

ThicknessWidget::~ThicknessWidget()
{
    delete d;
    Gui::Selection().rmvSelectionGate();
}

void ThicknessWidget::setupConnections()
{
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
}

Part::Thickness* ThicknessWidget::getObject() const
{
    return d->thickness;
}

void ThicknessWidget::onSpinOffsetValueChanged(double val)
{
    d->thickness->Value.setValue(val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::onModeTypeActivated(int val)
{
    d->thickness->Mode.setValue(val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::onJoinTypeActivated(int val)
{
    d->thickness->Join.setValue((long)val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::onIntersectionToggled(bool on)
{
    d->thickness->Intersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::onSelfIntersectionToggled(bool on)
{
    d->thickness->SelfIntersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::onFacesButtonToggled(bool on)
{
    if (on) {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c)
            it->setEnabled(false);
        d->ui.facesButton->setEnabled(true);
        d->ui.labelFaces->setText(tr("Select faces of the source object and press 'Done'"));
        d->ui.labelFaces->setEnabled(true);
        d->text = d->ui.facesButton->text();
        d->ui.facesButton->setText(tr("Done"));

        Gui::Application::Instance->showViewProvider(d->thickness->Faces.getValue());
        Gui::Application::Instance->hideViewProvider(d->thickness);
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new Private::FaceSelection(d->thickness->Faces.getValue()));
    }
    else {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (auto it : c)
            it->setEnabled(true);
        d->ui.facesButton->setText(d->text);
        d->ui.labelFaces->clear();

        d->selection = Gui::Command::getPythonTuple
            (d->thickness->Faces.getValue()->getNameInDocument(), d->thickness->Faces.getSubValues());
        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx();
        for (auto & it : sel) {
            if (it.getObject() == d->thickness->Faces.getValue()) {
                d->thickness->Faces.setValue(it.getObject(), it.getSubNames());
                d->selection = it.getAsPropertyLinkSubString();
                break;
            }
        }

        Gui::Selection().rmvSelectionGate();
        Gui::Application::Instance->showViewProvider(d->thickness);
        Gui::Application::Instance->hideViewProvider(d->thickness->Faces.getValue());
        if (d->ui.updateView->isChecked())
            d->thickness->getDocument()->recomputeFeature(d->thickness);
    }
}

void ThicknessWidget::onUpdateViewToggled(bool on)
{
    if (on) {
        d->thickness->getDocument()->recomputeFeature(d->thickness);
    }
}

bool ThicknessWidget::accept()
{
    if (d->ui.facesButton->isChecked())
        return false;

    try {
        if (!d->selection.empty()) {
            Gui::cmdAppObjectArgs(d->thickness, "Faces = %s", d->selection.c_str());
        }
        Gui::cmdAppObjectArgs(d->thickness, "Value = %f", d->ui.spinOffset->value().getValue());
        Gui::cmdAppObjectArgs(d->thickness, "Mode = %d", d->ui.modeType->currentIndex());
        Gui::cmdAppObjectArgs(d->thickness, "Join = %d", d->ui.joinType->currentIndex());
        Gui::cmdAppObjectArgs(d->thickness, "Intersection = %s",
            d->ui.intersection->isChecked() ? "True" : "False");
        Gui::cmdAppObjectArgs(d->thickness, "SelfIntersection = %s",
            d->ui.selfIntersection->isChecked() ? "True" : "False");

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!d->thickness->isValid())
            throw Base::CADKernelError(d->thickness->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(
            this, tr("Input error"), QCoreApplication::translate("Exception", e.what()));
        return false;
    }

    return true;
}

bool ThicknessWidget::reject()
{
    if (d->ui.facesButton->isChecked())
        return false;

    // save this and check if the object is still there after the
    // transaction is aborted
    std::string objname = d->thickness->getNameInDocument();
    App::DocumentObject* source = d->thickness->Faces.getValue();

    // roll back the done things
    Gui::Command::abortCommand();
    Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
    Gui::Command::updateActive();

    // Thickness object was deleted
    if (source && !source->getDocument()->getObject(objname.c_str())) {
        Gui::Application::Instance->getViewProvider(source)->show();
    }

    return true;
}

void ThicknessWidget::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    if (e->type() == QEvent::LanguageChange) {
        d->ui.retranslateUi(this);
        d->ui.labelOffset->setText(tr("Thickness"));
    }
}


/* TRANSLATOR PartGui::TaskThickness */

TaskThickness::TaskThickness(Part::Thickness* offset)
{
    widget = new ThicknessWidget(offset);
    widget->setWindowTitle(ThicknessWidget::tr("Thickness"));
    taskbox = new Gui::TaskView::TaskBox(
        Gui::BitmapFactory().pixmap("Part_Thickness"),
        widget->windowTitle(), true, nullptr);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

Part::Thickness* TaskThickness::getObject() const
{
    return widget->getObject();
}

void TaskThickness::open()
{
}

void TaskThickness::clicked(int)
{
}

bool TaskThickness::accept()
{
    return widget->accept();
}

bool TaskThickness::reject()
{
    return widget->reject();
}

#include "moc_TaskThickness.cpp"
