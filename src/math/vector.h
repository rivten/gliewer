#pragma once

/* ------------------------------
 *           v2 
 * ------------------------------ */

struct v2
{
    union
    {
        struct
        {
            float x;
            float y;
        };
        struct
        {
            float s;
            float t;
        };
        struct
        {
            float u;
            float v;
        };
        float E[2];
    };

    // NOTE(hugo) : Operator overloading
    void operator+=(v2 A);
    void operator-=(v2 A);
    void operator*=(float Lambda);
    void operator/=(float Lambda);
    void operator-();
};

v2 V2(float x, float y)
{
    v2 Result;
    Result.x = x;
    Result.y = y;
    return(Result);
}

v2 operator+(v2 A, v2 B)
{
    v2 Result = V2(A.x + B.x, A.y + B.y);
    return(Result);
}

v2 operator-(v2 A, v2 B)
{
    v2 Result = V2(A.x - B.x, A.y - B.y);
    return(Result);
}

v2 operator*(float Lambda, v2 A)
{
    v2 Result = V2(Lambda * A.x, Lambda * A.y);
    return(Result);
}

v2 operator/(v2 A, float Lambda)
{
    Assert(Lambda != 0.0f);
    return((1.0f / Lambda) * A);
}

void v2::operator+=(v2 A)
{
    *this = *this + A;
}

void v2::operator-=(v2 A)
{
    *this = *this - A;
}

void v2::operator*=(float Lambda)
{
    *this = Lambda * (*this);
}

void v2::operator/=(float Lambda)
{
    *this = (1.0f / Lambda) * (*this);
}

void v2::operator-()
{
    *this *= -1.0f;
}

float Dot(v2 A, v2 B)
{
    return(A.x * B.x + A.y * B.y);
}

float LengthSqr(v2 A)
{
    return(Dot(A, A));
}

void Normalize(v2* A)
{
    *A = (*A) / sqrt(LengthSqr(*A));
}

v2 Normalized(v2 A)
{
    v2 Result;
    Result = A / sqrt(LengthSqr(A));
    return(Result);
}

v2 Lerp(v2 A, float t, v2 B)
{
    return((1.0f - t) * A + t * B);
}

v2 Hadamard(v2 A, v2 B)
{
    v2 Result;
    Result.x = A.x * B.x;
    Result.y = A.y * B.y;

    return(Result);
}

v2 Perp(v2 A)
{
    v2 Result;
    Result.x = - A.y;
    Result.y = A.x;

    return(Result);
}

/* ------------------------------
 *           v3 
 * ------------------------------ */

struct v3
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
        };
        struct
        {
            float r;
            float g;
            float b;
        };
        float E[3];
        struct
        {
            v2 xy;
            float Ignored_;
        };
    };

    // NOTE(hugo) : Operator overloading
    void operator+=(v3 A);
    void operator-=(v3 A);
    void operator*=(float Lambda);
    void operator/=(float Lambda);
};

v3 V3(float x, float y, float z)
{
    v3 Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    return(Result);
}

v3 V3(v2 V, float z)
{
    return(V3(V.x, V.y, z));
}

v3 operator+(v3 A, v3 B)
{
    v3 Result = V3(A.x + B.x, A.y + B.y, A.z + B.z);
    return(Result);
}

v3 operator-(v3 A, v3 B)
{
    v3 Result = V3(A.x - B.x, A.y - B.y, A.z - B.z);
    return(Result);
}

v3 operator*(float Lambda, v3 A)
{
    v3 Result = V3(Lambda * A.x, Lambda * A.y, Lambda * A.z);
    return(Result);
}

v3 operator/(v3 A, float Lambda)
{
    Assert(Lambda != 0.0f);
    v3 Result;
    Result = (1.0f / Lambda) * A;
    return(Result);
}

void v3::operator+=(v3 A)
{
    *this = *this + A;
}

void v3::operator-=(v3 A)
{
    *this = *this - A;
}

void v3::operator*=(float Lambda)
{
    *this = Lambda * (*this);
}

void v3::operator/=(float Lambda)
{
    *this = (1.0f / Lambda) * (*this);
}

float Dot(v3 A, v3 B)
{
    return(A.x * B.x + A.y * B.y + A.z * B.z);
}

float LengthSqr(v3 A)
{
    return(Dot(A, A));
}

void Normalize(v3* A)
{
    *A = (*A) / sqrt(LengthSqr(*A));
}

v3 Normalized(v3 A)
{
    v3 Result;
    Result = A / sqrt(LengthSqr(A));
    return(Result);
}

v3 Cross(v3 A, v3 B)
{
	v3 Result;
	Result.x = A.y * B.z - A.z * B.y;
	Result.y = A.z * B.x - A.x * B.z;
	Result.z = A.x * B.y - A.y * B.x;

	return(Result);
}

/* ------------------------------
 *           v4 
 * ------------------------------ */

struct v4
{
    union
    {
        struct
        {
            float x;
            float y;
            float z;
            float w;
        };
        struct
        {
            float r;
            float g;
            float b;
            float a;
        };
		struct
		{
			v3 xyz;
			float Ignored0_;
		};
		struct
		{
			v2 xy;
			v2 Ignored1_;
		};
        float E[4];
    };

    // NOTE(hugo) : Operator overloading
    void operator+=(v4 V);
    void operator-=(v4 V);
    void operator*=(float Lambda);
    void operator/=(float Lambda);
};

v4 V4(float x, float y, float z, float w)
{
    v4 Result;
    Result.x = x;
    Result.y = y;
    Result.z = z;
    Result.w = w;
    return(Result);
}

v4 ToV4(v3 V)
{
	v4 Result;

    Result.x = V.x;
    Result.y = V.y;
    Result.z = V.z;
    Result.w = 1.0f;
    return(Result);
}

v4 operator+(v4 A, v4 B)
{
    v4 Result = V4(A.x + B.x, A.y + B.y, A.z + B.z, A.w + B.w);
    return(Result);
}

v4 operator-(v4 A, v4 B)
{
    v4 Result = V4(A.x - B.x, A.y - B.y, A.z - B.z, A.w - B.w);
    return(Result);
}

v4 operator*(float Lambda, v4 A)
{
    v4 Result = V4(Lambda * A.x, Lambda * A.y, Lambda * A.z, Lambda * A.w);
    return(Result);
}

v4 operator/(v4 A, float Lambda)
{
    Assert(Lambda != 0.0f);
    v4 Result;
    Result = (1.0f / Lambda) * A;
    return(Result);
}

void v4::operator+=(v4 V)
{
    *this = *this + V;
}

void v4::operator-=(v4 V)
{
    *this = *this - V;
}

void v4::operator*=(float Lambda)
{
    *this = Lambda * (*this);
}

void v4::operator/=(float Lambda)
{
    Assert(Lambda != 0.0f);
    *this = (*this) / Lambda;
}

float Dot(v4 A, v4 B)
{
    return(A.x * B.x + A.y * B.y + A.z * B.z + A.w * B.w);
}

float LengthSqr(v4 A)
{
    return(Dot(A, A));
}

void Normalize(v4* A)
{
    *A = (*A) / sqrt(LengthSqr(*A));
}

v4 Normalized(v4 A)
{
    v4 Result;
    Result = A / sqrt(LengthSqr(A));
    return(Result);
}

