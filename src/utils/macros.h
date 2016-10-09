#pragma once

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

