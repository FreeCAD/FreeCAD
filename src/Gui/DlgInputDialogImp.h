/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGINPUTDIALOGIMP_H
#define GUI_DIALOG_DLGINPUTDIALOGIMP_H

#include "ui_DlgInputDialog.h"

namespace Gui {
namespace Dialog {

/**
 * The DlgInputDialogImp dialog class does basically the same as Qt's QInputDialog
 * unless that it provides no static function but the application programmer must
 * create an instance and prepare it. This requires a little more work but increases
 * the flexibility.
 * \author Werner Mayer
 */
class GuiExport DlgInputDialogImp : public QDialog, public Ui_DlgInputDialog
{
  Q_OBJECT

public:
  enum Type { LineEdit, SpinBox, UIntBox, FloatSpinBox, ComboBox };

  DlgInputDialogImp( const QString& label, QWidget* parent = 0, bool modal = true, Type = LineEdit );
  ~DlgInputDialogImp();

  void setType( Type t );
  Type type() const;

  QSpinBox *getSpinBox() const;
  Gui::UIntSpinBox *getUIntBox() const;
  QDoubleSpinBox *getFloatSpinBox() const;
  QLineEdit *getLineEdit() const;
  QComboBox *getComboBox() const;

protected Q_SLOTS:
  void textChanged( const QString &s );
  void tryAccept();

protected:
  Type inputtype;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGINPUTDIALOGIMP_H
