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
}
 
DlgSettingsPDF::~DlgSettingsPDF() = default;

void DlgSettingsPDF::saveSettings()
{
    ui->comboBox->onSave();
}

void DlgSettingsPDF::loadSettings()
{
    ui->comboBox->onRestore();
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

void DlgSettingsPDF::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        ui->retranslateUi(this);
    }

    QWidget::changeEvent(e);
}
 
#include "moc_DlgSettingsPDF.cpp"
