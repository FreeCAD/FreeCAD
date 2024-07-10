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

#include "GuiDisplay.h"
#include "OpenGlWrapper.h"
#include "MillSimulation.h"
#include <cstddef>
#include "GlUtils.h"
#include <stdlib.h>

using namespace MillSim;

GuiItem guiItems[] = {
    {eGuiItemSlider,          0, 0, 270, 564, 0},
    {eGuiItemThumb,           0, 0, 358, 550, 1},
    {eGuiItemPause,           0, 0, 70, 550, 'P', true},
    {eGuiItemPlay,            0, 0, 70, 550, 'S', false},
    {eGuiItemSingleStep,      0, 0, 110, 550, 'T'},
    {eGuiItemFaster,          0, 0, 150, 550, 'F'},
    {eGuiItemRotate,          0, 0, 630, 550, ' ', false, GUIITEM_CHECKABLE},
    {eGuiItemCharXImg,        0, 0, 190, 550, 0, false, 0},  // 620
    {eGuiItemChar0Img,        0, 0, 230, 550, 0, false, 0},
    {eGuiItemChar1Img,        0, 0, 215, 550, 0, false, 0},
    {eGuiItemChar4Img,        0, 0, 210, 550, 0, true, 0},
    {eGuiItemPath,            0, 0, 670, 550, 'L', false, GUIITEM_CHECKABLE},
    {eGuiItemAmbientOclusion, 0, 0, 710, 550, 'A', false, GUIITEM_CHECKABLE},
    {eGuiItemView,            0, 0, 590, 550, 'V', false},
};

#define NUM_GUI_ITEMS (sizeof(guiItems) / sizeof(GuiItem))
#define TEX_SIZE 256

std::vector<std::string> guiFileNames = {"Slider.png",
                                         "Thumb.png",
                                         "Pause.png",
                                         "Play.png",
                                         "SingleStep.png",
                                         "Faster.png",
                                         "Rotate.png",
                                         "X.png",
                                         "0.png",
                                         "1.png",
                                         "4.png",
                                         "Path.png",
                                         "AmbientOclusion.png",
                                         "View.png"};

bool GuiDisplay::GenerateGlItem(GuiItem* guiItem)
{
    Vertex2D verts[4];
    int x = guiItem->texItem.tx;
    int y = guiItem->texItem.ty;
    int w = guiItem->texItem.w;
    int h = guiItem->texItem.h;

    verts[0] = {0, (float)h, mTexture.getTexX(x), mTexture.getTexY(y + h)};
    verts[1] = {(float)w, (float)h, mTexture.getTexX(x + w), mTexture.getTexY(y + h)};
    verts[2] = {0, 0, mTexture.getTexX(x), mTexture.getTexY(y)};
    verts[3] = {(float)w, 0, mTexture.getTexX(x + w), mTexture.getTexY(y)};

    // vertex buffer
    glGenBuffers(1, &(guiItem->vbo));
    glBindBuffer(GL_ARRAY_BUFFER, guiItem->vbo);
    glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Vertex2D), verts, GL_STATIC_DRAW);

    // glDrawElements(GL_TRIANGLES, numIndices, GL_UNSIGNED_SHORT, nullptr);
    //  vertex array
    glGenVertexArrays(1, &(guiItem->vao));
    glBindVertexArray(guiItem->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(Vertex2D),
                          (void*)offsetof(Vertex2D, tx));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBindVertexArray(0);

    return true;
}

void MillSim::GuiDisplay::DestroyGlItem(GuiItem* guiItem)
{
    GLDELETE_BUFFER((guiItem->vbo));
    GLDELETE_VERTEXARRAY((guiItem->vao));
}

bool GuiDisplay::InitGui()
{
    if (guiInitiated) {
        return true;
    }
    // index buffer
    glGenBuffers(1, &mIbo);
    GLshort indices[6] = {0, 2, 3, 0, 3, 1};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLushort), indices, GL_STATIC_DRAW);
    TextureLoader tLoader(":/gl_simulator/", guiFileNames, TEX_SIZE);
    unsigned int* buffer = tLoader.GetRawData();
    if (buffer == nullptr) {
        return false;
    }
    mTexture.LoadImage(buffer, TEX_SIZE, TEX_SIZE);
    for (int i = 0; i < NUM_GUI_ITEMS; i++) {
        guiItems[i].texItem = *tLoader.GetTextureItem(i);
        GenerateGlItem(&(guiItems[i]));
    }

    mThumbStartX = guiItems[eGuiItemSlider].sx - guiItems[eGuiItemThumb].texItem.w / 2;
    mThumbMaxMotion = (float)guiItems[eGuiItemSlider].texItem.w;

    UpdateSimSpeed(1);

    // shader
    mat4x4 projmat;
    // mat4x4 viewmat;
    mat4x4_ortho(projmat, 0, WINDSIZE_W, WINDSIZE_H, 0, -1, 1);
    mShader.CompileShader((char*)VertShader2DTex, (char*)FragShader2dTex);
    mShader.UpdateTextureSlot(0);
    mShader.UpdateProjectionMat(projmat);
    guiInitiated = true;
    return true;
}

void GuiDisplay::ResetGui()
{
    mShader.Destroy();
    for (int i = 0; i < NUM_GUI_ITEMS; i++) {
        DestroyGlItem(&(guiItems[i]));
    }
    mTexture.DestroyTexture();
    GLDELETE_BUFFER(mIbo);
    guiInitiated = false;
}

void GuiDisplay::RenderItem(int itemId)
{
    GuiItem* item = &(guiItems[itemId]);
    if (item->hidden) {
        return;
    }
    mat4x4 model;
    mat4x4_translate(model, (float)item->sx, (float)item->sy, 0);
    mShader.UpdateModelMat(model, nullptr);
    if (item == mPressedItem) {
        mShader.UpdateObjColor(mPressedColor);
    }
    else if (item->mouseOver) {
        mShader.UpdateObjColor(mHighlightColor);
    }
    else if (itemId > 1 && item->actionKey == 0) {
        mShader.UpdateObjColor(mTextColor);
    }
    else if (item->flags & GUIITEM_CHECKED) {
        mShader.UpdateObjColor(mToggleColor);
    }
    else {
        mShader.UpdateObjColor(mStdColor);
    }

    glBindVertexArray(item->vao);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
}

void GuiDisplay::MouseCursorPos(int x, int y)
{
    mMouseOverItem = nullptr;
    for (int i = 0; i < NUM_GUI_ITEMS; i++) {
        GuiItem* g = &(guiItems[i]);
        if (g->actionKey == 0) {
            continue;
        }
        g->mouseOver = !g->hidden &&
            (x > g->sx && y > g->sy && x < (g->sx + g->texItem.w) && y < (g->sy + g->texItem.h));
        if (g->mouseOver) {
            mMouseOverItem = g;
        }
    }
}

void MillSim::GuiDisplay::HandleActionItem(GuiItem* guiItem)
{
    if (guiItem->actionKey >= 32) {
        if (guiItem->flags & GUIITEM_CHECKABLE) {
            guiItem->flags ^= GUIITEM_CHECKED;
        }
        bool isChecked = (guiItem->flags & GUIITEM_CHECKED) != 0;
        mMillSim->HandleGuiAction(guiItem->name, isChecked);
    }
}

void GuiDisplay::MousePressed(int button, bool isPressed, bool isSimRunning)
{
    if (button == MS_MOUSE_LEFT) {
        if (isPressed) {
            //mPressedItem = nullptr;
            if (mMouseOverItem != nullptr) {
                mPressedItem = mMouseOverItem;
                HandleActionItem(mPressedItem);
            }
        }
        else  // button released
        {
            UpdatePlayState(isSimRunning);
            if (mPressedItem != nullptr) {
                MouseCursorPos(mPressedItem->sx + 1, mPressedItem->sy + 1);
                mPressedItem = nullptr;
            }
        }
    }
}

void GuiDisplay::MouseDrag(int buttons, int dx, int dy)
{
    if (mPressedItem == nullptr) {
        return;
    }
    if (mPressedItem->name == eGuiItemThumb) {
        int newx = mPressedItem->sx + dx;
        if (newx < mThumbStartX) {
            newx = mThumbStartX;
        }
        if (newx > ((int)mThumbMaxMotion + mThumbStartX)) {
            newx = (int)mThumbMaxMotion + mThumbStartX;
        }
        if (newx != mPressedItem->sx) {
            mMillSim->SetSimulationStage((float)(newx - mThumbStartX) / mThumbMaxMotion);
            mPressedItem->sx = newx;
        }
    }
}

void GuiDisplay::UpdatePlayState(bool isRunning)
{
    guiItems[eGuiItemPause].hidden = !isRunning;
    guiItems[eGuiItemPlay].hidden = isRunning;
}

void MillSim::GuiDisplay::UpdateSimSpeed(int speed)
{
    guiItems[eGuiItemChar0Img].hidden = speed == 1;
    guiItems[eGuiItemChar1Img].hidden = speed == 40;
    guiItems[eGuiItemChar4Img].hidden = speed != 40;
}

void MillSim::GuiDisplay::HandleKeyPress(int key)
{
    for (int i = 0; i < NUM_GUI_ITEMS; i++) {
        GuiItem* g = &(guiItems[i]);
        if (g->actionKey == key) {
            HandleActionItem(g);
        }
    }
}

bool MillSim::GuiDisplay::IsChecked(eGuiItems item)
{
    return (guiItems[item].flags & GUIITEM_CHECKED) != 0;
}

void GuiDisplay::Render(float progress)
{
    if (mPressedItem == nullptr || mPressedItem->name != eGuiItemThumb) {
        guiItems[eGuiItemThumb].sx = (int)(mThumbMaxMotion * progress) + mThumbStartX;
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    mTexture.Activate();
    mShader.Activate();
    mShader.UpdateTextureSlot(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < NUM_GUI_ITEMS; i++) {
        RenderItem(i);
    }
}
