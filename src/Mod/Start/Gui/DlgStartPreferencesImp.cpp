/***************************************************************************
 *   Copyright (c) 2018 Yorik van Havre <yorik@uncreated.net>              *
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

#include "DlgStartPreferencesImp.h"
#include "ui_DlgStartPreferences.h"
#include <Gui/PrefWidgets.h>
#include <Base/Console.h>
#include <Gui/Application.h>

using namespace StartGui;

/**
 *  Constructs a DlgStartPreferencesImp which is a child of 'parent'
 */
DlgStartPreferencesImp::DlgStartPreferencesImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgStartPreferences)
{
    ui->setupUi(this);

    // Hide currently unused controls
    ui->label_12->hide();
    ui->label_7->hide();
    ui->colorButton_7->hide();
    ui->radioButton_1->hide();
    ui->radioButton_2->hide();

    // fills the combo box with all available workbenches
    // sorted by their menu text
    QStringList work = Gui::Application::Instance->workbenches();
    QMap<QString, QString> menuText;
    for (QStringList::Iterator it = work.begin(); it != work.end(); ++it) {
        QString text = Gui::Application::Instance->workbenchMenuText(*it);
        menuText[text] = *it;
    }

    {   // add special workbench to selection
        QPixmap px = Gui::Application::Instance->workbenchIcon(QString::fromLatin1("NoneWorkbench"));
        QString key = QString::fromLatin1("<last>");
        QString value = QString::fromLatin1("$LastModule");
        if (px.isNull())
            ui->AutoloadModuleCombo->addItem(key, QVariant(value));
        else
            ui->AutoloadModuleCombo->addItem(px, key, QVariant(value));
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Gui::Application::Instance->workbenchIcon(it.value());
        if (px.isNull())
            ui->AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        else
            ui->AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
    }

}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgStartPreferencesImp::~DlgStartPreferencesImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgStartPreferencesImp::saveSettings()
{
    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")->
                          SetASCII("AutoloadModule", startWbName.toLatin1());
    ui->colorButton_1->onSave();
    ui->colorButton_2->onSave();
    ui->colorButton_3->onSave();
    ui->colorButton_4->onSave();
    ui->colorButton_5->onSave();
    ui->colorButton_6->onSave();
    ui->colorButton_7->onSave();
    ui->fileChooser_1->onSave();
    ui->fileChooser_2->onSave();
    ui->fileChooser_3->onSave();
    ui->radioButton_1->onSave();
    ui->radioButton_2->onSave();
    ui->checkBox->onSave();
    ui->checkBox_1->onSave();
    ui->checkBox_2->onSave();
    ui->checkBox_3->onSave();
    ui->checkBox_4->onSave();
    ui->checkBox_5->onSave();
    ui->checkBox_6->onSave();
    ui->checkBox_7->onSave();
    ui->lineEdit->onSave();
    ui->spinBox->onSave();
}

void DlgStartPreferencesImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")->
                                  GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QLatin1String(start.c_str());
    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(startWbName));
    ui->colorButton_1->onRestore();
    ui->colorButton_2->onRestore();
    ui->colorButton_3->onRestore();
    ui->colorButton_4->onRestore();
    ui->colorButton_5->onRestore();
    ui->colorButton_6->onRestore();
    ui->colorButton_7->onRestore();
    ui->fileChooser_1->onRestore();
    ui->fileChooser_2->onRestore();
    ui->fileChooser_3->onRestore();
    ui->radioButton_1->onRestore();
    ui->radioButton_2->onRestore();
    ui->checkBox->onRestore();
    ui->checkBox_1->onRestore();
    ui->checkBox_2->onRestore();
    ui->checkBox_3->onRestore();
    ui->checkBox_4->onRestore();
    ui->checkBox_5->onRestore();
    ui->checkBox_6->onRestore();
    ui->checkBox_7->onRestore();
    ui->lineEdit->onRestore();
    ui->spinBox->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgStartPreferencesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgStartPreferencesImp.cpp"
