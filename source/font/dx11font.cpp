#include <Windows.h>
#include <D3D11.h>
#include <slgdi.h>
#include <stdio.h>


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;
#include "dx11font.h"
#include <d3dx11effect.h>
#include "..\d3dcompiler.h"
#include "effects11.h"


typedef INT32 (__stdcall *D3DCompile_t)(
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


enum
{
    STYLE_NORMAL      = 0,
    STYLE_BOLD        = 1,
    STYLE_ITALIC      = 2,
    STYLE_BOLD_ITALIC = 3,
    STYLE_UNDERLINE   = 4,
    STYLE_STRIKEOUT   = 8
};


ID3D11Device*							device;
ID3D11DeviceContext*					FontDeviceContext;
ID3D11Device*							m_device11;
ID3D11InputLayout*						inputLayout;
ID3D11Buffer*							VB;
ID3D11Buffer*							IB;
ID3D11ShaderResourceView*				BatchTexSRV;
ID3D11ShaderResourceView*				shaderResourceView;
ID3D11BlendState*						TransparentBS;
ID3D11Texture2D*						m_texture11;
ID3D11Texture2D*						m_texturepass;
ID3D10Texture2D*						m_texture10;
ID3DX11EffectShaderResourceVariable*	D3D11Font_EffectShaderResourceVariable = NULL;
ID3DX11EffectPass*						effectPass;
D3DCompile_t							FntD3DCompile;
BOOLEAN									Initialized;
int										posX;
int										posY;


#define START_CHAR 33




extern "C" VOID* __stdcall RtlAllocateHeap(
    VOID*           HeapHandle,
    DWORD           Flags,
    SIZE_T    Size
    );
extern "C" BOOL __stdcall RtlFreeHeap(
    VOID* HeapHandle,
    DWORD Flags,
    VOID* HeapBase
    );


VOID
Init()
{
    m_device11      = 0;
    FontDeviceContext       = 0;
    Initialized     = 0;
    VB              = 0;
    IB              = 0;
    inputLayout     = 0;
    BatchTexSRV     = 0 ;
}


BYTE
GetAlpha( DWORD Argb )
{
    return (BYTE) (Argb >> AlphaShift);
}


INT32 GetCharMinX( GpBitmap *bitmap )
{
    UINT width, height;
    UINT x, y;

    GdipGetImageWidth( (GpImage*) bitmap, &width );
    GdipGetImageHeight( (GpImage*) bitmap, &height );

    for( x = 0; x < width; ++x )
    {
        for( y = 0; y < height; ++y )
        {
            DWORD color;

            GdipBitmapGetPixel(bitmap, x, y, &color);

            if ( GetAlpha(color) > 0 )
                 return x;
        }
    }

    return 0;
}


INT32 GetCharMaxX( GpBitmap *bitmap )
{
    UINT width, height;
    UINT x, y;

    GdipGetImageWidth( (GpImage*) bitmap, &width);
    GdipGetImageHeight( (GpImage*) bitmap, &height);

    for( x = width - 1; x >= 0; --x )
    {
        for( y = 0; y < height; ++y )
        {
            DWORD color;

            GdipBitmapGetPixel(bitmap, x, y, &color);

            if ( GetAlpha(color) > 0 )
                 return x;
        }
    }

    return width - 1;
}


BOOLEAN
Dx11Font::InitD3D11Sprite( )
{
    WORD indices[3072];
    UINT16 i;
    HRESULT hr;
    ID3D10Blob *    compiledFx = 0, * ErrorMsgs = 0;
    D3D11_SUBRESOURCE_DATA indexData = { 0 };
    ID3DX11Effect *effect = NULL;
    ID3DX11EffectTechnique* effectTechnique;
    ID3DX11EffectVariable *effectVariable;

    D3DX11_PASS_DESC passDesc;
    D3D11_BUFFER_DESC vbd;
    D3D11_BUFFER_DESC ibd;
	char *profileStr;

    D3D11_INPUT_ELEMENT_DESC layoutDesc[ ] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    CHAR effectFile[ ] = \
        "Texture2D SpriteTex;"
        "SamplerState samLinear {"
        "     Filter = MIN_MAG_MIP_LINEAR;"
        "     AddressU = WRAP;"
        "     AddressV = WRAP;"
        "};"
        "struct VertexIn {"
        "     float3 PosNdc : POSITION;"
        "     float2 Tex    : TEXCOORD;"
        "     float4 Color  : COLOR;"
        "};"
        "struct VertexOut {"
        "     float4 PosNdc : SV_POSITION;"
        "     float2 Tex    : TEXCOORD;"
        "     float4 Color  : COLOR;"
        "};"
        "VertexOut VS(VertexIn vin) {"
        "     VertexOut vout;"
        "     vout.PosNdc = float4(vin.PosNdc, 1.0f);"
        "     vout.Tex    = vin.Tex;"
        "     vout.Color  = vin.Color;"
        "     return vout;"
        "};"
        "float4 PS(VertexOut pin) : SV_Target {"
        "     return pin.Color*SpriteTex.Sample(samLinear, pin.Tex);"
        "};"
        "technique11 SpriteTech {"
        "     pass P0 {"
        "         SetVertexShader( CompileShader( vs_5_0, VS() ) );"
        "         SetHullShader( NULL );"
        "         SetDomainShader( NULL );"
        "         SetGeometryShader( NULL );"
        "         SetPixelShader( CompileShader( ps_5_0, PS() ) );"
        "     }"
        "}";

	FntD3DCompile = (D3DCompile_t)GetProcAddress(GetD3DCompiler(), "D3DCompile");

	switch (device->GetFeatureLevel()) 
	{
	case D3D_FEATURE_LEVEL_9_1:
		profileStr = "fx_4_0_level_9_1";
		break;
	case D3D_FEATURE_LEVEL_9_2:
		profileStr = "fx_4_0_level_9_2";
		break;
	case D3D_FEATURE_LEVEL_9_3:
		profileStr = "fx_4_0_level_9_3";
		break;
	case D3D_FEATURE_LEVEL_10_0:
		profileStr = "fx_4_0";
		break;
	case D3D_FEATURE_LEVEL_10_1:
		profileStr = "fx_4_1";
		break;
	case D3D_FEATURE_LEVEL_11_0:
		profileStr = "fx_5_0";
		break;
	}

    FntD3DCompile(
        effectFile, 
        strlen( effectFile ), 
        0, 
        0, 
        0, 
        "SpriteTech", 
		profileStr,
		0,
        0, 
        &compiledFx, 
        &ErrorMsgs 
        );

    // Create the UI effect object
    hr = D3DX11CreateEffectFromMemory(
        compiledFx->GetBufferPointer(),
        compiledFx->GetBufferSize(),
        0,
        m_device11,
        &effect
        );

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] D3DX11CreateEffectFromMemory failed!");
        return FALSE;
    }

    compiledFx->Release();

    effectTechnique = effect->GetTechniqueByName("SpriteTech");
    effectVariable = effect->GetVariableByName("SpriteTex");
	D3D11Font_EffectShaderResourceVariable = effectVariable->AsShaderResource();
    effectPass = effectTechnique->GetPassByIndex(0);

    effectPass->GetDesc(&passDesc);

    hr = device->CreateInputLayout(
            layoutDesc,
            sizeof( layoutDesc ) / sizeof( layoutDesc[ 0 ] ),
            passDesc.pIAInputSignature,
            passDesc.IAInputSignatureSize,
            &inputLayout
            );

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] ID3D11Device::CreateInputLayout failed!");
        return FALSE;
    }

    vbd.ByteWidth            = 2048 * sizeof( SpriteVertex );
    vbd.Usage                = D3D11_USAGE_DYNAMIC;
    vbd.BindFlags            = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags        = D3D11_CPU_ACCESS_WRITE;
    vbd.MiscFlags            = 0;
    vbd.StructureByteStride = 0;

    device->CreateBuffer( &vbd, 0, &VB );

    ibd.ByteWidth            = 3072 * sizeof( WORD );
    ibd.Usage                = D3D11_USAGE_IMMUTABLE;
    ibd.BindFlags            = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags        = 0;
    ibd.MiscFlags            = 0;
    ibd.StructureByteStride = 0;

    for( i = 0; i < 512; ++i )
    {
        indices[ i * 6 ]     = i * 4;
        indices[ i * 6 + 1 ] = i * 4 + 1;
        indices[ i * 6 + 2 ] = i * 4 + 2;
        indices[ i * 6 + 3 ] = i * 4;
        indices[ i * 6 + 4 ] = i * 4 + 2;
        indices[ i * 6 + 5 ] = i * 4 + 3;
    }

    indexData.pSysMem = &indices[ 0 ];

    device->CreateBuffer( &ibd, &indexData, &IB );

    D3D11_BLEND_DESC transparentDesc = { 0 };

    transparentDesc.AlphaToCoverageEnable   = FALSE;
    transparentDesc.IndependentBlendEnable  = FALSE;

    transparentDesc.RenderTarget[ 0 ].BlendEnable           = TRUE;
    transparentDesc.RenderTarget[ 0 ].SrcBlend              = D3D11_BLEND_SRC_ALPHA;
    transparentDesc.RenderTarget[ 0 ].DestBlend             = D3D11_BLEND_INV_SRC_ALPHA;
    transparentDesc.RenderTarget[ 0 ].BlendOp               = D3D11_BLEND_OP_ADD;
    transparentDesc.RenderTarget[ 0 ].SrcBlendAlpha         = D3D11_BLEND_ONE;
    transparentDesc.RenderTarget[ 0 ].DestBlendAlpha        = D3D11_BLEND_ZERO;
    transparentDesc.RenderTarget[ 0 ].BlendOpAlpha          = D3D11_BLEND_OP_ADD;
    transparentDesc.RenderTarget[ 0 ].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    device->CreateBlendState( &transparentDesc, &TransparentBS );

    HeapHandle = GetProcessHeap();
    Sprites = (Sprite*) RtlAllocateHeap(HeapHandle, 0, sizeof(Sprite));
    Initialized = TRUE;

    return TRUE;
}


Dx11Font::Dx11Font( ID3D11Device *Device )
{
    ID3D11Device *device11 = Device;

    VOID **vmt;

    Init();

   m_device11 = device11;
   device = device11;

   vmt = (VOID**) device11;
   vmt = (VOID**) vmt[0];

   device11->GetImmediateContext( &FontDeviceContext );

   vmt = (VOID**) FontDeviceContext;
   vmt = (VOID**) vmt[0];

   InitDeviceObjects();
   InitD3D11Sprite();
}


VOID
Dx11Font::Draw( RECT* destinationRect, RECT* sourceRect, XMCOLOR color )
{
    Sprite sprite;
    sprite.SrcRect  = *sourceRect;
    sprite.DestRect = *destinationRect;
    sprite.Color    = color;
    sprite.Z        = 0.0f;
    sprite.Angle    = 0.0f;
    sprite.Scale    = 1.0f;

    AddSprite( &sprite );
}


VOID Dx11Font::AddString( WCHAR *text, DWORD Color )
{
    UINT i, length = (UINT)wcslen(text);
    int xbackup = posX;
    XMCOLOR color;

    color.c = Color;

    for( i = 0; i < length; ++i )
    {
        WCHAR character = text[ i ];

        if (character == ' ')
            posX += m_dwSpacing;

        else if( character == '\n' )
        {
            posX  = xbackup;
            posY += m_dwFontHeight;
        }
        else
        {
            RECT charRect;
            
            charRect.left   = (LONG)((m_fTexCoords[character-32][0]) * m_dwTexWidth) + m_dwSpacing;
            charRect.top    = (LONG)(m_fTexCoords[character-32][1]) * m_dwTexWidth;
            charRect.right  = (LONG)((m_fTexCoords[character-32][2]) * m_dwTexWidth) - m_dwSpacing;
            charRect.bottom = (LONG)(m_fTexCoords[character-32][3]) * m_dwTexWidth;

            int width = charRect.right - charRect.left;
            int height = charRect.bottom - charRect.top;
            RECT rect = {posX, posY, posX + width, posY + height};
            
            Draw( &rect, &charRect, color);

            posX += width + 1;
      }
   }
}


VOID Dx11Font::DrawText( FLOAT sx, FLOAT sy, DWORD Color, WCHAR* Text )
{
    posX = (int)sx;
    posY = (int)sy;

    AddString(Text, Color);
}


VOID 
Dx11Font::BeginBatch( 
    ID3D11ShaderResourceView* texSRV 
    )
{
    ID3D11Resource * resource = 0;
    ID3D11Texture2D * tex;
    D3D11_TEXTURE2D_DESC texDesc;

    BatchTexSRV = texSRV;

    BatchTexSRV->AddRef();
    BatchTexSRV->GetResource(&resource);

    tex = (ID3D11Texture2D *) resource;

    tex->GetDesc(&texDesc);
    tex->Release();

    m_dwTexWidth  = texDesc.Width;
    m_dwTexHeight = texDesc.Height;
}


VOID
Dx11Font::DrawBatch( 
    UINT startSpriteIndex, 
    UINT spriteCount 
    )
{
    D3D11_MAPPED_SUBRESOURCE mappedData;
    SpriteVertex * v;
    UINT i;

    if (m_device11)
    {
        FontDeviceContext->Map(
            VB,
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &mappedData
            );
    }

    v = (SpriteVertex*) mappedData.pData;

    for( i = 0; i < spriteCount; ++i )
    {
        Sprite *sprite = &Sprites[ startSpriteIndex + i ];

        SpriteVertex quad[ 4 ];

        BuildSpriteQuad( sprite, quad );

        v[ i * 4 ]        = quad[ 0 ];
        v[ i * 4 + 1 ]    = quad[ 1 ];
        v[ i * 4 + 2 ]    = quad[ 2 ];
        v[ i * 4 + 3 ]    = quad[ 3 ];
    }

    FontDeviceContext->Unmap(VB, 0);
    FontDeviceContext->DrawIndexed( spriteCount * 6, 0, 0 );
}


VOID Dx11Font::EndBatch( )
{
    UINT viewportCount = 1, stride, offset, spritesToDraw;
    UINT startIndex;
    D3D11_VIEWPORT vp;

    FontDeviceContext->RSGetViewports(&viewportCount, &vp);

    ScreenWidth  = vp.Width;
    ScreenHeight = vp.Height;

    stride = sizeof( SpriteVertex );
    offset = 0;

    FontDeviceContext->IASetInputLayout( inputLayout );
    FontDeviceContext->IASetIndexBuffer(IB, DXGI_FORMAT_R16_UINT, 0 );
    FontDeviceContext->IASetVertexBuffers(0, 1, &VB, &stride, &offset );
    FontDeviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    
	if (!D3D11Font_EffectShaderResourceVariable)
		return;

	D3D11Font_EffectShaderResourceVariable->SetResource(BatchTexSRV);
    effectPass->Apply(0, FontDeviceContext);
    
    spritesToDraw = NumberOfSprites;
    startIndex = 0;

    while( spritesToDraw > 0 )
    {
        if( spritesToDraw <= 512 )
        {
            DrawBatch( startIndex, spritesToDraw );
            spritesToDraw = 0;
        }
        else
        {
            DrawBatch( startIndex, 512 );
            startIndex += 512;
            spritesToDraw -= 512;
        }
    }

    BatchTexSRV->Release();
}


VOID
Dx11Font::DrawString()
{
    float blendFactor[ 4 ] = { 1.0f };

    FontDeviceContext->OMSetBlendState( TransparentBS, blendFactor, 0xFFFFFFFF );
    BeginBatch( shaderResourceView );
    EndBatch( );
    FontDeviceContext->OMSetBlendState( 0, blendFactor, 0xFFFFFFFF );
}


VOID
Dx11Font::Begin()
{
    NumberOfSprites = 0;
}


VOID
Dx11Font::End()
{
   DrawString();
}


DWORD
Dx11Font::GetMaxTextureWidth()
{
    return 2048;
}


HRESULT
Dx11Font::CreateTexture()
{
    D3D11_TEXTURE2D_DESC texDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;

    texDesc.Width               = m_dwTexWidth;
    texDesc.Height              = m_dwTexHeight;
    texDesc.MipLevels           = 1;
    texDesc.ArraySize           = 1;
    texDesc.Format              = DXGI_FORMAT_B8G8R8A8_UNORM;
    texDesc.Usage               = D3D11_USAGE_DYNAMIC;
    texDesc.BindFlags           = D3D11_BIND_SHADER_RESOURCE | D3D10_BIND_SHADER_RESOURCE;
    texDesc.CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE;
    texDesc.MiscFlags           = 0;
    texDesc.SampleDesc.Count    = 1;
    texDesc.SampleDesc.Quality  = 0;

    device->CreateTexture2D( &texDesc, NULL, &m_texture11 );

    srvDesc.Format                      = DXGI_FORMAT_B8G8R8A8_UNORM;
    srvDesc.ViewDimension               = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels         = 1;
    srvDesc.Texture2D.MostDetailedMip   = 0;

    device->CreateShaderResourceView( m_texture11, &srvDesc, &shaderResourceView );

    return S_OK;
}


VOID
Dx11Font::LockTexture( D3DLOCKED_RECT *LockedRect )
{
    D3D11_MAPPED_SUBRESOURCE mappedData;

    FontDeviceContext->Map(m_texture11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

    LockedRect->pBits = mappedData.pData;
    LockedRect->Pitch = m_dwTexWidth * 4;
}


VOID
Dx11Font::UnlockTexture()
{
    FontDeviceContext->Unmap(m_texture11, 0);
}



