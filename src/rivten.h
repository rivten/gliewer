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
#define CopyArray(Source, Dest) for(int Index = 0; Index < ArrayCount((Source)); ++Index){(Dest)[Index] = (Source)[Index];}

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

