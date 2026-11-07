// SPDX-License-Identifier: LGPL-2.1-or-later

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

#include <QApplication>
#include <QByteArray>
#include <QDateTime>
#include <QDesktopServices>
#include <QLocale>
#include <QUrl>

#include <App/Application.h>
#include <App/Document.h>
#include <App/License.h>
#include <App/PropertyStandard.h>
#include <Base/UnitsApi.h>

#include "Dialogs/DlgProjectInformationImp.h"
#include "ui_DlgProjectInformation.h"

#include "MainWindow.h"

#if 0  // needed for Qt's lupdate utility
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
    : QDialog(parent, fl)
    , _doc(doc)
    , ui(new Ui_DlgProjectInformation)
{
    auto convertISODate = [](const char* isoDate) {
        auto str = QString::fromUtf8(isoDate);
        QDateTime dt = QDateTime::fromString(str, Qt::DateFormat::ISODate);
        if (dt.isNull()) {
            return str;
        }

        dt = dt.toLocalTime();
        QLocale loc = QLocale::system();
        return loc.toString(dt);
    };
    ui->setupUi(this);
    ui->lineEditName->setText(QString::fromUtf8(doc->Label.getValue()));
    ui->lineEditPath->setText(QString::fromUtf8(doc->FileName.getValue()));
    ui->lineEditUuid->setText(QString::fromUtf8(doc->Uid.getValueStr().c_str()));
    // A document that has never been saved or loaded carries no program version yet,
    // so show the version that saving it would write.
    QString programVersion = QString::fromUtf8(doc->getProgramVersion());
    if (programVersion.isEmpty()) {
        auto config = App::Application::Config();
        programVersion = QStringLiteral("%1.%2R%3")
                             .arg(
                                 QString::fromStdString(config["BuildVersionMajor"]),
                                 QString::fromStdString(config["BuildVersionMinor"]),
                                 QString::fromStdString(config["BuildRevision"])
                             );
    }
    ui->lineEditProgramVersion->setText(programVersion);
    ui->lineEditCreator->setText(QString::fromUtf8(doc->CreatedBy.getValue()));
    ui->lineEditDate->setText(convertISODate(doc->CreationDate.getValue()));
    ui->lineEditLastMod->setText(QString::fromUtf8(doc->LastModifiedBy.getValue()));
    ui->lineEditLastModDate->setText(convertISODate(doc->LastModifiedDate.getValue()));
    ui->lineEditCompany->setText(QString::fromUtf8(doc->Company.getValue()));

    // Load comboBox with unit systems
    auto addDesc = [&, index {0}](const std::string& item) mutable {
        ui->comboBox_unitSystem->addItem(QString::fromStdString(item), index++);
    };
    const auto descriptions = Base::UnitsApi::getDescriptions();
    std::for_each(descriptions.begin(), descriptions.end(), addDesc);

    ui->comboBox_unitSystem->setCurrentIndex(doc->UnitSystem.getValue());

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

    QStringList lines = comment.split(QLatin1String("\\n"), Qt::KeepEmptyParts);

    QString text = lines.join(QLatin1String("\n"));
    ui->textEditComment->setPlainText(text);

    // Only shown when the dialog is opened for a newly created document
    ui->widgetNewDocumentOptions->hide();
    ui->checkBoxDontShowAgain->hide();

    connect(ui->pushButtonOpenURL, &QPushButton::clicked, this, &DlgProjectInformationImp::open_url);
    connect(
        ui->comboLicense,
        qOverload<int>(&QComboBox::currentIndexChanged),
        this,
        &DlgProjectInformationImp::onLicenseTypeChanged
    );
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgProjectInformationImp::~DlgProjectInformationImp()
{
    // no need to delete child widgets, Qt does it all for us
    delete ui;
}

void DlgProjectInformationImp::setNewDocumentMode()
{
    _newDocumentMode = true;
    ui->widgetNewDocumentOptions->show();
    ui->checkBoxDontShowAgain->show();

    // Nothing in this section is known yet: the document has not been written, so it
    // has no path, no name of its own and no modification history
    ui->groupBoxFileInfo->hide();
}

void DlgProjectInformationImp::showForNewDocument(App::Document* doc)
{
    // Skip if no GUI (headless/scripted mode)
    if (!doc || !getMainWindow()) {
        return;
    }

    if (!App::GetApplication()
             .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
             ->GetBool("AskProjectInfoOnNewDocument", true)) {
        return;
    }

    DlgProjectInformationImp dlg(doc, getMainWindow());
    dlg.setNewDocumentMode();

    // Dismissing the dialog just leaves the metadata at its defaults
    dlg.exec();
}

/**
 * Applies the changes to the project information of the given document. Properties are
 * only written when their value actually changed, so that confirming the dialog without
 * editing anything does not mark the document as modified.
 */
void DlgProjectInformationImp::accept()
{
    auto setIfChanged = [](App::PropertyString& property, const QByteArray& value) {
        if (value != property.getValue()) {
            property.setValue(value.constData());
        }
    };

    const QByteArray creator {ui->lineEditCreator->text().toUtf8()};
    setIfChanged(_doc->CreatedBy, creator);
    setIfChanged(_doc->LastModifiedBy, creator);
    setIfChanged(_doc->Company, ui->lineEditCompany->text().toUtf8());
    getMainWindow()->setUserSchema(ui->comboBox_unitSystem->currentIndex());
    QByteArray licenseName {ui->comboLicense->currentData().toByteArray()};
    // Is this really necessary?
    if (licenseName.isEmpty()) {
        licenseName = ui->comboLicense->currentText().toUtf8();
    }
    setIfChanged(_doc->License, licenseName);
    setIfChanged(_doc->LicenseURL, ui->lineEditLicenseURL->text().toUtf8());

    // Replace newline escape sequence through '\\n' string
    QStringList lines
        = ui->textEditComment->toPlainText().split(QLatin1String("\n"), Qt::KeepEmptyParts);

    QString text = lines.join(QLatin1String("\\n"));
    setIfChanged(_doc->Comment, text.isEmpty() ? QByteArray() : text.toUtf8());

    applyNewDocumentOptions();

    QDialog::accept();
}

void DlgProjectInformationImp::reject()
{
    applyDontShowAgain();

    QDialog::reject();
}

/**
 * Stores the creator, the company and the license as defaults for new documents, and
 * suppresses this dialog for future new documents, according to the options the user
 * selected. Each option is applied independently of the others.
 */
void DlgProjectInformationImp::applyNewDocumentOptions()
{
    if (!_newDocumentMode) {
        return;
    }

    ParameterGrp::handle paramGrp {
        App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
    };

    if (ui->checkBoxRememberAuthor->isChecked()) {
        paramGrp->SetASCII("prefAuthor", ui->lineEditCreator->text().toUtf8());
    }

    if (ui->checkBoxRememberCompany->isChecked()) {
        paramGrp->SetASCII("prefCompany", ui->lineEditCompany->text().toUtf8());
    }

    if (ui->checkBoxRememberLicense->isChecked()) {
        // A license the document carries but that FreeCAD does not know about is appended
        // after the known ones, and has no index that the preference can refer to.
        const int index = ui->comboLicense->currentIndex();
        if (index >= 0 && index < App::countOfLicenses) {
            paramGrp->SetInt("prefLicenseType", index);
            paramGrp->SetASCII("prefLicenseUrl", ui->lineEditLicenseURL->text().toUtf8());
        }
    }

    applyDontShowAgain();
}

void DlgProjectInformationImp::applyDontShowAgain()
{
    if (!_newDocumentMode || !ui->checkBoxDontShowAgain->isChecked()) {
        return;
    }

    App::GetApplication()
        .GetParameterGroupByPath("User parameter:BaseApp/Preferences/Document")
        ->SetBool("AskProjectInfoOnNewDocument", false);
}

void DlgProjectInformationImp::onLicenseTypeChanged(int index)
{
    const char* url {
        index >= 0 && index < App::countOfLicenses ? App::licenseItems.at(index).at(App::posnOfUrl)
                                                   : _doc->LicenseURL.getValue()
    };

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
