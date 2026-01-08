// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   © 2015 Eivind Kvedalen <eivind@kvedalen.name>                            *
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/


#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include <Mod/Spreadsheet/App/Sheet.h>
#include <QDialog>

namespace Ui
{
class PropertiesDialog;
}

namespace SpreadsheetGui
{

class PropertiesDialog: public QDialog
{
    Q_OBJECT

public:
    explicit PropertiesDialog(
        Spreadsheet::Sheet* _sheet,
        const std::vector<App::Range>& _ranges,
        QWidget* parent = nullptr
    );
    ~PropertiesDialog() override;

    void apply();
    void selectAlias();

private Q_SLOTS:
    void foregroundColorChanged(const QColor& color);
    void backgroundColorChanged(const QColor& color);
    void alignmentChanged();
    void styleChanged();
    void displayUnitChanged(const QString& text);
    void aliasChanged(const QString& text);

private:
    Spreadsheet::Sheet* sheet;
    std::vector<App::Range> ranges;
    Ui::PropertiesDialog* ui;
    Base::Color foregroundColor;
    Base::Color backgroundColor;
    int alignment;
    std::set<std::string> style;
    Spreadsheet::DisplayUnit displayUnit;
    std::string alias;

    Base::Color orgForegroundColor;
    Base::Color orgBackgroundColor;
    int orgAlignment;
    std::set<std::string> orgStyle;
    Spreadsheet::DisplayUnit orgDisplayUnit;
    std::string orgAlias;

    bool displayUnitOk;
    bool aliasOk;
};

}  // namespace SpreadsheetGui

#endif  // PROPERTIESDIALOG_H
