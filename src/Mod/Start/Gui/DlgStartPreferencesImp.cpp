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

#include <Gui/Application.h>

#include "DlgStartPreferencesImp.h"
#include "ui_DlgStartPreferences.h"
#include "ui_DlgStartPreferencesAdvanced.h"


using namespace StartGui;

/**
 *  Constructs a DlgStartPreferencesImp which is a child of 'parent'
 */
DlgStartPreferencesImp::DlgStartPreferencesImp(QWidget* parent)
    : PreferencePage(parent)
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
    for (const auto& it : work) {
        QString text = Gui::Application::Instance->workbenchMenuText(it);
        menuText[text] = it;
    }

    // add special workbench to selection
    QPixmap px = Gui::Application::Instance->workbenchIcon(QString::fromLatin1("NoneWorkbench"));
    QString key = QString::fromLatin1("<last>");
    QString value = QString::fromLatin1("$LastModule");
    if (px.isNull()) {
        ui->AutoloadModuleCombo->addItem(key, QVariant(value));
    }
    else {
        ui->AutoloadModuleCombo->addItem(px, key, QVariant(value));
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Gui::Application::Instance->workbenchIcon(it.value());
        if (px.isNull()) {
            ui->AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        }
        else {
            ui->AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
        }
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgStartPreferencesImp::~DlgStartPreferencesImp() = default;

void DlgStartPreferencesImp::saveSettings()
{
    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")
        ->SetASCII("AutoloadModule", startWbName.toLatin1());
    ui->backgroundColorButton->onSave();
    ui->backgroundTextColorButton->onSave();
    ui->pageBackgroundColorButton->onSave();
    ui->pageTextColorButton->onSave();
    ui->boxBackgroundColorButton->onSave();
    ui->linkColorButton->onSave();
    ui->colorButton_7->onSave();
    ui->backgroundImageFileChooser->onSave();
    ui->showAdditionalFolderFileChooser->onSave();
    ui->radioButton_1->onSave();
    ui->radioButton_2->onSave();
    ui->showNotepadCheckBox->onSave();
    ui->showExamplesCheckBox->onSave();
    ui->closeStartCheckBox->onSave();
    ui->closeAndSwitchCheckBox->onSave();
    ui->showForumCheckBox->onSave();
    ui->useStyleSheetCheckBox->onSave();
    ui->showTipsCheckBox->onSave();
    ui->fontLineEdit->onSave();
    ui->fontSizeSpinBox->onSave();
    ui->showFileThumbnailIconsCheckBox->onSave();
    ui->fileThumbnailIconSizeSpinBox->onSave();
}

void DlgStartPreferencesImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication()
                .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")
                ->GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QLatin1String(start.c_str());
    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(startWbName));
    ui->backgroundColorButton->onRestore();
    ui->backgroundTextColorButton->onRestore();
    ui->pageBackgroundColorButton->onRestore();
    ui->pageTextColorButton->onRestore();
    ui->boxBackgroundColorButton->onRestore();
    ui->linkColorButton->onRestore();
    ui->colorButton_7->onRestore();
    ui->backgroundImageFileChooser->onRestore();
    ui->showAdditionalFolderFileChooser->onRestore();
    ui->radioButton_1->onRestore();
    ui->radioButton_2->onRestore();
    ui->showNotepadCheckBox->onRestore();
    ui->showExamplesCheckBox->onRestore();
    ui->closeStartCheckBox->onRestore();
    ui->closeAndSwitchCheckBox->onRestore();
    ui->showForumCheckBox->onRestore();
    ui->useStyleSheetCheckBox->onRestore();
    ui->showTipsCheckBox->onRestore();
    ui->fontLineEdit->onRestore();
    ui->fontSizeSpinBox->onRestore();
    ui->showFileThumbnailIconsCheckBox->onRestore();
    ui->fileThumbnailIconSizeSpinBox->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgStartPreferencesImp::changeEvent(QEvent* ev)
{
    if (ev->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        Gui::Dialog::PreferencePage::changeEvent(ev);
    }
}


/**
 *  Constructs a DlgStartPreferencesAdvancedImp which is a child of 'parent'
 */
DlgStartPreferencesAdvancedImp::DlgStartPreferencesAdvancedImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgStartPreferencesAdvanced)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgStartPreferencesAdvancedImp::~DlgStartPreferencesAdvancedImp() = default;

void DlgStartPreferencesAdvancedImp::saveSettings()
{
    ui->templateFileChooser->onSave();
    ui->customCSSTextEdit->onSave();
}

void DlgStartPreferencesAdvancedImp::loadSettings()
{
    ui->templateFileChooser->onRestore();
    ui->customCSSTextEdit->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgStartPreferencesAdvancedImp::changeEvent(QEvent* ev)
{
    if (ev->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        Gui::Dialog::PreferencePage::changeEvent(ev);
    }
}

#include "moc_DlgStartPreferencesImp.cpp"
