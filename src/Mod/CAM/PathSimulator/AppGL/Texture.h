// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2024 Shai Seger
// SPDX-FileNotice: Part of the FreeCAD project.

/******************************************************************************
 *                                                                            *
 *   FreeCAD is free software: you can redistribute it and/or modify          *
 *   it under the terms of the GNU Lesser General Public License as           *
 *   published by the Free Software Foundation, either version 2.1            *
 *   of the License, or (at your option) any later version.                   *
 *                                                                            *
 *   FreeCAD is distributed in the hope that it will be useful,               *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty              *
 *   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                  *
 *   See the GNU Lesser General Public License for more details.              *
 *                                                                            *
 *   You should have received a copy of the GNU Lesser General Public         *
 *   License along with FreeCAD. If not, see https://www.gnu.org/licenses     *
 *                                                                            *
 ******************************************************************************/

#pragma once

namespace CAMSimulator
{

class Texture
{
public:
    Texture()
    {}
    ~Texture();
    void DestroyTexture();
    bool LoadImage(unsigned int* image, int x, int y);
    bool Activate();
    bool unbind();
    float getTexX(int imgX)
    {
        return (float)imgX / (float)width;
    }
    float getTexY(int imgY)
    {
        return (float)imgY / (float)height;
    }

public:
    int width = 0;
    int height = 0;


protected:
    unsigned int mTextureId = 0;
};

}  // namespace CAMSimulator
