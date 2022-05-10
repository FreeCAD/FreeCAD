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


#ifndef DRAWINGGUI_DLGTEMPLATEFIELD_H
#define DRAWINGGUI_DLGTEMPLATEFIELD_H

#include <QDialog>
#include <QString>
#include <memory>

namespace TechDrawGui {

class Ui_dlgTemplateField;
class DlgTemplateField : public QDialog
{
    Q_OBJECT

public:
    DlgTemplateField( QWidget *parent = nullptr );
    virtual ~DlgTemplateField() = default;

    void setFieldName(std::string name);
    void setFieldContent(std::string content);
    QString getFieldContent();

public Q_SLOTS:
    void accept();
    void reject();

protected:
    void changeEvent(QEvent *e);

private:
    std::shared_ptr<Ui_dlgTemplateField> ui;
};

} // namespace TechDrawGui

#endif // DRAWINGGUI_DLGTEMPLATEFIELD_H
