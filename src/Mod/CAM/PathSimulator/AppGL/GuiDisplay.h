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
		eGuiItemFaster,
		eGuiItemRotate,
		eGuiItemMax
	};

	struct GuiItem
	{
		const char* imageData;
		unsigned int vbo, vao;
		int tx, ty;  // texture location
		int w, h;    // item size
		int sx, sy;  // screen location
		int actionKey; // action key when item pressed
		bool hidden; // is item hidden
		bool mouseOver;
	};

	struct Vertex2D
	{
		float x, y;
		float tx, ty;
	};

	class GuiDisplay
	{
	public:
		//GuiDisplay() {};
		bool InutGui();
		void Render(float progress);
		void MouseCursorPos(int x, int y);
		void MousePressed(int button, bool isPressed, bool isRunning);
		void MouseDrag(int buttons, int dx, int dy);
		void SetMillSimulator(MillSimulation* millSim) { mMillSim = millSim; }
		void UpdatePlayState(bool isRunning);

	private:
		bool ParseImage(GuiItem* guiItem, unsigned int* buffPos, int stride);
		bool GenerateGlItem(GuiItem* guiItem);
		int ReadNextVal();
		bool ReadNextPixel(unsigned int *pix, int *amount);
		void RenderItem(int itemId);

		vec3 mStdColor = { 0.8f, 0.8f, 0.4f };
		vec3 mHighlightColor = { 1.0f, 1.0f, 0.9f };
		vec3 mPressedColor = { 1.0f, 0.5f, 0.0f };


	private:
		const char* mPixPos;
		Shader mShader;
		Texture mTexture;
		eGuiItems mPressedItem = eGuiItemMax;
		MillSimulation* mMillSim = nullptr;
		unsigned int mIbo = 0;
		int mThumbStartX = 0;
		float mThumbMaxMotion = 0;
	};

}

#endif // __guidisplay_t__
