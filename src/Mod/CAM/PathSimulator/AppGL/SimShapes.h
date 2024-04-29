#ifndef __sim_shapes_h__
#define __sim_shapes_h__
#include "OpenGlWrapper.h"
#include "linmath.h"

#define SET_DUAL(var, idx, y, z) {var[idx++] = y; var[idx++] = z; }
#define SET_TRIPLE(var, idx, x, y, z) {var[idx++] = x; var[idx++] = y; var[idx++] = z; }

//#define SET_TRIPLE(var, idx, x, y, z) {var[idx] = x; var[idx+1] = y; var[idx+2] = z;}
//#define SET_TRIPLE_OFFS(var, idx, offs, x, y, z) {var[idx] = x + offs; var[idx+1] = y + offs; var[idx+2] = z + offs;}

#define SET_TRIPLE_OFFS(var, idx, offs, x, y, z) {var[idx++] = x + offs; var[idx++] = y + offs; var[idx++] = z + offs; }

namespace MillSim
{
	typedef unsigned int uint;

	struct Vertex
	{
		float x, y, z;
		float nx, ny, nz;
	};

	class Shape OpenGlInherit
	{
	public:
		Shape() {}
		~Shape();

	public:
		uint vao = 0;
		uint vbo = 0;
		uint ibo = 0;
		int numIndices = 0;

	public:
		void Render();
		void Render(mat4x4 modelMat, mat4x4 normallMat);
		void FreeResources();
		void RotateProfile(float* profPoints, int nPoints, float distance, float deltaHeight, int nSlices, bool isHalfTurn);
		void ExtrudeProfileRadial(float* profPoints, int nPoints, float radius, float angleRad, float deltaHeight, bool capStart, bool capEnd);
		void ExtrudeProfileLinear(float* profPoints, int nPoints, float fromX, float toX, float fromZ, float toZ, bool capStart, bool capEnd);

	protected:
		void GenerateModel(float* vbuffer, GLushort* ibuffer, int numVerts, int numIndices);
		void CalculateExtrudeBufferSizes(int nProfilePoints, bool capStart, bool capEnd,
			int* numVerts, int* numIndices, int* vc1idx, int* vc2idx, int* ic1idx, int* ic2idx);
	};

}
#endif


