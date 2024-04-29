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
		GL(glGenTextures(1, &mTextureId));
		GL(glBindTexture(GL_TEXTURE_2D, mTextureId));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
		GL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
		GL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image));
		GL(glBindTexture(GL_TEXTURE_2D, 0));
		return true;
	} 

	bool Texture::Activate()
	{
		GL(glActiveTexture(GL_TEXTURE0));
		GL(glBindTexture(GL_TEXTURE_2D, mTextureId));
		return true;
	}

	bool Texture::unbind()
	{
		glBindTexture(GL_TEXTURE_2D, 0);
		return true;
	}

}