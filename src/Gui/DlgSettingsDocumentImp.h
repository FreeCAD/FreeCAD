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


#ifndef GUI_DIALOG_DLGSETTINGSDOCUMENT_IMP_H
#define GUI_DIALOG_DLGSETTINGSDOCUMENT_IMP_H

#include "ui_DlgSettingsDocument.h"
#include "PropertyPage.h"

namespace Gui {
namespace Dialog {

/**
 * The DlgSettingsDocumentImp class implements a preference page to change settings
 * for the Inventor viewer.
 * \author Jürgen Riegel
 */
 class DlgSettingsDocumentImp : public PreferencePage, public Ui_DlgSettingsDocument
{ 
  Q_OBJECT

public:
  DlgSettingsDocumentImp( QWidget* parent = 0 );
  ~DlgSettingsDocumentImp();

  void saveSettings();
  void loadSettings();

protected:
  void changeEvent(QEvent *e);
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGSETTINGSDOCUMENT_IMP_H
