// SPDX-License-Identifier: LGPL-2.1-or-later

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


#pragma once

#include "PropertyPage.h"
#include <memory>

namespace Gui
{
namespace Dialog
{
class Ui_DlgOnlineHelp;

/** This class implements the dialog for downloading the online documentation.
 * Moreover it allows one to specify location of the start page an an external browser.
 * Here you can specify to use a proxy if necessary and some more stuff.
 * \author Werner Mayer
 */
class DlgOnlineHelpImp: public PreferencePage
{
    Q_OBJECT

public:
    DlgOnlineHelpImp(QWidget* parent = nullptr);
    ~DlgOnlineHelpImp() override;

    static QString getStartpage();

    void saveSettings() override;
    void loadSettings() override;

protected:
    void changeEvent(QEvent* e) override;

protected:
    void onLineEditDownloadFileNameSelected(const QString&);

private:
    std::unique_ptr<Ui_DlgOnlineHelp> ui;
};

}  // namespace Dialog
}  // namespace Gui
