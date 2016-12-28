#pragma once

#define TINYOBJLOADER_IMPLEMENTATION

#include <vector>

// TODO(hugo) : I used a memory framework
// which is not the Arena one (from Casey Muratori).
// Maybe see if I can change the memory framework for
// the latter

/*
 * NOTE(hugo)
 * A mesh must absolutely have : position. Texture and normals are optionals.
 * A file has to give us : position and triangles. Texuture and normal are optionals.
 */
struct vertex
{
	v3 P;
	v3 Normal;
	v2 Texture;
};

struct triangle
{
	union
	{
		struct
		{
			u32 Vertex0Index;
			u32 Vertex1Index;
			u32 Vertex2Index;
		};
		u32 VertexIndices[3];
	};
};

struct mesh
{
	vertex* Vertices;
	u32 VertexCount;
	u32 VertexPoolSize;

	triangle* Triangles;
	u32 TriangleCount;
	u32 TrianglePoolSize;
};

struct rect3
{
	v3 Min;
	v3 Max;
};

struct object
{
	u32 VertexArrayID;
	u32 VertexBufferID;
	u32 ElementBufferID;

	v4 Albedo;
	char Name[100];

	bool Visible;

	mesh Mesh;
	rect3 BoundingBox;
};

bool IsRect3Valid(rect3 Rect)
{
	bool Result = ((Rect.Min.x <= Rect.Max.x) 
			&& (Rect.Min.y <= Rect.Max.y) 
			&& (Rect.Min.z <= Rect.Max.z));

	return(Result);
}

rect3 MaxBoundingBox(void)
{
	rect3 Result;
	Result.Min = {MAX_REAL, MAX_REAL, MAX_REAL};
	Result.Max = {MIN_REAL, MIN_REAL, MIN_REAL};
	return(Result);
}

rect3 BoundingBox(mesh* Mesh)
{
	rect3 Box = MaxBoundingBox();
	for(u32 VertexIndex = 0; VertexIndex < Mesh->VertexCount; ++VertexIndex)
	{
		v3 Pos = Mesh->Vertices[VertexIndex].P;
		if(Pos.x > Box.Max.x)
		{
			Box.Max.x = Pos.x;
		}
		if(Pos.x < Box.Min.x)
		{
			Box.Min.x = Pos.x;
		}

		if(Pos.y > Box.Max.y)
		{
			Box.Max.y = Pos.y;
		}
		if(Pos.y < Box.Min.y)
		{
			Box.Min.y = Pos.y;
		}

		if(Pos.z > Box.Max.z)
		{
			Box.Max.z = Pos.z;
		}
		if(Pos.z < Box.Min.z)
		{
			Box.Min.z = Pos.z;
		}
	}

	Assert(IsRect3Valid(Box));
	return(Box);
}

bool Intersect3(rect3 A, rect3 B)
{
	// TODO(hugo) : check this
	bool Result = !(
			(A.Max.x < B.Min.x) ||
			(A.Max.y < B.Min.y) ||
			(A.Max.z < B.Min.z) ||
			(A.Min.x > B.Max.x) ||
			(A.Min.y > B.Max.y) ||
			(A.Min.z > B.Max.z)
			);

	return(Result);
}

void PushVertex(mesh* Mesh, vertex V)
{
	if(Mesh->VertexCount == Mesh->VertexPoolSize)
	{
		Mesh->Vertices = ReAllocateArray(Mesh->Vertices, vertex, 2 * Mesh->VertexPoolSize);
		Mesh->VertexPoolSize *= 2;
	}

	Mesh->Vertices[Mesh->VertexCount] = V;
	++Mesh->VertexCount;
}

void PushTriangle(mesh* Mesh, triangle Triangle)
{
	if(Mesh->TriangleCount == Mesh->TrianglePoolSize)
	{
		Mesh->Triangles = ReAllocateArray(Mesh->Triangles, triangle, 2 * Mesh->TrianglePoolSize);
		Mesh->TrianglePoolSize *= 2;
	}

	Mesh->Triangles[Mesh->TriangleCount] = Triangle;
	++Mesh->TriangleCount;
}

void ComputeNormal(mesh* Mesh)
{
	Assert(Mesh != 0);
	for (u32 TriangleIndex = 0; TriangleIndex < Mesh->TriangleCount; ++TriangleIndex)
	{
		triangle t = Mesh->Triangles[TriangleIndex];
		v3 v0 = Mesh->Vertices[t.VertexIndices[0]].P;
		v3 v1 = Mesh->Vertices[t.VertexIndices[1]].P;
		v3 v2 = Mesh->Vertices[t.VertexIndices[2]].P;

		v3 CrossProduct = Cross(v2 - v1, v0 - v1);
		v3 N = V3(0.0f, 0.0f, 0.0f);
		// TODO(hugo) : In theory, we should never have
		// LengthSqr(CrossProduct) == 0.0f... or should we ?
		if(LengthSqr(CrossProduct) != 0.0f)
		{
			N = Normalized(CrossProduct);
		}

		for (u32 i = 0; i < 3; ++i)
		{
			Mesh->Vertices[t.VertexIndices[i]].Normal += N;
		}
	}
	for (u32 VertexIndex = 0; VertexIndex < Mesh->VertexCount; ++VertexIndex)
	{
		if(LengthSqr(Mesh->Vertices[VertexIndex].Normal) != 0.0f)
		{
			Normalize(&Mesh->Vertices[VertexIndex].Normal);
		}
	}
}

void DrawTriangleObject(opengl_state* State, object* Object)
{
	BindVertexArray(State, Object->VertexArrayID);
	glDrawElements(GL_TRIANGLES, (GLsizei)(3 * Object->Mesh.TriangleCount), GL_UNSIGNED_INT, 0);
	BindVertexArray(State, 0);
}

void DrawWiredTriangleObject(opengl_state* State, object* Object)
{
	BindVertexArray(State, Object->VertexArrayID);
	glDrawElements(GL_LINES, (GLsizei)(3 * Object->Mesh.TriangleCount), GL_UNSIGNED_INT, 0);
	BindVertexArray(State, 0);
}

void DrawTriangleObjectInstances(opengl_state* State, object* Object, u32 InstanceCount)
{
	BindVertexArray(State, Object->VertexArrayID);
	glDrawElementsInstanced(GL_TRIANGLES, (GLsizei)(3 * Object->Mesh.TriangleCount), GL_UNSIGNED_INT, 0, InstanceCount);
	BindVertexArray(State, 0);
}

void DrawWiredTriangleObjectInstances(opengl_state* State, object* Object, u32 InstanceCount)
{
	BindVertexArray(State, Object->VertexArrayID);
	glDrawElementsInstanced(GL_LINES, (GLsizei)(3 * Object->Mesh.TriangleCount), GL_UNSIGNED_INT, 0, InstanceCount);
	BindVertexArray(State, 0);
}

struct vertex_hash
{
	vertex V;
	u32 VertexIndex;

	vertex_hash* NextInHash;
};

// NOTE(hugo) : Lot of pressure on that function this is called three time for each face
u32 GetIndexOfVertexInMesh(vertex V, mesh* Mesh, vertex_hash** VertexHash, u32 HashBucketCount)
{
	// TODO(hugo) : Better hash functions !
	u32 Hash = *(u32*)(void*)(&V.P.x);
	u32 HashBucket = Hash & (HashBucketCount - 1);
	vertex_hash* Found = 0;

	for(vertex_hash* Vertex = VertexHash[HashBucket]; Vertex; Vertex = Vertex->NextInHash)
	{
		if((Vertex->V.P.x == V.P.x) 
				&& (Vertex->V.P.y == V.P.y) 
				&& (Vertex->V.P.z == V.P.z))
		{
			// TODO(hugo) : This part has not been tested because
			// I did not try big enough meshes
			Found = Vertex;
			break;
		}
	}

	if(!Found)
	{
		PushVertex(Mesh, V);
		Found = AllocateStruct(vertex_hash);
		*Found = {};
		Found->V = V;
		Found->VertexIndex = Mesh->VertexCount - 1;
		Found->NextInHash = VertexHash[HashBucket];

		VertexHash[HashBucket] = Found;
	}

	return(Found->VertexIndex);
}

// TODO(hugo) : Make this go through the open_gl state framework ?
void GenerateDataBuffer(object* Object)
{
	glGenVertexArrays(1, &Object->VertexArrayID);

	glGenBuffers(1, &Object->VertexBufferID);

	glGenBuffers(1, &Object->ElementBufferID);

	glBindVertexArray(Object->VertexArrayID);
	glBindBuffer(GL_ARRAY_BUFFER, Object->VertexBufferID);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * Object->Mesh.VertexCount, (GLuint*)Object->Mesh.Vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, Object->ElementBufferID);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(triangle) * Object->Mesh.TriangleCount, (GLuint*)Object->Mesh.Triangles, GL_STATIC_DRAW);
	
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
}

// ---------------------------------------
// NOTE(hugo) : TinyObjLoader mesh implementation
// ---------------------------------------

#include "tiny_obj_loader.h"

std::vector<object> LoadOBJ(const std::string BaseDir, const std::string Filename)
{
	std::vector<object> Result;
	tinyobj::attrib_t Attributes;
	std::vector<tinyobj::shape_t> Shapes;
	std::vector<tinyobj::material_t> Materials;
	std::string Error;
	bool LoadingWorked = tinyobj::LoadObj(&Attributes, &Shapes, &Materials, &Error, (BaseDir + Filename).c_str(), BaseDir.c_str());
	Assert(LoadingWorked);

	for(u32 ShapeIndex = 0; ShapeIndex < Shapes.size(); ++ShapeIndex)
	{
		object Object = {};
		Object.Mesh.VertexCount = 0;
		Object.Mesh.VertexPoolSize = 1;
		Object.Mesh.Vertices = AllocateArray(vertex, Object.Mesh.VertexPoolSize);
		Object.Mesh.TriangleCount = 0;
		Object.Mesh.TrianglePoolSize = 1;
		Object.Mesh.Triangles = AllocateArray(triangle, Object.Mesh.TrianglePoolSize);
		Object.Visible = true;
		CopyArray(Object.Name, Shapes[ShapeIndex].name.c_str(), char, StringLength((char*)(Shapes[ShapeIndex].name.c_str())) + 1);
		Object.Albedo = V4(1.0f, 1.0f, 1.0f, 1.0f);
		for(u32 MaterialIndex = 0; MaterialIndex < Materials.size(); ++MaterialIndex)
		{
			if(!IsEmptyString(Object.Name) && AreStringIdentical((char*)Materials[MaterialIndex].name.c_str(), Object.Name))
			{
				tinyobj::material_t MeshMaterial = Materials[MaterialIndex];
				Object.Albedo = V4(MeshMaterial.ambient[0], MeshMaterial.ambient[1], MeshMaterial.ambient[2], 1.0f);
				break;
			}
			else if(AreStringIdentical((char*)Materials[MaterialIndex].name.c_str(), Object.Name) && AreStringIdentical(Object.Name, "light"))
			{
				// TODO(hugo) : Handle lights as meshes
				tinyobj::material_t MeshMaterial = Materials[MaterialIndex];
				Object.Albedo = V4(MeshMaterial.emission[0], MeshMaterial.emission[1], MeshMaterial.emission[2], 1.0f);
				break;
			}
		}

		bool NormalsComputed = true;

		// NOTE(hugo) : Must be a power of two
		vertex_hash* VertexHash[8];
		ZeroStruct(VertexHash);
		for(u32 TriangleIndex = 0; TriangleIndex < (Shapes[ShapeIndex].mesh.indices.size() / 3); ++TriangleIndex)
		{
			triangle Triangle = {};
			for(u32 i = 0; i < 3; ++i)
			{
				vertex V = {};
				u32 VertexIndex = Shapes[ShapeIndex].mesh.indices[3 * TriangleIndex + i].vertex_index;
				if(VertexIndex != -1)
				{
					V.P = V3(Attributes.vertices[3 * VertexIndex + 0],
							Attributes.vertices[3 * VertexIndex + 1],
							Attributes.vertices[3 * VertexIndex + 2]);
				}

				u32 NormalIndex = Shapes[ShapeIndex].mesh.indices[3 * TriangleIndex + i].normal_index;
				if(NormalsComputed && (NormalIndex != -1))
				{
					V.Normal = V3(Attributes.normals[3 * NormalIndex + 0],
							Attributes.normals[3 * NormalIndex + 1],
							Attributes.normals[3 * NormalIndex + 2]);
				}
				else
				{
					NormalsComputed = false;
				}

				u32 TextureIndex = Shapes[ShapeIndex].mesh.indices[3 * TriangleIndex + i].texcoord_index;
				if(TextureIndex != -1)
				{
					V.Texture = V2(Attributes.texcoords[2 * TextureIndex + 0],
							Attributes.texcoords[2 * TextureIndex + 1]);
				}

				u32 VertexIndexInMesh = GetIndexOfVertexInMesh(V, &Object.Mesh, VertexHash, ArrayCount(VertexHash));
				Triangle.VertexIndices[i] = VertexIndexInMesh;
			}
			PushTriangle(&Object.Mesh, Triangle);
		}

		// TODO(hugo) : Freeing vertex hash. Am I sure there is not a memory leak ??
		for(u32 VertexHashIndex = 0; VertexHashIndex < ArrayCount(VertexHash); ++VertexHashIndex)
		{
			vertex_hash* Vertex = VertexHash[VertexHashIndex];
			while(Vertex)
			{
				vertex_hash* Next = Vertex->NextInHash;
				Free(Vertex);
				Vertex = Next;
			}
		}

		if(!NormalsComputed)
		{
			ComputeNormal(&Object.Mesh);
		}

		Object.BoundingBox = BoundingBox(&Object.Mesh);

		GenerateDataBuffer(&Object);
		Result.push_back(Object);
	}


	return(Result);
}

