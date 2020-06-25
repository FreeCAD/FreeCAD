/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
#endif

#include <array>

#include <App/Material.h>
#include "DlgSettingsDrawStyles.h"
#include "ui_DlgSettingsDrawStyles.h"
#include "ViewParams.h"
#include "Application.h"
#include "Document.h"
#include "View3DInventorViewer.h"
#include "View3DInventor.h"
#include "PrefWidgets.h"

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsDrawStyles */

struct LinePattern {
    Qt::PenStyle style;
    int value;
    const char *name;

    LinePattern(Qt::PenStyle s, int v, const char *n)
        :style(s), value(v), name(n)
    {}
};
static const std::array<LinePattern, 5> _LinePatterns {{
    {Qt::SolidLine, 0, "No change"},
    {Qt::SolidLine, 0xffff, "Solid"},
    {Qt::DashLine, 0xf0f0, "Dash"},
    {Qt::DotLine, 0xaaaa, "Dot"},
    {Qt::DashDotLine, 0x33ff, "Dash Dot"},
}};

/**
 *  Constructs a DlgSettingsDrawStyles which is a child of 'parent', with the 
 *  name 'name' and widget flags set to 'f' 
 */
DlgSettingsDrawStyles::DlgSettingsDrawStyles(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsDrawStyles)
{
    ui->setupUi(this);

    ui->comboLinePattern->setIconSize (QSize(80,12));
    for(auto &v : _LinePatterns) {
        if(!v.value) {
            ui->comboLinePattern->addItem(QString::fromLatin1(v.name),v.value);
            continue;
        }
        QPixmap px(ui->comboLinePattern->iconSize());
        px.fill(Qt::white);
        QBrush brush(Qt::black);
        QPen pen(v.style);
        pen.setBrush(brush);
        pen.setWidth(2);

        QPainter painter(&px);
        painter.setPen(pen);
        double mid = ui->comboLinePattern->iconSize().height() / 2.0;
        painter.drawLine(0, mid, ui->comboLinePattern->iconSize().width(), mid);
        painter.end();

        ui->comboLinePattern->addItem(QIcon(px),QString::fromLatin1(v.name),v.value);
    }
    ui->comboLinePattern->setEditable(true);
    
    int pattern = ViewParams::getSelectionLinePattern();
    bool found = false;
    int i=-1;
    for(auto &v : _LinePatterns) {
        ++i;
        if(pattern == v.value) {
            found = true;
            ui->comboLinePattern->setCurrentIndex(i);
            break;
        }
    }
    if(!found) {
        ui->comboLinePattern->setEditText(
                QString::fromLatin1("0x%1").arg(pattern,0,16));
    }

    ui->ShadowDisplayMode->addItem(tr("Flat Lines"));
    ui->ShadowDisplayMode->addItem(tr("Shaded"));
    ui->ShadowDisplayMode->addItem(tr("As Is"));

#define FC_DRAW_STYLE_PARAMS \
    FC_DRAW_STYLE_PARAM(HiddenLineTransparency, value, setValue) \
    FC_DRAW_STYLE_PARAM(ShowSelectionOnTop, isChecked, setChecked) \
    FC_DRAW_STYLE_PARAM(HiddenLineShaded, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(HiddenLineOverrideColor, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(HiddenLineOverrideFaceColor, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(HiddenLineOverrideBackground, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(TransparencyOnTop, value, setValue)\
    FC_DRAW_STYLE_PARAM(SelectionLineThicken, value, setValue)\
    FC_DRAW_STYLE_PARAM(SelectionHiddenLineWidth, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowSpotLight, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(ShadowShowGround, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(ShadowDisplayMode, currentIndex, setCurrentIndex)\
    FC_DRAW_STYLE_PARAM(ShadowGroundShading, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(ShadowGroundBackFaceCull, isChecked, setChecked)\
    FC_DRAW_STYLE_PARAM(ShadowLightIntensity, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowGroundScale, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowSmoothBorder, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowSpreadSize, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowSpreadSampleSize, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowEpsilon, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowThreshold, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowPrecision, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowBoundBoxScale, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowGroundTransparency, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowMaxDistance, value, setValue)\
    FC_DRAW_STYLE_PARAM2(ShadowLightColor, color, setColor)\
    FC_DRAW_STYLE_PARAM2(ShadowGroundColor, color, setColor)\
    FC_DRAW_STYLE_PARAM2(HiddenLineColor, color, setColor)\
    FC_DRAW_STYLE_PARAM2(HiddenLineFaceColor, color, setColor)\
    FC_DRAW_STYLE_PARAM2(HiddenLineBackground, color, setColor)\
    FC_DRAW_STYLE_PARAM3(ShadowGroundTexture, fileName, setFileName)\
    FC_DRAW_STYLE_PARAM3(ShadowGroundBumpMap, fileName, setFileName)\
    FC_DRAW_STYLE_PARAM(ShadowGroundTextureSize, value, setValue)\
    FC_DRAW_STYLE_PARAM(ShadowTransparentShadow, isChecked, setChecked)\

#undef FC_DRAW_STYLE_PARAM
#define FC_DRAW_STYLE_PARAM(_name, _getter, _setter) do {\
        QString tooltip = QCoreApplication::translate("ViewParams", ViewParams::doc##_name()); \
        if (!tooltip.isEmpty()) {\
            ui->_name->setToolTip(tooltip); \
            if (QLabel *child = findChild<QLabel*>(QString::fromLatin1("label" #_name))) \
                child->setToolTip(tooltip);\
        }\
    }while(0);

#undef FC_DRAW_STYLE_PARAM2
#define FC_DRAW_STYLE_PARAM2 FC_DRAW_STYLE_PARAM

#undef FC_DRAW_STYLE_PARAM3
#define FC_DRAW_STYLE_PARAM3 FC_DRAW_STYLE_PARAM

    FC_DRAW_STYLE_PARAMS;

#undef FC_DRAW_STYLE_PARAM
#define FC_DRAW_STYLE_PARAM(_name, _getter, _setter) \
    ui->_name->_setter(ViewParams::get##_name());\

#undef FC_DRAW_STYLE_PARAM2
#define FC_DRAW_STYLE_PARAM2(_name, _getter, _setter) \
    ui->_name->_setter(App::Color((uint32_t)ViewParams::get##_name()).asValue<QColor>());\

#undef FC_DRAW_STYLE_PARAM3
#define FC_DRAW_STYLE_PARAM3(_name, _getter, _setter) \
    ui->_name->_setter(QString::fromUtf8(ViewParams::get##_name().c_str()));\

    FC_DRAW_STYLE_PARAMS;

    ui->HiddenLineColor->setEnabled(ui->HiddenLineOverrideColor->isChecked());
    ui->HiddenLineFaceColor->setEnabled(ui->HiddenLineOverrideFaceColor->isChecked());
    ui->HiddenLineBackground->setEnabled(ui->HiddenLineOverrideBackground->isChecked());
}

/** 
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsDrawStyles::~DlgSettingsDrawStyles()
{
    // no need to delete child widgets, Qt does it all for us
}

void DlgSettingsDrawStyles::loadSettings()
{
    int pattern = ViewParams::getSelectionLinePattern();
    bool found = false;
    int i=-1;
    for(auto &v : _LinePatterns) {
        ++i;
        if(pattern == v.value) {
            ui->comboLinePattern->setCurrentIndex(i);
            found = true;
            break;
        }
    }
    if(!found)
        ui->comboLinePattern->setEditText(QString::fromLatin1("0x%1").arg(pattern));

    FC_DRAW_STYLE_PARAMS;
}

void DlgSettingsDrawStyles::saveSettings()
{
    int idx = ui->comboLinePattern->currentIndex();
    if(idx >= 0) {
        ViewParams::setSelectionLinePattern(
                ui->comboLinePattern->itemData(idx).toInt());
    } else {
        QString pattern = ui->comboLinePattern->currentText();
        bool res;
        ViewParams::setSelectionLinePattern(pattern.toInt(&res,0));
        if(!res) {
            Base::Console().Warning("Invalid line pattern: %s'\n",
                    pattern.toLatin1().constData());
        }
    }

#undef FC_DRAW_STYLE_PARAM
#define FC_DRAW_STYLE_PARAM(_name, _getter, _setter) \
    ViewParams::set##_name(ui->_name->_getter());\

#undef FC_DRAW_STYLE_PARAM2
#define FC_DRAW_STYLE_PARAM2(_name, _getter, _setter) do {\
        App::Color color;\
        color.setValue(ui->_name->_getter());\
        ViewParams::set##_name(color.getPackedValue());\
    }while(0);

#undef FC_DRAW_STYLE_PARAM3
#define FC_DRAW_STYLE_PARAM3(_name, _getter, _setter) \
    ViewParams::set##_name(ui->_name->_getter().toUtf8().constData());\

    FC_DRAW_STYLE_PARAMS;

    for(auto doc : App::GetApplication().getDocuments()) {
        for(auto v : Application::Instance->getDocument(doc)->getMDIViews()) {
            View3DInventor* view = qobject_cast<View3DInventor*>(v);
            if(!view)
                continue;
            auto viewer = view->getViewer();
            if(viewer->getOverrideMode() != "As Is")
                viewer->applyOverrideMode();
        }
    }
}


/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsDrawStyles::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }
    else {
        QWidget::changeEvent(e);
    }
}

#include "moc_DlgSettingsDrawStyles.cpp"

