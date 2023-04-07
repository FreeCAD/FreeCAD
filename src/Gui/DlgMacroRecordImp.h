/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_DIALOG_DLGMACRORECORDIMP_H
#define GUI_DIALOG_DLGMACRORECORDIMP_H

#include <QDialog>
#include <memory>
#include "Window.h"

namespace Gui {
class MacroManager;
namespace Dialog {
class Ui_DlgMacroRecord;

/**
 * The DlgMacroRecordImp class implements a dialog to record a macro.
 * \author Jürgen Riegel
 */
class DlgMacroRecordImp : public QDialog, public Gui::WindowParameter
{
    Q_OBJECT

public:
    explicit DlgMacroRecordImp( QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags() );
    ~DlgMacroRecordImp() override;

protected:
    void setupConnections();
    void onButtonStartClicked();
    void onButtonStopClicked();
    void onButtonCloseClicked();
    void onButtonChooseDirClicked();
    void onMacroPathTextChanged ( const QString & );

private:
    std::unique_ptr<Ui_DlgMacroRecord> ui;
    /// convenience pointer
    MacroManager* macroManager;
    QString macroPath; // Macro file to save in
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGMACRORECORDIMP_H
