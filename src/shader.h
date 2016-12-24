#pragma once

struct shader
{
	GLuint Program;
};

// TODO(hugo) : Shaders are assets. They should go through the 
// same pipeline.
shader LoadShader(const char* VertexPath, const char* FragmentPath)
{
	shader Result = {};

	char* VertexCode = ReadFileContent(VertexPath);
	char* FragmentCode = ReadFileContent(FragmentPath);

	GLint Success;
	GLchar InfoLog[512];

	GLuint Vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Vertex, 1, &VertexCode, 0);
	glCompileShader(Vertex);
	glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Vertex, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(VertexCode);

	GLuint Fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Fragment, 1, &FragmentCode, 0);
	glCompileShader(Fragment);
	glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Fragment, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(FragmentCode);

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, Vertex);
	glAttachShader(Result.Program, Fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}

	glDeleteShader(Vertex);
	glDeleteShader(Fragment);

	return(Result);
}

shader LoadShader(const char* VertexPath, const char* GeometryPath, const char* FragmentPath)
{
	shader Result = {};

	char* VertexCode = ReadFileContent(VertexPath);
	char* GeometryCode = ReadFileContent(FragmentPath);
	char* FragmentCode = ReadFileContent(FragmentPath);

	GLint Success;
	GLchar InfoLog[512];

	GLuint Vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(Vertex, 1, &VertexCode, 0);
	glCompileShader(Vertex);
	glGetShaderiv(Vertex, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Vertex, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(VertexCode);

	GLuint Geometry = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(Geometry, 1, &GeometryCode, 0);
	glCompileShader(Geometry);
	glGetShaderiv(Geometry, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Geometry, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(GeometryCode);

	GLuint Fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(Fragment, 1, &FragmentCode, 0);
	glCompileShader(Fragment);
	glGetShaderiv(Fragment, GL_COMPILE_STATUS, &Success);
	if (!Success)
	{
		glGetShaderInfoLog(Fragment, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}
	Free(FragmentCode);

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, Vertex);
	glAttachShader(Result.Program, Geometry);
	glAttachShader(Result.Program, Fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &Success);
	if (!Success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, InfoLog);
		SDL_Log("%s\n", InfoLog);
	}

	glDeleteShader(Vertex);
	glDeleteShader(Geometry);
	glDeleteShader(Fragment);

	return(Result);
}

inline void UseShader(shader Shader)
{
    glUseProgram(Shader.Program);
}
