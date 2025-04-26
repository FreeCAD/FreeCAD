#include "PreCompiled.h"

#ifndef _PreComp_
# include <QApplication>
#endif
 
#include <App/Application.h>
 
#include "DlgSettingsPDF.h"
#include "ui_DlgSettingsPDF.h"
 
 
using namespace Gui::Dialog;
 
/* TRANSLATOR Gui::Dialog::DlgSettingsPDF */
 
DlgSettingsPDF::DlgSettingsPDF(QWidget* parent)
    : PreferencePage( parent )
    , ui(new Ui_DlgSettingsPDF)
{
    ui->setupUi(this);
    ui->warningLabel->setWordWrap(true);
    connect(ui->comboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &DlgSettingsPDF::onComboBoxIndexChanged);
}
 
DlgSettingsPDF::~DlgSettingsPDF() = default;

void DlgSettingsPDF::saveSettings()
{
    ui->comboBox->onSave();
}

void DlgSettingsPDF::loadSettings()
{
    ui->comboBox->onRestore();
    onComboBoxIndexChanged(ui->comboBox->currentIndex());
}

QPagedPaintDevice::PdfVersion DlgSettingsPDF::evaluatePDFVersion()
{
    auto hGrp = App::GetApplication()
                    .GetUserParameter()
                    .GetGroup("BaseApp")
                    ->GetGroup("Preferences")
                    ->GetGroup("Mod");

    const int ver = hGrp->GetInt("PDFVersion");

    switch (ver) {
        case 1:
            return QPagedPaintDevice::PdfVersion_A1b;
        case 2:
            return QPagedPaintDevice::PdfVersion_1_6;
        case 3:
            return QPagedPaintDevice::PdfVersion_X4;
        default:
            return QPagedPaintDevice::PdfVersion_1_4;
    }
}

void DlgSettingsPDF::onComboBoxIndexChanged(int index)
{
    switch (index) {
        case 1:
            ui->warningLabel->setText(
                QCoreApplication::translate("Gui::Dialog::DlgSettingsPDF", "This archival PDF format does not support transparency or layers. All content must be self-contained and static."));
            break;
        case 2:
            ui->warningLabel->setText(
                QCoreApplication::translate("Gui::Dialog::DlgSettingsPDF", "While this version supports more modern features, older PDF readers may not fully handle it."));
            break;
        case 3:
            ui->warningLabel->setText(
                QCoreApplication::translate("Gui::Dialog::DlgSettingsPDF", "This PDF format is intended for professional printing and requires all fonts to be embedded; some interactive features may not be supported."));
            break;
        default:
            ui->warningLabel->setText(
                QCoreApplication::translate("Gui::Dialog::DlgSettingsPDF", "This PDF version has limited support for modern features like embedded multimedia and advanced transparency effects."));
            break;
    }
}

void DlgSettingsPDF::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(e);
}
 
#include "moc_DlgSettingsPDF.cpp"
