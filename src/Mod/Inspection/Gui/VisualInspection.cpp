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
#include "VisualInspection.h"
#include "ui_VisualInspection.h"

#include <Base/UnitsApi.h>
#include <App/Document.h>
#include <App/DocumentObject.h>
#include <App/Application.h>
#include <Gui/Document.h>
#include <Gui/ViewProvider.h>
#include <Gui/Application.h>
#include <Gui/MainWindow.h>
#include <Gui/PrefWidgets.h>

using namespace InspectionGui;

namespace InspectionGui {
class SingleSelectionItem : public QTreeWidgetItem
{
public:
    SingleSelectionItem (QTreeWidget* parent)
        : QTreeWidgetItem(parent), _compItem(0)
    {
    }

    SingleSelectionItem (QTreeWidgetItem* parent)
        : QTreeWidgetItem (parent), _compItem(0)
    {
    }

    ~SingleSelectionItem ()
    {
    }

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
}

/* TRANSLATOR InspectionGui::DlgVisualInspectionImp */

/**
 *  Constructs a VisualInspection as a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'.
 */
VisualInspection::VisualInspection(QWidget* parent, Qt::WFlags fl)
    : QDialog(parent, fl), ui(new Ui_VisualInspection)
{
    ui->setupUi(this);
    connect(ui->treeWidgetActual, SIGNAL(itemClicked(QTreeWidgetItem*, int)), 
            this, SLOT(onActivateItem(QTreeWidgetItem*)));
    connect(ui->treeWidgetNominal, SIGNAL(itemClicked(QTreeWidgetItem*, int)), 
            this, SLOT(onActivateItem(QTreeWidgetItem*)));

    //FIXME: Not used yet
    ui->textLabel2->hide();
    ui->prefFloatSpinBox2->hide();
    ui->prefFloatSpinBox1->setDecimals(Base::UnitsApi::getDecimals());
    ui->prefFloatSpinBox2->setDecimals(Base::UnitsApi::getDecimals());

    connect(ui->buttonHelp, SIGNAL(clicked()), Gui::getMainWindow(), SLOT(whatsThis()));

    App::Document* doc = App::GetApplication().getActiveDocument();
    // disable Ok button and enable of at least one item in each view is on
    ui->buttonOk->setDisabled(true);

    if (!doc) {
        ui->treeWidgetActual->setDisabled(true);
        ui->treeWidgetNominal->setDisabled(true);
        return;
    }

    Gui::Document* gui = Gui::Application::Instance->getDocument(doc);

    std::vector<App::DocumentObject*> obj = doc->getObjects();
    Base::Type point = Base::Type::fromName("Points::Feature");
    Base::Type mesh  = Base::Type::fromName("Mesh::Feature");
    Base::Type shape = Base::Type::fromName("Part::Feature");
    for (std::vector<App::DocumentObject*>::iterator it = obj.begin(); it != obj.end(); ++it) {
        if ((*it)->getTypeId().isDerivedFrom(point) ||
            (*it)->getTypeId().isDerivedFrom(mesh)  ||
            (*it)->getTypeId().isDerivedFrom(shape)) {
            Gui::ViewProvider* view = gui->getViewProvider(*it);
            QIcon px = view->getIcon();
            SingleSelectionItem* item1 = new SingleSelectionItem(ui->treeWidgetActual);
            item1->setText(0, QString::fromUtf8((*it)->Label.getValue()));
            item1->setData(0, Qt::UserRole, QString::fromAscii((*it)->getNameInDocument()));
            item1->setCheckState(0, Qt::Unchecked);
            item1->setIcon(0, px);

            SingleSelectionItem* item2 = new SingleSelectionItem(ui->treeWidgetNominal);
            item2->setText(0, QString::fromUtf8((*it)->Label.getValue()));
            item2->setData(0, Qt::UserRole, QString::fromAscii((*it)->getNameInDocument()));
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
    ui->prefFloatSpinBox1->onRestore();
    ui->prefFloatSpinBox2->onRestore();
}

void VisualInspection::saveSettings()
{
    ui->prefFloatSpinBox1->onSave();
    ui->prefFloatSpinBox2->onSave();
}

void VisualInspection::onActivateItem(QTreeWidgetItem* item)
{
    if (item) {
        SingleSelectionItem* sel = (SingleSelectionItem*)item;
        SingleSelectionItem* cmp = sel->getCompetitiveItem();
        if (cmp && cmp->checkState(0) == Qt::Checked)
            cmp->setCheckState(0, Qt::Unchecked);
    }

    bool ok=false;
    for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; ++it) {
        SingleSelectionItem* sel = (SingleSelectionItem*)*it;
        if (sel->checkState(0) == Qt::Checked) {
            ok = true;
            break;
        }
    }

    if (ok) {
        ok = false;
        for (QTreeWidgetItemIterator it (ui->treeWidgetNominal); *it; ++it) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                ok = true;
                break;
            }
        }
    }

    ui->buttonOk->setEnabled(ok);
}

void VisualInspection::accept()
{
    onActivateItem(0);
    if (ui->buttonOk->isEnabled()) {
        QDialog::accept();
        saveSettings();

        // collect all nominal geometries
        QStringList nominalNames;
        for (QTreeWidgetItemIterator it(ui->treeWidgetNominal); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked)
                nominalNames << sel->data(0, Qt::UserRole).toString();
        }

        float searchRadius = ui->prefFloatSpinBox1->value();
        float thickness = ui->prefFloatSpinBox2->value();

        // open a new command
        Gui::Document* doc = Gui::Application::Instance->activeDocument();
        doc->openCommand("Visual Inspection");

        // create a group
        Gui::Application::Instance->runCommand(
            true, "App_activeDocument___InspectionGroup=App.ActiveDocument.addObject(\"Inspection::Group\",\"Inspection\")");
    
        // for each actual geometry create an inspection feature
        for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                QString actualName = sel->data(0, Qt::UserRole).toString();
                Gui::Application::Instance->runCommand(
                    true, "App_activeDocument___InspectionGroup.newObject(\"Inspection::Feature\",\"%s_Inspect\")", (const char*)actualName.toAscii());
                Gui::Application::Instance->runCommand(
                    true, "App.ActiveDocument.ActiveObject.Actual=App.ActiveDocument.%s\n"
                          "App_activeDocument___activeObject___Nominals=list()\n"
                          "App.ActiveDocument.ActiveObject.SearchRadius=%.3f\n"
                          "App.ActiveDocument.ActiveObject.Thickness=%.3f\n", (const char*)actualName.toAscii(), searchRadius, thickness);
                for (QStringList::Iterator it = nominalNames.begin(); it != nominalNames.end(); ++it) {
                    Gui::Application::Instance->runCommand(
                        true, "App_activeDocument___activeObject___Nominals.append(App.ActiveDocument.%s)\n", (const char*)(*it).toAscii());
                }
                Gui::Application::Instance->runCommand(
                    true, "App.ActiveDocument.ActiveObject.Nominals=App_activeDocument___activeObject___Nominals\n"
                          "del App_activeDocument___activeObject___Nominals\n");
            }
        }

        Gui::Application::Instance->runCommand(
            true, "del App_activeDocument___InspectionGroup\n");

        doc->commitCommand();
        doc->getDocument()->recompute();

        // hide the checked features
        for (QTreeWidgetItemIterator it(ui->treeWidgetActual); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                Gui::Application::Instance->runCommand(
                    true, "Gui.ActiveDocument.getObject(\"%s\").Visibility=False"
                        , (const char*)sel->data(0, Qt::UserRole).toString().toAscii());
            }
        }

        for (QTreeWidgetItemIterator it(ui->treeWidgetNominal); *it; it++) {
            SingleSelectionItem* sel = (SingleSelectionItem*)*it;
            if (sel->checkState(0) == Qt::Checked) {
                Gui::Application::Instance->runCommand(
                    true, "Gui.ActiveDocument.getObject(\"%s\").Visibility=False"
                        , (const char*)sel->data(0, Qt::UserRole).toString().toAscii());
            }
        }
    }
}

#include "moc_VisualInspection.cpp"
