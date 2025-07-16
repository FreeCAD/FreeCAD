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
GuiItem guiItems[] = {
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

#define NUM_GUI_ITEMS (sizeof(guiItems) / sizeof(GuiItem))
#define TEX_SIZE 256

std::vector<std::string> guiFileNames = {"Slider.png",
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
                                         "Home.png"};

void GuiDisplay::UpdateProjection()
{
    mat4x4 projmat;
    // mat4x4 viewmat;
    mat4x4_ortho(projmat, 0, gWindowSizeW, gWindowSizeH, 0, -1, 1);
    mShader.Activate();
    mShader.UpdateProjectionMat(projmat);
    mThumbMaxMotion = guiItems[eGuiItemAmbientOclusion].posx()
        + guiItems[eGuiItemAmbientOclusion].texItem.w
        - guiItems[eGuiItemSlider].posx();  // - guiItems[eGuiItemThumb].texItem.w;
    HStretchGlItem(&(guiItems[eGuiItemSlider]), mThumbMaxMotion, 10.0f);
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

bool MillSim::GuiDisplay::HStretchGlItem(GuiItem* guiItem, float newWidth, float edgeWidth)
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
    for (unsigned int i = 0; i < NUM_GUI_ITEMS; i++) {
        guiItems[i].texItem = *tLoader.GetTextureItem(i);
        GenerateGlItem(&(guiItems[i]));
    }

    mThumbStartX = guiItems[eGuiItemSlider].posx() - guiItems[eGuiItemThumb].texItem.w / 2;
    mThumbMaxMotion = (float)guiItems[eGuiItemSlider].texItem.w;

    // init shader
    mShader.CompileShader("GuiDisplay", (char*)VertShader2DTex, (char*)FragShader2dTex);
    mShader.UpdateTextureSlot(0);

    UpdateSimSpeed(1);
    UpdateProjection();
    guiInitiated = true;
    return true;
}

void GuiDisplay::ResetGui()
{
    mShader.Destroy();
    for (unsigned int i = 0; i < NUM_GUI_ITEMS; i++) {
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
    mat4x4_translate(model, (float)item->posx(), (float)item->posy(), 0);
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
    int nTriangles = (item->flags & GUIITEM_STRETCHED) == 0 ? 6 : 18;
    glDrawElements(GL_TRIANGLES, nTriangles, GL_UNSIGNED_SHORT, nullptr);
}

void MillSim::GuiDisplay::SetupTooltips()
{
    guiItems[eGuiItemPause].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Pause simulation", nullptr);
    guiItems[eGuiItemPlay].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Play simulation", nullptr);
    guiItems[eGuiItemSingleStep].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Single step simulation", nullptr);
    guiItems[eGuiItemSlower].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Decrease simulation speed", nullptr);
    guiItems[eGuiItemFaster].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Increase simulation speed", nullptr);
    guiItems[eGuiItemPath].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Show/Hide tool path", nullptr);
    guiItems[eGuiItemRotate].toolTip = QCoreApplication::translate("CAM:Simulator:Tooltips",
                                                                   "Toggle turn table animation",
                                                                   nullptr);
    guiItems[eGuiItemAmbientOclusion].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Toggle ambient oclusion", nullptr);
    guiItems[eGuiItemView].toolTip = QCoreApplication::translate("CAM:Simulator:Tooltips",
                                                                 "Toggle view simulation/model",
                                                                 nullptr);
    guiItems[eGuiItemHome].toolTip =
        QCoreApplication::translate("CAM:Simulator:Tooltips", "Reset camera", nullptr);
}

void GuiDisplay::MouseCursorPos(int x, int y)
{
    GuiItem* prevMouseOver = mMouseOverItem;
    mMouseOverItem = nullptr;
    for (unsigned int i = 0; i < NUM_GUI_ITEMS; i++) {
        GuiItem* g = &(guiItems[i]);
        if (g->actionKey == 0) {
            continue;
        }
        bool mouseCursorContained = x > g->posx() && x < (g->posx() + g->texItem.w) && y > g->posy()
            && y < (g->posy() + g->texItem.h);

        g->mouseOver = !g->hidden && mouseCursorContained;

        if (g->mouseOver) {
            mMouseOverItem = g;
        }
    }
    if (mMouseOverItem != prevMouseOver) {
        if (mMouseOverItem != nullptr && !mMouseOverItem->toolTip.isEmpty()) {
            QPoint pos(x, y);
            QPoint globPos = CAMSimulator::DlgCAMSimulator::GetInstance()->mapToGlobal(pos);
            QToolTip::showText(globPos, mMouseOverItem->toolTip);
        }
        else {
            QToolTip::hideText();
        }
    }
}

void MillSim::GuiDisplay::HandleActionItem(GuiItem* guiItem)
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

void GuiDisplay::UpdatePlayState(bool isRunning)
{
    guiItems[eGuiItemPause].hidden = !isRunning;
    guiItems[eGuiItemPlay].hidden = isRunning;
}

void MillSim::GuiDisplay::UpdateSimSpeed(int speed)
{
    guiItems[eGuiItem1].hidden = speed != 1;
    guiItems[eGuiItem5].hidden = speed != 5;
    guiItems[eGuiItem10].hidden = speed != 10;
    guiItems[eGuiItem25].hidden = speed != 25;
    guiItems[eGuiItem50].hidden = speed != 50;
}

void MillSim::GuiDisplay::HandleKeyPress(int key)
{
    for (unsigned int i = 0; i < NUM_GUI_ITEMS; i++) {
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

void MillSim::GuiDisplay::UpdateWindowScale()
{
    UpdateProjection();
}

void GuiDisplay::Render(float progress)
{
    if (mPressedItem == nullptr || mPressedItem->name != eGuiItemThumb) {
        guiItems[eGuiItemThumb].setPosx((int)(mThumbMaxMotion * progress) + mThumbStartX);
    }
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    mTexture.Activate();
    mShader.Activate();
    mShader.UpdateTextureSlot(0);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for (int i = 0; i < (int)NUM_GUI_ITEMS; i++) {
        RenderItem(i);
    }
}
