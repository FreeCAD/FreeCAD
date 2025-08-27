/***************************************************************************
 *   Copyright (c) 2024 Shai Seger <shaise at gmail>                       *
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

#ifndef __guidisplay_t__
#define __guidisplay_t__
#include "OpenGlWrapper.h"
#include "Texture.h"
#include "Shader.h"
#include "TextureLoader.h"
#include "GlUtils.h"
#include <QString>

namespace MillSim
{
class MillSimulation;

enum eGuiItems
{
    eGuiItemSlider,
    eGuiItemThumb,
    eGuiItemPause,
    eGuiItemPlay,
    eGuiItemSingleStep,
    eGuiItemSlower,
    eGuiItemFaster,
    eGuiItemX,
    eGuiItem1,
    eGuiItem5,
    eGuiItem10,
    eGuiItem25,
    eGuiItem50,
    eGuiItemRotate,
    eGuiItemPath,
    eGuiItemAmbientOclusion,
    eGuiItemView,
    eGuiItemHome,
    eGuiItemMax  // this element must be the last item always
};

struct GuiItem
{
    eGuiItems name;
    unsigned int vbo, vao;
    int sx, sy;      // screen location
    int actionKey;   // action key when item pressed
    bool hidden {};  // is item hidden
    unsigned int flags {};
    bool mouseOver {};
    TextureItem texItem {};
    QString toolTip {};

    int posx()
    {
        return sx >= 0 ? sx : gWindowSizeW + sx;
    }
    int posy()
    {
        return sy >= 0 ? sy : gWindowSizeH + sy;
    }
    void setPosx(int x)
    {
        sx = sx >= 0 ? x : x - gWindowSizeW;
    }
    void setPosy(int y)
    {
        sy = sy >= 0 ? y : y - gWindowSizeH;
    }
};

#define GUIITEM_CHECKABLE 0x01
#define GUIITEM_CHECKED 0x02
#define GUIITEM_STRETCHED 0x04


struct Vertex2D
{
    float x, y;
    float tx, ty;
};

class GuiDisplay
{
public:
    // GuiDisplay() {};
    bool InitGui();
    void ResetGui();
    void Render(float progress);
    void MouseCursorPos(int x, int y);
    void HandleActionItem(GuiItem* guiItem);
    void MousePressed(int button, bool isPressed, bool isRunning);
    void MouseDrag(int buttons, int dx, int dy);
    void SetMillSimulator(MillSimulation* millSim)
    {
        mMillSim = millSim;
    }
    void UpdatePlayState(bool isRunning);
    void UpdateSimSpeed(int speed);
    void HandleKeyPress(int key);
    bool IsChecked(eGuiItems item);
    void UpdateWindowScale();

public:
    bool guiInitiated = false;

private:
    void UpdateProjection();
    bool GenerateGlItem(GuiItem* guiItem);
    bool HStretchGlItem(GuiItem* guiItem, float newWidth, float edgeWidth);
    void DestroyGlItem(GuiItem* guiItem);
    void RenderItem(int itemId);
    void SetupTooltips();

    vec3 mStdColor = {0.8f, 0.8f, 0.4f};
    vec3 mToggleColor = {0.9f, 0.6f, 0.2f};
    vec3 mHighlightColor = {1.0f, 1.0f, 0.9f};
    vec3 mPressedColor = {1.0f, 0.5f, 0.0f};
    vec3 mTextColor = {1.0f, 0.5f, 0.0f};

    Shader mShader;
    Texture mTexture;
    GuiItem* mPressedItem = nullptr;
    GuiItem* mMouseOverItem = nullptr;
    MillSimulation* mMillSim = nullptr;
    unsigned int mIbo = 0;
    int mThumbStartX = 0;
    float mThumbMaxMotion = 0;
};

}  // namespace MillSim

#endif  // __guidisplay_t__
