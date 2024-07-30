/***************************************************************************
 *   Copyright (c) 2005 Werner Mayer <wmayer[at]users.sourceforge.net>     *
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

#ifndef GUI_SOFCBACKGROUNDGRADIENT_H
#define GUI_SOFCBACKGROUNDGRADIENT_H

#include <Inventor/SbColor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/nodes/SoSubNode.h>
#include <FCGlobal.h>


class SbColor;
class SoGLRenderAction;

namespace Gui {

class GuiExport SoFCBackgroundGradient : public SoNode {
    using inherited = SoNode;

    SO_NODE_HEADER(Gui::SoFCBackgroundGradient);

public:
    enum Gradient {
        LINEAR = 0,
        RADIAL = 1
    };
    static void initClass();
    static void finish();
    SoFCBackgroundGradient();

    void GLRender (SoGLRenderAction *action) override;
    void setGradient(Gradient grad);
    Gradient getGradient() const;
    void setColorGradient(const SbColor& fromColor,
                          const SbColor& toColor);
    void setColorGradient(const SbColor& fromColor,
                          const SbColor& toColor,
                          const SbColor& midColor);

private:
    Gradient gradient;

protected:
    ~SoFCBackgroundGradient() override;

    SbColor fCol, tCol, mCol;
};

} // namespace Gui


#endif // GUI_SOFCBACKGROUNDGRADIENT_H

