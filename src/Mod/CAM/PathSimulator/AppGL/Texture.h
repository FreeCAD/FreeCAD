#ifndef __texture_h__
#define __texture_h__
#include "OpenGlWrapper.h"

namespace MillSim
{

	class Texture
	{
	public:
		Texture() {}
		~Texture();
		bool LoadImage(unsigned int* image, int x, int y);
		bool Activate();
		bool unbind();
		float getTexX(int imgX) { return (float)imgX / (float)mWidth; }
		float getTexY(int imgY) { return (float)imgY / (float)mHeight; }

	public:
		int mWidth = 0, mHeight = 0;


	protected:
		unsigned int mTextureId = -1;


	};


}

#endif // !__texture_h__

