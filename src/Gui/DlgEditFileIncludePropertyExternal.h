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


#ifndef GUI_DIALOG_DlgEditFileIncludePropertyExternal_H
#define GUI_DIALOG_DlgEditFileIncludePropertyExternal_H

#include "DlgRunExternal.h"
#include <App/PropertyFile.h>

namespace Gui {
namespace Dialog {

/**
 *
 * \author Jürgen Riegel
 */
class GuiExport DlgEditFileIncludePropertyExternal : public DlgRunExternal
{
    Q_OBJECT

public:
    DlgEditFileIncludePropertyExternal( App::PropertyFileIncluded& Prop, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags() );
    virtual ~DlgEditFileIncludePropertyExternal();

    int Do(void);

protected Q_SLOTS:
protected:
    App::PropertyFileIncluded& Prop;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DlgEditFileIncludePropertyExternal_H
