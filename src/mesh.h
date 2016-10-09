#pragma once

#include <vector>
#include <fstream>
#include <unordered_map>
#include <iostream>

// TODO(hugo) : Find out how to get rid of this (but difficult problem since this isn't in the spec :( ... what's the most common case ?)
#define OBJ_VERTEX_OFFSET 1
#define GLIEWER_TRIANGLE_SUPPORT 1

// TODO(hugo) : Un-std::hash this. I think std::vector are fine for the time being.

/*
 * NOTE(hugo)
 * A mesh must absolutely have : position. Texture and normals are optionals.
 * A file has to give us : position and triangles. Texuture and normal are optionals.
 */
struct vertex
{
	// NOTE(hugo): float or GLfloat or v3/4 ?
	v3 Position;
	v3 Normal;
	v2 Texture;

    bool operator==(const vertex& V) const
    {
        return(Position.x == V.Position.x &&
           Position.y == V.Position.y &&
           Position.z == V.Position.z &&
           Normal.x == V.Normal.x &&
           Normal.y == V.Normal.y &&
           Normal.z == V.Normal.z &&
           Texture.u == V.Texture.u &&
           Texture.v == V.Texture.v);
    }
};

struct hash_vertex
{
    // TODO(hugo) : BETTER HASH FUNCTIONS ! 
    size_t operator()(const vertex& V) const
    {
        return (std::hash<float>()(V.Position.x) ^ std::hash<float>()(V.Position.y) ^ std::hash<float>()(V.Position.z));
    }
};

struct triangle
{
	unsigned int Vertices[3];
};

struct mesh
{
    // NOTE(hugo) : OpenGL Buffers
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

    // NOTE(hugo) : Mesh Data
	std::vector<vertex> Vertices;
	std::vector<triangle> Faces;

	mat4 ModelMatrix;
};


// TODO(hugo) : maybe consider changing vec3 to vec4 (faster for OpenGL) ?
void ComputeNormal(mesh* Mesh)
{
	Assert(Mesh != 0);
	for (int TriangleIndex = 0; TriangleIndex < Mesh->Faces.size(); ++TriangleIndex)
	{
		triangle t = Mesh->Faces[TriangleIndex];
		v3 v0 = Mesh->Vertices[t.Vertices[0]].Position;
		v3 v1 = Mesh->Vertices[t.Vertices[1]].Position;
		v3 v2 = Mesh->Vertices[t.Vertices[2]].Position;

		// TODO(hugo) : investigate with the angle to see if I do not get a "negative" normal vector
		// TODO(hugo) : need to normalize N after computation ?
		v3 N = Normalized(Cross(v2 - v1, v0 - v1));

		for (int i = 0; i < 3; ++i)
		{
			Mesh->Vertices[t.Vertices[i]].Normal += N;
		}
	}
	for (int VertexIndex = 0; VertexIndex < Mesh->Vertices.size(); ++VertexIndex)
	{
		Normalize(&Mesh->Vertices[VertexIndex].Normal);
	}
}


mesh LoadOFF(const std::string& filename)
{
	mesh Result;
	Result.Faces = std::vector<triangle>();
	Result.Vertices = std::vector<vertex>();

	std::ifstream File;
	File.open(filename.c_str(), std::fstream::in);
	if (!File.is_open())
	{
		//TODO(hugo)
		return(Result);
	}

	std::string Line;
	std::getline(File, Line); // NOTE(hugo) : 'OFF' line

	std::string VerticesNumberString;
	File >> VerticesNumberString;
	int VerticesNumber = std::stoi(VerticesNumberString);

	std::string FacesNumberString;
	File >> FacesNumberString;
	int FacesNumber = std::stoi(FacesNumberString);

	std::string Value;
	File >> Value; // NOTE(hugo) : Number of edges (useless for us)
	for (int VertexIndex = 0; VertexIndex < VerticesNumber; ++VertexIndex)
	{
		vertex V;
		File >> Value;
		V.Position.x = std::stof(Value);
		File >> Value;
		V.Position.y = std::stof(Value);
		File >> Value;
		V.Position.z = std::stof(Value);

		V.Normal = V3(0.0f, 0.0f, 0.0f);

		Result.Vertices.push_back(V);
	}

	for (int FaceIndex = 0; FaceIndex < FacesNumber; ++FaceIndex)
	{
		File >> Value;
		int FaceDegree = std::stoi(Value);

		Assert(FaceDegree == 3); //NOTE(hugo): I only consider faces which are triangles
		triangle T;
		File >> Value;
		T.Vertices[0] = std::stoi(Value);
		File >> Value;
		T.Vertices[1] = std::stoi(Value);
		File >> Value;
		T.Vertices[2] = std::stoi(Value);
		Result.Faces.push_back(T);
	}

	ComputeNormal(&Result);
	glGenVertexArrays(1, &Result.VAO);

	glGenBuffers(1, &Result.VBO);

	glGenBuffers(1, &Result.EBO);

	glBindVertexArray(Result.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Result.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * Result.Vertices.size(), &Result.Vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Result.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint) * 3 * Result.Faces.size(), (GLuint*)Result.Faces.data(), GL_STATIC_DRAW);
	
	// NOTE(hugo) : Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)0);

	// NOTE(hugo) : Vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (GLvoid*)offsetof(vertex, Normal));

	glBindVertexArray(0);

	return(Result);
}

void DrawTrianglesMesh(mesh* Mesh)
{
	glBindVertexArray(Mesh->VAO);
	glDrawElements(GL_TRIANGLES, (GLsizei)(3 * Mesh->Faces.size()), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

// NOTE(hugo) : Lot of pressure on that function this is called three time for each face
int GetPositionOfVertexInMesh(vertex V, mesh* Result, std::unordered_map<vertex, int, hash_vertex>& VertexIndices)
{
    // TODO(hugo) : Could this be improved using a hashmap of the index of the vertex in the OBJ positioning ?
    std::unordered_map<vertex, int, hash_vertex>::const_iterator it = VertexIndices.find(V);
    if(it == VertexIndices.end())
    {
        Result->Vertices.push_back(V);
        VertexIndices[V] = (u32)(Result->Vertices.size() - 1);
        return((u32)(Result->Vertices.size() - 1));
    }
    else
    {
        return(it->second);
    }
}

struct parse_obj_triangle_result
{
    bool PositionOnly;
    bool UsingTextures;
    int VertexIndex;
    int NormalIndex;
    int TextureIndex;
};

parse_obj_triangle_result ParseOBJTriangle(const std::string TriangleString)
{
    parse_obj_triangle_result Result = {};
    int VertexPosition = (u32)(TriangleString.find('/'));
    Result.PositionOnly = (VertexPosition == -1);
    std::string VertexIndexString = TriangleString.substr(0, VertexPosition);

    Result.VertexIndex = std::stoi(VertexIndexString) - OBJ_VERTEX_OFFSET;
    // TODO(hugo) : Maybe not optimal, I should consider using several types for classifying the mesh I'm currently parsing
    if(!Result.PositionOnly)
    {
        int TexturePosition = (u32)(TriangleString.find('/', VertexPosition + 1));
        std::string TextureIndexString = TriangleString.substr(VertexPosition + 1, TexturePosition - VertexPosition - 1);
        std::string NormalIndexString = TriangleString.substr(TexturePosition + 1);

        if(TextureIndexString != "")
        {
            Result.TextureIndex = std::stoi(TextureIndexString) - OBJ_VERTEX_OFFSET;
            Result.UsingTextures = true;
        }
        else
        {
            Result.UsingTextures = false;
        }
        Result.NormalIndex = std::stoi(NormalIndexString) - OBJ_VERTEX_OFFSET;
    }
    return(Result);
}

vertex CreateVertexFromTriangleString(std::string TriangleString, 
        std::vector<v3>& Positions, 
        std::vector<v3>& Normals, 
        std::vector<v2>& Textures)
{
    parse_obj_triangle_result ParsedTriangle = ParseOBJTriangle(TriangleString);
    vertex V = {};

    V.Position= Positions[ParsedTriangle.VertexIndex];
    if(!ParsedTriangle.PositionOnly)
    {
        if(ParsedTriangle.UsingTextures)
        {
            V.Texture = Textures[ParsedTriangle.TextureIndex];
        }
        V.Normal = Normals[ParsedTriangle.NormalIndex];
    }
    return(V);
}

mesh LoadOBJ(const std::string filename)
{
    // NOTE(hugo) : This OBJ parser was implemented with the use of this spec :
    // www.martinreddy.net/gfx/3d/obj.spec
    mesh Result = {};
    std::vector<v3> Positions;
    std::vector<v3> Normals;
    std::vector<v2> Textures;
    std::ifstream File;
    std::unordered_map<vertex, int, hash_vertex> VertexIndices;
#if defined GLIEWER_DEBUG
    int DEBUGLineIndex = 0;
#endif
    File.open(filename.c_str(), std::fstream::in);
    if(!File.is_open())
    {
        // TODO(hugo)
        return(Result);
    }

    while(!File.eof())
    {
        // NOTE(hugo) : One loop must process one entire line
        std::string Word;
        File >> Word;
        if(Word == "#")
        {
            // NOTE(hugo) : Comment. Ignore the rest of the line
            std::getline(File, Word);
            Word.clear();
        }
        else if(Word == "v")
        {
            float X, Y, Z, W;
            W = 1.0f;
            File >> X;
            File >> Y;
            File >> Z;
            char NextChar;
            File.get(NextChar);
            if(NextChar == '\n')
            {
                // NOTE(hugo) : No W was set
            }
            else
            {
                File >> W;
                Assert(W != 0.0f);
            }
            v3 Vertex = V3(X / W, Y / W, Z / W);
            Positions.push_back(Vertex);
        }
        else if(Word == "vt")
        {
            // NOTE(hugo) : We support only 2D textures for now
            float U, V;
            File >> U;
            File >> V;
            v2 VertexTexture = V2(U, V);
            Textures.push_back(VertexTexture);
        }
        else if(Word == "vn")
        {
            float X, Y, Z;
            File >> X;
            File >> Y;
            File >> Z;
            v3 VertexNormal = V3(X, Y, Z);
            Normals.push_back(VertexNormal);
        }
        else if(Word == "f")
        {
#if !GLIEWER_TRIANGLE_SUPPORT
            int LastVertexIndices[2] = {};
#endif
            triangle Triangle;
            std::string TriangleString;
            for(int Index = 0; Index < 3; ++Index)
            {
                File >> TriangleString; // NOTE(hugo) : Triangle string is of the form V/VT/VN or V//VN or V
                vertex V = CreateVertexFromTriangleString(TriangleString, Positions, Normals, Textures);

                int VertexIndexInMesh = GetPositionOfVertexInMesh(V, &Result, VertexIndices);
                Triangle.Vertices[Index] = VertexIndexInMesh;
            }

#if !GLIEWER_TRIANGLE_SUPPORT
            LastVertexIndices[0] = Triangle.Vertices[1];
            LastVertexIndices[1] = Triangle.Vertices[2];
#endif
            Result.Faces.push_back(Triangle);

#if !GLIEWER_TRIANGLE_SUPPORT
            LastVertexIndices[0] = Triangle.Vertices[1];
            // NOTE(hugo) : Support for polygon with more than 3 vertices
            char NextChar;
            File.get(NextChar);

            while(NextChar != '\n')
            {
                // NOTE(hugo) : Go to the next place that isn't a space
                while(NextChar == ' ')
                {
                    File.get(NextChar);
                }
                if(NextChar == '\n')
                {
                    break;
                }

                File.unget();
                File >> TriangleString; 
                vertex V = CreateVertexFromTriangleString(TriangleString, Positions, Normals, Textures);

                // NOTE(hugo) : I handle the polygon in a very special scheme since I could not find documentation about this in the specs.
                // I assume that the vertex v0, v1, v2 forms the first triangle. Then its v1, v2, v3. etc...
                // But it could (for example) be : v0, v1, v2 then v0, v2, v3, etc...
                int VertexIndexInMesh = GetPositionOfVertexInMesh(V, &Result, VertexIndices);
                Triangle.Vertices[0] = LastVertexIndices[0];
                Triangle.Vertices[1] = LastVertexIndices[1];
                Triangle.Vertices[2] = VertexIndexInMesh;

                LastVertexIndices[0] = LastVertexIndices[1];
                LastVertexIndices[1] = VertexIndexInMesh;

                File.get(NextChar);
            }
#else
            // NOTE(hugo) : Ignore the rest of the line
            std::getline(File, Word);
#endif
        }
        /*
         * TODO(hugo) : ALL THESE ! This include parsing and rendering according to MTL and TGA files !!
        else if(Word == "g")
        {
        }
        else if(Word == "s")
        {
        }
        else if(Word == "o")
        {
        }
        else if(Word == "mtllib")
        {
        }
        else if(Word == "usemtl")
        {
        }
        */
        else
        {
            // NOTE(hugo) : Ignore the rest of the line
            std::getline(File, Word);
        }
#if defined GLIEWER_DEBUG
        DEBUGLineIndex++;
#endif
    }

    // TODO(hugo) : We must compute normals only if : (a) users requires a normalized mesh, (b) OBJ file does not give us normals
    ComputeNormal(&Result);

    // TODO(hugo) : Here we assume that the normals were given ! Do not do this in the future
	glGenVertexArrays(1, &Result.VAO);

	glGenBuffers(1, &Result.VBO);

	glGenBuffers(1, &Result.EBO);

	glBindVertexArray(Result.VAO);
	glBindBuffer(GL_ARRAY_BUFFER, Result.VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * Result.Vertices.size(), (GLuint*)Result.Vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Result.EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle) * Result.Faces.size(), (GLuint*)Result.Faces.data(), GL_STATIC_DRAW);
	
	// NOTE(hugo) : Vertex positions
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)0);

	// NOTE(hugo) : Vertex normals
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(3 * sizeof(float)));

	// NOTE(hugo) : Vertex textures
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (GLvoid*)(6 * sizeof(float)));

	glBindVertexArray(0);

	Result.ModelMatrix = Identity4();

    return(Result);
}

