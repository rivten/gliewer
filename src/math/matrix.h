#pragma once

/* ------------------------------
 *           mat2 
 * ------------------------------ */

// NOTE(hugo) : mat 2
//  ( a   b )
//  ( c   d )
struct mat2
{
    union
    {
        struct
        {
            float a;
            float b;
            float c;
            float d;
        };
        struct
        {
            float m00;
            float m01;
            float m10;
            float m11;
        };
        float Data_[4];
    };

    void operator*=(float Lambda);
};

mat2 operator*(float Lambda, mat2 M)
{
    mat2 Result = {};
    for(u32 Index = 0;
            Index < ArrayCount(M.Data_);
            ++Index)
    {
        Result.Data_[Index] = Lambda * M.Data_[Index];
    }

    return(Result);
}

void mat2::operator*=(float Lambda)
{
    *this = Lambda * (*this);
}

v2 operator*(mat2 M, v2 U)
{
    v2 Result = {};
    Result.x = M.a * U.x + M.b * U.y;
    Result.y = M.c * U.x + M.d * U.y;

    return(Result);
}

float Det(mat2 M)
{
    float Result = M.a * M.d - M.b * M.c;

    return(Result);
}

bool Invertible(mat2 M)
{
    bool Result = (Det(M) != 0.0f);

    return(Result);
}

mat2 Inv(mat2 M)
{
    Assert(Invertible(M));
    float InvDet = 1.0f / Det(M);

    mat2 Result;
    Result.a = M.d;
    Result.b = -M.b;
    Result.c = -M.c;
    Result.d = M.a;
    Result *= InvDet;

    return(Result);
}

// NOTE(hugo) : Solves X for AX = B
v2 Solve(mat2 A, v2 B)
{
    Assert(Invertible(A));
    v2 Result = Inv(A) * B;

    return(Result);
}

/* ------------------------------
 *           mat4 
 * ------------------------------ */

struct mat4
{
    float Data_[16];

    void operator*=(float Lambda);
    void operator/=(float Lambda);
};

// TODO(hugo) : FASTER OPERATIONS !!
// Two main optims :
//    - CPU caches (in particular for SetValue)
//    - better matrix multiplication algorithm

inline float GetValue(mat4 M, int i, int j)
{
    Assert(i >= 0);
    Assert(j >= 0);
    Assert(i < 4);
    Assert(j < 4);
    return(M.Data_[4 * j + i]);
}

mat4 operator*(float Lambda, mat4 A)
{
	mat4 Result = {};
	for(u32 i = 0; i < 16; ++i)
	{
		Result.Data_[i] = Lambda * A.Data_[i];
	}
    return(Result);
}

mat4 operator/(mat4 A, float Lambda)
{
    Assert(Lambda != 0.0f);
    return((1.0f / Lambda) * A);
}

void mat4::operator*=(float Lambda)
{
    *this = Lambda * (*this);
}

void mat4::operator/=(float Lambda)
{
    *this = (1.0f / Lambda) * (*this);
}

inline void SetValue(mat4* M, int i, int j, float Value)
{
    Assert(i >= 0);
    Assert(j >= 0);
    Assert(i < 4);
    Assert(j < 4);
    M->Data_[4 * j + i] = Value;
}

// TODO(hugo) : optims
v4 operator*(mat4 M, v4 A)
{
    v4 Result;
    for(int i = 0; i < 4; ++i)
    {
        Result.E[i] = 0;
        for(int j = 0; j < 4; ++j)
        {
            Result.E[i] += A.E[j] * GetValue(M, i, j);
        }
    }
    return(Result);
}

// TODO(hugo) : optims
mat4 operator*(mat4 A, mat4 B)
{
    // NOTE(hugo) : This is naive
    mat4 Result = {};
    for(int i = 0; i < 4; ++i)
    {
        for(int j = 0; j < 4; ++j)
        {
            SetValue(&Result, i, j, 0.0f);
            for(int t = 0; t < 4; ++t)
            {
                SetValue(&Result, i, j, GetValue(Result, i, j) + GetValue(A, i, t) * GetValue(B, t, j));
            }
        }
    }
    return(Result);
}

mat4 Identity4(void)
{
    mat4 Result = {};
    SetValue(&Result, 0, 0, 1.0f);
    SetValue(&Result, 0, 1, 0.0f);
    SetValue(&Result, 0, 2, 0.0f);
    SetValue(&Result, 0, 3, 0.0f);

    SetValue(&Result, 1, 0, 0.0f);
    SetValue(&Result, 1, 1, 1.0f);
    SetValue(&Result, 1, 2, 0.0f);
    SetValue(&Result, 1, 3, 0.0f);

    SetValue(&Result, 2, 0, 0.0f);
    SetValue(&Result, 2, 1, 0.0f);
    SetValue(&Result, 2, 2, 1.0f);
    SetValue(&Result, 2, 3, 0.0f);

    SetValue(&Result, 3, 0, 0.0f);
    SetValue(&Result, 3, 1, 0.0f);
    SetValue(&Result, 3, 2, 0.0f);
    SetValue(&Result, 3, 3, 1.0f);

    return(Result);
}

mat4 Translation(v3 B)
{
    mat4 Result = {};
    SetValue(&Result, 0, 0, 1.0f);
    SetValue(&Result, 0, 1, 0.0f);
    SetValue(&Result, 0, 2, 0.0f);
    SetValue(&Result, 0, 3, B.x);

    SetValue(&Result, 1, 0, 0.0f);
    SetValue(&Result, 1, 1, 1.0f);
    SetValue(&Result, 1, 2, 0.0f);
    SetValue(&Result, 1, 3, B.y);

    SetValue(&Result, 2, 0, 0.0f);
    SetValue(&Result, 2, 1, 0.0f);
    SetValue(&Result, 2, 2, 1.0f);
    SetValue(&Result, 2, 3, B.z);

    SetValue(&Result, 3, 0, 0.0f);
    SetValue(&Result, 3, 1, 0.0f);
    SetValue(&Result, 3, 2, 0.0f);
    SetValue(&Result, 3, 3, 1.0f);

    return(Result);
}

mat4 Scaling(v3 Scale)
{
    mat4 Result = {};
    SetValue(&Result, 0, 0, Scale.x);
    SetValue(&Result, 0, 1, 0.0f);
    SetValue(&Result, 0, 2, 0.0f);
    SetValue(&Result, 0, 3, 0.0f);

    SetValue(&Result, 1, 0, 0.0f);
    SetValue(&Result, 1, 1, Scale.y);
    SetValue(&Result, 1, 2, 0.0f);
    SetValue(&Result, 1, 3, 0.0f);

    SetValue(&Result, 2, 0, 0.0f);
    SetValue(&Result, 2, 1, 0.0f);
    SetValue(&Result, 2, 2, Scale.z);
    SetValue(&Result, 2, 3, 0.0f);

    SetValue(&Result, 3, 0, 0.0f);
    SetValue(&Result, 3, 1, 0.0f);
    SetValue(&Result, 3, 2, 0.0f);
    SetValue(&Result, 3, 3, 1.0f);

    return(Result);
}

mat4 Rotation(float Angle, v3 Axis)
{
    mat4 Result = {};
    Normalize(&Axis);

    float CosAngle = cos(Angle);
    float SineAngle = sin(Angle);
    float OneMinusCos = 1.0f - cos(Angle);
    // NOTE(hugo) : This Rotation matrix expression was obtained via Wikipedia. So, blame them there 
    // if this doesn't work
    SetValue(&Result, 0, 0, CosAngle + Axis.x * Axis.x * OneMinusCos);
    SetValue(&Result, 0, 1, Axis.x * Axis.y * OneMinusCos - Axis.z * SineAngle);
    SetValue(&Result, 0, 2, Axis.x * Axis.z * OneMinusCos + Axis.y * SineAngle);
    SetValue(&Result, 0, 3, 0.0f);

    SetValue(&Result, 1, 0, Axis.x * Axis.y * OneMinusCos + Axis.z * SineAngle);
    SetValue(&Result, 1, 1, CosAngle + Axis.y * Axis.y * OneMinusCos);
    SetValue(&Result, 1, 2, Axis.y * Axis.z * OneMinusCos - Axis.x * SineAngle);
    SetValue(&Result, 1, 3, 0.0f);

    SetValue(&Result, 2, 0, Axis.x * Axis.z * OneMinusCos - Axis.y * SineAngle);
    SetValue(&Result, 2, 1, Axis.y * Axis.z * OneMinusCos + Axis.x * SineAngle);
    SetValue(&Result, 2, 2, CosAngle + Axis.z * Axis.z * OneMinusCos);
    SetValue(&Result, 2, 3, 0.0f);

    SetValue(&Result, 3, 0, 0.0f);
    SetValue(&Result, 3, 1, 0.0f);
    SetValue(&Result, 3, 2, 0.0f);
    SetValue(&Result, 3, 3, 1.0f);

    return(Result);
}

// NOTE(hugo) : FoV is assumed to be given in radians
mat4 Perspective(float FoV, float Aspect, float NearPlane, float FarPlane)
{
	mat4 Result = {};

	Assert(FoV > 0 && Aspect != 0);

	float FrustrumDepth = FarPlane - NearPlane;
	float OneOverDepth = 1.0 / FrustrumDepth;

	// NOTE(hugo) : Here we assert that Aspect = Width / Height
	SetValue(&Result, 1, 1, 1.0f / Tan(0.5f * FoV));
	SetValue(&Result, 0, 0, GetValue(Result, 1, 1) / Aspect);
	SetValue(&Result, 2, 2, - (FarPlane + NearPlane) * OneOverDepth);
	SetValue(&Result, 2, 3, - 2 * FarPlane * NearPlane * OneOverDepth);
	SetValue(&Result, 3, 2, -1);
	 
	return(Result);
}

mat4 Orthographic(float Width, float Height, float NearPlane, float FarPlane)
{
	mat4 Result = {};

	Assert(Width != 0.0f && Height != 0.0f);

	float FrustrumDepth = FarPlane - NearPlane;
	float OneOverDepth = 1.0 / FrustrumDepth;

	// NOTE(hugo) : Here we assert that Aspect = Width / Height
	SetValue(&Result, 0, 0, 2 / Width);
	SetValue(&Result, 1, 1, 2 / Height);
	SetValue(&Result, 2, 2, - 2 * OneOverDepth);
	SetValue(&Result, 2, 3, - (FarPlane + NearPlane) * OneOverDepth);
	SetValue(&Result, 3, 3, 1);
	 
	return(Result);
}

mat4 LookAt(v3 Eye, v3 Target, v3 WorldUp)
{
	mat4 Result = {};

	v3 ZAxis = Normalized(Eye - Target);
	v3 XAxis = Normalized(Cross(WorldUp, ZAxis));
	v3 YAxis = Cross(ZAxis, XAxis);

	SetValue(&Result, 0, 0, XAxis.x);
	SetValue(&Result, 0, 1, XAxis.y);
	SetValue(&Result, 0, 2, XAxis.z);
	SetValue(&Result, 0, 3, - Dot(Eye, XAxis));

	SetValue(&Result, 1, 0, YAxis.x);
	SetValue(&Result, 1, 1, YAxis.y);
	SetValue(&Result, 1, 2, YAxis.z);
	SetValue(&Result, 1, 3, - Dot(Eye, YAxis));

	SetValue(&Result, 2, 0, ZAxis.x);
	SetValue(&Result, 2, 1, ZAxis.y);
	SetValue(&Result, 2, 2, ZAxis.z);
	SetValue(&Result, 2, 3, - Dot(Eye, ZAxis));

	SetValue(&Result, 3, 0, 0);
	SetValue(&Result, 3, 1, 0);
	SetValue(&Result, 3, 2, 0);
	SetValue(&Result, 3, 3, 1);

	return(Result);
}

mat4 Transpose(mat4 A)
{
	mat4 Result = {};

	for(u32 RowIndex = 0; RowIndex < 4; ++RowIndex)
	{
		for(u32 ColIndex = 0; ColIndex < 4; ++ColIndex)
		{
			SetValue(&Result, ColIndex, RowIndex, GetValue(A, RowIndex, ColIndex));
		}
	}
	 
	return(Result);
}

// TODO(hugo) : Check that this is not wrong !!
float Det(mat4 M)
{
	float Result = 0.0f;

	Result += GetValue(M, 0, 0) * GetValue(M, 1, 1) * GetValue(M, 2, 2) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 0) * GetValue(M, 1, 2) * GetValue(M, 2, 3) * GetValue(M, 3, 1);
	Result += GetValue(M, 0, 0) * GetValue(M, 1, 3) * GetValue(M, 2, 1) * GetValue(M, 3, 2);

	Result += GetValue(M, 0, 1) * GetValue(M, 1, 0) * GetValue(M, 2, 3) * GetValue(M, 3, 2);
	Result += GetValue(M, 0, 1) * GetValue(M, 1, 2) * GetValue(M, 2, 0) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 1) * GetValue(M, 1, 3) * GetValue(M, 2, 2) * GetValue(M, 3, 0);

	Result += GetValue(M, 0, 2) * GetValue(M, 1, 0) * GetValue(M, 2, 1) * GetValue(M, 3, 3);
	Result += GetValue(M, 0, 2) * GetValue(M, 1, 1) * GetValue(M, 2, 3) * GetValue(M, 3, 0);
	Result += GetValue(M, 0, 2) * GetValue(M, 1, 3) * GetValue(M, 2, 0) * GetValue(M, 3, 1);

	Result += GetValue(M, 0, 3) * GetValue(M, 1, 0) * GetValue(M, 2, 2) * GetValue(M, 3, 1);
	Result += GetValue(M, 0, 3) * GetValue(M, 1, 1) * GetValue(M, 2, 0) * GetValue(M, 3, 2);
	Result += GetValue(M, 0, 3) * GetValue(M, 1, 2) * GetValue(M, 2, 1) * GetValue(M, 3, 0);

	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 1) * GetValue(M, 2, 3) * GetValue(M, 3, 2);
	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 2) * GetValue(M, 2, 1) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 0) * GetValue(M, 1, 3) * GetValue(M, 2, 2) * GetValue(M, 3, 1);
	
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 0) * GetValue(M, 2, 2) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 2) * GetValue(M, 2, 3) * GetValue(M, 3, 0);
	Result -= GetValue(M, 0, 1) * GetValue(M, 1, 3) * GetValue(M, 2, 0) * GetValue(M, 3, 2);

	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 0) * GetValue(M, 2, 3) * GetValue(M, 3, 1);
	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 1) * GetValue(M, 2, 0) * GetValue(M, 3, 3);
	Result -= GetValue(M, 0, 2) * GetValue(M, 1, 3) * GetValue(M, 2, 1) * GetValue(M, 3, 0);

	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 0) * GetValue(M, 2, 1) * GetValue(M, 3, 2);
	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 1) * GetValue(M, 2, 2) * GetValue(M, 3, 0);
	Result -= GetValue(M, 0, 3) * GetValue(M, 1, 2) * GetValue(M, 2, 0) * GetValue(M, 3, 1);

	return(Result);
}

// TODO(hugo) : Check that this is not wrong !!
mat4 Inverse(mat4 A)
{
	mat4 Result = {};

	float Determinant = Det(A);
	Assert(Determinant != 0.0f);
	float OneOverDeterminant = 1.0f / Determinant;

	float B00 =	
		GetValue(A, 1, 1) * GetValue(A, 2, 2) * GetValue(A, 3, 3) + 
		GetValue(A, 1, 2) * GetValue(A, 2, 3) * GetValue(A, 3, 1) +
		GetValue(A, 1, 3) * GetValue(A, 2, 1) * GetValue(A, 3, 2) -
		GetValue(A, 1, 1) * GetValue(A, 2, 3) * GetValue(A, 3, 2) -
		GetValue(A, 1, 2) * GetValue(A, 2, 1) * GetValue(A, 3, 3) -
		GetValue(A, 1, 3) * GetValue(A, 2, 2) * GetValue(A, 3, 1);

	float B01 =	
		GetValue(A, 0, 1) * GetValue(A, 2, 3) * GetValue(A, 3, 2) + 
		GetValue(A, 0, 2) * GetValue(A, 2, 1) * GetValue(A, 3, 3) +
		GetValue(A, 0, 3) * GetValue(A, 2, 2) * GetValue(A, 3, 1) -
		GetValue(A, 0, 1) * GetValue(A, 2, 2) * GetValue(A, 3, 3) -
		GetValue(A, 0, 2) * GetValue(A, 2, 3) * GetValue(A, 3, 1) -
		GetValue(A, 0, 3) * GetValue(A, 2, 1) * GetValue(A, 3, 2);

	float B02 =	
		GetValue(A, 0, 1) * GetValue(A, 1, 2) * GetValue(A, 3, 3) + 
		GetValue(A, 0, 2) * GetValue(A, 1, 3) * GetValue(A, 3, 1) +
		GetValue(A, 0, 3) * GetValue(A, 1, 1) * GetValue(A, 3, 2) -
		GetValue(A, 0, 1) * GetValue(A, 1, 3) * GetValue(A, 3, 2) -
		GetValue(A, 0, 2) * GetValue(A, 1, 1) * GetValue(A, 3, 3) -
		GetValue(A, 0, 3) * GetValue(A, 1, 2) * GetValue(A, 3, 1);

	float B03 =	
		GetValue(A, 0, 1) * GetValue(A, 1, 3) * GetValue(A, 2, 2) + 
		GetValue(A, 0, 2) * GetValue(A, 1, 1) * GetValue(A, 2, 3) +
		GetValue(A, 0, 3) * GetValue(A, 1, 2) * GetValue(A, 2, 1) -
		GetValue(A, 0, 1) * GetValue(A, 1, 2) * GetValue(A, 2, 3) -
		GetValue(A, 0, 2) * GetValue(A, 1, 3) * GetValue(A, 2, 1) -
		GetValue(A, 0, 3) * GetValue(A, 1, 1) * GetValue(A, 2, 2);

	float B10 =	
		GetValue(A, 1, 0) * GetValue(A, 2, 3) * GetValue(A, 3, 2) + 
		GetValue(A, 1, 2) * GetValue(A, 2, 0) * GetValue(A, 3, 3) +
		GetValue(A, 1, 3) * GetValue(A, 2, 2) * GetValue(A, 3, 0) -
		GetValue(A, 1, 0) * GetValue(A, 2, 2) * GetValue(A, 3, 3) -
		GetValue(A, 1, 2) * GetValue(A, 2, 3) * GetValue(A, 3, 0) -
		GetValue(A, 1, 3) * GetValue(A, 2, 0) * GetValue(A, 3, 2);

	float B11 =	
		GetValue(A, 0, 0) * GetValue(A, 2, 2) * GetValue(A, 3, 3) + 
		GetValue(A, 0, 2) * GetValue(A, 2, 3) * GetValue(A, 3, 0) +
		GetValue(A, 0, 3) * GetValue(A, 2, 0) * GetValue(A, 3, 2) -
		GetValue(A, 0, 0) * GetValue(A, 2, 3) * GetValue(A, 3, 2) -
		GetValue(A, 0, 2) * GetValue(A, 2, 0) * GetValue(A, 3, 3) -
		GetValue(A, 0, 3) * GetValue(A, 2, 2) * GetValue(A, 3, 0);

	float B12 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 3) * GetValue(A, 3, 2) + 
		GetValue(A, 0, 2) * GetValue(A, 1, 0) * GetValue(A, 3, 3) +
		GetValue(A, 0, 3) * GetValue(A, 1, 2) * GetValue(A, 3, 0) -
		GetValue(A, 0, 0) * GetValue(A, 1, 2) * GetValue(A, 3, 3) -
		GetValue(A, 0, 2) * GetValue(A, 1, 3) * GetValue(A, 3, 0) -
		GetValue(A, 0, 3) * GetValue(A, 1, 0) * GetValue(A, 3, 2);

	float B13 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 2) * GetValue(A, 2, 3) + 
		GetValue(A, 0, 2) * GetValue(A, 1, 3) * GetValue(A, 2, 0) +
		GetValue(A, 0, 3) * GetValue(A, 1, 0) * GetValue(A, 2, 2) -
		GetValue(A, 0, 0) * GetValue(A, 1, 3) * GetValue(A, 2, 2) -
		GetValue(A, 0, 2) * GetValue(A, 1, 0) * GetValue(A, 2, 3) -
		GetValue(A, 0, 3) * GetValue(A, 1, 2) * GetValue(A, 2, 0);

	float B20 =	
		GetValue(A, 1, 0) * GetValue(A, 2, 1) * GetValue(A, 3, 3) + 
		GetValue(A, 1, 1) * GetValue(A, 2, 3) * GetValue(A, 3, 0) +
		GetValue(A, 1, 3) * GetValue(A, 2, 0) * GetValue(A, 3, 1) -
		GetValue(A, 1, 0) * GetValue(A, 2, 3) * GetValue(A, 3, 1) -
		GetValue(A, 1, 1) * GetValue(A, 2, 0) * GetValue(A, 3, 3) -
		GetValue(A, 1, 3) * GetValue(A, 2, 1) * GetValue(A, 3, 0);

	float B21 =	
		GetValue(A, 0, 0) * GetValue(A, 2, 3) * GetValue(A, 3, 1) + 
		GetValue(A, 0, 1) * GetValue(A, 2, 0) * GetValue(A, 3, 3) +
		GetValue(A, 0, 3) * GetValue(A, 2, 1) * GetValue(A, 3, 0) -
		GetValue(A, 0, 0) * GetValue(A, 2, 1) * GetValue(A, 3, 3) -
		GetValue(A, 0, 1) * GetValue(A, 2, 3) * GetValue(A, 3, 0) -
		GetValue(A, 0, 3) * GetValue(A, 2, 0) * GetValue(A, 3, 1);

	float B22 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 1) * GetValue(A, 3, 3) + 
		GetValue(A, 0, 1) * GetValue(A, 1, 3) * GetValue(A, 3, 0) +
		GetValue(A, 0, 3) * GetValue(A, 1, 0) * GetValue(A, 3, 1) -
		GetValue(A, 0, 0) * GetValue(A, 1, 3) * GetValue(A, 3, 1) -
		GetValue(A, 0, 1) * GetValue(A, 1, 0) * GetValue(A, 3, 3) -
		GetValue(A, 0, 3) * GetValue(A, 1, 1) * GetValue(A, 3, 0);

	float B23 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 3) * GetValue(A, 2, 1) + 
		GetValue(A, 0, 1) * GetValue(A, 1, 0) * GetValue(A, 2, 3) +
		GetValue(A, 0, 3) * GetValue(A, 1, 1) * GetValue(A, 2, 0) -
		GetValue(A, 0, 0) * GetValue(A, 1, 1) * GetValue(A, 2, 3) -
		GetValue(A, 0, 1) * GetValue(A, 1, 3) * GetValue(A, 2, 0) -
		GetValue(A, 0, 3) * GetValue(A, 1, 0) * GetValue(A, 2, 1);

	float B30 =	
		GetValue(A, 1, 0) * GetValue(A, 2, 2) * GetValue(A, 3, 1) + 
		GetValue(A, 1, 1) * GetValue(A, 2, 0) * GetValue(A, 3, 2) +
		GetValue(A, 1, 2) * GetValue(A, 2, 1) * GetValue(A, 3, 0) -
		GetValue(A, 1, 0) * GetValue(A, 2, 1) * GetValue(A, 3, 2) -
		GetValue(A, 1, 1) * GetValue(A, 2, 2) * GetValue(A, 3, 0) -
		GetValue(A, 1, 2) * GetValue(A, 2, 0) * GetValue(A, 3, 1);

	float B31 =	
		GetValue(A, 0, 0) * GetValue(A, 2, 1) * GetValue(A, 3, 2) + 
		GetValue(A, 0, 1) * GetValue(A, 2, 2) * GetValue(A, 3, 0) +
		GetValue(A, 0, 2) * GetValue(A, 2, 0) * GetValue(A, 3, 1) -
		GetValue(A, 0, 0) * GetValue(A, 2, 2) * GetValue(A, 3, 1) -
		GetValue(A, 0, 1) * GetValue(A, 2, 0) * GetValue(A, 3, 2) -
		GetValue(A, 0, 2) * GetValue(A, 2, 1) * GetValue(A, 3, 0);

	float B32 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 2) * GetValue(A, 3, 1) + 
		GetValue(A, 0, 1) * GetValue(A, 1, 0) * GetValue(A, 3, 2) +
		GetValue(A, 0, 2) * GetValue(A, 1, 1) * GetValue(A, 3, 0) -
		GetValue(A, 0, 0) * GetValue(A, 1, 1) * GetValue(A, 3, 2) -
		GetValue(A, 0, 1) * GetValue(A, 1, 2) * GetValue(A, 3, 0) -
		GetValue(A, 0, 2) * GetValue(A, 1, 0) * GetValue(A, 3, 1);

	float B33 =	
		GetValue(A, 0, 0) * GetValue(A, 1, 1) * GetValue(A, 2, 2) + 
		GetValue(A, 0, 1) * GetValue(A, 1, 2) * GetValue(A, 2, 0) +
		GetValue(A, 0, 2) * GetValue(A, 1, 0) * GetValue(A, 2, 1) -
		GetValue(A, 0, 0) * GetValue(A, 1, 2) * GetValue(A, 2, 1) -
		GetValue(A, 0, 1) * GetValue(A, 1, 0) * GetValue(A, 2, 2) -
		GetValue(A, 0, 2) * GetValue(A, 1, 1) * GetValue(A, 2, 0);

	SetValue(&Result, 0, 0, B00);
	SetValue(&Result, 0, 1, B01);
	SetValue(&Result, 0, 2, B02);
	SetValue(&Result, 0, 3, B03);

	SetValue(&Result, 1, 0, B10);
	SetValue(&Result, 1, 1, B11);
	SetValue(&Result, 1, 2, B12);
	SetValue(&Result, 1, 3, B13);

	SetValue(&Result, 2, 0, B20);
	SetValue(&Result, 2, 1, B21);
	SetValue(&Result, 2, 2, B22);
	SetValue(&Result, 2, 3, B23);

	SetValue(&Result, 3, 0, B30);
	SetValue(&Result, 3, 1, B31);
	SetValue(&Result, 3, 2, B32);
	SetValue(&Result, 3, 3, B33);

	Result *= OneOverDeterminant;

	return(Result);
}
