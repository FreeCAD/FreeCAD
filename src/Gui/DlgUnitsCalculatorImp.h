/***************************************************************************
 *   Copyright (c) 2013 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_DIALOG_DlgActivateWindowImp_H
#define GUI_DIALOG_DlgActivateWindowImp_H

#include "ui_DlgUnitsCalculator.h"

namespace Gui {
namespace Dialog {

/**
 * The DlgUnitsCalculator provides a unit conversion dialog
 * \author Juergen Riegel 
 */
class DlgUnitsCalculator : public QDialog, public Ui_DlgUnitCalculator
{
    Q_OBJECT

public:
    DlgUnitsCalculator( QWidget* parent = 0, Qt::WFlags fl = 0 );
    ~DlgUnitsCalculator();

protected:
    void accept();
    void reject();

protected Q_SLOTS:
    void unitValueChanged(const Base::Quantity&);
    void valueChanged(const Base::Quantity&);

    void copy(void);
    void help(void);
    void returnPressed(void);

    void parseError(const QString& errorText);

private:
    Base::Quantity actValue;
    Base::Quantity actUnit;

 
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DlgActivateWindowImp_H
