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

#ifndef __texture_loader_h__
#define __texture_loader_h__
#include <string>
#include <vector>
#include <QImage>

namespace MillSim
{

struct TextureItem
{
    int tx{}, ty{};  // texture location
    int w{}, h{};    // item size
};

class TextureLoader
{
public:
    TextureLoader(std::string imgFolder, std::vector<std::string> fileNames, int textureSize);
    ~TextureLoader();
    unsigned int* GetRawData();
    TextureItem* GetTextureItem(int i);

protected:
    bool AddImage(TextureItem* guiItem, QImage& pixmap, unsigned int* buffPos, int stride);

protected:
    unsigned int* mRawData = nullptr;
    std::string mImageFolder;
};

}  // namespace MillSim
#endif  // !__texture_loader_h__
