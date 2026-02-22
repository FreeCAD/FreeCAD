/***************************************************************************
 *   Copyright (c) 2009 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/manips/SoTransformManip.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoText2.h>
#include <FCGlobal.h>

#include "BitmapFactory.h"


namespace Gui
{

/**
 * A text label with a background color.
 * @author Werner Mayer
 */
class GuiExport SoTextLabel: public SoText2
{
    using inherited = SoText2;

    SO_NODE_HEADER(SoTextLabel);

public:
    static void initClass();
    SoTextLabel();

    SoSFColor backgroundColor;
    SoSFBool background;
    SoSFFloat frameSize;

protected:
    ~SoTextLabel() override = default;
    void GLRender(SoGLRenderAction* action) override;
};

/**
 * A text label for the color bar.
 * @author Werner Mayer
 */
class GuiExport SoColorBarLabel: public SoText2
{
    using inherited = SoText2;

    SO_NODE_HEADER(SoColorBarLabel);

public:
    static void initClass();
    SoColorBarLabel();

protected:
    void computeBBox(SoAction* action, SbBox3f& box, SbVec3f& center) override;
};

class GuiExport SoStringLabel: public SoNode
{
    using inherited = SoNode;

    SO_NODE_HEADER(SoStringLabel);

public:
    static void initClass();
    SoStringLabel();

    SoMFString string;
    SoSFColor textColor;
    SoSFName name;
    SoSFInt32 size;

protected:
    ~SoStringLabel() override = default;
    void GLRender(SoGLRenderAction* action) override;
};

class GuiExport SoFrameLabel: public SoImage
{
    using inherited = SoImage;

    SO_NODE_HEADER(SoFrameLabel);

public:
    enum Justification
    {
        LEFT,
        RIGHT,
        CENTER
    };

    static void initClass();
    SoFrameLabel();
    void setIcon(const QPixmap& pixMap);

    SoMFString string;
    SoSFColor textColor;
    SoSFColor backgroundColor;
    SoSFEnum justification;
    SoSFName name;
    SoSFInt32 size;
    SoSFBool frame;
    SoSFBool border;
    SoSFBool backgroundUseBaseColor;
    SoSFBool textUseBaseColor;
    // SoSFImage  image;
    QPixmap iconPixmap;

protected:
    ~SoFrameLabel() override = default;
    void notify(SoNotList* list) override;
    void GLRender(SoGLRenderAction* action) override;

private:
    void drawImage();
};

class GuiExport TranslateManip: public SoTransformManip
{
    SO_NODE_HEADER(TranslateManip);

public:
    // Constructor
    TranslateManip();
    static void initClass();

private:
    // Destructor
    ~TranslateManip() override;
};

}  // namespace Gui
