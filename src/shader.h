#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

// TODO(hugo) : Use basic C parsing scheme (see OFF parsing in meshrekt)
struct shader
{
	GLuint Program;
};

shader LoadShader(const GLchar* vertexPath, const GLchar* fragmentPath)
{
	shader Result = {};
	std::string vertexCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;

	vShaderFile.exceptions(std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::badbit);
	try
	{
		vShaderFile.open(vertexPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream;
		std::stringstream fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		vShaderFile.close();
		fShaderFile.close();
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		SDL_Log("Error::shader::File not successfully read.\n");
	}
	const GLchar* vShaderCode = vertexCode.c_str();
	const GLchar* fShaderCode = fragmentCode.c_str();

	GLuint vertex;
	GLuint fragment;
	GLint success;
	GLchar infoLog[512];

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, 0);
	glCompileShader(vertex);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, 0);
	glCompileShader(fragment);
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, vertex);
	glAttachShader(Result.Program, fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	glDeleteShader(vertex);
	glDeleteShader(fragment);

	return(Result);
}

shader LoadShader(const GLchar* vertexPath, const GLchar* geometryPath, const GLchar* fragmentPath)
{
	shader Result = {};

	std::string vertexCode;
	std::string geometryCode;
	std::string fragmentCode;
	std::ifstream vShaderFile;
	std::ifstream gShaderFile;
	std::ifstream fShaderFile;

	vShaderFile.exceptions(std::ifstream::badbit);
	gShaderFile.exceptions(std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::badbit);
	try
	{
		vShaderFile.open(vertexPath);
		gShaderFile.open(geometryPath);
		fShaderFile.open(fragmentPath);
		std::stringstream vShaderStream;
		std::stringstream gShaderStream;
		std::stringstream fShaderStream;
		vShaderStream << vShaderFile.rdbuf();
		gShaderStream << gShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		vShaderFile.close();
		gShaderFile.close();
		fShaderFile.close();
		vertexCode = vShaderStream.str();
		geometryCode = gShaderStream.str();
		fragmentCode = fShaderStream.str();
	}
	catch (std::ifstream::failure e)
	{
		SDL_Log("Error::shader::File not successfully read.\n");
	}
	const GLchar* vShaderCode = vertexCode.c_str();
	const GLchar* gShaderCode = geometryCode.c_str();
	const GLchar* fShaderCode = fragmentCode.c_str();

	GLuint vertex;
	GLuint geometry;
	GLuint fragment;
	GLint success;
	GLchar infoLog[512];

	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, 0);
	glCompileShader(vertex);
	glGetShaderiv(vertex, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	geometry = glCreateShader(GL_GEOMETRY_SHADER);
	glShaderSource(geometry, 1, &gShaderCode, 0);
	glCompileShader(geometry);
	glGetShaderiv(geometry, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, 0);
	glCompileShader(fragment);
	glGetShaderiv(fragment, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(fragment, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	Result.Program = glCreateProgram();
	glAttachShader(Result.Program, vertex);
	glAttachShader(Result.Program, geometry);
	glAttachShader(Result.Program, fragment);
	glLinkProgram(Result.Program);
	glGetProgramiv(Result.Program, GL_LINK_STATUS, &success);
	if (!success)
	{
		glGetProgramInfoLog(Result.Program, 512, 0, infoLog);
		SDL_Log("%s\n", infoLog);
	}

	glDeleteShader(vertex);
	glDeleteShader(geometry);
	glDeleteShader(fragment);

	return(Result);
}

inline void UseShader(shader Shader)
{
    glUseProgram(Shader.Program);
}
