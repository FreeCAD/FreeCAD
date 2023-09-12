/***************************************************************************
 *   Copyright (c) 2011 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <cfloat>
#endif

#include <App/Document.h>
#include <App/DocumentObject.h>
#include <Gui/Application.h>
#include <Gui/Command.h>
#include <Gui/Document.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>
#include <Gui/ViewProvider.h>

#include "VisualInspection.h"
#include "ui_VisualInspection.h"


using namespace InspectionGui;

namespace InspectionGui
{
class SingleSelectionItem: public QTreeWidgetItem
{
public:
    explicit SingleSelectionItem(QTreeWidget* parent)
        : QTreeWidgetItem(parent)
        , _compItem(nullptr)
    {}

    explicit SingleSelectionItem(QTreeWidgetItem* parent)
        : QTreeWidgetItem(parent)
        , _compItem(nullptr)
    {}

    ~SingleSelectionItem() override = default;

    SingleSelectionItem* getCompetitiveItem() const
    {
        return _compItem;
    }

    void setCompetitiveItem(SingleSelectionItem* item)
    {
        _compItem = item;
    }

private:
    SingleSelectionItem* _compItem;
};
}  // namespace InspectionGui

/* TRANSLATOR InspectionGui::DlgVisualInspectionImp */

/**
 *  Constructs a VisualInspection as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
VisualInspection::VisualInspection(QWidget* parent, Qt::WindowFlags fl)
    : QDialog(parent, fl)
    , ui(new Ui_VisualInspection)
{
    ui->setupUi(this);
    connect(ui->treeWidgetActual,
            &QTreeWidget::itemClicked,
            this,
            &VisualInspection::onActivateItem);
    connect(ui->treeWidgetNominal,
            &QTreeWidget::itemClicked,
            this,
            &VisualInspection::onActivateItem);
    connect(ui->buttonBox,
            &QDialogButtonBox::helpRequested,
            Gui::getMainWindow(),
            &Gui::MainWindow::whatsThis);

    // FIXME: Not used yet
    ui->textLabel2->hide();
    ui->thickness->hide();
    ui->searchRadius->setUnit(Base::Unit::Length);
    ui->searchRadius->setRange(0, DBL_MAX);
    ui->thickness->setUnit(Base::Unit::Length);
    ui->thickness->setRange(0, DBL_MAX);

    App::Document* doc = App::GetApplication().getActiveDocument();
    // disable Ok button and enable of at least one item in each view is on
    buttonOk = ui->buttonBox->button(QDialogButtonBox::Ok);
    buttonOk->setDisabled(true);

    if (!doc) {
        ui->treeWidgetActual->setDisabled(true);
        ui->treeWidgetNominal->setDisabled(true);
        return;
    }

    Gui::Document* gui = Gui::Application::Instance->getDocument(doc);

    std::vector<App::DocumentObject*> obj = doc->getObjects();
    Base::Type point = Base::Type::fromName("Points::Feature");
    Base::Type mesh = Base::Type::fromName("Mesh::Feature");
    Base::Type shape = Base::Type::fromName("Part::Feature");
    for (auto it : obj) {
        if (it->getTypeId().isDerivedFrom(point) || it->getTypeId().isDerivedFrom(mesh)
            || it->getTypeId().isDerivedFrom(shape)) {
            Gui::ViewProvider* view = gui->getViewProvider(it);
            QIcon px = view->getIcon();
            SingleSelectionItem* item1 = new SingleSelectionItem(ui->treeWidgetActual);
            item1->setText(0, QString::fromUtf8(it->Label.getValue()));
            item1->setData(0, Qt::UserRole, QString::fromLatin1(it->getNameInDocument()));
            item1->setCheckState(0, Qt::Unchecked);
            item1->setIcon(0, px);

            SingleSelectionItem* item2 = new SingleSelectionItem(ui->treeWidgetNominal);
            item2->setText(0, QString::fromUtf8(it->Label.getValue()));
            item2->setData(0, Qt::UserRole, QString::fromLatin1(it->getNameInDocument()));
            item2->setCheckState(0, Qt::Unchecked);
            item2->setIcon(0, px);

            item1->setCompetitiveItem(item2);
            item2->setCompetitiveItem(item1);
        }
    }

    loadSettings();
}

/*
 *  Destroys the object and frees any allocated resources
 */
VisualInspection::~VisualInspection()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void VisualInspection::loadSettings()
{
    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Inspection/Inspection");

    double searchDistance = ui->searchRadius->value().getValue();
    searchDistance = handle->GetFloat("SearchDistance", searchDistance);
    ui->searchRadius->setValue(searchDistance);

    double thickness = ui->thickness->value().getValue();
    thickness = handle->GetFloat("Thickness", thickness);
    ui->thickness->setValue(thickness);
}

void VisualInspection::saveSettings()
{
    ParameterGrp::handle handle = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Inspection/Inspection");
    double searchDistance = ui->searchRadius->value().getValue();
    handle->SetFloat("SearchDistance", searchDistance);

    double thickness = ui->thickness->value().getValue();
    handle->SetFloat("Thickness", thickness);
}

void VisualInspection::onActivateItem(QTreeWidgetItem* item)
{
    if (item) {
        SingleSelectionItem* sel = static_cast<SingleSelectionItem*>(item);
        SingleSelectionItem* cmp = sel->getCompetitiveItem();
        if (cmp && cmp->checkState(0) == Qt::Checked) {
            cmp->setCheckState(0, Qt::Unchecked);
        }
    }

    bool ok = false;
    for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; ++it) {
        SingleSelectionItem* sel = (SingleSelectionItem*)*it;
        if (sel->checkState(0) == Qt::Checked) {
            ok = true;
            break;
        }
    }

    if (ok) {
        ok = false;
        for (QTreeWidgetItemIterator it(ui->treeWidgetNominal); *it; ++it) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                ok = true;
                break;
            }
        }
    }

    buttonOk->setEnabled(ok);
}

void VisualInspection::accept()
{
    onActivateItem(nullptr);
    if (buttonOk->isEnabled()) {
        QDialog::accept();
        saveSettings();

        // collect all nominal geometries
        QStringList nominalNames;
        for (QTreeWidgetItemIterator it(ui->treeWidgetNominal); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                nominalNames << sel->data(0, Qt::UserRole).toString();
            }
        }

        double searchRadius = ui->searchRadius->value().getValue();
        double thickness = ui->thickness->value().getValue();

        // open a new command
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        doc->openCommand(QT_TRANSLATE_NOOP("Command", "Visual Inspection"));

        // create a group
        Gui::Command::runCommand(Gui::Command::App,
                                 "App_activeDocument___InspectionGroup=App.ActiveDocument."
                                 "addObject(\"Inspection::Group\",\"Inspection\")");

        // for each actual geometry create an inspection feature
        for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                QString actualName = sel->data(0, Qt::UserRole).toString();
                Gui::Command::doCommand(Gui::Command::App,
                                        "App_activeDocument___InspectionGroup.newObject("
                                        "\"Inspection::Feature\",\"%s_Inspect\")",
                                        (const char*)actualName.toLatin1());
                Gui::Command::doCommand(
                    Gui::Command::App,
                    "App.ActiveDocument.ActiveObject.Actual=App.ActiveDocument.%s\n"
                    "App_activeDocument___activeObject___Nominals=list()\n"
                    "App.ActiveDocument.ActiveObject.SearchRadius=%.3f\n"
                    "App.ActiveDocument.ActiveObject.Thickness=%.3f\n",
                    (const char*)actualName.toLatin1(),
                    searchRadius,
                    thickness);
                for (const auto& it : nominalNames) {
                    Gui::Command::doCommand(Gui::Command::App,
                                            "App_activeDocument___activeObject___Nominals.append("
                                            "App.ActiveDocument.%s)\n",
                                            (const char*)it.toLatin1());
                }
                Gui::Command::doCommand(Gui::Command::App,
                                        "App.ActiveDocument.ActiveObject.Nominals=App_"
                                        "activeDocument___activeObject___Nominals\n"
                                        "del App_activeDocument___activeObject___Nominals\n");
            }
        }

        Gui::Command::runCommand(Gui::Command::App, "del App_activeDocument___InspectionGroup\n");

        doc->commitCommand();
        doc->getDocument()->recompute();

        // hide the checked features
        for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                Gui::Command::doCommand(
                    Gui::Command::App,
                    "Gui.ActiveDocument.getObject(\"%s\").Visibility=False",
                    (const char*)sel->data(0, Qt::UserRole).toString().toLatin1());
            }
        }

        for (QTreeWidgetItemIterator it(ui->treeWidgetNominal); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                Gui::Command::doCommand(
                    Gui::Command::App,
                    "Gui.ActiveDocument.getObject(\"%s\").Visibility=False",
                    (const char*)sel->data(0, Qt::UserRole).toString().toLatin1());
            }
        }
    }
}

#include "moc_VisualInspection.cpp"
