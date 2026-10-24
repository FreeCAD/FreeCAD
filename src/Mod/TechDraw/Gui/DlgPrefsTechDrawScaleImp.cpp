// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 FreeCAD Developers                                 *
 *   Author: WandererFan <wandererfan@gmail.com>                           *
 *   Based on src/Mod/FEM/Gui/DlgSettingsFEMImp.cpp                        *
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


#include "DlgPrefsTechDrawScaleImp.h"
#include "ui_DlgPrefsTechDrawScale.h"

#include <App/Application.h>
#include <QStringList>
#include <cmath>

namespace {
    double parseScaleInput(const QString& input) {
        QString s = input.trimmed();
        if (s.contains(QLatin1Char(':')) || s.contains(QLatin1Char('/'))) {
            QChar sep = s.contains(QLatin1Char(':')) ? QLatin1Char(':') : QLatin1Char('/');
            QStringList parts = s.split(sep);
            if (parts.size() == 2) {
                bool okNum, okDen;
                double num = parts[0].toDouble(&okNum);
                double den = parts[1].toDouble(&okDen);
                if (okNum && okDen && den != 0.0) {
                    return num / den;
                }
            }
        }
        bool ok;
        double val = s.toDouble(&ok);
        return ok ? val : 1.0; 
    }

    QString formatScaleOutput(double scale) {
        if (scale > 0.0 && scale < 1.0) {
            double inv = 1.0 / scale;
            if (std::abs(inv - std::round(inv)) < 0.0001) {
                return QString::fromLatin1("1:%1").arg(std::round(inv));
            }
        } else if (scale > 1.0) {
            if (std::abs(scale - std::round(scale)) < 0.0001) {
                return QString::fromLatin1("%1:1").arg(std::round(scale));
            }
        }
        return QString::number(scale);
    }
}

using namespace TechDrawGui;

DlgPrefsTechDrawScaleImp::DlgPrefsTechDrawScaleImp( QWidget* parent )
  : PreferencePage( parent )
  , ui(new Ui_DlgPrefsTechDrawScaleImp)
{
    ui->setupUi(this);

    ui->pdsbTemplateMark->setUnit(Base::Unit::Length);
    ui->pdsbTemplateMark->setMinimum(0);
    
    ui->cbViewCustomScale->addItems({QLatin1String("10:1"), QLatin1String("5:1"), QLatin1String("2:1"), QLatin1String("1:1"), QLatin1String("1:2"), QLatin1String("1:5"), QLatin1String("1:10"), QLatin1String("1:20"), QLatin1String("1:50"), QLatin1String("1:100")});

    connect(ui->cbViewScaleType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgPrefsTechDrawScaleImp::onScaleTypeChanged);
}

DlgPrefsTechDrawScaleImp::~DlgPrefsTechDrawScaleImp()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgPrefsTechDrawScaleImp::onScaleTypeChanged(int index)
{
    // disable custom scale if the scale type is not custom

    if (index == 2) // if custom
        ui->cbViewCustomScale->setEnabled(true);
    else
        ui->cbViewCustomScale->setEnabled(false);
}

void DlgPrefsTechDrawScaleImp::saveSettings()
{
    ui->pdsbPageScale->onSave();
    ui->cbViewScaleType->onSave();
    
    double scaleVal = parseScaleInput(ui->cbViewCustomScale->currentText());
    auto group = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw/General");
    group->SetFloat("DefaultViewScale", scaleVal);
    
    ui->pdsbVertexScale->onSave();
    ui->pdsbCenterScale->onSave();
    ui->pdsbTemplateMark->onSave();
    ui->pdsbSymbolScale->onSave();
    ui->cbLegacyScale->onSave();
    ui->cbScreenMode->onSave();
    ui->pdsbScreenVertexSize->onSave();
    ui->pdsbScreenEdgeWidth->onSave();
}

void DlgPrefsTechDrawScaleImp::loadSettings()
{
    ui->pdsbPageScale->onRestore();
    ui->cbViewScaleType->onRestore();
    
    auto group = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Mod/TechDraw/General");
    double currentScale = group->GetFloat("DefaultViewScale", 1.0);
    ui->cbViewCustomScale->setCurrentText(formatScaleOutput(currentScale));
    
    ui->pdsbVertexScale->onRestore();
    ui->pdsbCenterScale->onRestore();
    double markDefault = 3.0;
    ui->pdsbTemplateMark->setValue(markDefault);
    ui->pdsbTemplateMark->onRestore();
    ui->pdsbSymbolScale->onRestore();
    ui->cbLegacyScale->onRestore();
    ui->cbScreenMode->onRestore();
    ui->pdsbScreenVertexSize->onRestore();
    ui->pdsbScreenEdgeWidth->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgPrefsTechDrawScaleImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include <Mod/TechDraw/Gui/moc_DlgPrefsTechDrawScaleImp.cpp>
