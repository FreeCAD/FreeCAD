// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2006 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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
#include <vector>
#include <App/Material.h>

namespace Base
{
class Color;
}

namespace App
{
class Material;
}

namespace Gui
{
class ViewProvider;

namespace Dialog
{
class Ui_DlgMaterialProperties;

class GuiExport DlgMaterialPropertiesImp: public QDialog
{
    Q_OBJECT

public:
    explicit DlgMaterialPropertiesImp(QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgMaterialPropertiesImp() override;
    App::Material getCustomMaterial() const;
    void setCustomMaterial(const App::Material& mat);
    App::Material getDefaultMaterial() const;
    void setDefaultMaterial(const App::Material& mat);

private:
    void setupConnections();
    void onAmbientColorChanged();
    void onDiffuseColorChanged();
    void onEmissiveColorChanged();
    void onSpecularColorChanged();
    void onShininessValueChanged(int);
    void onTransparencyValueChanged(int);
    void onButtonReset();
    void onButtonDefault();
    void setButtonColors(const App::Material& mat);

private:
    std::unique_ptr<Ui_DlgMaterialProperties> ui;
    App::Material customMaterial;
    App::Material defaultMaterial;
};

}  // namespace Dialog
}  // namespace Gui
