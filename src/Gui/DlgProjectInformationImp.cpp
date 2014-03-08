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

#include <App/Document.h>
#include <App/PropertyStandard.h>

#include "DlgProjectInformationImp.h"
#include "ui_DlgProjectInformation.h"
#include "Document.h"
#include <QUrl>
#include <QDesktopServices>

using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgProjectInformationImp */

/**
 *  Constructs a Gui::Dialog::DlgProjectInformationImp as a child of 'parent', with the
 *  name 'name' and widget flags set to 'fl'.
 *
 *  The dialog will by default be modeless, unless you set 'modal' to
 *  TRUE to construct a modal dialog.
 */
DlgProjectInformationImp::DlgProjectInformationImp(App::Document* doc, QWidget* parent, Qt::WFlags fl)
  : QDialog(parent, fl), _doc(doc), ui(new Ui_DlgProjectInformation)
{
    ui->setupUi(this);
    ui->lineEditName->setText(QString::fromUtf8(doc->Label.getValue()));
    ui->lineEditPath->setText(QString::fromUtf8(doc->FileName.getValue()));
    ui->lineEditUuid->setText(QString::fromUtf8(doc->Uid.getValueStr().c_str()));
    ui->lineEditCreator->setText(QString::fromUtf8(doc->CreatedBy.getValue()));
    ui->lineEditDate->setText(QString::fromUtf8(doc->CreationDate.getValue()));
    ui->lineEditLastMod->setText(QString::fromUtf8(doc->LastModifiedBy.getValue()));
    ui->lineEditLastModDate->setText(QString::fromUtf8(doc->LastModifiedDate.getValue()));
    ui->lineEditCompany->setText(QString::fromUtf8(doc->Company.getValue()));
    ui->lineEditLicense->setText(QString::fromUtf8(doc->License.getValue()));
    ui->lineEditLicenseURL->setText(QString::fromUtf8(doc->LicenseURL.getValue()));

    // When saving the text to XML the newlines get lost. So we store also the newlines as '\n'.
    // See also accept().
    QString comment = QString::fromUtf8(doc->Comment.getValue());
    QStringList lines = comment.split(QLatin1String("\\n"), QString::KeepEmptyParts);
    QString text = lines.join(QLatin1String("\n"));
    ui->textEditComment->setPlainText( text );
    connect(ui->pushButtonOpenURL, SIGNAL(clicked()),this, SLOT(open_url()));
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
    _doc->License.setValue(ui->lineEditLicense->text().toUtf8());
    _doc->LicenseURL.setValue(ui->lineEditLicenseURL->text().toUtf8());

    // Replace newline escape sequence trough '\\n' string
    QStringList lines = ui->textEditComment->toPlainText().split
        (QLatin1String("\n"), QString::KeepEmptyParts);
    QString text = lines.join(QLatin1String("\\n"));
    _doc->Comment.setValue(text.isEmpty() ? "" : text.toUtf8());

    QDialog::accept();
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
