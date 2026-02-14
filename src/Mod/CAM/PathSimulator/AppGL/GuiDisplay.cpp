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

#include "GuiDisplay.h"
#include "OpenGlWrapper.h"
#include "MillSimulation.h"
#include <cstddef>
#include <QToolTip>
#include <QPoint>
#include <QCoreApplication>

using namespace MillSim;

// clang-format off
// NOLINTBEGIN(*-magic-numbers)
static const std::vector<DefaultGuiItem> defaultGuiItems = {
    {.name=eGuiItemSlider, .vbo=0, .vao=0, .sx=28, .sy=-80, .actionKey=0, .hidden=false, .flags=0},
    {.name=eGuiItemThumb, .vbo=0, .vao=0, .sx=328, .sy=-94, .actionKey=1, .hidden=false, .flags=0},
    {.name=eGuiItemPause, .vbo=0, .vao=0, .sx=28, .sy=-50, .actionKey='P', .hidden=true, .flags=0},
    {.name=eGuiItemPlay, .vbo=0, .vao=0, .sx=28, .sy=-50, .actionKey='S', .hidden=false, .flags=0},
    {.name=eGuiItemSingleStep, .vbo=0, .vao=0, .sx=68, .sy=-50, .actionKey='T', .hidden=false, .flags=0},
    {.name=eGuiItemSlower, .vbo=0, .vao=0, .sx=113, .sy=-50, .actionKey=' ', .hidden=false, .flags=0},
    {.name=eGuiItemFaster, .vbo=0, .vao=0, .sx=158, .sy=-50, .actionKey='F', .hidden=false, .flags=0},
    {.name=eGuiItemX, .vbo=0, .vao=0, .sx=208, .sy=-45, .actionKey=0, .hidden=false, .flags=0},
    {.name=eGuiItem1, .vbo=0, .vao=0, .sx=230, .sy=-50, .actionKey=0, .hidden=false, .flags=0},
    {.name=eGuiItem5, .vbo=0, .vao=0, .sx=230, .sy=-50, .actionKey=0, .hidden=true, .flags=0},
    {.name=eGuiItem10, .vbo=0, .vao=0, .sx=230, .sy=-50, .actionKey=0, .hidden=true, .flags=0},
    {.name=eGuiItem25, .vbo=0, .vao=0, .sx=230, .sy=-50, .actionKey=0, .hidden=true, .flags=0},
    {.name=eGuiItem50, .vbo=0, .vao=0, .sx=230, .sy=-50, .actionKey=0, .hidden=true, .flags=0},
    {.name=eGuiItemRotate, .vbo=0, .vao=0, .sx=-140, .sy=-50, .actionKey=' ', .hidden=false, .flags=GUIITEM_CHECKABLE},
    {.name=eGuiItemPath, .vbo=0, .vao=0, .sx=-100, .sy=-50, .actionKey='L', .hidden=false, .flags=GUIITEM_CHECKABLE},
    {.name=eGuiItemAmbientOclusion, .vbo=0, .vao=0, .sx=-60, .sy=-50, .actionKey='A', .hidden=false, .flags=GUIITEM_CHECKABLE},
    {.name=eGuiItemView, .vbo=0, .vao=0, .sx=-180, .sy=-50, .actionKey='V', .hidden=false, .flags=0},
    {.name=eGuiItemHome, .vbo=0, .vao=0, .sx=-220, .sy=-50, .actionKey='H', .hidden=false, .flags=0},
};
// NOLINTEND(*-magic-numbers)
// clang-format on

#define TEX_SIZE 256

static const std::vector<std::string> guiFileNames = {
    "Slider.png",
    "Thumb.png",
    "Pause.png",
    "Play.png",
    "SingleStep.png",
    "Slower.png",
    "Faster.png",
    "x.png",
    "1.png",
    "5.png",
    "10.png",
    "25.png",
    "50.png",
    "Rotate.png",
    "Path.png",
    "AmbientOclusion.png",
    "View.png",
    "Home.png"
};

GuiItem::GuiItem(const DefaultGuiItem& item, GuiDisplay& d)
    : DefaultGuiItem(item)
    , display(d)
{}

int GuiItem::posx()
{
    return sx >= 0 ? sx : display.width() + sx;
}

int GuiItem::posy()
{
    return sy >= 0 ? sy : display.height() + sy;
}

void GuiItem::setPosx(int x)
{
    sx = sx >= 0 ? x : x - display.width();
}

void GuiItem::setPosy(int y)
{
    sy = sy >= 0 ? y : y - display.height();
}

GuiDisplay::GuiDisplay()
{
    for (auto& item : defaultGuiItems) {
        mItems.emplace_back(item, *this);
    }
}

void GuiDisplay::UpdateProjection()
{
    mat4x4 projmat;
    // mat4x4 viewmat;
    mat4x4_ortho(projmat, 0, mWidth, mHeight, 0, -1, 1);
    mShader.Activate();
    mShader.UpdateProjectionMat(projmat);
    mThumbMaxMotion = mItems[eGuiItemAmbientOclusion].posx()
        + mItems[eGuiItemAmbientOclusion].texItem.w
        - mItems[eGuiItemSlider].posx();  // - guiItems[eGuiItemThumb].texItem.w;
    HStretchGlItem(&(mItems[eGuiItemSlider]), mThumbMaxMotion, 10.0f);
}

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
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, tx));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBindVertexArray(0);

    return true;
}

bool GuiDisplay::HStretchGlItem(GuiItem* guiItem, float newWidth, float edgeWidth)
{
    if (guiItem->vbo == 0) {
        return false;
    }

    DestroyGlItem(guiItem);
    Vertex2D verts[12];

    int x = guiItem->texItem.tx;
    int y = guiItem->texItem.ty;
    int h = guiItem->texItem.h;
    int w = guiItem->texItem.w;

    // left edge
    verts[0] = {0, (float)h, mTexture.getTexX(x), mTexture.getTexY(y + h)};
    verts[1] = {edgeWidth, (float)h, mTexture.getTexX(x + edgeWidth), mTexture.getTexY(y + h)};
    verts[2] = {0, 0, mTexture.getTexX(x), mTexture.getTexY(y)};
    verts[3] = {edgeWidth, 0, mTexture.getTexX(x + edgeWidth), mTexture.getTexY(y)};

    // right edge
    float px = newWidth - edgeWidth;
    verts[4] = {px, (float)h, mTexture.getTexX(x + w - edgeWidth), mTexture.getTexY(y + h)};
    verts[5] = {newWidth, (float)h, mTexture.getTexX(x + w), mTexture.getTexY(y + h)};
    verts[6] = {px, 0, mTexture.getTexX(x + w - edgeWidth), mTexture.getTexY(y)};
    verts[7] = {newWidth, 0, mTexture.getTexX(x + w), mTexture.getTexY(y)};

    // center
    verts[8] = {edgeWidth, (float)h, mTexture.getTexX(x + edgeWidth), mTexture.getTexY(y + h)};
    verts[9] = {px, (float)h, mTexture.getTexX(x + w - edgeWidth), mTexture.getTexY(y + h)};
    verts[10] = {edgeWidth, 0, mTexture.getTexX(x + edgeWidth), mTexture.getTexY(y)};
    verts[11] = {px, 0, mTexture.getTexX(x + w - edgeWidth), mTexture.getTexY(y)};

    guiItem->flags |= GUIITEM_STRETCHED;

    // vertex buffer
    glGenBuffers(1, &(guiItem->vbo));
    glBindBuffer(GL_ARRAY_BUFFER, guiItem->vbo);
    glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(Vertex2D), verts, GL_STATIC_DRAW);

    glGenVertexArrays(1, &(guiItem->vao));
    glBindVertexArray(guiItem->vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, x));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, tx));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBindVertexArray(0);

    return true;
}

void GuiDisplay::DestroyGlItem(GuiItem* guiItem)
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
    SetupTooltips();
    glGenBuffers(1, &mIbo);
    GLshort indices[18] = {0, 2, 3, 0, 3, 1, 4, 6, 7, 4, 7, 5, 8, 10, 11, 8, 11, 9};
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mIbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 18 * sizeof(GLushort), indices, GL_STATIC_DRAW);
    TextureLoader tLoader(":/gl_simulator/", guiFileNames, TEX_SIZE);
    unsigned int* buffer = tLoader.GetRawData();
    if (buffer == nullptr) {
        return false;
    }
    mTexture.LoadImage(buffer, TEX_SIZE, TEX_SIZE);
    for (unsigned int i = 0; i < mItems.size(); i++) {
        auto& item = mItems[i];
        item.texItem = *tLoader.GetTextureItem(i);
        GenerateGlItem(&item);
    }

    mThumbStartX = mItems[eGuiItemSlider].posx() - mItems[eGuiItemThumb].texItem.w / 2;
    mThumbMaxMotion = (float)mItems[eGuiItemSlider].texItem.w;

    // init shader
    mShader.CompileShader("GuiDisplay", (char*)VertShader2DTex, (char*)FragShader2dTex);
    mShader.UpdateTextureSlot(0);

    UpdatePlayState(false);
    UpdateSimSpeed(1);
    guiInitiated = true;

    UpdateWindowScale(800, 600);
    return true;
}

void GuiDisplay::ResetGui()
{
    mShader.Destroy();
    for (auto& item : mItems) {
        DestroyGlItem(&item);
    }
    mTexture.DestroyTexture();
    GLDELETE_BUFFER(mIbo);
    guiInitiated = false;
}

void GuiDisplay::RenderItem(int itemId)
{
    GuiItem* item = &(mItems[itemId]);
    if (item->hidden) {
        return;
    }
    mat4x4 model;
    mat4x4_translate(model, (float)item->posx(), (float)item->posy(), 0);
    mShader.UpdateModelMat(model, {});
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
    int nTriangles = (item->flags & GUIITEM_STRETCHED) == 0 ? 6 : 18;
    glDrawElements(GL_TRIANGLES, nTriangles, GL_UNSIGNED_SHORT, nullptr);
}

void GuiDisplay::SetupTooltips()
{
    mItems[eGuiItemPause].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Pause simulation", nullptr);
    mItems[eGuiItemPlay].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Play simulation", nullptr);
    mItems[eGuiItemSingleStep].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Single step simulation", nullptr);
    mItems[eGuiItemSlower].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Decrease simulation speed", nullptr);
    mItems[eGuiItemFaster].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Increase simulation speed", nullptr);
    mItems[eGuiItemPath].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Show/Hide tool path", nullptr);
    mItems[eGuiItemRotate].toolTip = QCoreApplication::translate(
        "CAM:Simulator:Tooltips",
        "Toggle turn table animation",
        nullptr
    );
    mItems[eGuiItemAmbientOclusion].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Toggle ambient occlusion", nullptr);
    mItems[eGuiItemView].toolTip = QCoreApplication::translate(
        "CAM:Simulator:Tooltips",
        "Toggle view simulation/model",
        nullptr
    );
    mItems[eGuiItemHome].toolTip
        = QCoreApplication::translate("CAM:Simulator:Tooltips", "Reset camera", nullptr);
}

void GuiDisplay::MouseCursorPos(int x, int y)
{
    GuiItem* prevMouseOver = mMouseOverItem;
    mMouseOverItem = nullptr;
    for (auto& item : mItems) {
        if (item.actionKey == 0) {
            continue;
        }
        bool mouseCursorContained = x > item.posx() && x < (item.posx() + item.texItem.w)
            && y > item.posy() && y < (item.posy() + item.texItem.h);

        item.mouseOver = !item.hidden && mouseCursorContained;

        if (item.mouseOver) {
            mMouseOverItem = &item;
        }
    }
    if (mMouseOverItem != prevMouseOver) {
        if (mMouseOverItem != nullptr && !mMouseOverItem->toolTip.isEmpty()) {
            const QWidget* w = CAMSimulator::DlgCAMSimulator::instance();
            const float ratio = w->devicePixelRatioF();
            const QPoint pos(x / ratio, y / ratio);
            const QPoint globPos = w->mapToGlobal(pos);
            QToolTip::showText(globPos, mMouseOverItem->toolTip);
        }
        else {
            QToolTip::hideText();
        }
    }
}

void GuiDisplay::HandleActionItem(GuiItem* guiItem)
{
    if (guiItem->actionKey >= ' ') {
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
            if (mMouseOverItem != nullptr) {
                mPressedItem = mMouseOverItem;
                HandleActionItem(mPressedItem);
            }
        }
        else  // button released
        {
            UpdatePlayState(isSimRunning);
            if (mPressedItem != nullptr) {
                MouseCursorPos(mPressedItem->posx() + 1, mPressedItem->posy() + 1);
                mPressedItem = nullptr;
            }
        }
    }
}

void GuiDisplay::MouseDrag(int /* buttons */, int dx, int /* dy */)
{
    if (mPressedItem == nullptr) {
        return;
    }
    if (mPressedItem->name == eGuiItemThumb) {
        int newx = mPressedItem->posx() + dx;
        if (newx < mThumbStartX) {
            newx = mThumbStartX;
        }
        if (newx > ((int)mThumbMaxMotion + mThumbStartX)) {
            newx = (int)mThumbMaxMotion + mThumbStartX;
        }
        if (newx != mPressedItem->posx()) {
            mMillSim->SetSimulationStage((float)(newx - mThumbStartX) / mThumbMaxMotion);
            mPressedItem->setPosx(newx);
        }
    }
}

void GuiDisplay::SetMillSimulator(MillSimulation* millSim)
{
    mMillSim = millSim;
}

void GuiDisplay::UpdatePlayState(bool isRunning)
{
    mItems[eGuiItemPause].hidden = !isRunning;
    mItems[eGuiItemPlay].hidden = isRunning;
}

void GuiDisplay::UpdateSimSpeed(int speed)
{
    mItems[eGuiItem1].hidden = speed != 1;
    mItems[eGuiItem5].hidden = speed != 5;
    mItems[eGuiItem10].hidden = speed != 10;
    mItems[eGuiItem25].hidden = speed != 25;
    mItems[eGuiItem50].hidden = speed != 50;
}

void GuiDisplay::HandleKeyPress(int key)
{
    for (auto& item : mItems) {
        if (item.actionKey == key) {
            HandleActionItem(&item);
        }
    }
}

bool GuiDisplay::IsChecked(eGuiItems item)
{
    return (mItems[item].flags & GUIITEM_CHECKED) != 0;
}

void GuiDisplay::UpdateWindowScale(int width, int height)
{
    if (!guiInitiated || (width == mWidth && height == mHeight)) {
        return;
    }

    mWidth = width;
    mHeight = height;

    UpdateProjection();
}

int GuiDisplay::width() const
{
    return mWidth;
}

int GuiDisplay::height() const
{
    return mHeight;
}

void GuiDisplay::Render(float progress)
{
    if (mPressedItem == nullptr || mPressedItem->name != eGuiItemThumb) {
        mItems[eGuiItemThumb].setPosx((int)(mThumbMaxMotion * progress) + mThumbStartX);
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    mTexture.Activate();
    mShader.Activate();
    mShader.UpdateTextureSlot(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < (int)mItems.size(); i++) {
        RenderItem(i);
    }
}
