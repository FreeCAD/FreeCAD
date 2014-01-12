#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <QDialog>
#include "PreCompiled.h"
#include <Mod/Spreadsheet/App/Sheet.h>

namespace Ui {
class PropertiesDialog;
}

namespace SpreadsheetGui {

class PropertiesDialog : public QDialog
{
    Q_OBJECT
    
public:
    explicit PropertiesDialog(Spreadsheet::Sheet *_sheet, const std::vector<Spreadsheet::Sheet::Range> & _ranges, QWidget *parent = 0);
    ~PropertiesDialog();
    
    void apply();

private Q_SLOTS:
    void foregroundColorChanged(const QColor &color);
    void backgroundColorChanged(const QColor &color);
    void alignmentChanged();
    void styleChanged();
    void displayUnitChanged(const QString &text);
private:
    Spreadsheet::Sheet * sheet;
    std::vector<Spreadsheet::Sheet::Range> ranges;
    Ui::PropertiesDialog *ui;
    App::Color foregroundColor;
    App::Color backgroundColor;
    int alignment;
    std::set<std::string> style;
    Spreadsheet::Sheet::DisplayUnit displayUnit;

    App::Color orgForegroundColor;
    App::Color orgBackgroundColor;
    int orgAlignment;
    std::set<std::string> orgStyle;
    Spreadsheet::Sheet::DisplayUnit orgDisplayUnit;
};

}

#endif // PROPERTIESDIALOG_H
