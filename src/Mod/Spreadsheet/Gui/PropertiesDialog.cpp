#include "PropertiesDialog.h"
#include <Mod/Spreadsheet/App/Expression.h>
#include <Gui/Command.h>
#include "ui_PropertiesDialog.h"

using namespace Spreadsheet;
using namespace SpreadsheetGui;

PropertiesDialog::PropertiesDialog(Sheet *_sheet, const std::vector<Sheet::Range> &_ranges, QWidget *parent) :
    QDialog(parent),
    sheet(_sheet),
    ranges(_ranges),
    ui(new Ui::PropertiesDialog),
    alignment(0)
{
    ui->setupUi(this);
    ui->foregroundColor->setStandardColors();
    ui->backgroundColor->setStandardColors();

    assert(ranges.size() > 0);
    Sheet::Range range = ranges[0];

    sheet->getCell(range.row(), range.column())->getForeground(foregroundColor);
    sheet->getCell(range.row(), range.column())->getBackground(backgroundColor);
    sheet->getCell(range.row(), range.column())->getAlignment(alignment);
    sheet->getCell(range.row(), range.column())->getStyle(style);
    sheet->getCell(range.row(), range.column())->getDisplayUnit(displayUnit);

    orgForegroundColor = foregroundColor;
    orgBackgroundColor = backgroundColor;
    orgAlignment = alignment;
    orgStyle = style;
    orgDisplayUnit = displayUnit;

    ui->foregroundColor->setCurrentColor(QColor::fromRgbF(foregroundColor.r,
                                                          foregroundColor.g,
                                                          foregroundColor.b,
                                                          foregroundColor.a));
    ui->backgroundColor->setCurrentColor(QColor::fromRgbF(backgroundColor.r,
                                                          backgroundColor.g,
                                                          backgroundColor.b,
                                                          backgroundColor.a));

    if (alignment & Sheet::CellContent::ALIGNMENT_LEFT)
        ui->alignLeft->setChecked(true);
    else if (alignment & Sheet::CellContent::ALIGNMENT_HCENTER)
        ui->alignHCenter->setChecked(true);
    else if (alignment & Sheet::CellContent::ALIGNMENT_RIGHT)
        ui->alignRight->setChecked(true);

    if (alignment & Sheet::CellContent::ALIGNMENT_TOP)
        ui->alignTop->setChecked(true);
    else if (alignment & Sheet::CellContent::ALIGNMENT_VCENTER)
        ui->alignVCenter->setChecked(true);
    else if (alignment & Sheet::CellContent::ALIGNMENT_BOTTOM)
        ui->alignBottom->setChecked(true);

    if (style.find("bold") != style.end())
        ui->styleBold->setChecked(true);
    if (style.find("italic") != style.end())
        ui->styleItalic->setChecked(true);
    if (style.find("underline") != style.end())
        ui->styleUnderline->setChecked(true);

    ui->displayUnit->setText(QString::fromStdString(displayUnit.stringRep));

    // Colors
    connect(ui->foregroundColor, SIGNAL(colorChanged(QColor)), this, SLOT(foregroundColorChanged(QColor)));
    connect(ui->backgroundColor, SIGNAL(colorChanged(QColor)), this, SLOT(backgroundColorChanged(QColor)));

    // Alignment
    connect(ui->alignLeft, SIGNAL(clicked()), this, SLOT(alignmentChanged()));
    connect(ui->alignRight, SIGNAL(clicked()), this, SLOT(alignmentChanged()));
    connect(ui->alignHCenter, SIGNAL(clicked()), this, SLOT(alignmentChanged()));
    connect(ui->alignTop, SIGNAL(clicked()), this, SLOT(alignmentChanged()));
    connect(ui->alignVCenter, SIGNAL(clicked()), this, SLOT(alignmentChanged()));
    connect(ui->alignBottom, SIGNAL(clicked()), this, SLOT(alignmentChanged()));

    // Style
    connect(ui->styleBold, SIGNAL(clicked()), this, SLOT(styleChanged()));
    connect(ui->styleItalic, SIGNAL(clicked()), this, SLOT(styleChanged()));
    connect(ui->styleUnderline, SIGNAL(clicked()), this, SLOT(styleChanged()));

    // Display unit
    connect(ui->displayUnit, SIGNAL(textEdited(QString)), this, SLOT(displayUnitChanged(QString)));
}

void PropertiesDialog::foregroundColorChanged(const QColor & color)
{
    foregroundColor = App::Color(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void PropertiesDialog::backgroundColorChanged(const QColor & color)
{
    backgroundColor = App::Color(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void PropertiesDialog::alignmentChanged()
{
    if (sender() == ui->alignLeft)
        alignment = (alignment & 0xf0) | Sheet::CellContent::ALIGNMENT_LEFT;
    else if (sender() == ui->alignHCenter)
        alignment = (alignment & 0xf0) | Sheet::CellContent::ALIGNMENT_HCENTER;
    else if (sender() == ui->alignRight)
        alignment = (alignment & 0xf0) | Sheet::CellContent::ALIGNMENT_RIGHT;
    else if (sender() == ui->alignTop)
        alignment = (alignment & 0x0f) | Sheet::CellContent::ALIGNMENT_TOP;
    else if (sender() == ui->alignVCenter)
        alignment = (alignment & 0x0f) | Sheet::CellContent::ALIGNMENT_VCENTER;
    else if (sender() == ui->alignBottom)
        alignment = (alignment & 0x0f) | Sheet::CellContent::ALIGNMENT_BOTTOM;
}

void PropertiesDialog::styleChanged()
{
    if (sender() == ui->styleBold) {
        if (ui->styleBold->isChecked())
            style.insert("bold");
        else
            style.erase("bold");
    }
    else if (sender() == ui->styleItalic) {
        if (ui->styleItalic->isChecked())
            style.insert("italic");
        else
            style.erase("italic");
    }
    else if (sender() == ui->styleUnderline) {
        if (ui->styleUnderline->isChecked())
            style.insert("underline");
        else
            style.erase("underline");
    }
}

void PropertiesDialog::displayUnitChanged(const QString & text)
{
    try {
        std::auto_ptr<UnitExpression> e(ExpressionParser::parseUnit(sheet, text.toUtf8().constData()));

        displayUnit = Sheet::DisplayUnit(text.toUtf8().constData(), e->getUnit(), e->getScaler());

        QPalette palette = ui->displayUnit->palette();
        palette.setColor(QPalette::Text, Qt::black);
        ui->displayUnit->setPalette(palette);
    }
    catch (...) {
        displayUnit = Sheet::DisplayUnit();
        QPalette palette = ui->displayUnit->palette();
        palette.setColor(QPalette::Text, text.size() == 0 ? Qt::black : Qt::red);
        ui->displayUnit->setPalette(palette);
    }
}

PropertiesDialog::~PropertiesDialog()
{
    delete ui;
}

void PropertiesDialog::apply()
{
    if (ranges.size() > 0) {
        Gui::Command::openCommand("Set cell properties");
        std::vector<Sheet::Range>::const_iterator i = ranges.begin();
        bool changes = false;

        for (; i != ranges.end(); ++i) {
            if (orgAlignment != alignment) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setAlignment('%s', '%s')", sheet->getNameInDocument(),
                                        i->rangeString().c_str(), Sheet::encodeAlignment(alignment).c_str());
                changes = true;
            }
            if (orgStyle != style) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setStyle('%s', '%s')", sheet->getNameInDocument(),
                                        i->rangeString().c_str(), Sheet::encodeStyle(style).c_str());
                changes = true;
            }
            if (orgForegroundColor != foregroundColor) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setForeground('%s', (%f,%f,%f,%f))", sheet->getNameInDocument(),
                                        i->rangeString().c_str(),
                                        foregroundColor.r,
                                        foregroundColor.g,
                                        foregroundColor.b,
                                        foregroundColor.a);
                changes = true;
            }
            if (orgBackgroundColor != backgroundColor) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setBackground('%s', (%f,%f,%f,%f))", sheet->getNameInDocument(),
                                        i->rangeString().c_str(),
                                        backgroundColor.r,
                                        backgroundColor.g,
                                        backgroundColor.b,
                                        backgroundColor.a);
                changes = true;
            }
            if (orgDisplayUnit != displayUnit) {
                Gui::Command::doCommand(Gui::Command::Doc,"App.ActiveDocument.%s.setDisplayUnit('%s', '%s')", sheet->getNameInDocument(),
                                        i->rangeString().c_str(), displayUnit.stringRep.c_str());
                changes = true;
            }
        }
        if (changes) {
            Gui::Command::commitCommand();
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        else
            Gui::Command::abortCommand();
    }
}

#include "moc_PropertiesDialog.cpp"
