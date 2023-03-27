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
#include <QLabel>
#include <sstream>
#endif

#include "DlgSettingsWorkbenchesImp.h"
#include "ui_DlgSettingsWorkbenches.h"
#include "Application.h"
#include "UserSettings.h"
#include "Workbench.h"
#include "WorkbenchManager.h"


using namespace Gui::Dialog;

namespace Gui::Dialog {
class wbListItem : public QWidget
{
    Q_OBJECT

public:
    explicit wbListItem(const QString& wbName, bool enabled, bool startupWb, bool autoLoad, QWidget* parent = nullptr);
    ~wbListItem() override;

    bool isEnabled();
    bool isAutoLoading();
    void setEnableCheckboxActivated(bool val);

protected Q_SLOTS:
    void onLoadClicked();
    void onWbActivated(bool checked);

Q_SIGNALS:
    void activatedWbListChanged();

private:
    QCheckBox* enableCheckBox;
    QCheckBox* autoloadCheckBox;
    QLabel* iconLabel;
    QLabel* textLabel;
    QLabel* loadLabel;
    QPushButton* loadButton;
};
}

wbListItem::wbListItem(const QString& wbName, bool enabled, bool startupWb, bool autoLoad, QWidget* parent) : QWidget(parent)
{
    this->setObjectName(wbName);

    auto wbTooltip = Application::Instance->workbenchToolTip(wbName);
    auto wbDisplayName = Application::Instance->workbenchMenuText(wbName);

    // 1: Enable checkbox
    enableCheckBox = new QCheckBox(this);
    enableCheckBox->setToolTip(tr("If unchecked, %1 will not appear in the available workbenches.").arg(wbDisplayName));
    enableCheckBox->setChecked(enabled);
    if (startupWb) {
        enableCheckBox->setChecked(true);
        enableCheckBox->setEnabled(false);
        enableCheckBox->setToolTip(tr("This is the current startup module, and must be enabled. See Preferences/General/Autoload to change."));
    }
    connect(enableCheckBox, &QCheckBox::toggled, this, [this](bool checked) { onWbActivated(checked); });

    // 2: Workbench Icon
    auto wbIcon = Application::Instance->workbenchIcon(wbName);
    iconLabel = new QLabel(wbDisplayName, this);
    iconLabel->setPixmap(wbIcon.scaled(QSize(20, 20), Qt::AspectRatioMode::KeepAspectRatio, Qt::TransformationMode::SmoothTransformation));
    iconLabel->setToolTip(wbTooltip);
    iconLabel->setContentsMargins(5, 0, 0, 5); // Left, top, right, bottom
    iconLabel->setEnabled(enableCheckBox->isChecked());

    // 3: Workbench Display Name
    textLabel = new QLabel(wbDisplayName, this);
    textLabel->setToolTip(wbTooltip);
    QFont font = textLabel->font();
    font.setBold(true);
    textLabel->setFont(font);
    textLabel->setEnabled(enableCheckBox->isChecked());
    textLabel->setMinimumSize(200, 0);


    // 4: Autoloaded checkBox.
    autoloadCheckBox = new QCheckBox(this);
    autoloadCheckBox->setText(tr("Auto-load"));
    autoloadCheckBox->setToolTip(tr("If checked, %1 will be loaded automatically when FreeCAD starts up").arg(wbDisplayName));
    autoloadCheckBox->setEnabled(enableCheckBox->isChecked());

    if (startupWb) { // Figure out whether to check and/or disable this checkBox:
        autoloadCheckBox->setChecked(true);
        autoloadCheckBox->setEnabled(false);
        autoloadCheckBox->setToolTip(tr("This is the current startup module, and must be autoloaded. See Preferences/General/Autoload to change."));
    }
    else if (autoLoad) {
        autoloadCheckBox->setChecked(true);
    }

    // 5: Load button/loaded indicator
    loadLabel = new QLabel(tr("Loaded"), this);
    loadLabel->setAlignment(Qt::AlignCenter);
    loadLabel->setEnabled(enableCheckBox->isChecked());
    loadButton = new QPushButton(tr("Load"), this);
    loadButton->setToolTip(tr("To preserve resources, FreeCAD does not load workbenches until they are used. Loading them may provide access to additional preferences related to their functionality."));
    loadButton->setEnabled(enableCheckBox->isChecked());
    connect(loadButton, &QPushButton::clicked, this, [this]() { onLoadClicked(); });
    if (WorkbenchManager::instance()->getWorkbench(wbName.toStdString())) {
        loadButton->setVisible(false);
    }
    else {
        loadLabel->setVisible(false);
    }

    auto layout = new QHBoxLayout(this);
    layout->addWidget(enableCheckBox);
    layout->addWidget(iconLabel);
    layout->addWidget(textLabel);
    layout->addWidget(loadButton);
    layout->addWidget(loadLabel);
    layout->addWidget(autoloadCheckBox);
    layout->setAlignment(Qt::AlignLeft);
    layout->setContentsMargins(10, 0, 0, 0);
}

wbListItem::~wbListItem()
{
}

bool wbListItem::isEnabled()
{
    return enableCheckBox->isChecked();
}

bool wbListItem::isAutoLoading()
{
    return autoloadCheckBox->isChecked();
}

void wbListItem::setEnableCheckboxActivated(bool val)
{
    enableCheckBox->setEnabled(val);
}

void wbListItem::onLoadClicked()
{
    // activate selected workbench
    Workbench* originalActiveWB = WorkbenchManager::instance()->active();
    Application::Instance->activateWorkbench(objectName().toStdString().c_str());
    Application::Instance->activateWorkbench(originalActiveWB->name().c_str());

    // replace load button with loaded indicator
    loadButton->setVisible(false);
    loadLabel->setVisible(true);
}

void wbListItem::onWbActivated(bool checked)
{
    // activate/deactivate the widgets
    iconLabel->setEnabled(checked);
    textLabel->setEnabled(checked);
    loadLabel->setEnabled(checked);
    loadButton->setEnabled(checked);
    autoloadCheckBox->setEnabled(checked);
    if (!checked) //disabling wb disable auto-load.
        autoloadCheckBox->setChecked(false);

    // Reset the start combo items.
    Q_EMIT activatedWbListChanged();
}

/* TRANSLATOR Gui::Dialog::DlgSettingsWorkbenchesImp */

/**
 *  Constructs a DlgSettingsWorkbenchesImp
 */
DlgSettingsWorkbenchesImp::DlgSettingsWorkbenchesImp( QWidget* parent )
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsWorkbenches)
{
    ui->setupUi(this);

    ui->wbList->setDragDropMode(QAbstractItemView::InternalMove);
    ui->wbList->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->wbList->viewport()->setAcceptDrops(true);
    ui->wbList->setDropIndicatorShown(true);
    ui->wbList->setDragEnabled(true);
    ui->wbList->setDefaultDropAction(Qt::MoveAction);

    connect(ui->AutoloadModuleCombo, QOverload<int>::of(&QComboBox::activated), this, [this](int index) { onStartWbChangedClicked(index); });
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsWorkbenchesImp::~DlgSettingsWorkbenchesImp()
{
}


void DlgSettingsWorkbenchesImp::saveSettings()
{
    std::ostringstream enabledStr, disabledStr, autoloadStr;

    for (int i = 0; i < ui->wbList->count(); i++) {
        wbListItem* wbItem = dynamic_cast<wbListItem*>(ui->wbList->itemWidget(ui->wbList->item(i)));
        if (!wbItem)
            continue;

        if (wbItem->isEnabled()) {
            if (!enabledStr.str().empty())
                enabledStr << ",";
            enabledStr << wbItem->objectName().toStdString();
        }
        else {
            if (!disabledStr.str().empty())
                disabledStr << ",";
            disabledStr << wbItem->objectName().toStdString();
        }

        if (wbItem->isAutoLoading()) {
            if (!autoloadStr.str().empty())
                autoloadStr << ",";
            autoloadStr << wbItem->objectName().toStdString();
        }
    }

    if (enabledStr.str().empty()) //make sure that we have at least one enabled workbench.
        enabledStr << "NoneWorkbench";
    else {
        if (!disabledStr.str().empty())
            disabledStr << ",";
        disabledStr << "NoneWorkbench"; //Note, NoneWorkbench is not in the table so it's not added before.
    }

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Workbenches");
    hGrp->SetASCII("Enabled", enabledStr.str().c_str());
    hGrp->SetASCII("Disabled", disabledStr.str().c_str());

    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        SetASCII("BackgroundAutoloadModules", autoloadStr.str().c_str());

    saveWorkbenchSelector();

    int index = ui->AutoloadModuleCombo->currentIndex();
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General")->
        SetASCII("AutoloadModule", startWbName.toLatin1());
}

void DlgSettingsWorkbenchesImp::loadSettings()
{
    loadWorkbenchSelector();

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

    //We set the startup setting after building the list so that we can put only the enabled wb.
    setStartWorkbenchComboItems();
}

/**
Build the list of unloaded workbenches.
*/
void DlgSettingsWorkbenchesImp::buildWorkbenchList()
{
    QStringList workbenches = Application::Instance->workbenches();
    QStringList enabledWbs = getEnabledWorkbenches();
    QStringList disabledWbs = getDisabledWorkbenches();

    //First we add the enabled wbs in their saved order.
    for (const auto& wbName : enabledWbs) {
        if (workbenches.contains(wbName)) {
            addWorkbench(wbName, true);
        }
        else {
            Base::Console().Warning("Ignoring unknown %s workbench found in user preferences.", wbName.toStdString().c_str());
        }
    }
    //Second we add workbench in alphabetical order that are either Disabled, or !enabled && !disabled, ie newly added wb.
    for (const auto& wbName : workbenches) {
        if (disabledWbs.contains(wbName)) {
            addWorkbench(wbName, false);
        }
        else if (!enabledWbs.contains(wbName)) {
            Base::Console().Warning("Adding unknown %s workbench.", wbName.toStdString().c_str());
            addWorkbench(wbName, false);
        }
    }
}

void DlgSettingsWorkbenchesImp::addWorkbench(const QString& wbName, bool enabled)
{
    if (wbName.toStdString() == "NoneWorkbench")
        return; // Do not list the default empty Workbench

    bool isStartupWb = wbName.toStdString() == _startupModule;
    bool autoLoad = std::find(_backgroundAutoloadedModules.begin(), _backgroundAutoloadedModules.end(),
        wbName.toStdString()) != _backgroundAutoloadedModules.end();
    wbListItem* widget = new wbListItem(wbName, enabled, isStartupWb, autoLoad, this);
    connect(widget, &wbListItem::activatedWbListChanged, this, &DlgSettingsWorkbenchesImp::setStartWorkbenchComboItems);
    auto wItem = new QListWidgetItem();
    wItem->setSizeHint(widget->sizeHint());
    ui->wbList->addItem(wItem);
    ui->wbList->setItemWidget(wItem, widget);
}


QStringList DlgSettingsWorkbenchesImp::getEnabledWorkbenches()
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

QStringList DlgSettingsWorkbenchesImp::getDisabledWorkbenches()
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
void DlgSettingsWorkbenchesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsWorkbenchesImp::saveWorkbenchSelector()
{
    //save workbench selector position
    auto index = ui->WorkbenchSelectorPosition->currentIndex();
    WorkbenchSwitcher::setIndex(index);
}

void DlgSettingsWorkbenchesImp::loadWorkbenchSelector()
{
    //workbench selector position combobox setup
    ui->WorkbenchSelectorPosition->clear();
    ui->WorkbenchSelectorPosition->addItem(tr("Toolbar"));
    ui->WorkbenchSelectorPosition->addItem(tr("Left corner"));
    ui->WorkbenchSelectorPosition->addItem(tr("Right corner"));
    ui->WorkbenchSelectorPosition->setCurrentIndex(WorkbenchSwitcher::getIndex());
}

void DlgSettingsWorkbenchesImp::setStartWorkbenchComboItems()
{
    ui->AutoloadModuleCombo->clear();

    // fills the combo box with activated workbenches.
    QStringList enabledWbs;
    for (int i = 0; i < ui->wbList->count(); i++) {
        wbListItem* wbItem = dynamic_cast<wbListItem*>(ui->wbList->itemWidget(ui->wbList->item(i)));
        if (wbItem && wbItem->isEnabled()) {
            enabledWbs << wbItem->objectName();
        }
    }

    QMap<QString, QString> menuText;
    for (const auto& it : enabledWbs) {
        QString text = Application::Instance->workbenchMenuText(it);
        menuText[text] = it;
    }

    {   // add special workbench to selection
        QPixmap px = Application::Instance->workbenchIcon(QString::fromLatin1("NoneWorkbench"));
        QString key = QString::fromLatin1("<last>");
        QString value = QString::fromLatin1("$LastModule");
        if (px.isNull()) {
            ui->AutoloadModuleCombo->addItem(key, QVariant(value));
        }
        else {
            ui->AutoloadModuleCombo->addItem(px, key, QVariant(value));
        }
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Application::Instance->workbenchIcon(it.value());
        if (px.isNull()) {
            ui->AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        }
        else {
            ui->AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
        }
    }

    ui->AutoloadModuleCombo->setCurrentIndex(ui->AutoloadModuleCombo->findData(QString::fromStdString(_startupModule)));
}

void Gui::Dialog::DlgSettingsWorkbenchesImp::onStartWbChangedClicked(int index)
{
    //Update _startupModule
    QVariant data = ui->AutoloadModuleCombo->itemData(index);
    QString wbName = data.toString();
    _startupModule = wbName.toStdString();

    //Change wb that user can't deactivate.
    for (int i = 0; i < ui->wbList->count(); i++) {
        wbListItem* wbItem = dynamic_cast<wbListItem*>(ui->wbList->itemWidget(ui->wbList->item(i)));
        if (wbItem) {
            wbItem->setEnableCheckboxActivated(!(wbItem->objectName() == wbName));
        }
    }
}

#include "moc_DlgSettingsWorkbenchesImp.cpp"
#include "DlgSettingsWorkbenchesImp.moc"