#include <Windows.h>
#include <D3D11.h>
#include <D3DX11async.h>
#include <slgdi.h>
#include <stdio.h>


typedef struct _D3DLOCKED_RECT
{
    INT                 Pitch;
    void*               pBits;
} D3DLOCKED_RECT;
#include "d3d11font.h"
#include "shaders.h"

#define SAFE_RELEASE(x) if (x) { x->Release(); x = 0; }

enum
{
    STYLE_NORMAL      = 0,
    STYLE_BOLD        = 1,
    STYLE_ITALIC      = 2,
    STYLE_BOLD_ITALIC = 3,
    STYLE_UNDERLINE   = 4,
    STYLE_STRIKEOUT   = 8
};


ID3D11Device*                           device;
ID3D11DeviceContext*                    deviceContext;
ID3D11Device*                           m_device11;
ID3D11InputLayout*                      inputLayout;
ID3D11Buffer*                           VB;
ID3D11Buffer*                           IB;
ID3D11ShaderResourceView*               BatchTexSRV;
ID3D11ShaderResourceView*               shaderResourceView;
ID3D11BlendState*                       TransparentBS;
ID3D11Texture2D*                        m_texture11;
ID3D11Texture2D*                        m_texturepass;
ID3D10Texture2D*                        m_texture10;
ID3D11VertexShader*                     D3D11Font_VertexShader = NULL;
ID3D11PixelShader*                      D3D11Font_PixelShader = NULL;
ID3D11RasterizerState*                  RasterizerState;
ID3D11RenderTargetView  *RenderTarget;
extern ID3D11Texture2D *BackBuffer;
extern D3D11_TEXTURE2D_DESC rtv_desc;

BOOLEAN                                 Initialized;


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


VOID Init()
{
    m_device11      = 0;
    deviceContext       = 0;
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


BOOLEAN Dx11Font::InitD3D11Sprite( )
{
    WORD indices[3072];
    UINT16 i;
    HRESULT hr;
    ID3D10Blob *vertexShaderBlob = NULL;
    ID3D10Blob *pixelShaderBlob = NULL;
    D3D11_SUBRESOURCE_DATA indexData = { 0 };
    D3D11_BUFFER_DESC vbd;
    D3D11_BUFFER_DESC ibd;
    char *vertexShaderModel;
    char *pixelShaderModel;

    switch (device->GetFeatureLevel())
    {
    case D3D_FEATURE_LEVEL_9_1:
        vertexShaderModel = "vs_4_0_level_9_1";
        pixelShaderModel  = "ps_4_0_level_9_1";
        break;
    case D3D_FEATURE_LEVEL_9_2:
        vertexShaderModel = "vs_4_0_level_9_2";
        pixelShaderModel  = "ps_4_0_level_9_2";
        break;
    case D3D_FEATURE_LEVEL_9_3:
        vertexShaderModel = "vs_4_0_level_9_3";
        pixelShaderModel  = "ps_4_0_level_9_3";
        break;
    case D3D_FEATURE_LEVEL_10_0:
        vertexShaderModel = "vs_4_0";
        pixelShaderModel  = "ps_4_0";
        break;
    case D3D_FEATURE_LEVEL_10_1:
        vertexShaderModel = "vs_4_1";
        pixelShaderModel  = "ps_4_1";
        break;
    case D3D_FEATURE_LEVEL_11_0:
        vertexShaderModel = "vs_5_0";
        pixelShaderModel  = "ps_5_0";
        break;
    }

    CompileShader(
        D3DXFont_VertexShaderData,
        sizeof(D3DXFont_VertexShaderData),
        "VS",
        vertexShaderModel,
        (void**)&vertexShaderBlob
        );

    device->CreateVertexShader(
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        NULL,
        &D3D11Font_VertexShader
        );

    D3D11_INPUT_ELEMENT_DESC layoutDesc[ ] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 }
    };

    UINT numElements = ARRAYSIZE(layoutDesc);

    hr = device->CreateInputLayout(
        layoutDesc,
        numElements,
        vertexShaderBlob->GetBufferPointer(),
        vertexShaderBlob->GetBufferSize(),
        &inputLayout
        );

    if (FAILED(hr))
    {
        OutputDebugStringW(L"[OVRENDER] ID3D11Device::CreateInputLayout failed!");
        return FALSE;
    }

    vertexShaderBlob->Release();

    CompileShader(
        D3DXFont_PixelShaderData,
        sizeof(D3DXFont_PixelShaderData),
        "PS",
        pixelShaderModel,
        (void**)&pixelShaderBlob
        );

    device->CreatePixelShader(
        pixelShaderBlob->GetBufferPointer(),
        pixelShaderBlob->GetBufferSize(),
        NULL,
        &D3D11Font_PixelShader
        );

    pixelShaderBlob->Release();

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

    D3D11_RASTERIZER_DESC rasterizerDescription;

    rasterizerDescription.FillMode = D3D11_FILL_SOLID;
    rasterizerDescription.CullMode = D3D11_CULL_NONE;
    rasterizerDescription.FrontCounterClockwise = false;
    rasterizerDescription.DepthBias = 0;
    rasterizerDescription.DepthBiasClamp = 0.f;
    rasterizerDescription.SlopeScaledDepthBias = 0.f;
    rasterizerDescription.DepthClipEnable = false;
    rasterizerDescription.ScissorEnable = false;
    rasterizerDescription.MultisampleEnable = false;
    rasterizerDescription.AntialiasedLineEnable = false;

    device->CreateRasterizerState(&rasterizerDescription, &RasterizerState);

    HeapHandle = GetProcessHeap();
    Sprites = (Sprite*) RtlAllocateHeap(HeapHandle, 0, sizeof(Sprite));

    device->CreateRenderTargetView(BackBuffer, NULL, &RenderTarget);

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

   device11->GetImmediateContext(&deviceContext);

   vmt = (VOID**)deviceContext;
   vmt = (VOID**) vmt[0];

   InitDeviceObjects();
   InitD3D11Sprite();
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
        deviceContext->Map(
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

    deviceContext->Unmap(VB, 0);
    deviceContext->DrawIndexed(spriteCount * 6, 0, 0);
}


VOID Dx11Font::EndBatch( )
{
    UINT viewportCount = 1, stride, offset, spritesToDraw;
    UINT startIndex;
    D3D11_VIEWPORT vp;
    ID3D11Buffer* vertexBuffer;
    UINT vertexBufferStride;
    UINT vertexBufferOffset;
    ID3D11Buffer* indexBuffer;
    DXGI_FORMAT format;
    UINT indexBufferOffset;
    ID3D11InputLayout* lastInputLayout;
    D3D11_PRIMITIVE_TOPOLOGY lastTopology;
    ID3D11RasterizerState* lastRasterizerState;
    ID3D11PixelShader *pixelShader;

    deviceContext->RSGetViewports(&viewportCount, &vp);

    ScreenWidth  = vp.Width;
    ScreenHeight = vp.Height;

    stride = sizeof( SpriteVertex );
    offset = 0;

    // Save device state
    deviceContext->RSGetState(&lastRasterizerState);
    deviceContext->IAGetInputLayout(&lastInputLayout);
    deviceContext->IAGetIndexBuffer(&indexBuffer, &format, &indexBufferOffset);
    deviceContext->IAGetVertexBuffers(0, 1, &vertexBuffer, &vertexBufferStride, &vertexBufferOffset);
    deviceContext->IAGetPrimitiveTopology(&lastTopology);
    deviceContext->PSGetShader(&pixelShader, NULL, 0);

    // Set new state
    deviceContext->OMSetRenderTargets(1, &RenderTarget, NULL);
    deviceContext->RSSetState(RasterizerState);
    deviceContext->IASetInputLayout( inputLayout );
    deviceContext->IASetIndexBuffer(IB, DXGI_FORMAT_R16_UINT, 0 );
    deviceContext->IASetVertexBuffers(0, 1, &VB, &stride, &offset );
    deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    deviceContext->PSSetShaderResources(0, 1, &shaderResourceView);
    deviceContext->VSSetShader(D3D11Font_VertexShader, NULL, 0);
    deviceContext->PSSetShader(D3D11Font_PixelShader, NULL, 0);

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

    // Restore device state
    deviceContext->RSSetState(lastRasterizerState);
    deviceContext->IASetInputLayout(lastInputLayout);
    deviceContext->IASetIndexBuffer(indexBuffer, format, indexBufferOffset);
    deviceContext->IASetVertexBuffers(0, 1, &vertexBuffer, &vertexBufferStride, &vertexBufferOffset);
    deviceContext->IASetPrimitiveTopology(lastTopology);
    deviceContext->PSSetShader(pixelShader, NULL, 0);

    // Release interfaces
    SAFE_RELEASE(BatchTexSRV);
    SAFE_RELEASE(indexBuffer);
    SAFE_RELEASE(vertexBuffer);
    SAFE_RELEASE(lastInputLayout);
    SAFE_RELEASE(pixelShader);
}


VOID
Dx11Font::DrawString()
{
    float blendFactor[ 4 ] = { 1.0f };

    deviceContext->OMSetBlendState(TransparentBS, blendFactor, 0xFFFFFFFF);
    BeginBatch( shaderResourceView );
    EndBatch( );
    deviceContext->OMSetBlendState(0, blendFactor, 0xFFFFFFFF);
}


VOID Dx11Font::Begin()
{
    D3D11_VIEWPORT vp2 = { 0 };
    vp2.Width = (float)rtv_desc.Width;
    vp2.Height = (float)rtv_desc.Height;
    vp2.MaxDepth = 1.f;

    deviceContext->RSSetViewports(1, &vp2);

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


VOID Dx11Font::LockTexture( D3DLOCKED_RECT *LockedRect )
{
    D3D11_MAPPED_SUBRESOURCE mappedData;

    deviceContext->Map(m_texture11, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

    LockedRect->pBits = mappedData.pData;
    LockedRect->Pitch = m_dwTexWidth * 4;
}


VOID Dx11Font::UnlockTexture()
{
    deviceContext->Unmap(m_texture11, 0);
}



