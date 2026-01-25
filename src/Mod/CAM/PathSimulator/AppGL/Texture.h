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

namespace MillSim
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


}  // namespace MillSim
