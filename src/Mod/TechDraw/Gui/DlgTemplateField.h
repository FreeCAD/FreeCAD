 /**************************************************************************
 *   Copyright (c) 2016 WandererFan <wandererfan@gmail.com>                *
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

#include <Mod/TechDraw/TechDrawGlobal.h>

#include <memory>
#include <QDialog>
#include <QString>


namespace TechDrawGui {

class Ui_dlgTemplateField;
class DlgTemplateField : public QDialog
{
    Q_OBJECT

public:
    explicit DlgTemplateField( QWidget *parent = nullptr );
    ~DlgTemplateField() override = default;

    void setFieldName(std::string name);
    void setFieldLength(int length);
    void setFieldContent(std::string content);
    void setAutofillContent(std::string content);
    QString getFieldContent();
    bool getAutofillState();

public Q_SLOTS:
    void accept() override;
    void reject() override;

protected:
    void changeEvent(QEvent *e) override;

private:
    std::shared_ptr<Ui_dlgTemplateField> ui;
};

} // namespace TechDrawGui