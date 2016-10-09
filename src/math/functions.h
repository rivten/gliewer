#pragma once

#define PI 3.141592653f

float Square(float X)
{
    float Result = X * X;

    return(Result);
}

float Clamp(float X, float Min, float Max)
{
    if(X > Max)
    {
        return(Max);
    }
    else if(X < Min)
    {
        return(Min);
    }
    else
    {
        return(X);
    }
}

float Radians(float Degrees)
{
    return(PI * Degrees / 180.0f);
}

#include <cmath>
float SquareRoot(float X)
{
    float Result = sqrt(X);
    return(Result);
}

int Floor(float X)
{
    int Result;
    Result = (int)floor(X);
    return(Result);
}

float Abs(float X)
{
    float Result;
    Result = abs(X);
    return(Result);
}

float Cos(float X)
{
	float Result;
	Result = cos(X);

	return(Result);
}

float Sin(float X)
{
	float Result;
	Result = sin(X);

	return(Result);
}

float Tan(float X)
{
	float Result;
	Result = tan(X);

	return(Result);
}
