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

#include <zlib.h>

#include "DlgSettingsDocumentImp.h"
#include "ui_DlgSettingsDocument.h"
#include "AutoSaver.h"
#include <App/License.h>


using namespace Gui::Dialog;

/* TRANSLATOR Gui::Dialog::DlgSettingsDocumentImp */

/**
 *  Constructs a DlgSettingsDocumentImp which is a child of 'parent', with the
 *  name 'name' and widget flags set to 'f'
 */
DlgSettingsDocumentImp::DlgSettingsDocumentImp( QWidget* parent )
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsDocument)
{
    ui->setupUi(this);
    addLicenseTypes();

    ui->prefSaveTransaction->hide();
    ui->prefDiscardTransaction->hide();

    QString tip = QString::fromLatin1("<html><head/><body><p>%1</p>"
                                      "<p>%2: %Y%m%d-%H%M%S</p>"
                                      "<p>%3: <a href=\"http://www.cplusplus.com/reference/ctime/strftime/\">C++ strftime</a>"
                                      "</p></body></html>").arg(tr("The format of the date to use."), tr("Default"), tr("Format"));
    ui->prefSaveBackupDateFormat->setToolTip(tip);

    ui->prefCountBackupFiles->setMaximum(INT_MAX);
    ui->prefCompression->setMinimum(Z_NO_COMPRESSION);
    ui->prefCompression->setMaximum(Z_BEST_COMPRESSION);
    connect(ui->prefLicenseType, qOverload<int>(&QComboBox::currentIndexChanged),
            this, &DlgSettingsDocumentImp::onLicenseTypeChanged);
}

/**
 *  Destroys the object and frees any allocated resources
 */
DlgSettingsDocumentImp::~DlgSettingsDocumentImp()
{
    // no need to delete child widgets, Qt does it all for us
}


void DlgSettingsDocumentImp::saveSettings()
{
    ui->prefCheckNewDoc->onSave();
    ui->prefCompression->onSave();

    ui->prefUndoRedo->onSave();
    ui->prefUndoRedoSize->onSave();
    ui->prefSaveTransaction->onSave();
    ui->prefDiscardTransaction->onSave();
    ui->prefSaveThumbnail->onSave();
    ui->prefThumbnailSize->onSave();
    ui->prefAddLogo->onSave();
    ui->prefSaveBackupFiles->onSave();
    ui->prefCountBackupFiles->onSave();
    ui->prefSaveBackupExtension->onSave();
    ui->prefSaveBackupDateFormat->onSave();
    ui->prefDuplicateLabel->onSave();
    ui->prefPartialLoading->onSave();
    ui->prefLicenseType->onSave();
    ui->prefLicenseUrl->onSave();
    ui->prefAuthor->onSave();
    ui->prefSetAuthorOnSave->onSave();
    ui->prefCompany->onSave();
    ui->prefRecovery->onSave();
    ui->prefAutoSaveEnabled->onSave();
    ui->prefAutoSaveTimeout->onSave();
    ui->prefCanAbortRecompute->onSave();

    int timeout = ui->prefAutoSaveTimeout->value();
    if (!ui->prefAutoSaveEnabled->isChecked())
        timeout = 0;
    AutoSaver::instance()->setTimeout(timeout * 60000);
}

void DlgSettingsDocumentImp::loadSettings()
{
    ui->prefCheckNewDoc->onRestore();
    ui->prefCompression->onRestore();

    ui->prefUndoRedo->onRestore();
    ui->prefUndoRedoSize->onRestore();
    ui->prefSaveTransaction->onRestore();
    ui->prefDiscardTransaction->onRestore();
    ui->prefSaveThumbnail->onRestore();
    ui->prefThumbnailSize->onRestore();
    ui->prefAddLogo->onRestore();
    ui->prefSaveBackupFiles->onRestore();
    ui->prefCountBackupFiles->onRestore();
    ui->prefSaveBackupExtension->onRestore();
    ui->prefSaveBackupDateFormat->onRestore();
    ui->prefDuplicateLabel->onRestore();
    ui->prefPartialLoading->onRestore();
    ui->prefLicenseType->onRestore();
    ui->prefLicenseUrl->onRestore();
    ui->prefAuthor->onRestore();
    ui->prefSetAuthorOnSave->onRestore();
    ui->prefCompany->onRestore();
    ui->prefRecovery->onRestore();
    ui->prefAutoSaveEnabled->onRestore();
    ui->prefAutoSaveTimeout->onRestore();
    ui->prefCanAbortRecompute->onRestore();
}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsDocumentImp::changeEvent(QEvent *e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
        int index = ui->prefLicenseType->currentIndex();
        addLicenseTypes();
        ui->prefLicenseType->setCurrentIndex(index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsDocumentImp::addLicenseTypes()
{
    ui->prefLicenseType->clear();
    auto rawLicenses = App::License::getLicenses();
    for (const auto& it : rawLicenses) {
        QString text = QApplication::translate("Gui::Dialog::DlgSettingsDocument", it.c_str());
        ui->prefLicenseType->addItem(text);
    }
}

/**
 * Set the correct URL depending on the license type
 */
void DlgSettingsDocumentImp::onLicenseTypeChanged(int index)
{
    App::License license{index};
    std::string url = license.getUrl();
    if (license.getType() == App::License::Type::Other) {
        ui->prefLicenseUrl->clear();
        ui->prefLicenseUrl->setReadOnly(false);
    }
    else {
        ui->prefLicenseUrl->setReadOnly(true);
        ui->prefLicenseUrl->setText(QString::fromStdString(url));
    }
}

#include "moc_DlgSettingsDocumentImp.cpp"
