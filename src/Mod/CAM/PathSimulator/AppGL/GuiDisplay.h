// SPDX-License-Identifier: LGPL-2.1-or-later

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

#pragma once
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

struct DefaultGuiItem
{
    eGuiItems name;
    unsigned int vbo, vao;
    int sx, sy;      // screen location
    int actionKey;   // action key when item pressed
    bool hidden {};  // is item hidden
    unsigned int flags {};
};

class GuiDisplay;

class GuiItem: public DefaultGuiItem
{
public:
    explicit GuiItem(const DefaultGuiItem& item, GuiDisplay& d);

    int posx();
    int posy();
    void setPosx(int x);
    void setPosy(int y);

public:
    bool mouseOver {};
    TextureItem texItem {};
    QString toolTip {};

    GuiDisplay& display;
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
    GuiDisplay();

    bool InitGui();
    void ResetGui();
    void Render(float progress);
    void MouseCursorPos(int x, int y);
    void HandleActionItem(GuiItem* guiItem);
    void MousePressed(int button, bool isPressed, bool isRunning);
    void MouseDrag(int buttons, int dx, int dy);
    void SetMillSimulator(MillSimulation* millSim);
    void UpdatePlayState(bool isRunning);
    void UpdateSimSpeed(int speed);
    void HandleKeyPress(int key);
    bool IsChecked(eGuiItems item);
    void UpdateWindowScale(int width, int height);

    int width() const;
    int height() const;

public:
    bool guiInitiated = false;

private:
    void UpdateProjection();
    bool GenerateGlItem(GuiItem* guiItem);
    bool HStretchGlItem(GuiItem* guiItem, float newWidth, float edgeWidth);
    void DestroyGlItem(GuiItem* guiItem);
    void RenderItem(int itemId);
    void SetupTooltips();

    std::vector<GuiItem> mItems;

    int mWidth = -1;
    int mHeight = -1;

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
