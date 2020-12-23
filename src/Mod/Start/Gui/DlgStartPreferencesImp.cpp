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
#include <Gui/PrefWidgets.h>
#include <Base/Console.h>
#include <Gui/Application.h>

using namespace StartGui;

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent' 
 */
DlgStartPreferencesImp::DlgStartPreferencesImp( QWidget* parent )
  : PreferencePage( parent )
{
    this->setupUi(this);
    
    // Hide currently unused controls
    label_12->hide();
    label_7->hide();
    colorButton_7->hide();
    radioButton_1->hide();
    radioButton_2->hide();
    
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
            AutoloadModuleCombo->addItem(key, QVariant(value));
        else
            AutoloadModuleCombo->addItem(px, key, QVariant(value));
    }

    for (QMap<QString, QString>::Iterator it = menuText.begin(); it != menuText.end(); ++it) {
        QPixmap px = Gui::Application::Instance->workbenchIcon(it.value());
        if (px.isNull())
            AutoloadModuleCombo->addItem(it.key(), QVariant(it.value()));
        else
            AutoloadModuleCombo->addItem(px, it.key(), QVariant(it.value()));
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
    int index = AutoloadModuleCombo->currentIndex();
    QVariant data = AutoloadModuleCombo->itemData(index);
    QString startWbName = data.toString();
    App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")->
                          SetASCII("AutoloadModule", startWbName.toLatin1());
    colorButton_1->onSave();
    colorButton_2->onSave();
    colorButton_3->onSave();
    colorButton_4->onSave();
    colorButton_5->onSave();
    colorButton_6->onSave();
    colorButton_7->onSave();
    fileChooser_1->onSave();
    fileChooser_2->onSave();
    fileChooser_3->onSave();
    radioButton_1->onSave();
    radioButton_2->onSave();
    checkBox->onSave();
    checkBox_1->onSave();
    checkBox_2->onSave();
    checkBox_3->onSave();
    checkBox_4->onSave();
    checkBox_5->onSave();
    checkBox_6->onSave();
    lineEdit->onSave();
    spinBox->onSave();
}

void DlgStartPreferencesImp::loadSettings()
{
    std::string start = App::Application::Config()["StartWorkbench"];
    start = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/Start")->
                                  GetASCII("AutoloadModule", start.c_str());
    QString startWbName = QLatin1String(start.c_str());
    AutoloadModuleCombo->setCurrentIndex(AutoloadModuleCombo->findData(startWbName));
    colorButton_1->onRestore();
    colorButton_2->onRestore();
    colorButton_3->onRestore();
    colorButton_4->onRestore();
    colorButton_5->onRestore();
    colorButton_6->onRestore();
    colorButton_7->onRestore();
    fileChooser_1->onRestore();
    fileChooser_2->onRestore();
    fileChooser_3->onRestore();
    radioButton_1->onRestore();
    radioButton_2->onRestore();
    checkBox->onRestore();
    checkBox_1->onRestore();
    checkBox_2->onRestore();
    checkBox_3->onRestore();
    checkBox_4->onRestore();
    checkBox_5->onRestore();
    checkBox_6->onRestore();
    lineEdit->onRestore();
    spinBox->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgStartPreferencesImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgStartPreferencesImp.cpp"
