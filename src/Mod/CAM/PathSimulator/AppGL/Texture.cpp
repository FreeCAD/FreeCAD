#include "Texture.h"
#include "GlUtils.h"

namespace MillSim
{
	Texture::~Texture()
	{
		glDeleteTextures(1, &mTextureId);
	}

	bool Texture::LoadImage(unsigned int* image, int width, int height)
	{
		mWidth = width;
		mHeight = height;
		glGenTextures(1, &mTextureId);
		glBindTexture(GL_TEXTURE_2D, mTextureId);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	} 

	bool Texture::Activate()
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, mTextureId);
		return true;
	}

	bool Texture::unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

}