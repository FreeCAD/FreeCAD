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
# include <QEventLoop>
# include <QMessageBox>
# include <QTextStream>
#endif

#include "ui_TaskOffset.h"
#include "TaskThickness.h"

#include <Gui/Application.h>
#include <Gui/BitmapFactory.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/Selection.h>
#include <Gui/SelectionFilter.h>
#include <Gui/ViewProvider.h>

#include <Base/Console.h>
#include <Base/Interpreter.h>
#include <Base/Tools.h>
#include <App/Application.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Command.h>
#include <Mod/Part/App/PartFeatures.h>


using namespace PartGui;

class ThicknessWidget::Private
{
public:
    Ui_TaskOffset ui;
    QEventLoop loop;
    QString text;
    std::string selection;
    Part::Thickness* thickness;
    Private() : thickness(nullptr)
    {
    }
    ~Private()
    {
    }

    class FaceSelection : public Gui::SelectionFilterGate
    {
        const App::DocumentObject* object;
    public:
        FaceSelection(const App::DocumentObject* obj)
            : Gui::SelectionFilterGate((Gui::SelectionFilter*)0), object(obj)
        {
        }
        bool allow(App::Document* /*pDoc*/, App::DocumentObject*pObj, const char*sSubName)
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
}

Part::Thickness* ThicknessWidget::getObject() const
{
    return d->thickness;
}

void ThicknessWidget::on_spinOffset_valueChanged(double val)
{
    d->thickness->Value.setValue(val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::on_modeType_activated(int val)
{
    d->thickness->Mode.setValue(val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::on_joinType_activated(int val)
{
    d->thickness->Join.setValue((long)val);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::on_intersection_toggled(bool on)
{
    d->thickness->Intersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::on_selfIntersection_toggled(bool on)
{
    d->thickness->SelfIntersection.setValue(on);
    if (d->ui.updateView->isChecked())
        d->thickness->getDocument()->recomputeFeature(d->thickness);
}

void ThicknessWidget::on_facesButton_clicked()
{
    if (!d->loop.isRunning()) {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (QList<QWidget*>::iterator it = c.begin(); it != c.end(); ++it)
            (*it)->setEnabled(false);
        d->ui.facesButton->setEnabled(true);
        d->ui.labelFaces->setText(tr("Select faces of the source object and press 'Done'"));
        d->ui.labelFaces->setEnabled(true);
        d->text = d->ui.facesButton->text();
        d->ui.facesButton->setText(tr("Done"));

        Gui::Application::Instance->showViewProvider(d->thickness->Faces.getValue());
        Gui::Application::Instance->hideViewProvider(d->thickness);
        Gui::Selection().clearSelection();
        Gui::Selection().addSelectionGate(new Private::FaceSelection(d->thickness->Faces.getValue()));
        d->loop.exec();
    }
    else {
        QList<QWidget*> c = this->findChildren<QWidget*>();
        for (QList<QWidget*>::iterator it = c.begin(); it != c.end(); ++it)
            (*it)->setEnabled(true);
        d->ui.facesButton->setText(d->text);
        d->ui.labelFaces->clear();
        d->loop.quit();

        d->selection = Gui::Command::getPythonTuple
            (d->thickness->Faces.getValue()->getNameInDocument(), d->thickness->Faces.getSubValues());
        std::vector<Gui::SelectionObject> sel = Gui::Selection().getSelectionEx();
        for (std::vector<Gui::SelectionObject>::iterator it = sel.begin(); it != sel.end(); ++it) {
            if (it->getObject() == d->thickness->Faces.getValue()) {
                d->thickness->Faces.setValue(it->getObject(), it->getSubNames());
                d->selection = it->getAsPropertyLinkSubString();
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

void ThicknessWidget::on_updateView_toggled(bool on)
{
    if (on) {
        d->thickness->getDocument()->recomputeFeature(d->thickness);
    }
}

bool ThicknessWidget::accept()
{
    if (d->loop.isRunning())
        return false;

    std::string name = d->thickness->getNameInDocument();
    try {
        if (!d->selection.empty()) {
            Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Faces = %s",
                name.c_str(),d->selection.c_str());
        }
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Value = %f",
            name.c_str(),d->ui.spinOffset->value().getValue());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Mode = %i",
            name.c_str(),d->ui.modeType->currentIndex());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Join = %i",
            name.c_str(),d->ui.joinType->currentIndex());
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.Intersection = %s",
            name.c_str(),d->ui.intersection->isChecked() ? "True" : "False");
        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.SelfIntersection = %s",
            name.c_str(),d->ui.selfIntersection->isChecked() ? "True" : "False");

        Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.recompute()");
        if (!d->thickness->isValid())
            throw Base::CADKernelError(d->thickness->getStatusString());
        Gui::Command::doCommand(Gui::Command::Gui,"Gui.ActiveDocument.resetEdit()");
        Gui::Command::commitCommand();
    }
    catch (const Base::Exception& e) {
        QMessageBox::warning(this, tr("Input error"), QString::fromLatin1(e.what()));
        return false;
    }

    return true;
}

bool ThicknessWidget::reject()
{
    if (d->loop.isRunning())
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
        widget->windowTitle(), true, 0);
    taskbox->groupLayout()->addWidget(widget);
    Content.push_back(taskbox);
}

TaskThickness::~TaskThickness()
{
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
