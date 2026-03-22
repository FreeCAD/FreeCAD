// SPDX-License-Identifier: LGPL-2.1-or-later

/**************************************************************************
 *   Copyright (c) 2021 FreeCAD Developers                                 *
 *   Author: Ajinkya Dahale                                                *
 *   Based on src/Gui/DlgAddProperty.h                                     *
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

#include <QDialog>
#include <memory>
#include <App/Document.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/PartDesignGlobal.h>


namespace PartDesignGui
{
class Ui_DlgActiveBody;

/** Dialog box to ask user to pick a Part Design body to make active
 *  or make a new one
 */
class PartDesignGuiExport DlgActiveBody: public QDialog
{
    Q_OBJECT

public:
    DlgActiveBody(QWidget* parent, App::Document*& doc, const QString& infoText = QString());
    ~DlgActiveBody() override;

    void accept() override;
    PartDesign::Body* getActiveBody() const
    {
        return activeBody;
    }

private:
    std::unique_ptr<Ui_DlgActiveBody> ui;
    App::Document* _doc;
    PartDesign::Body* activeBody;
};

}  // namespace PartDesignGui
