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
    explicit PropertiesDialog(Spreadsheet::Sheet* _sheet,
                              const std::vector<App::Range>& _ranges,
                              QWidget* parent = nullptr);
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
    App::Color foregroundColor;
    App::Color backgroundColor;
    int alignment;
    std::set<std::string> style;
    Spreadsheet::DisplayUnit displayUnit;
    std::string alias;

    App::Color orgForegroundColor;
    App::Color orgBackgroundColor;
    int orgAlignment;
    std::set<std::string> orgStyle;
    Spreadsheet::DisplayUnit orgDisplayUnit;
    std::string orgAlias;

    bool displayUnitOk;
    bool aliasOk;
};

}  // namespace SpreadsheetGui

#endif  // PROPERTIESDIALOG_H
