#include "GlUtils.h"
#include "Shader.h"
#include <iostream>

namespace MillSim
{

	Shader* CurrentShader = nullptr;

	void Shader::UpdateModelMat(mat4x4 tmat, mat4x4 nmat)
	{
		if (modelPos >= 0)
			glUniformMatrix4fv(modelPos, 1, GL_FALSE, (GLfloat*)tmat);
		if (normalRotPos >= 0)
			glUniformMatrix4fv(normalRotPos, 1, GL_FALSE, (GLfloat*)nmat);
	}

	void Shader::UpdateProjectionMat(mat4x4 mat)
	{
		if (projectionPos >= 0)
			glUniformMatrix4fv(projectionPos, 1, GL_FALSE, (GLfloat*)mat);
	}

	void Shader::UpdateViewMat(mat4x4 mat)
	{
		if (viewPos >= 0)
			if (viewPos >= 0)
				glUniformMatrix4fv(viewPos, 1, GL_FALSE, (GLfloat*)mat);
	}

	void Shader::UpdateEnvColor(vec3 lightPos, vec3 lightColor, vec3 ambient)
	{
		if (lightPosPos >= 0)
			glUniform3fv(lightPosPos, 1, lightPos);
		if (lightColorPos >= 0)
			glUniform3fv(lightColorPos, 1, lightColor);
		if (ambientPos >= 0)
			glUniform3fv(ambientPos, 1, ambient);
	}

	void Shader::UpdateObjColor(vec3 objColor)
	{
		if (objectColorPos >= 0)
			glUniform3fv(objectColorPos, 1, objColor);
	}

	void Shader::UpdateTextureSlot(int slot)
	{
		if (texSlotPos >= 0)
			glUniform1i(texSlotPos, slot);
	}

	bool CheckCompileResult(int shader)
	{
#ifndef HAVE_OPENGL_EXT
		char log[1024];
		int res = 0;
		GLsizei len;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &res);
		if (res != 0)
			return false;
		glGetShaderInfoLog(shader, 1020, &len, log);
		if (len > 1020)
			len = 1020;
		log[len] = 0;
		std::cout << log << std::endl;
#endif
		return true;
	}

	unsigned int Shader::CompileShader(char* _vertShader, char* _fragShader)
	{
		vertShader = _vertShader;
		fragShader = _fragShader;
		const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
		GLint res = 0;
		glShaderSource(vertex_shader, 1, &vertShader, NULL);
		glCompileShader(vertex_shader);
		if (CheckCompileResult(vertex_shader))
			return 0xdeadbeef;

		const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragment_shader, 1, &fragShader, NULL);
		glCompileShader(fragment_shader);
		if (CheckCompileResult(fragment_shader))
			return 0xdeadbeef;

		shaderId = glCreateProgram();
		glAttachShader(shaderId, vertex_shader);
		glAttachShader(shaderId, fragment_shader);
		glLinkProgram(shaderId);

		glGetProgramiv(shaderId, GL_LINK_STATUS, &res);
		if (res == 0)
			return 0xdeadbeef;

		// get all uniform parameters positions
		modelPos = glGetUniformLocation(shaderId, "model");
		normalRotPos = glGetUniformLocation(shaderId, "normalRot");
		projectionPos = glGetUniformLocation(shaderId, "projection");
		viewPos = glGetUniformLocation(shaderId, "view");
		lightPosPos = glGetUniformLocation(shaderId, "lightPos");
		lightColorPos = glGetUniformLocation(shaderId, "lightColor");
		ambientPos = glGetUniformLocation(shaderId, "ambient");
		objectColorPos = glGetUniformLocation(shaderId, "objectColor");
		texSlotPos = glGetUniformLocation(shaderId, "texSlot");
		Activate();
		return shaderId;
	}

	void Shader::Activate()
	{
		if (shaderId > 0)
			glUseProgram(shaderId);
		CurrentShader = this;
	}




	const char* VertShader3DNorm =
		"#version 330 core  \n"

		"layout(location = 0) in vec3 aPosition;  \n"
		"layout(location = 1) in vec3 aNormal;  \n"

		"out vec3 Normal;  \n"
		"out vec3 FragPos;  \n"

		"uniform mat4 model;  \n"
		"uniform mat4 view;  \n"
		"uniform mat4 projection;  \n"
		"uniform mat4 normalRot;  \n"

		"void main(void)  \n"
		"{  \n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0);  \n"
		"	FragPos = vec3(model * vec4(aPosition, 1.0));  \n"
		"	Normal = vec3(normalRot * vec4(aNormal, 1.0));  \n"
		"}  \n";

	const char* VertShader3DInvNorm =
		"#version 330 core  \n"

		"layout(location = 0) in vec3 aPosition;  \n"
		"layout(location = 1) in vec3 aNormal;  \n"

		"out vec3 Normal;  \n"
		"out vec3 FragPos;  \n"

		"uniform mat4 model;  \n"
		"uniform mat4 view;  \n"
		"uniform mat4 projection;  \n"
		"uniform mat4 normalRot;  \n"

		"void main(void)  \n"
		"{  \n"
		"	gl_Position = projection * view * model * vec4(aPosition, 1.0);  \n"
		"	FragPos = vec3(model * vec4(aPosition, 1.0));  \n"
		"	Normal = -vec3(normalRot * vec4(aNormal, 1.0));  \n"
		"}  \n";


	const char* VertShader2DTex =
		"#version 330 core  \n"

		"layout(location = 0) in vec2 aPosition;  \n"
		"layout(location = 1) in vec2 aTexCoord;  \n"

		"out vec2 texCoord;  \n"

		"uniform mat4 projection;  \n"
		"uniform mat4 model;  \n"

		"void main(void)  \n"
		"{  \n"
		"	gl_Position = projection * model * vec4(aPosition, 0.0, 1.0);  \n"
		"	texCoord = aTexCoord;  \n"
		"}  \n";

	const char* FragShader2dTex =
		"#version 330\n"

		"out vec4 FragColor;  \n"
		"in vec2 texCoord;  \n"

		"uniform vec3 objectColor;  \n"
		"uniform sampler2D texSlot;  \n"

		"void main()  \n"
		"{  \n"
		"   vec4 texColor = texture(texSlot, texCoord);  \n"
		"	FragColor = vec4(objectColor, 1.0) * texColor;  \n"
		"}  \n";



	const char* FragShaderNorm =
		"#version 330\n"

		"out vec4 FragColor;  \n"

		"in vec3 Normal;  \n"
		"in vec3 FragPos;  \n"

		"uniform vec3 lightPos;  \n"
		"uniform vec3 lightColor;  \n"
		"uniform vec3 objectColor;  \n"
		"uniform vec3 ambient;  \n"

		"void main()  \n"
		"{  \n"
		"   vec3 norm = normalize(Normal);  \n"
		"   vec3 lightDir = normalize(lightPos - FragPos);  \n"
		"   float diff = max(dot(norm, lightDir), 0.0);  \n"
		"	vec3 diffuse = diff * lightColor;  \n"
		"	vec3 result = (ambient + diffuse) * objectColor;  \n"
		"	FragColor = vec4(result, 1.0);  \n"
		"}  \n";

	const char* FragShaderFlat =
		"#version 330\n"

		"out vec4 FragColor;  \n"

		"in vec3 Normal;  \n"
		"in vec3 FragPos;  \n"
		"uniform vec3 objectColor;  \n"

		"void main()  \n"
		"{  \n"
		"	FragColor = vec4(objectColor, 1.0);  \n"
		"}  \n";

}