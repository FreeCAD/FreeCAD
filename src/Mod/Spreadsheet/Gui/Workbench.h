/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
 *   Copyright (c) 2015 Eivind Kvedalen (eivind@kvedalen.name)             *
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


#ifndef SPREADSHEET_WORKBENCH_H
#define SPREADSHEET_WORKBENCH_H

#include <Gui/Workbench.h>

class QtColorPicker;
class QColor;

namespace SpreadsheetGui {

/**
 * @author Eivind Kvedalen
 */

class SpreadsheetGuiExport WorkbenchHelper : public QObject
{
    Q_OBJECT
protected Q_SLOTS:
  void setForegroundColor(const QColor &color);
  void setBackgroundColor(const QColor &color);
};

class SpreadsheetGuiExport Workbench : public Gui::StdWorkbench
{
    TYPESYSTEM_HEADER();

public:
  Workbench();
  virtual ~Workbench();
  void activated();

private:
  bool initialized;
  std::unique_ptr<WorkbenchHelper> workbenchHelper;

protected:
  Gui::MenuItem *setupMenuBar() const;
  Gui::ToolBarItem* setupToolBars() const;
  Gui::ToolBarItem* setupCommandBars() const;
};

} // namespace SpreadsheetModGui


#endif // SPREADSHEET_WORKBENCH_H
