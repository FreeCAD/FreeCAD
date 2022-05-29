/***************************************************************************
 *   Copyright (c) 2004 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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


#ifndef GUI_DIALOG_DLGEDITORSETTINGSIMP_H
#define GUI_DIALOG_DLGEDITORSETTINGSIMP_H

#include "PropertyPage.h"
#include <memory>

class QTreeWidgetItem;

namespace Gui {
class PythonSyntaxHighlighter;

namespace Dialog {
class Ui_DlgEditorSettings;

/** This class implements a preferences page for the editor settings.
 *  Here you can change different color settings and font for editors.
 *  @author Werner Mayer
 */
struct DlgSettingsEditorP;
class DlgSettingsEditorImp : public PreferencePage
{
    Q_OBJECT

public:
    DlgSettingsEditorImp( QWidget* parent = nullptr );
    ~DlgSettingsEditorImp();

public:
    void saveSettings();
    void loadSettings();

protected Q_SLOTS:
    void on_displayItems_currentItemChanged(QTreeWidgetItem *i);
    void on_colorButton_changed();
    void on_fontFamily_activated(const QString&);
    void on_fontSize_valueChanged(const QString&);

protected:
    void changeEvent(QEvent *e);
    void setEditorTabWidth(int);

private:
    std::unique_ptr<Ui_DlgEditorSettings> ui;
    DlgSettingsEditorP* d;
    Gui::PythonSyntaxHighlighter* pythonSyntax;

    DlgSettingsEditorImp( const DlgSettingsEditorImp & );
    DlgSettingsEditorImp& operator=( const DlgSettingsEditorImp & );
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGEDITORSETTINGSIMP_H
