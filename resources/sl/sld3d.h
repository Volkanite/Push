#ifndef D3D_H
#define D3D_H


typedef struct _D3DVECTOR {
    FLOAT x;
    FLOAT y;
    FLOAT z;
} D3DVECTOR;


typedef struct _D3DCOLORVALUE {
    FLOAT r;
    FLOAT g;
    FLOAT b;
    FLOAT a;
} D3DCOLORVALUE;


typedef struct _D3DRECT {
    LONG x1;
    LONG y1;
    LONG x2;
    LONG y2;
} D3DRECT;


typedef struct _D3DMATRIX {
    union {
        struct {
            FLOAT        _11, _12, _13, _14;
            FLOAT        _21, _22, _23, _24;
            FLOAT        _31, _32, _33, _34;
            FLOAT        _41, _42, _43, _44;

        };
        FLOAT m[4][4];
    };
} D3DMATRIX;


typedef struct _D3DDISPLAYMODE
{
    UINT32 Width;
    UINT32 Height;
    UINT32 RefreshRate;
    UINT32 Format;
} D3DDISPLAYMODE;


typedef struct _D3DVIEWPORT {
    UINT32      X;
    UINT32      Y;     
    UINT32      Width;
    UINT32      Height;
    FLOAT       MinZ;  
    FLOAT       MaxZ;
} D3DVIEWPORT;


typedef struct _D3DMATERIAL {
    D3DCOLORVALUE   Diffuse;        /* Diffuse color RGBA */
    D3DCOLORVALUE   Ambient;        /* Ambient color RGB */
    D3DCOLORVALUE   Specular;       /* Specular 'shininess' */
    D3DCOLORVALUE   Emissive;       /* Emissive color RGB */
    FLOAT           Power;          /* Sharpness if specular highlight */
} D3DMATERIAL;


typedef struct _D3DLIGHT {
    UINT32          Type;        
    D3DCOLORVALUE   Diffuse;     
    D3DCOLORVALUE   Specular;    
    D3DCOLORVALUE   Ambient;     
    D3DVECTOR       Position;    
    D3DVECTOR       Direction;   
    FLOAT           Range;       
    FLOAT           Falloff;     
    FLOAT           Attenuation0;
    FLOAT           Attenuation1;
    FLOAT           Attenuation2;
    FLOAT           Theta;       
    FLOAT           Phi;         
} D3DLIGHT;


typedef struct _D3DCLIPSTATUS {
    DWORD ClipUnion;
    DWORD ClipIntersection;
} D3DCLIPSTATUS;


typedef struct _D3DDEVICE_CREATION_PARAMETERS
{
    UINT32          AdapterOrdinal;
    UINT32          DeviceType;
    VOID*           hFocusWindow;
    DWORD           BehaviorFlags;
} D3DDEVICE_CREATION_PARAMETERS;


typedef struct _D3DRASTER_STATUS
{
    INTBOOL            InVBlank;
    UINT32          ScanLine;
} D3DRASTER_STATUS;


typedef struct _D3DGAMMARAMP
{
    WORD                red  [256];
    WORD                green[256];
    WORD                blue [256];
} D3DGAMMARAMP;


typedef struct _D3DRECTPATCH_INFO
{
    UINT32        StartVertexOffsetWidth;
    UINT32        StartVertexOffsetHeight;
    UINT32        Width;
    UINT32        Height;
    UINT32        Stride;
    UINT32        Basis;
    UINT32        Order;
} D3DRECTPATCH_INFO;


typedef struct _D3DTRIPATCH_INFO
{
    UINT32        StartVertexOffset;
    UINT32        NumVertices;
    UINT32        Basis;
    UINT32        Order;
} D3DTRIPATCH_INFO;


typedef struct _D3DXFONT_DESCA
{
    INT32   Height;
    UINT32  Width;
    UINT32  Weight;
    UINT32  MipLevels;
    INTBOOL     Italic;
    BYTE    CharSet;
    BYTE    OutputPrecision;
    BYTE    Quality;
    BYTE    PitchAndFamily;
    CHAR    FaceName[32];

} D3DXFONT_DESCA;


typedef struct _D3DXFONT_DESCW
{
    INT32   Height;
    UINT32  Width;
    UINT32  Weight;
    UINT32  MipLevels;
    INTBOOL     Italic;
    BYTE    CharSet;
    BYTE    OutputPrecision;
    BYTE    Quality;
    BYTE    PitchAndFamily;
    WCHAR   FaceName[32];

} D3DXFONT_DESCW;


typedef enum D3D_USAGE { 
  D3D_USAGE_DEFAULT    = 0,
  D3D_USAGE_IMMUTABLE  = 1,
  D3D_USAGE_DYNAMIC    = 2,
  D3D_USAGE_STAGING    = 3
} D3D_USAGE;


typedef struct D3D_SAMPLE_DESC {
  UINT32 Count;
  UINT32 Quality;
} D3D_SAMPLE_DESC;


typedef struct D3D_TEXTURE2D_DESC {
  UINT32             Width;
  UINT32             Height;
  UINT32             MipLevels;
  UINT32             ArraySize;
  UINT32           Format;
  D3D_SAMPLE_DESC  SampleDesc;
  D3D_USAGE        Usage;
  UINT32             BindFlags;
  UINT32             CPUAccessFlags;
  UINT32             MiscFlags;
} D3D_TEXTURE2D_DESC;


typedef struct D3D_SUBRESOURCE_DATA {
  const void *pSysMem;
  UINT32       SysMemPitch;
  UINT32       SysMemSlicePitch;
} D3D_SUBRESOURCE_DATA;


typedef struct D3D_TEX2D_SRV {
  UINT32 MostDetailedMip;
  UINT32 MipLevels;
} D3D_TEX2D_SRV;


typedef struct D3D_SHADER_RESOURCE_VIEW_DESC
{
    UINT32          Format;
    UINT32          ViewDimension;
    D3D_TEX2D_SRV   Texture2D;
    UINT64 filler;

} D3D_SHADER_RESOURCE_VIEW_DESC; //24 bytes


typedef struct D3D_PASS_DESC {
  CHAR*         Name;
  UINT32        Annotations;
  BYTE*         pIAInputSignature;
  UINT_B  IAInputSignatureSize;
  UINT32        StencilRef;
  UINT32        SampleMask;
  FLOAT         BlendFactor[4];
} D3D_PASS_DESC;


typedef struct D3D_INPUT_ELEMENT_DESC {
  CHAR*                     SemanticName;
  UINT32                       SemanticIndex;
  UINT32                Format;
  UINT32                       InputSlot;
  UINT32                       AlignedByteOffset;
  UINT32                        InputSlotClass;
  UINT32                       InstanceDataStepRate;
} D3D_INPUT_ELEMENT_DESC;



#endif //D3D_H