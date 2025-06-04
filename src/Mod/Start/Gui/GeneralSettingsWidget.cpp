// SPDX-License-Identifier: LGPL-2.1-or-later
/****************************************************************************
 *                                                                          *
 *   Copyright (c) 2024 The FreeCAD Project Association AISBL               *
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
#include <QApplication>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLayout>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#endif

#include <algorithm>
#include "GeneralSettingsWidget.h"
#include <gsl/pointers>
#include <App/Application.h>
#include <Base/Parameter.h>
#include <Base/UnitsApi.h>
#include <Gui/Language/Translator.h>
#include <Gui/Navigation/NavigationStyle.h>

using namespace StartGui;

GeneralSettingsWidget::GeneralSettingsWidget(QWidget* parent)
    : QWidget(parent)
    , _languageLabel {nullptr}
    , _unitSystemLabel {nullptr}
    , _navigationStyleLabel {nullptr}
    , _languageComboBox {nullptr}
    , _unitSystemComboBox {nullptr}
    , _navigationStyleComboBox {nullptr}
{
    setObjectName(QLatin1String("GeneralSettingsWidget"));
    setupUi();
    qApp->installEventFilter(this);
}

void GeneralSettingsWidget::setupUi()
{
    if (layout()) {
        qDeleteAll(findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly));
        delete layout();
    }
    _languageLabel = gsl::owner<QLabel*>(new QLabel);
    _navigationStyleLabel = gsl::owner<QLabel*>(new QLabel);
    _unitSystemLabel = gsl::owner<QLabel*>(new QLabel);
    createLanguageComboBox();
    createUnitSystemComboBox();
    createNavigationStyleComboBox();
    createHorizontalUi();
    retranslateUi();
}

void GeneralSettingsWidget::createHorizontalUi()
{
    auto mainLayout = gsl::owner<QHBoxLayout*>(new QHBoxLayout(this));
    const int extraSpace {36};
    mainLayout->addWidget(_languageLabel);
    mainLayout->addWidget(_languageComboBox);
    mainLayout->addSpacing(extraSpace);
    mainLayout->addWidget(_unitSystemLabel);
    mainLayout->addWidget(_unitSystemComboBox);
    mainLayout->addSpacing(extraSpace);
    mainLayout->addWidget(_navigationStyleLabel);
    mainLayout->addWidget(_navigationStyleComboBox);
}


QString GeneralSettingsWidget::createLabelText(const QString& translatedText) const
{
    static const auto h2Start = QLatin1String("<h2>");
    static const auto h2End = QLatin1String("</h2>");
    return h2Start + translatedText + h2End;
}

gsl::owner<QComboBox*> GeneralSettingsWidget::createLanguageComboBox()
{
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    auto langToStr = Gui::Translator::instance()->activeLanguage();
    QByteArray language = hGrp->GetASCII("Language", langToStr.c_str()).c_str();
    auto comboBox = gsl::owner<QComboBox*>(new QComboBox);
    comboBox->addItem(QStringLiteral("English"), QByteArray("English"));
    Gui::TStringMap list = Gui::Translator::instance()->supportedLocales();
    int index {1};
    for (auto it = list.begin(); it != list.end(); ++it, ++index) {
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
            if (native[0].isLetter()) {
                native[0] = native[0].toUpper();
            }
            langname = native;
        }

        comboBox->addItem(langname, lang);
        if (language == lang) {
            comboBox->setCurrentIndex(index);
        }
    }
    if (QAbstractItemModel* model = comboBox->model()) {
        model->sort(0);
    }
    _languageComboBox = comboBox;
    connect(_languageComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &GeneralSettingsWidget::onLanguageChanged);
    return comboBox;
}

gsl::owner<QComboBox*> GeneralSettingsWidget::createUnitSystemComboBox()
{
    // Contents are created in retranslateUi()
    auto comboBox = gsl::owner<QComboBox*>(new QComboBox);
    _unitSystemComboBox = comboBox;
    connect(_unitSystemComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &GeneralSettingsWidget::onUnitSystemChanged);
    return comboBox;
}

gsl::owner<QComboBox*> GeneralSettingsWidget::createNavigationStyleComboBox()
{
    // Contents are created in retranslateUi()
    auto comboBox = gsl::owner<QComboBox*>(new QComboBox);
    _navigationStyleComboBox = comboBox;
    connect(_navigationStyleComboBox,
            qOverload<int>(&QComboBox::currentIndexChanged),
            this,
            &GeneralSettingsWidget::onNavigationStyleChanged);
    return comboBox;
}

void GeneralSettingsWidget::onLanguageChanged(int index)
{
    if (index < 0) {
        return;  // happens when clearing the combo box in retranslateUi()
    }
    Gui::Translator::instance()->activateLanguage(
        _languageComboBox->itemData(index).toByteArray().data());
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/General");
    auto langToStr = Gui::Translator::instance()->activeLanguage();
    hGrp->SetASCII("Language", langToStr.c_str());
}

void GeneralSettingsWidget::onUnitSystemChanged(int index)
{
    if (index < 0) {
        return;  // happens when clearing the combo box in retranslateUi()
    }
    Base::UnitsApi::setSchema(index);
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    hGrp->SetInt("UserSchema", index);
}

void GeneralSettingsWidget::onNavigationStyleChanged(int index)
{
    if (index < 0) {
        return;  // happens when clearing the combo box in retranslateUi()
    }
    auto navStyleName = _navigationStyleComboBox->itemData(index).toByteArray();
    ParameterGrp::handle hGrp =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    hGrp->SetASCII("NavigationStyle", navStyleName.constData());
}

bool GeneralSettingsWidget::eventFilter(QObject* object, QEvent* event)
{
    if (object == this && event->type() == QEvent::LanguageChange) {
        this->retranslateUi();
    }
    return QWidget::eventFilter(object, event);
}

void GeneralSettingsWidget::retranslateUi()
{
    _languageLabel->setText(createLabelText(tr("Language")));
    _unitSystemLabel->setText(createLabelText(tr("Unit System")));

    _unitSystemComboBox->clear();

    const ParameterGrp::handle hGrpUnits =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Units");
    auto userSchema = hGrpUnits->GetInt("UserSchema", 0);

    auto addItem = [&, index {0}](const std::string& item) mutable {
        _unitSystemComboBox->addItem(QString::fromStdString(item), index++);
    };
    auto descriptions = Base::UnitsApi::getDescriptions();
    std::for_each(descriptions.begin(), descriptions.end(), addItem);

    _unitSystemComboBox->setCurrentIndex(userSchema);

    _navigationStyleLabel->setText(createLabelText(tr("Navigation Style")));
    _navigationStyleComboBox->clear();
    ParameterGrp::handle hGrpNav =
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/View");
    auto navStyleName =
        hGrpNav->GetASCII("NavigationStyle", Gui::CADNavigationStyle::getClassTypeId().getName());
    std::map<Base::Type, std::string> styles = Gui::UserNavigationStyle::getUserFriendlyNames();
    for (const auto& style : styles) {
        QByteArray data(style.first.getName());
        QString name = QApplication::translate(style.first.getName(), style.second.c_str());
        _navigationStyleComboBox->addItem(name, data);
        if (navStyleName == style.first.getName()) {
            _navigationStyleComboBox->setCurrentIndex(_navigationStyleComboBox->count() - 1);
        }
    }
}
