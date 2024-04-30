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

#ifndef __shader_h__
#define __shader_h__

#include "OpenGlWrapper.h"
#include "linmath.h"

namespace MillSim
{
	class Shader
	{
	public:
		Shader() {}

	public:
		unsigned int shaderId = 0;
		void UpdateModelMat(mat4x4 transformMat, mat4x4 normalMat);
		void UpdateProjectionMat(mat4x4 mat);
		void UpdateViewMat(mat4x4 mat);
		void UpdateEnvColor(vec3 lightPos, vec3 lightColor, vec3 ambient);
		void UpdateObjColor(vec3 objColor);
		void UpdateTextureSlot(int slot);
		unsigned int CompileShader(char* vertShader, char* fragShader);
		void Activate();


	protected:
		int modelPos = -1;
		int normalRotPos = -1;
		int projectionPos = -1;
		int viewPos = -1;
		int lightPosPos = -1;
		int lightColorPos = -1;
		int ambientPos = -1;
		int objectColorPos = -1;
		int texSlotPos = -1;

		const char* vertShader = nullptr;
		const char* fragShader = nullptr;
	};

	extern Shader* CurrentShader;

	extern const char* FragShaderNorm;
	extern const char* FragShaderFlat;
	extern const char* VertShader3DNorm;
	extern const char* VertShader3DInvNorm;
	extern const char* VertShader2DTex;
	extern const char* FragShader2dTex;
}
#endif // !__shader_h__

