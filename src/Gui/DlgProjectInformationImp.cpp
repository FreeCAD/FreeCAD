/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <QApplication>
#include <QByteArray>
#include <QDesktopServices>
#include <QUrl>
#endif

#include <App/Document.h>
#include <App/License.h>

#include "DlgProjectInformationImp.h"
#include "ui_DlgProjectInformation.h"


#if 0 // needed for Qt's lupdate utility
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "All rights reserved");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution-ShareAlike");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution-NoDerivatives");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution-NonCommercial");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution-NonCommercial-ShareAlike");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Creative Commons Attribution-NonCommercial-NoDerivatives");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Public Domain");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "FreeArt");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "CERN Open Hardware Licence strongly-reciprocal");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "CERN Open Hardware Licence weakly-reciprocal");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "CERN Open Hardware Licence permissive");
    qApp->translate("Gui::Dialog::DlgSettingsDocument", "Other");
#endif

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgProjectInformationImp */

/**
 *  Constructs a Gui::Dialog::DlgProjectInformationImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'fl'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  true to construct a modal dialog.
 */
DlgProjectInformationImp::DlgProjectInformationImp(App::Document* doc, QWidget* parent, Qt::WindowFlags fl)
  : QDialog(parent, fl), _doc(doc), ui(new Ui_DlgProjectInformation)
{
    ui->setupUi(this);
    ui->lineEditName->setText(QString::fromUtf8(doc->Label.getValue()));
    ui->lineEditPath->setText(QString::fromUtf8(doc->FileName.getValue()));
    ui->lineEditUuid->setText(QString::fromUtf8(doc->Uid.getValueStr().c_str()));
    ui->lineEditProgramVersion->setText(QString::fromUtf8(doc->getProgramVersion()));
    ui->lineEditCreator->setText(QString::fromUtf8(doc->CreatedBy.getValue()));
    ui->lineEditDate->setText(QString::fromUtf8(doc->CreationDate.getValue()));
    ui->lineEditLastMod->setText(QString::fromUtf8(doc->LastModifiedBy.getValue()));
    ui->lineEditLastModDate->setText(QString::fromUtf8(doc->LastModifiedDate.getValue()));
    ui->lineEditCompany->setText(QString::fromUtf8(doc->Company.getValue()));

    // load comboBox with license names
    for (const auto& item : App::licenseItems) {
        const char* name {item.at(App::posnOfFullName)};
        QString translated = QApplication::translate("Gui::Dialog::DlgSettingsDocument", name);
        ui->comboLicense->addItem(translated, QByteArray(name));
    }

    // set default position to match document
    int index = ui->comboLicense->findData(QByteArray(doc->License.getValue()));
    if (index >= 0) {
        ui->comboLicense->setCurrentIndex(index);
    }
    else {
        index = ui->comboLicense->count();
        QString text = QString::fromUtf8(doc->License.getValue());
        ui->comboLicense->addItem(text);
        ui->comboLicense->setCurrentIndex(index);
    }

    ui->lineEditLicenseURL->setText(QString::fromUtf8(doc->LicenseURL.getValue()));

    // When saving the text to XML the newlines get lost. So we store also the newlines as '\n'.
    // See also accept().
    QString comment = QString::fromUtf8(doc->Comment.getValue());
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
    QStringList lines = comment.split(QLatin1String("\\n"), Qt::KeepEmptyParts);
#else
    QStringList lines = comment.split(QLatin1String("\\n"), QString::KeepEmptyParts);
#endif
    QString text = lines.join(QLatin1String("\n"));
    ui->textEditComment->setPlainText( text );
    connect(ui->pushButtonOpenURL, &QPushButton::clicked,
            this, &DlgProjectInformationImp::open_url);
    connect(ui->comboLicense, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgProjectInformationImp::onLicenseTypeChanged);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgProjectInformationImp::~DlgProjectInformationImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

/**
 * Applies the changes to the project information of the given document.
 */
void DlgProjectInformationImp::accept()
{
    _doc->CreatedBy.setValue(ui->lineEditCreator->text().toUtf8());
    _doc->LastModifiedBy.setValue(ui->lineEditCreator->text().toUtf8());
    _doc->Company.setValue(ui->lineEditCompany->text().toUtf8());
    QByteArray licenseName {ui->comboLicense->currentData().toByteArray()};
    // Is this really necessary?
    if (licenseName.isEmpty()) {
        licenseName = ui->comboLicense->currentText().toUtf8();
    }
    _doc->License.setValue(licenseName);
    _doc->LicenseURL.setValue(ui->lineEditLicenseURL->text().toUtf8());

    // Replace newline escape sequence through '\\n' string
    QStringList lines = ui->textEditComment->toPlainText().split
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
        (QLatin1String("\n"), Qt::KeepEmptyParts);
#else
        (QLatin1String("\n"), QString::KeepEmptyParts);
#endif
    QString text = lines.join(QLatin1String("\\n"));
    _doc->Comment.setValue(text.isEmpty() ? "" : text.toUtf8());

    QDialog::accept();
}

void DlgProjectInformationImp::onLicenseTypeChanged(int index)
{
    const char* url {index >= 0 && index < App::countOfLicenses ? App::licenseItems.at(index).at(App::posnOfUrl)
                                                  : _doc->LicenseURL.getValue()};

    ui->lineEditLicenseURL->setText(QString::fromLatin1(url));
}

/**
 * Opens the text in the LicenseURL property in external browser.
 */
void DlgProjectInformationImp::open_url()
{
    QString url = ui->lineEditLicenseURL->text();
    QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

#include "moc_DlgProjectInformationImp.cpp"
