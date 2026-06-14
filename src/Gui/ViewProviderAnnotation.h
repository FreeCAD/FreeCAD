/***************************************************************************
 *   Copyright (c) 2008 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include "ViewProviderDocumentObject.h"
#include <App/PropertyUnits.h>
#include "SoTextLabel.h"

class SoFont;
class SoText2;
class SoAsciiText;
class SoBaseColor;
class SoTranslation;
class SoTransform;
class SoRotationXYZ;
class SoImage;
class SoCoordinate3;

namespace Gui
{

class GuiExport ViewProviderAnnotation: public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderAnnotation);

public:
    /// Constructor
    ViewProviderAnnotation();
    ~ViewProviderAnnotation() override;

    // Display properties
    App::PropertyColor TextColor;
    App::PropertyEnumeration Justification;
    App::PropertyFloat FontSize;
    App::PropertyFont FontName;
    App::PropertyFloat LineSpacing;
    App::PropertyAngle Rotation;
    App::PropertyEnumeration RotationAxis;

    void attach(App::DocumentObject*) override;
    void updateData(const App::Property*) override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

protected:
    void onChanged(const App::Property* prop) override;

private:
    SoFont* pFont;
    SoText2* pLabel;
    SoAsciiText* pLabel3d;
    SoBaseColor* pColor;
    SoTranslation* pTranslation;
    SoRotationXYZ* pRotationXYZ;

    static const char* JustificationEnums[];
    static const char* RotationAxisEnums[];
};

/**
 * This is a different implementation of an annotation object which uses an
 * SoImage node instead of an SoText2 or SoAsciiText node.
 * This approach gives a bit more flexibility since it can render arbitrary
 * annotations.
 */
class GuiExport ViewProviderAnnotationLabel: public ViewProviderDocumentObject
{
    PROPERTY_HEADER_WITH_OVERRIDE(Gui::ViewProviderAnnotationLabel);

public:
    /// Constructor
    ViewProviderAnnotationLabel();
    ~ViewProviderAnnotationLabel() override;

    // Display properties
    App::PropertyColor TextColor;
    App::PropertyColor BackgroundColor;
    App::PropertyEnumeration Justification;
    App::PropertyFloat FontSize;
    App::PropertyFont FontName;
    App::PropertyBool Frame;

    void attach(App::DocumentObject*) override;
    void updateData(const App::Property*) override;
    std::vector<std::string> getDisplayModes() const override;
    void setDisplayMode(const char* ModeName) override;

protected:
    void onChanged(const App::Property* prop) override;
    void drawImage(const std::vector<std::string>&);

private:
    static void dragStartCallback(void* data, SoDragger* d);
    static void dragFinishCallback(void* data, SoDragger* d);
    static void dragMotionCallback(void* data, SoDragger* d);

private:
    SoCoordinate3* pCoords;
    SoImage* pImage;
    SoBaseColor* pColor;
    SoTranslation* pBaseTranslation;
    TranslateManip* pTextTranslation;

    static const char* JustificationEnums[];
};

}  // namespace Gui
