/***************************************************************************
 *   Copyright (c) 2020 Chris Hennes (chennes@pioneerlibrarysystem.org)    *
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
#include <QCheckBox>
#include <QPushButton>
#include <sstream>
#endif

#include "DlgSettingsLazyLoadedImp.h"
#include "ui_DlgSettingsLazyLoaded.h"
#include "Application.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


using namespace Gui::Dialog;

const uint DlgSettingsLazyLoadedImp::WorkbenchNameRole = Qt::UserRole;

// this enum defines the order of the columns
enum Column {
    Enabled,
    Icon,
    Name,
    CheckBox,
    Load
};

/* TRANSLATOR Gui::Dialog::DlgSettingsLazyLoadedImp */

/**
 *  Constructs a DlgSettingsLazyLoadedImp
 */
DlgSettingsLazyLoadedImp::DlgSettingsLazyLoadedImp( QWidget* parent )
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsLazyLoaded)
{
    ui->setupUi(this);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsLazyLoadedImp::~DlgSettingsLazyLoadedImp()
{
}


void DlgSettingsLazyLoadedImp::saveSettings()
{
    //Save enabled
    std::ostringstream enabledStr;
    std::ostringstream disabledStr;
    for (const auto& checkBox : _enabledCheckBoxes) {
        if (checkBox.second->isChecked()) {
            if (!enabledStr.str().empty())
                enabledStr << ",";
            enabledStr << checkBox.first.toStdString();
        }
        else {
            if (!disabledStr.str().empty())
                disabledStr << ",";
            disabledStr << checkBox.first.toStdString();
        }
    }

    if (!enabledStr.str().empty()) //make sure that we have at least one enabled workbench.
        enabledStr << "NoneWorkbench";

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    hGrp->SetASCII("Enabled", enabledStr.str().c_str());
    hGrp->SetASCII("Disabled", disabledStr.str().c_str());

    //Save auto-load
    std::ostringstream csv;
    for (const auto& checkBox : _autoloadCheckBoxes) {
        if (checkBox.second->isChecked()) {
            if (!csv.str().empty())
                csv << ",";
            csv << checkBox.first.toStdString();
        }
    }
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        SetASCII("BackgroundAutoloadModules", csv.str().c_str());
}

void DlgSettingsLazyLoadedImp::loadSettings()
{
    // There are two different "autoload" settings: the first, in FreeCAD since 2004,
    // controls the module the user sees first when starting FreeCAD, and defaults to the Start workbench
    std::string start = App::Application::Config()["StartWorkbench"];
    _startupModule = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetASCII("AutoloadModule", start.c_str());

    // The second autoload setting does a background autoload of any number of other modules
    std::string autoloadCSV = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        GetASCII("BackgroundAutoloadModules", "");

    // Tokenize the comma-separated list
    _backgroundAutoloadedModules.clear();
    std::stringstream stream(autoloadCSV);
    std::string workbench;
    while (std::getline(stream, workbench, ','))
        _backgroundAutoloadedModules.push_back(workbench);


    buildUnloadedWorkbenchList();
}

void DlgSettingsLazyLoadedImp::onLoadClicked(const QString &wbName)
{
    // activate selected workbench
    Workbench* originalActiveWB = WorkbenchManager::instance()->active();
    Application::Instance->activateWorkbench(wbName.toStdString().c_str());
    Application::Instance->activateWorkbench(originalActiveWB->name().c_str());

    // replace load button with loaded indicator
    auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);
    for (int i = 0; i < ui->workbenchTable->rowCount(); i++) {
        QWidget* widget = ui->workbenchTable->cellWidget(i, Name);
        auto textLabel = dynamic_cast<QLabel*>(widget);
        if (textLabel && textLabel->text() == wbDisplayName) {
            auto label = new QLabel(tr("Loaded"));
            label->setAlignment(Qt::AlignCenter);
            ui->workbenchTable->setCellWidget(i, Load, label);
            break;
        }
    }
}


/**
Build the list of unloaded workbenches.
*/
void DlgSettingsLazyLoadedImp::buildUnloadedWorkbenchList()
{
    QStringList workbenches = Application::Instance->workbenches();
    workbenches.sort();
    QStringList enabled_wbs_list = getEnabledWorkbenches();

    ui->workbenchTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->workbenchTable->setRowCount(0);
    _autoloadCheckBoxes.clear(); // setRowCount(0) just invalidated all of these pointers
    _enabledCheckBoxes.clear();// setRowCount(0) just invalidated all of these pointers
    ui->workbenchTable->setColumnCount(5);
    ui->workbenchTable->setSelectionMode(QAbstractItemView::SelectionMode::NoSelection);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(Enabled, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(Icon, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(Name, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(CheckBox, QHeaderView::ResizeMode::ResizeToContents);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(Load, QHeaderView::ResizeMode::ResizeToContents);
    QStringList columnHeaders;
    for (int i = 0; i < 5; i++) {
        switch (i) {
            case Enabled : columnHeaders << tr("Enable");       break;
            case Icon    : columnHeaders << QString();            break;
            case Name    : columnHeaders << tr("Workbench Name"); break;
            case CheckBox: columnHeaders << tr("Autoload");      break;
            case Load    : columnHeaders << QString();            break;
        }
    }
    ui->workbenchTable->setHorizontalHeaderLabels(columnHeaders);

    unsigned int rowNumber = 0;
    for (const auto& wbName : workbenches) {
        if (wbName.toStdString() == "NoneWorkbench")
            continue; // Do not list the default empty Workbench

        ui->workbenchTable->insertRow(rowNumber);
        auto wbTooltip = Application::Instance->workbenchToolTip(wbName);
        auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);

        // Column 0: Enabled checkbox.
        auto enCheckWidget = new QWidget(this);
        auto enableCheckBox = new QCheckBox(this);
        enableCheckBox->setToolTip(tr("If unchecked, %1 will not appear in the available workbenches.").arg(wbDisplayName));
        auto enCheckLayout = new QHBoxLayout(enCheckWidget);
        enCheckLayout->addWidget(enableCheckBox);
        enCheckLayout->setAlignment(Qt::AlignCenter);
        enCheckLayout->setContentsMargins(0, 0, 0, 0);
        enableCheckBox->setChecked(enabled_wbs_list.contains(wbName));
        _enabledCheckBoxes.insert(std::make_pair(wbName, enableCheckBox));
        ui->workbenchTable->setCellWidget(rowNumber, Enabled, enCheckWidget);

        // Column 1: Workbench Icon
        auto wbIcon = Application::Instance->workbenchIcon(wbName);
        auto iconLabel = new QLabel();
        iconLabel->setPixmap(wbIcon.scaled(QSize(20,20), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
        iconLabel->setToolTip(wbTooltip);
        iconLabel->setContentsMargins(5, 3, 3, 3); // Left, top, right, bottom
        ui->workbenchTable->setCellWidget(rowNumber, Icon, iconLabel);

        // Column 2: Workbench Display Name
        auto textLabel = new QLabel(wbDisplayName);
        textLabel->setToolTip(wbTooltip);
        ui->workbenchTable->setCellWidget(rowNumber, Name, textLabel);

        // Column 3: Autoloaded checkBox
        //
        // To get the checkBox centered, we have to jump through some hoops...
        auto checkWidget = new QWidget(this);
        auto autoloadCheckBox = new QCheckBox(this);
        autoloadCheckBox->setToolTip(tr("If checked, %1 will be loaded automatically when FreeCAD starts up").arg(wbDisplayName));
        auto checkLayout = new QHBoxLayout(checkWidget);
        checkLayout->addWidget(autoloadCheckBox);
        checkLayout->setAlignment(Qt::AlignCenter);
        checkLayout->setContentsMargins(0, 0, 0, 0);

        // Figure out whether to check and/or disable this checkBox:
        if (wbName.toStdString() == _startupModule) {
            autoloadCheckBox->setChecked(true);
            autoloadCheckBox->setEnabled(false);
            autoloadCheckBox->setToolTip(tr("This is the current startup module, and must be autoloaded. See Preferences/General/Autoload to change."));
        }
        else if (std::find(_backgroundAutoloadedModules.begin(), _backgroundAutoloadedModules.end(),
                           wbName.toStdString()) != _backgroundAutoloadedModules.end()) {
            autoloadCheckBox->setChecked(true);
            _autoloadCheckBoxes.insert(std::make_pair(wbName, autoloadCheckBox));
        }
        else {
            _autoloadCheckBoxes.insert(std::make_pair(wbName, autoloadCheckBox));
        }
        ui->workbenchTable->setCellWidget(rowNumber, CheckBox, checkWidget);

        // Column 4: Load button/loaded indicator
        if (WorkbenchManager::instance()->getWorkbench(wbName.toStdString())) {
            auto label = new QLabel(tr("Loaded"));
            label->setAlignment(Qt::AlignCenter);
            ui->workbenchTable->setCellWidget(rowNumber, Load, label);
        }
        else {
            auto button = new QPushButton(tr("Load now"));
            connect(button, &QPushButton::clicked, this, [this,wbName]() { onLoadClicked(wbName); });
            ui->workbenchTable->setCellWidget(rowNumber, Load, button);
        }

        ++rowNumber;
    }
}

QStringList DlgSettingsLazyLoadedImp::getEnabledWorkbenches()
{
    QString enabled_wbs;
    QStringList enabled_wbs_list;
    ParameterGrp::handle hGrp;
    QString allWorkbenches = QString::fromLatin1("ALL");

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    enabled_wbs = QString::fromStdString(hGrp->GetASCII("Enabled", allWorkbenches.toStdString().c_str()).c_str());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    enabled_wbs_list = enabled_wbs.split(QLatin1String(","), Qt::SkipEmptyParts);
#else
    enabled_wbs_list = enabled_wbs.split(QLatin1String(","), QString::SkipEmptyParts);
#endif

    if (enabled_wbs_list.at(0) == allWorkbenches) {
        enabled_wbs_list.removeFirst();
        QStringList workbenches = Application::Instance->workbenches();
        for (QStringList::Iterator it = workbenches.begin(); it != workbenches.end(); ++it) {
            enabled_wbs_list.append(*it);
        }
        enabled_wbs_list.sort();
    }
    return enabled_wbs_list;
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsLazyLoadedImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsLazyLoadedImp.cpp"
