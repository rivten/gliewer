#pragma once

#include "vector.h"
#include "matrix.h"
#include "functions.h"
#include "rectangle.h"

// NOTE(hugo) : Equation of type ax + by + c = 0
struct straight_line2
{
    float a;
    float b;
    float c;
};

straight_line2 FromNormalAndPoint(v2 Normal, v2 Point)
{
    straight_line2 Result = {};
    Result.a = Normal.x;
    Result.b = Normal.y;
    Result.c = - Dot(Normal, Point);

    return(Result);
}

bool Intersect(straight_line2 U, straight_line2 V)
{
    mat2 IntersectionMatrix = {};
    IntersectionMatrix.a = U.a;
    IntersectionMatrix.b = U.b;
    IntersectionMatrix.c = V.a;
    IntersectionMatrix.d = V.b;

    bool Result = Invertible(IntersectionMatrix);
    return(Result);
}

v2 IntersectionPoint(straight_line2 U, straight_line2 V)
{
    Assert(Intersect(U, V));

    mat2 IntersectionMatrix = {};
    IntersectionMatrix.a = U.a;
    IntersectionMatrix.b = U.b;
    IntersectionMatrix.c = V.a;
    IntersectionMatrix.d = V.b;

    v2 ToSolve = {- U.c, -V.c};
    v2 Result = Solve(IntersectionMatrix, ToSolve);

    return(Result);
}

float DistPointToLine(v2 Point, straight_line2 Line)
{
    float Result = Abs(Line.a * Point.x + Line.b * Point.y + Line.c) / SquareRoot(Line.a * Line.a + Line.b * Line.b);

    return(Result);
}

straight_line2 FromTwoPoints(v2 A, v2 B)
{
    Assert(A.x != B.x || A.y != B.y);

    straight_line2 Result;
    v2 Normal = Normalized(Perp(B - A));
    Result = FromNormalAndPoint(Normal, B);

    return(Result);
}

// NOTE(hugo) : Is floating operation correct ?
bool IsInLine(v2 P, straight_line2 Line)
{
    bool Result = (Line.a * P.x + Line.b * P.y + Line.c == 0.0f);

    return(Result);
}

bool IsInSegment(v2 P, v2 SegmentBegin, v2 SegmentEnd)
{
    bool Result = false;
    straight_line2 Line = FromTwoPoints(SegmentBegin, SegmentEnd);
    if(IsInLine(P, Line))
    {
        Result = (Dot(P - SegmentBegin, SegmentEnd - SegmentBegin) >= 0) && 
            (Dot(P - SegmentEnd, SegmentBegin - SegmentEnd) >= 0);

    }

    return(Result);
}
