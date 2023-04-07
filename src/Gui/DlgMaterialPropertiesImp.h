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


#ifndef GUI_DIALOG_DLGMATERIALPROPERTIES_IMP_H
#define GUI_DIALOG_DLGMATERIALPROPERTIES_IMP_H

#include <QDialog>
#include <memory>
#include <vector>

namespace Gui {
class ViewProvider;

namespace Dialog {
class Ui_DlgMaterialProperties;

class DlgMaterialPropertiesImp : public QDialog
{
    Q_OBJECT

public:
    explicit DlgMaterialPropertiesImp(const std::string& mat, QWidget* parent = nullptr, Qt::WindowFlags fl = Qt::WindowFlags());
    ~DlgMaterialPropertiesImp() override;
    void setViewProviders(const std::vector<Gui::ViewProvider*>&);
    QColor diffuseColor() const;

private:
    void setupConnections();
    void onAmbientColorChanged();
    void onDiffuseColorChanged();
    void onEmissiveColorChanged();
    void onSpecularColorChanged();
    void onShininessValueChanged(int);

private:
    std::unique_ptr<Ui_DlgMaterialProperties> ui;
    std::string material;
    std::vector<Gui::ViewProvider*> Objects;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGMATERIALPROPERTIES_IMP_H

