// SPDX-License-Identifier: LGPL-2.1-or-later

 /****************************************************************************
  *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
  *   Copyright (c) 2023 FreeCAD Project Association                         *
  *                                                                          *
  *   This file is part of FreeCAD.                                          *
  *                                                                          *
  *   FreeCAD is free software: you can redistribute it and/or modify it     *
  *   under the terms of the GNU Lesser General Public License as            *
  *   published by the Free Software Foundation, either version 2.1 of the   *
  *   License, or (at your option) any later version.                        *
  *                                                                          *
  *   FreeCAD is distributed in the hope that it will be useful, but         *
  *   WITHOUT ANY WARRANTY; without even the implied warranty of             *
  *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU       *
  *   Lesser General Public License for more details.                        *
  *                                                                          *
  *   You should have received a copy of the GNU Lesser General Public       *
  *   License along with FreeCAD. If not, see                                *
  *   <https://www.gnu.org/licenses/>.                                       *
  *                                                                          *
  ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cmath>
# include <limits>
# include <QApplication>
# include <QFileDialog>
# include <QLocale>
# include <QMessageBox>
# include <algorithm>
# include <boost/filesystem.hpp>
#endif

#include <Base/Parameter.h>
#include <Base/UnitsApi.h>

#include <Gui/Document.h>

#include <Gui/Action.h>
#include <Gui/Application.h>
#include <Gui/DlgCreateNewPreferencePackImp.h>
#include <Gui/DlgPreferencesImp.h>
#include <Gui/DlgPreferencePackManagementImp.h>
#include <Gui/DlgRevertToBackupConfigImp.h>
#include <Gui/MainWindow.h>
#include <Gui/PreferencePackManager.h>
#include <Gui/Language/Translator.h>

#include "DlgSettingsGeneral.h"
#include "ui_DlgSettingsGeneral.h"

using namespace Gui::Dialog;
namespace fs = boost::filesystem;
using namespace Base;

/* TRANSLATOR Gui::Dialog::DlgSettingsGeneral */

/**
 *  Constructs a DlgSettingsGeneral which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgSettingsGeneral::DlgSettingsGeneral( QWidget* parent )
  : PreferencePage(parent)
  , localeIndex(0)
  , themeChanged(false)
  , ui(new Ui_DlgSettingsGeneral)
{
    ui->setupUi(this);

    recreatePreferencePackMenu();

    connect(ui->ImportConfig, &QPushButton::clicked, this, &DlgSettingsGeneral::onImportConfigClicked);
    connect(ui->SaveNewPreferencePack, &QPushButton::clicked, this, &DlgSettingsGeneral::saveAsNewPreferencePack);
    connect(ui->themesCombobox, qOverload<int>(&QComboBox::activated), this, &DlgSettingsGeneral::onThemeChanged);

    ui->ManagePreferencePacks->setToolTip(tr("Manage preference packs"));
    connect(ui->ManagePreferencePacks, &QPushButton::clicked, this, &DlgSettingsGeneral::onManagePreferencePacksClicked);

    // If there are any saved config file backs, show the revert button, otherwise hide it:
    const auto & backups = Application::Instance->prefPackManager()->configBackups();
    ui->RevertToSavedConfig->setEnabled(backups.empty());
    connect(ui->RevertToSavedConfig, &QPushButton::clicked, this, &DlgSettingsGeneral::revertToSavedConfig);

    connect(ui->comboBox_UnitSystem, qOverload<int>(&QComboBox::currentIndexChanged), this, &DlgSettingsGeneral::onUnitSystemIndexChanged);
    ui->spinBoxDecimals->setMaximum(std::numeric_limits<double>::digits10 + 1);

    int num = static_cast<int>(Base::UnitSystem::NumUnitSystemTypes);
    for (int i = 0; i < num; i++) {
        QString item = Base::UnitsApi::getDescription(static_cast<Base::UnitSystem>(i));
        ui->comboBox_UnitSystem->addItem(item, i);
        ui->comboBox_projectUnitSystem->addItem(item, i);
    }

    // Enable/disable the fractional inch option depending on system
    if (UnitsApi::getSchema() == UnitSystem::ImperialBuilding)
    {
        ui->comboBox_FracInch->setVisible(true);
        ui->fractionalInchLabel->setVisible(true);
    }
    else
    {
        ui->comboBox_FracInch->setVisible(false);
        ui->fractionalInchLabel->setVisible(false);
    }
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsGeneral::~DlgSettingsGeneral() = default;

/** Sets the size of the recent file list from the user parameters.
 * @see RecentFilesAction
 * @see StdCmdRecentFiles
 */
void DlgSettingsGeneral::setRecentFileSize()
{
    auto recent = getMainWindow()->findChild<RecentFilesAction *>
        (QLatin1String("recentFiles"));
    if (recent) {
        ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("RecentFiles");
        recent->resizeList(hGrp->GetInt("RecentFiles", 4));
    }
}

bool DlgSettingsGeneral::setLanguage()
{
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QString lang = QLocale::languageToString(QLocale().language());
    QByteArray language = hGrp->GetASCII("Language", (const char*)lang.toLatin1()).c_str();
    QByteArray current = ui->Languages->itemData(ui->Languages->currentIndex()).toByteArray();
    if (current != language) {
        hGrp->SetASCII("Language", current.constData());
        Translator::instance()->activateLanguage(current.constData());
        return true;
    }
    return false;
}

void DlgSettingsGeneral::setNumberLocale(bool force/* = false*/)
{
    int localeFormat = ui->UseLocaleFormatting->currentIndex();

    // Only make the change if locale setting has changed or if forced
    // Except if format is "OS" where we don't want to run setLocale
    if (localeIndex == localeFormat && (!force || localeFormat == 0)) {
        return;
    }

    if (localeFormat == 0) {
        Translator::instance()->setLocale(); // Defaults to system locale
    }
    else if (localeFormat == 1) {
        QByteArray current = ui->Languages->itemData(ui->Languages->currentIndex()).toByteArray();
        Translator::instance()->setLocale(current.constData());
    }
    else if (localeFormat == 2) {
        Translator::instance()->setLocale("C");
    }
    else {
        return; // Prevent localeIndex updating if localeFormat is out of range
    }
    localeIndex = localeFormat;
}

void DlgSettingsGeneral::setDecimalPointConversion(bool on)
{
    if (Translator::instance()->isEnabledDecimalPointConversion() != on) {
        Translator::instance()->enableDecimalPointConversion(on);
    }
}

void DlgSettingsGeneral::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    int FracInch;  // minimum fractional inch to display
    int viewSystemIndex; // currently selected View System (unit system)

    ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath
    ("User parameter:BaseApp/Preferences/Units");
    hGrpu->SetInt("UserSchema", ui->comboBox_UnitSystem->currentIndex());
    hGrpu->SetInt("Decimals", ui->spinBoxDecimals->value());

    // Set actual value
    Base::UnitsApi::setDecimals(ui->spinBoxDecimals->value());

    // Convert the combobox index to the its integer denominator. Currently
    // with 1/2, 1/4, through 1/128, this little equation directly computes the
    // denominator given the combobox integer.
    //
    // The inverse conversion is done when loaded. That way only one thing (the
    // numerical fractional inch value) needs to be stored.
    FracInch = std::pow(2, ui->comboBox_FracInch->currentIndex() + 1);
    hGrpu->SetInt("FracInch", FracInch);

    // Set the actual format value
    Base::QuantityFormat::setDefaultDenominator(FracInch);

    // Set and save the Unit System
    viewSystemIndex = ui->comboBox_UnitSystem->currentIndex();
    auto activeDoc = Gui::Application::Instance->activeDocument();
    bool projectUnitSystemIgnore = ui->checkBox_projectUnitSystemIgnore->isChecked();
    if(activeDoc){
    	activeDoc->setProjectUnitSystemIgnore( projectUnitSystemIgnore );
    	if(!projectUnitSystemIgnore){
			int projectUnitSystemIndex = ui->comboBox_projectUnitSystem->currentIndex();
			activeDoc->setProjectUnitSystem( projectUnitSystemIndex );
			UnitsApi::setSchema(static_cast<UnitSystem>(projectUnitSystemIndex));
    	}else{
    		UnitsApi::setSchema(static_cast<UnitSystem>(viewSystemIndex));
    	}
    }else{
    	UnitsApi::setSchema(static_cast<UnitSystem>(viewSystemIndex));
    }
    //

    ui->SubstituteDecimal->onSave();
    ui->UseLocaleFormatting->onSave();
    ui->RecentFiles->onSave();
    ui->EnableCursorBlinking->onSave();
    ui->SplashScreen->onSave();

    setRecentFileSize();
    bool force = setLanguage();
    // In case type is "Selected language", we need to force locale change
    setNumberLocale(force);
    setDecimalPointConversion(ui->SubstituteDecimal->isChecked());

    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    QVariant size = ui->toolbarIconSize->itemData(ui->toolbarIconSize->currentIndex());
    int pixel = size.toInt();
    hGrp->SetInt("ToolbarIconSize", pixel);
    getMainWindow()->setIconSize(QSize(pixel,pixel));

    int blinkTime{hGrp->GetBool("EnableCursorBlinking", true) ? -1 : 0};
    qApp->setCursorFlashTime(blinkTime);

    saveDockWindowVisibility();

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    hGrp->SetBool("TiledBackground", ui->tiledBackground->isChecked());

    if (themeChanged)
        saveThemes();
}

void DlgSettingsGeneral::loadSettings()
{
    int FracInch;
    int cbIndex;

    ParameterGrp::handle hGrpu = App::GetApplication().GetParameterGroupByPath
    ("User parameter:BaseApp/Preferences/Units");
    ui->comboBox_UnitSystem->setCurrentIndex(hGrpu->GetInt("UserSchema", 0));
    ui->spinBoxDecimals->setValue(hGrpu->GetInt("Decimals", Base::UnitsApi::getDecimals()));

    // Get the current user setting for the minimum fractional inch
    FracInch = hGrpu->GetInt("FracInch", Base::QuantityFormat::getDefaultDenominator());

    // Convert fractional inch to the corresponding combobox index using this
    // handy little equation.
    cbIndex = std::log2(FracInch) - 1;
    ui->comboBox_FracInch->setCurrentIndex(cbIndex);
    
    
    auto activeDoc = Gui::Application::Instance->activeDocument();
    if(activeDoc){
		int us = activeDoc->getProjectUnitSystem();
		if(us >= 0){//Valid unit system:
			ui->comboBox_projectUnitSystem->setCurrentIndex( us );
			int pusIgnore = activeDoc->getProjectUnitSystemIgnore();
			ui->checkBox_projectUnitSystemIgnore->setChecked( pusIgnore );
		}else{
			ui->comboBox_projectUnitSystem->setCurrentIndex( 0 );
			ui->checkBox_projectUnitSystemIgnore->setChecked( false );
		}
    }else{
    	ui->checkBox_projectUnitSystemIgnore->setEnabled(false);
		ui->comboBox_projectUnitSystem->setEnabled(false);
    }



    ui->SubstituteDecimal->onRestore();
    ui->UseLocaleFormatting->onRestore();
    ui->RecentFiles->onRestore();
    ui->EnableCursorBlinking->onRestore();
    ui->SplashScreen->onRestore();

    // search for the language files
    ParameterGrp::handle hGrp = WindowParameter::getDefaultParameter()->GetGroup("General");
    auto langToStr = Translator::instance()->activeLanguage();
    QByteArray language = hGrp->GetASCII("Language", langToStr.c_str()).c_str();

    localeIndex = ui->UseLocaleFormatting->currentIndex();

    int index = 1;
    TStringMap list = Translator::instance()->supportedLocales();
    ui->Languages->clear();
    ui->Languages->addItem(QString::fromLatin1("English"), QByteArray("English"));
    for (auto it = list.begin(); it != list.end(); ++it, index++) {
        QByteArray lang = it->first.c_str();
        QString langname = QString::fromLatin1(lang.constData());

        if (it->second == "sr-CS") {
            // Qt does not treat sr-CS (Serbian, Latin) as a Latin-script variant by default: this
            // forces it to do so.
            it->second = "sr_Latn";
        }

        QLocale locale(QString::fromLatin1(it->second.c_str()));
        QString native = locale.nativeLanguageName();
        if (!native.isEmpty()) {
            if (native[0].isLetter())
                native[0] = native[0].toUpper();
            langname = native;
        }

        ui->Languages->addItem(langname, lang);
        if (language == lang) {
            ui->Languages->setCurrentIndex(index);
        }
    }

    QAbstractItemModel* model = ui->Languages->model();
    if (model)
        model->sort(0);

    int current = getMainWindow()->iconSize().width();
    current = hGrp->GetInt("ToolbarIconSize", current);
    ui->toolbarIconSize->clear();
    ui->toolbarIconSize->addItem(tr("Small (%1px)").arg(16), QVariant((int)16));
    ui->toolbarIconSize->addItem(tr("Medium (%1px)").arg(24), QVariant((int)24));
    ui->toolbarIconSize->addItem(tr("Large (%1px)").arg(32), QVariant((int)32));
    ui->toolbarIconSize->addItem(tr("Extra large (%1px)").arg(48), QVariant((int)48));
    index = ui->toolbarIconSize->findData(QVariant(current));
    if (index < 0) {
        ui->toolbarIconSize->addItem(tr("Custom (%1px)").arg(current), QVariant((int)current));
        index = ui->toolbarIconSize->findData(QVariant(current));
    }
    ui->toolbarIconSize->setCurrentIndex(index);

    //TreeMode combobox setup.
    loadDockWindowVisibility();

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");
    ui->tiledBackground->setChecked(hGrp->GetBool("TiledBackground", false));

    loadThemes();
}

void DlgSettingsGeneral::saveThemes()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

    // First we check if the theme has actually changed.
    std::string previousTheme = hGrp->GetASCII("Theme", "").c_str();
    std::string newTheme = ui->themesCombobox->currentText().toStdString();

    if (previousTheme == newTheme) {
        themeChanged = false;
        return;
    }

    // Save the name of the theme
    hGrp->SetASCII("Theme", newTheme);

    // Then we apply the themepack.
    Application::Instance->prefPackManager()->rescan();
    auto packs = Application::Instance->prefPackManager()->preferencePacks();

    for (const auto& pack : packs) {
        if (pack.first == newTheme) {

            if (Application::Instance->prefPackManager()->apply(pack.first)) {
                auto parentDialog = qobject_cast<DlgPreferencesImp*> (this->window());
                if (parentDialog)
                    parentDialog->reload();
            }
            break;
        }
    }

    // Set the StyleSheet
    QString sheet = QString::fromStdString(hGrp->GetASCII("StyleSheet"));
    bool tiledBackground = hGrp->GetBool("TiledBackground", false);
    Application::Instance->setStyleSheet(sheet, tiledBackground);

    themeChanged = false;
}

void DlgSettingsGeneral::loadThemes()
{
    ui->themesCombobox->clear();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/MainWindow");

    QString currentTheme = QString::fromLatin1(hGrp->GetASCII("Theme", "").c_str());

    Application::Instance->prefPackManager()->rescan();
    auto packs = Application::Instance->prefPackManager()->preferencePacks();
    for (const auto& pack : packs) {
        if (pack.second.metadata().type() == "Theme") {
            ui->themesCombobox->addItem(QString::fromStdString(pack.first));
        }
    }

    int index = ui->themesCombobox->findText(currentTheme);
    if (index >= 0 && index < ui->themesCombobox->count()) {
        ui->themesCombobox->setCurrentIndex(index);
    }
}

void DlgSettingsGeneral::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        int index = ui->UseLocaleFormatting->currentIndex();
        int index2 = ui->comboBox_UnitSystem->currentIndex();
        int pusIndex = ui->comboBox_projectUnitSystem->currentIndex();
        ui->retranslateUi(this);
        ui->UseLocaleFormatting->setCurrentIndex(index);
        ui->comboBox_UnitSystem->setCurrentIndex(index2);
        ui->comboBox_projectUnitSystem->setCurrentIndex(pusIndex);
    }
    else {
        QWidget::changeEvent(event);
    }
}

void DlgSettingsGeneral::saveDockWindowVisibility()
{
    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DockWindows");
    bool treeView = hGrp->GetGroup("TreeView")->GetBool("Enabled", false);
    bool propertyView = hGrp->GetGroup("PropertyView")->GetBool("Enabled", false);
    bool comboView = hGrp->GetGroup("ComboView")->GetBool("Enabled", true);
    switch (ui->treeMode->currentIndex()) {
    case 0:
        comboView = true;
        treeView = propertyView = false;
        break;
    case 1:
        treeView = propertyView = true;
        comboView = false;
        break;
    }

    hGrp->GetGroup("ComboView")->SetBool("Enabled", comboView);
    hGrp->GetGroup("TreeView")->SetBool("Enabled", treeView);
    hGrp->GetGroup("PropertyView")->SetBool("Enabled", propertyView);
}

void DlgSettingsGeneral::loadDockWindowVisibility()
{
    ui->treeMode->clear();
    ui->treeMode->addItem(tr("Combo View"));
    ui->treeMode->addItem(tr("TreeView and PropertyView"));

    auto hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/DockWindows");
    bool propertyView = hGrp->GetGroup("PropertyView")->GetBool("Enabled", false);
    bool treeView = hGrp->GetGroup("TreeView")->GetBool("Enabled", false);
    bool comboView = hGrp->GetGroup("ComboView")->GetBool("Enabled", true);
    int index = -1;
    if (propertyView || treeView) {
        index = 1;
    }
    else if (comboView) {
        index = 0;
    }
    ui->treeMode->setCurrentIndex(index);
}

void DlgSettingsGeneral::recreatePreferencePackMenu()
{
    ui->PreferencePacks->setRowCount(0); // Begin by clearing whatever is there
    ui->PreferencePacks->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->PreferencePacks->setColumnCount(3);
    ui->PreferencePacks->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    ui->PreferencePacks->horizontalHeader()->setStretchLastSection(false);
    ui->PreferencePacks->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
    ui->PreferencePacks->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::Stretch);
    ui->PreferencePacks->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);
    QStringList columnHeaders;
    columnHeaders << tr("Preference Pack Name")
                  << tr("Tags")
                  << QString(); // for the "Load" buttons
    ui->PreferencePacks->setHorizontalHeaderLabels(columnHeaders);

    // Populate the Preference Packs list
    Application::Instance->prefPackManager()->rescan();
    auto packs = Application::Instance->prefPackManager()->preferencePacks();

    // Remove the Themes.
    std::vector<std::string> packsToRemove;
    for (const auto& pack : packs) {
        if (pack.second.metadata().type() == "Theme") {
            packsToRemove.push_back(pack.first); // Store the keys to remove later
        }
    }
    for (const auto& key : packsToRemove) {
        packs.erase(key); // Remove the elements from the map
    }

    ui->PreferencePacks->setRowCount(packs.size());

    int row = 0;
    QIcon icon = style()->standardIcon(QStyle::SP_DialogApplyButton);
    for (const auto& pack : packs) {
        auto name = new QTableWidgetItem(QString::fromStdString(pack.first));
        name->setToolTip(QString::fromStdString(pack.second.metadata().description()));
        ui->PreferencePacks->setItem(row, 0, name);
        auto tags = pack.second.metadata().tag();
        QString tagString;
        for (const auto& tag : tags) {
            if (tagString.isEmpty())
                tagString.append(QString::fromStdString(tag));
            else
                tagString.append(QStringLiteral(", ") + QString::fromStdString(tag));
        }
        auto kind = new QTableWidgetItem(tagString);
        ui->PreferencePacks->setItem(row, 1, kind);
        auto button = new QPushButton(icon, tr("Apply"));
        button->setToolTip(tr("Apply the %1 preference pack").arg(QString::fromStdString(pack.first)));
        connect(button, &QPushButton::clicked, this, [this, pack]() { onLoadPreferencePackClicked(pack.first); });
        ui->PreferencePacks->setCellWidget(row, 2, button);
        ++row;
    }
}

void DlgSettingsGeneral::saveAsNewPreferencePack()
{
    // Create and run a modal New PreferencePack dialog box
    auto packs = Application::Instance->prefPackManager()->preferencePackNames();
    newPreferencePackDialog = std::make_unique<DlgCreateNewPreferencePackImp>(this);
    newPreferencePackDialog->setPreferencePackTemplates(Application::Instance->prefPackManager()->templateFiles());
    newPreferencePackDialog->setPreferencePackNames(packs);
    connect(newPreferencePackDialog.get(), &DlgCreateNewPreferencePackImp::accepted, this, &DlgSettingsGeneral::newPreferencePackDialogAccepted);
    newPreferencePackDialog->open();
}

void DlgSettingsGeneral::revertToSavedConfig()
{
    revertToBackupConfigDialog = std::make_unique<DlgRevertToBackupConfigImp>(this);
    connect(revertToBackupConfigDialog.get(), &DlgRevertToBackupConfigImp::accepted, this, [this]() {
        auto parentDialog = qobject_cast<DlgPreferencesImp*> (this->window());
        if (parentDialog) {
            parentDialog->reload();
        }
    });
    revertToBackupConfigDialog->open();
}

void DlgSettingsGeneral::newPreferencePackDialogAccepted()
{
    auto preferencePackTemplates = Application::Instance->prefPackManager()->templateFiles();
    auto selection = newPreferencePackDialog->selectedTemplates();
    std::vector<PreferencePackManager::TemplateFile> selectedTemplates;
    std::copy_if(preferencePackTemplates.begin(), preferencePackTemplates.end(), std::back_inserter(selectedTemplates), [selection](PreferencePackManager::TemplateFile& tf) {
        for (const auto& item : selection) {
            if (item.group == tf.group && item.name == tf.name) {
                return true;
            }
        }
        return false;
    });
    auto preferencePackName = newPreferencePackDialog->preferencePackName();
    Application::Instance->prefPackManager()->save(preferencePackName, selectedTemplates);
    recreatePreferencePackMenu();
}

void DlgSettingsGeneral::onManagePreferencePacksClicked()
{
    if (!this->preferencePackManagementDialog) {
        this->preferencePackManagementDialog = std::make_unique<DlgPreferencePackManagementImp>(this);
        connect(this->preferencePackManagementDialog.get(), &DlgPreferencePackManagementImp::packVisibilityChanged,
            this, &DlgSettingsGeneral::recreatePreferencePackMenu);
    }
    this->preferencePackManagementDialog->show();
}

void DlgSettingsGeneral::onImportConfigClicked()
{
    auto path = fs::path(QFileDialog::getOpenFileName(this,
        tr("Choose a FreeCAD config file to import"),
        QString(),
        QString::fromUtf8("*.cfg")).toStdString());
    if (!path.empty()) {
        // Create a name from the filename:
        auto packName = path.filename().stem().string();
        std::replace(packName.begin(), packName.end(), '_', ' ');
        auto existingPacks = Application::Instance->prefPackManager()->preferencePackNames();
        if (std::find(existingPacks.begin(), existingPacks.end(), packName)
            != existingPacks.end()) {
            auto result = QMessageBox::question(
                this, tr("File exists"),
                tr("A preference pack with that name already exists. Overwrite?"));
            if (result == QMessageBox::No) { // Maybe someday ask for a new name?
                return;
            }
        }
        Application::Instance->prefPackManager()->importConfig(packName, path);
        recreatePreferencePackMenu();
    }
}

void DlgSettingsGeneral::onLoadPreferencePackClicked(const std::string& packName)
{
    if (Application::Instance->prefPackManager()->apply(packName)) {
        auto parentDialog = qobject_cast<DlgPreferencesImp*> (this->window());
        if (parentDialog)
            parentDialog->reload();
    }
}

void DlgSettingsGeneral::onUnitSystemIndexChanged(int index)
{
    if (index < 0)
        return; // happens when clearing the combo box in retranslateUi()

    // Enable/disable the fractional inch option depending on system
    if (static_cast<UnitSystem>(index) == UnitSystem::ImperialBuilding)
    {
        ui->comboBox_FracInch->setVisible(true);
        ui->fractionalInchLabel->setVisible(true);
    }
    else
    {
        ui->comboBox_FracInch->setVisible(false);
        ui->fractionalInchLabel->setVisible(false);
    }
}

void DlgSettingsGeneral::on_checkBox_projectUnitSystemIgnore_stateChanged(int state)
{
    if (state < 0)
        return; // happens when clearing the combo box in retranslateUi()

    // Enable/disable the projectUnitSystem if being ignored:
    if(state == 2){//ignore
    	ui->comboBox_projectUnitSystem->setEnabled(false);
    }else if(state == 0){
    	ui->comboBox_projectUnitSystem->setEnabled(true);
    }
}

void DlgSettingsGeneral::onThemeChanged(int index) {
    Q_UNUSED(index);
    themeChanged = true;
}

#include "moc_DlgSettingsGeneral.cpp"
