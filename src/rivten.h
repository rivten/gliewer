#pragma once

/* NOTE(hugo)
 *    This file contains types, macros and functions I often use.
 */

#include <cstdint>

#ifdef __unix__
#include <sys/types.h>
#endif

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef bool b32;
typedef float r32;
typedef double r64;

typedef size_t memory_index;

#define ArrayCount(x) (sizeof((x))/(sizeof((x)[0])))
#define Assert(x) do{if(!(x)){*(int*)0=0;}}while(0)

#define global_variable static
// TODO(hugo) : Find out how to restablish this #define
// and then put the internal back on the functions I changed
//#define internal static
#define local_persist static

#define InvalidCodePath Assert(!"InvalidCodePath");
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
void ZeroSize(memory_index Size, void* Ptr)
{
	u8* Byte = (u8 *)Ptr;
	while(Size > 0)
	{
		*Byte = 0;
		++Byte;
		--Size;
	}
}

// TODO(hugo) : To get rid of this in order to use only arena framework ?
// {
#include <stdlib.h>
#define ReAllocateArray(Buffer, type, Size) (type *)ReAllocate_(Buffer, (Size) * sizeof(type))
#define AllocateArray(type, Size) (type *)Allocate_((Size) * sizeof(type))
#define AllocateStruct(type) AllocateArray(type, 1)
#define Free(Buffer) free(Buffer)
#define CopyArray(Dest, Source, type, Size) memcpy(Dest, Source, (Size) * sizeof(type))

void* Allocate_(size_t Size)
{
	void* Result = malloc(Size);
	Assert(Result);

	return(Result);
}

void* ReAllocate_(void* Buffer, size_t Size)
{
	void* Result = realloc(Buffer, Size);
	Assert(Result);

	return(Result);
}
// }

struct memory_arena
{
	memory_index Size;
	memory_index Used;
	u8* Base;

	u32 TemporaryCount;
};

struct temporary_memory
{
	memory_arena* Arena;
	memory_index Used;
};

void InitializeArena(memory_arena* Arena, memory_index Size, void* Base)
{
	Arena->Size = Size;
	Arena->Used = 0;
	Arena->TemporaryCount = 0;
	Arena->Base = (u8 *)Base;
}

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type))
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type))
#define PushSize(Arena, Size) PushSize_(Arena, Size)
void* PushSize_(memory_arena* Arena, memory_index Size)
{
	Assert(Arena->Used + Size <= Arena->Size);
	void* Result = Arena->Base + Arena->Used;
	Arena->Used += Size;

	return(Result);
}

temporary_memory BeginTemporaryMemory(memory_arena* Arena)
{
	temporary_memory Result = {};

	Result.Arena = Arena;
	Result.Used = 0;

	++Arena->TemporaryCount;

	return(Result);
}

void EndTemporaryMemory(temporary_memory TemporaryMemory)
{
	memory_arena* Arena = TemporaryMemory.Arena;
	Assert(Arena->Used >= TemporaryMemory.Used);
	Arena->Used -= TemporaryMemory.Used;
	Assert(Arena->TemporaryCount > 0);
	--Arena->TemporaryCount;
}
