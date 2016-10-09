#pragma once

struct rectangle2
{
    v2 Min;
    v2 Max;
};

rectangle2 RectFromCenterDim(v2 Center, v2 Dim)
{
    rectangle2 Result = {};

    Result.Min = Center - 0.5 * Dim;
    Result.Max = Center + 0.5 * Dim;

    return(Result);
}

bool Included(rectangle2 A, rectangle2 B)
{
    bool Result;
    Result = A.Min.x >= B.Min.x &&
             A.Min.y >= B.Min.y &&
             A.Max.x <= B.Max.x &&
             A.Max.y <= B.Max.y;

    return(Result);
}

bool Intersect(rectangle2 A, rectangle2 B)
{
    bool Result;
    Result = !(A.Min.x >= B.Max.x ||
               A.Min.y >= B.Max.y ||
               A.Max.x <= B.Min.x ||
               A.Max.y <= B.Min.y);

    return(Result);
}
