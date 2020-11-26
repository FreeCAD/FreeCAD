/***************************************************************************
 *   Copyright (c) 2009 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef GUI_DIALOG_DlgRunExternal_H
#define GUI_DIALOG_DlgRunExternal_H

#include <QProcess>
#include <QDialog>

namespace Gui {
namespace Dialog {
class Ui_DlgRunExternal;

/**
 * The DlgRunExternal class implements a dialog to start and control external
 * programs to edit FreeCAD controlled content.
 * \author Jürgen Riegel
 */
class GuiExport DlgRunExternal : public QDialog
{
    Q_OBJECT

public:
    DlgRunExternal(QWidget* parent = 0, Qt::WindowFlags fl = Qt::WindowFlags());
    virtual ~DlgRunExternal();

    int Do(void);

    QString ProcName;
    QStringList arguments;

protected Q_SLOTS:
    virtual void reject(void);
    virtual void accept(void);
    virtual void abort(void);
    virtual void advanced(void);
    void finished (int exitCode, QProcess::ExitStatus exitStatus);
    void on_chooseProgram_clicked();

protected:
    QProcess process;
    bool advancedHidden;

private:
    Ui_DlgRunExternal* ui;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DlgRunExternal_H
