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

#ifndef GUI_SOTEXTLABEL_H
#define GUI_SOTEXTLABEL_H

#include <Inventor/fields/SoSubField.h>
#include <Inventor/nodes/SoSubNode.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoText2.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFFloat.h>
#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFImage.h>
#include <Inventor/manips/SoTransformManip.h>

namespace Gui {

/**
 * A text label with a background color.
 * @author Werner Mayer
 */
class GuiExport SoTextLabel : public SoText2 {
    typedef SoText2 inherited;

    SO_NODE_HEADER(SoTextLabel);

public:
    static void initClass();
    SoTextLabel();

    SoSFColor backgroundColor;
    SoSFBool background;
    SoSFFloat frameSize;

protected:
    virtual ~SoTextLabel() {};
    virtual void GLRender(SoGLRenderAction *action);
};

class GuiExport SoStringLabel : public SoNode {
    typedef SoNode inherited;

    SO_NODE_HEADER(SoStringLabel);

public:
    static void initClass();
    SoStringLabel();

    SoMFString string;
    SoSFColor  textColor;
    SoSFName   name;
    SoSFInt32  size;

protected:
    virtual ~SoStringLabel() {};
    virtual void GLRender(SoGLRenderAction *action);
};

class GuiExport SoFrameLabel : public SoImage {
    typedef SoImage inherited;

    SO_NODE_HEADER(SoFrameLabel);

public:
    enum Justification {
        LEFT, RIGHT, CENTER
    };

    static void initClass();
    SoFrameLabel();

    SoMFString string;
    SoSFColor  textColor;
    SoSFColor  backgroundColor;
    SoSFEnum   justification;
    SoSFName   name;
    SoSFInt32  size;
    SoSFBool   frame;
  //SoSFImage  image;

protected:
    virtual ~SoFrameLabel() {};
    virtual void notify(SoNotList * list);
    virtual void GLRender(SoGLRenderAction *action);

private:
    void drawImage();
};

class GuiExport TranslateManip : public SoTransformManip
{
    SO_NODE_HEADER(TranslateManip);

public:
    // Constructor
    TranslateManip();
    static void initClass();

private:
    // Destructor
    ~TranslateManip();
};

} // namespace Gui

#endif // GUI_SOTEXTLABEL_H

