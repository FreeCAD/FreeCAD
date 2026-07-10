// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2009 Werner Mayer <wmayer[at]users.sourceforge.net>
// SPDX-FileCopyrightText: 2026 Joao Matos
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1 of the     *
 *   License, or (at your option) any later version.                          *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful, but           *
 *   WITHOUT ANY WARRANTY; without even the implied warranty of               *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Lesser General Public License for more details.                      *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD.  If not, see                                *
 *   <https://www.gnu.org/licenses/>.                                         *
 *                                                                            *
 ******************************************************************************/

#pragma once

#include <Inventor/fields/SoSFBool.h>
#include <Inventor/fields/SoSFColor.h>
#include <Inventor/fields/SoSFEnum.h>
#include <Inventor/fields/SoSFInt32.h>
#include <Inventor/fields/SoSFName.h>
#include <Inventor/SbColor.h>
#include <Inventor/SbMatrix.h>
#include <Inventor/SbVec2f.h>
#include <Inventor/SbVec2s.h>
#include <Inventor/nodes/SoImage.h>
#include <Inventor/nodes/SoFaceSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoSwitch.h>
#include <Inventor/nodes/SoTexture2.h>
#include <Inventor/nodes/SoVertexProperty.h>
#include <Inventor/nodes/SoText2.h>
#include <FCGlobal.h>

#include "BitmapFactory.h"


namespace Gui
{

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
    ~SoStringLabel() override;
    void GLRender(SoGLRenderAction* action) override;
    void notify(SoNotList* list) override;

private:
    void ensureTextGeometry(SoState* state);

    mutable SoSwitch* textSwitch {nullptr};
    mutable SoSeparator* textSeparator {nullptr};
    mutable SoTexture2* textTexture {nullptr};
    mutable SoFaceSet* textFaceSet {nullptr};
    mutable SoVertexProperty* textVertexProperty {nullptr};
    mutable bool textGeometryDirty {true};
    mutable SbMatrix cachedModelMatrix;
    mutable SbMatrix cachedViewingMatrix;
    mutable SbMatrix cachedProjectionMatrix;
    mutable SbVec2s cachedViewportSize;
    mutable SbVec2f cachedAnchor;
    mutable int cachedImageWidth {0};
    mutable int cachedImageHeight {0};
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
    void prepareImage(SoState* state);
    void drawImage(const SbColor& effectiveBackground, const SbColor& effectiveText);

    bool imageDirty {true};
    bool effectiveColorsValid {false};
    SbColor cachedEffectiveBackground;
    SbColor cachedEffectiveText;
};

}  // namespace Gui
