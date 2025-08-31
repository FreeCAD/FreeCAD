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

#include "TextureLoader.h"

using namespace MillSim;

TextureItem texItems[] = {
    {1, 40, 0, 0},
    {1, 1, 0, 0},
    {70, 1, 0, 0},
    {100, 1, 0, 0},
    {135, 1, 0, 0},
    {30, 1, 0, 0},
    {170, 1, 0, 0},
    {1, 130, 0, 0},
    {30, 130, 0, 0},
    {55, 130, 0, 0},
    {85, 130, 0, 0},
    {140, 130, 0, 0},
    {195, 130, 0, 0},
    {210, 1, 0, 0},
    {95, 50, 0, 0},
    {130, 50, 0, 0},
    {170, 50, 0, 0},
    {210, 50, 0, 0},
};


#define NUM_TEX_ITEMS (sizeof(texItems) / sizeof(TextureItem))

int sssize = -1;

TextureLoader::TextureLoader(std::string imgFolder,
                             std::vector<std::string> fileNames,
                             int textureSize)
    : mImageFolder(imgFolder)
{
    size_t buffsize =
        static_cast<size_t>(textureSize) * static_cast<size_t>(textureSize) * sizeof(unsigned int);
    mRawData = (unsigned int*)malloc(buffsize);
    if (mRawData == nullptr) {
        return;
    }
    memset(mRawData, 0x00, buffsize);
    for (std::size_t i = 0; i < fileNames.size(); i++) {
        QImage pixmap((imgFolder + fileNames[i]).c_str());
        AddImage(&(texItems[i]), pixmap, mRawData, textureSize);
    }
}

// parse compressed image into a texture buffer
bool TextureLoader::AddImage(TextureItem* texItem,
                             QImage& pixmap,
                             unsigned int* buffPos,
                             int stride)
{
    int width = pixmap.width();
    int height = pixmap.height();
    buffPos += stride * texItem->ty + texItem->tx;
    for (int i = 0; i < height; i++) {
        unsigned int* line = reinterpret_cast<unsigned int*>(pixmap.scanLine(i));
        for (int j = 0; j < width; j++) {
            buffPos[j] = line[j];
        }
        buffPos += stride;
    }
    texItem->w = width;
    texItem->h = height;
    return true;
}

MillSim::TextureLoader::~TextureLoader()
{
    if (mRawData != nullptr) {
        free(mRawData);
    }
}

unsigned int* MillSim::TextureLoader::GetRawData()
{
    return mRawData;
}

TextureItem* MillSim::TextureLoader::GetTextureItem(int i)
{
    return texItems + i;
}
