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
#include "BitmapFactory.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


using namespace Gui::Dialog;

const uint DlgSettingsLazyLoadedImp::WorkbenchNameRole = Qt::UserRole;

const int DlgSettingsLazyLoadedImp::columnCount = 1;

const QString DlgSettingsLazyLoadedImp::loadLabelStr = QString::fromLatin1("loadLabel");
const QString DlgSettingsLazyLoadedImp::loadButtonStr = QString::fromLatin1("loadButton");
const QString DlgSettingsLazyLoadedImp::enableCheckboxStr = QString::fromLatin1("enableCheckbox");
const QString DlgSettingsLazyLoadedImp::autoloadCheckboxStr = QString::fromLatin1("autoloadCheckbox");

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
    std::ostringstream enabledStr, disabledStr, autoloadStr;

    for (int i = 0; i < ui->workbenchTable->rowCount(); i++) {
        QWidget* widget = ui->workbenchTable->cellWidget(i, 0);
        if (!widget)
            continue;
        QCheckBox* enableBox = widget->findChild<QCheckBox*>(enableCheckboxStr);
        if (!enableBox)
            continue;

        if (enableBox->isChecked()) {
            if (!enabledStr.str().empty())
                enabledStr << ",";
            enabledStr << widget->objectName().toStdString();
        }
        else {
            if (!disabledStr.str().empty())
                disabledStr << ",";
            disabledStr << widget->objectName().toStdString();
        }

        QCheckBox* autoloadBox = widget->findChild<QCheckBox*>(autoloadCheckboxStr);
        if (!autoloadBox)
            continue;

        if (autoloadBox->isChecked()) {
            if (!autoloadStr.str().empty())
                autoloadStr << ",";
            autoloadStr << widget->objectName().toStdString();
        }
    }

    if (enabledStr.str().empty()) //make sure that we have at least one enabled workbench.
        enabledStr << "NoneWorkbench";
    else
        disabledStr << "NoneWorkbench"; //Note, NoneWorkbench is not in the table so it's not added before.

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    hGrp->SetASCII("Enabled", enabledStr.str().c_str());
    hGrp->SetASCII("Disabled", disabledStr.str().c_str());

    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        SetASCII("BackgroundAutoloadModules", autoloadStr.str().c_str());
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


    buildWorkbenchList();
}

void DlgSettingsLazyLoadedImp::onLoadClicked(const QString &wbName)
{
    // activate selected workbench
    Workbench* originalActiveWB = WorkbenchManager::instance()->active();
    Application::Instance->activateWorkbench(wbName.toStdString().c_str());
    Application::Instance->activateWorkbench(originalActiveWB->name().c_str());

    // replace load button with loaded indicator
    for (int i = 0; i < ui->workbenchTable->rowCount(); i++) {
        QWidget* widget = ui->workbenchTable->cellWidget(i, 0);
        if (widget && widget->objectName() == wbName) {
            QWidget* loadLabel = widget->findChild<QWidget*>(loadLabelStr);
            QWidget* loadButton = widget->findChild<QWidget*>(loadButtonStr);
            loadButton->setVisible(false);
            loadLabel->setVisible(true);
            break;
        }
    }
}

void DlgSettingsLazyLoadedImp::onUpDownClicked(const QString& wbName, bool up)
{
    int rowIndex = 0;
    //find the index of the row that is moving.
    for (int i = 0; i < ui->workbenchTable->rowCount(); i++) {
        QWidget* widget = ui->workbenchTable->cellWidget(i, 0);
        if(widget->objectName() == wbName) {
            break;
        }
        rowIndex++;
    }

    //Check if it can move
    if (((rowIndex == 0) && up) || ((rowIndex == ui->workbenchTable->rowCount() - 1) && !up))
        return;

    //Move in the _enabledCheckBoxes vector.
    //std::iter_swap(_enabledCheckBoxes.begin() + rowIndex, _enabledCheckBoxes.begin() + rowIndex + (up ? -1 : 1));

    //Move the rows in the table
    auto widget = ui->workbenchTable->cellWidget(rowIndex, 0);
    auto widget2 = ui->workbenchTable->cellWidget(rowIndex + (up ? -1 : 1), 0);

    auto newWidget = new QWidget(this);
    newWidget->setObjectName(widget->objectName());
    newWidget->setLayout(widget->layout());

    auto newWidget2 = new QWidget(this);
    newWidget2->setObjectName(widget2->objectName());
    newWidget2->setLayout(widget2->layout());

    ui->workbenchTable->removeCellWidget(rowIndex, 0);
    ui->workbenchTable->removeCellWidget(rowIndex + (up ? -1 : 1), 0);

    ui->workbenchTable->setCellWidget(rowIndex + (up ? -1 : 1), 0, newWidget);
    ui->workbenchTable->setCellWidget(rowIndex                , 0, newWidget2);
}

/**
Build the list of unloaded workbenches.
*/
void DlgSettingsLazyLoadedImp::buildWorkbenchList()
{
    QStringList workbenches = Application::Instance->workbenches();
    QStringList enabledWbs = getEnabledWorkbenches();
    QStringList disabledWbs = getDisabledWorkbenches();

    ui->workbenchTable->setSelectionMode(QAbstractItemView::NoSelection);
    ui->workbenchTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->workbenchTable->setRowCount(0);
    ui->workbenchTable->setColumnCount(columnCount);
    ui->workbenchTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::ResizeToContents);
    QStringList columnHeaders;
    columnHeaders << tr("Enable");
    ui->workbenchTable->setHorizontalHeaderLabels(columnHeaders);


    //First we add the enabled wbs in their saved order.
    for (const auto& wbName : enabledWbs) {
        if (workbenches.contains(wbName)) {
            addWorkbench(wbName, true);
        }
        else {
            qDebug() << "Ignoring unknown" << wbName << "workbench found in user preferences.";
        }
    }
    //Second we add workbench in alphabetical order that are either Disabled, or !enabled && !disabled, ie newly added wb.
    for (const auto& wbName : workbenches) {
        if (disabledWbs.contains(wbName)) {
            addWorkbench(wbName, false);
        }
        else if (!enabledWbs.contains(wbName)) {
            qDebug() << "Adding unknown " << wbName << "workbench.";
            addWorkbench(wbName, false);
        }
    }
}

void DlgSettingsLazyLoadedImp::addWorkbench(const QString& wbName, bool enabled)
{
    if (wbName.toStdString() == "NoneWorkbench")
        return; // Do not list the default empty Workbench

    int rowNumber = ui->workbenchTable->rowCount();
    ui->workbenchTable->insertRow(rowNumber);
    QWidget* widget = createWorkbenchWidget(wbName, enabled);
    ui->workbenchTable->setCellWidget(rowNumber, 0, widget);
}

QWidget* DlgSettingsLazyLoadedImp::createWorkbenchWidget(const QString& wbName, bool enabled)
{
    auto wbTooltip = Application::Instance->workbenchToolTip(wbName);
    auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);

    // 1: Enable checkbox
    auto enableCheckBox = new QCheckBox(this);
    enableCheckBox->setToolTip(tr("If unchecked, %1 will not appear in the available workbenches.").arg(wbDisplayName));
    enableCheckBox->setChecked(enabled);
    enableCheckBox->setObjectName(enableCheckboxStr);
    if (wbName.toStdString() == _startupModule) {
        enableCheckBox->setChecked(true);
        enableCheckBox->setEnabled(false);
        enableCheckBox->setToolTip(tr("This is the current startup module, and must be enabled. See Preferences/General/Autoload to change."));
    }

    // 2: Workbench Icon
    auto wbIcon = Application::Instance->workbenchIcon(wbName);
    auto iconLabel = new QLabel(wbDisplayName);
    iconLabel->setPixmap(wbIcon.scaled(QSize(20, 20), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
    iconLabel->setToolTip(wbTooltip);
    iconLabel->setContentsMargins(5, 0, 0, 5); // Left, top, right, bottom

    // 3: Workbench Display Name
    auto textLabel = new QLabel(wbDisplayName);
    textLabel->setToolTip(wbTooltip);

    // 4: Autoloaded checkBox.
    auto autoloadCheckBox = new QCheckBox(this);
    autoloadCheckBox->setObjectName(autoloadCheckboxStr);
    autoloadCheckBox->setText(tr("Auto-load"));
    autoloadCheckBox->setToolTip(tr("If checked, %1 will be loaded automatically when FreeCAD starts up").arg(wbDisplayName));

    if (wbName.toStdString() == _startupModule) { // Figure out whether to check and/or disable this checkBox:
        autoloadCheckBox->setChecked(true);
        autoloadCheckBox->setEnabled(false);
        autoloadCheckBox->setToolTip(tr("This is the current startup module, and must be autoloaded. See Preferences/General/Autoload to change."));
    }
    else if (std::find(_backgroundAutoloadedModules.begin(), _backgroundAutoloadedModules.end(),
        wbName.toStdString()) != _backgroundAutoloadedModules.end()) {
        autoloadCheckBox->setChecked(true);
    }

    // 5: Load button/loaded indicator
    auto loadLabel = new QLabel(tr("Loaded"));
    loadLabel->setAlignment(Qt::AlignCenter);
    loadLabel->setObjectName(loadLabelStr);
    auto loadButton = new QPushButton(tr("Load"));
    loadButton->setObjectName(loadButtonStr);
    connect(loadButton, &QPushButton::clicked, this, [this, wbName]() { onLoadClicked(wbName); });
    if (WorkbenchManager::instance()->getWorkbench(wbName.toStdString())) {
        loadButton->setVisible(false);
    }
    else {
        loadLabel->setVisible(false);
    }

    // 6: up down buttons
    auto downButton = new QToolButton(this);
    auto upButton = new QToolButton(this);
    downButton->setToolTip(tr("Move %1 down").arg(wbDisplayName));
    upButton->setToolTip(tr("Move %1 up").arg(wbDisplayName));
    downButton->setIcon(Gui::BitmapFactory().iconFromTheme("button_down"));
    upButton->setIcon(Gui::BitmapFactory().iconFromTheme("button_up"));
    connect(upButton, &QToolButton::clicked, this, [this, wbName]() { onUpDownClicked(wbName, true); });
    connect(downButton, &QToolButton::clicked, this, [this, wbName]() { onUpDownClicked(wbName, false); });


    auto mainWidget = new QWidget(this);
    mainWidget->setObjectName(wbName);
    auto layout = new QHBoxLayout(mainWidget);
    layout->addWidget(enableCheckBox);
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addStretch();
    layout->addWidget(loadButton);
    layout->addWidget(loadLabel);
    layout->addWidget(autoloadCheckBox);
    layout->addWidget(downButton);
    layout->addWidget(upButton);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(10, 0, 0, 0);

    return mainWidget;
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

QStringList DlgSettingsLazyLoadedImp::getDisabledWorkbenches()
{
    QString disabled_wbs;
    QStringList disabled_wbs_list;
    ParameterGrp::handle hGrp;

    hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    disabled_wbs = QString::fromStdString(hGrp->GetASCII("Disabled", ""));
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    disabled_wbs_list = disabled_wbs.split(QLatin1String(","), Qt::SkipEmptyParts);
#else
    disabled_wbs_list = disabled_wbs.split(QLatin1String(","), QString::SkipEmptyParts);
#endif

    return disabled_wbs_list;
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
