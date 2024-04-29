#include "GlUtils.h"
#include <iostream>

#define ASSERT(X) if (!(X)) __debugbreak()

namespace MillSim
{

	int gDebug = -1;

	mat4x4 identityMat = { {1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1} };

	void GLClearError()
	{
		while (glGetError() != GL_NO_ERROR);
	}

	bool GLLogError()
	{
		while (GLenum err = glGetError())
		{
			std::cout << "[Opengl Error] (" << err << ")" << std::endl;
			return false;
		}
		return true;
	}


	typedef struct Vertex
	{
		vec3 pos;
		vec3 col;
	} Vertex;

}
