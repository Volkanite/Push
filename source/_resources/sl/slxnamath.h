typedef __m128 XMVECTOR;
typedef const XMVECTOR FXMVECTOR;


#define XMFINLINE __forceinline
#define XMINLINE __inline
#define _DECLSPEC_ALIGN_16_   __declspec(align(16))
//#define XMGLOBALCONST extern CONST __declspec(selectany)
#define XMASSERT(Expression) ((VOID)0)
typedef struct _XMCOLOR XMCOLOR;
XMFINLINE VOID XMStoreColor
(
    XMCOLOR* pDestination, 
    FXMVECTOR V
);
XMFINLINE XMVECTOR XMVectorSet
(
    FLOAT x, 
    FLOAT y, 
    FLOAT z, 
    FLOAT w
);
// 2D Vector; 32 bit floating point components
typedef struct _XMFLOAT2
{
    FLOAT x;
    FLOAT y;

	#ifdef __cplusplus

    _XMFLOAT2() {};
    _XMFLOAT2(FLOAT _x, FLOAT _y) : x(_x), y(_y) {};
    _XMFLOAT2(CONST FLOAT *pArray);

    _XMFLOAT2& operator= (CONST _XMFLOAT2& Float2)
		{
    x = Float2.x;
    y = Float2.y;
    return *this;
}

#endif // __cplusplus

} XMFLOAT2;


// 3D Vector; 32 bit floating point components
typedef struct _XMFLOAT3
{
    FLOAT x;
    FLOAT y;
    FLOAT z;

#ifdef __cplusplus

    _XMFLOAT3() {};
    _XMFLOAT3(FLOAT _x, FLOAT _y, FLOAT _z) : x(_x), y(_y), z(_z) {};
    _XMFLOAT3(CONST FLOAT *pArray);

    _XMFLOAT3& operator= (CONST _XMFLOAT3& Float3);

#endif // __cplusplus

} XMFLOAT3;


// ARGB Color; 8-8-8-8 bit unsigned normalized integer components packed into
// a 32 bit integer.  The normalized color is packed into 32 bits using 8 bit
// unsigned, normalized integers for the alpha, red, green, and blue components.
// The alpha component is stored in the most significant bits and the blue
// component in the least significant bits (A8R8G8B8):
// [32] aaaaaaaa rrrrrrrr gggggggg bbbbbbbb [0]
typedef struct _XMCOLOR
{
    union
    {
        struct
        {
            UINT b    : 8;  // Blue:    0/255 to 255/255
            UINT g    : 8;  // Green:   0/255 to 255/255
            UINT r    : 8;  // Red:     0/255 to 255/255
            UINT a    : 8;  // Alpha:   0/255 to 255/255
        };
        UINT c;
    };

} XMCOLOR;


// Matrix type: Sixteen 32 bit floating point components aligned on a
// 16 byte boundary and mapped to four hardware vector registers
typedef _DECLSPEC_ALIGN_16_ struct _XMMATRIX
{
    union
    {
        XMVECTOR r[4];
        struct
        {
            FLOAT _11, _12, _13, _14;
            FLOAT _21, _22, _23, _24;
            FLOAT _31, _32, _33, _34;
            FLOAT _41, _42, _43, _44;
        };
        FLOAT m[4][4];
    };

} XMMATRIX;
//typedef const XMMATRIX& CXMMATRIX;

typedef _DECLSPEC_ALIGN_16_ struct XMVECTORU32 {
    union {
        UINT u[4];
        XMVECTOR v;
    };
	#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORU32;

// Conversion types for constants
typedef _DECLSPEC_ALIGN_16_ struct XMVECTORF32 {
    union {
        float f[4];
        XMVECTOR v;
    };
	#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORF32;

typedef _DECLSPEC_ALIGN_16_ struct XMVECTORI32 {
    union {
        INT i[4];
        XMVECTOR v;
    };
#if defined(__cplusplus)
    inline operator XMVECTOR() const { return v; }
#if !defined(_XM_NO_INTRINSICS_) && defined(_XM_SSE_INTRINSICS_)
    inline operator __m128i() const { return reinterpret_cast<const __m128i *>(&v)[0]; }
    inline operator __m128d() const { return reinterpret_cast<const __m128d *>(&v)[0]; }
#endif
#endif // __cplusplus
} XMVECTORI32;


/****************************************************************************
 *
 * Globals
 *
 ****************************************************************************/

// The purpose of the following global constants is to prevent redundant 
// reloading of the constants when they are referenced by more than one
// separate inline math routine called within the same function.  Declaring
// a constant locally within a routine is sufficient to prevent redundant
// reloads of that constant when that single routine is called multiple
// times in a function, but if the constant is used (and declared) in a 
// separate math routine it would be reloaded.

extern CONST __declspec(selectany) XMVECTORF32 g_XMIdentityR2        = {0.0f, 0.0f, 1.0f, 0.0f};
extern CONST __declspec(selectany) XMVECTORF32 g_XMIdentityR3        = {0.0f, 0.0f, 0.0f, 1.0f};
extern CONST __declspec(selectany) XMVECTORI32 g_XMMaskX			 = {0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000};
extern CONST __declspec(selectany) XMVECTORI32 g_XMMaskY             = {0x00000000, 0xFFFFFFFF, 0x00000000, 0x00000000};
extern CONST __declspec(selectany) XMVECTORI32 g_XMMaskZ             = {0x00000000, 0x00000000, 0xFFFFFFFF, 0x00000000};
extern CONST __declspec(selectany) XMVECTORF32 g_XMOne               = { 1.0f, 1.0f, 1.0f, 1.0f};
extern CONST __declspec(selectany) XMVECTORF32 g_XMZero              = { 0.0f, 0.0f, 0.0f, 0.0f};
extern CONST __declspec(selectany) XMVECTORF32 g_XMNegateX           = {-1.0f, 1.0f, 1.0f, 1.0f};


//------------------------------------------------------------------------------
// Initialize a vector with four floating point values
XMFINLINE XMVECTOR XMVectorSet
(
    FLOAT x, 
    FLOAT y, 
    FLOAT z, 
    FLOAT w
)
{
    return _mm_set_ps( w, z, y, x );
}


XMFINLINE XMMATRIX XMMatrixScalingFromVector
(
    FXMVECTOR Scale
)
{
    XMMATRIX M;
    M.r[0] = _mm_and_ps(Scale, g_XMMaskX.v);
    M.r[1] = _mm_and_ps(Scale, g_XMMaskY.v);
    M.r[2] = _mm_and_ps(Scale, g_XMMaskZ.v);
    M.r[3] = g_XMIdentityR3.v;
    return M;
}


XMINLINE XMMATRIX XMMatrixRotationZ
(
    FLOAT Angle
)
{
    FLOAT SinAngle = sinf(Angle);
    FLOAT CosAngle = cosf(Angle);
	XMMATRIX M;
    XMVECTOR vSin = _mm_set_ss(SinAngle);
    XMVECTOR vCos = _mm_set_ss(CosAngle);
    // x = cos,y = sin,z = 0, w = 0
    vCos = _mm_unpacklo_ps(vCos,vSin);
    
    M.r[0] = vCos;
    // x = sin,y = cos,z = 0, w = 0
    vCos = _mm_shuffle_ps(vCos,vCos,_MM_SHUFFLE(3,2,0,1));
    // x = cos,y = -sin,z = 0, w = 0
    vCos = _mm_mul_ps(vCos,g_XMNegateX.v);
    M.r[1] = vCos;
    M.r[2] = g_XMIdentityR2.v;
    M.r[3] = g_XMIdentityR3.v;
    return M;
}


// Perform a 4x4 matrix multiply by a 4x4 matrix
XMFINLINE XMMATRIX XMMatrixMultiply
(
	XMMATRIX *M1, 
    XMMATRIX *M2
)
{
    XMMATRIX mResult;
    // Use vW to hold the original row
    XMVECTOR vW = M1->r[0];
    // Splat the component X,Y,Z then W
    XMVECTOR vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    XMVECTOR vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    XMVECTOR vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    // Perform the opertion on the first row
    vX = _mm_mul_ps(vX,M2->r[0]);
    vY = _mm_mul_ps(vY,M2->r[1]);
    vZ = _mm_mul_ps(vZ,M2->r[2]);
    vW = _mm_mul_ps(vW,M2->r[3]);
    // Perform a binary add to reduce cumulative errors
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[0] = vX;
    // Repeat for the other 3 rows
    vW = M1->r[1];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,M2->r[0]);
    vY = _mm_mul_ps(vY,M2->r[1]);
    vZ = _mm_mul_ps(vZ,M2->r[2]);
    vW = _mm_mul_ps(vW,M2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[1] = vX;
    vW = M1->r[2];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,M2->r[0]);
    vY = _mm_mul_ps(vY,M2->r[1]);
    vZ = _mm_mul_ps(vZ,M2->r[2]);
    vW = _mm_mul_ps(vW,M2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[2] = vX;
    vW = M1->r[3];
    vX = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(0,0,0,0));
    vY = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(1,1,1,1));
    vZ = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(2,2,2,2));
    vW = _mm_shuffle_ps(vW,vW,_MM_SHUFFLE(3,3,3,3));
    vX = _mm_mul_ps(vX,M2->r[0]);
    vY = _mm_mul_ps(vY,M2->r[1]);
    vZ = _mm_mul_ps(vZ,M2->r[2]);
    vW = _mm_mul_ps(vW,M2->r[3]);
    vX = _mm_add_ps(vX,vZ);
    vY = _mm_add_ps(vY,vW);
    vX = _mm_add_ps(vX,vY);
    mResult.r[3] = vX;
    return mResult;
}


XMINLINE XMMATRIX XMMatrixAffineTransformation2D
(
    FXMVECTOR Scaling, 
    FXMVECTOR RotationOrigin, 
    FLOAT    Rotation, 
    FXMVECTOR Translation
)
{
    XMMATRIX M;
    XMVECTOR VScaling;
    XMMATRIX MScaling;
    XMVECTOR VRotationOrigin;
    XMMATRIX MRotation;
    XMVECTOR VTranslation;
    static const XMVECTORU32 Mask2 = {0xFFFFFFFFU,0xFFFFFFFFU,0,0};
    static const XMVECTORF32 ZW1 = {0,0,1.0f,1.0f};

    // M = MScaling * Inverse(MRotationOrigin) * MRotation * MRotationOrigin * MTranslation;

    VScaling = _mm_and_ps(Scaling, Mask2.v);
    VScaling = _mm_or_ps(VScaling, ZW1.v);
    MScaling = XMMatrixScalingFromVector(VScaling);
    VRotationOrigin = _mm_and_ps(RotationOrigin, Mask2.v);
    MRotation = XMMatrixRotationZ(Rotation);
    VTranslation = _mm_and_ps(Translation, Mask2.v);

    M      = MScaling;
    M.r[3] = _mm_sub_ps(M.r[3], VRotationOrigin);
    M      = XMMatrixMultiply(&M, &MRotation);
    M.r[3] = _mm_add_ps(M.r[3], VRotationOrigin);
    M.r[3] = _mm_add_ps(M.r[3], VTranslation);
	return M;
}


XMFINLINE XMVECTOR XMLoadFloat3
(
    CONST XMFLOAT3* pSource
)
{	
    __m128 x = _mm_load_ss( &pSource->x );
    __m128 y = _mm_load_ss( &pSource->y );
    __m128 z = _mm_load_ss( &pSource->z );
    __m128 xy = _mm_unpacklo_ps( x, y );
    return _mm_movelh_ps( xy, z );
}


XMFINLINE XMVECTOR XMVector3TransformCoord
(
    FXMVECTOR V, 
    XMMATRIX *M
)
{
	XMVECTOR vTemp;
    XMVECTOR vResult = _mm_shuffle_ps(V,V,_MM_SHUFFLE(0,0,0,0));
    vResult = _mm_mul_ps(vResult,M->r[0]);
    vTemp = _mm_shuffle_ps(V,V,_MM_SHUFFLE(1,1,1,1));
    vTemp = _mm_mul_ps(vTemp,M->r[1]);
    vResult = _mm_add_ps(vResult,vTemp);
    vTemp = _mm_shuffle_ps(V,V,_MM_SHUFFLE(2,2,2,2));
    vTemp = _mm_mul_ps(vTemp,M->r[2]);
    vResult = _mm_add_ps(vResult,vTemp);
    vResult = _mm_add_ps(vResult,M->r[3]);
    vTemp = _mm_shuffle_ps(vResult,vResult,_MM_SHUFFLE(3,3,3,3));
    vResult = _mm_div_ps(vResult,vTemp);
    return vResult;
}


XMFINLINE VOID XMStoreFloat3
(
    XMFLOAT3* pDestination, 
    FXMVECTOR V
)
{
    XMVECTOR T1 = _mm_shuffle_ps(V,V,_MM_SHUFFLE(1,1,1,1));
    XMVECTOR T2 = _mm_shuffle_ps(V,V,_MM_SHUFFLE(2,2,2,2));
    _mm_store_ss( &pDestination->x, V );
    _mm_store_ss( &pDestination->y, T1 );
    _mm_store_ss( &pDestination->z, T2 );
}


XMFINLINE VOID XMStoreColor
(
    XMCOLOR* pDestination, 
    FXMVECTOR V
)
{
	__m128i vInt;

    static CONST XMVECTORF32  Scale = {255.0f,255.0f,255.0f,255.0f};
    // Set <0 to 0
    XMVECTOR vResult = _mm_max_ps(V,g_XMZero.v);
    // Set>1 to 1
    vResult = _mm_min_ps(vResult,g_XMOne.v);
    // Convert to 0-255
    vResult = _mm_mul_ps(vResult,Scale.v);
    // Shuffle RGBA to ARGB
    vResult = _mm_shuffle_ps(vResult,vResult,_MM_SHUFFLE(3,0,1,2));
    // Convert to int 
    vInt = _mm_cvtps_epi32(vResult);
    // Mash to shorts
    vInt = _mm_packs_epi32(vInt,vInt);
    // Mash to bytes
    vInt = _mm_packus_epi16(vInt,vInt);
    // Store the color
    _mm_store_ss((float*)(&pDestination->c), _mm_castsi128_ps(vInt));
}