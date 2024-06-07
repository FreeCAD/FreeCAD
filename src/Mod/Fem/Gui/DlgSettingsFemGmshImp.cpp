/***************************************************************************
 *   Copyright (c) 2016 FreeCAD Developers                                 *
 *   Author: Bernd Hahnebach <bernd@bimstatik.ch>                          *
 *   Based on src/Mod/Fem/Gui/DlgSettingsFemCcxImp.cpp                     *
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

#include "PreCompiled.h"
#ifndef _PreComp_
#include <QMessageBox>
#endif
#include <App/Application.h>
#include "DlgSettingsFemGmshImp.h"
#include "ui_DlgSettingsFemGmsh.h"
#include <App/Application.h>
#include <Base/Console.h>

using namespace FemGui;

DlgSettingsFemGmshImp::DlgSettingsFemGmshImp(QWidget* parent)
    : PreferencePage(parent)
    , ui(new Ui_DlgSettingsFemGmshImp)
{
    ui->setupUi(this);

    connect(ui->fc_gmsh_binary_path,
            &Gui::PrefFileChooser::fileNameChanged,
            this,
            &DlgSettingsFemGmshImp::onfileNameChanged);

}

DlgSettingsFemGmshImp::~DlgSettingsFemGmshImp() = default;

void DlgSettingsFemGmshImp::saveSettings()
{
    
    ui->cb_gmsh_binary_std->onSave();
    ui->fc_gmsh_binary_path->onSave();
    ui->cb_mesh_file_format->onSave();

    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh");
    std::string meshFormats[] = {".unv", ".vtk", ".inp", ".med"};
    int ind = ui->cb_mesh_file_format->currentIndex();
    hGrp->SetASCII("MeshFileFormat", meshFormats[ind]);
    
}

void DlgSettingsFemGmshImp::loadSettings()
{
    ParameterGrp::handle hGrp = App::GetApplication().GetParameterGroupByPath(
        "User parameter:BaseApp/Preferences/Mod/Fem/Gmsh");
    std::string str = hGrp->GetASCII("MeshFileFormat", ".unv");
    int ind = 0;
    if(str == ".unv")
    {
        ind =0;
    }
    else if(str == ".vtk")
    {
        ind =1;
    }
    else if(str == ".inp")
    {
        ind =2;
    }
    else if(str == ".med")
    {
        ind =3;
    }

    ui->cb_mesh_file_format->setCurrentIndex(ind);
    ui->cb_gmsh_binary_std->onRestore();
    ui->fc_gmsh_binary_path->onRestore();
    ui->cb_mesh_file_format->onRestore();


}

/**
 * Sets the strings of the subwidgets using the current language.
 */
void DlgSettingsFemGmshImp::changeEvent(QEvent* e)
{
    if (e->type() == QEvent::LanguageChange) {
        int c_index = ui->cb_mesh_file_format->currentIndex();
        ui->retranslateUi(this);
        ui->cb_mesh_file_format->setCurrentIndex(c_index);
    }
    else {
        QWidget::changeEvent(e);
    }
}

void DlgSettingsFemGmshImp::onfileNameChanged(QString FileName)
{
    if (!QFileInfo::exists(FileName)) {
        QMessageBox::critical(this,
                              tr("File does not exist"),
                              tr("The specified executable\n'%1'\n does not exist!\n"
                                 "Specify another file please.")
                                  .arg(FileName));
    }
}

#include "moc_DlgSettingsFemGmshImp.cpp"
