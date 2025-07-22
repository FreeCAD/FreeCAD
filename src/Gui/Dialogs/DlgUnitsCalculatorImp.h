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

#include <QDialog>
#include <memory>
#include <Base/Quantity.h>
#include <Base/Unit.h>

namespace Gui {
namespace Dialog {
class Ui_DlgUnitCalculator;

/**
 * The DlgUnitsCalculator provides a unit conversion dialog
 * \author Juergen Riegel
 */
class DlgUnitsCalculator : public QDialog
{
    Q_OBJECT

public:
    explicit DlgUnitsCalculator(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgUnitsCalculator() override;

protected:
    void accept() override;
    void reject() override;

protected:
    void textChanged(const QString);
    void valueChanged(const Base::Quantity&);
    void onUnitsBoxActivated(int);
    void onComboBoxSchemeActivated(int);
    void onSpinBoxDecimalsValueChanged(int);

    void copy();
    void returnPressed();

    void parseError(const QString& errorText);

private:
    Base::Quantity actValue;
    std::unique_ptr<Ui_DlgUnitCalculator> ui;
    QList<Base::Unit> units;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DlgActivateWindowImp_H
