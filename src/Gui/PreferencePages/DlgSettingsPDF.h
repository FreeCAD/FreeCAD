#pragma once

#include <Gui/PropertyPage.h>
#include <QPagedPaintDevice>
#include <memory>

namespace Gui
{
namespace Dialog
{
class Ui_DlgSettingsPDF;

/**
 * The DlgSettingsPDF class implements a preference page to change settings
 * for the PDF Import-Export.
 */
class GuiExport DlgSettingsPDF: public PreferencePage
{
    Q_OBJECT

public:
    explicit DlgSettingsPDF(QWidget* parent = nullptr);
    ~DlgSettingsPDF() override;

    void saveSettings() override;
    void loadSettings() override;
    static QPagedPaintDevice::PdfVersion evaluatePDFVersion();

protected:
    void changeEvent(QEvent* e) override;

private:
    void onComboBoxIndexChanged(int index);

    std::unique_ptr<Ui_DlgSettingsPDF> ui;

    //      Q_DISABLE_COPY_MOVE(DlgSettingsPDF)
};

}  // namespace Dialog
}  // namespace Gui
