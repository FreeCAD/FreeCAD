// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) 2023 David Carter <dcarter@david.carter.ca>             *
 *                                                                         *
 *   This file is part of FreeCAD.                                         *
 *                                                                         *
 *   FreeCAD is free software: you can redistribute it and/or modify it    *
 *   under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the  *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   FreeCAD is distributed in the hope that it will be useful, but        *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 *                                                                         *
 *   You should have received a copy of the GNU Lesser General Public      *
 *   License along with FreeCAD. If not, see                               *
 *   <https://www.gnu.org/licenses/>.                                      *
 *                                                                         *
 **************************************************************************/

#pragma once

#include <FCConfig.h>
#include <Gui/View3DInventorViewer.h>
#include <Gui/View3DSettings.h>

class SoGroup;
class SoMaterial;
class SoSwitch;
class SoTexture2;
class SoTextureCoordinateEnvironment;

namespace MatGui
{

class AppearanceSettings: public Gui::View3DSettings
{
public:
    AppearanceSettings(const ParameterGrp::handle& hGrp, Gui::View3DInventorViewer*);
    AppearanceSettings(const ParameterGrp::handle& hGrp,
                       const std::vector<Gui::View3DInventorViewer*>&);
    ~AppearanceSettings() = default;

    /// Observer message from the ParameterGrp
    void OnChange(ParameterGrp::SubjectType& rCaller, ParameterGrp::MessageType Reason) override;
};

class AppearancePreview: public Gui::View3DInventorViewer
{
    Q_OBJECT

public:
    explicit AppearancePreview(QWidget* parent = nullptr);
    ~AppearancePreview() override;

    void setAmbientColor(const QColor& color);
    void setDiffuseColor(const QColor& color);
    void setSpecularColor(const QColor& color);
    void setEmissiveColor(const QColor& color);
    void setShininess(double value);
    void setTransparency(double value);
    void setTexture(const QImage& image);
    void setTextureScaling(double scale);

    void resetAmbientColor();
    void resetDiffuseColor();
    void resetSpecularColor();
    void resetEmissiveColor();
    void resetShininess();
    void resetTransparency();
    void resetTexture();
    void resetTextureScaling();

private:
    SoSeparator* _group;
    SoSwitch* _switch;
    SoMaterial* _material;
    SoTexture2* _texture;
    SoTextureCoordinateEnvironment* _environment;
    std::unique_ptr<AppearanceSettings> viewSettings;

    void applySettings();
    void setCoinTexture();
    void setCoinMaterial();
};

}  // namespace MatGui