/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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
# include <QRegExp>
# include <memory>
#endif

#include "DlgSettings3DViewImp.h"
#include "NavigationStyle.h"
#include "PrefWidgets.h"
#include "ui_MouseButtons.h"
#include <App/Application.h>
#include <Base/Console.h>
#include <Base/Parameter.h>
#include <Base/Tools.h>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettings3DViewImp */

/**
 *  Constructs a DlgSettings3DViewImp which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettings3DViewImp::DlgSettings3DViewImp(QWidget* parent)
    : PreferencePage( parent )
{
    this->setupUi(this);
    retranslate();
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettings3DViewImp::~DlgSettings3DViewImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettings3DViewImp::saveSettings()
{
    // must be done as very first because we create a new instance of NavigatorStyle
    // where we set some attributes afterwards
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    QVariant data = comboNavigationStyle->itemData(comboNavigationStyle->currentIndex(), Qt::UserRole);
    hGrp->SetASCII("NavigationStyle", (const char*)data.toByteArray());

    int index = comboOrbitStyle->currentIndex();
    hGrp->SetInt("OrbitStyle", index);

    checkBoxZoomAtCursor->onSave();
    checkBoxInvertZoom->onSave();
    spinBoxZoomStep->onSave();
    checkBoxAntiAliasing->onSave();
    CheckBox_CornerCoordSystem->onSave();
    CheckBox_ShowFPS->onSave();
    CheckBox_UseAutoRotation->onSave();
    FloatSpinBox_EyeDistance->onSave();
    checkBoxBacklight->onSave();
    backlightColor->onSave();
    sliderIntensity->onSave();
    radioPerspective->onSave();
    radioOrthographic->onSave();
}

void DlgSettings3DViewImp::loadSettings()
{
    checkBoxZoomAtCursor->onRestore();
    checkBoxInvertZoom->onRestore();
    spinBoxZoomStep->onRestore();
    checkBoxAntiAliasing->onRestore();
    CheckBox_CornerCoordSystem->onRestore();
    CheckBox_ShowFPS->onRestore();
    CheckBox_UseAutoRotation->onRestore();
    FloatSpinBox_EyeDistance->onRestore();
    checkBoxBacklight->onRestore();
    backlightColor->onRestore();
    sliderIntensity->onRestore();
    radioPerspective->onRestore();
    radioOrthographic->onRestore();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath
        ("User parameter:BaseApp/Preferences/View");
    std::string model = hGrp->GetASCII("NavigationStyle",CADNavigationStyle::getClassTypeId().getName());
    int index = comboNavigationStyle->findData(QByteArray(model.c_str()));
    if (index > -1) comboNavigationStyle->setCurrentIndex(index);

    index = hGrp->GetInt("OrbitStyle", int(NavigationStyle::Trackball));
    index = Base::clamp(index, 0, comboOrbitStyle->count()-1);
    comboOrbitStyle->setCurrentIndex(index);
}

void DlgSettings3DViewImp::on_mouseButton_clicked()
{
    QDialog dlg(this);
    Ui_MouseButtons ui;
    ui.setupUi(&dlg);

    QVariant data = comboNavigationStyle->itemData(comboNavigationStyle->currentIndex(), Qt::UserRole);
    void* instance = Base::Type::createInstanceByName((const char*)data.toByteArray());
    std::auto_ptr<UserNavigationStyle> ns(static_cast<UserNavigationStyle*>(instance));
    ui.groupBox->setTitle(ui.groupBox->title()+QString::fromAscii(" ")+comboNavigationStyle->currentText());
    QString descr;
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::SELECTION));
    ui.selectionLabel->setText(QString::fromAscii("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::PANNING));
    ui.panningLabel->setText(QString::fromAscii("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::DRAGGING));
    ui.rotationLabel->setText(QString::fromAscii("<b>%1</b>").arg(descr));
    descr = qApp->translate((const char*)data.toByteArray(),ns->mouseButtons(NavigationStyle::ZOOMING));
    ui.zoomingLabel->setText(QString::fromAscii("<b>%1</b>").arg(descr));
    dlg.exec();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettings3DViewImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        int navigation = comboNavigationStyle->currentIndex();
        int orbit = comboOrbitStyle->currentIndex();
        retranslateUi(this);
        retranslate();
        comboNavigationStyle->setCurrentIndex(navigation);
        comboOrbitStyle->setCurrentIndex(orbit);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettings3DViewImp::retranslate()
{
    std::vector<Base::Type> types;
    Base::Type::getAllDerivedFrom(UserNavigationStyle::getClassTypeId(), types);
    comboNavigationStyle->clear();

    QRegExp rx(QString::fromAscii("^\\w+::(\\w+)Navigation\\w+$"));
    for (std::vector<Base::Type>::iterator it = types.begin(); it != types.end(); ++it) {
        if (*it != UserNavigationStyle::getClassTypeId()) {
            QString data = QString::fromAscii(it->getName());
            QString name = data.mid(data.indexOf(QLatin1String("::"))+2);
            if (rx.indexIn(data) > -1) {
                name = tr("%1 navigation").arg(rx.cap(1));
            }
            comboNavigationStyle->addItem(name, data);
        }
    }
}

#include "moc_DlgSettings3DViewImp.cpp"

