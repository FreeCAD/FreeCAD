// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2015 Eivind Kvedalen <eivind@kvedalen.name>             *
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


#include <cctype>

#include <App/Document.h>
#include <App/ExpressionParser.h>
#include <App/Range.h>
#include <Base/Tools.h>
#include <Gui/CommandT.h>
#include <QCoreApplication>

#include "PropertiesDialog.h"
#include "ui_PropertiesDialog.h"


using namespace App;
using namespace Spreadsheet;
using namespace SpreadsheetGui;

namespace
{
bool hasValidAliasSyntax(const std::string& candidate)
{
    if (candidate.empty()) {
        return false;
    }

    const unsigned char first = static_cast<unsigned char>(candidate[0]);
    if (!std::isalpha(first)) {
        return false;
    }

    for (char ch : candidate) {
        const unsigned char uch = static_cast<unsigned char>(ch);
        if (!(std::isalnum(uch) || ch == '_')) {
            return false;
        }
    }

    return true;
}

QString aliasHelpTooltip()
{
    return QCoreApplication::translate(
        "PropertiesDialog",
        "Refer to cell by alias, for example\n"
        "Spreadsheet.my_alias_name instead of Spreadsheet.B1"
    );
}

QString reservedAliasTooltip()
{
    return QCoreApplication::translate(
        "PropertiesDialog",
        "Cannot use a unit/constant name as alias. Reserved examples: m = meters, "
        "s = seconds, e = Euler's number, A = amperes, T = tesla, G = gauss/giga, "
        "H = henry, L = liter. Try 'myM', 'LenM', 'Sec' etc. instead."
    );
}
}  // namespace

PropertiesDialog::PropertiesDialog(Sheet* _sheet, const std::vector<Range>& _ranges, QWidget* parent)
    : QDialog(parent)
    , sheet(_sheet)
    , ranges(_ranges)
    , ui(new Ui::PropertiesDialog)
    , alignment(0)
    , displayUnitOk(true)
    , aliasOk(true)
{
    ui->setupUi(this);
    ui->foregroundColor->setStandardColors();
    ui->backgroundColor->setStandardColors();

    assert(ranges.size() > 0);
    Range range = ranges[0];

    Cell* cell = sheet->getNewCell(*range);

    assert(cell);

    (void)cell->getForeground(foregroundColor);
    (void)cell->getBackground(backgroundColor);
    (void)cell->getAlignment(alignment);
    (void)cell->getStyle(style);
    (void)cell->getDisplayUnit(displayUnit);
    (void)cell->getAlias(alias);

    orgForegroundColor = foregroundColor;
    orgBackgroundColor = backgroundColor;
    orgAlignment = alignment;
    orgStyle = style;
    orgDisplayUnit = displayUnit;
    orgAlias = alias;

    ui->foregroundColor->setCurrentColor(
        QColor::fromRgbF(foregroundColor.r, foregroundColor.g, foregroundColor.b, foregroundColor.a)
    );
    ui->backgroundColor->setCurrentColor(
        QColor::fromRgbF(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a)
    );

    if (alignment & Cell::ALIGNMENT_LEFT) {
        ui->alignLeft->setChecked(true);
    }
    else if (alignment & Cell::ALIGNMENT_HCENTER) {
        ui->alignHCenter->setChecked(true);
    }
    else if (alignment & Cell::ALIGNMENT_RIGHT) {
        ui->alignRight->setChecked(true);
    }

    if (alignment & Cell::ALIGNMENT_TOP) {
        ui->alignTop->setChecked(true);
    }
    else if (alignment & Cell::ALIGNMENT_VCENTER) {
        ui->alignVCenter->setChecked(true);
    }
    else if (alignment & Cell::ALIGNMENT_BOTTOM) {
        ui->alignBottom->setChecked(true);
    }

    if (style.find("bold") != style.end()) {
        ui->styleBold->setChecked(true);
    }
    if (style.find("italic") != style.end()) {
        ui->styleItalic->setChecked(true);
    }
    if (style.find("underline") != style.end()) {
        ui->styleUnderline->setChecked(true);
    }

    ui->displayUnit->setText(QString::fromStdString(displayUnit.stringRep));

    ui->alias->setText(QString::fromStdString(alias));

    // Colors
    connect(
        ui->foregroundColor,
        &QtColorPicker::colorChanged,
        this,
        &PropertiesDialog::foregroundColorChanged
    );
    connect(
        ui->backgroundColor,
        &QtColorPicker::colorChanged,
        this,
        &PropertiesDialog::backgroundColorChanged
    );

    // Alignment
    connect(ui->alignLeft, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);
    connect(ui->alignRight, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);
    connect(ui->alignHCenter, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);
    connect(ui->alignTop, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);
    connect(ui->alignVCenter, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);
    connect(ui->alignBottom, &QRadioButton::clicked, this, &PropertiesDialog::alignmentChanged);

    // Style
    connect(ui->styleBold, &QCheckBox::clicked, this, &PropertiesDialog::styleChanged);
    connect(ui->styleItalic, &QCheckBox::clicked, this, &PropertiesDialog::styleChanged);
    connect(ui->styleUnderline, &QCheckBox::clicked, this, &PropertiesDialog::styleChanged);

    // Display unit
    connect(ui->displayUnit, &QLineEdit::textEdited, this, &PropertiesDialog::displayUnitChanged);

    // Alias is only allowed for a single cell
    ui->tabWidget->widget(4)->setEnabled(_ranges.size() == 1 && _ranges[0].size() == 1);

    // Alias
    connect(ui->alias, &QLineEdit::textEdited, this, &PropertiesDialog::aliasChanged);
    ui->aliasStatus->setVisible(false);
    ui->alias->setToolTip(aliasHelpTooltip());

    ui->tabWidget->setCurrentIndex(0);
}

void PropertiesDialog::foregroundColorChanged(const QColor& color)
{
    foregroundColor = Base::Color(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void PropertiesDialog::backgroundColorChanged(const QColor& color)
{
    backgroundColor = Base::Color(color.redF(), color.greenF(), color.blueF(), color.alphaF());
}

void PropertiesDialog::alignmentChanged()
{
    if (sender() == ui->alignLeft) {
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_LEFT;
    }
    else if (sender() == ui->alignHCenter) {
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_HCENTER;
    }
    else if (sender() == ui->alignRight) {
        alignment = (alignment & ~Cell::ALIGNMENT_HORIZONTAL) | Cell::ALIGNMENT_RIGHT;
    }
    else if (sender() == ui->alignTop) {
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_TOP;
    }
    else if (sender() == ui->alignVCenter) {
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_VCENTER;
    }
    else if (sender() == ui->alignBottom) {
        alignment = (alignment & ~Cell::ALIGNMENT_VERTICAL) | Cell::ALIGNMENT_BOTTOM;
    }
}

void PropertiesDialog::styleChanged()
{
    if (sender() == ui->styleBold) {
        if (ui->styleBold->isChecked()) {
            style.insert("bold");
        }
        else {
            style.erase("bold");
        }
    }
    else if (sender() == ui->styleItalic) {
        if (ui->styleItalic->isChecked()) {
            style.insert("italic");
        }
        else {
            style.erase("italic");
        }
    }
    else if (sender() == ui->styleUnderline) {
        if (ui->styleUnderline->isChecked()) {
            style.insert("underline");
        }
        else {
            style.erase("underline");
        }
    }
}

void PropertiesDialog::displayUnitChanged(const QString& text)
{
    if (text.isEmpty()) {
        displayUnit = DisplayUnit();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        return;
    }

    QPalette palette = ui->displayUnit->palette();
    try {
        std::unique_ptr<UnitExpression> expr(
            App::ExpressionParser::parseUnit(sheet, text.toUtf8().constData())
        );

        if (expr) {
            displayUnit = DisplayUnit(text.toStdString(), expr->getUnit(), expr->getScaler());
            palette.setColor(QPalette::Text, Qt::black);
            displayUnitOk = true;
        }
        else {
            displayUnit = DisplayUnit();
            palette.setColor(QPalette::Text, text.size() == 0 ? Qt::black : Qt::red);
            displayUnitOk = false;
        }
    }
    catch (...) {
        displayUnit = DisplayUnit();
        palette.setColor(QPalette::Text, text.size() == 0 ? Qt::black : Qt::red);
        displayUnitOk = false;
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(displayUnitOk && aliasOk);
    ui->displayUnit->setPalette(palette);
}

void PropertiesDialog::aliasChanged(const QString& text)
{
    QPalette palette = ui->alias->palette();
    const std::string aliasText = text.toStdString();
    QString tooltip = aliasHelpTooltip();
    QString statusText;

    aliasOk = text.isEmpty() || sheet->isValidAlias(aliasText);
    if (!text.isEmpty() && !aliasOk) {
        if (ExpressionParser::isTokenAUnit(aliasText)) {
            tooltip = tr("Alias conflicts with a reserved unit token used by expressions.");
            statusText = tr("Invalid: reserved unit token");
        }
        else if (ExpressionParser::isTokenAConstant(aliasText)) {
            tooltip = tr("Alias conflicts with a reserved constant token used by expressions.");
            statusText = tr("Invalid: reserved constant token");
        }
        else if (sheet->isReservedAliasName(aliasText)) {
            tooltip = reservedAliasTooltip();
            statusText = tr("Invalid: reserved unit or constant name");
        }
        else if (!sheet->getAddressFromAlias(aliasText).empty()) {
            tooltip = tr("Alias already defined.");
            statusText = tr("Invalid: alias already exists");
        }
        else if (sheet->getCells()->isValidCellAddressName(aliasText)) {
            tooltip = tr("Alias cannot look like a cell address such as A1 or C12.");
            statusText = tr("Invalid: alias matches cell address pattern");
        }
        else if (sheet->getPropertyByName(aliasText.c_str())) {
            tooltip = tr("Alias conflicts with an existing spreadsheet property name.");
            statusText = tr("Invalid: conflicts with existing property name");
        }
        else if (!hasValidAliasSyntax(aliasText)) {
            tooltip = tr("Alias must start with a letter and contain only letters, digits, and '_'.");
            statusText = tr("Invalid: bad alias syntax");
        }
        else {
            tooltip = tr("Alias is not valid in this spreadsheet context.");
            statusText = tr("Invalid alias");
        }
    }

    alias = aliasOk ? aliasText : "";
    palette.setColor(QPalette::Text, aliasOk ? Qt::black : Qt::red);
    ui->alias->setPalette(palette);
    ui->alias->setToolTip(tooltip);
    ui->aliasStatus->setText(statusText);
    ui->aliasStatus->setVisible(!statusText.isEmpty());
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(displayUnitOk && aliasOk);
}

PropertiesDialog::~PropertiesDialog()
{
    delete ui;
}

void PropertiesDialog::apply()
{
    if (!ranges.empty()) {
        Gui::Command::openCommand(QT_TRANSLATE_NOOP("Command", "Set cell properties"));
        std::vector<Range>::const_iterator i = ranges.begin();
        bool changes = false;

        for (; i != ranges.end(); ++i) {
            if (orgAlignment != alignment) {
                Gui::cmdAppObjectArgs(
                    sheet,
                    "setAlignment('%s', '%s')",
                    i->rangeString().c_str(),
                    Cell::encodeAlignment(alignment).c_str()
                );
                changes = true;
            }
            if (orgStyle != style) {
                Gui::cmdAppObjectArgs(
                    sheet,
                    "setStyle('%s', '%s')",
                    i->rangeString().c_str(),
                    Cell::encodeStyle(style).c_str()
                );
                changes = true;
            }
            if (orgForegroundColor != foregroundColor) {
                Gui::cmdAppObjectArgs(
                    sheet,
                    "setForeground('%s', (%f,%f,%f,%f))",
                    i->rangeString().c_str(),
                    foregroundColor.r,
                    foregroundColor.g,
                    foregroundColor.b,
                    foregroundColor.a
                );
                changes = true;
            }
            if (orgBackgroundColor != backgroundColor) {
                Gui::cmdAppObjectArgs(
                    sheet,
                    "setBackground('%s', (%f,%f,%f,%f))",
                    i->rangeString().c_str(),
                    backgroundColor.r,
                    backgroundColor.g,
                    backgroundColor.b,
                    backgroundColor.a
                );
                changes = true;
            }
            if (orgDisplayUnit != displayUnit) {
                std::string escapedstr = Base::Tools::escapedUnicodeFromUtf8(
                    displayUnit.stringRep.c_str()
                );
                escapedstr = Base::Tools::escapeQuotesFromString(escapedstr);
                Gui::cmdAppObjectArgs(
                    sheet,
                    "setDisplayUnit('%s', '%s')",
                    i->rangeString().c_str(),
                    escapedstr.c_str()
                );
                changes = true;
            }
            if (ranges.size() == 1 && ranges[0].size() == 1 && orgAlias != alias) {
                Gui::cmdAppObjectArgs(sheet, "setAlias('%s', '%s')", i->address().c_str(), alias.c_str());
                changes = true;
            }
        }
        if (changes) {
            Gui::Command::commitCommand();
            Gui::Command::doCommand(Gui::Command::Doc, "App.ActiveDocument.recompute()");
        }
        else {
            Gui::Command::abortCommand();
        }
    }
}

void PropertiesDialog::selectAlias()
{
    ui->tabWidget->setCurrentIndex(4);
    ui->alias->setFocus();
}

#include "moc_PropertiesDialog.cpp"
