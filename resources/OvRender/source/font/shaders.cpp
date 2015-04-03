#include <Windows.h>
#include <d3dcommon.h>
#include <stdio.h>

#define D3DCOMPILE_DEBUG                          (1 << 0)

typedef INT32(__stdcall *TYPE_D3DCompile)(
    LPCVOID pSrcData,
    SIZE_T SrcDataSize,
    LPCSTR pSourceName,
    VOID *pDefines,
    VOID *pInclude,
    LPCSTR pEntrypoint,
    LPCSTR pTarget,
    UINT Flags1,
    UINT Flags2,
    ID3DBlob** ppCode,
    ID3DBlob**ppErrorMsgs
    );

static TYPE_D3DCompile IMP_D3DCompile;


char D3DXFont_VertexShaderData[] = {
    "struct VertexIn {"
    "   float3 PosNdc : POSITION;"
    "   float2 Tex    : TEXCOORD;"
    "   float4 Color  : COLOR;"
    "};"
    "struct VertexOut {"
    "   float4 PosNdc : SV_POSITION;"
    "   float2 Tex    : TEXCOORD;"
    "   float4 Color  : COLOR;"
    "};"
    "VertexOut VS(VertexIn vin) {"
    "   VertexOut vout;"
    "   vout.PosNdc = float4(vin.PosNdc, 1.0f);"
    "   vout.Tex    = vin.Tex;"
    "   vout.Color  = vin.Color;"
    "   return vout;"
    "};"
};

char D3DXFont_PixelShaderData[] = {
    "Texture2D SpriteTex;"
    "SamplerState samLinear {"
    "     Filter = MIN_MAG_MIP_LINEAR;"
    "     AddressU = WRAP;"
    "     AddressV = WRAP;"
    "};"
    "struct VertexOut {"
    "   float4 PosNdc : SV_POSITION;"
    "   float2 Tex    : TEXCOORD;"
    "   float4 Color  : COLOR;"
    "};"
    "float4 PS(VertexOut pin) : SV_Target {"
    "   return pin.Color*SpriteTex.Sample(samLinear, pin.Tex);"
    "};"
};


HMODULE GetD3DCompiler()
{
    WCHAR buf[32];
    int i;
    HMODULE mod;

    for (i = 43; i >= 30; i--)
    {
        swprintf_s(
            buf,
            ARRAYSIZE(buf),
            L"D3DCompiler_%d.dll",
            i
            );

        mod = LoadLibraryExW(buf, NULL, NULL);

        if (mod)
            return mod;
    }

    return NULL;
}


VOID CompileShader(
    _In_ char* Data,
    _In_ SIZE_T DataLength,
    _In_ char* EntryPoint,
    _In_ char* ShaderModel,
    _Out_ ID3DBlob** Blob
    )
{
    DWORD shaderFlags = 0;
    ID3D10Blob *errorMessages = NULL;

#if defined( DEBUG ) || defined( _DEBUG )
    shaderFlags |= D3DCOMPILE_DEBUG;
#endif

    IMP_D3DCompile = (TYPE_D3DCompile)GetProcAddress(
        GetD3DCompiler(), 
        "D3DCompile"
        );

    IMP_D3DCompile(
        Data, 
        DataLength, 
        NULL, 
        NULL, 
        NULL, 
        EntryPoint, 
        ShaderModel, 
        shaderFlags, 
        0, 
        Blob, 
        &errorMessages
        );

    if (errorMessages)
    {
        char *error = (char *)errorMessages->GetBufferPointer();

        OutputDebugStringA(error);
        errorMessages->Release();
    }
}